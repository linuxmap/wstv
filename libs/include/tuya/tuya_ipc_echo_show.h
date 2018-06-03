/*********************************************************************************
  *Copyright(C),2017, 涂鸦科技 www.tuya.comm
  *FileName:    tuya_echo_show.h
**********************************************************************************/

#ifndef __TUYA_IPC_ECHO_SHOW_H__
#define __TUYA_IPC_ECHO_SHOW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"
#include "tuya_ipc_media.h"



/**
 * \fn OPERATE_RET tuya_ipc_echo_show_init(VOID)
 * \brief 注册echo show后台服务任务
 * \param[in] convertToULaw To convert 16-bit PCM data to 8-bit u-law or not, prior to streaming
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_echo_show_init(IN IPC_MEDIA_INFO_S minfo);

/**
 * \fn UINT tuya_ipc_video_fifo_put(IN CHAR *buf,IN UINT len)
 * \brief 该函数是注入H264的视频帧数据,考虑到服务器转发带宽，
 * \brief 所以目前仅仅支持CIF, QCIF,QVGA格式大小的视频数据
 * \param[in] buf 视频帧数据指针
 * \param[in] len 是传入的视频帧数据长度
 * \return UINT
 */
UINT tuya_ipc_video_fifo_put(IN CHAR *buf,IN UINT len);

/**
 * \fn UINT tuya_ipc_audio_fifo_put(IN CHAR *buf,IN UINT len)
 * \brief 该函数是注入PCM的音频数据，考虑云端数据转发越少越好，
 * \brief 目前该库支持PCM的16bit，单channel，采样率8000， 5*160=800的pcm音频数据传入
 * \param[in] buf 音频数据指针
 * \param[in] len 传入的音频数据长度
 * \return UINT
 */
UINT tuya_ipc_audio_fifo_put(IN CHAR *buf,IN UINT len);

#ifdef __cplusplus
}
#endif

#endif
