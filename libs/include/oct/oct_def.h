/**
 * @file oct_def.h
 * @brief Oct类型定义
 * @author 程行通
 * @copyright Copyright (c) 2017 Jovision
 */

#pragma once

#ifdef _WIN32
#  define OCT_C_API _declspec(dllexport)
#  define OCT_C_CALLTYPE _stdcall
#else
#  define OCT_C_API __attribute__((visibility("default")))
#  define OCT_C_CALLTYPE
#endif

#include <stdint.h>

/* 错误码 */
#define OCT_ERRORCODE_NOERROR                         0 /*!< 无错误 */
#define OCT_ERRORCODE_ERROR                          -1 /*!< 未知错误 */
#define OCT_ERRORCODE_INVALID_PARAM                  -2 /*!< 无效参数 */
#define OCT_ERRORCODE_INVALID_HANDLE                 -3 /*!< 无效的句柄 */
#define OCT_ERRORCODE_USER_ABORT                     -4 /*!< 用户中断请求 */
#define OCT_ERRORCODE_PARSE_ADDR_FAILED              -5 /*!< 地址解析失败 */
#define OCT_ERRORCODE_CREATE_SOCKET_FAILED           -6 /*!< 创建套接字失败 */
#define OCT_ERRORCODE_SYSTEM_CALL_FAILED             -7 /*!< 系统调用失败 */
#define OCT_ERRORCODE_CONNECT_TIMEOUT                -8 /*!< 连接超时 */
#define OCT_ERRORCODE_CONNECT_FAILED                 -9 /*!< 连接失败 */
#define OCT_ERRORCODE_USER_VERIFY_FAILED            -10 /*!< 用户验证失败 */
#define OCT_ERRORCODE_CONNECTION_ABORT              -11 /*!< 连接异常断开 */
#define OCT_ERRORCODE_ALLOC_MEMORY_FAILED           -12 /*!< 内存分配失败 */
#define OCT_ERRORCODE_TIMEOUT                       -13 /*!< 操作超时 */
#define OCT_ERRORCODE_ALLOC_CONTEXT_FAILED          -14 /*!< 分配上下文失败 */
#define OCT_ERRORCODE_START_THREAD_FAILED           -15 /*!< 启动工作线程失败 */
#define OCT_ERRORCODE_INIT_RES_FAILED               -16 /*!< 初始化资源失败 */
#define OCT_ERRORCODE_UNINIT_RES                    -17 /*!< 未初始化的资源 */
#define OCT_ERRORCODE_SERIAL_DATA_FAILED            -18 /*!< 序列化数据失败 */
#define OCT_ERRORCODE_PARSE_DATA_FAILED             -19 /*!< 解析数据失败 */
#define OCT_ERRORCODE_SEND_DATA_FAILED              -20 /*!< 发送数据失败 */
#define OCT_ERRORCODE_RECV_DATA_FAILED              -21 /*!< 接收数据失败 */
#define OCT_ERRORCODE_BIND_ADDR_FAILED              -22 /*!< 绑定地址失败 */
#define OCT_ERRORCODE_BIND_SERVICE_FAILED           -23 /*!< 绑定网络传输服务失败 */
#define OCT_ERRORCODE_REPEAT_CALL                   -24 /*!< 重复调用 */
#define OCT_ERRORCODE_INVALID_UOID                  -25 /*!< 无效的设备证书 */
#define OCT_ERRORCODE_INVALID_EID                   -26 /*!< 无效的设备识别码 */
#define OCT_ERRORCODE_DEV_NOT_ONLINE                -27 /*!< 设备未上线 */
#define OCT_ERRORCODE_SERVER_ERROR                  -28 /*!< 服务器错误 */
#define OCT_ERRORCODE_PEER_NOT_READY                -29 /*!< 对端未就绪 */
#define OCT_ERRORCODE_PEER_HAVE_NO_RES              -30 /*!< 对端没有足够资源 */
#define OCT_ERRORCODE_SERVER_REJECT_DEV_ONLINE      -31 /*!< 服务器拒绝设备上线 */
#define OCT_ERRORCODE_SERVER_ABORT_ONLINE           -32 /*!< 服务器中断设备上线状态 */
#define OCT_ERRORCODE_OPEN_FILE_FAILED              -33 /*!< 打开文件失败 */
#define OCT_ERRORCODE_OVER_LIMIT                    -34 /*!< 超过限制 */
#define OCT_ERRORCODE_RELAY_CONN_DURATION_LIMITED   -35 /*!< 超过单次连接转发时长限制 */
#define OCT_ERRORCODE_NO_SUCH_STREAM                -36 /*!< 没有符合条件的流 */
#define OCT_ERRORCODE_NO_DEV_INFO                   -37 /*!< 没有设置设备信息 */

