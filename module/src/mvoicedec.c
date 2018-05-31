/*
 * mvoicedec.c
 *
 *  Created on: 2014年9月22日
 *      Author: lfx
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <jv_ao.h>

#include <jv_ai.h>
#include <utl_ifconfig.h>
#include <utl_timer.h>

#include "mvoicedec.h"
#include "maudio.h"
#include "malarmout.h"
#include "mtransmit.h"
#include "mioctrl.h"
#include "bls.h"
#include "xw_media_device.h"
#include "JvServer.h"

#define DEBUG_LEVEL 1

#define MVPrintf(dbglevel, fmt...)  \
do{\
	if(DEBUG_LEVEL >= dbglevel){	\
		printf(fmt);} \
} while(0)



struct{
	wifiap_t ap2Set;
	BOOL toset; //message from
	BOOL setting; //in thread status
	BOOL bCustom; //是否手动输入
}sWifiInfo;

#define MAX_CODE_CNT	(1024)
#define MAX_RAW_CODE_CNT	(5*(MAX_CODE_CNT))

typedef struct{
		unsigned char code[MAX_RAW_CODE_CNT];
		int cnt;
}rawCode_t;

typedef struct{
	unsigned char code[MAX_CODE_CNT];
	int cnt;
}resultCode_t;


typedef struct{
	unsigned char freqList[200]; //与上一个高点之间的距离
	int freqCnt;	//最高点出现的次数

	rawCode_t rawCode;

	BOOL bUping; // 是否在上升期
	unsigned int globalIndex;		//采样的点的索引
	unsigned int globalLastHighIndex;	//上一个波峰的索引
	unsigned int globalLastLowIndex; //上一个波谷的索引
	unsigned short lastValue; //上一个点的值

}VoiceTrans_t;


#define IS_FIRST_BIGGER(a, b) (a > ((b)+0x100))


//注意，在8K采样率的情况下，实测只能支持 [1600,2200]范围内的频率
#define FREQ_BASE 1600		//最低频率
#define FREQ_GAP 150		//频率之间的差值
#define FREQ_OFFSET ((FREQ_GAP)/2)	//频率分界点
static BOOL bVoicedecEnabled = FALSE;

unsigned int __code2Freq(unsigned char code)
{
	static unsigned int value[] = {
	};
	return value[code];
}

//一个更严格的算法
static unsigned char audio_code_freq2rawcode_ex(unsigned int freq)
{
	static unsigned int value[] = {
			1600,
			1777,//4.5//1777
			2000,//4//2000
			2285,//3.5//2285
			2666,//3//2666
			//2909,//2.8//2909
			3200,//2.5//3200
	};
#undef FREQ_OFFSET
#define FREQ_OFFSET 50
	static unsigned int offset[] = {
			(1777-1600)/2,
			(2000-1777)/2,
			(2285-2000)/2,
			(2666-2285)/2,
			//(2909-2666)/2,
			(3200-2666)/2
	};
	unsigned char code = -1;
	int j;
	for (j=0;j<sizeof(value)/sizeof(value[0]);j++)
	{
		if (freq < (value[j]+offset[j>4?4:j])*3 && freq >= (value[j]-offset[(j-1)<0?0:(j-1)])*3)
		{
			code = j;
			break;
		}
	}
	return code;
}

static unsigned char audio_code_freq2rawcode(unsigned int freq)
{
	return audio_code_freq2rawcode_ex(freq);
	static unsigned int value[] = {
#if 1
			1777 -  75,  //4.5
			2000 - ((2000-1777)/2) ,  //4
			2285 - ((2285-2000)/2),  //3.5
			2666 - ((2666-2285)/2),  //3
			2909 - ((2909-2666)/2),  //2.8
			3200 - ((3200-2909)/2),  //2.5
			3200 + 150
#else
			FREQ_BASE + FREQ_GAP*0 - FREQ_OFFSET,
			FREQ_BASE + FREQ_GAP*1 - FREQ_OFFSET,
			FREQ_BASE + FREQ_GAP*2 - FREQ_OFFSET,
			FREQ_BASE + FREQ_GAP*3 - FREQ_OFFSET,
			FREQ_BASE + FREQ_GAP*4 - FREQ_OFFSET,
			FREQ_BASE + FREQ_GAP*5 - FREQ_OFFSET, //Invalid
#endif
	};

	unsigned char code = -1;
	int j;
	for (j=0;j<sizeof(value)/sizeof(value[0]);j++)
	{
		if (freq < value[j])
		{
			code = j-1;
			break;
		}
	}
	return code;
}

typedef struct{
	short value;
	short cnt;
}VC_T;
static int audio_code_raw2code(rawCode_t *rowCode, resultCode_t *result)
{
	int i;
	VC_T vc[MAX_CODE_CNT];
	int cnt=0;

#if DEBUG_LEVEL >= 3
	for (i=0;i<rowCode->cnt;i++)
	{
		printf("%02d,", rowCode->code[i]);
	}
	printf("\n");
#endif

	vc[cnt].value = rowCode->code[0];
	vc[cnt].cnt = 1;
	for (i=1;i<rowCode->cnt;i++)
	{
		if (vc[cnt].value == rowCode->code[i])
		{
			vc[cnt].cnt++;
		}
		else
		{
			cnt++;
			vc[cnt].value = rowCode->code[i];
			vc[cnt].cnt=1;
		}
	}
	cnt++;

#if DEBUG_LEVEL >= 3
	//debug
	printf("1cnt=%d\n",cnt);
	for (i=0;i<cnt;i++)
	{
		//debug
		int j;
		for (j=0;j<vc[i].cnt;j++)
			printf("%02d,", vc[i].value);
		printf("\n");
		//after debug
	}
#endif

	//删除只有一个的点。因为它可能打断一个信号
	for (i=0;i<cnt;i++)
	{
		if (vc[i].cnt == 1)
		{
			memcpy(&vc[i], &vc[i+1], sizeof(VC_T)*(cnt-i-1));
			cnt--;
			i--;
		}
	}

	//去重，组装
	
	for (i=0;i<cnt-1;i++)
	{
		while (vc[i].value == vc[i+1].value && i < cnt-1)
		{
			vc[i].cnt += vc[i+1].cnt;
			memcpy(&vc[i+1], &vc[i+2], sizeof(VC_T)*(cnt-i-2));
			cnt--;
			i--;
		}
	}
	

#if 1
	//删除只有两个的点。因为它可能打断一个信号
	//注意，一定要分别删除一个的点，和两个的点。因为删除一个时，有可能把2个的点凑起来
	for (i=0;i<cnt;i++)
	{
		if (vc[i].cnt == 2)
		{
			memcpy(&vc[i], &vc[i+1], sizeof(VC_T)*(cnt-i-1));
			cnt--;
			i--;
		}
	}

	//去重
	for (i=0;i<cnt-1;i++)
	{
		while (vc[i].value == vc[i+1].value && i < cnt-1)
		{
			vc[i].cnt += vc[i+1].cnt;
			memcpy(&vc[i+1], &vc[i+2], sizeof(VC_T)*(cnt-i-2));
			cnt--;
			i--;
		}
	}
	
#if DEBUG_LEVEL >= 3
		//debug
		printf("5cnt=%d\n",cnt);
		for (i=0;i<cnt;i++)
		{
			//debug
			int j;
			for (j=0;j<vc[i].cnt;j++)
				printf("%02d,", vc[i].value);
			printf("\n");
			//after debug
		}
#endif
#endif

	result->cnt = 0;
	for (i=0;i<cnt;i++)
	{
		if (vc[i].cnt >= 4)
		{
			result->code[result->cnt++] = vc[i].value;
		}
	}
	return 0;
}

static int audio_code_voice2raw(VoiceTrans_t *vt, unsigned char *buffer, int len, rawCode_t *rawCode)
{
	int i;
	int alen = len/2;
	unsigned short *audio = (unsigned short *)buffer;

	for (i=0;i<alen;i++)
	{
		unsigned short curVal = audio[i] + 0x8000;

		vt->globalIndex++;
		if (vt->bUping)
		{
			//找到波峰
			if (IS_FIRST_BIGGER(vt->lastValue, curVal))
			{
//				printf("cur: %x, last: %x\n", curVal, vt->lastValue);
//				if (IS_FIRST_BIGGER(curVal, vt->lastValue))
				{
					vt->bUping = FALSE;

					int val = vt->globalIndex - vt->globalLastHighIndex;
					if (val > 0 && val <= 10)
					{
						vt->freqList[vt->freqCnt++] = val;
						//printf("vt->freqList[%d]=%d\n",vt->freqCnt,vt->freqList[vt->freqCnt]);
					}

					vt->globalLastHighIndex = vt->globalIndex;
				}
			}
		}
		else
		{
			if (IS_FIRST_BIGGER(curVal, vt->lastValue))
			{
				vt->bUping = TRUE;
				vt->globalLastLowIndex = vt->globalIndex;
			}
		}

		vt->lastValue = curVal;

		static int errCnt = 0;
		if ((vt->globalIndex%30) == 0)
		{
#define FREQ_CNT 4
			int oldfreqCnt = vt->freqCnt;
			if (oldfreqCnt >= 6)
			{
				while (vt->freqCnt >= 4)
				{
					errCnt = 0;
					MVPrintf(3, "real: %d ==> %d,%d,%d,%d,%d,%d,%d,%d ,", vt->freqCnt
							, vt->freqList[0]
							, vt->freqList[1]
							, vt->freqList[2]
							, vt->freqList[3]
							, vt->freqList[4]
							, vt->freqList[5]
							, vt->freqList[6]
							, vt->freqList[7]
							               );
					int j;
					unsigned int total = 0;
					for (j=0;j<FREQ_CNT;j++)
					{
						total += vt->freqList[j];
					}
					unsigned char code = audio_code_freq2rawcode(24000 * FREQ_CNT / total);
					if (code != 0xFF)
					{
						vt->rawCode.code[vt->rawCode.cnt++] = code;
						if(vt->rawCode.cnt >= sizeof(vt->rawCode.code))
						{
							vt->rawCode.cnt = 0;
						}
					}
					memcpy(vt->freqList, &vt->freqList[FREQ_CNT], sizeof(vt->freqList[0])*(vt->freqCnt-FREQ_CNT));
					MVPrintf(3, "oldfreqCnt: %d, global Index: %d, vt->freqCnt: %d, freq: %d\n", 
						oldfreqCnt, vt->globalIndex, vt->freqCnt, 24000 * FREQ_CNT / total );

					vt->freqCnt -= FREQ_CNT;
				}

			}
			else if (errCnt++ < 4)
			{

			}
			else
			{
				vt->freqCnt = 0;
				errCnt = 0;
//				printf("vt->rawCode.cnt: %d\n", vt->rawCode.cnt);
				if (vt->rawCode.cnt > 200)
				{
					memcpy(rawCode, &vt->rawCode, sizeof(rawCode_t));
					vt->rawCode.cnt = 0;
					break;
				}
				vt->globalIndex = 0;
				vt->rawCode.cnt = 0;
			}
		}

		if (vt->freqCnt >= sizeof(vt->freqList))
		{
			vt->freqCnt = 0;
		}
	}

	return 0;
}


typedef struct{
	char bError; //是否有错
	char validCnt; //有效数据个数
	char validInSep; //分隔符内数量
	unsigned char result;
	unsigned char code[4]; //有效数据
}CodeInfo_t;

#define BYTE_SEP 5
#define BIT_SEP 4

static int __audio_code2uchar(unsigned char *code, int len, CodeInfo_t *result, int maxlen)
{
	int dl = 0;
	int i,j;
	unsigned char val;

	i=0;
	while(code[i] != BYTE_SEP && i < len)
		i++;
	while (i<len)
	{
		char bError = FALSE;
		if (code[i] != BYTE_SEP)
		{
//			printf("BYTE_SEP not finded\n");
			bError = TRUE;
		}
		else
		{
			i++;
		}
		result[dl].validCnt = 0;
		while(code[i] != BYTE_SEP && i < len)
		{
			result[dl].code[(int)result[dl].validCnt++] = code[i];
			i++;
			if (result[dl].validCnt == 4)
			{
				j=0;
				result[dl].validInSep = result[dl].validCnt;
				while (code[i+j] != BYTE_SEP && i+j < len)
				{
					result[dl].validInSep++;
					j++;
				}
				if (result[dl].validInSep != result[dl].validCnt)
					bError = TRUE;
				break;
			}
		}

		//calc data
		result[dl].result = 0;
		for (j=0;j<result[dl].validCnt;j++)
		{
			val = result[dl].code[j];
			result[dl].result <<= 2;
			result[dl].result |= val;
		}
		if (result[dl].validCnt != 4)
		{
			bError = TRUE;
			result[dl].bError = bError;
			continue;
		}
		result[dl].bError = bError;

		MVPrintf(1, "%c,0x%02x, err: %d, cnt: %d, code: %d,%d,%d,%d\n"
			, result[dl].result
				, result[dl].result
				, result[dl].bError
				, result[dl].validCnt
				, result[dl].code[0]
				, result[dl].code[1]
				, result[dl].code[2]
				, result[dl].code[3]);

		dl++;
	}

	return dl;
}

// 8bit CRC (X(8) + X(2) + X(1) + 1)
#define AL2_FCS_COEF ((1 << 7) + (1 << 6) + (1 << 5))

unsigned char crc_8(unsigned char * data, int length)
{
	unsigned char cFcs = 0;
   int i, j;

   for( i = 0; i < length; i ++ )
   {
      cFcs ^= data[i];
      for(j = 0; j < 8; j ++)
      {
         if(cFcs & 1)
         {
            cFcs = (unsigned char)((cFcs >> 1) ^ AL2_FCS_COEF);
         }
         else
         {
            cFcs >>= 1;
         }
      }
   }

   return cFcs;
}

unsigned int crc_16(unsigned char *buf, unsigned int length)
{
    unsigned int i;
    unsigned int j;
    unsigned int c;
    unsigned int crc = 0xFFFF;
    for (i=0; i<length; i++)
    {
        c = *(buf+i) & 0x00FF;
        crc^=c;
        for (j=0; j<8; j++)
        {
             if (crc & 0x0001)
             {
                crc >>= 1;
                crc ^= 0xA001;
             }
             else
             {
                crc >>= 1;
             }
        }
   }
    crc = (crc>>8) + (crc<<8);
    return(crc);
}

/**
 *@brief 解码成最终的字符
 *
 *@param code
 *@param result
 *@param maxlen
 *
 *@return len of result
 */
