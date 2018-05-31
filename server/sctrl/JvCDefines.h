
#pragma once

#include <stdint.h>

//StartCode-------------------------------------------------
#define JVSC900_STARTCODE	0x0453564A
#define DVR8004_STARTCODE	0x0553564A
#define JVSC950_STARTCODE	0x0653564A
#define JVSC951_STARTCODE	0x0753564A
#define JVSC920_STARTCODE	0x0953564A
#define JVSDEC05_STARTCODE	0x0A53564A

#define IPC3507_STARTCODE	0x1053564A
#define IPC_DEC_STARTCODE	0x1153564A //1080

#define JVSNVR_STARTCODE	0x2053564A

//设备类型
#define JVS_DEVICETYPE_CARD	0x0001
#define JVS_DEVICETYPE_DVR	0x0002
#define JVS_DEVICETYPE_IPC	0x0003
#define JVS_DEVICETYPE_NVR	0x0004
#define JVS_DEVICETYPE_JNVR	0x0005

//编码类型
#define JVS_VIDEOCODECTYPE_JENC04	0x0000	//04版编码器
#define JVS_VIDEOCODECTYPE_H264		0x0001	//标准h264
#define JVS_VIDEOCODECTYPE_H265		0x0002	//标准h265

#define JVS_AUDIOCODECTYPE_PCM			0x0000	//PCM原始数据
#define JVS_AUDIOCODECTYPE_AMR			0x0001
#define JVS_AUDIOCODECTYPE_G711_alaw	0x0002
#define JVS_AUDIOCODECTYPE_G711_ulaw	0x0003

//视频数据类型
#define JVS_VIDEODATATYPE_VIDEO			0x0000
#define JVS_VIDEODATATYPE_VIDEOANDAUDIO	0x0001

//视频模式
#define JVS_VIDEOFORMAT_PAL				0x0000
#define JVS_VIDEOFORMAT_NTSC			0x0001

//录像类型
#define JVS_RECFILETYPE_SV4				0x0000
#define JVS_RECFILETYPE_SV5				0x0001
#define JVS_RECFILETYPE_SV6				0x0002
#define JVS_RECFILETYPE_MP4				0x0003
#define JVS_RECFILETYPE_JVFS 			0x0004 //中维自定义文件系统

//typedef struct _JVS_FILE_HEADER
//{
//	uint32_t	dwFLAGS;			//中维录像文件头标志字段
//	uint32_t	dwFrameWidth;		//帧宽
//	uint32_t	dwFrameHeight;		//帧高
//	uint32_t	dwTotalFrames;		//总帧数
//	uint32_t	dwVideoFormat;		//源视频格式：0表示PAL，1表示NTSC
//	uint32_t	bThrowFrame;		//是否抽帧：0表示不抽帧，1表示抽帧（互联网模式）
//	uint32_t	dwSubFLAGS;			//子文件头标记(NVR用)
//	uint32_t	dwReserved2;		//保留字段
//} JVS_FILE_HEADER, *PJVS_FILE_HEADER;

typedef struct _JVS_FILE_HEADER_EX
{
	//老文件头，为兼容以前版本分控，保证其能正常预览
	uint8_t			ucOldHeader[32];//JVS_FILE_HEADER	oldHeader; //此处定义不可直接定义为JVS_FILE_HEADER类型，否则会有结构体成员对齐问题

	//结构体信息
	uint8_t			ucHeader[3];		//结构体识别码，设置为‘J','F','H'
	uint8_t			ucVersion;			//结构体版本号，当前版本为1

	//设备相关
	uint16_t		wDeviceType;		//设备类型

	//视频部分
	uint16_t		wVideoCodecID;		//视频编码类型
	uint16_t		wVideoDataType;		//数据类型
	uint16_t		wVideoFormat;		//视频模式
	uint16_t		wVideoWidth;		//视频宽
	uint16_t		wVideoHeight;		//视频高
	uint16_t		wFrameRateNum;		//帧率分子
	uint16_t		wFrameRateDen;		//帧率分母

	//音频部分
	uint16_t		wAudioCodecID;		//音频编码格式
	uint16_t		wAudioSampleRate;	//音频采样率
	uint16_t		wAudioChannels;		//音频声道数
	uint16_t		wAudioBits;			//音频采样位数

	//录像相关
	uint32_t		dwRecFileTotalFrames;	//录像总帧数
	uint16_t		wRecFileType;		//录像类型

	uint8_t 		   ucGrpcVersion; //GRPC版本号，非0表示支持GRPC
	uint8_t 		   ucReserved1[1]; //对齐用

	//保留位
	uint8_t 		   ucReserved[28];		  //请全部置0

} JVS_FILE_HEADER_EX, *PJVS_FILE_HEADER_EX;

//inline BOOL IsFILE_HEADER_EX(void *pBuffer, uint32_t dwSize)
//{
//	uint8_t *pacBuffer = (uint8_t*)pBuffer;
//
//	if(pBuffer == NULL || dwSize < sizeof(JVS_FILE_HEADER_EX))
//	{
//		return FALSE;
//	}
//
//	return pacBuffer[32] == 'J' && pacBuffer[33] == 'F' && pacBuffer[34] == 'H';
//}
