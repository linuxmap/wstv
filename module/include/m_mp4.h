#ifndef _M_MP4_H_
#define _M_MP4_H_

#include "Jmp4pkg.h"
#include "jv_common.h"

#define _FOPEN_BUF_SIZE (64*1024)

typedef struct
{
	unsigned int 		iDataStart;					//读取没封装完毕的文件需要用到。
	
	BOOL			bNormal;						// TRUE, 表示已完成封装,FALSE, 封装正在进行中
	
	BOOL				bHasVideo;					// 是否有视频
	BOOL				bHasAudio;					// 是否有音频

	unsigned int		iFrameWidth;				// 视频宽度
	unsigned int		iFrameHeight;				// 视频高度
	double				dFrameRate;					// 帧速
	unsigned int		iNumVideoSamples;			// VideoSample个数
	unsigned int		iNumAudioSamples;			// AudioSample个数
	MP4_AVCC			avcC;						// mp4 sps\pps

	char				szVideoMediaDataName[8];	// 视频编码名字 "avc1"
	char				szAudioMediaDataName[8];	// 音频编码名字 "samr" "alaw" "ulaw"	
	AUDIO_TYPE_E		nMp4AudioType;
	VIDEO_TYPE_E		enMp4VideoType;
}MP4_READ_INFO;


//打开MP4读文件
//strFile			:文件名
//pInfo 			:文件的信息
//return NULL 失败，>0 成功
void *MP4_Open_Read(char *strFile, MP4_READ_INFO *pInfo);

//关闭MP4读文件
void MP4_Close_Read(void *handle, MP4_READ_INFO *pInfo);

//读取MP4文件I帧
//handle  MP4文件打开的句柄
//pInfo   文件信息
//pPack   解封h264或amr帧数据结构体 
//bForword  表示查找方向, TRUE向前(往右), FALSE, 向后(往左) 
//return    FALSE失败  TRUE 成功
BOOL MP4_ReadIFrame(void *handle, MP4_READ_INFO *pInfo, PAV_UNPKT pPack, BOOL bForword);

//读取MP4文件一帧数据
//handle  MP4文件打开的句柄
//pInfo   文件信息
//pPack   解封h264或amr帧数据结构体 
//return    正在进行封装的 返回0表示无数据可读，否则表示有数据读，对于完成封装的 FALSE失败  TRUE 成功
BOOL MP4_ReadOneFrame(void *handle, MP4_READ_INFO *pInfo, PAV_UNPKT pPack);

//获取和iFrameVideo同步的音频帧号
int MP4_GetSyncAudioFrame(void *handle, MP4_READ_INFO *pInfo, int iFrameVideo);

#endif