int audio_code_decode(resultCode_t *code, unsigned char *result, int maxlen)
{
	int i;
	static struct{
		CodeInfo_t cinfo[128];
		int cnt;
	}olderData = {.cnt=0};

	//delete the BIT_SEP
	int tl = 0;
	for (i=0;i<code->cnt;i++)
	{
		if (code->code[i] != BIT_SEP)
		{
			code->code[tl++] = code->code[i];
		}
	}
	code->cnt = tl;

	int clen; // uchar len
	CodeInfo_t codeInfo[128];
	clen = __audio_code2uchar(code->code, code->cnt, codeInfo, maxlen);
	for (i=0;i<clen;i++)
		result[i] = codeInfo[i].result;

	if (0 == crc_16(result, clen))
	{
		clen -= 2;
		result[clen] = '\0';//crc值不需要
		olderData.cnt = 0;
		return clen;
	}


	//尝试恢复数据
	Printf("Now try to repair\n");
	if (abs(clen - olderData.cnt) < 4)
	{
		//TOOD: Try to make data useful

		//互补一下先
		if (clen == olderData.cnt)
		{
			for(i=0;i<clen;i++)
			{
				if (codeInfo[i].bError)
				{
					if (!olderData.cinfo[i].bError)
					{
						memcpy(&codeInfo[i].bError, &olderData.cinfo[i].bError, sizeof(CodeInfo_t));
					}
				}
			}
			for (i=0;i<clen;i++)
				result[i] = codeInfo[i].result;

			if (0 == crc_16(result, clen))
			{
				clen -= 2;
				result[clen] = '\0';//crc值不需要
				olderData.cnt = 0;
				return clen;
			}
		}
	}

	memcpy(olderData.cinfo, codeInfo, sizeof(olderData.cinfo));
	olderData.cnt = clen;

	Printf("ERROR: Failed checksum\n");

	return 0;
}

