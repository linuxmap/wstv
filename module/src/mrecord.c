

#include "jv_common.h"
#include "mstream.h"
#include "mstorage.h"
#include "mlog.h"
#include "utl_timer.h"
#include "mrecord.h"
#include "Jmp4pkg.h"
#include <sp_connect.h>
#include <utl_ifconfig.h>
#include "malarmin.h"
#include "msensor.h"
#include "mcloud.h"
#include "jhlsupload.h"
#include "JvServer.h"
#include "mtransmit.h"
#include "mipcinfo.h"
#include "SYSFuncs.h"
#include "mioctrl.h"
#include "mvoicedec.h"
#include "maudio.h"
#include "malarmout.h"
#include "utl_common.h"
#include "utl_algorithm.h"

#define MRECORED_FIFO		//是否开启录像分离线程,FIFO模式

#define CLOUD_ALARM_REC_LEN		(30)
#define CLOUD_ALARM_TS_LEN		(10)
static int MIN_MB_SPACE_NEEDED		=	200;
int record_chn = 0;
#define _MP4_INDEX ".jdx"
#define ALIGN_4(x)	((((unsigned int)(x)) + 3) & 0xFFFFFFFC)
#define U8P_CAST(x)	((unsigned char *)(x))
typedef struct{
	int timer;
	alarm_type_e alarmType;//要执行的动作
	unsigned int alarmStartSecond;//报警录像的开始时间
	RECTYPE type;
	MP4_PKG_HANDLE fd;
	JVS_FILE_HEADER	stHeader;
	int byteWrited;	//已经写过的字节数
	U32 startSecond;//录像的开始时间(系统运行时间)
	char filenme[128];//录像文件名

	REC_REQ_PARAM	ReqParam;		// 临时性录像请求
}record_status_t;

typedef struct {
	pthread_mutex_t mutex;
	mrecord_attr_t recorelist[MAX_REC_TASK];
	record_status_t status[MAX_REC_TASK];
}record_info_t;

static record_info_t recordinfo;
static char mrecord_onemin_stop = 0;
typedef struct
{
	JHLSHandle_t  hlsHandle;
	char hlsName[128];
	int type;
	U32 alarmTime; //报警时间; 
	U32 startSecond;//报警录像的开始时间;
	BOOL bHasIFrame;
	pthread_mutex_t mutex;
}cloudStorage_t;
static cloudStorage_t cloudStorage = {0};

typedef struct _prerecord_head_t
{
	unsigned char *buffer;
	int len;
	unsigned int frametype;
	unsigned long long timeStamp;
	struct _prerecord_head_t *next;
}prerecord_head_t;

typedef struct
{
	unsigned char *bufferStart ;//环形BUFFER
	int bufferLen ;
	unsigned char *bufferEnd ;
	prerecord_head_t *first;//第一条有效帧
	prerecord_head_t *last;//最后一条有效帧
	int totalFrameNum;
	int curFrameNum;
}prerecord_info_t;

typedef struct
{
	unsigned char *bufferStart ;//环形BUFFER
	int bufferLen ;
	unsigned char *bufferEnd ;
	prerecord_head_t *first;//第一条有效帧
	prerecord_head_t *last;//最后一条有效帧
	pthread_mutex_t mutex;
	pthread_cond_t dataReady;	// 环形BUFFER不为空的条件变量
	pthread_t consumerTid;
	int shutdown;
}cloudrecord_info_t;
static cloudrecord_info_t cloudinfo[MAX_REC_TASK];

static int _mrecord_stop(int channelid);
static void* _mrecord_timing_thread(void* p);

#ifdef MRECORED_FIFO
int  __mrecord_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timestamp, BOOL *bWaitIframe);
static cloudrecord_info_t recorde_fifio_intfo[MAX_REC_TASK];
#endif

#define ENABLE_PRE_RECORD	1

/*
预录制功能，实现方式：

bufferLen = mrecord_attr_t.alarm_pre_record * mstream_attr_t.bitrate * 1.5;
bufferStart = malloc(bufferLen );
__________________________________________________
|head0|...data0... |head1| ... data1 ...|...
|__________________________________________________

1，prerecord_buffer作为一整个循环Buffer，持续写入预录制的内容
2，其内存，是临时申请的。其大小，为预录制时间x 码率x 1.2
3，在未开启录像功能时，才会做预录制动作
4，在push一条新记录时，记录第一条有效内容。
5，一但开启录像功能，无论是哪一种录像，则先将预录制的内容写入文件中
6，为避免一次性写入造成延迟，采取每来一帧新数据，写入两帧的方式，直到
    所有预录制内容都写入文件，再恢复正常写入。

*/

static prerecord_info_t preinfo[MAX_REC_TASK];
static int pre_prepare[2] = {1, 1};

void mrecord_pre_reinit()
{
	int i = 0;
	for(i = 0; i < MAX_REC_TASK; i++)
		pre_prepare[i] = 1;
}

static void _mrecord_pre_prepare(int channelid)
{
#if ENABLE_PRE_RECORD

	mstream_attr_t attr;
	if (preinfo[channelid].bufferStart != NULL)
	{
		free(preinfo[channelid].bufferStart );
		preinfo[channelid].bufferStart = NULL;
	}

	mstream_get_running_param(channelid,&attr);

	/*recordinfo.recorelist[channelid].alarm_pre_record += 1;*/

	preinfo[channelid].curFrameNum = 0;
	preinfo[channelid].totalFrameNum = (recordinfo.recorelist[channelid].alarm_pre_record) * (attr.framerate + 25);
	preinfo[channelid].bufferLen  = (recordinfo.recorelist[channelid].alarm_pre_record) * 
		(attr.bitrate * 1024 / 8 + 25 * 320);

	preinfo[channelid].bufferStart = malloc(preinfo[channelid].bufferLen);
	if (preinfo[channelid].bufferStart == NULL)
	{
		printf("No Enough memory!\n");
		return ;
	}
	preinfo[channelid].bufferEnd = preinfo[channelid].bufferStart + preinfo[channelid].bufferLen;
	printf("_mrecord pre_prepare start: 0x%x ,end: 0x%x, len: %d, num: %d, bitrate: %d\n", (int)preinfo[channelid].bufferStart, (int)preinfo[channelid].bufferEnd, preinfo[channelid].bufferLen, preinfo[channelid].totalFrameNum, attr.bitrate);
	preinfo[channelid].first = NULL;
	preinfo[channelid].last = NULL;

#endif
}

static int _mrecord_pre_push(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timestamp)
{
#if ENABLE_PRE_RECORD
	prerecord_head_t *cur;

	if (pre_prepare[channelid] == 1)
	{
		pre_prepare[channelid] = 0;
		_mrecord_pre_prepare(channelid);
		if (preinfo[channelid].bufferStart == NULL)
			return -1;
	}
	if (preinfo[channelid].last == NULL)
	{
		preinfo[channelid].first = preinfo[channelid].last = (prerecord_head_t *)preinfo[channelid].bufferStart;
		cur = preinfo[channelid].last;

		preinfo[channelid].curFrameNum++;
	}
	else
	{
		cur = preinfo[channelid].last->next;

		preinfo[channelid].curFrameNum++;

		if (U8P_CAST(cur) + sizeof(prerecord_head_t) + len > preinfo[channelid].bufferEnd)
		{
			cur = (prerecord_head_t *)preinfo[channelid].bufferStart;
			preinfo[channelid].last->next = cur;
		}

		while ((cur <= preinfo[channelid].first
			&& U8P_CAST(cur) + sizeof(prerecord_head_t) + len + 3 >= U8P_CAST(preinfo[channelid].first)) ||
			preinfo[channelid].curFrameNum > preinfo[channelid].totalFrameNum)
		{
			do
			{
				preinfo[channelid].first = preinfo[channelid].first->next;
				preinfo[channelid].curFrameNum--;
			}while(preinfo[channelid].first->frametype != JV_FRAME_TYPE_I && preinfo[channelid].first != preinfo[channelid].last);
		}
	}

	cur->buffer = U8P_CAST(cur) + sizeof (prerecord_head_t);
	cur->len = len;
	cur->frametype = frametype;
	cur->timeStamp = timestamp;
	memcpy(cur->buffer, buffer, len);
	cur->next = (prerecord_head_t *)ALIGN_4(cur->buffer + len);
	preinfo[channelid].last = cur;
	//printf("Push: 0x%x, len: %d,  start: 0x%x ,end: 0x%x, cur_num: %d, first: 0x%x, last: 0x%x\n", (int)cur, len, (int)preinfo[channelid].bufferStart, (int)preinfo[channelid].bufferEnd, preinfo[channelid].curFrameNum, (int)preinfo[channelid].first, (int)preinfo[channelid].last);

#endif
	return 0;
}

