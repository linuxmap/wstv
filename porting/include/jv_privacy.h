/**
 *@file jv_privacy.h file about privacy
 * define the interface of privacy.
 *@author Liu Fengxiang
 */

#ifndef _JV_PRIVACY_H_
#define _JV_PRIVACY_H_
#include "jv_common.h"

typedef struct{
	
	RECT	rect[16];
	int cnt;
	BOOL bEnable;
}jv_privacy_attr_t;

/**
 *@brief do initialize 
 *@return 0 if success.
 *
 */
int jv_privacy_init(void);

/**
 *@brief do de-initialize
 *@return 0 if success.
 *
 */
int jv_privacy_deinit(void);


/**
 *@brief set attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of privacy. 
 *@return 0 if success.
 *
 */
int jv_privacy_set_attr(int channelid, jv_privacy_attr_t *attr);

/**
 *@brief get attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of privacy. 
 *@return 0 if success.
 *
 */
int jv_privacy_get_attr(int channelid, jv_privacy_attr_t *attr);

/**
 *@brief start privacy 
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_privacy_start(int channelid);

/**
 *@brief stop privacy
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_privacy_stop(int channelid);


#endif

