/**
 * @file oct_down_server.h
 * @brief 下载服务服务端
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
 * @brief 初始化下载服务模块(警告：认真处理下载授权事件文件路径，防止出现安全漏洞)
 * @param event_proc 下载事件处理函数
 * @param custom_procs 自定义文件处理函数，不使用传NULL
 * @return 成功返回0，失败返回负值
 */
OCT_C_API int OCT_C_CALLTYPE octs_down_init_module(octs_down_event_proc_t event_proc, 
    const octs_down_custom_procs_t* custom_procs);

/**
 * @brief 释放下载服务模块
 */
OCT_C_API void OCT_C_CALLTYPE octs_down_release_module();

#ifdef __cplusplus
}
#endif
