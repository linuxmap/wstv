/*
 * sp_image.c
 *
 *  Created on: 2013-11-20
 *      Author: LK
 */

#include "sp_image.h"
#include "msensor.h"
#include <SYSFuncs.h>

#define INVALID(x,min,max) (((x < min) || (x > max)))

/**
 * @brief 获取画面调节参数
 *
 * @param image 画面参数输出
 * @return 0成功 -1失败
 */
int sp_image_get_param(SPImage_t *image)
{
	msensor_attr_t sensor;

	msensor_getparam(&sensor);

	image->contrast = sensor.contrast;
	image->saturation = sensor.saturation;
	image->brightness = sensor.brightness;
	image->sharpen = sensor.sharpness;
	image->exposureMax = sensor.ae.exposureMax;
	image->exposureMin = sensor.ae.exposureMin;
	image->daynightMode = sensor.daynightMode;
	image->dayStart.hour = sensor.dayStart.hour;
	image->dayStart.minute = sensor.dayStart.minute;
	image->dayEnd.hour = sensor.dayEnd.hour;
	image->dayEnd.minute = sensor.dayEnd.minute;
	switch (sensor.sence)
	{
	case SENCE_INDOOR:
		image->scene = SP_SCENE_INDOOR;
		break; //室内
	case SENCE_OUTDOOR:
		image->scene = SP_SCENE_OUTDOOR;
		break; //室外
	case SENCE_DEFAULT:
		image->scene = SP_SCENE_DEFAULT;
		break; //默认
	case SENCE_MODE1:
		image->scene = SP_SCENE_SOFT;
		break; //柔和
	default:
		image->scene = SP_SCENE_DEFAULT;
		break;
	}

	image->bEnableWDynamic = sensor.bEnableWdr;
	image->bAutoLowFrameEn= sensor.AutoLowFrameEn;

	if ((sensor.effect_flag >> EFFECT_AWB) & 0x01)
		image->bEnableAWB = TRUE;
	else
		image->bEnableAWB = FALSE;
	if ((sensor.effect_flag >> EFFECT_MIRROR) & 0x01)
		image->bEnableMI = TRUE;
	else
		image->bEnableMI = FALSE;
	if ((sensor.effect_flag >> EFFECT_TURN) & 0x01)
		image->bEnableST = TRUE;
	else
		image->bEnableST = FALSE;
	if ((sensor.effect_flag >> EFFECT_NOCOLOR) & 0x01)
		image->bEnableNoC = TRUE;
	else
		image->bEnableNoC = FALSE;
	if ((sensor.effect_flag >> EFFECT_LOW_FRAME) & 0x01)
		image->bNightOptimization = TRUE;
	else
		image->bNightOptimization = FALSE;

	return 0;
}

/**
 * @brief 设置画面调节参数
 *
 * @param image 画面参数
 * @return 0成功 -1失败
 */
int sp_image_set_param(SPImage_t *image)
{
	msensor_attr_t sensor;

	if (INVALID(image->brightness, 0.0, 255.0)
			|| INVALID(image->contrast, 0.0, 255.0)
			|| INVALID(image->saturation, 0.0, 255.0)
			|| INVALID(image->sharpen, 0.0, 255.0)
			)
	{
		printf("ERROR: sp_image_set_param bad param\n");
		printf("image set param: %d, %d, %d ,%d\n", image->brightness, image->contrast, image->saturation, image->sharpen);
		return -1;
	}

	msensor_getparam(&sensor);
	sensor.contrast = image->contrast;
	sensor.saturation = image->saturation;
	sensor.brightness = image->brightness;
	sensor.sharpness = image->sharpen;
	sensor.ae.exposureMax = image->exposureMax;
	sensor.ae.exposureMin = image->exposureMin;
	sensor.daynightMode = image->daynightMode;
	sensor.dayStart.hour = image->dayStart.hour;
	sensor.dayStart.minute = image->dayStart.minute;
	sensor.dayEnd.hour = image->dayEnd.hour;
	sensor.dayEnd.minute = image->dayEnd.minute;

	switch (image->scene)
	{
	case SP_SCENE_INDOOR:
		sensor.sence = SENCE_INDOOR;
		break; //室内
	case SP_SCENE_OUTDOOR:
		sensor.sence = SENCE_OUTDOOR;
		break; //室外
	case SP_SCENE_DEFAULT:
		sensor.sence = SENCE_DEFAULT;
		break; //默认
	case SP_SCENE_SOFT:
		sensor.sence = SENCE_MODE1;
		break; //柔和
	default:
		sensor.sence = SENCE_MAX;
		break;
	}

	//sensor.bSupportWdr = TRUE;
	sensor.bEnableWdr = image->bEnableWDynamic;
	sensor.AutoLowFrameEn= image->bAutoLowFrameEn;
	sensor.effect_flag = 0;

	if (image->bEnableAWB)
		sensor.effect_flag |= (1 << EFFECT_AWB);
	if (image->bEnableMI)
		sensor.effect_flag |= (1 << EFFECT_MIRROR);
	if (image->bEnableST)
		sensor.effect_flag |= (1 << EFFECT_TURN);
	if (image->bEnableNoC)
		sensor.effect_flag |= (1 << EFFECT_NOCOLOUR);
	if (image->bNightOptimization)
		sensor.effect_flag |= (1 << EFFECT_LOW_FRAME);

	msensor_setparam(&sensor);
	msensor_flush(0);
	WriteConfigInfo();
	return 0;
}
