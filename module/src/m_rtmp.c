#include "JvCDefines.h"
#include "JVNSDKDef.h"
#include "m_rtmp.h"
#include "utl_common.h"
#include "utl_cmd.h"
#include <stdio.h>
#include "mlog.h"
#include "mplay_remote.h"
#include "mstream.h"

#include "JvMediaClient.h"

#define MAX_LIVE_NUM	((MAX_STREAM))
#define MAX_RTMP_NUM	((MAX_LIVE_NUM) + (NET_ALARM_MAX_PLAY))

#define LOG_PREFIX_LEN	40
#define DEBUGLEVEL		JMC_LOGLEVEL_ALL
#define LOGDIR			"/tmp/jmclog/"
#define LOGSIZE			128*1024

#define URL_MAX_LENTH	128

//#define IS_VALID_CH(ch)			(((ch) >= 0) && ((ch) < MAX_VSOURCE))
#define IS_VALID_STRM(st)		(((st) >= 0) && ((st) < MAX_STREAM))
#define IS_VALID_HDL(n)			(((n) >= 0) && ((n) < MAX_RTMP_NUM))
#define IS_LIVE_HDL(n)			(((n) >= 0) && ((n) < MAX_LIVE_NUM))		// 是否为直播Handle
#define RTMP_CHANNEL(n)			(IS_LIVE_HDL(n) ? ((n) / MAX_STREAM) : (RTMP_INVALID_HDL))
#define RTMP_STREAM(n)			(IS_LIVE_HDL(n) ? ((n) % MAX_STREAM) : (RTMP_INVALID_HDL))
#define RTMP_PLAYINDEX(n)		(IS_LIVE_HDL(n) ? (RTMP_INVALID_HDL) : ((n) - MAX_LIVE_NUM))


#define	pr_info(x...)			do{\
									printf("%s ", __FUNCTION__);\
									printf(x);\
								}while(0)

#if 0
#define pr_dbg(x...)			do{\
									printf("%s ", __FUNCTION__);\
									printf(x);\
								}while(0)
#else
#define pr_dbg(x...)
#endif

#define RTMP_CHECK_HDL(n)		do\
								{\
									if (!IS_VALID_HDL(n))\
									{\
										pr_info("Invalid handle: %d\n", n);\
										return -1;\
									}\
								} while(0)
		
#define RTMP_CHECK_HDL_NORET(n)	do\
								{\
									if (!IS_VALID_HDL(n))\
									{\
										pr_info("Invalid handle: %d\n", n);\
										return;\
									}\
								} while(0)

//等待flag变为1，timeout是超时时间，单位ms
#define Wait_timeout(Flag, ms)	utl_WaitTimeout(Flag, ms)
#define Common_GetMs			utl_get_ms

typedef struct
{
	JMC_HANDLE		Handle;
	RTMP_STATE_e	State;
	BOOL			bStopped;
	// BOOL			bReqIFrame;
	RtmpCallback	pCallback;
	void* 			arg;
	pthread_mutex_t	Mutex;
	S8 				URL[URL_MAX_LENTH * 2];
}RTMP_CONF;

static RTMP_CONF	s_RtmpConf[MAX_RTMP_NUM];					// RTMP配置(直播+回放)

static pthread_t	s_RtmpReconn = 0;
static BOOL			bReconThreadRunning = FALSE;


// 内部函数声明
static void		rtmp_EventCallback(JMC_HANDLE hHandle, void* pUserData, S32 nEvent, const char* pParam);
static void		rtmp_DataCallback(JMC_HANDLE hHandle, void* pUserData, S32 nType, void* pData, S32 nSize, S32 nPts, S32 nDts);
static void*	rtmp_ReconnThread(void *param);
//static S32		rtmp_CheckLog();
static S32		rtmp_GetLiveConf(U8 Channel, U8 nStream, JMC_Metadata_t* pData);
static S8*		rtmp_log_prefix(S32 nHandle);


