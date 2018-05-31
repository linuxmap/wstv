/*
 * sp_firmup.h
 *
 *  Created on: 2015-10-10
 *      Author: Qin Lichao
 */

#ifndef SP_FIRMUP_H_
#define SP_FIRMUP_H_
#include "sp_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief 获取当前设备是否需要升级
 *
 *@version 最新版本号
 *
 *@return 0:不需要升级，1: 需要升级，< 0 获取失败
 */
int sp_firmup_update_check(char *version);

/**
 *@brief 开始升级
 *
 *@method 升级方式: http, ftp, usb
 *
 *@url ftp方式升级需要升级文件路径
 *
 *@return 0: 升级成功，-1: 升级失败
 */
int sp_firmup_update(const char *method, const char *url);

/**
 *@brief 获取当前升级状态
 *
 *@phase 当前升级阶段: download, erase, write
 *
 *@progress 进度百分比: 0-100
 *
 *@return 0: 获取成功，< 0 获取失败
 */
int sp_firmup_update_get_progress(char *phase, int *progress);

#ifdef __cplusplus
}
#endif


#endif /* SP_FIRMUP_H_ */
