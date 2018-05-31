/*
 * sp_privacy.c
 *
 *  Created on: 2013-11-18
 *      Author: LK
 */

#include <jv_common.h>
#include "sp_privacy.h"
#include "mprivacy.h"
#include <SYSFuncs.h>

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param region 视频遮挡区域参数
 *
 *@return 0 或者错误号
 *
 */
int sp_privacy_get_param(int channelid, SPRegion_t *region)
{
	int i;
	REGION mregion;
	mprivacy_get_param(channelid, &mregion);

	for (i=0;i<MAX_PYRGN_NUM;i++)
	{
		region->bEnable = mregion.bEnable;
		region->stRect[i].x = mregion.stRect[i].x;
		region->stRect[i].y = mregion.stRect[i].y;
		region->stRect[i].w = mregion.stRect[i].w;
		region->stRect[i].h = mregion.stRect[i].h;
	}

	return 0;
}

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param region 视频遮挡区域参数
 *
 *@return 0 或者错误号
 *
 */
int sp_privacy_set_param(int channelid, SPRegion_t *region)
{
	int i;
	REGION mregion;
	mregion.bEnable = region->bEnable ;
	for (i=0;i<MAX_PYRGN_NUM;i++)
	{
		mregion.stRect[i].x = region->stRect[i].x;
		mregion.stRect[i].y = region->stRect[i].y;
		mregion.stRect[i].w = region->stRect[i].w;
		mregion.stRect[i].h = region->stRect[i].h;
	}

	mprivacy_set_param(channelid, &mregion);
	mprivacy_flush(channelid);
	WriteConfigInfo();
	return 0;
}