static void __send_msg_code_voice_set(char *info)
{
	//char info[512] = {0};
	//strcpy(info, "dlink_zy;23456789     ;;;23.93e515df871b40627d0d78b07eb7a404.2592000.1455184818.2013387716-7499131;0aea6643b5dc11e5bbdff80f41fbe890;");
	//printf("__send_msg_code_voice_set %s\n", info);

	maudio_resetAIAO_mode(2);
    maudio_speaker(VOICE_REV_NET_SETTING,TRUE, TRUE, FALSE);

	char* end = NULL;
	char* start = NULL;

	memset(&sWifiInfo, 0 ,sizeof(sWifiInfo));
	//SSID
	start = info;
	if (*start == 0)
		return;
	end = strchr(start, ';');
	if (end != NULL)
	{
		if (start != end)
		{
			*end = 0;
			strncpy(sWifiInfo.ap2Set.name, start, sizeof(sWifiInfo.ap2Set.name));
		}
	}
	printf("%s\n", sWifiInfo.ap2Set.name);
	//PWD
	start = end + 1;
	if (*start != 0)
	{
		end = strchr(start, ';');
		if (end != NULL)
		{
			if (start != end)
			{
				*end = 0;
				strncpy(sWifiInfo.ap2Set.passwd, start, sizeof(sWifiInfo.ap2Set.passwd));
			}
		}
		else
		{
			strncpy(sWifiInfo.ap2Set.passwd, start, sizeof(sWifiInfo.ap2Set.passwd));
			sWifiInfo.toset = TRUE;			
			return;
		}	
	}
	//AUTH
	start = end + 1;
	if (*start != 0)
	{
		end = strchr(start, ';');
		if (end != NULL)
		{
			if (start != end)
			{
				*end = 0;
				sWifiInfo.ap2Set.iestat[0] = atoi(start);
				sWifiInfo.bCustom = TRUE;
			}
		}
	}
	//mdType
	start = end + 1;
	if (*start != 0)
	{
		end = strchr(start, ';');
		if (end != NULL)
		{
			if (start != end)
			{
				*end = 0;
				sWifiInfo.ap2Set.iestat[1] = atoi(start);
				sWifiInfo.bCustom = TRUE;
			}
		}
	}
	
	printf("voice setting ssid : %s ,  passwd : %s , auth : %d , mdType : %d \n",sWifiInfo.ap2Set.name,
		sWifiInfo.ap2Set.passwd, sWifiInfo.ap2Set.iestat[0], sWifiInfo.ap2Set.iestat[1]);

	sWifiInfo.toset = TRUE;


	//百度云
	BLSInfoType BLSinfo;
	memset(&BLSinfo, 0, sizeof(BLSInfoType));
	//access_token
	start = end + 1;
	if (*start == 0)
		return;
	end = strchr(start, ';');
	if (end != NULL)
	{
		if (start != end)
		{
			*end = 0;
			memcpy(BLSinfo.access_token, start, sizeof(BLSinfo.access_token));
		}
	}
	//stream_id
	start = end + 1;
	if (*start == 0)
		return;
	end = strchr(start, ';');
	if (end != NULL)
	{
		if (start != end)
		{
			*end = 0;
			memcpy(BLSinfo.stream_ID, start, sizeof(BLSinfo.stream_ID));
		}
	}
	printf("voice setting access_token: %s, streamid: %s\n", BLSinfo.access_token,
				BLSinfo.stream_ID);
}

