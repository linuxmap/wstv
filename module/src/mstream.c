#include <jv_common.h>

#include "mipcinfo.h"
#include "mrecord.h"
#include "mosd.h"
#include "msnapshot.h"
#include "mdetect.h"
#include "mstream.h"
#include "mwdt.h"
#include <jv_ai.h>
#include <jv_ao.h>
#include <utl_cmd.h>
#include "utl_ifconfig.h"
#include "utl_timer.h"
#include "mlog.h"
#include <jv_sensor.h>
#include <mprivacy.h>
#include "maccount.h"
#include <sp_connect.h>
#include <msensor.h>
#include <maudio.h>
#include "SYSFuncs.h"

#ifdef LIVE555_SUPPORT
#include <jvlive.h>
#endif
#ifdef GB28181_SUPPORT
#include <gb28181.h>
#endif
#ifdef ZRTSP_SUPPORT
#include <libzrtsp.h>
#endif
#include "m_rtmp.h"
#include "mivp.h"
#include "JvServer.h"
#include "mfirmup.h"


#define MAX_CALLBACK_CNT 5

typedef struct _RESTART_MODULE
{
	BOOL		stream;
	BOOL        rec;
	BOOL		md;
	BOOL		osd;
	BOOL		ive;
	BOOL		privacy;
} RESTART_MODULE_S;

typedef struct
{
	mstream_attr_t streamlist[MAX_STREAM];
	BOOL		needRestart[MAX_STREAM];///< 标记是否在#mstream_flush时是否需要重新播放
	BOOL        changMainRes;		//判断是否是主码流分辨率修改
	BOOL		running[MAX_STREAM];
	mstream_transmit_callback_t callback[MAX_CALLBACK_CNT];
	mstream_roi_t mroi;
	RESTART_MODULE_S stMNeedRestart[MAX_STREAM];
} stream_status_t;

static int bVencTypeChanged = 0;

static jv_thread_group_t group;
static stream_status_t status;
static int list_stream = 0;
static int only_stream = 0;

#define SIZE_SKIPFRAME		12				//skip帧长度,skip帧不应该存在,而应该在播放时使用帧率控制

static int nMaxFramerate = 0;
//static U8 acSkipFrame[SIZE_SKIPFRAME]={0};
static U32 nFrameCounter[MAX_STREAM]={0};

//补skip帧索引,lck20120814
static BOOL arrSkip[][30]=
{
{	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19,	20,	21,	22,	23,	24,	25,	26,	27,	28,	29,	30},//标记
{	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0}, //30帧
{	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},	//25帧
{	1,	0,	0,	0,	0,	1,	0,	0,	0,	0,	1,	0,	0,	0,	0,	1,	0,	0,	0,	0,	1,	0,	0,	0,	0,	1,	0,	0,	0,	0},	//20帧
{	1,	1,	0,	0,	0,	1,	1,	0,	0,	0,	1,	1,	0,	0,	0,	1,	1,	0,	0,	0,	1,	1,	0,	0,	0,	1,	1,	0,	0,	0},	//15帧
{	1,	1,	1,	0,	0,	1,	1,	1,	0,	0,	1,	1,	1,	0,	0,	1,	1,	1,	0,	0,	1,	1,	1,	0,	0,	1,	1,	1,	0,	0},	//10帧
{	1,	1,	1,	1,	0,	1,	1,	1,	1,	0,	1,	1,	1,	1,	0,	1,	1,	1,	1,	0,	1,	1,	1,	1,	0,	1,	1,	1,	1,	0},	//05帧
};
int mstream_start(int channelid);
int mstream_stop(int channelid);


static int sStreamCnt[MAX_STREAM];
BOOL __check_stream_timer(int tid, void *param)
{
	static int streamDog[MAX_STREAM] = {0};
	int i;

	if(mfirmup_b_updating())
	{
		printf("Now updating... no check stream!\n");
		return 1;
	}

	for (i=0;i<hwinfo.streamCnt;i++)
	{
		//printf("streamcnt[%d]: %d\n", i, sStreamCnt[i]);
		if (sStreamCnt[i])
		{
			sStreamCnt[i] = 0;
			streamDog[i] = 0;
		}
		else
		{
			streamDog[i]++;
			if (streamDog[i] > 10)
			{
				printf("mtransmit.c:__check_stream_timer: ERROR Happened, Failed got stream %d reboot\n", i);
				mlog_write("stream error reboot");
				utl_system("reboot");
			}
		}
	}
	return 1;
}


