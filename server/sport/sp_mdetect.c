/*
 * sp_mdetect.c
 *
 *  Created on: 2013-11-15
 *      Author: lfx
 */

#include <jv_common.h>
#include "sp_define.h"
#include "sp_mdetect.h"
#include "mdetect.h"
#include "SYSFuncs.h"
#include "jv_mdetect.h"


/**
 *@brief 设置参数
 *@param channelid 频道号
 *@param md 要设置的属性结构体
 *@note 如果不能确定所有属性的值，请先#mdetect_get_param获取原本的值
 *@return 0 成功，-1 失败
 *
 */
int sp_mdetect_set_param(int channelid, SPMdetect_t *md)
{
	__FUNC_DBG__();
	MD tmd;
	mdetect_get_param(channelid, &tmd);
	tmd.bEnable = md->bEnable;
	tmd.bEnableRecord= md->bEnableRecord;
	tmd.nSensitivity = md->nSensitivity;
	tmd.nRectNum = md->nRectNum;
	int i;
	memset(tmd.stRect, 0, sizeof(tmd.stRect));
	for (i=0;i<tmd.nRectNum;i++)
	{
		tmd.stRect[i].x = md->stRect[i].x;
		tmd.stRect[i].y = md->stRect[i].y;
		tmd.stRect[i].w = md->stRect[i].w;
		tmd.stRect[i].h = md->stRect[i].h;
	}
	tmd.nDelay = md->nDelay;
	tmd.bOutClient = md->bOutClient;
	tmd.bOutEMail = md->bOutEMail;
	mdetect_set_param(channelid, &tmd);
	mdetect_flush(channelid);
//	printf("mdetect type: %d, channelid: %d\n", tmd.bEnable, channelid);
	WriteConfigInfo();
	return 0;
}

/**
 *@brief 获取参数
 *@param channelid 频道号
 *@param md 要设置的属性结构体
 *@return 0 成功，-1 失败
 *
 */
int sp_mdetect_get_param(int channelid, SPMdetect_t *md)
{
	__FUNC_DBG__();
	MD tmd;
	mdetect_get_param(channelid, &tmd);
	md->bEnable = tmd.bEnable;
	md->bEnableRecord= tmd.bEnableRecord;
	md->nSensitivity = tmd.nSensitivity;
	md->nRectNum = tmd.nRectNum;
	int i;
	for (i=0;i<md->nRectNum;i++)
	{
		md->stRect[i].x = tmd.stRect[i].x;
		md->stRect[i].y = tmd.stRect[i].y;
		md->stRect[i].w = tmd.stRect[i].w;
		md->stRect[i].h = tmd.stRect[i].h;
	}
	md->nDelay = tmd.nDelay;
	md->bOutClient = tmd.bOutClient;
	md->bOutEMail = tmd.bOutEMail;
	return 0;
}

/**
 *@brief 检查是否正在移动检测报警
 *
 *@return 为真，则表示有移动检测报警，反之则没有
 */
BOOL sp_mdetect_balarming(int channelid)
{
//	__FUNC_DBG__();
	return mdetect_b_alarming(channelid);
}

void sp_StopMotionDetect()
{
	jv_mdetect_stop(0);
}