/* 扩展错误码(外部使用) */
#define OCT_ERRORCODE_EX_MIN_VALUE                  -1000 /*!< 扩展错误码起始值 */
#define OCT_ERRORCODE_EX_PERM_DENIED                -1001 /*!< 权限被拒绝 */
#define OCT_ERRORCODE_EX_SERV_OCCUPIED              -1002 /*!< 服务被占用 */

/* 日志等级 */
#define OCT_LOGLEVEL_ALL    0 /*!< 所有 */

#define OCT_LOGLEVEL_DEBUG  1 /*!< 调试 */
#define OCT_LOGLEVEL_INFO   2 /*!< 信息 */
#define OCT_LOGLEVEL_NOTIFY 3 /*!< 通知 */
#define OCT_LOGLEVEL_WARN   4 /*!< 警告 */
#define OCT_LOGLEVEL_ERROR  5 /*!< 错误 */

#define OCT_LOGLEVEL_NONE   6 /*!< 无 */

/* 连接选项 */
#define OCT_CONNFLAGS_TCP           0x00000000  /*!< 优先使用TCP协议连接 */
#define OCT_CONNFLAGS_UDP           0x00000001  /*!< 优先使用UDP协议连接 */
#define OCT_CONNFLAGS_NOP2P         0x00000002  /*!< 不尝试P2P穿透连接 */
#define OCT_CONNFLAGS_NOLOCALTRY    0x00000004  /*!< 不尝试本地直接连接 */

/* 传输服务选项 */
#define OCT_TRANS_FLAGS_WITH_UDP   0x00000001  /*!< UDP协议支持 */
#define OCT_TRANS_FLAGS_WITH_TCP   0x00000002  /*!< TCP协议支持 */
#define OCT_TRANS_FLAGS_WITH_IPV6  0x00000004  /*!< IPv6支持 */

/* 连接事件类型 */
#define OCT_CONNECT_EVENT_CONNECT       1  /*!< 连接操作通知 */
#define OCT_CONNECT_EVENT_PEERCONNECT   2  /*!< 对端连接通知 */
#define OCT_CONNECT_EVENT_DISCONNECT    3  /*!< 连接断开通知 */
#define OCT_CONNECT_EVENT_SWITCH_CONN   4  /*!< 切换底层连接方式 */

/* 连接类型 */
#define OCT_CONNECT_TYPE_UDP_P2P        0  /*!< UDP点对点直连 */
#define OCT_CONNECT_TYPE_TCP_P2P        1  /*!< TCP点对点直连 */
#define OCT_CONNECT_TYPE_UDP_TURN       2  /*!< UDP转发连接 */
#define OCT_CONNECT_TYPE_TCP_TURN       3  /*!< TCP转发连接 */

/* 连接类型掩码 */
#define OCT_CONNECT_TYPE_MASK_UDP       0x00
#define OCT_CONNECT_TYPE_MASK_TCP       0x01
#define OCT_CONNECT_TYPE_MASK_P2P       0x00
#define OCT_CONNECT_TYPE_MASK_TURN      0x02

