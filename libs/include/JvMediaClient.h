/**
 * @file JvMediaClient.h
 * @brief 中维流媒体客户端
 * @author 程行通
 * @copyright Copyright 2015 by Jovision
 */

#pragma once

#ifdef BUILD_JMC_DYNAMIC
#  ifdef LINUX
#    define JMC_API __attribute__((visibility ("default")))
#  else
#    define JMC_API __declspec(dllexport)
#  endif
#else
#  define JMC_API
#endif

typedef void* JMC_HANDLE;

#define JMC_LOGLEVEL_ALL					0
#define JMC_LOGLEVEL_DEBUG					1
#define JMC_LOGLEVEL_INFO					2
#define JMC_LOGLEVEL_WARN					3
#define JMC_LOGLEVEL_ERROR					4
#define JMC_LOGLEVEL_NONE					5

#define JMC_EVENTTYPE_CONNECTED				0x01 //连接已建立
#define JMC_EVENTTYPE_CONNECTFAILED			0x02 //连接失败
#define JMC_EVENTTYPE_DISCONNECTED			0x03 //连接已断开
#define JMC_EVENTTYPE_INSKEYFRAME			0x11 //插入关键帧(发布模式，此时进行插入关键帧等操作)
#define JMC_EVENTTYPE_STOPSTREAM			0x12 //停止流(发布模式，无播放器播放码流时收到该信号)
#define JMC_EVENTTYPE_STARTSTREAM			0x13 //开启流(发布模式，码流停止状态下新播放连接接入时收到该信号)

#define JMC_DATATYPE_METADATA				0x00
#define JMC_DATATYPE_H264_I					0x01
#define JMC_DATATYPE_H264_BP				0x02
#define JMC_DATATYPE_ALAW					0x11
#define JMC_DATATYPE_ULAW					0x12

#define JMC_DATATYPE_CUSTOM                 0x20 //自定义数据，不适合发送大数据量数据
                                                 //dts、pts参数会被忽略,仅支持中维扩展协议的服务程序和客户端支持
                                                 //支持正向(发不端到播放端)和反向发送(播放端到发布端),反向发送效率较低,应控制频率


typedef struct _JMC_Metadata_t
{
	int nVideoWidth;			//视频宽度
	int nVideoHeight;			//视频高度
	int nVideoFrameRateNum;		//视频帧率分子
	int nVideoFrameRateDen;		//视频帧率分母
	int nAudioDataType;			//音频数据类型(JMC_DATATYPE_*)
	int nAudioSampleRate;		//音频采样率
	int nAudioSampleBits;		//音频采样位数
	int nAudioChannels;			//音频声道数
	const char* szPublisherInfo;//发布端相关信息(不使用时应设置为NULL)
	const char* szPublisherVer;	//发布端版本信息(只在回调时有效,发送Metadata时此字段应设置为NULL)
	int nDuration;				//毫秒持续时间(一般传0或录像文件时长)
}
JMC_Metadata_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief 事件回调
 * @param hHandle 连接句柄
 * @param pUserData 用户数据(JMC_Connect时传入)
 * @param nEvent 事件类型(JMC_EVENTTYPE_*)
 * @param pParam 事件参数,数据视事件类型而定
 */
typedef void (*FunJvRtmpEventCallback_t)(JMC_HANDLE hHandle, void* pUserData, int nEvent, const char* pParam);

/*
 * @brief 发送数据帧(发布模式有效)
 * @param hHandle 连接句柄
 * @param pUserData 用户数据(JMC_Connect时传入)
 * @param nType 数据类型(JMC_DATATYPE_*)
 * @param pData 帧数据区
 * @param nSize 帧数据长度
 * @param nPts 预览时间磋
 * @param nDts 解码时间磋(不考虑B帧情况下等于nPts即可)
 */
typedef void (*FunJvRtmpDataCallback_t)(JMC_HANDLE hHandle, void* pUserData, int nType, void* pData, int nSize, int nPts, int nDts);

/*
 * @brief 获取版本字符串
 */
JMC_API const char* JMC_GetVersion();

/*
 * @brief 开启日志
 * @param nLevel 日志等级(默认等级JMC_LOGLEVEL_NONE)
 * @param szPath 日志文件目录
 * @param nMaxSize 日志所占最大空间,<=0时不做限制,若使用限制，该置建议设置为128*1024以上
 */
JMC_API void JMC_EnableLog(int nLevel, const char* szPath, int nMaxSize);

/*
 * @brief 注册回调函数
 * @param funEventCallback 事件回调
 * @param funDataCallback 数据回调
 */
JMC_API void JMC_RegisterCallback(FunJvRtmpEventCallback_t funEventCallback, FunJvRtmpDataCallback_t funDataCallback);

/*
 * @brief 建立连接
 * @param szUrl RTMP URL
 * @param bPublic 发布流(TRUE)或播放流(FALSE)
 * @param nBufSize 发送缓存最大大小(发布模式下有效，建议值 平均码率Kbps * 关键帧周期s * (1.5~2.0) * 1024 / 8)
 * @param pUserData 用户数据，会在回调函数返回
 * @return 成功返回连接句柄，失败返回NULL
 */
JMC_API JMC_HANDLE JMC_Connect(const char* szUrl, int bPublic, int nBufSize, void* pUserData);

/*
 * @brief 发送数据帧(发布模式有效)
 * @param hHandle 连接句柄
 * @param nType 数据类型(JVRTMP_DATATYPE_*)
 * @param pData 帧数据区
 * @param nSize 帧数据长度
 * @param nPts 预览时间磋
 * @param nDts 解码时间磋(不考虑B帧情况下等于nPts即可)
 * @return 成功返回正数，发送缓冲已满返回0，连接断开返回-1
 */
JMC_API int JMC_SendFrame(JMC_HANDLE hHandle, int nType, void* pData, int nSize, int nPts, int nDts);

/*
 * @brief 关闭连接
 * @param hHandle 连接句柄
 */
JMC_API void JMC_Close(JMC_HANDLE hHandle);

#ifdef __cplusplus
}
#endif