static prerecord_head_t* _mrecord_pre_pull(int channelid)
{
	prerecord_head_t *pull = NULL;
#if ENABLE_PRE_RECORD

	pull = preinfo[channelid].first;
	if (pull == NULL)
	{
		return NULL;
	}
	if (pull <= preinfo[channelid].last)
	{
		if (U8P_CAST(pull->next) >= U8P_CAST(preinfo[channelid].last))
		{
			preinfo[channelid].first = NULL;
			preinfo[channelid].last = NULL;

			preinfo[channelid].curFrameNum = 0;
			Printf("pull last one\n");
			return pull;
		}
	}
	preinfo[channelid].first = pull->next;
	preinfo[channelid].curFrameNum--;
	//printf("Pull: 0x%x, len: %d, last-first: %d, cun_num: %d\n", (int)pull, pull->len, U8P_CAST(preinfo[channelid].last)- U8P_CAST(preinfo[channelid].first), preinfo[channelid].curFrameNum);

#endif
	return pull;
}

static void *_mrecord_cloudupload_process(void *arg);
static void _mrecord_cloud_prepare(int channelid)
{
#ifdef OBSS_CLOUDSTORAGE

	if (cloudinfo[channelid].bufferStart!= NULL)
	{
		free(cloudinfo[channelid].bufferStart );
		cloudinfo[channelid].bufferStart = NULL;
	}

	cloudinfo[channelid].bufferLen  = 256*1024;//512 * 1024;
	if (cloudinfo[channelid].bufferLen == 0)
	{
		Printf("Pre Record not setted\n");
		return ;
	}
	cloudinfo[channelid].bufferStart = (unsigned char *)malloc(cloudinfo[channelid].bufferLen);
	if (cloudinfo[channelid].bufferStart == NULL)
	{
		printf("No Enough memory!\n");
		return ;
	}
	cloudinfo[channelid].bufferEnd = cloudinfo[channelid].bufferStart + cloudinfo[channelid].bufferLen;
	//printf("start: 0x%x ,end: 0x%x, len: %d\n", cloudinfo[channelid].bufferStart, cloudinfo[channelid].bufferEnd, cloudinfo[channelid].bufferLen);
	cloudinfo[channelid].first = NULL;
	cloudinfo[channelid].last = NULL;
	pthread_mutex_init(&cloudinfo[channelid].mutex, NULL);
	pthread_cond_init(&cloudinfo[channelid].dataReady, NULL);
	cloudinfo[channelid].shutdown = 0;
	pthread_create(&cloudinfo[channelid].consumerTid, NULL, _mrecord_cloudupload_process, (void *)channelid);

#endif
}

static int _mrecord_cloud_push(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timeStamp)
{
#ifdef OBSS_CLOUDSTORAGE

	if (buffer == NULL || len <= 0)
	{
		return -1;
	}

	pthread_mutex_lock(&cloudinfo[channelid].mutex);
	if (cloudinfo[channelid].bufferStart == NULL)
	{
		pthread_mutex_unlock(&cloudinfo[channelid].mutex);
		return -1;
	}

	int dataLen = ALIGN_4(sizeof(prerecord_head_t) + len);
	prerecord_head_t *cur = NULL;
	if (cloudinfo[channelid].last == NULL)
	{
		cloudinfo[channelid].first = cloudinfo[channelid].last = (prerecord_head_t *)cloudinfo[channelid].bufferStart;
		cur = cloudinfo[channelid].last;
	}
	else
	{
		cur = cloudinfo[channelid].last->next;
		prerecord_head_t *tmp = cur;
		if (U8P_CAST(cur) + sizeof(prerecord_head_t) + dataLen > cloudinfo[channelid].bufferEnd)
		{
			cur = (prerecord_head_t *)cloudinfo[channelid].bufferStart;
			cloudinfo[channelid].last->next = cur;
		}

		if (cur <= cloudinfo[channelid].first && 
			U8P_CAST(cur) + sizeof(prerecord_head_t) + dataLen + 3 > U8P_CAST(cloudinfo[channelid].first))
		{
			printf("wait for Consumer .... %d....\n", frametype);
			cloudinfo[channelid].last->next = tmp;
			pthread_mutex_unlock(&cloudinfo[channelid].mutex);
			return -1;
		}
/*
		while (cur <= cloudinfo[channelid].first && U8P_CAST(cur) + dataLen > U8P_CAST(cloudinfo[channelid].first))
		{
			cloudinfo[channelid].first = cloudinfo[channelid].first->next;
			printf("1111111111111111111111111111111111111111111111111\n");
		}
*/
	}

	//printf("Push: 0x%x, len: %d,  start: 0x%x ,end: 0x%x\n", cur, len, cloudinfo[channelid].bufferStart, cloudinfo[channelid].bufferEnd);
	cur->len = len;
	cur->frametype = frametype;
	cur->timeStamp = timeStamp;
	cur->buffer = U8P_CAST(cur) + ALIGN_4(sizeof(prerecord_head_t));
	memcpy(cur->buffer, buffer, len);
	cur->next = (prerecord_head_t *)(U8P_CAST(cur) + dataLen);
	cloudinfo[channelid].last = cur;
	pthread_mutex_unlock(&cloudinfo[channelid].mutex);
	pthread_cond_signal(&cloudinfo[channelid].dataReady);
#endif
	return 0;
}

static prerecord_head_t* _mrecord_cloud_pull(int channelid)
{
	prerecord_head_t *pull = NULL;
#ifdef OBSS_CLOUDSTORAGE

	pull = cloudinfo[channelid].first;
	if (pull == NULL)
	{
		return NULL;
	}

	if (pull == cloudinfo[channelid].last)
	{
		cloudinfo[channelid].first = cloudinfo[channelid].last = NULL;
	}
	else
	{
		cloudinfo[channelid].first = pull->next;
	}

#endif
	return pull;
}

static void *_mrecord_cloudupload_process(void *arg)
{
#ifdef OBSS_CLOUDSTORAGE

	int channelid = (int)arg;
	printf("_mrecord_cloudupload_process %d\n", channelid);
	prerecord_head_t *head = NULL;
	int bufLen = 0;
	unsigned int frametype;
	JHLSFrameType_e hlsFrameType;
	unsigned long long timeStamp;

	int maxLen = 128*1024;//2048 * 1024 / 8;
	U8 *buf = (U8 *)malloc(maxLen);
	if(buf == NULL)
		return NULL;
	pthreadinfo_add((char *)__func__);

	while (1)
	{
		pthread_mutex_lock(&cloudinfo[channelid].mutex);
		while (cloudinfo[channelid].last == NULL && cloudinfo[channelid].shutdown == 0)
		{
			//printf("wait for Producer ........\n");
			pthread_cond_wait(&cloudinfo[channelid].dataReady, &cloudinfo[channelid].mutex);
		}
		if (cloudinfo[channelid].shutdown != 0)
		{
			pthread_mutex_unlock(&cloudinfo[channelid].mutex);
			break;
		}
		head = _mrecord_cloud_pull(0);
		if (head)
		{
			bufLen = head->len;
			frametype = head->frametype;
			timeStamp = head->timeStamp;
			if(bufLen > maxLen)
			{
				U8 *prealloc = (U8 *)realloc(buf, bufLen);
				maxLen = bufLen;
				if(prealloc == NULL)
				{
					free(buf);
					return NULL;
				}
				else
					buf = prealloc;
			}
			memcpy(buf, head->buffer, bufLen);
			pthread_mutex_unlock(&cloudinfo[channelid].mutex);

			if(frametype == JV_FRAME_TYPE_I)
			{
				hlsFrameType = JHLS_FRAME_TYPE_VIDEO_I;
			}
			else if(frametype == JV_FRAME_TYPE_A)
			{
				hlsFrameType = JHLS_FRAME_TYPE_AUDIO;
			}
			else
			{
				hlsFrameType = JHLS_FRAME_TYPE_VIDEO_P;
			}

			if (utl_ifconfig_net_prepared > 0)
				jhlsup_inputData(cloudStorage.hlsHandle, hlsFrameType, timeStamp, (const unsigned char *)buf, bufLen, cloudStorage.type);

			if(jhlsup_bFinished(cloudStorage.hlsHandle))
			{
				printf("call jhlsup_close : filename=%s, time=%u, start=%u\n", cloudStorage.hlsName, utl_get_sec(), cloudStorage.startSecond);
				jhlsup_close(cloudStorage.hlsHandle);

				pthread_mutex_lock(&cloudStorage.mutex);
				cloudStorage.hlsHandle = NULL;
				cloudStorage.hlsName[0] = '\0';
				cloudStorage.type = 0;
				cloudStorage.alarmTime = 0;
				cloudStorage.startSecond = 0;
				cloudStorage.bHasIFrame = FALSE;
				pthread_mutex_unlock(&cloudStorage.mutex);

				pthread_mutex_lock(&cloudinfo[channelid].mutex);
				cloudinfo[channelid].first = cloudinfo[channelid].last = NULL;
				pthread_mutex_unlock(&cloudinfo[channelid].mutex);
			}
			 
		}
		else
		{
			pthread_mutex_unlock(&cloudinfo[channelid].mutex);
		}
	}

	free(buf);
	buf = NULL;

#endif
	return NULL;
}