S32 Rtmp_Init()
{
	U8 i = 0;

	for (i = 0; i < MAX_RTMP_NUM; ++i)
	{
		pthread_mutex_init(&s_RtmpConf[i].Mutex, NULL);
	}

	JMC_EnableLog(DEBUGLEVEL, LOGDIR, LOGSIZE);
	const S8 * version = JMC_GetVersion();
	JMC_RegisterCallback(rtmp_EventCallback, rtmp_DataCallback);

	if(version != NULL)
	{
		pr_info("time=%ld, jmc version:%s\n", time(NULL), version);
		mlog_write("流媒体库版本:%s", version);
	}
	pthread_create(&s_RtmpReconn, NULL, rtmp_ReconnThread, NULL);

	return 0;
}

S32 Rtmp_Deinit()
{
	U8 i = 0;

	for (i = 0; i < MAX_RTMP_NUM; ++i)
	{
		Rtmp_Stop(i);
		pthread_mutex_destroy(&s_RtmpConf[i].Mutex);
	}

	bReconThreadRunning = FALSE;

	return 0;
}

S32 Rtmp_SetURL(S32 nHandle, const S8 *pURL)
{
	S8 *pToken = strstr(pURL, "token=");
	U32 nUrlLen = strlen(pURL);

	RTMP_CHECK_HDL(nHandle);

	if (pToken != NULL)
	{
		//只判断URL不包含token的部分是否有变化
		nUrlLen = pToken - pURL - 1;
	}

	if ((0 == strncmp(s_RtmpConf[nHandle].URL, pURL, nUrlLen))
			&& (s_RtmpConf[nHandle].State != RTMP_DISCONNECTED))
	{
		pr_info("URL is the same\n");
		//_mlog_write(GetStr(ID_S_RTMP_ADDR_NO_CHG), rtmp_log_prefix(nHandle));
		// mstream_request_idr(streamid);
		//s_RtmpConf[nHandle].bReqIFrame = TRUE;
		return 1;
	}

	mlog_write("%s 配置流媒体服务器地址", rtmp_log_prefix(nHandle));
	STRNCPY(s_RtmpConf[nHandle].URL, pURL, sizeof(s_RtmpConf[nHandle].URL));

	return 0;
}

S32 Rtmp_Start(S32 nHandle)
{
	RTMP_CHECK_HDL(nHandle);

	pthread_mutex_lock(&s_RtmpConf[nHandle].Mutex);

	if(strlen(s_RtmpConf[nHandle].URL) == 0)
	{
		pr_info("Invalid URL!\n");
		pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);
		return -1;
	}
	if(s_RtmpConf[nHandle].Handle != NULL)
	{
		pr_info("Rtmp already started!\n");
		mlog_write("%s 正在连接流媒体...", rtmp_log_prefix(nHandle));
		pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);
		return -1;
	}

	pr_info("JMC_Connect: %s\n", s_RtmpConf[nHandle].URL);
	//_mlog_write(GetStr(ID_S_RTMP_CONNECT), rtmp_log_prefix(nHandle));

	// 主码流和回放通道都要配置成大缓存(RTMP_STREAM(nHandle)分别为0、-1)
	s_RtmpConf[nHandle].Handle = JMC_Connect(s_RtmpConf[nHandle].URL, 1, (RTMP_STREAM(nHandle) > 0) ? (256 * 1024) : (512 * 1024), (void*)nHandle);

	if(NULL == s_RtmpConf[nHandle].Handle)
	{
		pr_info("JMC_Connect %s failed\n", s_RtmpConf[nHandle].URL);
		mlog_write("%s连接流媒体服务器失败!", rtmp_log_prefix(nHandle));
		pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);
		return -1;
	}

	s_RtmpConf[nHandle].bStopped = FALSE;
	s_RtmpConf[nHandle].State = RTMP_CONNECTING;

	pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);

	return 0;
}

S32 Rtmp_RegCallback(S32 nHandle, RtmpCallback pRtmpCallback, void* arg)
{
	RTMP_CHECK_HDL(nHandle);

	s_RtmpConf[nHandle].pCallback= pRtmpCallback;
	s_RtmpConf[nHandle].arg = arg;

	return 0;
}

