#include "jv_common.h"
#include "mrtmp.h"
#include "JvMediaClient.h"
#include "mstream.h"
#include "mlog.h"
#include <dlfcn.h>

#define DEBUGLEVEL	JMC_LOGLEVEL_ALL
#define LOGDIR	"/tmp/jmclog/"
#define LOGSIZE	128*1024

static JMC_HANDLE rtmpHandle[MAX_STREAM] = {NULL};
static char rtmpURL[MAX_STREAM][1024];
//static BOOL bConnected[MAX_STREAM] = {FALSE};
//static BOOL bConnecting[MAX_STREAM] = {FALSE};
static RTMP_STATUS_e rtmpConnStatus[MAX_STREAM] = {RTMP_DISCONNECTED};
static pthread_t reconThread = 0;
static BOOL bReconThreadRunning = FALSE;
static int reconStreamid = 0;
static pthread_mutex_t rtmpMutex = PTHREAD_MUTEX_INITIALIZER;

void* reconnect_thread(void *param)
{

	pthreadinfo_add((char *)__func__);

	printf("time=%ld, reconnect_thread\n", time(NULL));
	int ret = 0;
	int count = 0;
	int streamid = 0;
	if(bReconThreadRunning)
	{
		printf("time=%ld, reconnect_thread is running\n", time(NULL));
		return NULL;
	}
	bReconThreadRunning = TRUE;
	while(bReconThreadRunning)
	{
		for(streamid = 0; streamid<MAX_STREAM; streamid++)
		{
			if(rtmpConnStatus[streamid] == RTMP_DISCONNECTED && strlen(rtmpURL[streamid]) > 0)
			{
				mrtmp_stop(streamid);
				sleep(1);
				ret = mrtmp_start(streamid);
				if(ret >= 0)
				{
					/*
					while(mrtmp_getStatus(streamid) != 1)
					{
						count++;
						if(count > 10)
						{
							break;
						}
						usleep(300000);
					}
					if(mrtmp_getStatus(streamid) == 1)
					{
						RTMP_Metadata_t metadata;
						mstream_attr_t streamAttr;
						jv_audio_attr_t aenc;
						
						mstream_get_param(streamid, &streamAttr);
						jv_ai_get_attr(0, &aenc);
						metadata.nAudioChannels = 1;
						if(aenc.encType == JV_AUDIO_ENC_G711_A)
							metadata.nAudioDataType = 0x11;
						else if(aenc.encType == JV_AUDIO_ENC_G711_U)
							metadata.nAudioDataType = 0x12;
						
						if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_8)
							metadata.nAudioSampleBits = 8;
						else if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_16)
							metadata.nAudioSampleBits = 16;
						else if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_32)
							metadata.nAudioSampleBits = 32;
						metadata.nAudioSampleRate = aenc.sampleRate;
						metadata.nVideoFrameRateDen = 1;
						metadata.nVideoFrameRateNum = streamAttr.framerate;
						metadata.nVideoHeight = streamAttr.height;
						metadata.nVideoWidth = streamAttr.width;
						
						mrtmp_sendMetadata(streamid, &metadata, sizeof(metadata));
						mstream_request_idr(streamid);
					}
					*/
				}
			}
		}
		sleep(15);
	}
	printf("time=%ld, reconnect_thread out\n", time(NULL));
	return NULL;
}

/*
static int _check_log()
{
	printf("libjmc _check_log\n");
	const char *logFile = LOGDIR"libjmc.log";
	struct stat buf;
	memset(&buf, 0, sizeof(struct stat));
	int ret = stat(logFile, &buf);
	if(ret == 0)
	{
		int fileSize = (int)(buf.st_size);
		printf("log file exists, size=%d\n", fileSize);
		if(fileSize >= LOGSIZE)
		{
			mlog_write("清除JMC日志!");
			utl_system("rm -rf "LOGDIR"libjmc.log");
		}
	}
	return 0;
}
*/