void mrecord_cloud_destroy(int channelid)
{
#ifdef OBSS_CLOUDSTORAGE

	if (cloudinfo[channelid].shutdown == 1)
	{
		return;
	}
	cloudinfo[channelid].shutdown = 1;
	pthread_cond_broadcast(&cloudinfo[channelid].dataReady);
	pthread_join(cloudinfo[channelid].consumerTid, NULL);
	pthread_mutex_destroy(&cloudinfo[channelid].mutex);
	pthread_cond_destroy(&cloudinfo[channelid].dataReady);
	free(cloudinfo[channelid].bufferStart);
	cloudinfo[channelid].bufferStart = NULL;
	
#endif
}

void mrecord_cloud_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timeStamp)
{
#ifdef OBSS_CLOUDSTORAGE

#if ENABLE_PRE_RECORD
	// 数据一直进行缓存，防止出现跳跃
	_mrecord_pre_push(channelid, buffer, len, frametype, timeStamp);
#endif
	pthread_mutex_lock(&cloudStorage.mutex);
	static BOOL bError = FALSE;
	if(cloudStorage.hlsHandle != NULL)
	{
#if ENABLE_PRE_RECORD
		prerecord_head_t* tmp_record_info = NULL;
		tmp_record_info = _mrecord_pre_pull(channelid);

		if(NULL != tmp_record_info)
		{
			buffer = tmp_record_info->buffer;
			len    = tmp_record_info->len;
			frametype = tmp_record_info->frametype;
			timeStamp = tmp_record_info->timeStamp;
		}
#endif
		if(!cloudStorage.bHasIFrame && frametype != JV_FRAME_TYPE_I)
		{
			pthread_mutex_unlock(&cloudStorage.mutex);
			return ;
		}
	
		JHLSFrameType_e hlsFrameType;
		if(frametype == JV_FRAME_TYPE_I)
		{
			hlsFrameType = JHLS_FRAME_TYPE_VIDEO_I;
		}
		else if(frametype == JV_FRAME_TYPE_A)
		{
			hlsFrameType = JHLS_FRAME_TYPE_AUDIO;
		}
		else
		{
			hlsFrameType = JHLS_FRAME_TYPE_VIDEO_P;
		}
	
		if(!bError || frametype == JV_FRAME_TYPE_I)
		{
			if(_mrecord_cloud_push(0, buffer, len, frametype, timeStamp) != 0)
			{
				bError = TRUE;
			}
			else
			{
				bError = FALSE;
				if(frametype == JV_FRAME_TYPE_I)
				{
					cloudStorage.bHasIFrame = TRUE;
				}
			}
		}
	}
	pthread_mutex_unlock(&cloudStorage.mutex);

#endif
}

static int _mrecord_fifo_push(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timeStamp)
{
#ifdef MRECORED_FIFO

	if (buffer == NULL || len <= 0)
	{
		return -1;
	}

	pthread_mutex_lock(&recorde_fifio_intfo[channelid].mutex);
	if (recorde_fifio_intfo[channelid].bufferStart == NULL)
	{
		pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
		return -1;
	}

	int dataLen = ALIGN_4(sizeof(prerecord_head_t) + len);
	prerecord_head_t *cur = NULL;
	if (recorde_fifio_intfo[channelid].last == NULL)
	{
		recorde_fifio_intfo[channelid].first = recorde_fifio_intfo[channelid].last = (prerecord_head_t *)recorde_fifio_intfo[channelid].bufferStart;
		cur = recorde_fifio_intfo[channelid].last;
	}
	else
	{
		cur = recorde_fifio_intfo[channelid].last->next;
		prerecord_head_t *tmp = cur;
		if (U8P_CAST(cur) + sizeof(prerecord_head_t) + dataLen > recorde_fifio_intfo[channelid].bufferEnd)
		{
			cur = (prerecord_head_t *)recorde_fifio_intfo[channelid].bufferStart;
			recorde_fifio_intfo[channelid].last->next = cur;
		}

		if (cur <= recorde_fifio_intfo[channelid].first && 
			U8P_CAST(cur) + sizeof(prerecord_head_t) + dataLen + 3 > U8P_CAST(recorde_fifio_intfo[channelid].first))
		{
			printf("@@@@@@@@@@@@@@@@@@@@@%s  wait for Consumer .... %d....\n", __func__,frametype);
			recorde_fifio_intfo[channelid].last->next = tmp;
			pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
			return -1;
		}
/*
		while (cur <= cloudinfo[channelid].first && U8P_CAST(cur) + dataLen > U8P_CAST(cloudinfo[channelid].first))
		{
			cloudinfo[channelid].first = cloudinfo[channelid].first->next;
			printf("1111111111111111111111111111111111111111111111111\n");
		}
*/
	}

	//printf("Push: 0x%x, len: %d,  start: 0x%x ,end: 0x%x\n", cur, len, cloudinfo[channelid].bufferStart, cloudinfo[channelid].bufferEnd);
	cur->len = len;
	cur->frametype = frametype;
	cur->timeStamp = timeStamp;
	cur->buffer = U8P_CAST(cur) + ALIGN_4(sizeof(prerecord_head_t));
	memcpy(cur->buffer, buffer, len);
	cur->next = (prerecord_head_t *)(U8P_CAST(cur) + dataLen);
	recorde_fifio_intfo[channelid].last = cur;
	pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
	pthread_cond_signal(&recorde_fifio_intfo[channelid].dataReady);
#endif
	return 0;

}
static prerecord_head_t* _mrecord_fifo_pull(int channelid)
{
	prerecord_head_t *pull = NULL;
#ifdef MRECORED_FIFO

	pull = recorde_fifio_intfo[channelid].first;
	if (pull == NULL)
	{
		return NULL;
	}

	if (pull == recorde_fifio_intfo[channelid].last)
	{
		recorde_fifio_intfo[channelid].first = recorde_fifio_intfo[channelid].last = NULL;
	}
	else
	{
		recorde_fifio_intfo[channelid].first = pull->next;
	}

#endif
	return pull;
}
static void *_mrecord_fifowrite_process(void *arg)
{
#ifdef MRECORED_FIFO

	int channelid = (int)arg;
	prerecord_head_t *head = NULL;
	int bufLen = 0;
	unsigned int frametype;
	static BOOL bWaitIframe=0;
	unsigned long long timeStamp;
	int maxLen = 128*1024;//2048 * 1024 / 8;
	U8 *buf = (U8 *)malloc(maxLen);
	if(buf == NULL)
		return NULL;
	pthreadinfo_add((char *)__func__);

	while (1)
	{
		pthread_mutex_lock(&recorde_fifio_intfo[channelid].mutex);
		while (recorde_fifio_intfo[channelid].last == NULL && recorde_fifio_intfo[channelid].shutdown == 0)
		{
			//printf(".................wait for Producer ........\n");
			pthread_cond_wait(&recorde_fifio_intfo[channelid].dataReady, &recorde_fifio_intfo[channelid].mutex);
		}
		/*if (recorde_fifio_intfo[channelid].shutdown != 0)
		{
			pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
			break;
		}*/
		head = _mrecord_fifo_pull(0);
		if (head)
		{
			bufLen = head->len;
			frametype = head->frametype;
			timeStamp = head->timeStamp;
			if(bWaitIframe)
			{
				if(frametype == JV_FRAME_TYPE_I)
				{
					bWaitIframe = FALSE;
				}
				else
				{
					pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
					continue;
				}

			}
			if(bufLen > maxLen)
			{
				U8 *prealloc = (U8 *)realloc(buf, bufLen);
				maxLen = bufLen;
				if(prealloc == NULL)
				{
					free(buf);
					//return NULL;
					pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
					printf("$$$$$$$$$$$$$$$$$$$Error %s %d relloc failed \n ",__func__,__LINE__);
					continue;
					
				}
				else
					buf = prealloc;
			}
			memcpy(buf, head->buffer, bufLen);
			pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
			__mrecord_write(0, buf,bufLen, frametype, timeStamp,&bWaitIframe);

	
			 
		}
		else
		{
			pthread_mutex_unlock(&recorde_fifio_intfo[channelid].mutex);
		}
	}

	free(buf);
	buf = NULL;

#endif
	return NULL;
}
static void _mrecord_fifo_prepare(int channelid)
{
#ifdef MRECORED_FIFO

	if (recorde_fifio_intfo[channelid].bufferStart!= NULL)
	{
		free(recorde_fifio_intfo[channelid].bufferStart );
		recorde_fifio_intfo[channelid].bufferStart = NULL;
	}

	recorde_fifio_intfo[channelid].bufferLen  = 300*1024;//256在1Mb/s下丢帧
	if (recorde_fifio_intfo[channelid].bufferLen == 0)
	{
		printf("@@@@@@@@@@@@@Pre Record not setted\n");
		return ;
	}
	recorde_fifio_intfo[channelid].bufferStart = (unsigned char *)malloc(recorde_fifio_intfo[channelid].bufferLen);
	if (recorde_fifio_intfo[channelid].bufferStart == NULL)
	{
		printf("######################No Enough memory!\n");
		return ;
	}
	recorde_fifio_intfo[channelid].bufferEnd = recorde_fifio_intfo[channelid].bufferStart + recorde_fifio_intfo[channelid].bufferLen;
	//printf("start: 0x%x ,end: 0x%x, len: %d\n", cloudinfo[channelid].bufferStart, cloudinfo[channelid].bufferEnd, cloudinfo[channelid].bufferLen);
	recorde_fifio_intfo[channelid].first = NULL;
	recorde_fifio_intfo[channelid].last = NULL;
	pthread_mutex_init(&recorde_fifio_intfo[channelid].mutex, NULL);
	pthread_cond_init(&recorde_fifio_intfo[channelid].dataReady, NULL);
	recorde_fifio_intfo[channelid].shutdown = 0;
	pthread_create(&recorde_fifio_intfo[channelid].consumerTid, NULL, _mrecord_fifowrite_process, (void *)channelid);

#endif
}