static void __ai_pcm_callback(int channelid, jv_audio_frame_t *frame)
{
#if 1
	static VoiceTrans_t sVT;
	if (bVoicedecEnabled == FALSE)
		return ;
	if (sWifiInfo.setting)
		return ;
	rawCode_t raw;
	resultCode_t code;
	raw.cnt = 0;
	audio_code_voice2raw(&sVT, frame->aData, frame->u32Len, &raw);

	if (raw.cnt)
	{
		audio_code_raw2code(&raw, &code);
		unsigned char dst[512];
		int len;
		MVPrintf(3,"sVT.result.cnt: %d\n", code.cnt);
		len = audio_code_decode(&code, dst, sizeof(dst));
		if (len > 0)
		{
			dst[len] = '\0';
			MVPrintf(1, "dst len: %d\n", len);
			MVPrintf(1, "dst: %s\n", dst);
			__send_msg_code_voice_set((char *)dst);
		}
	}

#elif 0
	{
		static FILE *fp = NULL;
		if (!fp)
		{
			fp = fopen("audio.pcm", "wb");
		}
		if (fp)
		{
			fwrite(frame->aData, 1, frame->u32Len, fp);
		}
	}
#elif 1
	static int cnt = 0;
	if (0)//cnt++ < 4)
	{
		return ;
	}
	cnt = 0;
	int i;
	unsigned short *pp = (unsigned short *)frame->aData;
	for (i=0;i<frame->u32Len/2;i++)
	{
		int j;
		unsigned char c = ((pp[i]+0x8000)>>8) & 0xFF;
		printf("\n%02x", c);
		for (j=0x70;j<c; j+= 1)
		{
			printf("f");
		}
	}

#else
	printf("ai getted: bit: %d, len: %d\n"
			, frame->enBitwidth
			, frame->u32Len
			);
	int i;
	unsigned short *pp = frame->aData;
	for (i=0;i<100;i++)
	{
		if ((i%16) == 0)
			printf("\n");
		printf("%02x,", ((pp[i]+0x8000)>>8) & 0xFF);
	}
	printf("\n\n");
#endif
}