static int autype = -1;
static void _mstream_parse(int channelid, jv_frame_info_t *info)
{
	U32 nLength = 0;
	nLength = sizeof(JVS_FRAME_HEADER);

	//在NXP平台，有可能存在一些控制信息，没有数据
	if (info->buffer == NULL || info->len == 0)
		return ;

	if (list_stream == channelid+1 && (info->frametype!=JV_FRAME_TYPE_A))
	{
		printf("(new)stream(%d) frameType(%s):Size = %d\n", channelid+1, (info->frametype==JV_FRAME_TYPE_I) ? "I":"P" , info->len);
		{
			static int iii = 0;
			iii++;
			if ((info->frametype==JV_FRAME_TYPE_I))
			{
				printf("frame: %d\n", iii);
				iii = 0;
			}
		}
	}
#ifdef ZRTSP_SUPPORT
	if (gp.bNeedZrtsp)
	{
#if defined PLATFORM_hi3516D || defined PLATFORM_hi3516EV100
		if( info->frametype == JV_FRAME_TYPE_A)
		{
			if(channelid == 0)
			{
				transStream_ex(channelid, (char *)info->buffer, info->len, info->timestamp, info->frametype,  0x80000001);
			}
		}
		else
		{
			transStream_ex(channelid, (char *)info->buffer, info->len, info->timestamp, info->frametype,  0x80000001);
		}
#else
	    //放在I帧前面发送sps和pps数据
		if( info->frametype == JV_FRAME_TYPE_A)
		{
		    if(channelid == 0)
		    {
		    	jv_audio_attr_t audio_attr;;
		    	jv_ai_get_attr(channelid, &audio_attr);
	            
	            AUDIO_CODEC audioType;
	    		switch (audio_attr.encType)
	    		{
	    		case JV_AUDIO_ENC_ADPCM:
	    			audioType = AC_ADPCM;
	    			break;
	    		case JV_AUDIO_ENC_G711_A:
	    			audioType = AC_G711A;
	    			break;
	    		case JV_AUDIO_ENC_G711_U:
	    			audioType = AC_G711U;
	    			break;
	    		case JV_AUDIO_ENC_G726_40K:
	    			audioType = AC_G726;
	    			break;
	            default:
	    			audioType = AC_G711U;
	    			break;
	    		}
	            if(autype != audioType)
	            {
	                autype = audioType;
	                Printf("=======\n   transStream   audioType:  %d\n", audioType);
	            }
		    	transStream(channelid, info->buffer, info->len, info->timestamp, e_NALU_TYPE_AUDIO, audioType, 0x80000001);
		    }
		}
		else
		{
			transStream(0, info->buffer, info->len, info->timestamp, jv_stream_get_nalu_tpye(), channelid, 0x80000001);
		}
#endif
	}
#endif

	//网传
	int i;
	for (i=0;i<MAX_CALLBACK_CNT;i++)
	{
		if (status.callback[i])
			status.callback[i](channelid, info->buffer, info->len, info->frametype, info->timestamp);
		else
			break;
	}

	{
    	jv_audio_attr_t ai_attr;
    	// jv_ai_get_attr(channelid, &ai_attr);		// 不能用channelid，只有一个声道
    	jv_ai_get_attr(0, &ai_attr);

		Rtmp_SendData(RTMP_LIVE_HDL(0, channelid), Rtmp_ConvDataType(info->frametype, ai_attr.encType),
						(char*)info->buffer, info->len, info->timestamp, info->timestamp);
	}
#ifdef LIVE555_SUPPORT
	if (info->frametype == JV_FRAME_TYPE_A)
	{
		if (list_stream != 4)
		jvlive_rtsp_write_audio(channelid, info->buffer, info->len);
	}
	else if (only_stream == 0 || only_stream == channelid+1)
	{
		static int bIframeOK[MAX_STREAM] = {0};
		//当一个I帧序列中，有一个数据失败后，其余的数据就没必要再往里放了。直到又一个I帧序列开始
		if (bIframeOK[channelid] || info->frametype == JV_FRAME_TYPE_I)
		{
			bIframeOK[channelid] = !jvlive_rtsp_write(channelid, info->buffer, info->len);
		}
	}
#endif
#ifdef GB28181_SUPPORT
	if (channelid == 0)
	{
		if (info->frametype == JV_FRAME_TYPE_A)
		{
			gb_send_data_a(channelid, info->buffer, info->len, 0, 0);//A帧默认和视频保持同步
		}
		else if (info->frametype == JV_FRAME_TYPE_I)
		{
			gb_send_data_i(channelid, info->buffer, info->len,
					status.streamlist[channelid].framerate,
					status.streamlist[channelid].width,
					status.streamlist[channelid].height,
					status.streamlist[channelid].bitrate,
					info->timestamp);

		}
		else if (info->frametype == JV_FRAME_TYPE_P)
		{
			gb_send_data_p(channelid, info->buffer, info->len, info->timestamp);
		}
	}
#endif

	//云存储视频上传第二码流，音频第一码流
	if ((info->frametype == JV_FRAME_TYPE_A) &&
		(channelid == 0))
	{
		mrecord_cloud_write(RECORD_CLOUD_CHN, info->buffer, info->len, info->frametype,info->timestamp);
	}
	if ((info->frametype != JV_FRAME_TYPE_A) &&
		(channelid == RECORD_CLOUD_CHN))
	{
		mrecord_cloud_write(RECORD_CLOUD_CHN, info->buffer, info->len, info->frametype,info->timestamp);
	}

	//录像要放到最后。因为它内部将数据改变了
	if(RECORD_CHN == channelid)
	{
		mrecord_write(0, info->buffer, info->len, info->frametype,info->timestamp);
	}
	
}

int focus_reference_value = 0;

static void _mstream_process(void *param)
{
	int i, j;
	jv_stream_pollfd fd[MAX_STREAM];
	static jv_frame_info_t info;
	jv_audio_frame_t frame;
	int cnt;
	int ret;

	pthreadinfo_add((char *)__func__);

	while(group.running)
	{
		cnt = 0;
		for (i=0; i<HWINFO_STREAM_CNT; i++)
		{
			if (status.running[i])
			{
				fd[cnt].channelid = i;
				fd[cnt++].received = 0;
			}
		}

		if (cnt == 0)
		{
			usleep(10000);
			continue;
		}

		if (jv_stream_poll(fd, cnt, 40) > 0)
		{
			int videoFrameType;
			for (i=0; i<cnt; i++)
			{
				if (fd[i].received)
				{
					ret = jv_stream_read(fd[i].channelid, &info);
					videoFrameType = info.frametype;
					if (ret < 0)
					{
						Printf("ERROR: jv_stream_read read failed: %d\n",ret);
						continue;
					}

					sStreamCnt[info.streamid]++;

					_mstream_parse(info.streamid, &info);
					jv_stream_release(info.streamid, &info);

					if (info.streamid == 0)
					{
						if (videoFrameType == JV_FRAME_TYPE_I)
						{
							focus_reference_value = info.len;
//							printf("focus_reference_value: %d\n", focus_reference_value);
						}

						if(status.streamlist[0].bAudioEn)
						{
							for (;;)
							{
								ret = jv_ai_get_frame(info.streamid, &frame);

								if (ret != 0)
									break;
								memcpy(info.buffer, frame.aData, frame.u32Len);
								info.len = frame.u32Len;
								info.frametype = JV_FRAME_TYPE_A;
								info.timestamp = frame.u64TimeStamp/1000;
								for (j=0; j<HWINFO_STREAM_CNT; j++)
								{
									_mstream_parse(j, &info);
								}
							}
						}
					}
				}
			}
		}
		usleep(1);
	}
}

#include    <sys/wait.h>   
#include    <errno.h>   
#include    <fcntl.h>   
//#include    "ourhdr.h"   
  
static pid_t    *childpid = NULL;  
                        /* ptr to array allocated at run-time */  
static int      maxfd;  /* from our open_max(), {Prog openmax} */  
  
#define SHELL   "/bin/sh"   


FILE *  
popen111(const char *cmdstring, const char *type)  
{  
    int     i, pfd[2];  
    pid_t   pid;  
    FILE    *fp;  

    if (childpid == NULL) {     /* first time through */  
                /* allocate zeroed out array for child pids */  
        maxfd = 10;  
        if ( (childpid = calloc(maxfd, sizeof(pid_t))) == NULL)  
			Printf("calloc Failed: maxfd: %d, pid_t: %d\n", maxfd, sizeof(pid_t));
            return(NULL);  
    }  
  Printf("test\n");
    if (pipe(pfd) < 0)  
    {
		Printf("pipe Failed\n");
        return(NULL);   /* errno set by pipe() */  
    }
	Printf("test\n");
  
    if ( (pid = fork()) < 0)  
    {
		Printf("fork Failed\n");
        return(NULL);   /* errno set by fork() */  
    }
    else if (pid == 0) {                            /* child */  
		Printf("test\n");
        if (*type == 'r') {  
            close(pfd[0]);  
            if (pfd[1] != STDOUT_FILENO) {  
                dup2(pfd[1], STDOUT_FILENO);  
                close(pfd[1]);  
            }  
        } else {  
            close(pfd[1]);  
            if (pfd[0] != STDIN_FILENO) {  
                dup2(pfd[0], STDIN_FILENO);  
                close(pfd[0]);  
            }  
        }  
            /* close all descriptors in childpid[] */  
        for (i = 0; i < maxfd; i++)  
            if (childpid[ i ] > 0)  
                close(i);  
  
        execl(SHELL, "sh", "-c", cmdstring, (char *) 0);  
        _exit(127);  
    }  
                                /* parent */  
								Printf("test\n");
    if (*type == 'r') {  
		Printf("test\n");
        close(pfd[1]);  
        if ( (fp = fdopen(pfd[0], type)) == NULL)  
        {
			Printf("fdopen Failed\n");
            return(NULL);  
        }
		Printf("test\n");
    } else {  
        close(pfd[0]);  
        if ( (fp = fdopen(pfd[1], type)) == NULL)  
	{
		Printf("fdopen Failed\n");
		return(NULL);  
	}
    }  
	Printf("test\n");
    childpid[fileno(fp)] = pid; /* remember child pid for this fd */  
	Printf("test\n");
    return(fp);  
}  

