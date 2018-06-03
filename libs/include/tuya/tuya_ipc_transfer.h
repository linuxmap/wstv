/*********************************************************************************
  *Copyright(C),2017, 涂鸦科技 www.tuya.comm
  *FileName:    tuya_ipc_transfer.h
**********************************************************************************/

#ifndef __TUYA_IPC_TRANSFER_H__
#define __TUYA_IPC_TRANSFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"

/**
 * \brief 设备在线状态
 * \enum TRANSFER_ONLINE_E
 */
typedef enum
{
    TY_DEVICE_OFFLINE,
    TY_DEVICE_ONLINE,
}TRANSFER_ONLINE_E;

/**
 * \brief 音频编码格式
 * \enum TRANSFER_AUDIO_CODEC_E
 */
typedef enum
{
    TY_AUDIO_CODEC_AAC_ADTS,
    TY_AUDIO_CODEC_G711U,   /**< g711 u-law*/
    TY_AUDIO_CODEC_G711A,   /**< g711 a-law*/
    TY_AUDIO_CODEC_ADPCM,
    TY_AUDIO_CODEC_PCM,
    TY_AUDIO_CODEC_SPEEX,
    TY_AUDIO_CODEC_MP3,
    TY_AUDIO_CODEC_G726,
}TRANSFER_AUDIO_CODEC_E;

/**
 * \brief 音频采样率
 * \enum TRANSFER_AUDIO_SAMPLE_E
 */
typedef enum
{
    TY_AUDIO_SAMPLE_8K,
    TY_AUDIO_SAMPLE_11K,
    TY_AUDIO_SAMPLE_12K,
    TY_AUDIO_SAMPLE_16K,
    TY_AUDIO_SAMPLE_22K,
    TY_AUDIO_SAMPLE_24K,
    TY_AUDIO_SAMPLE_32K,
    TY_AUDIO_SAMPLE_44K,
    TY_AUDIO_SAMPLE_48K,
}TRANSFER_AUDIO_SAMPLE_E;

/**
 * \brief 音频位宽
 * \enum TRANSFER_AUDIO_DATABITS_E
 */
typedef enum
{
    TY_AUDIO_DATABITS_8,
    TY_AUDIO_DATABITS_16,
}TRANSFER_AUDIO_DATABITS_E;

/**
 * \brief 音频通道
 * \enum TRANSFER_AUDIO_CHANNEL_E
 */
typedef enum
{
    TY_AUDIO_CHANNEL_MONO,
    TY_AUDIO_CHANNEL_STERO,
}TRANSFER_AUDIO_CHANNEL_E;

/**
 * \brief IPC传输音频帧结构体定义
 * \struct TRANSFER_AUDIO_FRAME_S
 */
typedef struct
{
    TRANSFER_AUDIO_CODEC_E audio_codec;
    TRANSFER_AUDIO_SAMPLE_E audio_sample;
    TRANSFER_AUDIO_DATABITS_E audio_databits;
    TRANSFER_AUDIO_CHANNEL_E audio_channel;
    BYTE *p_audio_buf; /**< 音频帧buffer */
    UINT buf_len;  /**< 音频帧buffer长度 */	
    UINT timestamp; /**< 音频帧时间戳，单位为毫秒。若赋值为0，则采用sdk库内部提供的时戳 */

    VOID *p_reserved;
}TRANSFER_AUDIO_FRAME_S;

/**
 * \brief 视频编码格式
 * \enum TRANSFER_VIDEO_CODEC_E
 */
typedef enum
{
    TY_VIDEO_CODEC_MPEG4,
    TY_VIDEO_CODEC_H263,
    TY_VIDEO_CODEC_H264,
    TY_VIDEO_CODEC_MJPEG,
    TY_VIDEO_CODEC_H265,
}TRANSFER_VIDEO_CODEC_E;

/**
 * \brief 视频帧类型
 * \enum TRANSFER_VIDEO_FRAME_TYPE_E
 */
typedef enum
{
    TY_VIDEO_FRAME_PBFRAME,	   /**< P/B frame */
    TY_VIDEO_FRAME_IFRAME,     /**< I frame */
}TRANSFER_VIDEO_FRAME_TYPE_E;

/**
 * \brief 视频直播清晰度类型
 * \enum TRANSFER_VIDEO_CLARITY_TYPE_E
 */