#ifdef SOUND_WAVE_DEC_SUPPORT
#include "sound_wave_dec.h"

void * HSoundWaveDec = NULL;

static void __sound_wave_dec_done_cb(void * str)
{
	printf("=============>>>>>>>> sound wave dev done %s  <<<<<<<<<============\n\n", (char*)str);
	__send_msg_code_voice_set((char *)str);
}


static void __ai_pcm_lib_callback(int channelid, jv_audio_frame_t *frame)
{
	if (bVoicedecEnabled == FALSE)
		return ;
	//printf("%d\n", frame->u32Len);
	sound_wave_dec_w_buf(HSoundWaveDec, frame->aData, frame->u32Len);
	
}
#endif

static int timer_count_for_voiceOrsmart = -1;
static BOOL s_AppendPrivateSearchInfo = FALSE;		// 搜索时添加自定义数据的标志

static BOOL _time_for_count()
{
	utl_timer_destroy(timer_count_for_voiceOrsmart);
	timer_count_for_voiceOrsmart = -1;

#ifdef YST_SVR_SUPPORT
	JVN_SetLSPrivateInfo("timer_count=0;", strlen("timer_count=0;")+1);
#endif

	s_AppendPrivateSearchInfo = FALSE;

	return FALSE;
}

int Start_timer_for_count()
{
	if (timer_count_for_voiceOrsmart == -1)
		timer_count_for_voiceOrsmart = utl_timer_create("timer_for_count", 180000, (utl_timer_callback_t)_time_for_count, NULL);
	else
		utl_timer_reset(timer_count_for_voiceOrsmart, 180000, (utl_timer_callback_t)_time_for_count, NULL);

	s_AppendPrivateSearchInfo = TRUE;

#ifdef YST_SVR_SUPPORT
	JVN_SetLSPrivateInfo("timer_count=1;", strlen("timer_count=1;") + 1);
#endif

	return 0;
}