void transmit_list_client(void);

static int __mstream_debug_func(int argc, char *argv[])
{
	int channelid;
	printf("Here stream setting\n");
	if (argc < 3)
	{
		printf("too little argc: %d\n",argc);
		mchnosd_display_focus_reference_value(TRUE);
		return -1;
	}

	channelid = atoi(argv[2]);
	if (!strcmp(argv[1], "start"))
	{
		printf("start stream\n");
		mstream_start(channelid);

	}
	else if (!strcmp(argv[1], "stop"))
	{
		printf("stop stream\n");
		mstream_stop(channelid);
	}
	else if (!strcmp(argv[1], "restart"))
	{
		printf("restart stream\n");
		mstream_stop(channelid);
		mstream_start(channelid);
	}
	else if (!strcmp(argv[1], "roi"))
	{
		printf("set  roi  test\n");
		mstream_attr_t attr;
		mstream_get_param(channelid, &attr);
//		attr.mroi.jv_roi.roiCnt = 1;
//		printf("argv3: %s\n", argv[3]);
//		attr.mroi.jv_roi.roiWeight = atoi(argv[3]);
//		printf("attr.roiWeight: %d\n", attr.mroi.jv_roi.roiWeight);
//		attr.mroi.jv_roi.roi[0].w = status.streamlist[channelid].viWidth/2;
//		attr.mroi.jv_roi.roi[0].h = status.streamlist[channelid].viHeight/2;
//		attr.mroi.jv_roi.roi[0].x = 0;//attr.width/4;
//		attr.mroi.jv_roi.roi[0].y = 0;//attr.height/4;
//		mstream_set_param(channelid, &attr);
		mstream_flush(channelid);
	}
	else if (!strcmp(argv[1], "flush"))
	{
		printf("flush stream\n");
		mstream_flush(channelid);
	}
	else if (!strcmp(argv[1], "debug"))
	{
		printf("debug received stream\n");
		list_stream = channelid;
	}
	else if (!strcmp(argv[1], "only"))
	{
		printf("debug received stream\n");
		only_stream = channelid;
	}
	else if (!strcmp(argv[1], "set"))
	{
		int value;
		printf("set param\n");
		value = atoi(argv[4]);
		if (!strcmp(argv[3], "width"))
		{
			mstream_set(channelid, width, value);
		}
		else if (!strcmp(argv[3], "height"))
		{
			mstream_set(channelid, height, value);
		}
		else if (!strcmp(argv[3], "framerate"))
		{
			mstream_set(channelid, framerate, value);
		}
		else if (!strcmp(argv[3], "nGOP_S"))
		{
			mstream_set(channelid, nGOP_S, value);
		}
		else if (!strcmp(argv[3], "bitrate"))
		{
			mstream_set(channelid, bitrate, value);
		}
	}
	else if (!strcmp(argv[1], "popen"))
	{
		FILE *ret;
		if (channelid == 0)
		{
			ret = popen111("ls", "r");
			if (ret == NULL)
			{
				Printf("ERROR: Failed popen111: %s\n", strerror(errno));
			}
		}
		else
		{
			ret = popen("ls", "r");
			if (ret == NULL)
			{
				Printf("ERROR: Failed popen111: %s\n", strerror(errno));
			}
		}
	}
	else if (!strcmp(argv[1], "snap"))
	{
		msnapshot_get_file(channelid, "abc.jpg");
	}
	return 0;
}

/**
 * 设置RTSP播放器的用户名和密码
 *@note 可以这样登录：rtsp://192.168.11.79/live0.264?user=admin&passwd=aaa
 */
int mstream_rtsp_user_changed(void)
{
	if (!maccount_need_check_power())
		return 0;
#ifdef LIVE555_SUPPORT
	int cnt = maccount_get_cnt();
	int i;
	ACCOUNT *act;

	for (i=0;i<cnt;i++)
	{
		act = maccount_get(i);
		jvlive_rtsp_user_add_start(act->acID, act->acPW);
	}
	jvlive_rtsp_user_add_finished();
#endif
	return 0;
}

static pthread_t jvliveThread;

