/*
 * sp_privacy.h
 *
 *  Created on: 2013-11-18
 *      Author: LK
 */

#ifndef SP_PRIVACY_H_
#define SP_PRIVACY_H_

#include "sp_define.h"

#define MAX_PYRGN_NUM		8		//遮挡区域个数privacy

#ifdef __cplusplus
extern "C" {
#endif

//视频遮挡区域
typedef struct
{
	BOOL	bEnable;
	SPRect_t	stRect[MAX_PYRGN_NUM];
}SPRegion_t;

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param region 视频遮挡区域参数
 *
 *@return 0 或者错误号
 *
 */
int sp_privacy_get_param(int channelid, SPRegion_t *region);

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param region 视频遮挡区域参数
 *
 *@return 0 或者错误号
 *
 */
int sp_privacy_set_param(int channelid, SPRegion_t *region);

#ifdef __cplusplus
}
#endif

#endif /* SP_PRIVACY_H_ */