BOOL CheckbAppendSearchPrivateInfo()
{
	return s_AppendPrivateSearchInfo;
}

static void *__set_wifi(void *arg)
{
	int time_elapsed = 0;
	int k = 0;
	wifiap_t * ap = NULL;
	pthreadinfo_add((char *)__func__);
	
	while(1)
	{		
		if (!sWifiInfo.toset)
		{
			usleep(300*1000);
			continue;
		}
		sWifiInfo.setting = TRUE;
		time_elapsed = 0;
/*
1，收到网络配置
2，找不到所设置的网络
3，网络配置成功
4，网络连接失败
5，获取 IP 地址失败
 */
		do{
			if(utl_ifconfig_wifi_smart_get_recvandsetting())		//如果直连路由已经收到配置信息，则这里直接翻跳过配置
				break;
			
			net_deinit();

			// 移动到关闭智联路由之后，ifconfig ra0 0.0.0.0时会先up网卡，可能与智联路由线程冲突
			// if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
			// {
			// 	utl_ifconfig_wifi_sta_ip_clear();
			// }

			//TODO: Speak that: Received Net Configuration
			MVPrintf(1, "Received Wifi Configuration, ssid: %s, passwd: %s\n", sWifiInfo.ap2Set.name, sWifiInfo.ap2Set.passwd);
			//break;
			//printf("maudio_speaker,VOICE_REV_NET_SETTING\n");

			if (utl_ifconfig_bsupport_smartlink(utl_ifconfig_wifi_get_model()))
			{
				utl_ifconfig_wifi_smart_connect_close(TRUE);
			}
			else			//此处非7601模块要等待AP加载成功之后在进行配置
			{
				while(1)			
				{
					if(mio_get_get_st() == DEV_ST_WIFI_SETTING)
						break;

					sleep(4);
				}
			}

			// 退出智联路由后，再清除ra0 IP，避免与智联路由冲突
			if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
			{
			 	utl_ifconfig_wifi_sta_ip_clear();
			}

			if(sWifiInfo.bCustom)
			{
				ap = &(sWifiInfo.ap2Set);
			}
			else
			{
				for (k = 0; k < 3; k++)
				{
					ap = utl_ifconfig_wifi_get_by_ssid(sWifiInfo.ap2Set.name);
					if (ap)
					{
						break;
					}
					sleep(1);
				}
			}
			if (!ap)
			{
				maudio_speaker(VOICE_NOT_FIND_NET,TRUE, TRUE, TRUE);
			 	maudio_resetAIAO_mode(1);
				if (utl_ifconfig_bsupport_smartlink(utl_ifconfig_wifi_get_model()))
				{
					utl_ifconfig_wifi_smart_connect();
				}
				break;
			}
			
			Start_timer_for_count();						//收到网络配置时开始计时

			strcpy(ap->passwd, sWifiInfo.ap2Set.passwd);

			utl_ifconfig_wifi_start_sta();
			int ret = utl_ifconfig_wifi_connect(ap);
			if (ret == 0)
			{
				//printf("maudio_speaker,VOICE_NET_SET_SUCC\n");
				mvoicedec_disable();
			}
			else
			{
				maudio_resetAIAO_mode(1);	//声波配置失败还要设置回去，要不重新配置不起作用
				//printf("maudio_speaker,VOICE_GET_IP_FAIL\n");
				if (utl_ifconfig_bsupport_smartlink(utl_ifconfig_wifi_get_model()))
				{
					utl_ifconfig_wifi_smart_connect();
				}
			}
				
		} while(0);

		sWifiInfo.toset = FALSE;
		sWifiInfo.setting = FALSE;
		sWifiInfo.bCustom = FALSE;
	}
	return NULL;
}
/*音频输入输出切换到24k采样率 来使能声波配置WIFI功能，这时声音不能被编码*/
int mvoicedec_enable()
{
	if (strcmp(hwinfo.devName, "HXBJRB") == 0)
		return 0;

	if (!hwinfo.bSupportVoiceConf)
		return 0;

	if (bVoicedecEnabled == TRUE)
		return 0;

	maudio_resetAIAO_mode(1);
#ifdef SOUND_WAVE_DEC_SUPPORT
	if(hwinfo.bNewVoiceDec)
	{
		printf("=============>>>>>>>>  voiceseting start!!!!! <<<<<<<<<============\n");
		HSoundWaveDec = sound_wave_dec_init(480 * 2);
		sound_wave_dec_regist_done_cb(HSoundWaveDec, __sound_wave_dec_done_cb);
	}
#endif
	bVoicedecEnabled = TRUE;

	printf("enable mvoicedec\n");
	return 0;
}

