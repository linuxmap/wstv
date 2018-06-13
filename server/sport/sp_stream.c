/*
 * sp_stream.c
 *
 *  Created on: 2013-11-20
 *      Author: LK
 */
#include "sp_define.h"
#include "sp_stream.h"
#include "sp_ifconfig.h"
#include "mstream.h"
#include <SYSFuncs.h>

#define MAX_STREAM_CALLBACK_CNT 10

static sp_stream_callback_t sp_stream_callback[MAX_STREAM_CALLBACK_CNT];

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int sp_stream_get_param(int channelid, SPStreamAttr_t *attr)
{
	mstream_attr_t mattr;
	mstream_get_running_param(channelid, &mattr);
	attr->bEnable = mattr.bEnable;
	attr->bAudioEn = mattr.bAudioEn;
	attr->bitrate = mattr.bitrate;
	attr->framerate = mattr.framerate;
	attr->height = mattr.height;
	if(mattr.height==1520)
		attr->height=1536;
	attr->maxQP = mattr.maxQP;
	attr->minQP = mattr.minQP;
	attr->viHeight = mattr.viHeight;
	attr->viWidth = mattr.viWidth;
	attr->width = mattr.width;
	attr->ngop_s = mattr.nGOP_S;
	attr->rcMode = mattr.rcMode;
	attr->encLevel = ENCODE_H264_LEVEL_MAIN;
	attr->quality = 40;

	return 0;
}

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int sp_stream_set_param(int channelid, SPStreamAttr_t *attr)
{
	mstream_attr_t mattr;
	int oldMainStreamW = 0;
	int oldMainStreamH = 0;
	jvstream_ability_t tmp_ab;

	jv_stream_get_ability(channelid, &tmp_ab);

	if(attr->height==1536)
		attr->height=1520;

	mstream_get_param(channelid, &mattr);

	mattr.bEnable = attr->bEnable;
	mattr.bAudioEn = attr->bAudioEn;
	mattr.bitrate = attr->bitrate;
	mattr.framerate = attr->framerate;
	mattr.height = attr->height;
	mattr.maxQP = attr->maxQP;
	mattr.minQP = attr->minQP;
	mattr.viHeight = attr->viHeight;
	mattr.viWidth = attr->viWidth;
	mattr.width = attr->width;
	mattr.nGOP_S = attr->ngop_s;
	mattr.rcMode = attr->rcMode;

#ifdef PLATFORM_hi3516D
	if(hwinfo.encryptCode == ENCRYPT_300W)
	{
		if(channelid == 1)
		{
			if(mattr.framerate > 28)
			{
				mattr.framerate = 28;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
		if(channelid == 2)
		{
			if(mattr.framerate > 15)
			{
				mattr.framerate = 15;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
	}
	if(hwinfo.sensor == SENSOR_OV4689 && strstr(hwinfo.type,"40"))	//400w的帧率
	{
		if(channelid == 0)
		{
			if(tmp_ab.maxStreamRes[0]==mattr.width*mattr.height && 
				mattr.framerate > 25)
			{
				mattr.framerate = 25;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
		else if(channelid == 1)
		{
			if(mattr.framerate > 25)
			{
				mattr.framerate = 25;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
		else
		{
			if(mattr.framerate > 25)
			{
				mattr.framerate = 25;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
	}
#endif

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100)
	if(hwinfo.encryptCode== ENCRYPT_200W)
	{
		if(channelid == 0)
		{
			if(tmp_ab.maxStreamRes[0]==mattr.width*mattr.height && 
				mattr.framerate>15)
			{
				mattr.framerate = 15;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
		else if(channelid == 1)
		{
			if(mattr.width*mattr.height > tmp_ab.maxStreamRes[1])
			{
				mattr.width = 704;
				mattr.height = 576;
			}
			if(mattr.width*mattr.height>640*480 && mattr.framerate>20)
			{
				mattr.framerate = 20;
				mattr.bitrate = __CalcBitrate(mattr.width, mattr.height, mattr.framerate, mattr.vencType);
			}
		}
	}
#endif

	int flag,tmpWidth,tmpHeight;
	if((hwinfo.sensor == SENSOR_AR0130 && mattr.height > 960))
	{
		flag =1;
		tmpWidth = mattr.width;
		tmpHeight = mattr.height;
	}

	mstream_set_param(channelid, &mattr);
	mstream_flush(channelid);
	if(channelid == 0)
	{
		if(oldMainStreamW!=mattr.width || oldMainStreamH!=mattr.height)
		{
			int i;
			for(i=1; i<HWINFO_STREAM_CNT; i++)
			{
				mstream_flush(i);
			}
		}
	}
	WriteConfigInfo();
	return 0;
}

static SPRes_t sResList[5][12];
static int sResCnt[5] = {0};

int sp_stream_get_ability(int channelid, SPStreamAbility_t *ability)
{
	jvstream_ability_t ab;
	jv_stream_get_ability(0, &ab);

//	ability->resListCnt      = ab.resListCnt    ;
	ability->maxNGOP         = ab.maxNGOP       ;
	ability->minNGOP         = ab.minNGOP       ;
	ability->maxFramerate    = ab.maxFramerate  ;
	ability->minFramerate    = ab.minFramerate  ;
	ability->maxKBitrate     = ab.maxKBitrate   ;
	ability->minKBitrate     = ab.minKBitrate   ;
	ability->maxRoi          = ab.maxRoi        ;
	if (sResCnt[channelid] == 0)
	{
		int i;
		int minw = 704;
//		int minh = 480;
		int maxw = 720;
//		int maxh = 576;
		if (channelid == 0)
		{
			for (i=0;i<ab.resListCnt;i++)
			{
				if (ab.resList[i].width >= minw)
				{
					sResList[channelid][sResCnt[channelid]].w = ab.resList[i].width;
					sResList[channelid][sResCnt[channelid]].h = ab.resList[i].height;
					//海康NVR分辨率列表只支持1536，兼容之
					sResList[channelid][sResCnt[channelid]].h = sResList[channelid][sResCnt[channelid]].h==1520?1536:sResList[channelid][sResCnt[channelid]].h;
					sResCnt[channelid]++;
				}
			}
		}
		else
		{
			for (i=0;i<ab.resListCnt;i++)
			{
				if (ab.resList[i].width <= maxw && ab.resList[i].width > 0)
				{
					sResList[channelid][sResCnt[channelid]].w = ab.resList[i].width;
					sResList[channelid][sResCnt[channelid]].h = ab.resList[i].height;
					sResCnt[channelid]++;
				}
			}
		}
	}
	ability->resList = sResList[channelid];
	ability->resListCnt      = sResCnt[channelid];

	ability->inputRes.w = ab.inputRes.width;
	ability->inputRes.h = ab.inputRes.height;

	return 0;
}

/**
 *@brief 获取播放用的URI
 */
char *sp_stream_get_stream_uri(int channelid, char *uri, int maxUriLen)
{
	SPEth_t eth;
	sp_ifconfig_eth_get(&eth);
	snprintf(uri, maxUriLen, "rtsp://%s:554/live%d.264", eth.addr, channelid);

	return uri;
}

/**
 *@brief 强制生成关键帧
 */
void sp_stream_request_idr(int channelid)
{
	mstream_request_idr(channelid);
}

static void sp_stream_get_stream(S32 nChannel, VOID *pData, U32 nSize, jv_frame_type_e nType, unsigned long long timestamp)
{
	//printf("..........get a stream:chn[%d];size[%d],type:[%d]\n",nChannel,nSize,nType);
	sp_frame_type_e sp_type;
	switch(nType)
	{
	case JV_FRAME_TYPE_I:
		sp_type = SP_FRAME_TYPE_I;
		break;
	case JV_FRAME_TYPE_P:
		sp_type = SP_FRAME_TYPE_P;
		break;
	case JV_FRAME_TYPE_B:
		sp_type = SP_FRAME_TYPE_B;
		break;
	case JV_FRAME_TYPE_A:
		sp_type = SP_FRAME_TYPE_A;
		break;
	default:
		sp_type = SP_FRAME_TYPE_I;
		break;
	}
	int i;
	for (i=0;i<MAX_STREAM_CALLBACK_CNT;i++)
	{
		if(sp_stream_callback[i])
			sp_stream_callback[i](nChannel,pData,nSize,sp_type, timestamp);
		else
			break;
	}
}
/*
 * @brief 注册获取码流的回调函数
 */
int sp_stream_register_callback(sp_stream_callback_t callback)
{
	{
		static int bInited = 0;
		if (!bInited)
		{
			bInited = 1;
			mstream_set_transmit_callback(sp_stream_get_stream);
		}
	}

	int i;
	for (i=0;i<MAX_STREAM_CALLBACK_CNT;i++)
	{
		if (!sp_stream_callback[i])
		{
			sp_stream_callback[i] = callback;
			break;
		}
		else if (sp_stream_callback[i] == callback)
			break;
	}
	return 0;
}