#ifdef ZRTSP_SUPPORT
//static MNG_ACC userinfo;
//static int ptype = -1;
int devGetInfoMng(DEV_INFO_TYPEs type, char *buf, int chn, int platformId)
{
    //if(ptype != type)
    //{
    //    ptype = type;
    //    printf("%s  %d\n", __FUNCTION__, type); 
    //}
#if 1
    switch(type)
    {
        case e_DEV_INFO_SECRIT_STATE:
        {printf(".....................devGetInfoMng e_DEV_INFO_SECRIT_STATE\n");
            return 0; //获取隐私保护状态
        }
        case e_DEV_INFO_VIDEO_NUM:
        {printf(".....................devGetInfoMng e_DEV_INFO_VIDEO_NUM\n");
            return 1; //视频通道数，ipc返回1，dvr返回实际通道数
        }
        case e_DEV_INFO_MAX_USER: //返回最大用户数
            return 8;
        case e_DEV_INFO_DEV_NAME: //返回设备名称
        {printf(".....................devGetInfoMng e_DEV_INFO_DEV_NAME\n");
            strcpy(buf, "unknow");
            return strlen("unknow")+1;
        }
        case e_DEV_INFO_VIDEO_CODEC: //返回视频编码
        {
        	//Printf(".....................devGetInfoMng e_DEV_INFO_VIDEO_CODEC\n");
        	switch(status.streamlist[0].vencType)
        	{
        	default:
        	case JV_PT_H264:
        		return VC_H264;
        	case JV_PT_H265:
        		return VC_H265;
        	}
        }
        case e_DEV_INFO_AUDIO_CODEC:
        {
        	if(!status.streamlist[0].bAudioEn)
        		return -1;
            //return AC_G711A;	//返回音频编码
			jv_audio_attr_t ai_attr;
    		jv_ai_get_attr(0, &ai_attr);
			if(buf != NULL)
			{
				AUDIO_CODEC_ATTR* audio_attr = (AUDIO_CODEC_ATTR*)buf;
				audio_attr->MicDev = 1;
				audio_attr->sampleRate = ai_attr.sampleRate;
	//			memcpy(buf, (char *)&audio_attr, sizeof(audio_attr));

			}
    		switch (ai_attr.encType)
    		{
    		case JV_AUDIO_ENC_ADPCM: 
                {
                    Printf(".....................devGetInfoMng e_DEV_INFO_AUDIO_CODEC   audioType:  AC_ADPCM\n");
        			return AC_ADPCM;
        		}
    		case JV_AUDIO_ENC_G711_A:
                {
                	Printf(".....................devGetInfoMng e_DEV_INFO_AUDIO_CODEC   audioType:  AC_G711A\n");
        			return AC_G711A;
        		}
    		case JV_AUDIO_ENC_G711_U:
                {
                	Printf(".....................devGetInfoMng e_DEV_INFO_AUDIO_CODEC   audioType:  AC_G711U\n");
        			return AC_G711U;
        		}
    		case JV_AUDIO_ENC_G726_40K:
                {
                	Printf(".....................devGetInfoMng e_DEV_INFO_AUDIO_CODEC   audioType:  AC_G726\n");
        			return AC_G726;
        		}
			case JV_AUDIO_ENC_AAC:
                {
                	Printf(".....................devGetInfoMng e_DEV_INFO_AUDIO_CODEC   audioType:  AC_AAC\n");
        			return AC_AAC;
        		}
            default:
                {
                	Printf(".....................devGetInfoMng e_DEV_INFO_AUDIO_CODEC   audioType:  AC_G711U\n");
        			return AC_G711U;
        		}
    		}
        }
        case e_DEV_INFO_USER_INFO:   //获取用户账号信息,填充MNG_ACC
        {printf(".....................devGetInfoMng e_DEV_INFO_USER_INFO\n");
        	MNG_ACC user = {0};
            MNG_ACC *tmp = (MNG_ACC*)buf;
        	ACCOUNT *act;
            int i;
            int cnt = maccount_get_cnt();
        	
            user.level=0;
        	for (i=0; i<cnt; i++)
        	{
        		act = maccount_get(i);
                if(strcmp(tmp->name, act->acID) == 0)
                {
                    printf("====================%s:%s(%s)   %d\n",act->acID,act->acPW,act->acDescript,act->nPower);
                    //user.level = act->nPower;
                    strncpy(user.description,act->acDescript,16);
                    strcpy(user.name,act->acID);
                	strcpy(user.pwd,act->acPW); 
                    break;
                }   
        	}
			
			if(i >= cnt)
			{
				return -1;
			}
			else
			{
				memcpy(buf, (char *)&user, sizeof(MNG_ACC));
				return 0;
			}
            break;
        }
        case e_DEV_INFO_VIDEO_SPS_PPS: //获取SPS PPS信息
        {printf(".....................devGetInfoMng e_DEV_INFO_VIDEO_SPS_PPS\n");
            SPS_PPS *p = (SPS_PPS *)buf;
            int n = p->sps[0];
            SPS_PPS psppps;
            jv_stream_get_spspps(n,&psppps);
			//printf("chn:%d	psppps	(%s)	(%s ; %s)\n",n, psppps.sps, psppps.pps[0],psppps.pps[1]);
            memcpy(buf, (char *)&psppps, sizeof(SPS_PPS));
            return sizeof(SPS_PPS);
        }
		case e_DEV_INFO_LOCAL_ADDR://获取本机ip
        {printf(".....................devGetInfoMng e_DEV_INFO_LOCAL_ADDR\n");
			eth_t ueth;
			int ipret;
			ipret = utl_ifconfig_eth_get(&ueth);
			if(ipret == 0)
			{
            	strcpy(buf, ueth.addr);
				printf("IP:%s	eth:%s\n", buf, ueth.addr);
			}
            return 0;
        }
		case e_DEV_INFO_MEDIA_URL_INFO:
        {
            DEV_MEDIA_URL_INFO *p = (DEV_MEDIA_URL_INFO *)buf;
			char *token = strstr(p->url, "hashtoken");
            //分析p->url后,填充chn,media,stream,返回0时使用此处返回的信息,返回非0时,使用内部url分析
//rtsp://localip:localport/realplay?devid=xxx&channelno=xxx&streamtype=xxx&hashtoken=xxx
			if(token != NULL)
			{
				printf("p->url = %s\n", p->url);
				p->chn = 0;
				if(strstr(p->url, "track2") != NULL || strstr(p->url, "streamid=1") != NULL)
					p->media = 0;//media=1是视频,=0是音频
				else
					p->media = 1;//media=1是视频,=0是音频
				char *streamtype = strstr(p->url, "streamtype");
				p->stream = atoi(streamtype+11)-1;
				char hashtoken[64];
				strncpy(hashtoken, token+11, 32);
				//telestore_token_verify(atoi(streamtype+11), hashtoken);
				printf("p->chn=%d, p->media=%d, p->stream=%d\n",p->chn,p->media,p->stream);
            	return 0;
			}
			else
			{
				printf("p->url = %s\n", p->url);
            	return 1;
			}
        }
		//RTSP和account中的密码长度不同，是个BUG，这个解决办法目前只在18E起作用
		//如果想在其他单板上解决，要找音视频实验室提供对应的.a库文件和.h
		case e_DEV_INFO_USER_PWD:  
		{
        	ACCOUNT *act;
            int i;
            int cnt = maccount_get_cnt();
        	
        	for (i=0; i<cnt; i++)
        	{
        		act = maccount_get(i);
                if(strcmp(buf, act->acID) == 0)
                {
                    break;
                }   
        	}
        	if(i >= cnt)
        	{
        		return 0;
        	}
        	memcpy(buf, act->acPW,sizeof(act->acPW));
			return strlen(act->acPW);
		}
		case e_DEV_INFO_VIDEO_BITRATE:
		{
			return status.streamlist[chn].bitrate;
		}
		case e_DEV_INFO_IPPOWER:
		{
			return TRUE;
		}
        default:
        	break;
    }
#endif
	return 0;
}

//static int qtype =-1; //调试用
int devSetInfoMng(DEV_INFO_TYPEs type, char *buf, int paraLen)
{
    //if(qtype != type)
    //{
    //    qtype = type;
    //    printf("%s  %d\n", __FUNCTION__, type); 
    //}
#if 1
    switch(type)
    {
		case e_DEV_INFO_VIDEO_CTRL: 
        {printf("++++++++++++++++++++devSetInfoMng e_DEV_INFO_VIDEO_CTRL	->");
			DEV_VIDOE_CTRL *p = (DEV_VIDOE_CTRL *)buf;
            printf("DEV_VIDOE_CTRL:%d, %d, %d	", p->cmd, p->chn, p->stream);
			switch(p->cmd)
            {
                case e_MEDIA_CTRL_START:
                {
					printf("	e_MEDIA_CTRL_START\n");
                    break;
                }
                case e_MEDIA_CTRL_STOP:
                {       
					printf("	e_MEDIA_CTRL_STOP\n");
                    break;
                }
                case e_MDEIA_CTRL_NEED_IFRAME://申请I帧
                {
					printf("	e_MDEIA_CTRL_NEED_IFRAME\n");
					mstream_request_idr(p->stream);
                    //msgIFreamAdd(session, p->chn, p->stream, 1);    
                    break;
                }
				default:
					printf("	default\n");
					break;
            }
            return 0;
        }
		
        case e_DEV_INFO_USER_STATE: //设置连接状态
        {printf("++++++++++++++++++++devSetInfoMng e_DEV_INFO_USER_STATE\n");
            DEV_USER_STATE *state= (DEV_USER_STATE *)buf;
            SPConnection_t con;
        	memset(&con, 0, sizeof(con));
        	struct in_addr in;
    		in.s_addr = state->ip;
    		char *ipaddr = inet_ntoa(in);
//    		printf("USER:%s  ipaddr:%s  sockid:%ld  ", state->acc, ipaddr, state->id);
        	if(state->st == e_DEV_USER_STATE_CONNECTED)
        	{
        	    printf("rtsp:connected\n");
        		strcpy(con.addr, ipaddr);
        		con.conType = SP_CON_RTSP;
        		con.key = state->id;
        		strcpy(con.protocol, "rtsp");
        		strcpy(con.user, state->acc);//userinfo.name);
        		sp_connect_add(&con);
        	}
        	else if(state->st == e_DEV_USER_STATE_STOP)
        	{
        	    printf("rtsp:disconnected\n");
        	    con.conType = SP_CON_RTSP;
        		con.key = state->id;
        		sp_connect_del(&con);
        	}
        	else if(state->st == e_DEV_USER_STATE_AUTH_FAILED)
        	{
                printf("auth failed\n");  
        	}
        	else if(state->st == e_DEV_USER_STATE_PLAY)
        	{
                printf("play\n");  
        	}
        	else if(state->st == e_DEV_USER_STATE_SETUP)
        	{
                printf("setup\n");  
        	}
        	else if(state->st == e_DEV_USER_STATE_PAUSE)
        	{
                printf("pause\n");  
        	}
        }
            break;
        default:
        	break;
    }
#endif
    return 0;
}
#endif