/*音频输入输出切换到8k采样率 来禁止声波配置WIFI功能，这时声音可以被编码*/
int mvoicedec_disable()
{
	if (!hwinfo.bSupportVoiceConf)
		return 0;

	maudio_resetAIAO_mode(2);
	bVoicedecEnabled = FALSE;

	speakerowerStatus = JV_SPEAKER_OWER_NONE;
#ifdef SOUND_WAVE_DEC_SUPPORT
	if(hwinfo.bNewVoiceDec && (NULL != HSoundWaveDec))
	{
		sound_wave_dec_finalize(HSoundWaveDec);
		HSoundWaveDec = NULL;
	}
#endif
	printf("disable mvoicedec\n");
	return 0;
}

int mvoicedec_init(void)
{
	pthread_t sThreadID;
	if(!hwinfo.bHomeIPC || strcmp(hwinfo.devName, "HXBJRB") == 0)
		return 0;

	if (!hwinfo.bSupportVoiceConf)
		return 0;

#ifdef SOUND_WAVE_DEC_SUPPORT
	if(hwinfo.bNewVoiceDec)
		jv_ai_set_pcm_callback(0, __ai_pcm_lib_callback);
	else
		jv_ai_set_pcm_callback(0, __ai_pcm_callback);
#else
	jv_ai_set_pcm_callback(0, __ai_pcm_callback);
#endif

	//mvoicedec_enable();
	pthread_create(&sThreadID, NULL, __set_wifi, NULL);

	return 0;
}

BOOL isVoiceSetting()
{
	return bVoicedecEnabled;
}

BOOL isVoiceRecvAndSetting()
{
	return sWifiInfo.setting;
}