/* 连接事件类型 */
#define OCT_ONLINE_EVENT_ONLINEOK       1  /*!< 设备上线成功 */
#define OCT_ONLINE_EVENT_ONLINEFAILED   2  /*!< 设备上线失败 */
#define OCT_ONLINE_EVENT_OFFLINE        3  /*!< 设备下线 */

/* 服务模块 */
#define OCT_SERVICEID_MAXSIZE       1024    /*!< 最大服务ID */
#define OCT_SERVICEID_USERCUSTOM    128     /*!< 用户自定义服务起始ID */

/* RPC命令ID */
#define OCT_RPC_COMMAND_USERCUSTOM  1024    /*!< 用户自定义命令起始ID */

/* 客户端类型 */
#define OCT_CLIENTTYPE_UNKNOWN      0 /*!< 未知客户端 */
#define OCT_CLIENTTYPE_NVR          1 /*!< NVR设备 */
#define OCT_CLIENTTYPE_CV           2 /*!< CV客户端 */
#define OCT_CLIENTTYPE_JNVR         3 /*!< 中维高清客户端 */
#define OCT_CLIENTTYPE_CLOUDSEE     4 /*!< CloudSEE手机APP */
#define OCT_CLIENTTYPE_NVSIP        5 /*!< NVSIP手机APP */
#define OCT_CLIENTTYPE_SOOVVI       6 /*!< 小维智慧家庭 */
#define OCT_CLIENTTYPE_WEB_CLIENT   7 /*!< Web客户端 */


/**
 * @brief 设备授权信息
 */
typedef struct _oct_uoid_info_t
{
    const char* eid;            /*!< 设备名 */

    int enterprise_code;        /*!< 企业编码, 有效取值范围0x041-0x3FF, 0x000表示未知企业 */
    int region_code;            /*!< 区域编码, 有效取值范围0x041-0x3FF, 0x000表示未知区域 */
    int serial_num;             /*!< 序列号, 有效取值范围0x00000001-0x3FFFFFFF */
    int check_code;             /*!< 校验码 */

    int max_channels;           /*!< 最大通道数 */
    int max_conns;              /*!< 最大连接数 */
} oct_uoid_info_t;

/**
 * @brief 设备信息
 */
typedef struct _octs_dev_info_t
{
    const char* name;           /*!< 设备名 */
    uint32_t dev_type;          /*!< 设备主类型(0:未知;1:板卡;2:DVR;3:普通IPC;4:家用IPC;5:NVR) */
    uint32_t dev_sub_type;      /*!< 设备子类型 */
    int channel_num;            /*!< 设备视频通道数(通道号1开始) */
    int sub_stream_num;         /*!< 每通道子码流个数(子码流号1开始，码流质量由高到低排列) */
    const char* props;          /*!< 设备属性(JSON格式) */
} octs_dev_info_t;

/**
 * @brief 设备信息
 */
typedef struct _octc_dev_info_t
{
    const char* uoid_eid;       /*!< 设备ID */
    const char* name;           /*!< 设备名 */
    uint32_t dev_type;          /*!< 设备主类型(0:未知;1:板卡;2:DVR;3:普通IPC;4:家用IPC;5:NVR) */
    uint32_t dev_sub_type;      /*!< 设备子类型 */
    int channel_num;            /*!< 设备视频通道数(通道号1开始) */
    int sub_stream_num;         /*!< 每通道子码流个数(子码流号1开始，码流质量由高到低排列) */
    const char* props;          /*!< 设备属性(JSON格式) */
} octc_dev_info_t;

/**
 * @brief 日期时间
 */
typedef struct _oct_datetime_t
{
    int year;       /*!< 年 */
    int month;      /*!< 月 */
    int day;        /*!< 日 */
    int hour;       /*!< 时 */
    int minute;     /*!< 分 */
    int second;     /*!< 秒 */
    int millisec;	/*!< 毫秒数 */
} oct_datetime_t;

/**
 * @brief 获取设备信息返回数据
 */