#ifdef LIVE555_SUPPORT
static void __jvlive_callback(jvlive_info_t *info)
{
	SPConnection_t con;
	memset(&con, 0, sizeof(con));
	if (info->type == JVLIVE_CMD_TYPE_CONNECTED)
	{
		printf("connected addr: %s\n", info->connect.addr);
		strcpy(con.addr, info->connect.addr);
		con.conType = SP_CON_RTSP;
		con.key = info->connect.sessionID;
		strcpy(con.protocol, "rtsp");
		strcpy(con.user, "rtsp");
		sp_connect_add(&con);
	}
	else if (info->type == JVLIVE_CMD_TYPE_DISCONNECTED)
	{
		con.conType = SP_CON_RTSP;
		con.key = info->connect.sessionID;
		sp_connect_del(&con);
	}
}
#endif

static void *_jvlive_thread(void *param)
{
	pthreadinfo_add((char *)__func__);
	//稍等片刻，将帧率参数读到先
	usleep(2*1000*1000);
#ifdef LIVE555_SUPPORT
	jvliveParam_t liveInit;
	liveInit.callback = __jvlive_callback;
	liveInit.max_frame_len = MAX_FRAME_LEN;
	jvlive_rtsp_init(&liveInit);

	{
		jv_audio_attr_t ai_attr;
		char *audioType;
		int bufSize[] = {2*1024*1024, 512*1024, 256*1024};
		int frameList[3];
		jv_ai_get_attr(0, &ai_attr);
		switch (ai_attr.encType)
		{
		default:
			audioType = NULL;
			break;
		case JV_AUDIO_ENC_ADPCM:
			audioType = "DVI4";
			break;
		case JV_AUDIO_ENC_G711_A:
			audioType = "PCMA";
			break;
		case JV_AUDIO_ENC_G711_U:
			audioType = "PCMU";
			break;
		case JV_AUDIO_ENC_G726_40K:
			audioType = "G726-40";
			break;
		}
		Printf("Current frame: %s\n", audioType);
		int i;
		for (i=0;i<HWINFO_STREAM_CNT;i++)
		{
			jv_stream_attr attr;
			jv_stream_get_attr(i, &attr);
			{
				float cur, normal;
				BOOL bLowFrame = jv_sensor_get_b_low_frame(0, &cur, &normal);
				if (bLowFrame)
				{
					attr.framerate = attr.framerate * cur / normal;
				}
			}

			frameList[i] = attr.framerate;
		}
		mstream_rtsp_user_changed();
//		int bTongWei = 1;//同为的破机器不支持音频的处理
//		if(bTongWei)
//			audioType = NULL;
		jvlive_rtsp_start_ex("live%d.264", 554, bufSize, frameList, HWINFO_STREAM_CNT, audioType, 8000);
	}
#endif
	return NULL;
}

/**
 *@brief 初始化
 *
 */
int mstream_init(void)
{
#ifdef ZRTSP_SUPPORT
	if (gp.bNeedZrtsp)
	{
		printf("zrtsp start......\n");
		serviceStart(554, 6000, devGetInfoMng, devSetInfoMng, 0);
	}
#endif
	memset(&status, 0, sizeof(status));
	jv_stream_init();

	//获取支持的最大帧率,lck20120814
	jvstream_ability_t ability;
	jv_stream_get_ability(0, &ability);
	nMaxFramerate = ability.maxFramerate;

	group.running = TRUE;
	pthread_mutex_init(&group.mutex, NULL);
#ifdef LIVE555_SUPPORT
	pthread_create(&jvliveThread, NULL, (void *)_jvlive_thread, NULL);
#endif

	// 提高线程优先级，改善清缓存时的卡顿问题
	{
	#if 0
		int rr_min_priority, rr_max_priority;	
		struct sched_param thread_dec_param;
		pthread_attr_t thread_dec_attr;
		pthread_attr_init(&thread_dec_attr);
		pthread_attr_getschedparam(&thread_dec_attr, &thread_dec_param);
		pthread_attr_setstacksize(&thread_dec_attr, LINUX_THREAD_STACK_SIZE);					//栈大小
		
		rr_min_priority = sched_get_priority_min(SCHED_RR); 
		rr_max_priority = sched_get_priority_max(SCHED_RR);
		thread_dec_param.sched_priority = (rr_min_priority + rr_max_priority)/2+1 ; 

		pthread_attr_setschedpolicy(&thread_dec_attr, SCHED_RR);
		pthread_attr_setschedparam(&thread_dec_attr, &thread_dec_param);
		pthread_attr_setinheritsched(&thread_dec_attr, PTHREAD_EXPLICIT_SCHED); 	



		pthread_create(&group.thread, &thread_dec_attr, (void *)_mstream_process, NULL);

		pthread_attr_destroy(&thread_dec_attr);
	#else
		int ret;
		pthread_attr_t attr;
		struct sched_param param;

		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, LINUX_THREAD_STACK_SIZE);
		pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

		pthread_attr_getschedparam(&attr, &param);
		param.sched_priority = 60;
		pthread_attr_setschedparam(&attr, &param);

		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

		ret = pthread_create(&group.thread, &attr, (void *)_mstream_process, NULL);
		if (ret)
		{
			printf("pthread create failed with %s(%d), ret: %d!\n", strerror(errno), errno, ret);
			exit(0);
		}

		pthread_attr_destroy(&attr);	
		#endif
	}

	utl_timer_create("stream check", 3000, __check_stream_timer, NULL);

	utl_cmd_insert("stream",
	               "for stream test",
	               "\tstream CMD CHANNELID\n"
	               "\tstream set CHANNELID TYPE VALUE\n"
	               "\n\tCMD:\n"
	               "\t\tstart	start the stream\n"
	               "\t\tstop	stop the stream\n"
	               "\t\tflush	flush the stream\n"
	               "\t\trestart	restart the stream\n"
	               "\t\tset	set param\n"
	               "\t\tdebug	if be 1, print the received stream package\n"
	               "\n\tTYPE When set:\n"
	               "\t\twidth resolution width\n"
	               "\t\theight resolution height\n"
	               "\t\tframerate framerate such as 30,25,20,15,10...\n"
	               "\t\tnGOP I frame between\n"
	               "\t\tbitrate bitrate with unit of Kbit Per Second\n"
	               "\n",
	               __mstream_debug_func);

	return 0;
}

/**
 *@brief 结束
 *
 */