int mrecord_get_recmode(int* nChFrameSec)
{
	RecordMode_e Type = RECORD_MODE_STOP;
	mrecord_attr_t* pAttr = &recordinfo.recorelist[0];
	
	if(pAttr->bEnable)
		Type = RECORD_MODE_NORMAL;		// 手动录像
	else if(pAttr->chFrame_enable)
	{
		Type = RECORD_MODE_CHFRAME;		// 抽帧录像
		if (nChFrameSec)
			*nChFrameSec = pAttr->chFrameSec;
	}
	else if(pAttr->alarm_enable)
		Type = RECORD_MODE_ALARM;		// 报警录像
	else 
	{
	}

	return Type;
}

int mrecord_set_recmode(int Type, int nChFrameSec)
{
	mrecord_attr_t* pAttr = &recordinfo.recorelist[0];
	
	pAttr->bEnable = FALSE;
	pAttr->discon_enable = FALSE;
	pAttr->disconnected = FALSE;
	pAttr->timing_enable = FALSE;
	pAttr->alarm_enable = FALSE;	
	pAttr->chFrame_enable = FALSE;

	switch (Type)
	{
	case RECORD_MODE_NORMAL:
		pAttr->bEnable = TRUE;
		break;
	case RECORD_MODE_ALARM:
		pAttr->alarm_enable = TRUE;
		break;
	case RECORD_MODE_CHFRAME:
		pAttr->chFrame_enable = TRUE;
		pAttr->chFrameSec = nChFrameSec;
		break;
	default:
		break;
	}

	mrecord_flush(0);
	WriteConfigInfo();

	return 0;
}

static int timer_check = -1;

static BOOL __mrecord_disconnect_check(int tid, void *param)
{
	//重启和断电重启，检测无连接时若为断开时录像模式则直接录像
	static BOOL bFirstCK = TRUE;
	BOOL bNetOK = TRUE;

	DEV_ST_NET netSt = mio_get_get_st();
	switch(netSt)
	{
		case DEV_ST_ETH_OK:
		case DEV_ST_WIFI_OK:
		case DEV_ST_WIFI_CONNECTED:
			bNetOK = TRUE;
			break;
		default:
			bNetOK = FALSE;
			break;
	}
	
	//当前是否有连接
	static BOOL bOldConnected = 0;
	BOOL bNewConnected ;
	if (recordinfo.recorelist[0].discon_enable)
	{
		if (!bNetOK || sp_connect_get_cnt(SP_CON_ALL) == 0)
		{
			bNewConnected = FALSE;
		}
		else
		{
			bNewConnected = TRUE;
		}

		if (bOldConnected != bNewConnected)
		{	
			bOldConnected = bNewConnected;
			printf("status changed:  connection: %d\n", bNewConnected);
			recordinfo.recorelist[0].disconnected = !bNewConnected;
			bFirstCK = FALSE;
			mrecord_flush(0);
		}
		else
		{
			if(bFirstCK == TRUE)
			{
				printf("restart here:  connection: %d\n", bNewConnected);
				recordinfo.recorelist[0].disconnected = !bNewConnected;
				bFirstCK = FALSE;
				mrecord_flush(0);
			}
		}
	}
	return TRUE;
}

/**
 *@brief 初始化
 *
 *@return 0
 */
int mrecord_init(void)
{
	int i;

	memset(cloudinfo, 0, sizeof(cloudinfo));
	cloudinfo[0].shutdown = 1;
	memset(&cloudStorage, 0, sizeof(cloudStorage_t));
	pthread_mutex_init(&cloudStorage.mutex, NULL);
	if (hwinfo.bSupportXWCloud == TRUE)
	{
		_mrecord_cloud_prepare(0);
	}

	memset(&recordinfo, 0, sizeof(recordinfo));
	for (i=0;i<MAX_REC_TASK;i++)
	{
		recordinfo.status[i].timer= -1;
		if(0 >= recordinfo.recorelist[i].file_length)
			recordinfo.recorelist[i].file_length = 600;
	}
	record_chn = 0;
#if SD_RECORD_SUPPORT
    JP_InitSDK(1024*4,2);
#endif
	pthread_mutex_init(&recordinfo.mutex, NULL);
    
	timer_check = utl_timer_create("timer_check", 500, (utl_timer_callback_t)__mrecord_disconnect_check, NULL);

	pthread_t rec_timer_id;
	pthread_create(&rec_timer_id, NULL, _mrecord_timing_thread, NULL);
	pthread_detach(rec_timer_id);

#ifdef MRECORED_FIFO
	_mrecord_fifo_prepare(0);
#endif
	
	return 0;
}

/**
 *@brief 结束
 *
 *@return 0
 */
int mrecord_deinit(void)
{
 #if SD_RECORD_SUPPORT
	int i;
	for (i=0;i<MAX_REC_TASK;i++)
	{
		_mrecord_stop(i);
		if (recordinfo.status[i].timer != -1)
		{
			utl_timer_destroy(recordinfo.status[i].timer);
			recordinfo.status[i].timer = -1;
		}
	}
    JP_ReleaseSDK();
	pthread_mutex_destroy(&recordinfo.mutex);
#endif
	return 0;
}

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param attr 属性指针
 *
 *@return 0 或者是错误号
 */
int mrecord_set_param(int channelid, mrecord_attr_t *attr)
{
	attr->timing_start = VALIDVALUE(attr->timing_start, 0, 24*3600);
	attr->timing_stop = VALIDVALUE(attr->timing_stop, 0, 24*3600);
	attr->alarm_duration = VALIDVALUE(attr->alarm_duration, 0, 1000);
	attr->alarm_pre_record = VALIDVALUE(attr->alarm_pre_record, 0, 10);

	memcpy(&recordinfo.recorelist[channelid], attr, sizeof(mrecord_attr_t));

	return 0;
}

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param attr 属性指针
 *
 *@return 0 或者是错误号
 */
int mrecord_get_param(int channelid, mrecord_attr_t *attr)
{
	memcpy(attr, &recordinfo.recorelist[channelid], sizeof(mrecord_attr_t));

	return 0;
}

void MP4_GetIndexFile(char *strFile, char *strIndex)
{
	int len = strlen(strFile);
	strcpy(strIndex, strFile);
	strcpy(&strIndex[len-4], _MP4_INDEX);
}

/**
 *@brief 开始录制
 *@param channelid 通道号
 *
 *@return 0 或者是错误号
 */