typedef struct _oct_get_dev_info_result_t
{
    int count;                   /*!< 设备数量 */
    octc_dev_info_t* dev_info;    /*!< 设备信息  */
}oct_get_dev_info_result_t;

/**
 * @brief 设备在线状态
 */
typedef struct _octc_dev_online_status_t
{
    const char* uoid_eid;        /*!< 设备ID */
    int online;                  /*!< 设备是否在线，是：1，否：0 */
}octc_dev_online_status_t;

/**
 * @brief 获取设备在线状态返回数据
 */
typedef struct _oct_get_dev_online_status_result_t
{
    int count;                                       /*!< 设备数量 */
    octc_dev_online_status_t* dev_online_status;     /*!< 设备在线信息  */
}oct_get_dev_online_status_result_t;

/* 回调函数 */
/**
 * @brief 设备上线通知回调
 * @param type 事件类型(OCT_ONLINE_EVENT_*)
 * @param ec 错误代码
 * @param em 错误信息
 */
typedef void (*oct_online_event_proc_t)(int type, int ec, const char* em);

/**
 * @brief 用户认证回调(仅主控端)
 * @param conn 连接
 * @param user 用户名
 * @param pwd 密码
 * @param host 对端主机名
 * @return 成功返回0，失败返回负数错误码
 */
typedef int (*oct_verify_user_proc_t)(int conn, const char* user, const char* pwd, const char* host);

/**
 * @brief 连接事件回调
 * @param conn 连接
 * @param type 事件类型(OCT_CONNECT_EVENT_*)
 * @param ctype 连接类型(OCT_CONNECT_TYPE_*)
 * @param ec 错误代码，连接成功返回0
 * @param em 错误描述信息
 * @param host 对端主机名
 * @param client_type 客户端类型(仅对EDK有效，OCT_CLIENTTYPE_*)
 */
typedef void (*oct_connnect_event_proc_t)(int conn, int type, int ctype, int ec, 
    const char* em, const char* host, int client_type);

/**
 * @brief 消息处理回调
 * @param conn 连接
 * @param stream 流ID
 * @param service 服务ID
 * @param data 静载数据
 * @param size 静载数据长度
 */
typedef void (*oct_msg_proc_t)(int conn, int stream, int service, const uint8_t* data, int size);

// 直播点播服务模块----------------------------------------------------------------------------------------
/**
 * @brief 视频编码
 */
typedef enum _oct_stream_video_codec_t
{
    OCT_SVC_NONE   = 0, /*!< 无视频数据 */
    OCT_SVC_H264   = 1, /*!< h264 编码 */
    OCT_SVC_H265   = 2, /*!< h265 编码 */
} oct_stream_video_codec_t;

/**
 * @brief 音频编码
 */
typedef enum _oct_stream_audio_codec_t
{
    OCT_SAC_NONE   = 0, /*!< 无视频数据 */
    OCT_SAC_ALAW   = 1, /*!< a-law/g711a 编码 */
    OCT_SAC_ULAW   = 2, /*!< u-law/mu-law/g711u 编码 */
    OCT_SAC_AAC    = 3, /*!< aac 编码 */
    OCT_SAC_AMR    = 4, /*!< AMR 编码 */
    OCT_SAC_G729   = 5, /*!< G729 编码 */
} oct_stream_audio_codec_t;

/**
 * @brief 帧类型
 */
typedef enum _oct_stream_frame_type_t
{
    OCT_SFT_UNKNOWN,    /*!< 未知类型 */
    OCT_SFT_METADATA,   /*!< 元数据帧(oct_stream_metadata_t 类型) */
    OCT_SFT_VIDEO,      /*!< 视频帧(oct_stream_video_frame_t 类型) */
    OCT_SFT_AUDIO,      /*!< 音频帧(oct_stream_audio_frame_t 类型) */
} oct_stream_frame_type_t;

/**
 * @brief 视频帧类型
 */
