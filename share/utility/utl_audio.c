/*
 * utl_audio.c
 *
 *  Created on: 2014年9月26日
 *      Author: Administrator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>

#include "utl_audio.h"


typedef struct
{
	struct{
		char szRiffID[4];    // 'R','I','F','F'
		int  dwRiffSize;	//从下一个地址开始到文件尾的总字节数。高位字节在后面，
		char szRiffFormat[4]; // 'W','A','V','E'
	}RIFF_HEADER;

	struct{
		char szFmtID[4]; // 'f','m','t',' '
		int dwFmtSize;	//下一结构体的大小，其值为16
		struct{
			short wFormatTag;	//为1时表示线性PCM编码，大于1时表示有压缩的编码
			short  wChannels;	//1为单声道，2为双声道
		    int dwSamplesPerSec;	//采样率
			int  dwAvgBytesPerSec;	//
			short wBlockAlign;
			short wBitsPerSample;	//Bits per sample
		}WAVE_FORMAT;
	}FMT_BLOCK;

} WAVE_FILE_HEADER;

typedef struct
{
	char szFactID[4]; // 'd','a','t','a'
	int dwFactSize;
}Block;


typedef struct{
	char fname[256];
	UtlAudioFileType_e type;
	FILE *fp;
	int audioStart; //音频开始的位置

	UtlAudioInfo_t info;
}UAParam_t;

/**
 *@brief 测试是否为wav格式
 *
 *@return 1 if yes, 0 otherwise
 */
static int __utl_audio_try_parse_wav(UAParam_t *uaudio)
{
	WAVE_FILE_HEADER wh;

	fseek(uaudio->fp, 0, SEEK_SET);

	int len = fread(&wh, 1, sizeof(wh), uaudio->fp);
	if (len != sizeof(wh))
		return 0;
/*
	printf("fmtsize: %d, samplePerSec: %d, bitsPerSample: %d\n"
			, wh.FMT_BLOCK.dwFmtSize
			, wh.FMT_BLOCK.WAVE_FORMAT.dwSamplesPerSec
			, wh.FMT_BLOCK.WAVE_FORMAT.wBitsPerSample
			);
			*/
	uaudio->info.bitsPerSample = wh.FMT_BLOCK.WAVE_FORMAT.wBitsPerSample;
	uaudio->info.encType = wh.FMT_BLOCK.WAVE_FORMAT.wFormatTag;
	uaudio->info.samplerate = wh.FMT_BLOCK.WAVE_FORMAT.dwSamplesPerSec;
	/*
	char *S1="WAVE";
	char *S2="fmt";
	char *s3 ="fact";
	char *s4="data";

	 */

	if (memcmp(wh.RIFF_HEADER.szRiffID, "RIFF", 4) != 0
			|| memcmp(wh.RIFF_HEADER.szRiffFormat, "WAVE", 4) != 0
			|| memcmp(wh.FMT_BLOCK.szFmtID, "fmt ", 4) != 0
			)
	{
		printf("not wave file\n");
		return 0;
	}
	uaudio->audioStart += len;

	unsigned short addition;
	//带有2字节附加信息
	if (wh.FMT_BLOCK.dwFmtSize == 18)
	{
		fread(&addition, 1, 2, uaudio->fp);
		uaudio->audioStart += 2;
	}

	//block
	Block block;
	while(1)
	{
		len = fread(&block, 1, sizeof(block), uaudio->fp);
		if (len != sizeof(block))
		{
			return 0;
		}
		uaudio->audioStart += len;
		if (memcmp(block.szFactID, "fact", 4) == 0)
		{
			fseek(uaudio->fp, block.dwFactSize, SEEK_CUR);
			uaudio->audioStart += len;
			continue;
		}
		else if (memcmp(block.szFactID, "data", 4) == 0)
		{
			uaudio->info.audioSize = block.dwFactSize;
			break;
		}
	}

	return 1;
}

/**
 *@brief 打开音频文件
 *
 *@param fname 音频文件名
 *@param type 音频文件类型。填 #UTL_AUDIO_TYPE_UNKNOWN时内部可自动识别
 *
 *@return Handle of audio file
 */
UtlAudioHandle_t utl_audio_open(const char *fname, UtlAudioFileType_e type)
{
	UAParam_t *handle = malloc(sizeof(UAParam_t));

	memset(handle, 0, sizeof(UAParam_t));
	strncpy(handle->fname, fname, sizeof(handle->fname));
	handle->fp = fopen(fname, "rb");
	if (!handle->fp)
	{
		printf("utl_audio_fopen failed open file: %s, because: %s\n", fname, strerror(errno));
		free(handle);
		return NULL;
	}

	if (type == UTL_AUDIO_TYPE_WAV)
	{
		if (__utl_audio_try_parse_wav(handle))
		{
			return handle;
		}

	}

	//others
	if (__utl_audio_try_parse_wav(handle))
	{
		return handle;
	}

	return NULL;
}

/**
 *@brief 关闭音频文件
 *
 *@param handle Handle of audio file
 *
 *@return 0 if success
 */
int utl_audio_close(UtlAudioHandle_t handle)
{
	UAParam_t *uaudio = (UAParam_t *)handle;
	if (!handle)
	{
		printf("ERROR: %s, bad param\n", __func__);
		return -1;
	}

	fclose(uaudio->fp);
	memset(uaudio, 0, sizeof(UAParam_t));
	free(uaudio);

	return 0;
}

/**
 *@brief 读取音频数据
 */
int utl_audio_read(UtlAudioHandle_t handle, unsigned char *buffer, int len)
{
	UAParam_t *uaudio = (UAParam_t *)handle;
	if (!handle)
	{
		printf("ERROR: %s, bad param\n", __func__);
		return -1;
	}

	return fread(buffer, 1, len, uaudio->fp);
}
/**
 *@brief 移动音频文件的读取指针。使用方法参考 #fseek
 *
 *@brief 关闭音频文件
 *@param offset 偏移。
 *@param whence SEEK_CUR, SEEK_SET, SEEK_END
 *
 *@return 0 if success
 */
int utl_audio_seek(UtlAudioHandle_t handle, int offset, int whence)
{
	UAParam_t *uaudio = (UAParam_t *)handle;
	if (!handle)
	{
		printf("ERROR: %s, bad param\n", __func__);
		return -1;
	}
	int realoffset;
	if (whence == SEEK_SET)
	{
		realoffset = uaudio->audioStart+offset;
	}
	else
	{
		realoffset = offset;
	}

	return fseek(uaudio->fp, uaudio->audioStart+offset, whence);
}

/**
 *@brief 获取音频文件信息
 *
 *@param handle Handle of audio file
 *@param info 音频信息
 *
 *@return 0 if success
 */
int utl_audio_get_fileinfo(UtlAudioHandle_t handle, UtlAudioInfo_t *info)
{
	UAParam_t *uaudio = (UAParam_t *)handle;
	if (!handle)
	{
		printf("utl_audio_get_fileinfo bad param\n");
		return -1;
	}

	memcpy(info, &uaudio->info, sizeof(UtlAudioInfo_t));

	return 0;
}


