/*
 * jhlsupload.h
 *
 *  Created on: 2015Äê6ÔÂ12ÈÕ
 *      Author: LiuFengxiang
 *		 Email: lfx@jovision.com
 */

#ifndef JHLSUPLOAD_H_
#define JHLSUPLOAD_H_

#include "jhlstype.h"

#ifdef __cplusplus
extern "C"{
#endif /* End of #ifdef __cplusplus */

#if USE_CUSTOM_CLOUD_API
#include "JVCS_CloudStore.h"
#endif

#if USE_CUSTOM_CLOUD_API
/**
 *@brief open an jhls to upload data
 *
 *@param yun_yst
 *@param yun_id 
 *@param yun_key 
 *@param yun_host
 *@param yun_bkt 
 *@param yun_type 
 */
int jhlsup_init(const char *yun_yst, const char *yun_id, const char *yun_key, 
				const char *yun_host, const char *yun_bkt, const JOV_YUN_TYPE_EN yun_type);

void jhlsup_deinit();
#endif

#if OBSS_CLOUDSTORAGE

int jhlsup_init(const char* obssHost, 
	const int httpPort, 
	const char* accessId, 
	const char* accessKey, 
	const char* securityToken,
	const char* userAgent,
	const char* bucketname,
	int timeout);

void jhlsup_deinit();

int jhlsup_reset(const char* accessId, 
	const char* accessKey, 
	const char* securityToken);

int jhlsup_loadFile(const char* remoteFileName,const char* localFileName,size_t* psize);

#endif

/**
 *@brief open an jhls to upload data
 *
 *@param baseName just like this: [baseName].m3u8, [baseName]-1.ts, [baseName]-2.ts...
 *@param targetFrameCnt length with frame cnt, for this hls.
 *@param tsFileSize size of one ts file. it will auto adpt to multiple of 188
 *@param tsDuration target len of one ts file, unit with second
 */
JHLSHandle_t jhlsup_open(int year, int month, int day, const char *baseName, int targetFrameCnt, int tsfileSize, int tsDuration);

/**
 *@brief set param
 *@param handle handle return from #jhlsup_open
 *@param vtype type of video
 *@param atype type of audio
 *
 *@return 0 if success
 */
int jhlsup_set_param(JHLSHandle_t handle, JHLSVideoStreamType_e vtype, JHLSAudioStreamType_e atype);

/**
 *@brief input video and audio data
 *
 *@param handle handle return from #jhlsup_open
 *@param type frame type
 *@param timeStamp time stamp of current frame, unit: us
 *@param frame data
 *@param len data len
 *
 *@return 0 if success
 */
int jhlsup_inputData(JHLSHandle_t handle, JHLSFrameType_e type, unsigned long long timeStamp, const unsigned char* frame, int len, int nType);

/**
 *@brief check if finished.
 *
 *@param handle handle return from #jhlsup_open
 *
 *@return true if finished. false otherwise
 */
int jhlsup_bFinished(JHLSHandle_t handle);

/**
 *@brief finish the upload
 *
 *@param handle handle return from #jhlsup_open
 *
 *@return 0
 */
int jhlsup_close(JHLSHandle_t handle);

/**
 *@brief one ts file is finished
 *
 *@param handle handle return from #jhlsup_open
 *
 *@return 1 finished; 0 no finished
 */
int jhlsup_btsFileFinished(JHLSHandle_t handle);

/**
 *@brief set one ts file size
 *
 *@param handle handle return from #jhlsup_open
 *@param handle tsFileSize the next ts file size
 *
 *@return 0
 */
int jhlsup_setTsFileSize(JHLSHandle_t handle,int tsFileSize);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */


#endif /* JHLSUPLOAD_H_ */