typedef enum _oct_video_frame_type_t
{
    OCT_VFT_UNKNOWN             = 0, /*!< 未知类型 */
    OCT_VFT_KEY_FRAME           = 1, /*!< 关键帧 */
    OCT_VFT_NORMAL_FRAME        = 2, /*!< 普通帧 */
    OCT_VFT_VIRTUAL_KEY_FRAME   = 3, /*!< 虚拟关键帧 */
} oct_video_frame_type_t;

/**
 * @brief 设备端直播事件类型
 */
typedef enum _octs_stream_event_type_t
{
    OCTS_SET_NEW_CLIENT,        /*!< 新客户端接入(无参数) */
    OCTS_SET_INSERT_KEYFRAME,   /*!< 客户端请求插入关键帧(无参数) */
    OCTS_SET_OPEN,              /*!< 打开流事件(等价于OCTS_SET_NEW_CLIENT) */
    OCTS_SET_CLOSE,             /*!< 关闭流事件 */
} octs_stream_event_type_t;

/**
 * @brief 客户端直播事件类型
 */
typedef enum _octc_stream_event_type_t
{
    OCTC_SET_OPEN,              /*!< 打开流事件(ec为0表示开启成功) */
    OCTC_SET_CLOSE,             /*!< 关闭流事件 */
} octc_stream_event_type_t;

/**
 * @brief 点播事件类型
 */
typedef enum _octs_vod_event_type_t
{
    OCTS_VET_START,     /*!< 开始播放(param1：毫秒单位起始位置 param2：录像文件名) */
                        /*!<         (时间轴模式下：param1：通道号 param2：起始播放位置,oct_datetime_t* 类型) */
    OCTS_VET_STOP,      /*!< 停止播放(无参数) */
    OCTS_VET_SET_SPEED, /*!< 设置播放速度(param1：播放速度 speed=0:暂停;speed>0:speed倍速播放;speed<0:1/(-speed)倍速播放) */
    OCTS_VET_SKIP_TO,   /*!< 跳转到(param1：毫秒单位跳转位置) */
                        /*!<       (时间轴模式下：param2：跳转位置,oct_datetime_t* 类型) */
} octs_vod_event_type_t;

/**
 * @brief 点播事件类型
 */
typedef enum _octc_vod_event_type_t
{
    OCTC_VET_OPEN,      /*!< 打开流事件(ec为0表示开启成功) */
    OCTC_VET_CLOSE,     /*!< 关闭流事件 */
} octc_vod_event_type_t;

/**
 * @brief 流元数据帧
 */
typedef struct _oct_stream_metadata_t
{
    oct_stream_video_codec_t video_codec;   /*!< 视频编码 */
    int video_width;                        /*!< 视频宽 */
    int video_height;                       /*!< 视频高 */
    int video_framerate;                    /*!< 视频帧率 * 10000 */
    oct_stream_audio_codec_t audio_codec;   /*!< 音频编码 */
    int audio_channels;                     /*!< 音频通道数 */
    int audio_sample_rate;                  /*!< 音频采样率 */
    int audio_sample_bits;                  /*!< 音频采样位数 */
    int duration;                           /*!< 毫秒单位持续时长(仅点播模式) */
    uint8_t* user_defined;                  /*!< 用户自定义数据*/    
    int user_defined_len;                   /*!< 用户自定义数据长度*/
} oct_stream_metadata_t;

/**
 * @brief 视频帧
 */
typedef struct _oct_stream_video_frame_t
{
    oct_video_frame_type_t type;    /*!< 帧类型 */
    const uint8_t* playload;        /*!< 静载数据 */
    int playload_length;            /*!< 静载数据长度 */
    uint64_t timestamp;             /*!< 毫秒单位时间搓 */
} oct_stream_video_frame_t;

/**
 * @brief 音频帧
 */
typedef struct _oct_stream_audio_frame_t
{
    const uint8_t* playload;    /*!< 静载数据 */
    int playload_length;        /*!< 静载数据长度 */
    uint64_t timestamp;         /*!< 毫秒单位时间搓 */
} oct_stream_audio_frame_t;

