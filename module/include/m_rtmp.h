/******************************************************************************

  Copyright (c) 2015 Jovision Technology Co., Ltd. All rights reserved.

  File Name     : m_rtmp.h
  Version       : 
  Author        : Wang Tao
  Created       : 2015-10-27

  Description   : RTMP portings for Net Alarm, call procedure: 
  Rtmp_SetURL()->Rtmp_Start()->Rtmp_SetMetadata()->Rtmp_SendData()->Rtmp_Stop()

  History       : 
  1.Date        : 2015-10-27
    Author      : Wang Tao
    Modification: Created file
******************************************************************************/
#ifndef _M_RTMP_H
#define _M_RTMP_H

#include "jv_common.h"
#include "JvMediaClient.h"

#define NET_ALARM_MAX_PLAY		3 //远程回放路数


#define RTMP_INVALID_HDL		(-1)
#define RTMP_LIVE_HDL(ch, st)	(((ch) * MAX_STREAM) + (st))
#define RTMP_PLAY_HDL(n)		((MAX_STREAM) + (n))


typedef	S32						RTMP_HDL;

// RTMP连接状态
typedef enum
{
	RTMP_DISCONNECTED,			// 未连接
	RTMP_CONNECTING,			// 连接中(等待回调)
	RTMP_CONNECTED,				// 已连接
	RTMP_WAITDISCONNECT			// 等待断开(连接中不能断开，需要等连接成功后再断开)
}RTMP_STATE_e;

typedef enum
{
	RTMP_EVENT_GETMETA,			// 获取MetaData
	RTMP_EVENT_DISCON			// 流媒体断开
}RTMP_EVENT_e;

typedef	void					(*RtmpCallback)(RTMP_EVENT_e eEvent, void* pParam, void* arg);


S32 Rtmp_Init();

S32 Rtmp_Deinit();

// 设置流媒体的URL
S32 Rtmp_SetURL(RTMP_HDL nHandle, const S8 *pURL);


// 开始流媒体连接
S32 Rtmp_Start(S32 nHandle);

S32 Rtmp_RegCallback(S32 nHandle, RtmpCallback pReqMetaData, void* arg);

/*更新RTMP码流信息*/
S32 Rtmp_UpdateMetadata(S32 nHandle);

S32 Rtmp_SendMetaData(RTMP_HDL nHandle, JMC_Metadata_t* pData);

S32 Rtmp_ConvDataType(S32 nType, U16 AudioCodec);

/*发送RTMP码流数据*/
S32 Rtmp_SendData(RTMP_HDL nHandle, S32 nType, S8 *pData, S32 nSize, S32 nPts, S32 nDts);

S32 Rtmp_GetStatus(RTMP_HDL nHandle);

S32 Rtmp_Stop(RTMP_HDL nHandle);

#endif /* _M_RTMP_H */

