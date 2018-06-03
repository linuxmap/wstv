#ifndef __MSTREAM_H__
#define __MSTREAM_H__
#include "jv_ai.h"
#include "jv_stream.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int ior_reverse;				//是否反选
	RECT roi[MAX_ROI_REGION];		//范围，以VIWidthxVIHeight为基准
	int roiWeight;				///< 权重。其值介于  0 ~ 255之间
}mstream_roi_t;

typedef struct
{
	BOOL		bEnable;	///< 是否使能
	BOOL		bAudioEn;	//是否带有音频数据

	unsigned int	viWidth;		///< 输入的宽度
	unsigned int	viHeight;		///< 输入的高度

	unsigned int	width;			///< resolution width
	unsigned int	height;			///< resolution height
	unsigned int	framerate;		///< framerate such as 25, 30 ...
	unsigned int	nGOP_S;			///< I帧间隔，以秒为单位
	jv_venc_rc_mode_e rcMode;
	unsigned int	bitrate;		///< KBit per second
	int minQP;
	int maxQP;

	BOOL bRectStretch; //对于16：9与4：3之间的差异，是拉伸，还是裁剪。为真时表示拉伸
	VENC_TYPE vencType;		// 视频编码协议类型
	BOOL bLDCEnable;
	int  nLDCRatio;
}mstream_attr_t;

/**
 *@brief 用于网传的回调函数
 * mstream将会把收到的每个帧通过这个函数传回
 *@param channelid 帧所在的通道号
 *@param data 数据地址
 *@param size 数据长度
 *@param type 帧类型
 *@param timestamp 时间戳
 *
 */
typedef void (*mstream_transmit_callback_t)(int channelid, void *data, unsigned int size, jv_frame_type_e type, unsigned long long timestamp);

/**
 *@brief 初始化
 *
 */
int mstream_init(void);

/**
 *@brief 结束
 *
 */
int mstream_deinit(void);

/**
 *@brief 设置网传的回调函数
 *@param callback 网传的回调函数指针
 *
 */
int mstream_set_transmit_callback(mstream_transmit_callback_t callback);

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_param(int channelid, mstream_attr_t *attr);

int mstream_stop(int channelid);
/**
 *@brief 刷新通道，使之前的设置生效
 *@param channelid 通道号
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_flush(int channelid);

/**
 *@brief 关闭通道
 *@param channelid 通道号
 *
 *@retval 0 成功
 *@retval <0 errno
 *
 */
int mstream_stop(int channelid);

/**
 *@brief 设置参数的简化写法
 *设置完参数后，执行#mstream_flush 动作
 *
 */
#define mstream_set(channelid, key, value)\
do{\
	mstream_attr_t attr;\
	mstream_get_param(channelid, &attr);\
	attr.key = value;\
	mstream_set_param(channelid, &attr);\
}while(0)


/**
 *@brief 获取参数，设置时调用
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_get_param(int channelid, mstream_attr_t *attr);

/**
 *@brief 获取运行参数，编解码相关时，调用
 *@note 有些时候，限于编码能力的限制，设置的参数与实际运行参数不符
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_get_running_param(int channelid, mstream_attr_t *attr);

/**
 *@brief 设置帧率
 *@param channelid 通道号
 *@param framerate 帧率，如30，25，20。。。
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_framerate(int channelid, unsigned int framerate);

/**
 *@brief 设置帧率
 *@param channelid 通道号
 *@param framerate 帧率，如30，25，20。。。
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_resolution(int channelid, unsigned int width, unsigned int height);

/**
 *@brief 设置关键帧间隔
 *@param channelid 通道号
 *@param gop 关键帧间隔，以秒为单位
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_gop(int channelid, unsigned int gop);

int mstream_set_bitrate(int channelid,unsigned int bitrate);

/**
 *@brief 获取支持的分辨率的列表
 *@param list 输出 分辨率列表
 *
 *@return 分辨率列表的个数
 *
 */
#define mstream_get_valid_resolution jv_stream_get_valid_resolution

/**
 *@brief 以指定 的宽度和高度，寻找最匹配的分辨率
 *
 */
void mstream_resolution_valid(int channelid, unsigned int *width, unsigned int *height);

/**
 *@brief 向解码器申请一帧关键帧
 *在录像开始或者有客户端连接时调用此函数
 *
 *@param channelid 通道号，表示要申请此通道输出关键帧
 *
 */
void mstream_request_idr(int channelid); 

/**
 *@brief 设置指定数据流的亮度
 *
 *@param channelid 通道号，目前摄像机为单sensor，传入0即可
 *@param brightness 亮度值
 *
 */
void mstream_set_brightness(int channelid, int brightness); 

/**
 *@brief 设置指定数据流的去雾强度
 *
 *@param channelid 通道号，目前摄像机为单sensor，传入0即可
 *@param antifog 去雾强度
 *
 */
void mstream_set_antifog(int channelid, int antifog); 

/**
 *@brief 设置指定数据流的饱和度
 *
 *@param channelid 通道号，目前摄像机为单sensor，传入0即可
 *@param saturation 饱和度值
 *
 */
void mstream_set_saturation(int channelid, int saturation); 

/**
 * 设置RTSP播放器的用户名和密码
 */
int mstream_rtsp_user_changed(void);

int mstream_audio_set_param(int channelid, jv_audio_attr_t *attr);
int mstream_audio_restart(int channelid,int bEn);

/**
 * 获取感兴趣区域信息
 */
int mstream_get_roi(mstream_roi_t *mroi);
/**
 * 设置感兴趣区域信息
 */
int mstream_set_roi(mstream_roi_t *mroi);

#ifdef __cplusplus
}
#endif

#endif

