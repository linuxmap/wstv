#ifndef _JV_AO_H_
#define _JV_AO_H_

#include <jv_ai.h>
typedef struct {
	unsigned int					aoBufTotalNum;    /* total number of channel buffer */
	unsigned int					aoBufFreeNum;	   /* free number of channel buffer */
	unsigned int					aoBufBusyNum;	   /* busy number of channel buffer */
}jv_ao_status;
/**
 *@brief 打开音频输出
 *
 *@param channelid 音频通道号
 *@param attr 音频通道的设置参数
 *
 *@return 0 成功
 *
 */
int jv_ao_start(int channelid ,jv_audio_attr_t *attr);

/**
 *@brief 关闭音频输出
 *
 *@param channelid 音频通道号
 *
 *@return 0 成功
 *
 */
int jv_ao_stop(int channelid);

/**
 *@brief 播放一帧音频数据
 *
 *@param channelid 音频通道号
 *@param frame 音频帧
 *
 *@return 0 成功， -1 失败
 *
 */
int jv_ao_send_frame(int channelid, jv_audio_frame_t *frame);
/**
 *@brief获取当前被占用的缓存块数
 *
 *@param channelid 音频通道号
 *@param aoStatus AO缓存占用状态结构体
 *
 *@return  -1 :失败，0:成功
 *
 */

int jv_ao_get_status(int channelid, jv_ao_status *aoStatus);

void jv_ao_adec_end();

/**
 *@brief 音频输出是否静音
 *
 *@param channelid 音频通道号
 *@param mute 是否静音
 *
 *@return 0 成功， <0 失败
 *
 */
int jv_ao_mute(BOOL bMute);

int jv_ao_get();


int jv_ao_ctrl(int volCtrl);

int jv_ao_get_attr(int channelid, jv_audio_attr_t *attr);

#endif