int mstream_deinit(void)
{
	int i;
	group.running = FALSE;

	for (i=0; i<HWINFO_STREAM_CNT; i++)
	{
		if (status.running[i])
			mstream_stop(i);
	}
	
	pthread_join(group.thread, NULL);
	pthread_mutex_destroy(&group.mutex);
	jv_stream_deinit();
	return 0;
}

/**
 *@brief 设置网传的回调函数
 *@param callback 网传的回调函数指针
 *
 */
int mstream_set_transmit_callback(mstream_transmit_callback_t callback)
{
	int i;
	for (i = 0; i < MAX_CALLBACK_CNT; ++i)
	{
		if (!status.callback[i])
		{
			status.callback[i] = callback;
			break;
		}
		else if (status.callback[i] == callback)
			break;
	}

	if (i >= MAX_CALLBACK_CNT)
	{
		printf("\n%s, ==========Warnning: callback not registered!\n\n", __func__);
	}
	return 0;
}

/**
 *@brief 检查参数，修正超过范围的参数
 */
static void _mstream_valid_param(int channelid, mstream_attr_t *attr)
{
	jvstream_ability_t ability;
	jv_stream_get_ability(channelid, &ability);
	mstream_resolution_valid(channelid, &attr->width, &attr->height);
	attr->framerate = VALIDVALUE(attr->framerate, ability.minFramerate, ability.maxFramerate);
	int gop = attr->nGOP_S * attr->framerate;
	gop = VALIDVALUE(gop, ability.minNGOP, ability.maxNGOP);

	attr->nGOP_S = (gop + attr->framerate - 1)/attr->framerate;
	attr->bitrate = VALIDVALUE(attr->bitrate, ability.minKBitrate, ability.maxKBitrate);
	attr->minQP = VALIDVALUE(attr->minQP, 0, 51);
	attr->maxQP = VALIDVALUE(attr->maxQP, attr->minQP, 51);

	if(channelid == 1)//解决海康4路nvr次码流无法连接>1024kbps的问题。这里是为了方便客户升级新版本之后不再手动设置，以后可以去掉。
	{
		if(attr->bitrate == 1081)
			attr->bitrate = 1024;
	}
}

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_param(int channelid, mstream_attr_t *attr)
{
	static char first = 1;
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	jv_assert(attr != NULL, return JVERR_BADPARAM);
	_mstream_valid_param(channelid, attr);
	if (status.streamlist[channelid].width != attr->width
	        || status.streamlist[channelid].height != attr->height)
	{
		status.needRestart[channelid] = TRUE;
		if(channelid==0)
		{
			status.changMainRes = TRUE;
			status.needRestart[1] = TRUE;
			status.needRestart[2] = TRUE;
		}
		mlog_write("Stream Setting: Resolution Changed [%dx%d]", attr->width, attr->height);
	}
	if (status.streamlist[channelid].rcMode != attr->rcMode)
	{
		status.needRestart[channelid] = TRUE;
		mlog_write("Stream Setting: rcMode Changed [%d]", attr->rcMode);
	}
	if (status.streamlist[channelid].framerate != attr->framerate)
	{
#ifdef LIVE555_SUPPORT
		jvlive_rtsp_set_framerate(channelid, attr->framerate);
#endif

		status.needRestart[channelid] = TRUE;
		mlog_write("Stream Setting: Framerate Changed [%dfps]", attr->framerate);
	}
	if (status.streamlist[channelid].nGOP_S != attr->nGOP_S)
	{
		mlog_write("Stream Setting: IFrame Interval Changed [%d]", attr->nGOP_S);
	}
	if (status.streamlist[channelid].bitrate != attr->bitrate)
	{
		mlog_write("Stream Setting: Bitrate Changed [%dkbps]", attr->bitrate);
	}
//	if (status.streamlist[channelid].mroi.jv_roi.roiCnt != attr->mroi.jv_roi.roiCnt
//			|| 0 != memcmp(status.streamlist[channelid].mroi.jv_roi.roi, attr->mroi.jv_roi.roi, sizeof(attr->mroi.jv_roi.roi))
//			|| status.streamlist[channelid].mroi.jv_roi.roiWeight != attr->mroi.jv_roi.roiWeight)
//	{
//		status.needRestart[channelid] = TRUE;
//		mlog_write("Stream Setting: ROI Changed [%dkbps]", attr->bitrate);
//	}
	if (status.streamlist[channelid].vencType != attr->vencType)
	{
		status.needRestart[channelid] = TRUE;
		bVencTypeChanged = 1;
	}
	if(attr->bRectStretch != 0)
	{
		attr->bRectStretch = 0;
	}
#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100)
	if(hwinfo.encryptCode == ENCRYPT_200W)
	{
		attr->bRectStretch = 1;
	}
#endif

	memcpy(&status.streamlist[channelid], attr, sizeof(mstream_attr_t));
	if(channelid == 1 || channelid == 0)
	{
		if(status.streamlist[0].framerate <= 25 && status.streamlist[1].framerate>25)
		{
			status.streamlist[1].framerate = 25;
			if(channelid == 0)
			{
				jv_stream_attr sAttr;
				jv_stream_get_attr(1, &sAttr);
				sAttr.framerate = status.streamlist[1].framerate;
				jv_stream_set_attr(1, &sAttr);
			}
		}
	}

	return 0;
}

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_get_param(int channelid, mstream_attr_t *attr)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	jv_assert(attr != NULL, return JVERR_BADPARAM);

	jv_stream_get_vi_resolution(channelid,&status.streamlist[channelid].viWidth, &status.streamlist[channelid].viHeight);
	memcpy(attr, &status.streamlist[channelid], sizeof(mstream_attr_t));
	return 0;
}

