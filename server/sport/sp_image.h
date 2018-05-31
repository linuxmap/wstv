/*
 * sp_image.h
 *
 *  Created on: 2013-11-20
 *      Author: Administrator
 */

#ifndef SP_IMAGE_H_
#define SP_IMAGE_H_

#include "sp_define.h"

#define EFFECT_AWB				0		//自动白平衡
#define EFFECT_MIRROR			0x01	//镜像--即左右翻转
#define EFFECT_TURN				0x02	//翻转--上下翻转
#define EFFECT_NOCOLOR			0x03	//黑白模式

typedef enum
{
	SP_SCENE_INDOOR,				//室内
	SP_SCENE_OUTDOOR,				//室外
	SP_SCENE_DEFAULT,				//默认
	SP_SCENE_SOFT					//柔和
}SPScene_e;
typedef enum{
	SP_SENSOR_DAYNIGHT_AUTO,  		//自动
	SP_SENSOR_DAYNIGHT_ALWAYS_DAY,	//一直白天
	SP_SENSOR_DAYNIGHT_ALWAYS_NIGHT,	//一直夜间
	SP_SENSOR_DAYNIGHT_TIMER,		//定时白天
}SPMSensorDaynightMode_e;	//日夜模式

typedef struct _IMAGE_ADJUST
{
	unsigned int contrast; 		//对比度 0-255
	unsigned int brightness; 	//亮度 0-255
	unsigned int saturation; 	//饱和度0-255
	unsigned int sharpen; 		//锐度0-255

	unsigned int exposureMax; //最大曝光时间。曝光时间为 ： 1/exposureMax 秒，取值 3 - 100000
	unsigned int exposureMin;
	SPScene_e scene;			//场景
    SPMSensorDaynightMode_e daynightMode;	//日夜模式
    struct{
    	char hour;
    	char minute;
    }dayStart, dayEnd;

	BOOL bEnableAWB;			//是否自动白平衡..auto white balance
	BOOL bEnableMI;				//是否画面镜像..mirror image
	BOOL bEnableST;				//是否画面翻转..screen turn
	BOOL bEnableNoC;			//是否黑白模式..no color

	BOOL bEnableWDynamic;		//是否开启宽动态..wide dynamic
	BOOL bNightOptimization;  //是否夜视优化
	BOOL bAutoLowFrameEn;  //是否夜视自动降帧
}SPImage_t;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 获取画面对比度
 *
 * @param image 画面
 * @return 0成功 -1失败
 */
int sp_image_get_param(SPImage_t *image);

/**
 * @brief 设置画面调节参数
 *
 * @param image 画面参数
 * @return 0成功 -1失败
 */
int sp_image_set_param(SPImage_t *image);

#ifdef __cplusplus
}
#endif


#endif /* SP_IMAGE_H_ */
