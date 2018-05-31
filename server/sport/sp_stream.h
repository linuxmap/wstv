/*
 * sp_stream.h
 *
 *  Created on: 2013-11-20
 *      Author: LK
 */

#ifndef SP_STREAM_H_
#define SP_STREAM_H_
#include "sp_define.h"
typedef enum{
	ENCODE_H264_LEVEL_BASE,
	ENCODE_H264_LEVEL_MAIN,
	ENCODE_H264_LEVEL_HIGH,
}EncodeH264Level_e;

typedef enum
{
	SP_VENC_RC_MODE_CBR,
	SP_VENC_RC_MODE_VBR,
	SP_VENC_RC_MODE_FIXQP
}sp_venc_rc_mode_e;

typedef struct
{
	BOOL		bEnable;	///< 是否使能
	BOOL		bAudioEn;	//是否带有音频数据

	unsigned int	viWidth;		///< 输入的宽度
	unsigned int	viHeight;		///< 输入的高度

	unsigned int	width;			///< resolution width
	unsigned int	height;			///< resolution height
	unsigned int	framerate;		///< framerate such as 25, 30 ...
	unsigned int	bitrate;		///< KBit per second
	unsigned int	ngop_s;			///< I帧间隔，以秒为单位
	sp_venc_rc_mode_e rcMode;
	EncodeH264Level_e encLevel;

	int quality; 		//视频质量，取值 0 ～ 100
	int minQP;
	int maxQP;
}SPStreamAttr_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int sp_stream_get_param(int channelid, SPStreamAttr_t *attr);

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int sp_stream_set_param(int channelid, SPStreamAttr_t *attr);

typedef struct{
	SPRes_t *resList;//输出分辨率的列表
	int resListCnt;//输出分辨率的可选个数
	int maxNGOP;//最大nGOP
	int minNGOP;//最小nGOP
	int maxFramerate;//最大帧率
	int minFramerate;//最小帧率
	int maxKBitrate;//最大码率，单位为KBit
	int minKBitrate;//最小码率
	SPRes_t inputRes;//输入分辨率

	int maxRoi;//感兴趣区域编码所支持的个数。0表示不支持
}SPStreamAbility_t;

int sp_stream_get_ability(int channelid,SPStreamAbility_t *ability);

/**
 *@brief 获取播放用的URI
 */
char *sp_stream_get_stream_uri(int channelid, char *uri, int maxUriLen);

/**
 *@brief 强制生成关键帧
 */
void sp_stream_request_idr(int channelid);

/**********************************************JVS IPC SDK LK140721***************************************/
/**
 *@brief frame type definiton
 */
typedef enum
{
	SP_FRAME_TYPE_I,
	SP_FRAME_TYPE_P,
	SP_FRAME_TYPE_B,
	SP_FRAME_TYPE_A,	///< 音频帧
	SP_FRAME_TYPE_MAX
}sp_frame_type_e;
/**
 *@brief SDK外部回调函数
 *       我们将会把每个帧通过这个函数传回给外部客户
 *@param channelid 帧所在的通道号：设置0即可
 *@param data 数据buffer
 *@param size 数据长度
 *@param type 帧类型
 *
 */
typedef void (*sp_stream_callback_t)(int channelid, void *data, unsigned int size, sp_frame_type_e type, unsigned long long timestamp);
/*
 * @brief 注册获取码流的回调函数
 */
int sp_stream_register_callback(sp_stream_callback_t callback);

/**********************************************JVS IPC SDK***************************************/
#ifdef __cplusplus
}
#endif

#endif /* SP_STREAM_H_ */
