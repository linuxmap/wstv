/*
 * jv_ptz.c
 *
 *  Created on: 2014年8月12日
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */

#include <jv_common.h>

#include "jv_ptz.h"


jv_ptz_func_t g_ptzfunc ;

/**
 *@brief 初始化
 */
int jv_ptz_init()
{
	memset(&g_ptzfunc, 0, sizeof(g_ptzfunc));

	return 0;
}

/**
 *@brief 结束
 */
int jv_ptz_deinit()
{
	return 0;
}


