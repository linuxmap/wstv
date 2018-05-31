/*
 * sp_osd.h
 *
 *  Created on: 2013-11-20
 *      Author: LK
 */

#ifndef SP_OSD_H_
#define SP_OSD_H_

#include "sp_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	SP_CHNOSD_POS_LEFT_TOP=0,
	SP_CHNOSD_POS_LEFT_BOTTOM,
	SP_CHNOSD_POS_RIGHT_TOP,
	SP_CHNOSD_POS_RIGHT_BOTTOM,
	SP_CHNOSD_POS_HIDE,
}SPChnOsdPos_e;

/**
 *@brief channel osd status
 */
typedef struct
{
	BOOL				bShowOSD;			///< 是否在通道中显示OSD
	char				timeFormat[32];		///< 时间格式 YYYY-MM-DD hh:mm:ss 可随意组合
	SPChnOsdPos_e		position;			///< OSD的位置 0, 左上，1，左下，2，右上，3，右下
	SPChnOsdPos_e		timePos;			///< OSD的位置，时间的位置 0, 左上，1，左下，2，右上，3，右下
	char				channelName[32];	///<通道名称
	BOOL osdbInvColEn;		//是否反色lk20131218
	BOOL bLargeOSD;			//是否用超大OSD
}SPChnOsdAttr_t;

/**
 *@brief 获取OSD的参数
 *
 *@param channelid 通道号
 *@param attr 用于存储要获取的属性的指针
 *
 *@return 0 成功
 */
int sp_chnosd_get_param(int channelid, SPChnOsdAttr_t *attr);

/**
 *@brief 设置OSD的参数
 *
 *@param channelid 通道号
 *@param attr 要设置的属性
 *
 *@return 0 成功
 */
int sp_chnosd_set_param(int channelid, SPChnOsdAttr_t *attr);


#ifdef __cplusplus
}
#endif

#endif /* SP_OSD_H_ */