/**
 * @brief 消息处理回调
 * @param conn 连接
 * @param stream 流ID
 * @param type 帧类型
 * @param frame 帧数据,为NULL表示对端关闭流
 */
typedef void (*octc_stream_frame_proc_t)(int conn, int stream, oct_stream_frame_type_t type, void* frame);

/**
 * @brief 事件处理回调
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param ec 错误码
 * @param em 错误信息
 */
typedef void (*octc_stream_event_proc_t)(int conn, int stream, octc_stream_event_type_t type, int ec, const char* em);

/**
 * @brief 直播事件处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param channel 通道
 * @param sub_stream 子码流
 * @param type 事件类型
 * @param param1 参数1
 * @param param2 参数2
 * @return 成功返回0，失败返回错误码
 */
typedef int (*octs_stream_event_proc_t)(int conn, int stream, int channel, int sub_stream,
    octs_stream_event_type_t type, int param1, void* param2);

/**
 * @brief 客户端点播事件处理回调
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param ec 错误码
 * @param em 错误信息
 */
typedef void (*octc_vod_event_proc_t)(int conn, int stream, octc_vod_event_type_t type, int ec, const char* em);

/**
 * @brief 设备端点播事件处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param param1 参数1
 * @param param2 参数2
 * @return 成功返回0，失败返回错误码
 */
typedef int (*octs_vod_event_proc_t)(int conn, int stream, octs_vod_event_type_t type, int param1, void* param2);

// 下载服务模块--------------------------------------------------------------------------------------------
/**
 * @brief 设备端下载事件类型
 */
typedef enum _octs_down_event_type_t
{
    OCTS_DET_AUTHOR,      /*!< 授权验证(param2：下载文件名) */
} octs_down_event_type_t;

/**
 * @brief 客户端下载事件类型
 */
typedef enum _octc_down_event_type_t
{
    OCTC_DET_OPEN,      /*!< 开始下载(ec为0表示开启成功) */
    OCTC_DET_CLOSE,     /*!< 结束下载 */
} octc_down_event_type_t;

/**
 * @brief 下载包
 */
typedef struct _oct_down_package_t
{
    int pkg_id;                 /*!< 包索引 */
    const uint8_t* playload;    /*!< 静载数据 */
    int playload_length;        /*!< 静载数据长度 */
    uint64_t file_size;         /*!< 文件长度 */
    uint64_t send_pos;          /*!< 发送当前包后的发送位置 */
} oct_down_package_t;

/**
 * @brief 下载数据包处理回调
 * @param conn 连接
 * @param stream 流ID
 * @param package 数据包,为NULL表示对端关闭流
 */
typedef void (*octc_down_package_proc_t)(int conn, int stream, const oct_down_package_t* package);

/**
 * @brief 客户端下载事件处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param param1 参数1
 * @param param2 参数2
 */
typedef void (*octc_down_event_proc_t)(int conn, int stream, octc_down_event_type_t type, int ec, const char* em);

/**
 * @brief 设备端下载事件处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param param1 参数1
 * @param param2 参数2
 * @return 成功返回0，失败返回错误码
 */
typedef int (*octs_down_event_proc_t)(int conn, int stream, octs_down_event_type_t type, int param1, void* param2);

/**
 * @brief 打开文件函数
 * @param conn 连接
 * @param stream 流ID
 * @param filename 文件名
 * @param start_pos 起始文件偏移
 * @param user_data 保存一个用户数据指针
 * @return 成功返回文件大小，失败返回负数
 */
typedef int64_t (*octs_down_open_file_proc_t)(int conn, int stream, const char* filename, int64_t start_pos, void** user_data);

/**
 * @brief 获取下次读取的数据块大小
 * @param conn 连接
 * @param stream 流ID
 * @param user_data 用户数据指针
 * @return 返回下次读取块大小,返回0表示文件读取结束
 */
typedef int (*octs_down_get_next_block_size_proc_t)(int conn, int stream, void* user_data);