S32 Rtmp_UpdateMetadata(S32 nHandle)
{
	JMC_Metadata_t Metadata;

	RTMP_CHECK_HDL(nHandle);

	pthread_mutex_lock(&s_RtmpConf[nHandle].Mutex);

	if(s_RtmpConf[nHandle].Handle == NULL || s_RtmpConf[nHandle].State != RTMP_CONNECTED)
	{
		pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);
		return -1;
	}

	pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);

	memset(&Metadata, 0, sizeof(JMC_Metadata_t));

	if (IS_LIVE_HDL(nHandle))
	{
		rtmp_GetLiveConf(RTMP_CHANNEL(nHandle), RTMP_STREAM(nHandle), &Metadata);
	}
	else if (s_RtmpConf[nHandle].pCallback != NULL)
	{
		s_RtmpConf[nHandle].pCallback(RTMP_EVENT_GETMETA, &Metadata, s_RtmpConf[nHandle].arg);
	}
	else
	{
		pr_info("ERROR: No metadata!\n");
		return -2;
	}

	pr_info("JMC_SendMetadata\n");
	Rtmp_SendMetaData(nHandle, &Metadata);

	return 0;
}

S32 Rtmp_SendMetaData(S32 nHandle, JMC_Metadata_t* pData)
{
	S32 ret = RET_SUCC;
	RTMP_CHECK_HDL(nHandle);

	if (NULL == pData)
	{
		pr_info("Invalid data!\n");
		return -1;
	}

	if ((0 == pData->nVideoWidth) || (0 == pData->nVideoHeight) || (0 == pData->nVideoFrameRateNum))
	{
		pr_info("Don't send empty metadata!!\n"
				"w: %d, h: %d, fr: %d, frd: %d\n",
				pData->nVideoWidth, pData->nVideoHeight, pData->nVideoFrameRateNum, pData->nVideoFrameRateDen);
		return -1;
	}

	pthread_mutex_lock(&s_RtmpConf[nHandle].Mutex);

	if(s_RtmpConf[nHandle].Handle == NULL || s_RtmpConf[nHandle].State != RTMP_CONNECTED)
	{
		pr_info("Don't send metadata before connection established!\n");
		pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);
		return -1;
	}

	ret = JMC_SendFrame(s_RtmpConf[nHandle].Handle, JMC_DATATYPE_METADATA, pData, sizeof(JMC_Metadata_t), 0, 0);
	
	pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);

	mlog_write("%s发送码流信息", rtmp_log_prefix(nHandle));

	//pr_info("nHandle: %d, w: %d, h: %d, fr: %d, nDuration: %d  ret %d\n", nHandle, pData->nVideoWidth, pData->nVideoHeight, pData->nVideoFrameRateNum, pData->nDuration, ret);

	pr_info("w: %d, h: %d, fr: %d, frd: %d, \n"
			"at: %d, ar: %d, ab: %d, ac: %d, \n"
			"pi: %s, pv: %s, nDuration: %d  ret %d\n", 
			pData->nVideoWidth, pData->nVideoHeight, pData->nVideoFrameRateNum, pData->nVideoFrameRateDen, 
			pData->nAudioDataType, pData->nAudioSampleRate, pData->nAudioSampleBits, pData->nAudioChannels, 
			pData->szPublisherInfo, pData->szPublisherVer, pData->nDuration, 
			ret);
	
	return RET_SUCC;
}

S32 Rtmp_ConvDataType(S32 nType, U16 AudioCodec)
{
	S32 JMC_Type = -1;

	switch(nType)
	{
	case JV_FRAME_TYPE_I:			JMC_Type = JMC_DATATYPE_H264_I;			break;
	case JV_FRAME_TYPE_B:			JMC_Type = JMC_DATATYPE_H264_BP;		break;
	case JV_FRAME_TYPE_P:			JMC_Type = JMC_DATATYPE_H264_BP;		break;
	case JV_FRAME_TYPE_A:
		switch (AudioCodec)
		{
		case JV_AUDIO_ENC_G711_A:		JMC_Type = JMC_DATATYPE_ALAW;		break;
		case JV_AUDIO_ENC_G711_U:		JMC_Type = JMC_DATATYPE_ULAW;		break;
		default:															break;
		}
		break;
	default:															break;
	}	

	return JMC_Type;
}