/**
 *@brief 获取运行参数，有些时候，限于编码能力的限制，设置的参数与实际运行参数不符
 *@param channelid 通道号
 *@param attr 属性
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_get_running_param(int channelid, mstream_attr_t *attr)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	jv_assert(attr != NULL, return JVERR_BADPARAM);

	jv_stream_get_vi_resolution(channelid,&status.streamlist[channelid].viWidth, &status.streamlist[channelid].viHeight);
	memcpy(attr, &status.streamlist[channelid], sizeof(mstream_attr_t));
	jv_stream_attr jvattr;
	jv_stream_get_attr(channelid, &jvattr);
	if(jvattr.width>0 && jvattr.height>0 && jvattr.framerate!=0)
	{
		attr->width = jvattr.width;
		attr->height = jvattr.height;
		attr->framerate = jvattr.framerate;
	}

	return 0;
}

int mstream_stop(int channelid)
{
	if (status.running[channelid])
	{
		status.running[channelid] = FALSE;
		jv_stream_stop(channelid);
	}
	return 0;
}

int mstream_start(int channelid)
{
	if (status.running[channelid])
	{
		return -1;
	}
	jv_stream_start(channelid);
	status.running[channelid] = TRUE;
	return 0;
}

int mstream_restart(int channelid)
{
	mstream_stop(channelid);
	jv_stream_start(channelid);
	return 0;
}

/**
 *@brief 刷新通道，使之前的设置生效
 *@param channelid 通道号
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_flush(int channelid)
{
	if (channelid >= HWINFO_STREAM_CNT)
		return 0;
	static int RestartOther = 0;

	JVRotate_e rotate = msensor_get_rotate();

	jv_stream_attr attr;
	jv_stream_get_attr(channelid, &attr);
	attr.width = status.streamlist[channelid].width;
	attr.height = status.streamlist[channelid].height;
	attr.framerate = status.streamlist[channelid].framerate;
	attr.nGOP = status.streamlist[channelid].nGOP_S*status.streamlist[channelid].framerate;
	attr.bitrate = status.streamlist[channelid].bitrate;
	attr.rcMode = status.streamlist[channelid].rcMode;
	attr.maxQP = status.streamlist[channelid].maxQP;
	attr.minQP = status.streamlist[channelid].minQP;
	attr.roiInfo.ior_reverse = status.mroi.ior_reverse;
	attr.roiInfo.roiWeight = status.mroi.roiWeight*50/255;
	attr.bRectStretch = status.streamlist[channelid].bRectStretch;
	attr.vencType = status.streamlist[channelid].vencType;

	int i;
	unsigned int viWidth, viHeight;
	jv_stream_get_vi_resolution(0, &viWidth, &viHeight);
	for (i=0;i<MAX_ROI_REGION;i++)
	{
		attr.roiInfo.roi[i].x = status.mroi.roi[i].x * attr.width /  viWidth;
		attr.roiInfo.roi[i].w = status.mroi.roi[i].w * attr.width /  viWidth;
		attr.roiInfo.roi[i].y = status.mroi.roi[i].y * attr.height / viHeight;
		attr.roiInfo.roi[i].h = status.mroi.roi[i].h * attr.height / viHeight;

		attr.roiInfo.roi[i].x = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].x, 16);
		attr.roiInfo.roi[i].y = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].y, 16);
		attr.roiInfo.roi[i].w = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].w, 16);
		attr.roiInfo.roi[i].h = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].h, 16);
	}

#if 0
	printf("mstream_flush: channelid=%d, bEnable=%d, needRestart=%d, running=%d, changMainRes=%d\n",
		channelid, status.streamlist[channelid].bEnable, status.needRestart[channelid], 
		status.running[channelid], status.changMainRes);
#endif

	if (status.streamlist[channelid].bEnable)
	{
		if (status.needRestart[channelid])
		{
			status.needRestart[channelid] = FALSE;
			if (status.running[channelid])
			{
				if ((channelid == 1 && !status.changMainRes) || (status.changMainRes && channelid == 0))
					mdetect_stop(0);
				if ((channelid == RECORD_CHN && !status.changMainRes) || (status.changMainRes && channelid == 0))
					mrecord_stop(0);
				if(channelid == 0)
				{
					mivp_stop(0);
				}
				mchosd_region_stop();
				if(!status.changMainRes)
				{
					mchnosd_stop(channelid);
				}
				if(status.changMainRes && channelid == 0)
				{
					for(i = 0;i < HWINFO_STREAM_CNT;i++)
					{
						mchnosd_stop(i);
					}
				}

				if(!status.changMainRes)
					mstream_stop(channelid);
				if(status.changMainRes && channelid == 0)
				{
					for(i = 0;i < HWINFO_STREAM_CNT;i++)
						mstream_stop(i);
				}

			}
		}
		if (rotate != JVSENSOR_ROTATE_NONE)
		{
			for (i=0;i<MAX_ROI_REGION;i++)
			{
				msensor_rotate_calc(rotate, attr.width, attr.height, &attr.roiInfo.roi[i]);
				attr.roiInfo.roi[i].x = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].x, 16);
				attr.roiInfo.roi[i].y = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].y, 16);
				attr.roiInfo.roi[i].w = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].w, 16);
				attr.roiInfo.roi[i].h = JV_ALIGN_FLOOR(attr.roiInfo.roi[i].h, 16);
			}
			if (rotate == JVSENSOR_ROTATE_90 || rotate == JVSENSOR_ROTATE_270)
			{
				int temp;
				temp = attr.height;
				attr.height = attr.width;
				attr.width = temp;
			}
		}
		
		jv_stream_set_attr(channelid, &attr);
		if (!status.running[channelid])
		{
			mstream_start(channelid);

			if(!status.changMainRes)
			{
				mchnosd_flush(channelid);
				mprivacy_flush(0);
				ipcinfo_t ipcinfo;
				ipcinfo_get_param(&ipcinfo);
				multiosd_process(&ipcinfo);
				if(channelid == 0)
					mivp_start(0);				
			}
			
			if ((channelid == 1 && !status.changMainRes) || (status.changMainRes && channelid == HWINFO_STREAM_CNT - 1))
				mdetect_flush(0);
			if ((channelid == RECORD_CHN && !status.changMainRes) || (status.changMainRes && channelid == HWINFO_STREAM_CNT - 1))
				mrecord_flush(0);
			if(channelid == HWINFO_STREAM_CNT - 1 && status.changMainRes)
			{
				for(i = 0;i < HWINFO_STREAM_CNT;i++)
				{
					mchnosd_flush(i);
				}
				mprivacy_flush(0);
				ipcinfo_t ipcinfo;
				ipcinfo_get_param(&ipcinfo);
				multiosd_process(&ipcinfo);
				mivp_start(0);				
				status.changMainRes = FALSE;
			}
		}
	}
	else
	{
		if (status.running[channelid])
		{
			mstream_stop(channelid);
			if (channelid == 1)
				mdetect_stop(0);
			if (channelid == RECORD_CHN)
				mrecord_stop(0);
		}
	}
	return 0;
}



/**
 *@brief 设置帧率
 *@param channelid 通道号
 *@param framerate 帧率，如30，25，20。。。
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_framerate(int channelid, unsigned int framerate)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);

	mlog_write("Stream Setting: Framerate Changed [%d]", framerate);
	status.streamlist[channelid].framerate= VALIDVALUE(framerate, 1, 50);
	mstream_flush(channelid);
	return 0;
}

/**
 *@brief 设置帧率
 *@param channelid 通道号
 *@param framerate 帧率，如30，25，20。。。
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_resolution(int channelid, unsigned int width, unsigned int height)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);

	mlog_write("Stream Setting: Resolution Changed [%dx%d]", width, height);

	mstream_resolution_valid(channelid, &width, &height);
	status.streamlist[channelid].width= width;
	status.streamlist[channelid].height= height;
	status.needRestart[channelid] = TRUE;
	mstream_flush(channelid);

	return 0;
}

/**
 *@brief 设置关键帧间隔
 *@param channelid 通道号
 *@param gop 关键帧间隔
 *
 *@retval 0 成功
 *@retval <0 errno such as #JVERR_BADPARAM
 *
 */
int mstream_set_gop(int channelid, unsigned int gop)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	if (status.streamlist[channelid].nGOP_S != gop)
	{
		mlog_write("Stream Setting: IFrame Interval Changed [%d]", gop);
	}
	status.streamlist[channelid].nGOP_S= gop;
	_mstream_valid_param(channelid, &status.streamlist[channelid]);
	mstream_flush(channelid);
	return 0;
}

int mstream_set_bitrate(int channelid,unsigned int bitrate)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	if (status.streamlist[channelid].bitrate != bitrate)
	{
		mlog_write("Stream Setting: bitrate  Changed [%d]", bitrate);
	}
	status.streamlist[channelid].bitrate = bitrate;
	_mstream_valid_param(channelid, &status.streamlist[channelid]);
	mstream_flush(channelid);
	return 0;	
}

