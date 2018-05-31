/******************************************************************************

  Copyright (c) 2013 . All rights reserved.

  ******************************************************************************
  File Name     : BizDevLib.h
  Version       : Daomeng Han <itangwang@126.com>
  Author        : Daomeng Han <itangwang@126.com>
  Created       : 2015年05月01日
  Description   : 
  History       : 
  1.Date        : 2015年05月01日
    Author      : Daomeng Han <itangwang@126.com>
	Modification:
  2.Date        : 2016年07月05日
	Author      : Daomeng Han <itangwang@126.com>
	Modification: 增加cloudstorage是否支持的功能
  3.Date        : 2017年01月11日
	Author      : Daomeng Han <itangwang@126.com>
	Modification: 报警字段的data支持.(2.3版本)
  4.Date        : 2017年01月13日
	Author      : Daomeng Han <itangwang@126.com>
	Modification: 修改了ClearRequestPkt.(2.4版本)
******************************************************************************/


#ifndef _BIZ_DEV_LIB_H_
#define _BIZ_DEV_LIB_H_


#include <BizCpConfig.h>


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

	#define BIZ_DEV_LIB_VERSION		"3.0"

	typedef struct BizDevCb {
	   /** 
		*  @brief 重新刷新DNS列表
		*  @param data	  用户参数，这个参数是在BizDevInit调用时传入的.
		*  @return 0 ok -1 failed.
		*/
		int (*OnRefreshServer)(void *data);

	   /** 
		*  @brief 用户信息更改
		*  @param username  用户名.
		*  @param password  密码.
		*  @param data	  用户参数，这个参数是在BizDevInit调用时传入的.
		*  @return 0 ok -1 failed.
		*/
		int (*OnUpdateUserInfo)(ZINT8 *username, ZINT8 *password, void *data);

		/** 
		*  @brief 接收到推送信息,信息的内容需要应用层自己解析
		*  @param payload  接收到的推送内容.
		*  @param len	   接收到的内容的长度.
		*  @param type	   接收到的推送类型.
		*  @param data	  用户参数，这个参数是在BizDevInit调用时传入的.
		*  @return 0 ok -1 failed.
		*/
		int (*OnPush)(ZUINT8 *payload, ZUINT32 len, ZUINT32 type, void *data);

		/** 
		*  @brief IPC上线状态更改
		*  @param online  IPC上线状态，0：offline，1：online.
		*  @param data	  用户参数，这个参数是在BizDevInit调用时传入的.
		*  @return 0 ok -1 failed.
		*/
		int (*OnOnlineStatus)(ZUINT8 online, void *data);

	} BizDevCb;

   /** 
	*  @brief 初始化设备库.
	*  @param cb	   回调函数.
	*  @param cfgpath  本地路径，用于存储日志、配置文件等等.
	*  @param dnspath  本地DNS文件路径，用户读取DNS文件.
	*  @param data	   用户参数，这个参数用于在回调函数时返回.
	*  @return ZUINT 0 成功 -1 失败.
	*/
	ZUINT BizDevInit(ZINT8 *yst, BizDevCb *cb, ZINT8 *cfgpath, ZINT8 *dnspath, void *data);

   /** 
	*  @brief 断开连接.
	*  @return ZUINT 0 成功 -1 失败.
	*/
	ZUINT BizDevClose();

   /** 
	*  @brief 释放设备库.
	*  @return ZUINT 0 成功 -1 失败.
	*/
	ZUINT BizDevTerm();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _BIZ_DEV_LIB_H_ */
