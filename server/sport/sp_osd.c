/*
 * sp_osd.c
 *
 *  Created on: 2013-11-20
 *      Author: LK
 */
#include <jv_common.h>
#include "sp_osd.h"
#include "mosd.h"
#include <SYSFuncs.h>

/**
 *@brief 获取OSD的参数
 *
 *@param channelid 通道号
 *@param attr 用于存储要获取的属性的指针
 *
 *@return 0 成功
 */
int sp_chnosd_get_param(int channelid, SPChnOsdAttr_t *attr)
{
	mchnosd_attr mattr;
	mchnosd_get_param(channelid, &mattr);

	attr->bShowOSD = mattr.bShowOSD;
	attr->bLargeOSD= mattr.bLargeOSD;
	attr->osdbInvColEn= mattr.osdbInvColEn;
	strncpy(attr->channelName, mattr.channelName, 32);
	strncpy(attr->timeFormat, mattr.timeFormat, 32);

	switch (mattr.position)
	{
	case MCHNOSD_POS_LEFT_TOP:
		attr->position = SP_CHNOSD_POS_LEFT_TOP;
		break;
	case MCHNOSD_POS_LEFT_BOTTOM:
		attr->position = SP_CHNOSD_POS_LEFT_BOTTOM;
		break;
	case MCHNOSD_POS_RIGHT_TOP:
		attr->position = SP_CHNOSD_POS_RIGHT_TOP;
		break;
	case MCHNOSD_POS_RIGHT_BOTTOM:
		attr->position = SP_CHNOSD_POS_RIGHT_BOTTOM;
		break;
	case MCHNOSD_POS_HIDE:
		attr->position = SP_CHNOSD_POS_HIDE;
		break;
	default:
		break;
	}
	switch (mattr.timePos)
	{
	case MCHNOSD_POS_LEFT_TOP:
		attr->timePos = SP_CHNOSD_POS_LEFT_TOP;
		break;
	case MCHNOSD_POS_LEFT_BOTTOM:
		attr->timePos = SP_CHNOSD_POS_LEFT_BOTTOM;
		break;
	case MCHNOSD_POS_RIGHT_TOP:
		attr->timePos = SP_CHNOSD_POS_RIGHT_TOP;
		break;
	case MCHNOSD_POS_RIGHT_BOTTOM:
		attr->timePos = SP_CHNOSD_POS_RIGHT_BOTTOM;
		break;
	case MCHNOSD_POS_HIDE:
		attr->timePos = SP_CHNOSD_POS_HIDE;
		break;
	default:
		break;
	}

	return 0;
}

/**
 *@brief 设置OSD的参数
 *
 *@param channelid 通道号
 *@param attr 要设置的属性
 *
 *@return 0 成功
 */
int sp_chnosd_set_param(int channelid, SPChnOsdAttr_t *attr)
{
	mchnosd_attr mattr;
	memset(&mattr, 0, sizeof(mchnosd_attr));
	mattr.bShowOSD = attr->bShowOSD;
	mattr.bLargeOSD= attr->bLargeOSD;
	mattr.osdbInvColEn= attr->osdbInvColEn;
	strncpy(mattr.channelName, attr->channelName, 32);
	strncpy(mattr.timeFormat, attr->timeFormat, 32);
	switch(attr->position)
	{
	case SP_CHNOSD_POS_LEFT_TOP:
		mattr.position = MCHNOSD_POS_LEFT_TOP;
		break;
	case SP_CHNOSD_POS_LEFT_BOTTOM:
		mattr.position = MCHNOSD_POS_LEFT_BOTTOM;
		break;
	case SP_CHNOSD_POS_RIGHT_TOP:
		mattr.position = MCHNOSD_POS_RIGHT_TOP;
		break;
	case SP_CHNOSD_POS_RIGHT_BOTTOM:
		mattr.position = MCHNOSD_POS_RIGHT_BOTTOM;
		break;
	case SP_CHNOSD_POS_HIDE:
		mattr.position = MCHNOSD_POS_HIDE;
		break;
	default:
		break;
	}
	switch(attr->timePos)
	{
	case SP_CHNOSD_POS_LEFT_TOP:
		mattr.timePos = MCHNOSD_POS_LEFT_TOP;
		break;
	case SP_CHNOSD_POS_LEFT_BOTTOM:
		mattr.timePos = MCHNOSD_POS_LEFT_BOTTOM;
		break;
	case SP_CHNOSD_POS_RIGHT_TOP:
		mattr.timePos = MCHNOSD_POS_RIGHT_TOP;
		break;
	case SP_CHNOSD_POS_RIGHT_BOTTOM:
		mattr.timePos = MCHNOSD_POS_RIGHT_BOTTOM;
		break;
	case SP_CHNOSD_POS_HIDE:
		mattr.timePos = MCHNOSD_POS_HIDE;
		break;
	default:
		break;
	}

	mchnosd_set_param(channelid, &mattr);
	mchnosd_stop(channelid);
	mchnosd_flush(channelid);
	WriteConfigInfo();
	return 0;
}
