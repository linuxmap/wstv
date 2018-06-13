#include "jv_common.h"
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "mfirmup.h"
#include "utl_filecfg.h"
#include "utl_ifconfig.h"

int debugFlag = 0;
firmup_info_t firmupInfo;

/**
 *@brief 读取版本文件内容
 *
 */
static int _firmup_get_ver(char *fname, FirmFile_t *ff)
{
	keyvalue_t *kv;

	memset(ff, 0, sizeof(firmup_info_t));
	ff->cnt = -1;

	ff->fileSize = FIRMUP_FILELEN;
	int cur = 0;
	utl_fcfg_start_getnext(fname);
	while(1)
	{
		kv = utl_fcfg_get_next(fname, &cur);
		if (kv == NULL)
			break;
		//printf("%s=%s\n", kv->key, kv->value);
		if (strcmp("module", kv->key) == 0)
		{
			ff->cnt++;
			if (ff->cnt >= sizeof(ff->list) / sizeof(FirmModule_t))
			{
				printf("ERROR: too many modules.\n");
				break;
			}
			strncpy(ff->list[ff->cnt].name, kv->value, sizeof(ff->list[ff->cnt].name));
		}
		else if (strcmp("checksum", kv->key) == 0)
		{
			ff->checksum = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("fileSize", kv->key) == 0)
		{
			ff->fileSize = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("product", kv->key) == 0)
		{
			strncpy(ff->product, kv->value, sizeof(ff->product));
		}
		if (ff->cnt == -1)
			continue;

		if (strcmp("ver", kv->key) == 0)
		{
			ff->list[ff->cnt].ver = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("offset", kv->key) == 0)
		{
			ff->list[ff->cnt].offset = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("size", kv->key) == 0)
		{
			ff->list[ff->cnt].size = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("dev", kv->key) == 0)
		{
			strncpy(ff->list[ff->cnt].dev, kv->value, sizeof(ff->list[ff->cnt].dev));
		}
	}
	utl_fcfg_end_getnext(fname);
	ff->cnt++;
	utl_fcfg_close(fname);

	{
		int i;
		for (i=0;i<ff->cnt;i++)
		{
			printf("module: %s, ver: %d, offset: 0x%x, size: 0x%x, dev: %s\n", 
				ff->list[i].name, ff->list[i].ver, ff->list[i].offset, ff->list[i].size, ff->list[i].dev);
		}
	}
	return 0;
}

static int _WriteFlashPartion(S32 fd, char *dev, U8 *pBuf, S32 nBufSize, S32 nStart, S32 nCount)
{
	S32 dev_fd;
	S32 ret;
	S32 i;
	printf("Write param:nBufSize=%d, nStart=%d, nCount=%d\n", nBufSize, nStart, nCount);

	dev_fd = open (dev, O_SYNC | O_WRONLY);
	if(dev_fd < 0)
	{
	    printf("open dev fail \n");
	    return -1;
	}

    //定位
	lseek(fd, nStart*nBufSize, SEEK_SET);
    
	for (i=0; i<nCount; i++)
	{
		ret = read(fd, pBuf, nBufSize);
		if (ret != nBufSize)
		{
			printf("read failed\n");
	        	close(dev_fd);
			return -1;
		}
		ret = write(dev_fd ,pBuf, nBufSize) ;
		if (ret != nBufSize)
		{
			printf("write failed: %d != %d, errno: %s\n", ret, nBufSize, strerror(errno));
        	close(dev_fd);
            return -1;
		}
	}
	close(dev_fd);
    return 0;
}

//写flash
static int _WriteFlash(char *pFileName, FirmFile_t *target, FirmFile_t *mine)
{
	int fd = -1;
	int ret;
	unsigned char buf[BLOCK_BUF_SIZE]={0};
    
    if(NULL == pFileName)
    {
        return -1;
    }

    fd = open (pFileName, O_RDONLY);
    if(fd < 0)
    {
        printf("open file failed\n");
        return -1;
    }

	//_flash_write_unlock();	//开始升级，解锁

	int i;
	for (i=0; i<target->cnt;i++)
	{
		if (target->list[i].needUpdate)
		{
			ret = _WriteFlashPartion(fd,target->list[i].dev,buf, BLOCK_BUF_SIZE, target->list[i].offset/(64*1024), target->list[i].size/(64*1024));
			if (ret)
			{
				close(fd);
				printf("Write Failed...\n");
				return ret;
			}
		}
	}
	//_flash_write_lock();		//升级完毕，重新上锁
	close(fd);
	printf("Write success\n");
    return 0;
}

