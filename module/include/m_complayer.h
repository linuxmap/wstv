/****************************************************************************
文件描述    ：播放器模块: 根据指定文件、播放状态输出视频数据的通用播放器
创建日期    ：2015-11-11
创建者      ：王涛
修改记录	：
    1. 2015-11-11:创建
*****************************************************************************/

#ifndef _M_COM_PLAYER_H
#define _M_COM_PLAYER_H

#include "jv_common.h"

#if (SD_RECORD_SUPPORT)

#define	MAX_FILENAME_LEN		(256)


typedef void*	PLAYER_HANDLE;

typedef enum
{
	CPLAYER_CMD_PLAY,		// 播放
	CPLAYER_CMD_PAUSE,		// 暂停
	CPLAYER_CMD_RESUME,		// 继续
	CPLAYER_CMD_SEEK,		// 定位
	CPLAYER_CMD_FAST,		// 快进
	CPLAYER_CMD_SLOW,		// 慢放
	CPLAYER_CMD_SPEED,		// 指定速度播放
	CPLAYER_CMD_MUTE,		// 静音
	CPLAYER_CMD_UNMUTE,		// 取消静音
	CPLAYER_CMD_MAX
}CPLAYER_CMD_e;

typedef enum
{
	CPLAYER_TYPE_I,
	CPLAYER_TYPE_B,
	CPLAYER_TYPE_P,
	CPLAYER_TYPE_ALAW,
	CPLAYER_TYPE_ULAW,
	CPLAYER_TYPE_AAC,
	CPLAYER_TYPE_MAX
}CPLAYER_TYPE_e;

typedef enum
{
	CPLAYER_STATE_NOPLAY,	// 非回放状态
	CPLAYER_STATE_PLAY,		// 播放
	CPLAYER_STATE_PAUSE,	// 暂停
	CPLAYER_STATE_STEP,		// 单帧
	CPLAYER_STATE_SPEED,	// 快进或慢放
	CPLAYER_STATE_MAX
}CPlayState_e;

typedef enum
{
	CPLAYER_EVENT_KEEP,		// 保持连接，远程回放时防止断开连接
	CPLAYER_EVENT_END_OK,	// 正常播放完成
	CPLAYER_EVENT_END_ERR,	// 异常播放完成
	CPLAYER_EVENT_I_FRAME,	// 发送I帧
	CPLAYER_EVENT_PROGRESS,	// 进度更新事件
	CPLAYER_EVENT_WAITPLAY,	// 等待开始播放
	CPLAYER_EVENT_MAX
}CPlayEvent_e;

typedef struct
{
	U32				msTime;						// 录像总时长(ms)
	// 视频信息
	VIDEO_TYPE_E	nVDataType; 				// 视频编码格式
	S32 			nVTotalFrame;				// 总帧数
	S32 			nVWidth;					// 宽度
	S32 			nVHeight;					// 高度
	S32 			nVFrameRateNum; 			// 帧率分子
	S32 			nVFrameRateDen; 			// 帧率分母
	// 音频信息
	AUDIO_TYPE_E	nADataType; 				// 音频数据格式
	S32 			nASampleRate;				// 采样率
	S32 			nASampleBits;				// 采样位数
	S32 			nAChannels; 				// 声道数
}VideoFileInfo_t;

typedef struct
{
	S8				fname[MAX_FILENAME_LEN];
	CPlayState_e	PlayState;
	float			PlaySpeed;
	U32				nTotalFrame;
	U32				nCurFrame;
	BOOL			bMute;
}CPlayerStatus_t;

// 文件信息
typedef S32	(*FunSendMetaData)(const PLAYER_HANDLE Handle, const VideoFileInfo_t* pMetaData, void* arg);

// 视频/音频流
typedef S32 (*FunSendData)(const PLAYER_HANDLE Handle, CPLAYER_TYPE_e nType, const void* pData, S32 nSize, S32 nSeq, S32 nPts, S32 nDts, void* arg);

// 事件回调，播放完成等事件
typedef void (*FunEventCallback)(const PLAYER_HANDLE Handle, CPlayEvent_e nEvent, S32 Param1, S32 Param2, void* arg);


// 创建播放器
// fname			:要播放的文件名，NULL为只创建播放器，不播放文件
// pSendMetaData	:回调函数，发送MetaData
// pSendData		:回调函数，发送数据
// pSendData		:回调函数，事件
// arg				:回调时回传
// return			:非NULL 成功，NULL 失败
PLAYER_HANDLE ComPlayer_Create(const char *fname, FunSendMetaData pSendMetaData, FunSendData pSendData, FunEventCallback pEventFunc, void* arg);

// 关闭远程回放播放器
// Handle			:播放器Handle
// return			:0成功
S32 ComPlayer_Destroy(PLAYER_HANDLE Handle, BOOL bBlock);

// 获取文件信息
// Handle			:播放器Handle
// pFileInfo		:文件信息
// return			:0成功
S32 ComPlayer_GetFileInfo(PLAYER_HANDLE Handle, VideoFileInfo_t* pFileInfo);

// 设定/变更播放文件
// Handle			:播放器Handle
// fname			:文件名(包含路径)
// bSendMetaData	:是否需要发送MetaData
// return			:0成功
S32 ComPlayer_ChgPlayFile(PLAYER_HANDLE Handle, const char* fname, BOOL bSendMetaData);

// 变更播放状态
// Handle			:播放器Handle
// cmd				:播放指令
// Param			:参数，目前用作传定位帧号和播放速度等级
// return			:成功返回0，失败返回负值
S32 ComPlayer_ChgPlayState(PLAYER_HANDLE Handle, CPLAYER_CMD_e cmd, void* Param);

// 获取播放状态
// Handle			:播放器Handle
// pState			:播放状态
// return			:成功返回0，失败返回负值
S32 ComPlayer_GetPlayStatus(PLAYER_HANDLE Handle, CPlayerStatus_t* pState);

#endif

#endif

