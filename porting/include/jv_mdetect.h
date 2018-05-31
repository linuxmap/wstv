/**
 *@file jv_mdetect.h motion detection about
 * define the interface of motion detection
 *@author Liu Fengxiang
 */

#ifndef _JV_MDETECT_H_
#define _JV_MDETECT_H_

#include "jv_common.h"

typedef struct{
	U32		nSensitivity;			///< ÁéÃô¶È
	U32		nThreshold;			///< ÒÆ¶¯¼ì²âãÐÖµ
	//U32		nRatio;				///< ÒÆ¶¯¼ì²âÁéÃô¶È
	RECT	rect[16];
//	RECT	undetect_rect[16];		//ÆÁ±ÎÒÆ¶¯Õì²â
//	RECT	detect_line[16];
	int cnt;

	U32		nRegion[32];		//ÒÆ¶¯Õì²â·¶Î§¿é
	int		nRow;				//ÐÐÊý
	int 	nColumn;			//ÁÐÊý
	int		nRegionCnt;			//Ñ¡ÖÐµÄ¿éÊý
}jv_mdetect_attr_t;

typedef void (*jv_mdetect_callback_t)(int channelid, void *param);

/**
 *@brief do initialize of motion detection
 *@return 0 if success.
 *
 */
int jv_mdetect_init(void);

/**
 *@brief do de-initialize of motion detection
 *@return 0 if success.
 *
 */
int jv_mdetect_deinit(void);

/**
 *@brief set callback
 *@return 0 if success.
 *
 */
int jv_mdetect_set_callback(jv_mdetect_callback_t callback,void *param);

/**
 *@brief set attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of motion detect. 
 *@return 0 if success.
 *
 */
int jv_mdetect_set_attr(int channelid, jv_mdetect_attr_t *attr);

/**
 *@brief get attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of motion detect. 
 *@return 0 if success.
 *
 */
int jv_mdetect_get_attr(int channelid, jv_mdetect_attr_t *attr);

/**
 *@brief start motion detection 
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param callback funcptr will be called when motion detected.
 *@param param when callback occured, param is filled in the parameter
 *@return 0 if success.
 *
 */
int jv_mdetect_start(int channelid, jv_mdetect_callback_t callback, void *param);

/**
 *@brief stop motion detection 
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_mdetect_stop(int channelid);

void jv_mdetect_set_sensitivity(int channelid, int sens);

void jv_mdetect_silence_callback(BOOL flag);
#endif
 