static int _mrecord_start(int channelid, RECTYPE euType)
{
#if SD_RECORD_SUPPORT
	int ret = 0;
	int iRet = 0;
	mstream_attr_t stAttr;
    PKG_VIDEO_PARAM pVideoParam;
   	//pthread_mutex_lock(&recordinfo.mutex);
	if(recordinfo.status[channelid].fd > 0)
		return JVERR_ALREADY_OPENED;

	mstream_get_running_param(RECORD_CHN, &stAttr);

	Printf("_mrecord_start need sizeInMB:%d--\n", MIN_MB_SPACE_NEEDED);
	iRet = mstorage_allocate_space(MIN_MB_SPACE_NEEDED);
	if (iRet < 0)
	{
		Printf("Failed allocate enough space for recording...\n");
		ret = -1;
		recordinfo.status[channelid].filenme[0] = '\0';
	}
	else
	{
		//从这里开始正式启动录像
		char acRecPath[128] = {0};
		char acFolder[MAX_PATH]={0};
		char acFilePath[MAX_PATH]={0};
		mstorage_enter_using();
		sprintf(acRecPath, "%s", mstorage_get_cur_recpath(NULL, 0));

		time_t	timep		= time(NULL);
		struct tm* tmDate	=localtime(&timep);
	
		sprintf(acFolder , "%s%.4d%.2d%.2d" ,acRecPath , tmDate->tm_year+1900,tmDate->tm_mon+1,tmDate->tm_mday);
		if(access(acFolder , F_OK))
		{
			if (0 != mkdir(acFolder , 0777))
			{
				Printf("StartRecord: mkdir %s failed\n",acFolder);
				mstorage_leave_using();
				ret = -1;
				return ret;
			}
		}

		sprintf(acFilePath , "%s/%c%.2d%.2d%.2d%.2d%s", acFolder, euType, channelid+1,
			tmDate->tm_hour, tmDate->tm_min, tmDate->tm_sec, FILE_FLAG);
        
        pVideoParam.fFrameRate = stAttr.framerate;
        pVideoParam.iFrameWidth = stAttr.width;
        pVideoParam.iFrameHeight = stAttr.height;

		float cur, normal;
		if(jv_sensor_get_b_low_frame(0, &cur, &normal))
		{
			if (stAttr.framerate > cur)
				pVideoParam.fFrameRate = cur;
		}
        
       // printf("fFrameRate:%d,width:%d,height:%d\n", pVideoParam.fFrameRate,pVideoParam.iFrameWidth,pVideoParam.iFrameHeight);

        memset(&pVideoParam.avcC, 0, sizeof(MP4_AVCC));	
        char strIndex[128];
        MP4_GetIndexFile(acFilePath,strIndex);
        jv_audio_attr_t attr;
        jv_ai_get_attr(0, &attr);
		int iACodec =  attr.encType == JV_AUDIO_ENC_G711_A ? JVS_ACODEC_ALAW : JVS_ACODEC_ULAW;
		int iAVCodec = stAttr.vencType == JV_PT_H265 ? JVS_VCODEC_HEVC : JVS_VCODEC_H264;  
		recordinfo.status[channelid].fd = JP_OpenPackage(&pVideoParam, TRUE, TRUE, acFilePath, strIndex,
				iAVCodec | iACodec,	0);
		if(recordinfo.status[channelid].fd > 0)
		{
			mlog_write("Record Start: Success");
			recordinfo.status[channelid].type = euType;
			recordinfo.status[channelid].byteWrited = 0;
			recordinfo.status[channelid].startSecond = utl_get_sec();
			sprintf(recordinfo.status[channelid].filenme, "%s", acFilePath);
			Printf("Record start success: file=%s, beginTime=%u\n", acFilePath, recordinfo.status[channelid].startSecond);
		}
		else
		{
			Printf("StartRecord: open %s failed\n", acFilePath);
			mstorage_leave_using();
			ret = -1;
		}
	}
	//pthread_mutex_unlock(&recordinfo.mutex);
#endif
	return 0;
}

static void* _mrecord_notify_rec_req_finsihed(void* p)
{
	FuncRecFinish	pNotify = (FuncRecFinish)p;
	pNotify();

	return NULL;
}

static int _mrecord_alarm_start(int channelid, RECTYPE euType)
{
#if SD_RECORD_SUPPORT
	if(cloudStorage.hlsHandle > 0)
	{
		printf("==============cloudStorage.hlsHandle>0\n");
		return 0;
	}
 
	time_t timep = cloudStorage.alarmTime;
	struct tm tmDate;
	char filename[MAX_PATH]={0};

	localtime_r(&timep, &tmDate);
#ifdef OBSS_CLOUDSTORAGE

	ipcinfo_t ipcinfo;
	char ystNo[64] = {0};
	ipcinfo_get_param(&ipcinfo);
	jv_ystNum_parse(ystNo, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);

	sprintf(filename, "%s/%s/%.4d%.2d%.2d/%c%.2d%.2d%.2d%.2d",obss_info.days,ystNo,tmDate.tm_year + 1900,tmDate.tm_mon + 1,tmDate.tm_mday,euType, 
		channelid+1, tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec);
#else
	sprintf(filename, "%c%.2d%.2d%.2d%.2d", euType, 
		channelid+1, tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec);
#endif
	mstream_attr_t stAttr;
	float cur, normal;
	mstream_get_running_param(RECORD_CLOUD_CHN, &stAttr);
	if(jv_sensor_get_b_low_frame(0, &cur, &normal))
	{
		if(stAttr.framerate > cur)
			stAttr.framerate = cur;
	}

#endif

#ifdef OBSS_CLOUDSTORAGE

	int maxFrameCnt = CLOUD_ALARM_REC_LEN * stAttr.framerate;
	int maxTsFileSize = 250000;
	printf("jhlsup open maxTsFileSize:%d,maxFrameCnt:%d\n",maxTsFileSize,maxFrameCnt);
	cloudStorage.hlsHandle = jhlsup_open(tmDate.tm_year+1900, tmDate.tm_mon+1, tmDate.tm_mday, filename, maxFrameCnt, maxTsFileSize, CLOUD_ALARM_TS_LEN);
	if(cloudStorage.hlsHandle != NULL)
	{
		jv_audio_attr_t attr;
		JHLSAudioStreamType_e audioType;
		jv_ai_get_attr(0, &attr);
		switch(attr.encType)
		{
			case JV_AUDIO_ENC_G711_A:
				audioType = JHLS_AUDIO_STREAMTYPE_G711A;
				break;
			case JV_AUDIO_ENC_G711_U:
				audioType = JHLS_AUDIO_STREAMTYPE_G711U;
				break;
			default:
				audioType = JHLS_AUDIO_STREAMTYPE_MAX;
				break;
		}
		JHLSVideoStreamType_e videoType;
		switch(stAttr.vencType)
		{
			case JV_PT_H264:
				videoType = JHLS_VIDEO_STREAMTYPE_H264;
				break;
			case JV_PT_H265:
				videoType = JHLS_VIDEO_STREAMTYPE_H265;
				break;
			default:
				videoType = JHLS_VIDEO_STREAMTYPE_H264;
				break;
		}
		printf("videoType:%d audioType:%d\n", videoType, audioType);
		jhlsup_set_param(cloudStorage.hlsHandle, videoType, audioType);
		sprintf(cloudStorage.hlsName, "%s", filename);
		printf("jhlsup_open ok: filename=%s, time=%u\n", filename, utl_get_sec());
	}
#endif

	return 0;
}

/**
 *@brief 结束录制
 *@param channelid 通道号
 *
 *@return 0 或者是错误号
 */
static int _mrecord_stop(int channelid)
{
#if SD_RECORD_SUPPORT
	if (recordinfo.status[channelid].fd > 0)
	{
		//同步会导致延时
//		fsync(recordinfo.status[channelid].fd);
		switch (recordinfo.status[channelid].type)
		{
		case REC_ONE_MIN:
		case REC_PUSH:
		// case REC_ALARM:
		// case REC_MOTION:
			if (recordinfo.status[channelid].ReqParam.pCallback)
			{
				pthread_create_detached(NULL, NULL, _mrecord_notify_rec_req_finsihed, (void*)recordinfo.status[channelid].ReqParam.pCallback);
			}
			memset(&recordinfo.status[channelid].ReqParam, 0, sizeof(recordinfo.status[channelid].ReqParam));
			break;
		default:
			break;
		}
		JP_ClosePackage(recordinfo.status[channelid].fd);
		recordinfo.status[channelid].fd = 0;
		recordinfo.status[channelid].type = REC_STOP;
		mlog_write("Record Stop: Success");
		mstorage_leave_using();
		Printf("Record stop success: file=%s, endTime=%u\n", 
			recordinfo.status[channelid].filenme, utl_get_sec());
		recordinfo.status[channelid].filenme[0] = '\0';
	}
#endif
	return 0;
}

/**
 *@brief 检查定时录制功能距离录制所需的秒数
 *@param channelid 通道号
 *
 *@retval 0 现在录制
 *@retval 1 定时录制
 *
 */
static int _mrecord_timing_second2start(int channelid, int *sec2start, int *sec2end)
{
	struct tm *ptm ;
	time_t tt, cur,start,end;
	tt = time(NULL);
	ptm = localtime(&tt);
	cur = ptm->tm_hour*3600 + ptm->tm_min*60 + ptm->tm_sec;
	start = recordinfo.recorelist[channelid].timing_start;
	end = recordinfo.recorelist[channelid].timing_stop;

	if (isInTimeRange(cur,start,end))
	{
		if (sec2start) *sec2start = 0;
		if (sec2end) *sec2end = (end > cur) ? end - cur : 3600 * 24 + end - cur;
		return 0;
	}
	if (sec2start) *sec2start = start > cur ? start - cur : 3600 * 24 + start - end;
	if (sec2end) *sec2end = (end > cur) ? end - cur : 3600 * 24 + end - cur;
	return 1;
}

