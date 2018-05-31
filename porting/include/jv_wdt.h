/*
 *
 * Copyright 2007-2008 VN, Inc.  All rights reserved.
 *
 */

#ifndef __WDT_H__
#define __WDT_H__
#include "jv_common.h"

typedef S32 HWDT;
/**
 *@brief 打开看门狗
 *@note 驱动必须先安装
 *@return 成功，返回打开的设备句柄；失败返回 -1
 *
 */
HWDT jv_open_wdt();

/**
 *@brief 关闭看门狗
 *@param S32 iDog 已打开的设备句柄
 */
void jv_close_wdt(HWDT iDog);

/**
 *@brief 喂狗
 *@param S32 iDog 已打开的设备句柄
 */
void jv_feed_wdt(HWDT iDog);

#endif /* __WDT_H__ */