S32 Rtmp_SendData(S32 nHandle, S32 JMC_Type, S8 *pData, S32 nSize, S32 nPts, S32 nDts)
{
	S32 ret = 0;
	static S32 told = 0;
	S32 tnow = time(NULL);

	RTMP_CHECK_HDL(nHandle);

	pthread_mutex_lock(&s_RtmpConf[nHandle].Mutex);

	//printf("time=%lld, Rtmp_SendData, nHandle: %d, Handle: %08x, state: %d!\n", Common_GetMs(), nHandle, (U32)s_RtmpConf[nHandle].Handle, s_RtmpConf[nHandle].State);

	if(s_RtmpConf[nHandle].Handle == NULL || s_RtmpConf[nHandle].State != RTMP_CONNECTED)
	{
		pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);
		return -1;
	}

	switch (JMC_Type)
	{
	case JMC_DATATYPE_H264_I:
	case JMC_DATATYPE_H264_BP:
	case JMC_DATATYPE_ALAW:
	case JMC_DATATYPE_ULAW:
		// if (0 == nPts)				// 手动添加时间戳
		// 全部手动添加时间戳，防止音视频时间戳不对应导致音频异常的问题。
		{
			nPts = Common_GetMs();
		}
		break;
	default:
		break;
	}

#if 0
	if (s_RtmpConf[nHandle].bReqIFrame)
	{
		if (JMC_DATATYPE_H264_I == JMC_Type)
		{
			ret = JMC_SendFrame(s_RtmpConf[nHandle].Handle, JMC_Type, pData, nSize, nPts, nDts);
			s_RtmpConf[nHandle].bReqIFrame = FALSE;
		}
		ret = -2;
	}
	else
	{
		ret = JMC_SendFrame(s_RtmpConf[nHandle].Handle, JMC_Type, pData, nSize, nPts, nDts);
	}
#else
	ret = JMC_SendFrame(s_RtmpConf[nHandle].Handle, JMC_Type, pData, nSize, nPts, nDts);
#endif
	#if 0
	switch (JMC_Type)
	{
	case JMC_DATATYPE_H264_I:
		pr_info("time=%d, nPts: %d, nHandle: %d, IIIIIIIIII, nSize: %d, ret: %d!\n", nPts, nOldPts, nHandle, nSize, ret);
		break;
	case JMC_DATATYPE_H264_BP:
		pr_info("time=%d, nPts: %d, nHandle: %d, PPPPPPPPPP, nSize: %d, ret: %d!\n", nPts, nOldPts, nHandle, nSize, ret);
		break;
	case JMC_DATATYPE_ALAW:
	case JMC_DATATYPE_ULAW:
		pr_info("time=%d, nPts: %d, nHandle: %d, AAAAAAAAAA, nSize: %d, ret: %d!\n", nPts, nOldPts, nHandle, nSize, ret);
		break;
	case JMC_DATATYPE_CUSTOM:
		// pr_info("time=%lld, nHandle: %d, CCCCCCCCCC, Data: %s, ret: %d!\n", Common_GetMs(), nHandle, pData, ret);
		break;
	default:
		break;
	}
	#endif
	if(ret == 0)
	{
		if(tnow % 30 == 0 && tnow != told)
		{
			told = tnow;
			pr_info("ret: %d!\n", ret);
			//_mlog_write(GetStr(ID_S_RTMP_BUFFER_FLOW), rtmp_log_prefix(nHandle), ret);			
		}
	}

	pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);

	return 0;
}

S32 Rtmp_GetStatus(S32 nHandle)
{
	RTMP_CHECK_HDL(nHandle);

	return s_RtmpConf[nHandle].State;
}