/**
 *@brief 以指定 的宽度和高度，寻找最匹配的分辨率
 *
 */
void mstream_resolution_valid(int channelid, unsigned int *width, unsigned int *height)
{
	jvstream_ability_t ability;
	resolution_t *reslist;
	int cnt;
	int i;
	int tmp, min, sub;
	int w_equal = -1;
	int h_equal = -1;

	{//有人会设置为0的分辨率。下面检查不出来
		if (*width == 0 || *height == 0)
		{
			*width = status.streamlist[channelid].width;
			*height = status.streamlist[channelid].height;
			return ;
		}
	}
	jv_stream_get_ability(channelid, &ability);

	if (*width * *height > ability.maxStreamRes[channelid])
	{
		*width = status.streamlist[channelid].width;
		*height = status.streamlist[channelid].height;
		return ;
	}

	cnt = ability.resListCnt;
	reslist = ability.resList;

	for (i=0; i<cnt; i++)
	{
		if (reslist[i].width == *width)
		{
			if (reslist[i].height == *height)
				//找到了完全匹配的
				return ;
			else
				w_equal = i;
		}
		else if(reslist[i].height == *height)
			h_equal = i;
	}

	//查找只匹配了一个的
	if (w_equal != -1)
	{
		tmp = w_equal;
	}
	else if(h_equal != -1)
		tmp = h_equal;
	else
	{
		//一个匹配的也没有，找个差值最小的
		min = 1000000;
		tmp = 0;
		for (i=0; i<cnt; i++)
		{
			sub = abs(reslist[i].width + reslist[i].height - *width - *height);
			if (min > sub)
			{
				min = sub;
				tmp = i;
			}
		}
	}

	*width = reslist[tmp].width;
	*height = reslist[tmp].height;
	return ;
}

/**
 *@brief 向解码器申请一帧关键帧
 *在录像开始或者有客户端连接时调用此函数
 *
 *@param channelid 通道号，表示要申请此通道输出关键帧
 */
void mstream_request_idr(int channelid)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return );
	jv_stream_request_idr(channelid);
}

/**
 *@brief 设置指定数据流的亮度
 *
 *@param channelid 通道号，目前摄像机为单sensor，传入0即可
 *@param brightness 亮度值
 *
 */
void mstream_set_brightness(int channelid, int brightness)
{
	jv_sensor_brightness(channelid, brightness);
}

/**
 *@brief 设置指定数据流的去雾强度
 *
 *@param channelid 通道号，目前摄像机为单sensor，传入0即可
 *@param antifog 去雾强度
 *
 */
void mstream_set_antifog(int channelid, int antifog)
{
	jv_sensor_antifog(channelid, antifog);
}

/**
 *@brief 设置指定数据流的饱和度
 *
 *@param channelid 通道号，目前摄像机为单sensor，传入0即可
 *@param saturation 饱和度值
 *
 */
void mstream_set_saturation(int channelid, int saturation)
{
	jv_sensor_saturation(channelid, saturation);
}

int mstream_audio_restart(int channelid,int bEn)
{
	jv_audio_attr_t attr;
	jv_ai_get_attr(channelid, &attr);
	jv_ai_stop(channelid);
	printf("********************************AI stop\n");
	if(bEn)
	{
		printf("********************************AI start\n");
		jv_ai_start(channelid, &attr);
	}
	return 0;
}

int mstream_audio_set_param(int channelid, jv_audio_attr_t *attr)
{
	jv_audio_attr_t oldAiAttr;
	jv_audio_attr_t oldAoAttr;
	jv_ai_get_attr(channelid, &oldAiAttr);
	jv_ao_get_attr(channelid, &oldAoAttr);
#if 0
	printf("setting: %d\t%d\t%d\t%d\t%d\n", 
		attr->sampleRate, attr->bitWidth, attr->encType, 
		attr->level, attr->micGain);
	printf("cur ai: %d\t%d\t%d\t%d\t%d\n", 
		oldAiAttr.sampleRate, oldAiAttr.bitWidth, oldAiAttr.encType, 
		oldAiAttr.level, oldAiAttr.micGain);
	printf("cur ao: %d\t%d\t%d\t%d\t%d\n", 
		oldAoAttr.sampleRate, oldAoAttr.bitWidth, oldAoAttr.encType, 
		oldAoAttr.level, oldAoAttr.micGain);
#endif

	jv_ai_set_attr(channelid, attr);
	if (oldAiAttr.bitWidth != attr->bitWidth
			|| oldAiAttr.encType != attr->encType
			|| oldAiAttr.sampleRate != attr->sampleRate
			|| oldAiAttr.micGain != attr->micGain)
	{
		jv_ai_stop(channelid);
		if(status.streamlist[channelid].bAudioEn)
			jv_ai_start(channelid, attr);
		
#ifdef LIVE555_SUPPORT
	{
		JVLiveAudioInfo_t audioInfo;
		audioInfo.timeStampFrequency = 8000;

		switch (attr->encType)
		{
		default:
			audioInfo.audioType[0] = '\0';
			break;
		case JV_AUDIO_ENC_ADPCM:
			strcpy(audioInfo.audioType, "DVI4");
			break;
		case JV_AUDIO_ENC_G711_A:
			strcpy(audioInfo.audioType, "PCMA");
			break;
		case JV_AUDIO_ENC_G711_U:
			strcpy(audioInfo.audioType, "PCMU");
			break;
		case JV_AUDIO_ENC_G726_40K:
			strcpy(audioInfo.audioType, "G726-40");
			break;
		}
		int i;
		for (i=0;i<HWINFO_STREAM_CNT;i++)
		{
			jvlive_rtsp_set_audio_info(i, &audioInfo);
		}
	}
#endif

	}

	if (oldAoAttr.bitWidth != attr->bitWidth
		|| oldAoAttr.encType != attr->encType
		|| oldAoAttr.sampleRate != attr->sampleRate)
	{
		jv_ao_stop(channelid);
		jv_ao_start(channelid, attr);
	}

	if(oldAoAttr.level != attr->level)
	{
		attr->level = (attr->level <= 0)?(1):(attr->level);
		jv_ao_ctrl(attr->level);	//设置音量
	}
	else
	{
		if((!strcmp(hwinfo.devName,"HA220-H2-A")) && (attr->level <= 0))
		{
			jv_ao_ctrl(0x02);
		}
		if((!strcmp(hwinfo.devName,"HC420-H2")) && 0x09 == oldAoAttr.level)
		{
			jv_ao_ctrl(0x04);
		}
	}

	return 0;
}
#define LKDEBUG 1
/**
 * 获取感兴趣区域信息
 */
int mstream_get_roi(mstream_roi_t *mroi)
{
#if LKDEBUG
	memcpy(mroi,&(status.mroi),sizeof(mstream_roi_t));
#endif
	return 0;
}
/**
 * 设置感兴趣区域信息
 */
int mstream_set_roi(mstream_roi_t *mroi)
{
#if LKDEBUG
	memcpy(&status.mroi,mroi,sizeof(mstream_roi_t));

	int i;
	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		status.needRestart[i] = TRUE;
		//mstream_flush(i);
	}

#endif
	return 0;
}