/**
 *@brief 强制停止录像
 *
 *	功能主要针对当分辨率发生变化时，重启录像功能
 *@param channelid 通道号
 *
 *@return 0 或者是错误号
 */
int mrecord_stop(int channelid)
{
	int ret;
	pthread_mutex_lock(&recordinfo.mutex);
	ret = _mrecord_stop(channelid);
	pthread_mutex_unlock(&recordinfo.mutex);
	return ret;
}

static int mrecord_timing_cnt = 0;
static void* _mrecord_timing_thread(void* p)
{
	int channelid = 0;
	time_t tim;
	struct tm tm;
	
	pthreadinfo_add((char *)__func__);
	while (1)
	{
		if(mrecord_timing_cnt > 0)
		{
			mrecord_timing_cnt--;
			if(mrecord_timing_cnt <= 0)
			{
				switch (recordinfo.status[channelid].type)
				{
				case REC_ONE_MIN:
				case REC_PUSH:
					// case REC_ALARM:
					// case REC_MOTION:
					printf("%s, Record request finished, type: %c\n", __func__, recordinfo.status[channelid].type);
					recordinfo.status[channelid].ReqParam.ReqType = RECORD_REQ_NONE;
					break;
				default:
					break;
				}
				mrecord_flush(0);
			}
		}

		tim = time(NULL);
		localtime_r(&tim, &tm);
		if(tm.tm_hour == 0 && tm.tm_min == 0 && tm.tm_sec == 0)
		{
			mrecord_stop(channelid);
			mrecord_flush(channelid);
		}
		sleep(1);
	}

	return 0;
}




/**
 *@brief 刷新某通道的设置，使之生效。
 *
 *	功能主要针对定时器
 *@param channelid 通道号
 *
 *@return 0 或者是错误号
 */
int mrecord_flush(int channelid)
{
	RECTYPE recType;
	int timing_second = 0;
	int sec2start,sec2end;//定时录制时的开始及结束录制的时间
	unsigned int now;

	pthread_mutex_lock(&recordinfo.mutex);

	//决定要以哪种录制方式执行
	if (recordinfo.recorelist[channelid].bEnable)
		recType = REC_NORMAL;
	else if (recordinfo.recorelist[channelid].disconnected)
		recType = REC_DISCON;
	else if (recordinfo.recorelist[channelid].detecting 
		&& recordinfo.status[channelid].alarmType == ALARM_TYPE_MOTION
		&& recordinfo.recorelist[channelid].alarm_enable)
		recType = REC_MOTION;
	else if (recordinfo.recorelist[channelid].alarming 
		&& recordinfo.status[channelid].alarmType == ALARM_TYPE_ALARM
		&& recordinfo.recorelist[channelid].alarm_enable)
		recType = REC_ALARM;
	else if (recordinfo.recorelist[channelid].ivping
		&& recordinfo.status[channelid].alarmType == ALARM_TYPE_IVP
		&& recordinfo.recorelist[channelid].alarm_enable)
		recType = REC_IVP;
	else if(recordinfo.recorelist[channelid].chFrame_enable)
		recType = REC_CHFRAME;
	else 
		recType = REC_STOP;
	printf("##########alarming=%d recType=%d\n",recordinfo.recorelist[channelid].alarming,recType);
	if (recType != REC_NORMAL)
	{
		if (recordinfo.recorelist[channelid].timing_enable)
		{
			if (0 == _mrecord_timing_second2start(channelid, &sec2start, &sec2end))
			{
				//录制已开始，定时器负责结束
				recType = REC_TIME;
				timing_second = sec2end;
			}
			else
			{
				//录制未开始，定时器负责录制
				timing_second = sec2start;
			}
		}
	}

	switch (recordinfo.status[channelid].ReqParam.ReqType)
	{
	case RECORD_REQ_ONEMIN:
		recType = REC_ONE_MIN;
		timing_second = recordinfo.status[channelid].ReqParam.nDuration;
		break;
	case RECORD_REQ_PUSH:
		recType = REC_PUSH;
		timing_second = recordinfo.status[channelid].ReqParam.nDuration;
		break;
	case RECORD_REQ_ALARM:
		break;
	default:
		break;
	}

	if (recType == REC_ALARM || recType == REC_MOTION || recType == REC_IVP)
	{
		unsigned int duration = recordinfo.recorelist[channelid].alarm_pre_record 
			+ recordinfo.recorelist[channelid].alarm_duration;
		unsigned int endtime = recordinfo.status[channelid].alarmStartSecond + duration;
		now = utl_get_sec();
		if (now >= endtime)
		{
			//停止录制
			recType = REC_STOP;
			recordinfo.status[channelid].alarmStartSecond = 0;
			recordinfo.status[channelid].alarmType = ALARM_TYPE_NONE;
		}
		else
		{
			//选个较短的刷新时间，用于结束录制
			timing_second = (timing_second != 0) &&timing_second < endtime - now ? timing_second : endtime - now;
		}
	}

	if (recType == REC_STOP)
	{
		_mrecord_stop(channelid);
	}
	else
	{
		int bRestart = 0;
		switch(recordinfo.status[channelid].type)
		{
		default:
		case REC_STOP:
			bRestart = 1;
			break;
		case REC_ONE_MIN:
			bRestart = 1;
			break;
		case REC_NORMAL:
			if (recType == REC_ONE_MIN || recType == REC_PUSH || recType == REC_CHFRAME)
				bRestart = 1;
			break;
		case REC_CHFRAME:
			bRestart = 1;
			break;
		case REC_TIME:
			//REC_NROMAL的优先级比较高
			if (recType == REC_NORMAL || recType == REC_CHFRAME || recType == REC_ONE_MIN || recType == REC_PUSH)
				bRestart = 1;
			break;
		case REC_MOTION:
		case REC_ALARM:
		case REC_IVP:
		case REC_DISCON:
			//还是优先级问题
			if (recType == REC_NORMAL || recType == REC_TIME || recType == REC_CHFRAME || recType == REC_ONE_MIN || recType == REC_PUSH)
				bRestart = 1;
			break;
		}
		if (bRestart)
		{
			_mrecord_stop(channelid);
			Printf("mrecord start type %c\n", recType);
			_mrecord_start(channelid, recType);
		}
	}

	if (timing_second != 0)
	{
		Printf("timint to flush: %d\n", timing_second);
		mrecord_timing_cnt = timing_second;
	}
	
	pthread_mutex_unlock(&recordinfo.mutex);
	return 0;
}

#ifdef MRECORED_FIFO
/**
 *@brief 保存一帧数据
 *@param channelid 通道号
 *@param buffer
 *@param len
 *@param frametype
 *@param info 要保存的码流信息
 *
 */	
int __mrecord_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timestamp, BOOL *bWaitIframe)
{
#if SD_RECORD_SUPPORT
	//只有I帧时
	if (frametype == JV_FRAME_TYPE_I)
	{
		U32 now = utl_get_sec();
		U32 RecSec = 0;

		if(recordinfo.status[channelid].fd > 0)
		{
			// 保险处理
			if (recordinfo.recorelist[channelid].file_length <= 0)
			{
				recordinfo.recorelist[channelid].file_length = 600;
			}
			RecSec = recordinfo.recorelist[channelid].file_length;
			if (recordinfo.status[channelid].type == REC_CHFRAME)
			{
				/* 如果设置时间小于4，则默认为4，防止后面除0异常 */
				if(recordinfo.recorelist[channelid].chFrameSec < 4)
				{
					recordinfo.recorelist[channelid].chFrameSec = 4;
				}
				RecSec = recordinfo.recorelist[channelid].chFrameSec * 25 * 60;
			}
			if(now - recordinfo.status[channelid].startSecond >= RecSec)
			{
				Printf("File segment occured\n");
				mrecord_stop(channelid);
				mrecord_flush(channelid);
			}
		}
	}
	pthread_mutex_lock(&recordinfo.mutex);
	
	if(recordinfo.status[channelid].type == REC_CHFRAME)
	{
		static int count = 0;

		if (frametype != JV_FRAME_TYPE_I)
		{
			goto END_MUTEX;
		}
		if((count++) % (recordinfo.recorelist[channelid].chFrameSec / 4))
		{
			goto END_MUTEX;
		}
	}

	if(recordinfo.status[channelid].fd > 0)
	{
        AV_PACKET pAVPkt;
        pAVPkt.iSize=len;
        pAVPkt.pData=buffer;
        pAVPkt.iType=JVS_PKG_VIDEO;
        if(frametype == JV_FRAME_TYPE_A)//写入音频
        	pAVPkt.iType=JVS_PKG_AUDIO;
		pAVPkt.iPts  = (recordinfo.status[channelid].type == REC_CHFRAME) ? 0 : timestamp;
		pAVPkt.iDts = 0;

		// 确保第一帧为I帧
		if ((0 == recordinfo.status[channelid].byteWrited) && (frametype != JV_FRAME_TYPE_I))
		{
			mstream_request_idr(channelid);
			*bWaitIframe = TRUE;
			goto END_MUTEX;
		}
		
		int ret = JP_PackageOneFrame(recordinfo.status[channelid].fd, &pAVPkt);
        if(ret != 1)
        {
            printf("JP_PackageOneFrame ERR\n");
        }
        //fsync(recordinfo.status[channelid].fd);
		recordinfo.status[channelid].byteWrited += len;
		if(frametype != JV_FRAME_TYPE_A)
		{
			recordinfo.status[channelid].stHeader.nTotalFrames++;
		}
	}
END_MUTEX:		
	pthread_mutex_unlock(&recordinfo.mutex);
	
#endif
	return 0;
}