static int _CopyFile(char *fDst, char *fSrc)
{
    FILE *fIn, *fOut;
    unsigned char	strBuffer[4096]={0};
    int nSize;
    
	fIn	= fopen(fSrc, "rb");
	if(fIn == NULL)
	{
		return -1;
    }
	fOut = fopen(fDst, "wb");
	if(fOut == NULL)
	{
		fclose(fIn);
		return -1;
	}
	while(!feof(fIn))
	{
		nSize = fread(strBuffer, 1, 4096, fIn);
		fwrite(strBuffer, 1, nSize, fOut);
	}
	fclose(fOut);
	fclose(fIn);

	return 0;
}

/******
*
*替换字符串中字符串
*
*****/

static int _strreplace(char* pSrc,char* pRep,char* pDest,char* pOut)
{
	char* pi = pSrc;
	char* po = pOut;

	int nLeftLen = 0;
	
	int repLen = strlen(pRep);
	int desLen = strlen(pDest);
	
	char* p = strstr(pi,pRep);

	if(p)
	{
		nLeftLen = p - pi;
		memcpy(po,pi,nLeftLen);
		memcpy(po + nLeftLen,pDest,desLen);

		p += repLen;

		strcpy(po + nLeftLen + desLen,p);
	}
	else
	{
		strcpy(po,pi);
	}
	
	return 0;
}

/**
 *@从配置文件中初始化升级所需的参数
 *
 *
 */
static void _firmup_get_config(firmup_info_t *info)
{
	keyvalue_t *kv;
	memset(info, 0, sizeof(firmup_info_t));

	int cur = 0;
	int bchangeflag = 0;			//是否升级路径要改变
	char wifiMode[8] = {0};		//wifi 模块名称
	char devName[32];
	char product[32];
	
    strcpy(product, "Unknown");
    utl_fcfg_get_value_ex(JOVISION_CONFIG_FILE, "product", product, sizeof(product));
	utl_fcfg_get_value_ex(CONFIG_HWCONFIG_FILE, "product", devName, sizeof(devName)); //devname
	
	utl_fcfg_start_getnext(JOVISION_CONFIG_FILE);
	
	while(1)
	{
		char chTmp[128] = {0};
		char chNewPath[128] = {0};
		char relWifi[16] = {0};
		kv = utl_fcfg_get_next(JOVISION_CONFIG_FILE, &cur);
		if (kv == NULL)
			break;
		
		//printf("%s=%s\n", kv->key, kv->value);
		if (strcmp("firmup-url", kv->key) == 0)
		{
			if(NULL == strstr(kv->value,"3518esi") && strstr(product,"JVS-HI3518ES"))
				sprintf(chNewPath,"%s%s",kv->value,"i");	//IPC也改了路径名称
			else 
				strcpy(chNewPath,kv->value);
			printf("=================> IPC new Path is : %s \n",chNewPath);
			if (info->urlCnt < 4)
			{
				strcpy(info->url[info->urlCnt++], chNewPath);
			}
		}
		else if (strcmp("firmup-binName", kv->key) == 0)
		{
			strcpy(chNewPath,kv->value);
			
			strcpy(info->binName, chNewPath);
		}
		else if (strcmp("firmup-verName", kv->key) == 0)
		{
			strcpy(chNewPath,kv->value);
			
			strcpy(info->verName, chNewPath);
		}
		else if (strcmp("product", kv->key) == 0 || bchangeflag)  //因为product字段最先被读到，bchangeflag后被赋值，所以作此操作
		{
			if(!bchangeflag && NULL != kv->value)
				strcpy(info->product, kv->value);
			else
				printf("=================> new Product is : %s \n",info->product);
		}
	}
	utl_fcfg_end_getnext(JOVISION_CONFIG_FILE);

	sprintf(info->curVer, "%s/%s", CONFIG_PATH, info->verName);
	sprintf(info->tmpVer, "/tmp/%s", info->verName);
	sprintf(info->tmpBin, "/tmp/%s", info->binName);
	info->inited = 1;
}
static FirmModule_t *__module_get_info(FirmFile_t *firm, char *module)
{
	int i;
	for (i=0;i<firm->cnt;i++)
	{
		if (0 == strcmp(firm->list[i].name, module))
		{
			return &firm->list[i];
		}
	}

	return NULL;
}

