/*********************************************************************************
  *Copyright(C),2017, 涂鸦科技 www.tuya.comm
  *FileName:    tuya_ipc_chromecast.h
**********************************************************************************/
#ifndef TUYA_IPC_CHROMECAST_H
#define TUYA_IPC_CHROMECAST_H

#include "defines.h"
#include "tuya_cloud_types.h"

/**
 * \brief chromecast请求类型及处理结果
 * \enum CHROMECAST_REQUEST_TYPE
 */
typedef enum{
    CHROMECAST_REQUEST_START_SUCCESS = 0, /**< 收到chromecast开始请求，并正常开启服务 */
    CHROMECAST_REQUEST_STOP_SUCCESS, /**< 收到chromecast停止请求，并正常停止服务 */
    CHROMECAST_REQUEST_START_FAILED, /**< 收到chromecast开始请求，但开启服务失败 */
    CHROMECAST_REQUEST_STOP_FAILED, /**< 收到chromecast停止请求，但停止服务失败 */
}CHROMECAST_REQUEST_TYPE;

/**
 * \typedef typedef VOID (*CHROMECAST_REQUEST_CB)(CHROMECAST_REQUEST_TYPE type)
 * \brief CHROMECAST请求回调函数
 * \param[in] type 请求类型
 */
typedef VOID (*CHROMECAST_REQUEST_CB)(CHROMECAST_REQUEST_TYPE type);

/**
 * \brief chromecast发送缓存情况反馈
 * \enum CHROMECAST_BUFFER_STATUS_TYPE
 */
typedef enum{
    CHROMECAST_BUFFER_STATUS_25 = 0,
    CHROMECAST_BUFFER_STATUS_75,
}CHROMECAST_BUFFER_STATUS_TYPE;

/**
 * \typedef typedef VOID (*CHROMECAST_BUFFER_STATUS_CB)(CHROMECAST_BUFFER_STATUS_TYPE status)
 * \brief CHROMECAST发送缓存情况回调函数
 * \param[in] status 情况类型
 */
typedef VOID (*CHROMECAST_BUFFER_STATUS_CB)(CHROMECAST_BUFFER_STATUS_TYPE status);

/**
 * \brief CHROMECAST环境变量定义
 * \struct CHROMECAST_ENV
 */
typedef struct
{
    CHROMECAST_REQUEST_CB chromecast_request_cb;
    CHROMECAST_BUFFER_STATUS_CB chromecast_buffer_status_cb;
}CHROMECAST_ENV_S;

/**
 * \brief CHROMECAST视频编码格式
 * \enum CHROMECAST_VIDEO_CODEC_E
 */
typedef enum
{
    CHROMECAST_VIDEO_CODEC_H264 = 0, //只支持H264
}CHROMECAST_VIDEO_CODEC_E;

/**
 * \brief CHROMECAST视频帧类型
 * \enum CHROMECAST_VIDEO_NALU_TYPE_E
 * \note 不支持B帧
 */
typedef enum
{
    CHROMECAST_VIDEO_NALU_PSLICE = 1,                         /**<PSLICE types*/
    CHROMECAST_VIDEO_NALU_ISLICE = 5,                         /**<ISLICE types*/
    CHROMECAST_VIDEO_NALU_SEI    = 6,                         /**<SEI types*/
    CHROMECAST_VIDEO_NALU_SPS    = 7,                         /**<SPS types*/
    CHROMECAST_VIDEO_NALU_PPS    = 8,                         /**<PPS types*/
}CHROMECAST_VIDEO_NALU_TYPE_E;

/**
 * \brief CHROMECAST传输视频帧结构体定义
 * \struct CHROMECAST_VIDEO_FRAME_S
 */
typedef struct
{
    CHROMECAST_VIDEO_CODEC_E video_codec;
    CHROMECAST_VIDEO_NALU_TYPE_E video_nalu_type;
    BYTE *video_buf; /**< 视频帧buffer */
    UINT buf_len;  /**< 视频帧buffer长度 */
    UINT ntimestamp; /**<  视频时间戳 */
    UINT frame_rate; /**< 视频帧率 */

    VOID *p_reserved;
}CHROMECAST_VIDEO_FRAME_S;

/**
 * \brief CHROMECAST视频编码格式
 * \enum CHROMECAST_VIDEO_CODEC_E
 */
