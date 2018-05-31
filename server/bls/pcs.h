/****************************************************************************
 *
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 *
 ***************************************************************************/

/**
 * @file: pcs.h
 * @author: yaoxinhong@baidu.com
 * @date: 2014/10/10 13:39:16
 * @version: 1.0 
 * @description: PCS API
 **/

#ifndef PCS_H_INCLUDE
#define PCS_H_INCLUDE

#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <iconv.h>
#include <stdint.h>

#define LOG_DEBUG   16
#define LOG_WARNING 8
#define LOG_ERROR   4

#ifdef __cpluscplus
extern "c" {
#endif

	/**
	 * 打开日志
	 *
	 * @param[in] log_path: 表示日志路径，如果为NULL的话，会默认使用stderr
	 * @param[in] log_level: 日志级别，参考LOG_DEBUG等宏定义；如果设置log_level为LOG_ERROR，仅会打印error日志
	 * @return 返回打开日志的结果
	 * - 0 表示成功
	 * - 1 表示失败
	 */
    int open_log(const char* log_path, int log_level);

	/**
	 * 关闭日志
	 *
	 * @return 返回关闭日志的结果
	 * - 0 表示成功
	 * - 1 表示失败
	 */
    int close_log();

	/**
	 * 从云摄像头平台获取直播服务器地址列表；当设备要开始上传音视频数据时，使用该接口获取上行的服务器地址信息
	 *
	 * @param[in] deviceid: 设备ID
	 * @param[in] streamid: 设备从百度云摄像头平台获取的流ID
	 * @param[out] result: 平台请求返回的结果，json格式的字符串，
	               形如{"count":2,"server_list":["qd.cam.baidu.com:1935","hz.cam.baidu.com:1935"],"request_id":1985858699}
	 * @param[in|out] ulen: in表示result的有效空间大小；out表示result的实际使用空间大小
	 * @return 返回获取直播服务器地址的结果信息
	 * - 0 表示成功
	 * - 1 表示失败
	 */
    int locate_upload(const char* deviceid, const char* streamid, char* result, unsigned long* ulen);

	/**
	 * 从云摄像头平台获取新的access_token信息；当access_token过期时，可以用该接口对access_token进行更新
	 *
	 * @param[in] deviceid: 设备ID
	 * @param[in] access_token: 设备配置过程，从百度云平台获取的access_token
	 * @param[out] result: 平台请求返回的结果，json格式的字符串，
	               形如{"access_token":"21.AFFSFSDSADS.2592000.1413810342.XXXXXX-YYYYYY","request_id":3788721243}
	 * @param[in|out] ulen: in表示result的有效空间大小；out表示result的实际使用空间大小
	 * @return 返回获取access_token的结果信息
	 * - 0 表示成功
	 * - 1 表示失败
	 */
    int get_user_token(const char* deviceid, const char* access_token, char* result, unsigned long* ulen);

	/**
	 * 从云摄像头平台获取连接百度直播服务器时需要的临时身份信息
	 *
	 * @param[in] deviceid: 设备ID
	 * @param[in] access_token: 设备配置过程，从百度云平台获取的access_token
	 * @param[out] result: 平台请求返回的结果，json格式的字符串，
	               形如{"conn_token":"eecfe65b33a48bc9643949f178fec504","request_id":1067337865}
	 * @param[in|out] ulen: in表示result的有效空间大小；out表示result的实际使用空间大小
	 * @return 返回获取临时身份的结果信息
	 * - 0 表示成功
	 * - 1 表示失败
	 */	
	int get_conn_token(const char* deviceid, const char* access_token, char* result, unsigned long* ulen);

#ifdef __cplusplus
}
#endif




#endif