typedef enum
{
    TY_VIDEO_CLARITY_STANDARD = 0, /**< 标清 */
    TY_VIDEO_CLARITY_HIGH,     /**< 高清 */
}TRANSFER_VIDEO_CLARITY_TYPE_E;

/**
 * \brief IPC传输视频帧结构体定义
 * \struct TRANSFER_VIDEO_FRAME_S
 */
typedef struct
{
    TRANSFER_VIDEO_CODEC_E video_codec;
    TRANSFER_VIDEO_FRAME_TYPE_E video_frame_type;
    BYTE *p_video_buf; /**< 视频帧buffer */
    UINT buf_len;  /**< 视频帧buffer长度 */	
    UINT timestamp; /**< 视频帧时间戳，单位为毫秒。若赋值为0，则采用sdk库内部提供的时戳 */

    VOID *p_reserved;
}TRANSFER_VIDEO_FRAME_S;

/**
 * \brief 直播模式下网络负载变更通知回调参数结构体
 * \note 当负载变更时，IPC传输模块会有回调信息通知用户，用户可以根据负载变更状况调整视频帧直播帧率等。
 * \struct EVENT_LIVE_LOAD_PARAM_S
 */
typedef struct
{
    INT curr_load_level; /**< 0:best 4:worst */
    INT new_load_level; /**< 0:best 4:worst */

    VOID *pReserved;
}EVENT_LIVE_LOAD_PARAM_S;

/**
 * \brief 回放模式下网络负载变更通知回调参数结构体
 * \note 当负载变更时，IPC传输模块会有回调信息通知用户，用户可以根据负载变更状况调整视频帧直播帧率等。
 * \struct EVENT_PLAYBACK_LOAD_PARAM_S
 */
typedef struct
{
    INT client_index; /**< 通道号 */
    INT curr_load_level; /**< 0:best 4:worst */
    INT new_load_level; /**< 0:best 4:worst */

    VOID *pReserved;
}EVENT_PLAYBACK_LOAD_PARAM_S;

/**
 * \brief 按月查询IPC本地视频通知回调参数结构体
 * \struct EVENT_PLAYBACK_QUERY_MONTH_SIMPLIFY_PARAM_S
 * \note 当APP侧按月查询IPC本地视频时，IPC传输模块会有回调信息通知用户，用户填充后返回。
 * \note
return_days:
UINT一共有32位，每1位表示对应天数是否有数据，最右边一位表示第0天。
比如 return_days = 26496 = B0110 0111 1000 0000
那么表示第7,8,9,10,13,14天有回放数据。
 */
typedef struct
{
    IN USHORT query_year; /**< 要查询的年份 */
    IN USHORT query_month; /**< 要查询的月份 */
    OUT UINT return_days; /**< 有回放数据的天 */
}EVENT_PLAYBACK_QUERY_MONTH_SIMPLIFY_PARAM_S;

/**
 * \brief 单个回放文件的时间戳信息
 * \struct PLAYBACK_TIME_TS_S
 */
typedef struct
{
    UINT start_timestamp; /**< 回放文件开始时间戳（以秒为单位） */
    UINT end_timestamp;   /**< 回放文件结束时间戳（以秒为单位） */
} PLAYBACK_TIME_TS_S;


/**
 * \struct PB_QUERY_DAY_TS_ARR_S
 */
typedef struct
{
    UINT file_count; /**< 当天有多少个回放文件 */
    PLAYBACK_TIME_TS_S file_arr[0]; /**< 回放文件数组 */
} PB_QUERY_DAY_TS_ARR_S;

/**
 * \brief 按天查询IPC本地视频通知回调参数结构体
 * \struct EVENT_PLAYBACK_QUERY_DAY_TS_PARAM_S
 * \note 当APP侧按天查询IPC本地视频时，IPC传输模块会有回调信息通知用户，用户填充后返回。
 * \note
p_query_arr:
该指针可调用API tuya_ipc_malloc_time_s 来获取，IPC传输模块会负责内存的释放。
 */
typedef struct
{
    IN USHORT query_year; /**< 要查询的年份 */
    IN UCHAR query_month; /**< 要查询的月份 */
    IN UCHAR query_day; /**< 要查询的天数 */

    OUT PB_QUERY_DAY_TS_ARR_S *p_query_ts_arr; /**< 用户返回的查询结果 */

    VOID *pReserved;
}EVENT_PLAYBACK_QUERY_DAY_TS_PARAM_S;