/**
 * @brief 读取块
 * @param conn 连接
 * @param stream 流ID
 * @param buf 数据缓存
 * @param size 要读取的大小(通过octs_down_get_next_block_size_proc_t获取)
 * @param user_data 用户数据指针
 * @return 成功返回0，失败返回负数
 */
typedef int (*octs_down_read_next_block_proc_t)(int conn, int stream, uint8_t* buf, int size, void* user_data);

/**
 * @brief 关闭文件函数
 * @param conn 连接
 * @param stream 流ID
 * @param user_data 用户数据指针
 */
typedef void (*octs_down_close_file_proc_t)(int conn, int stream, void* user_data);

/**
 * @brief 下载服务自定义函数
 */
typedef struct _octs_down_custom_procs_t
{
    octs_down_open_file_proc_t open_file;                       /*!< 打开文件 */
    octs_down_get_next_block_size_proc_t get_next_block_size;   /*!< 获取下一次读取大小 */
    octs_down_read_next_block_proc_t read_next_block;           /*!< 读取块 */
    octs_down_close_file_proc_t close_file;                     /*!< 关闭文件 */
} octs_down_custom_procs_t;

// 控制命令服务模块------------------------------------------------------------------------------------------
/**
 * @brief 用户通知类型
 */
typedef enum _oct_cmd_notify_type_t
{
    OCT_CNT_MIN_VALUE = 1024,

    OCT_CNT_ALARM,          /*!< 报警事件通知 */
    OCT_CNT_WEB_DATA,       /*!< web数据 */
    OCT_CNT_REMOTE_DESKTOP, /*!< NVR远程桌面数据 */
    OCT_CNT_JSON_DATA,      /*!< JSON数据 */

    OCT_CNT_MAX_VALUE = 2048,
} oct_cmd_notify_type_t;

/**
 * @brief 控制命令请求类型
 */
typedef enum _oct_cmd_request_type_t
{
    OCT_CRT_GET_REC_FILE_LIST,  /*!< 获取录像文件列表(param2：请求参数 oct_cmd_get_rec_file_param_t 类型) */
    OCT_CRT_REMOTE_CONFIG,      /*!< 远程设置(param1：命令类型; param2：请求数据 oct_cmd_remote_config_data_t 类型) */
} oct_cmd_request_type_t;

/**
 * @brief 获取文件列表参数
 */
typedef struct _oct_cmd_get_rec_file_param_t
{
    int channel;                /*!< 视频通道ID，不做限制传-1 */
    oct_datetime_t start_time;  /*!< 开始时间，时区以设备端为准，不限制则全部置0 */
    oct_datetime_t end_time;    /*!< 结束时间，时区以设备端为准，不限制则全部置0 */
} oct_cmd_get_rec_file_param_t;

/**
 * @brief 录像文件信息
 */
typedef struct _oct_cmd_rec_file_info_t
{
    const char* name;           /*!< 录像名 */
    const char* path;           /*!< 带路径的录像文件名 */
    uint64_t file_size;         /*!< 文件大小 */
    oct_datetime_t start_time;  /*!< 录像开始时间，时区以设备端为准 */
    int duration;               /*!< 毫秒单位录像持续时长 */
    int rec_type;               /*!< 录像类型(定时录像、报警录像等等，由外部定义) */
} oct_cmd_rec_file_info_t;

/**
 * @brief 获取文件列表响应结果
 */
typedef struct _oct_cmd_get_rec_file_result_t
{
    int rec_file_num;                           /*!< 录像文件数 */
    oct_cmd_rec_file_info_t* rec_file_infos;    /*!< 录像文件信息 */
} oct_cmd_get_rec_file_result_t;

/**
 * @brief 远程设置数据
 */
typedef struct _oct_cmd_remote_config_data_t
{
    uint8_t* data;              /*!< 数据指针 */
    int len;                    /*!< 数据长度 */
} oct_cmd_remote_config_data_t;


/**
 * @brief 用户通知处理函数
 * @param conn 连接
 * @param command 用户通知类型
 * @param data 通知数据
 * @param len 通知数据长度
 */
