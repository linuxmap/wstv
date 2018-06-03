/*********************************************************************************
  *Copyright(C),2017, 涂鸦科技 www.tuya.comm
  *FileName:    tuya_ipc_cloud_storage.h
**********************************************************************************/

#ifndef __TUYA_IPC_CLOUD_STORAGE_H__
#define __TUYA_IPC_CLOUD_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"

/**
 * \brief 云存储状态
 * \struct ClOUD_STORAGE_STATUS_E
 */
typedef enum
{
    ClOUD_STORAGE_STATUS_DISABLE,
    ClOUD_STORAGE_STATUS_START,
    ClOUD_STORAGE_STATUS_NORMAL,
    ClOUD_STORAGE_STATUS_UPLOADING,
    ClOUD_STORAGE_STATUS_HTTPC_ERROR,
    ClOUD_STORAGE_STATUS_ODER_INVALID,
    ClOUD_STORAGE_STATUS_FILE_NAME_ERROR,
    ClOUD_STORAGE_STATUS_COMMON_ERROR
}ClOUD_STORAGE_STATUS_E;

/**
 * \brief 云存储store mode
 * \struct ClOUD_STORAGE_TYPE_E
 */
typedef enum
{
    ClOUD_STORAGE_TYPE_CONTINUE,  /**< 连续上传云存储数据，结束以订单结束时间为准 */
    ClOUD_STORAGE_TYPE_EVENT,  /**< 事件区间上传云存储数据，结束以硬件告知sdk结束为准或订单告知结束为准 */ 
    ClOUD_STORAGE_TYPE_INVALID
}ClOUD_STORAGE_TYPE_E;

typedef enum
{
    ClOUD_STORAGE_VIDEO_FILE = 0,
    ClOUD_STORAGE_AUDIO_FILE,
    ClOUD_STORAGE_TS_FILE,
    ClOUD_STORAGE_PICTURE_FILE,
    ClOUD_STORAGE_INDEX_FILE,
    ClOUD_STORAGE_FILE_TYPE_MAX
}ClOUD_STORAGE_FILE_TYPE_E;

/**
 * \brief the nalu type of H264E
 * \struct ClOUD_STORAGE_H264_SLICE_E
 */
typedef enum 
{
     VIDEO_H264_NALU_P = 1,
     VIDEO_H264_NALU_I = 5,
     VIDEO_H264_NALU_SEI = 6,
     VIDEO_H264_NALU_SPS = 7,
     VIDEO_H264_NALU_PPS = 8,
     VIDEO_H264_NALU_IP = 9,
     VIDEO_H264_NALU_MAX
} ClOUD_STORAGE_H264_SLICE_E;

typedef enum
{
    VIDEO_H264_P_FRAME = 0,
    VIDEO_H264_I_FRAME = 1
}CLOUD_STORAGE_H264_FRAME_E;

/**
 * \fn OPERATE_RET tuya_ipc_cloud_storage_init
 * \param[in] frame_rate 视频帧率
 * \param[in] GOP_size  一个GOP的帧数
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_cloud_storage_init(IN INT frame_rate, IN INT GOP_size);

/**
 * \fn OPERATE_RET tuya_ipc_upload_h264_video_frame
 * \brief 往云服务器上传数据接口
 * \param[in] frame_buffer 上传的数据地址
 * \param[in] frame_size 上传的数据大小
 * \param[in] frame_type 上传的数据帧的类型
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_upload_h264_video_frame(IN CHAR* frame_buf, IN INT frame_size, IN INT frame_type);

#ifdef __cplusplus
}
#endif

#endif
