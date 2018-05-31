/*
 * sp_mdetect.h
 *
 *  Created on: 2013-11-15
 *      Author: lfx
 */

#ifndef SP_MDETECT_H_
#define SP_MDETECT_H_

#include "sp_define.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
	int		bEnable;			//是否开启移动检测
	int		bEnableRecord;			//是否开启移动检测录像
	int		nSensitivity;		//灵敏度, 0 ~ 255之间
	int		nRectNum;			//移动检测区域个数，最大为4，0表示全画面检测
	SPRect_t stRect[4];

	int		nDelay;
	int		bOutClient;
	int		bOutEMail;
}SPMdetect_t;

typedef void (*sp_mdetect_callback_t)(int channelid, void *param);
/**
 *@brief 设置参数
 *@param channelid 频道号
 *@param md 要设置的属性结构体
 *@note 如果不能确定所有属性的值，请先#mdetect_get_param获取原本的值
 *@return 0 成功，-1 失败
 *
 */
int sp_mdetect_set_param(int channelid, SPMdetect_t *md);

/**
 *@brief 获取参数
 *@param channelid 频道号
 *@param md 要设置的属性结构体
 *@return 0 成功，-1 失败
 *
 */
int sp_mdetect_get_param(int channelid, SPMdetect_t *md);

/**
 *@brief 检查是否正在移动检测报警
 *
 *@return 为真，则表示有移动检测报警，反之则没有
 */
BOOL sp_mdetect_balarming(int channelid);

void sp_StopMotionDetect();


#ifdef __cplusplus
}
#endif


#endif /* SP_MDETECT_H_ */