typedef void (*oct_cmd_notify_proc_t)(int conn, oct_cmd_notify_type_t command, const uint8_t* data, int len);

/**
 * @brief 发送完成处理回调
 * @param conn 连接
 * @param stream 流ID
 * @param service 服务ID
 */
typedef void (*oct_send_complete_proc_t)(int conn, int stream, int service);

/**
 * @brief 控制命令请求处理函数
 * @param conn 连接
 * @param type 请求类型
 * @param req_id 请求ID
 * @param param1 参数1
 * @param param2 参数2
 * @return 成功返回0，失败返回错误码
 */
typedef int (*octs_cmd_request_proc_t)(int conn, oct_cmd_request_type_t type, int req_id, int param1, void* param2);

// 设备搜索服务模块------------------------------------------------------------------------------------------
/**
 * @brief 设备搜索事件类型
 */
typedef enum _oct_search_event_type_t
{
    OCT_SET_FIND_DEV,   /*!< 找到设备 */
    OCT_SET_TIMEOUT,    /*!< 搜索超时结束 */
} oct_search_event_type_t;

/**
 * @brief 设备信息
 */
typedef struct _oct_search_dev_info_t
{
    octc_dev_info_t dev_info;   /*!< 设备信息 */
    const char* ip;             /*!< IP地址 */
    int trans_port;             /*!< 传输服务端口 */
} oct_search_dev_info_t;

/**
 * @brief 设备搜索事件回调
 * @param type 事件类型
 * @param dev 设备信息
 */
typedef void (*octc_search_device_event_proc_t)(oct_search_event_type_t type, oct_search_dev_info_t* dev);

// 对讲服务模块----------------------------------------------------------------------------------------------
/**
 * @brief 客户端对讲事件类型
 */
typedef enum _octc_chat_event_type_t
{
    OCTC_CET_OPEN,      /*!< 开启对讲(ec为0表示开启成功) */
    OCTC_CET_CLOSE,     /*!< 停止对讲 */
} octc_chat_event_type_t;

/**
 * @brief 设备端对讲事件类型
 */
typedef enum _octs_chat_event_type_t
{
    OCTS_CET_START,      /*!< 开启对讲 */
    OCTS_CET_STOP,       /*!< 停止对讲 */
    OCTS_CET_DATA,       /*!< 对讲数据(仅EDK端) */
} octs_chat_event_type_t;

/**
 * @brief 对讲参数
 */
typedef struct _oct_chat_param_t
{
    oct_stream_audio_codec_t audio_codec;   /*!< 音频编码 */
    int audio_channels;                     /*!< 音频通道数 */
    int audio_sample_rate;                  /*!< 音频采样率 */
    int audio_sample_bits;                  /*!< 音频采样位数 */
} oct_chat_param_t;

/**
 * @brief 对讲事件处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param data 对讲数据
 * @param len 对讲数据长度
 * @param timestamp 时间搓
 * @param param type为OCT_CET_START时返回对讲参数
 * @return 成功返回0，失败返回错误码
 */
typedef int (*octs_chat_event_proc_t)(int conn, int stream, octs_chat_event_type_t type, 
    const uint8_t* data, int len, int64_t timestamp, oct_chat_param_t* param);

/**
 * @brief 对讲事件处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param type 事件类型
 * @param ec 错误代码
 * @param param type为OCT_CET_START时返回对讲参数
 */
typedef void (*octc_chat_event_proc_t)(int conn, int stream, octc_chat_event_type_t type, 
    int ec, const oct_chat_param_t* param);

/**
 * @brief 对讲数据处理函数
 * @param conn 连接
 * @param stream 流ID
 * @param data 对讲数据,为NULL时对讲结束
 * @param len 对讲数据长度
 * @param timestamp 时间搓
 */
typedef void (*octc_chat_data_proc_t)(int conn, int stream, 
    const uint8_t* data, int len, int64_t timestamp);
