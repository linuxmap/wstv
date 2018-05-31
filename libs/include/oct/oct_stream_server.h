/**
 * @file oct_stream_server.h
 * @brief 直播服务服务端
 * @author 程行通
 * @copyright Copyright (c) 2017 Jovision
 */

#pragma once

#include "oct_def.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief 流接入验证函数
 * @param conn 连接id
 * @param channel 视频通道
 * @param sub_stream 码流ID
 * @return 成功返回0，失败返回错误码
 */
typedef int (*octs_stream_verify_proc_t)(int conn, int channel, int sub_steam);


/**
 * @brief 初始化直播服务模块
 * @param enable_frame_queue 是否开启帧队列(1:开启 0：关闭)
 * @param client_limit 直播流总数限制(0不做限制)
 * @param event_proc 时间处理函数
 * @return 成功返回0，失败返回负值
 * @node 帧队列存放关键帧开始的一个帧序列，开启帧队列后，新接入流将从队列数据开始发送，
 *       否者将会从下一个新写入的关键帧开始发送。
 *       开启帧队列会造成内存占用增加、出图速度加快、初始播放延迟变大。
 *       推荐支持插入关键帧的设备不开启帧队列，保证低资源占用和低初始播放延迟。
 *       推荐不支持插入关键帧的设备开启，保证客户端出图速度。
 */
OCT_C_API int OCT_C_CALLTYPE octs_stream_init_module(int enable_frame_queue,
    int client_limit, octs_stream_event_proc_t event_proc);

/**
 * @brief 释放直播服务模块
 */
OCT_C_API void OCT_C_CALLTYPE octs_stream_release_module();

/**
 * @brief 设置自定义回调函数
 * @param verify_proc 流接入验证函数
 */
OCT_C_API void OCT_C_CALLTYPE octs_stream_set_cumtom_callback(octs_stream_verify_proc_t verify_proc);

/**
 * @brief 创建直播流
 * @param channel 视频通道
 * @param sub_stream 码流ID
 * @return 成功返回0，失败返回负数
 */
OCT_C_API int OCT_C_CALLTYPE octs_stream_create(int channel, int sub_stream);

/**
 * @brief 释放直播流
 * @param channel 视频通道
 * @param sub_stream 码流ID
 */
OCT_C_API void OCT_C_CALLTYPE octs_stream_release(int channel, int sub_stream);

/**
 * @brief 写入直播流数据
 * @param channel 视频通道
 * @param sub_stream 码流ID
 * @param type 帧类型
 * @param frame 数据帧指针
 * @return 成功返回0，失败返回负数
 */
OCT_C_API int OCT_C_CALLTYPE octs_stream_write(int channel, int sub_stream, 
    oct_stream_frame_type_t type, void* frame);

/**
 * @brief 关闭客户端
 * @param conn 连接ID
 * @param stream 流ID
 */
OCT_C_API void OCT_C_CALLTYPE octs_stream_close_client(int conn, int stream);

#ifdef __cplusplus
}
#endif