static BOOL _firmup_is_need_update(FirmFile_t *target, FirmFile_t *mine)
{
	BOOL ret = FALSE;
	int i;
	FirmModule_t *fm;

	for (i=0;i<target->cnt;i++)
	{
		//BOOT不升级
		if (strcmp(target->list[i].name, "boot") == 0)
		{
			continue;
		}
		fm = __module_get_info(mine, target->list[i].name);
		if (fm == NULL)
		{
			printf("Failed find module: %s\n",target->list[i].name );
			target->list[i].needUpdate = TRUE;
			ret = TRUE;
		}
		else if (target->list[i].ver != fm->ver)
		{
			printf("[%s] version diff..., ver: %d != %d, name: %s\n", target->list[i].name, target->list[i].ver, fm->ver, fm->name);
			target->list[i].needUpdate = TRUE;
			ret = TRUE;
		}

	}
	return ret;
}

//检查校验并顺便解密
static int _ChecksumAndDecrypt(U32 *nChecksum)
{
    U32 i,j;
    U32 pBuf[1024]={0};
    FILE *fp;

    *nChecksum = 0;
    fp = fopen(firmupInfo.tmpBin, "rb+");
    if(NULL == fp)
    {
        return -1;
    }
	FirmFile_t target;
	
	_firmup_get_ver(firmupInfo.tmpVer, &target);

    for (i=0; i<(target.fileSize/1024); i++)
    {
        fread(pBuf, 4, 256, fp);
        for (j=0; j<256; j++)
        {
            *nChecksum+=pBuf[j];
            pBuf[j] = pBuf[j]^0x1f441f44;
        }
        fseek(fp, -1024, SEEK_CUR);
        fwrite(pBuf, 4, 256, fp);
    }

    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
	unsigned int nChecksum;
	int ret;
	struct stat fileStat;

	_firmup_get_config(&firmupInfo);

	FirmFile_t target,mine;
	//_flash_write_lock_init();

	//printf("firmup_get_ver\n");
	_firmup_get_ver(firmupInfo.tmpVer, &target);
	_firmup_get_ver(firmupInfo.curVer, &mine);

	//检查获取版本结果
	if (target.cnt <= 0)
	{
		printf("target.cnt <= 0 failed\n");
		goto FIRMUP_END;
	}
	_firmup_is_need_update(&target, &mine);

	//检查校验并顺便解密
	ret = _ChecksumAndDecrypt(&nChecksum);
	if (0 != ret || (nChecksum != target.checksum) )
	{
		printf("verTarget.nChecksum=0x%x, local=0x%x\n", target.checksum, nChecksum);
		goto FIRMUP_END;
	}
	//烧写flash
	printf("WriteFlash\n");
	if (_WriteFlash(firmupInfo.tmpBin, &target, &mine))
	{
		printf("WriteFlash failed\n");
		goto FIRMUP_END;
	}
	//升级完成，复制新的版本文件
	printf("CopyFile\n");
	_CopyFile(firmupInfo.curVer, firmupInfo.tmpVer);
	printf("ReleaseFirmup\n");
	unlink(firmupInfo.tmpBin);
	printf("firmup ok\n");
	//_flash_write_lock_deinit();
	return 0;

FIRMUP_END:
	unlink(firmupInfo.tmpBin);
	//_flash_write_lock_deinit();
	return -1;
}

