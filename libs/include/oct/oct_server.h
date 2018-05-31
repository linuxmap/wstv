/**
 * @file oct_server.h
 * @brief Oct网络SDK服务端
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
 * @brief 获取版本字符串
 * @return 返回版本字符串
 */
OCT_C_API const char* OCT_C_CALLTYPE octs_get_version();

/**
 * @brief 初始化SDK
 * @param uoid 设备证书数据
 * @param uoid_len 输入设备证书数据区长度，返回设备证书实际长度
 * @param temp_dir 临时文件目录，需保证有读写权限
 * @param app 应用名
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_init_sdk(const uint8_t* uoid, int* uoid_len, 
    const char* temp_dir, const char* app);

/**
 * @brief 释放SDK
 */
OCT_C_API void OCT_C_CALLTYPE octs_release_sdk();

/**
 * @brief 获取设备授权信息
 * @param info 返回设备授权信息
 */
OCT_C_API void OCT_C_CALLTYPE octs_get_uoid_info(oct_uoid_info_t* info);

/**
 * @brief 设置设备信息
 * @param dev_info 设备信息
 */
OCT_C_API void OCT_C_CALLTYPE octs_set_device_info(const octs_dev_info_t* dev_info);

/**
 * @brief 注册回调函数(传NULL将使用默认处理函数)
 * @param online_event_proc 设备上线事件处理函数
 * @param verify_user_proc 用户验证回调
 * @param connnect_event_proc 连接事件回调
 */
OCT_C_API void OCT_C_CALLTYPE octs_register_callbacks(
    oct_online_event_proc_t online_event_proc,
    oct_verify_user_proc_t verify_user_proc,
    oct_connnect_event_proc_t connnect_event_proc);

/**
 * @brief 开启日志
 * @param file_out_level 文件输出日志等级(OCT_LOGLEVEL_*)
 * @param file_out_path 文件输出日志目录
 * @param std_out_level 标准输出日志等级(OCT_LOGLEVEL_*)
 */
OCT_C_API void OCT_C_CALLTYPE octs_enable_log(int file_out_level, const char* file_out_path, int std_out_level);

/**
 * @brief 断开连接
 * @param conn 连接ID
 */
OCT_C_API void OCT_C_CALLTYPE octs_close_conn(int conn);

/**
 * @brief 开启传输服务
 * @param port 服务端口
 * @param conn_limit 连接数总数（tcp+udp）限制，无限制传0
 * @param conn_limit_udp udp连接数限制，无限制传0
 * @param flags 连接选项(详见 OCT_TRANS_FLAGS_* 掩码值)
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_start_transmission_service(int port, int conn_limit, int conn_limit_udp, int flags);

/**
 * @brief 停止传输服务
 */
OCT_C_API void OCT_C_CALLTYPE octs_stop_transmission_service();

/**
 * @brief 开启上线服务
 * @param port 服务端口
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_start_cloudsee_online_service(int port);

/**
 * @brief 停止上线服务
 */
OCT_C_API void OCT_C_CALLTYPE octs_stop_cloudsee_online_service();

/**
 * @brief 开启设备搜索服务
 * @param port 服务端口
 * @param net_card 网卡名，不限定网卡传NULL
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_start_search_device_service(int port, const char* net_card);

/**
 * @brief 设备搜索服务
 */
OCT_C_API void OCT_C_CALLTYPE octs_stop_search_device_service();

/**
 * @brief 设备向服务器发送告警事件
 * @param type 设备告警事件类型
 * @param data_len 设备告警事件数据长度
 * @param data 设备告警事件数据
 * @return 成功返回0，失败返回错误码
 */
OCT_C_API int OCT_C_CALLTYPE octs_push_alarm_to_server(int type,int data_len,const uint8_t* data);

#ifdef __cplusplus
}
#endif