/**
 *@brief 保存一帧数据
 *@param channelid 通道号
 *@param buffer
 *@param len
 *@param frametype
 *@param info 要保存的码流信息
 *
 */
int mrecord_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timestamp)
{
	static BOOL bWaitIframe = 0;
	int ret =0;
#if SD_RECORD_SUPPORT

#if ENABLE_PRE_RECORD
	// 数据一直进行缓存，防止出现跳跃
	_mrecord_pre_push(channelid, buffer, len, frametype, timestamp);

	if(recordinfo.status[channelid].type == REC_MOTION ||recordinfo.status[channelid].type == REC_ALARM||recordinfo.status[channelid].type == REC_IVP)
	{
		prerecord_head_t* tmp_record_info = NULL;
		tmp_record_info = _mrecord_pre_pull(channelid);

		if(NULL != tmp_record_info)
		{
			buffer = tmp_record_info->buffer;
			len    = tmp_record_info->len;
			frametype = tmp_record_info->frametype;
			timestamp = tmp_record_info->timeStamp;
		}
	}
#endif
	if(bWaitIframe )
	{
		if( frametype == JV_FRAME_TYPE_I)
		{
			bWaitIframe =FALSE;
		}
		else
		{
			return 0;
		}
	}
	 ret=_mrecord_fifo_push( channelid, buffer,  len,  frametype, timestamp);
	if(ret < 0)
	{
		bWaitIframe = TRUE;
		printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ERROR %s %d  :  Push Fifo  failed,Will wait I frame \n",__func__,__LINE__);
	}
	
#endif
	return ret;
}



#else

/**
 *@brief 保存一帧数据
 *@param channelid 通道号
 *@param buffer
 *@param len
 *@param frametype
 *@param info 要保存的码流信息
 *
 */
int mrecord_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timestamp)
{
#if SD_RECORD_SUPPORT

#if ENABLE_PRE_RECORD
	// 数据一直进行缓存，防止出现跳跃
	_mrecord_pre_push(channelid, buffer, len, frametype, timestamp);

	if(recordinfo.status[channelid].type == REC_MOTION ||recordinfo.status[channelid].type == REC_ALARM||recordinfo.status[channelid].type == REC_IVP)
	{
		prerecord_head_t* tmp_record_info = NULL;
		tmp_record_info = _mrecord_pre_pull(channelid);

		if(NULL != tmp_record_info)
		{
			buffer = tmp_record_info->buffer;
			len    = tmp_record_info->len;
			frametype = tmp_record_info->frametype;
			timestamp = tmp_record_info->timeStamp;
		}
	}
#endif
	//只有I帧时
	if (frametype == JV_FRAME_TYPE_I)
	{
		U32 now = utl_get_sec();
		U32 RecSec = 0;

		if(recordinfo.status[channelid].fd > 0)
		{
			// 保险处理
			if (recordinfo.recorelist[channelid].file_length <= 0)
			{
				recordinfo.recorelist[channelid].file_length = 600;
			}
			RecSec = recordinfo.recorelist[channelid].file_length;
			if (recordinfo.status[channelid].type == REC_CHFRAME)
			{
				/* 如果设置时间小于4，则默认为4，防止后面除0异常 */
				if(recordinfo.recorelist[channelid].chFrameSec < 4)
				{
					recordinfo.recorelist[channelid].chFrameSec = 4;
				}
				RecSec = recordinfo.recorelist[channelid].chFrameSec * 25 * 60;
			}
			if(now - recordinfo.status[channelid].startSecond >= RecSec)
			{
				Printf("File segment occured\n");
				mrecord_stop(channelid);
				mrecord_flush(channelid);
			}
		}
	}
	pthread_mutex_lock(&recordinfo.mutex);
	
	if(recordinfo.status[channelid].type == REC_CHFRAME)
	{
		static int count = 0;

		if (frametype != JV_FRAME_TYPE_I)
		{
			goto END_MUTEX;
		}
		if((count++) % (recordinfo.recorelist[channelid].chFrameSec / 4))
		{
			goto END_MUTEX;
		}
	}

	if(recordinfo.status[channelid].fd > 0)
	{
        AV_PACKET pAVPkt;
        pAVPkt.iSize=len;
        pAVPkt.pData=buffer;
        pAVPkt.iType=JVS_PKG_VIDEO;
        if(frametype == JV_FRAME_TYPE_A)//写入音频
        	pAVPkt.iType=JVS_PKG_AUDIO;
		pAVPkt.iPts  = (recordinfo.status[channelid].type == REC_CHFRAME) ? 0 : timestamp;
		pAVPkt.iDts = 0;

		// 确保第一帧为I帧
		if ((0 == recordinfo.status[channelid].byteWrited) && (frametype != JV_FRAME_TYPE_I))
		{
			mstream_request_idr(channelid);
			goto END_MUTEX;
		}
		
		int ret = JP_PackageOneFrame(recordinfo.status[channelid].fd, &pAVPkt);
        if(ret != 1)
        {
            printf("JP_PackageOneFrame ERR\n");
        }
        //fsync(recordinfo.status[channelid].fd);
		recordinfo.status[channelid].byteWrited += len;
		if(frametype != JV_FRAME_TYPE_A)
		{
			recordinfo.status[channelid].stHeader.nTotalFrames++;
		}
	}
END_MUTEX:		
	pthread_mutex_unlock(&recordinfo.mutex);
	
#endif
	return 0;
}
#endif


/**
 *@brief 发生外部报警或者是移动检测报警
 *@param channelid 通道号
 *@param type 报警类型
 *@return 0 或者错误号
 */
int mrecord_alarming(int channelid, alarm_type_e type, void *param)
{
	if(param != NULL)
	{
		pthread_mutex_lock(&cloudStorage.mutex);
		JV_ALARM *alarm = (JV_ALARM *)param;
		cloudStorage.type = (alarm->alarmType==ALARM_DOOR)?1:0;
		if(cloudStorage.hlsHandle <= 0)
		{
			RECTYPE recType;
			if(type == ALARM_TYPE_MOTION)
				recType = REC_MOTION;
			else if(type == ALARM_TYPE_IVP)
				recType = REC_IVP;
			else
				recType = REC_ALARM;
			cloudStorage.alarmTime = alarm->time;
			cloudStorage.startSecond = utl_get_sec();
			_mrecord_alarm_start(channelid, recType);
			mstream_request_idr(RECORD_CLOUD_CHN);
		}
		pthread_mutex_unlock(&cloudStorage.mutex);
	}

	if (mstorage_allocate_space(MIN_MB_SPACE_NEEDED) < 0)
		return 0;

	if(!recordinfo.recorelist[channelid].alarm_enable)
		return 0;

	pthread_mutex_lock(&recordinfo.mutex);
	recordinfo.status[channelid].alarmType = type;
	recordinfo.status[channelid].alarmStartSecond = utl_get_sec();
	pthread_mutex_unlock(&recordinfo.mutex);
	mrecord_flush(channelid);
	return 0;
}

/**
 *@brief 获取报警录像文件的文件名称
 *@param chFileName 录像文件名
 *@return 0 或者错误号return -1 没有SD卡
 */
