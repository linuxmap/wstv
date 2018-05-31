/*
 * utl_audio.h
 *
 *  Created on: 2014年9月26日
 *      Author: Administrator
 */

#ifndef UTL_AUDIO_H_
#define UTL_AUDIO_H_

#include <stdio.h>

typedef enum{
	UTL_AUDIO_TYPE_UNKNOWN,
	UTL_AUDIO_TYPE_WAV,

}UtlAudioFileType_e;

typedef enum{
	UTL_AUDIO_ENC_PCM,
	UTL_AUDIO_ENC_G711_A,
	UTL_AUDIO_ENC_G711_U,
	UTL_AUDIO_ENC_G726_16K, // not support
	UTL_AUDIO_ENC_G726_24K, // not support
	UTL_AUDIO_ENC_G726_32K, // not support
	UTL_AUDIO_ENC_G726_40K,
	UTL_AUDIO_ENC_ADPCM,
}UtlAudioEncType_e;

typedef void * UtlAudioHandle_t;

typedef struct{
	UtlAudioEncType_e encType;
	int samplerate; //采样率，8000，16000，32000，。。。
	int bitsPerSample; //bits per sample 8, 16
	int audioSize; //音频部分占的字节数
}UtlAudioInfo_t;

/**
 *@brief 打开音频文件
 *
 *@param fname 音频文件名
 *@param type 音频文件类型。填 #UTL_AUDIO_TYPE_UNKNOWN时内部可自动识别
 *
 *@return Handle of audio file
 */
UtlAudioHandle_t utl_audio_open(const char *fname, UtlAudioFileType_e type);

/**
 *@brief 关闭音频文件
 *
 *@param handle Handle of audio file
 *
 *@return 0 if success
 */
int utl_audio_close(UtlAudioHandle_t handle);

/**
 *@brief 读取音频数据
 */
int utl_audio_read(UtlAudioHandle_t handle, unsigned char *buffer, int len);

/**
 *@brief 移动音频文件的读取指针。使用方法参考 #fseek
 *
 *@brief 关闭音频文件
 *@param offset 偏移。
 *@param whence SEEK_CUR, SEEK_SET, SEEK_END
 *
 *@return 0 if success
 */
int utl_audio_seek(UtlAudioHandle_t handle, int offset, int whence);

/**
 *@brief 获取音频文件信息
 *
 *@param handle Handle of audio file
 *@param info 音频信息
 *
 *@return 0 if success
 */
int utl_audio_get_fileinfo(UtlAudioHandle_t handle, UtlAudioInfo_t *info);

#endif /* UTL_AUDIO_H_ */
