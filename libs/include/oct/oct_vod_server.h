/**
 * @file oct_vod_server.h
 * @brief 点播服务服务端
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
 * @brief 初始化点播服务模块
 * @param event_proc 事件处理函数
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_vod_init_module(octs_vod_event_proc_t event_proc);

/**
 * @brief 释放点播服务模块
 */
OCT_C_API void OCT_C_CALLTYPE octs_vod_release_module();

/**
 * @brief 写入点播流数据
 * @param conn 连接
 * @param stream 流ID
 * @param type 帧类型
 * @param frame 数据帧指针
 * @return 成功返回0，失败返回负数
 */
OCT_C_API int OCT_C_CALLTYPE octs_vod_write(int conn, int stream,
    oct_stream_frame_type_t type, void* frame);

/**
 * @brief 关闭客户端
 * @param conn 连接ID
 * @param stream 流ID
 */
OCT_C_API void OCT_C_CALLTYPE octs_vod_close_client(int conn, int stream);

#ifdef __cplusplus
}
#endif