typedef enum
{
    CHROMECAST_AUDIO_CODEC_AAC_MAIN = 1,
    CHROMECAST_AUDIO_CODEC_AAC_LC = 2,//建议使用LC-AAC
    CHROMECAST_AUDIO_CODEC_AAC_SSR = 3,
    CHROMECAST_AUDIO_CODEC_AAC_LTP = 4,
}CHROMECAST_AUDIO_CODEC_E;

/**
 * \brief CHROMECAST音频采样率
 * \enum CHROMECAST_AUDIO_SAMPLE_E
 */
typedef enum
{
    CHROMECAST_AUDIO_SAMPLE_96000 = 0,
    CHROMECAST_AUDIO_SAMPLE_88200 = 1,
    CHROMECAST_AUDIO_SAMPLE_64000 = 2,
    CHROMECAST_AUDIO_SAMPLE_48000 = 3,
    CHROMECAST_AUDIO_SAMPLE_44100 = 4,
    CHROMECAST_AUDIO_SAMPLE_32000 = 5,
    CHROMECAST_AUDIO_SAMPLE_24000 = 6,
    CHROMECAST_AUDIO_SAMPLE_22050 = 7,
    CHROMECAST_AUDIO_SAMPLE_16000 = 8,
    CHROMECAST_AUDIO_SAMPLE_12000 = 9,
    CHROMECAST_AUDIO_SAMPLE_11025 = 10,
    CHROMECAST_AUDIO_SAMPLE_8000 = 11,//建议使用8000
    CHROMECAST_AUDIO_SAMPLE_7350 = 12,
}CHROMECAST_AUDIO_SAMPLE_E;

/**
 * \brief CHROMECAST音频位宽
 * \enum CHROMECAST_AUDIO_DATABITS_E
 */
typedef enum
{
    CHROMECAST_AUDIO_DATABITS_8 = 0,
    CHROMECAST_AUDIO_DATABITS_16 = 1,
}CHROMECAST_AUDIO_DATABITS_E;

/**
 * \brief CHROMECAST音频模式
 * \enum CHROMECAST_AUDIO_SOUNDMODE_E
 */
typedef enum
{
    CHROMECAST_AUDIO_SOUNDMODE_MONO = 1,
    CHROMECAST_AUDIO_SOUNDMODE_STEREO = 2,
}CHROMECAST_AUDIO_SOUNDMODE_E;

/**
 * \brief CHROMECAST传输视频帧结构体定义
 * \struct CHROMECAST_VIDEO_FRAME_S
 */
typedef struct
{
    CHROMECAST_AUDIO_CODEC_E audio_codec;
    CHROMECAST_AUDIO_SAMPLE_E audio_sample;
    CHROMECAST_AUDIO_DATABITS_E audio_databits;
	CHROMECAST_AUDIO_SOUNDMODE_E audio_soundmode;
    BYTE *audio_buf; /**< 音频帧buffer */
    UINT buf_len;  /**< 音频帧buffer长度 */
    UINT ntimestamp; /**<  音频时间戳 */

    VOID *p_reserved;
}CHROMECAST_AUDIO_FRAME_S;


/**
 * \typedef OPERATE_RET tuya_ipc_chromecast_init(IN CHROMECAST_ENV_S *env)
 * \brief CHROMECAST服务初始化函数
 * \param[in] env CHROMECAST请求回调函数
 */
OPERATE_RET tuya_ipc_chromecast_init(IN CHROMECAST_ENV_S *env);

/**
 * \typedef OPERATE_RET tuya_ipc_chromecast_deinit(VOID)
 * \brief CHROMECAST服务去初始化函数
 * \note 该函数会停止chromecast的后台服务，并且解除相关函数的注册
 */
OPERATE_RET tuya_ipc_chromecast_deinit(VOID);

/**
 * \typedef OPERATE_RET tuya_ipc_chromecast_send_video(CHROMECAST_VIDEO_FRAME_S* frame)
 * \brief CHROMECAST发送视频帧函数
 * \param[in] frame CHROMECAST视频信息结构体
 */
OPERATE_RET tuya_ipc_chromecast_send_video(CHROMECAST_VIDEO_FRAME_S *frame);

/**
 * \typedef OPERATE_RET tuya_ipc_chromecast_send_audio(CHROMECAST_AUDIO_FRAME_S *frame)
 * \brief CHROMECAST发送音频帧函数
 * \param[in] frame CHROMECAST音频信息结构体
 */
OPERATE_RET tuya_ipc_chromecast_send_audio(CHROMECAST_AUDIO_FRAME_S *frame);

#endif//TUYA_IPC_CHROMECAST_H