int mrecord_alarm_get_attachfile(RECTYPE recType, void *alarmInfo)
{
	if(alarmInfo == NULL)
		return -1;

	JV_ALARM *alarm = (JV_ALARM *)alarmInfo;
	sprintf(alarm->VideoName, "%s", recordinfo.status[0].filenme);
	if(alarm->pushType == ALARM_PUSH_YST_CLOUD)
	{
		sprintf(alarm->cloudVideoName, "%s.m3u8", cloudStorage.hlsName);
	}

	BOOL bFoundSDCard = TRUE;
	int ret = mstorage_allocate_space(MIN_MB_SPACE_NEEDED);
	if (ret < 0)
	{
		Printf("Failed allocate enough space for recording...\n");
		bFoundSDCard = FALSE;
	}

	char acRecPath[128] = {0};
	char acFolder[MAX_PATH]={0};
	struct tm tmDate;
	localtime_r(&alarm->time, &tmDate);

	if(bFoundSDCard)
	{
		sprintf(acRecPath, "%s", mstorage_get_cur_recpath(NULL, 0));
		sprintf(acFolder , "%s%.4d%.2d%.2d" ,acRecPath , tmDate.tm_year+1900,tmDate.tm_mon+1,tmDate.tm_mday);
		if(access(acFolder , F_OK))
		{
			if (0 != mkdir(acFolder , 0777))
			{
				Printf("mkdir %s failed\n",acFolder);
				return -1;
			}
		}

		sprintf(alarm->PicName , "%s/%c%.2d%.2d%.2d%.2d.jpg", acFolder, recType, 1, 
			tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec);
		if(alarm->pushType == ALARM_PUSH_YST_CLOUD)
			strcpy(alarm->cloudPicName, alarm->PicName);
	}
	else
	{
		alarm->PicName[0] = '\0';
		if(alarm->pushType == ALARM_PUSH_YST_CLOUD)
		{
			sprintf(acFolder, "/tmp");
			sprintf(alarm->cloudPicName , "%s/%c%.2d%.2d%.2d%.2d.jpg", acFolder, recType, 1, 
				tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec);
		}
	}

	return 0;
}

const char* mrecord_get_now_recfile()
{
	return recordinfo.status[0].filenme;
}

int mrecord_request_start_record(int channelid, const REC_REQ_PARAM* pReqParam)
{
    STORAGE stStorage;
	FuncRecFinish	pFunc = NULL;

	if(mstorage_get_info(&stStorage) != 0)
	{
		printf("%s, failed by no valid storage found!\n", __func__);
		return -1;
	}
	pthread_mutex_lock(&recordinfo.mutex);
	if (recordinfo.status[channelid].ReqParam.ReqType > pReqParam->ReqType)
	{
		pthread_mutex_unlock(&recordinfo.mutex);
		printf("%s, Ignore requst, new: %d, now: %d\n", __func__, 
					pReqParam->ReqType, recordinfo.status[channelid].ReqParam.ReqType);
		return -1;
	}

	// 调用flush前，先不覆盖Callback，确保stop时，Callback是正确的
	pFunc = recordinfo.status[channelid].ReqParam.pCallback;
	recordinfo.status[channelid].ReqParam = *pReqParam;
	recordinfo.status[channelid].ReqParam.pCallback = pFunc;
	pthread_mutex_unlock(&recordinfo.mutex);

	mrecord_flush(channelid);
	recordinfo.status[channelid].ReqParam = *pReqParam;

	return 0;
}

int mrecord_request_stop_record(int channelid, REC_REQ Request)
{
	FuncRecFinish	pFunc = NULL;

	pthread_mutex_lock(&recordinfo.mutex);
	if (recordinfo.status[channelid].ReqParam.ReqType != Request)
	{
		pthread_mutex_unlock(&recordinfo.mutex);
		printf("%s, Ignore requst, new: %d, now: %d\n", __func__, 
					Request, recordinfo.status[channelid].ReqParam.ReqType);
		return -1;
	}

	pFunc = recordinfo.status[channelid].ReqParam.pCallback;
	memset(&recordinfo.status[channelid].ReqParam, 0, sizeof(recordinfo.status[channelid].ReqParam));
	recordinfo.status[channelid].ReqParam.pCallback = pFunc;
	pthread_mutex_unlock(&recordinfo.mutex);

	mrecord_flush(channelid);
	memset(&recordinfo.status[channelid].ReqParam, 0, sizeof(recordinfo.status[channelid].ReqParam));

	return 0;
}

static int _mrecord_item_cmp(const mrecord_item_t* a, const mrecord_item_t* b)
{
	if (a->date > b->date)
		return 1;
	if (a->date < b->date)
		return -1;
	if (a->stime > b->stime)
		return 1;
	if (a->stime < b->stime)
		return -1;
	return 0;
}

int mrecord_search_file(int startdate, int starttime, int enddate, int endtime, mrecord_item_t* pResult, int maxcnt)
{
	DIR* pDir = NULL;
	struct dirent *pDirent	= NULL;
	U8	VideoType;		// 搜索到的录像文件类型
	U32 VideoChn = 0;
	U32 VideoStartTime = 0;
	S8 strPath[256] = {0};
	S8 strFilePath[256]  = {0};
	STORAGE stStorage;
	U16 nSearchCnt = 0;
	mrecord_item_t* pItem = pResult;
	struct stat statbuf;
	struct tm tmEnd;
	int i, j;
	int hour, min, sec;

#define CHECK_DATE(d)		(((d) >= 19700101) && ((d) <= 21000000))
#define CHECK_TIME(t)		(((t) >= 000000) && ((t) <= 235959))

	printf("startdate:%d starttime:%d enddate:%d endtime:%d maxcnt:%d\n",
			startdate, starttime, enddate, endtime, maxcnt);
	if (!CHECK_DATE(startdate)
		|| !CHECK_DATE(enddate)
		|| !CHECK_TIME(starttime)
		|| !CHECK_TIME(endtime))
	{
		printf("%s, Invalid start or end time, start: %d-%d, end: %d-%d\n", __func__, startdate, starttime, enddate, endtime);
		return -1;
	}
	if ((NULL == pResult) || (maxcnt <= 0))
	{
		printf("%s, Invalid argument pResult: %p, maxcnt: %d\n", __func__, pResult, maxcnt);
		return -1;
	}

	mstorage_get_info(&stStorage);

	for (i = 0; (i < 1) && (nSearchCnt < maxcnt); ++i)
	{
		for (j = startdate; (j <= enddate) && (nSearchCnt < maxcnt); ++j)
		{
			// 获取完整路径，格式: ./rec/00/20151109
			snprintf(strPath, sizeof(strPath), "%s/%08d", mstorage_get_partpath(i, 0, NULL, 0), j);
			printf("%s, strPath: %s\n", __func__, strPath);

			if(access(strPath, F_OK))
			{
				printf("%s, Failed access dir: %s\n", __func__, strPath);
				continue;// 如果文件夹不存在则继续搜索下一日期
			}
			// 打开目录
			pDir = opendir(strPath);
			if (pDir == NULL)
			{
				printf("%s, Failed open dir: %s\n", __func__, strPath);
				continue;
			}

			while ((pDirent = readdir(pDir)) != NULL)
			{
				// 防止出现越界
				if (strlen(pDirent->d_name) < strlen(FILE_FLAG))
				{
					continue;
				}

				if (0 == strcmp(FILE_FLAG, pDirent->d_name + strlen(pDirent->d_name) - strlen(FILE_FLAG)))
				{
					sscanf(pDirent->d_name, "%c%2d%6d", &VideoType, &VideoChn, &VideoStartTime);
					sprintf(strFilePath, "%s/%s", strPath, pDirent->d_name);
					stat(strFilePath, &statbuf);
					localtime_r(&statbuf.st_mtime, &tmEnd);

					if (((j == startdate) && (VideoStartTime >= starttime)) 
						|| ((j == enddate) && (VideoStartTime <= endtime))
						|| ((j > startdate) && (j < enddate)))
					{
						pItem->date = j;
						pItem->stime = VideoStartTime;
						pItem->fileTime = VideoStartTime;
						if(VideoType == REC_MOTION ||VideoType == REC_IVP )
						{
							/* 移动侦测报警录像存在预录 */
							pItem->stime = utl_time_modify(UTL_TIME_HHMMSS, VideoStartTime,
									-1 * recordinfo.recorelist[0].alarm_pre_record);
						}
						pItem->etime = tmEnd.tm_hour * 10000 + tmEnd.tm_min * 100 + tmEnd.tm_sec;

						// 跨天录像特殊处理
						// 改系统时间也会引起跨天，这里只处理跨到第二天或之后的情况
						if ((tmEnd.tm_year + 1900) * 10000 + (tmEnd.tm_mon + 1) * 100 + tmEnd.tm_mday > j)
						{
							// 0000xx -> 2400xx
							pItem->etime += 240000;
						}
						pItem->type = VideoType;
						pItem->part = i;

						++pItem;
						++nSearchCnt;
						if (nSearchCnt >= maxcnt)
						{
							printf("%s, Search result reaches limit: %d\n", __func__, nSearchCnt);
							break;
						}
					}
				}
			}
			closedir(pDir);
		}
	}


	// 排序
	if (nSearchCnt > 1)
	{
		utl_qsort(pResult, nSearchCnt, sizeof(*pResult), (FuncCmp)_mrecord_item_cmp);
	}

	return nSearchCnt;
}

const char* mrecord_get_filename(const mrecord_item_t* pResult, char* name, int len)
{
	int hour, min, sec;
	int VideoStartTime = pResult->fileTime;

	snprintf(name, len, "%s%08d/%c%02d%06d.mp4", 
						mstorage_get_partpath(0, pResult->part, NULL, 0), 
						pResult->date, pResult->type, 1, VideoStartTime);
	return name;
}