/**
 * \brief 回放模式下某个client申请开始回放通知回调参数结构体
 * \struct EVENT_PLAYBACK_START_PARAM_S
 * \note 当APP侧按天查询IPC本地视频时，IPC传输模块会有回调信息通知用户，用户填充后返回。
 * \note 1.当APP申请开始回放IPC本地视频时，IPC传输模块会有回调信息通知用户，用户可以根据回放时间点开始播放特定的本地视频。。
 * \note 2.当APP侧回放视频时进行拖动操作时，IPC传输模块会有回调信息通知用户，用户可以根据最新的回放时间点开始播放特定的本地视频。
 */
typedef struct
{
    INT client_index; /**< client id */
    UINT play_timestamp; /**< 回放的开始时间戳（以秒为单位）*/
    PLAYBACK_TIME_TS_S pb_file_info;
    VOID *pReserved;
}EVENT_PLAYBACK_START_TS_PARAM_S;

/**
 * \brief 回放模式下某个client申请暂停回放通知回调参数结构体
 * \struct EVENT_PLAYBACK_START_PARAM_S
 * \note 当APP申请暂停回放IPC本地视频时，IPC传输模块会有回调信息通知用户，用户可以暂停播放特定的本地视频。
 */
typedef struct
{
    INT client_index; /**< client id */
    VOID *pReserved;
}EVENT_PLAYBACK_PAUSE_PARAM_S;

/**
 * \brief 回放模式下某个client申请继续回放通知回调参数结构体
 * \struct EVENT_PLAYBACK_RESUME_PARAM_S
 * \note 当APP申请继续回放IPC本地视频时，IPC传输模块会有回调信息通知用户，用户可以继续播放特定的本地视频。
 */
typedef struct
{
    INT client_index; /**< client id */
    VOID *pReserved;
}EVENT_PLAYBACK_RESUME_PARAM_S;

/**
 * \brief 回放模式下某个client申请静音/取消静音通知回调参数结构体
 * \struct EVENT_PLAYBACK_MUTED_PARAM_S
 * \note 当APP在回放IPC本地视频时申请静音/取消静音，IPC传输模块会有回调信息通知用户，用户可以静音/取消静音。
 */
typedef struct
{
    INT client_index; /**< client id */
    INT muted; /**< 1:muted 0: un-muted */
    VOID *pReserved;
}EVENT_PLAYBACK_MUTED_PARAM_S;

/**
 * \brief 回放模式下某个client申请停止回放通知回调参数结构体
 * \struct EVENT_PLAYBACK_STOP_PARAM_S
 * \note 当APP停止回放IPC本地视频时，IPC传输模块会有回调信息通知用户，用户可以停止播放。
 */
typedef struct
{
    INT client_index; /**< client id */
    VOID *pReserved;
}EVENT_PLAYBACK_STOP_PARAM_S;

/**
 * \brief 直播模式下申请修改或者查询清晰度回调参数结构体
 * \struct EVENT_VIDEO_CLARITY_PARAM_S
 */
typedef struct
{
    TRANSFER_VIDEO_CLARITY_TYPE_E clarity; /**< 视频清晰度 */
    VOID *pReserved;
}EVENT_VIDEO_CLARITY_PARAM_S;

/**
 * \brief IPC传输模块回调信息类型定义
 * \struct TRANSFER_EVENT_E
 */
