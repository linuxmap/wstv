#include <stdio.h>
#include <stdlib.h>

#include "gb2312_codec.hh"


static int _utl_Gb2312ToUnicode(char *src, char *des)
{
	int first = 0;
	int end = MAX_UNI_INDEX - 1;
	int mid = 0;
	unsigned short gb2312, unicode = 0xFFFF;
	gb2312 = (unsigned char)src[0] << 8 | (unsigned char)src[1];

	while (first <= end)
	{
		mid = (first + end) / 2;

		if (GB_TO_UNI[mid][1] == gb2312)
		{
			unicode = GB_TO_UNI[mid][0];
			break;
		}
		else if (GB_TO_UNI[mid][1] > gb2312)
		{
			end = mid - 1;
		}
		else
		{
			first = mid + 1;
		}
	}
	if (unicode != 0xFFFF)
	{
		//printf("find unicode: 0x%x\n", unicode);
		des[0] = (unicode & 0xFF00) >> 8;
		des[1] = unicode & 0x00FF;
		return 0;
	}
	printf("_utl_Gb2312ToUnicode：failed find data, gb2312: 0x%x, src0: 0x%x, 0x%x\n", gb2312, src[0], src[1]);
	des[0] = src[0];
	des[1] = src[1];
	return -1;
}

/**
 *@brief 检查字符串，检查其结束位是否正常
 * 在编码非法时，作为字符串的结束
 *
 *
 */
int utl_iconv_gb2312_fix(char *str, int len)
{
	int ret;
	int i=0;
	char unicode[2];

	while(str && *str && i < len-2)
	{
		if( (signed char )(*str) >= 0)
		{
			str++;
			i++;
		}
		else
		{
			ret = _utl_Gb2312ToUnicode(str,unicode);
			if (ret != 0)
			{
				str[0] = '\0';
				str[1] = '\0';
				return 0;
			}
			else
			{
				str += 2;
			}
			i += 2;;
		}
	}

	if (i >= len-2)
	{
		if ((signed char )(*str) >= 0)
		{
			str[1] = '\0';
		}
		else
		{
			str[0] = '\0';
		}
	}
	return 0;
}

/**
 *@brief 将GB2312转化为UTF8编码
 *@param src GB2312编码的字符串，也可能是英文或数字
 *@param des 编码后的输出
 *@param maxLen 编码后的最大长度
 *
 *@note des的长度，必须是src的1.5倍，才能保证其不越界
 *
 *@return 长度
 */
int utl_iconv_gb2312toutf8(char *src, char *des, int maxLen)
{
	int i,j;
	int ret;
	
	char unicode[2];
	char *p = des;

	i=0;
	j=0;

	while(src && *src && j < maxLen)
	{
		if( (signed char )(*src) >= 0)
		{
			*des++ = *src++;
			j++;
		}
		else
		{
			ret = _utl_Gb2312ToUnicode(src,unicode);
			if (ret != 0)
			{
				src += 2;
				//*des++ = *src++;
				//*des++ = *src++;
				continue;
			}
			unsigned short int tmp = 0;
			//unicode to utf8
			tmp = *des++ = (0xE0 | ((unicode[1] & 0xF0) >> 4));
			tmp = *des++ = (0x80 | ((unicode[1] & 0x0F) << 2)) + ((unicode[0] & 0xC0) >> 6);
			tmp = *des++ = (0x80 | (unicode[0] & 0x3F));
			src += 2;
			j += 3;
		}
	}
	*des = '\0';
	return des-p;
}

#if 1
static int _utl_UnicodeToGB2312(char *src, char *des)
{
	int i;
	unsigned short gb2312 = 0xFFFF, unicode = 0xFFFF;
	unicode = (unsigned char)src[0] << 8 | (unsigned char)src[1];

	for(i=0;i<MAX_UNI_INDEX;i++)
	{
		if (GB_TO_UNI[i][0] == unicode)
		{
			gb2312 = GB_TO_UNI[i][1];
			break;
		}
	}

	if (gb2312 != 0xFFFF)
	{
		//printf("find unicode: 0x%x\n", unicode);
		des[0] = (gb2312 & 0xFF00) >> 8;
		des[1] = gb2312 & 0x00FF;
		return 0;
	}
	printf("_utl_UnicodeToGB2312：failed find data, gb2312: 0x%x, src0: 0x%x, 0x%x\n", gb2312, src[0], src[1]);
	des[0] = src[0];
	des[1] = src[1];
	return -1;
}
//GB_TO_UNIvoid
static void _utl_UTF8ToUnicode(char *pOut,char *pText)
{

	char* uchar = (char *)pOut;
	//printf("utf8:%x %x %x\n",pText[0],pText[1],pText[2]);
	uchar[1] = ((pText[0] & 0x0F) << 4) + ((pText[1] >> 2) & 0x0F);
	uchar[0] = ((pText[1] & 0x03) << 6) + (pText[2] & 0x3F);
	return;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/*
 *
 */
void utl_iconv_utf8togb2312(char *src, char *des, int maxLen)
{
	char Ctemp[4]={0};
	int i =0;
	int j = 0;
	int srcLen = strlen(src);
	while(i < maxLen && i < srcLen)
	{
//		printf("src[%d]=%c\n", i, src[i]);
		if((signed char )src[i] >= 0)
		{
			des[j++] = src[i++];
		}
		else
		{
			char Wtemp[2] = {0};
			_utl_UTF8ToUnicode(Wtemp,src + i);
			_utl_UnicodeToGB2312(Wtemp,Ctemp);
			des[j] = Ctemp[0];
			des[j + 1] = Ctemp[1];
			i += 3;
			j += 2;
		}
	}
	des[j] = '\0';
	return;
}

#endif