void funEventCallback(JMC_HANDLE hHandle, void *pUserData, int nEvent, const char *szMsg)
{
	int streamid = 0;
	int i = 0;
	for(i=0; i<MAX_STREAM; i++)
	{
		if(rtmpHandle[i] == hHandle)
		{
			streamid = i;
			break;
		}
	}
	//_check_log();
	if(nEvent == JMC_EVENTTYPE_CONNECTED)
	{
		printf("time=%ld, rtmp connected\n", time(NULL));
		mlog_write("stream_%d连接流媒体服务器成功!", streamid);
		RTMP_Metadata_t metadata;
		mstream_attr_t streamAttr;
		jv_audio_attr_t aenc;
		
		mstream_get_param(streamid, &streamAttr);
		jv_ai_get_attr(0, &aenc);
		metadata.nAudioChannels = 1;
		if(aenc.encType == JV_AUDIO_ENC_G711_A)
			metadata.nAudioDataType = 0x11;
		else if(aenc.encType == JV_AUDIO_ENC_G711_U)
			metadata.nAudioDataType = 0x12;
		
		if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_8)
			metadata.nAudioSampleBits = 8;
		else if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_16)
			metadata.nAudioSampleBits = 16;
		else if(aenc.bitWidth == JV_AUDIO_BIT_WIDTH_32)
			metadata.nAudioSampleBits = 32;
		metadata.nAudioSampleRate = aenc.sampleRate;
		metadata.nVideoFrameRateDen = 1;
		metadata.nVideoFrameRateNum = streamAttr.framerate;
		metadata.nVideoHeight = streamAttr.height;
		metadata.nVideoWidth = streamAttr.width;
		metadata.szPublisherInfo = NULL;
		metadata.szPublisherVer = NULL;
		
		mrtmp_sendMetadata(streamid, &metadata, sizeof(metadata));
		mstream_request_idr(streamid);
		rtmpConnStatus[streamid] = RTMP_CONNECTED;
		
	}
	else if(nEvent == JMC_EVENTTYPE_INSKEYFRAME)
	{
		printf("time=%ld, rtmp request IDR\n", time(NULL));
		mstream_request_idr(streamid);		
	}
	else if(nEvent == JMC_EVENTTYPE_CONNECTFAILED)
	{
		rtmpConnStatus[streamid] = RTMP_DISCONNECTED;
		printf("time=%ld, rtmp connect failed\n", time(NULL));
		mlog_write("stream_%d连接流媒体服务器失败!", streamid);
	}
	else if(nEvent == JMC_EVENTTYPE_DISCONNECTED)
	{
		rtmpConnStatus[streamid] = RTMP_DISCONNECTED;
		printf("time=%ld, rtmp disconnected\n", time(NULL));
		mlog_write("stream_%d流媒体服务器连接断开!", streamid);
	}
	else
	{
		rtmpConnStatus[streamid] = RTMP_DISCONNECTED;
		printf("time=%ld, unknown event\n", time(NULL));
		mlog_write("连接失败，原因未知");
	}
}

void funDataCallback(JMC_HANDLE hHandle, void *pUserData, int nType, void *pData, int nSize, int nPts, int nDts)
{
	printf("funDataCallback\n");
	
}

int mrtmp_init()
{
	JMC_EnableLog(DEBUGLEVEL, LOGDIR, 128*1024);
	const char * version = JMC_GetVersion();
	JMC_RegisterCallback(funEventCallback, funDataCallback);
	if(version != NULL)
	{
		printf("time=%ld, jmc version:%s\n", time(NULL), version);
		mlog_write("流媒体库版本:%s", version);
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&reconThread, NULL, reconnect_thread, NULL);
	pthread_attr_destroy (&attr);
	return 0;
}

int mrtmp_deinit()
{
	bReconThreadRunning = FALSE;
	return 0;
}

