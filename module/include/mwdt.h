#ifndef __MWDT_H__
#define __MWDT_H__

#include "jv_common.h"
#include "jv_wdt.h"
/**
 *@brief 打开看门狗
 *@note 驱动必须先安装
 *@return 成功，返回打开的设备句柄；失败返回 -1
 *
 */
HWDT OpenWatchDog();

/**
 *@brief 关闭看门狗
 *@param S32 iDog 已打开的设备句柄
 */
void CloseWatchDog();

/**
 *@brief 喂狗
 *@param S32 iDog 已打开的设备句柄
 */
void FeedWatchDog();

#endif /* __MWDT_H__ */
