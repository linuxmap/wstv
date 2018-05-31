/**
 * @file oct_chat_server.h
 * @brief 对讲服务服务端
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
 * @brief 初始化对讲服务模块
 * @param event_proc 事件处理函数
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_chat_init_module(octs_chat_event_proc_t event_proc);

/**
 * @brief 释放对讲服务模块
 */
OCT_C_API void OCT_C_CALLTYPE octs_chat_release_module();

/**
 * @brief 写入对讲数据
 * @param conn 连接ID
 * @param stream 流ID
 * @param data 对讲数据
 * @param len 对讲数据长度
 * @param timestamp 时间搓
 * @return 成功返回0，失败返回负数
 */
OCT_C_API int OCT_C_CALLTYPE octs_chat_write(int conn, int stream, 
    const uint8_t* data, int len, int64_t timestamp);

/**
 * @brief 关闭客户端
 * @param conn 连接ID
 * @param stream 流ID
 */
OCT_C_API void OCT_C_CALLTYPE octs_chat_close_client(int conn, int stream);

#ifdef __cplusplus
}
#endif