int mrtmp_start(int streamid)
{
	pthread_mutex_lock(&rtmpMutex);
	if(strlen(rtmpURL[streamid]) == 0)
	{
		pthread_mutex_unlock(&rtmpMutex);
		return -1;
	}
	if(rtmpHandle[streamid] != NULL)
	{
		printf("time=%ld, rtmp already started\n", time(NULL));
		mlog_write("正在连接流媒体");
		pthread_mutex_unlock(&rtmpMutex);
		return -1;
	}
	rtmpConnStatus[streamid] = RTMP_CONNECTING;
	
	printf("time=%ld, JMC_Connect:%s\n", time(NULL), rtmpURL[streamid]);
	mlog_write("stream_%d连接:%s", streamid, rtmpURL[streamid]);
	rtmpHandle[streamid] = JMC_Connect(rtmpURL[streamid], 1, (streamid>0)?(512*1024):(1024*1024), NULL);
	if(rtmpHandle[streamid] == NULL)
	{
		printf("time=%ld, JMC_Connect %s failed\n", time(NULL), rtmpURL[streamid]);
		mlog_write("stream_%d连接流媒体服务器失败", streamid);
		pthread_mutex_unlock(&rtmpMutex);
		return -1;
	}
	pthread_mutex_unlock(&rtmpMutex);
	return 0;
}

int mrtmp_set_url(int streamid, const char *url)
{

#if 1
	char *token = strstr(url, "token=");
	if(token == NULL)
	{
		printf("time=%ld, rtmpURL is incorrect\n", time(NULL));
		mlog_write("stream_%d流媒体地址错误!", streamid);
		return -1;
	}

	//只判断URL不包含token的部分是否有变化
	if(strncmp(rtmpURL[streamid], url, token-url-1) == 0 && rtmpConnStatus[streamid] != RTMP_CONNECTING)
	{
		printf("time=%ld, rtmpURL is the same\n", time(NULL));
		mlog_write("stream_%d流媒体地址不变，不用重新配置!", streamid);
		mstream_request_idr(streamid);
		return -1;
	}
#else
	if(strcmp(rtmpURL[streamid], url) == 0 && bConnected[streamid] == TRUE)
	{
		printf("rtmpURL is the same\n");
		mlog_write("流媒体地址不变，不用重新配置!");
		return -1;
	}
#endif

	mlog_write("stream_%d配置流媒体服务器地址!", streamid);
	strcpy(rtmpURL[streamid], url);
	return 0;
}

int mrtmp_sendMetadata(int streamid, RTMP_Metadata_t *pData, int nSize)
{
	pthread_mutex_lock(&rtmpMutex);
	if(rtmpHandle[streamid] != NULL)
	{
		printf("time=%ld, mrtmp_sendMetadata\n", time(NULL));
		mlog_write("stream_%d发送Metadata", streamid);
		JMC_SendFrame(rtmpHandle[streamid], 0x00, pData, nSize, 0, 0);
	}
	pthread_mutex_unlock(&rtmpMutex);
	return 0;

}

int mrtmp_sendframe(int streamid, int nType, char *pData, int nSize, int nPts, int nDts)
{
	int ret = 0;
	pthread_mutex_lock(&rtmpMutex);
	if(rtmpHandle[streamid] != NULL && rtmpConnStatus[streamid] == RTMP_CONNECTED)
	{
		static int told = 0;
		int tnow = time(NULL);
		//printf("nType=%d\n", nType);
		ret = JMC_SendFrame(rtmpHandle[streamid], nType, pData, nSize, nPts, nDts);
		if(ret == 0)
		{
			if(tnow%60 == 0 && tnow != told)
			{
				told = tnow;
				//printf("time=%ld, RTMP发送数据返回:%d!\n", time(NULL), ret);
				mlog_write("stream_%dRTMP发送数据返回:%d!", streamid, ret);			
			}
		}
	}
	pthread_mutex_unlock(&rtmpMutex);
	return 0;
}

int mrtmp_getStatus(int streamid)
{
	return rtmpConnStatus[streamid];
}

int mrtmp_stop(int streamid)
{
	pthread_mutex_lock(&rtmpMutex);
	if(rtmpHandle[streamid] != NULL && rtmpConnStatus[streamid] != RTMP_CONNECTING)
	{
		printf("time=%ld, before JMC_Close\n", time(NULL));
		JMC_Close(rtmpHandle[streamid]);
		printf("time=%ld, JMC_Close\n", time(NULL));
		rtmpHandle[streamid] = NULL;
	}
	pthread_mutex_unlock(&rtmpMutex);
	return 0;
}