S32 Rtmp_Stop(S32 nHandle)
{
	S32 ret = 0;

	pthreadinfo_add((char *)__func__);

	RTMP_CHECK_HDL(nHandle);

	pthread_mutex_lock(&s_RtmpConf[nHandle].Mutex);

	if(s_RtmpConf[nHandle].Handle != NULL && s_RtmpConf[nHandle].State != RTMP_CONNECTING)
	{
		pr_info("before JMC_Close\n");
		JMC_Close(s_RtmpConf[nHandle].Handle);
		pr_info("after JMC_Close\n");

		//memset(s_RtmpConf[nHandle].URL, 0, sizeof(s_RtmpConf[nHandle].URL));
		s_RtmpConf[nHandle].State = RTMP_DISCONNECTED;
		s_RtmpConf[nHandle].pCallback = NULL;
		s_RtmpConf[nHandle].Handle = NULL;
		ret = 0;
	}
	else
	{
		ret = -1;
	}
	pthread_mutex_unlock(&s_RtmpConf[nHandle].Mutex);

	return ret;
}

static char* rtmp_log_prefix(S32 nHandle)
{
	static char Log_Prefix[LOG_PREFIX_LEN];
	static pthread_mutex_t Log_Mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&Log_Mutex);

	if (!IS_VALID_HDL(nHandle))
	{
		Log_Prefix[0] = 0;
		return Log_Prefix;
	}
	else if (IS_LIVE_HDL(nHandle))
	{
		sprintf(Log_Prefix, "chn:%d stream:%d", RTMP_CHANNEL(nHandle) + 1, RTMP_STREAM(nHandle) + 1);
	}
	else
	{
		sprintf(Log_Prefix, "%d", RTMP_PLAYINDEX(nHandle) + 1);
	}

	pthread_mutex_unlock(&Log_Mutex);

	return Log_Prefix;
}

static void* rtmp_ReconnThread(void *param)
{
	S32 ret = 0;
	S32 nHandle = 0;

	pthreadinfo_add(__func__);

	pr_info("Reconnect_thread started!\n");

	if(bReconThreadRunning)
	{
		pr_info("Reconnect_thread is running!\n");
		return NULL;
	}

	bReconThreadRunning = TRUE;
	while(bReconThreadRunning)
	{
		// 回放流媒体不做重连
		for (nHandle = 0; nHandle < MAX_LIVE_NUM; ++nHandle)
		{
			if((s_RtmpConf[nHandle].State == RTMP_DISCONNECTED) && (strlen(s_RtmpConf[nHandle].URL) > 0))
			{
				// 流媒体无人观看时，不重连
				if (s_RtmpConf[nHandle].bStopped)
				{
					continue;
				}
				Rtmp_Stop(nHandle);
				sleep(1);
				ret = Rtmp_Start(nHandle);
				if(ret < 0)
				{
					pr_info("nHandle %d, Rtmp_Start failed!\n", nHandle);
				}
			}
		}
		Wait_timeout(!bReconThreadRunning, 15*1000);
		//sleep(15);
	}

	pr_info("Reconnect_thread end\n");

	return NULL;
}

#if 0
static S32 rtmp_CheckLog()
{
	const S8 *logFile = LOGDIR"libjmc.log";
	struct stat buf;
	S32 ret = 0;

	pr_info("libjmc _check_log\n");
	memset(&buf, 0, sizeof(struct stat));

	ret = stat(logFile, &buf);
	if(ret == 0)
	{
		S32 fileSize = (S32)(buf.st_size);
		if(fileSize >= LOGSIZE)
		{
			pr_info("JMC log clear, size=%d\n", fileSize);
			//_mlog_write("清除JMC日志!");
			utl_system("rm -rf "LOGDIR"libjmc.log");
		}
	}
	return 0;
}
#endif

static S32 rtmp_GetLiveConf(U8 Channel, U8 streamid, JMC_Metadata_t* pData)
{

	//pr_info("time=%ld, rtmp connected\n", time(NULL));
	mlog_write("stream_%d连接流媒体服务器成功!", streamid);
	
	mstream_attr_t streamAttr;
	jv_audio_attr_t aenc;

	memset(&streamAttr, 0, sizeof(streamAttr));
	memset(&aenc, 0, sizeof(aenc));
	
	mstream_get_param(streamid, &streamAttr);
	jv_ai_get_attr(0, &aenc);
	pData->nAudioChannels = 1;
	if(aenc.encType == JV_AUDIO_ENC_G711_A)
		pData->nAudioDataType = JMC_DATATYPE_ALAW;
	else if(aenc.encType == JV_AUDIO_ENC_G711_U)
		pData->nAudioDataType = JMC_DATATYPE_ULAW;
	
	if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_8)
		pData->nAudioSampleBits = 8;
	else if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_16)
		pData->nAudioSampleBits = 16;
	else if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_32)
		pData->nAudioSampleBits = 32;
	pData->nAudioSampleRate = aenc.sampleRate;
	pData->nVideoFrameRateDen = 1;
	pData->nVideoFrameRateNum = streamAttr.framerate;
	pData->nVideoHeight = streamAttr.height;
	pData->nVideoWidth = streamAttr.width;
	pData->szPublisherInfo = NULL;
	pData->szPublisherVer = NULL;

	return 0;
}

