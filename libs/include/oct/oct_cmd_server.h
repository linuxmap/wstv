/**
 * @file oct_cmd_server.h
 * @brief 控制命令服务
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
 * @brief 初始化控制命令服务模块
 * @param request_proc 请求处理函数
 * @param notify_proc 用户通知回调
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_cmd_init_module(
    octs_cmd_request_proc_t request_proc,
    oct_cmd_notify_proc_t notify_proc);

/**
 * @brief 释放控制命令服务模块
 */
OCT_C_API void OCT_C_CALLTYPE octs_cmd_release_module();

/**
 * @brief 发送用户通知
 * @param conn 连接
 * @param command 通知命令类型
 * @param data 通知数据
 * @param len 通知数据长度
 * @return 成功0，失败返回负数
 */
OCT_C_API int OCT_C_CALLTYPE octs_cmd_send_notify(int conn, 
    oct_cmd_notify_type_t command, const uint8_t* data, int len);

/**
 * @brief 发送获取录像列表响应
 * @param conn 连接
 * @param req_id 请求ID
 * @param result 响应数据
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_cmd_send_get_rec_file_list_response(int conn, int req_id, const oct_cmd_get_rec_file_result_t* result);

/**
 * @brief 发送远程设置响应
 * @param conn 连接
 * @param req_id 请求ID
 * @param result 响应数据
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_cmd_send_remote_config_response(int conn, int req_id, const oct_cmd_remote_config_data_t* result);

#ifdef __cplusplus
}
#endif