typedef enum
{
    TRANS_LIVE_VIDEO_START, /**< 开始视频直播，无参数 */
    TRANS_LIVE_VIDEO_STOP, /**< 停止视频直播，无参数 */
    TRANS_LIVE_VIDEO_CLARITY_SET, /**< 设置视频直播清晰度 */
    TRANS_LIVE_VIDEO_CLARITY_QUERY, /**< 查询视频直播清晰度 */
    TRANS_LIVE_AUDIO_START, /**< 开始音频直播，无参数 */
    TRANS_LIVE_AUDIO_STOP, /**< 停止音频直播，无参数 */
    TRANS_LIVE_LOAD_ADJUST, /**< 直播负载变更，参数为 EVENT_LIVE_LOAD_PARAM_S */
    TRANS_PLAYBACK_LOAD_ADJUST, /**< 开始回放，参数为 EVENT_PLAYBACK_LOAD_PARAM_S */

    TRANS_PLAYBACK_QUERY_MONTH_SIMPLIFY, /**< 按月查询本地视频信息，参数为 EVENT_PLAYBACK_QUERY_MONTH_SIMPLIFY_PARAM_S */
    TRANS_PLAYBACK_QUERY_DAY_TS, /**< 按天查询本地视频信息，参数为 EVENT_PLAYBACK_QUERY_DAY_TS_PARAM_S */

    TRANS_PLAYBACK_START_TS, /**< 开始回放视频，参数为 EVENT_PLAYBACK_START_TS_PARAM_S */
    TRANS_PLAYBACK_PAUSE, /**< 暂停回放视频，参数为 EVENT_PLAYBACK_PAUSE_PARAM_S */
    TRANS_PLAYBACK_RESUME, /**< 继续回放视频，参数为 EVENT_PLAYBACK_RESUME_PARAM_S */
    TRANS_PLAYBACK_MUTED, /**< 静音/取消静音，参数为 EVENT_PLAYBACK_MUTED_PARAM_S */
    TRANS_PLAYBACK_STOP, /**< 停止回放视频，参数为 EVENT_PLAYBACK_STOP_PARAM_S */

    TRANS_SPEAKER_START, /**< 开始对讲，无参数 */
    TRANS_SPEAKER_STOP,  /**< 停止对讲，无参数 */
}TRANSFER_EVENT_E;

/**
 * \typedef TRANSFER_EVENT_CB
 * \brief IPC传输模块回调函数定义
 * \param[in] event 回调信息定义
 * \param[in] args 回调信息附带的参数
 */
typedef VOID (*TRANSFER_EVENT_CB)(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID args);

/**
 * \typedef TRANSFER_REV_AUDIO_CB
 * \brief IPC对讲模式声音回调函数定义
 * \param [in] p_audio_frame APP侧发送的音频帧
 * \param [in] time_stamp 音频帧采集的时间戳
 * \param [in] frame_no 音频帧序号
 */
typedef VOID (*TRANSFER_REV_AUDIO_CB)(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT time_stamp, IN CONST UINT frame_no);

/**
 * \typedef TRANSFER_ONLINE_CB
 * \brief 设备在线状态回调函数定义
 * \param[in] status 设备在线状态
 */
typedef VOID (*TRANSFER_ONLINE_CB)(IN TRANSFER_ONLINE_E status);

/**
 * \brief 直播模式下的直播视频质量
 * \enum TRANS_LIVE_QUALITY_E
 */
typedef enum
{
    TRANS_LIVE_QUALITY_MAX = 0,     /**< ex. 640*480, 15fps, 320kbps (or 1280x720, 5fps, 320kbps) */
    TRANS_LIVE_QUALITY_HIGH,        /**< ex. 640*480, 10fps, 256kbps */
    TRANS_LIVE_QUALITY_MIDDLE,      /**< ex. 320*240, 15fps, 256kbps */
    TRANS_LIVE_QUALITY_LOW,         /**< ex. 320*240, 10fps, 128kbps */
    TRANS_LIVE_QUALITY_MIN,         /**< ex. 160*120, 10fps, 64kbps */
}TRANS_LIVE_QUALITY_E;

/**
 * \brief P2P传输基本信息结构体
 * \struct TUYA_IPC_TRANSFER_VAR_S
 */
typedef struct
{
    TRANSFER_ONLINE_CB online_cb;/**< 设备P2P在线状态回调 */

    TRANSFER_REV_AUDIO_CB on_rev_audio_cb; /**< 对讲模式下获取来自APP端的声音回调函数定义 */
    TRANSFER_AUDIO_CODEC_E rev_audio_codec; /**< 对讲模式下允许获取来自APP端的声音编码格式，固定参数为（8K，16BIT，MONO） */

    TRANSFER_EVENT_CB on_event_cb; /**< 涂鸦音视频传输模块事件回调响应 */
    TRANS_LIVE_QUALITY_E live_quality; /**< 直播时视频质量 */

    INT max_client_num;/**< 最大支持客户端数量 */
    INT resend_ratio;/**< ratio>0 ResendSize = 1024 * ratio; ratio<=0 skip; ratio=0 default*/

    VOID *p_reserved;
}TUYA_IPC_TRANSFER_VAR_S;