static void rtmp_EventCallback(JMC_HANDLE hHandle, void* pUserData, S32 nEvent, const char* pParam)
{
	S32 nHandle = (S32)pUserData;

	// pr_info("nHandle: %d, nEvent: %d!\n", nHandle, nEvent);

	RTMP_CHECK_HDL_NORET(nHandle);

	//rtmp_CheckLog();
	if(nEvent == JMC_EVENTTYPE_CONNECTED)
	{
		pr_info("nHandle: %d, Rtmp event: connected!\n", nHandle);
		mlog_write("%s连接流媒体服务器成功!", rtmp_log_prefix(nHandle));
		s_RtmpConf[nHandle].State = RTMP_CONNECTED;

		Rtmp_UpdateMetadata(nHandle);
		if (IS_LIVE_HDL(nHandle))
		{
			mstream_request_idr(RTMP_STREAM(nHandle));
		}
	}
	else if(nEvent == JMC_EVENTTYPE_INSKEYFRAME)
	{
		//pr_info("Rtmp event: request IDR\n");
		pr_info("nHandle: %d, Rtmp event: request IDR!\n", nHandle);
		if (IS_LIVE_HDL(nHandle))
		{
			mstream_request_idr(RTMP_STREAM(nHandle));
		}
		//s_RtmpConf[nHandle].bReqIFrame = TRUE;
	}
	else if(nEvent == JMC_EVENTTYPE_CONNECTFAILED)
	{
		s_RtmpConf[nHandle].State = RTMP_DISCONNECTED;
		pr_info("nHandle: %d, Rtmp event: connect failed!\n", nHandle);
		mlog_write("%s连接流媒体服务器失败!",rtmp_log_prefix(nHandle));
	}
	else if(nEvent == JMC_EVENTTYPE_DISCONNECTED)
	{
		s_RtmpConf[nHandle].State = RTMP_DISCONNECTED;
		pr_info("nHandle: %d, Rtmp event: disconnected!\n", nHandle);
		mlog_write("%s流媒体连接断开!", rtmp_log_prefix(nHandle));

		if (s_RtmpConf[nHandle].pCallback != NULL)
		{
			s_RtmpConf[nHandle].pCallback(RTMP_EVENT_DISCON, NULL, s_RtmpConf[nHandle].arg);
		}
	}
	else if(nEvent == JMC_EVENTTYPE_STOPSTREAM)
	{
		pr_info("nHandle: %d, Rtmp event: stop stream!\n", nHandle);

		if (s_RtmpConf[nHandle].pCallback != NULL)
		{
			pr_info("===================Rtmp event: stop stream!===============\n");
			s_RtmpConf[nHandle].pCallback(RTMP_EVENT_DISCON, NULL, s_RtmpConf[nHandle].arg);
		}
		else
		{
			// 无人观看
			s_RtmpConf[nHandle].bStopped = TRUE;
			pthread_t pid;
			pthread_create_detached(&pid, NULL, (void* (*)(void*))Rtmp_Stop, (void*)nHandle);
		}
	}
	else
	{
		pr_info("nHandle: %d, Rtmp Unknown event: %d\n", nHandle, nEvent);
	}
}

static void rtmp_DataCallback(JMC_HANDLE hHandle, void* pUserData, S32 nType, void* pData, S32 nSize, S32 nPts, S32 nDts)
{
	pr_info("Rtmp_DataCallback\n");
}