/**
 * \fn OPERATE_RET tuya_ipc_tranfser_init(IN CONST TUYA_IPC_TRANSFER_VAR_S *p_var)
 * \brief 启动涂鸦IPC传输模块
 * \note IPC传输模块需要在IPC模块已经激活后才可以启动。
 * \param[in] p_var 运行环境配置结构体
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_tranfser_init(IN CONST TUYA_IPC_TRANSFER_VAR_S *p_var);

/**
 * \fn OPERATE_RET tuya_ipc_tranfser_quit(VOID)
 * \brief 停止涂鸦IPC传输模块
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_tranfser_quit(VOID);

/**
 * \fn OPERATE_RET tuya_ipc_live_send_video_frame(IN CONST TRANSFER_VIDEO_FRAME_S *p_video_frame)
 * \brief IPC传输模块发送视频直播帧
 * \param[in] p_video_frame 视频直播帧
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_live_send_video_frame(IN CONST TRANSFER_VIDEO_FRAME_S *p_video_frame);

/**
 * \fn OPERATE_RET tuya_ipc_live_send_audio_frame(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame)
 * \brief IPC传输模块发送音频直播帧
 * \param[in] p_audio_frame 音频直播帧结构体指针
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_live_send_audio_frame(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame);

/**
 * \fn OPERATE_RET tuya_ipc_playback_send_video_frame(IN CONST UINT client, IN CONST TRANSFER_VIDEO_FRAME_S *p_video_frame)
 * \brief IPC传输模块发送视频回放帧
 * \param[in] client cliend id
 * \param[in] p_video_frame 视频回放帧
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_playback_send_video_frame(IN CONST UINT client, IN CONST TRANSFER_VIDEO_FRAME_S *p_video_frame);

/**
 * \fn OPERATE_RET tuya_ipc_playback_send_audio_frame(IN CONST UINT client, IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame)
 * \brief IPC传输模块发送音频回放帧
 * \param[in] client cliend id
 * \param[in] p_audio_frame 音频回放帧
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_playback_send_audio_frame(IN CONST UINT client, IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame);

/**
 * \fn OPERATE_RET tuya_ipc_playback_send_finish(IN CONST UINT client)
 * \brief 本地视频文件播放完成后告知client
 * \param[in] client cliend id
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_playback_send_finish(IN CONST UINT client);

/**
 * \fn OPERATE_RET tuya_ipc_malloc_time_s(IN CONST UINT num, OUT PLAYBACK_TIME_S **p_p_time_arr)
 * \brief 获取查询的指针数组内存
 * \param[in] num 要获取的数量
 * \param[out] p_p_time_arr 内存指针
 * \return OPERATE_RET
*/
OPERATE_RET tuya_ipc_malloc_time_s(IN CONST UINT num, OUT PB_QUERY_DAY_TS_ARR_S **p_p_time_arr);

/**
 * \brief 设备连接状态信息结构体
 * \struct TUYA_IPC_TRANSFER_VAR_S
 */
typedef struct
{
    UCHAR p2p_mode; /**< 0: P2P mode, 1: Relay mode, 2: LAN mode, 255: Not connected. */
    UCHAR local_nat_type; /**< The local NAT type, 0: Unknown type, 1: Type 1, 2: Type 2, 3: Type 3, 10: TCP only */
    UCHAR remote_nat_type; /**< The remote NAT type, 0: Unknown type, 1: Type 1, 2: Type 2, 3: Type 3, 10: TCP only */
    UCHAR relay_type; /**< 0: Not Relay, 1: UDP Relay, 2: TCP Relay */

    VOID *p_reserved;
}CLIENT_CONNECT_INFO_S;

/**
 * \fn OPERATE_RET tuya_ipc_get_client_conn_info(OUT UINT *p_client_num, OUT CLIENT_CONNECT_INFO_S **p_p_conn_info)
 * \brief IPC传输模块获取连接client的状态
 * \param[out] p_client_num 有效client的数目
 * \param[out] p_p_conn_info 指针数组
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_get_client_conn_info(OUT UINT *p_client_num, OUT CLIENT_CONNECT_INFO_S **p_p_conn_info);

/**
 * \fn OPERATE_RET tuya_ipc_free_client_conn_info(IN CLIENT_CONNECT_INFO_S *p_conn_info)
 * \brief 释放 tuya_ipc_get_client_conn_info 返回的指针
 * \param[in] p_conn_info 要释放的指针
 * \return OPERATE_RET
 */
OPERATE_RET tuya_ipc_free_client_conn_info(IN CLIENT_CONNECT_INFO_S *p_conn_info);


#ifdef __cplusplus
}
#endif

#endif
