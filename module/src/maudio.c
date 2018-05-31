/*
 * maudio.c
 *
 *  Created on: 2014年9月28日
 *      Author: QinLichao
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <jv_ao.h>
#include <jv_ai.h>
#include <utl_ifconfig.h>
#include <utl_audio.h>
#include "maudio.h"
#include "mioctrl.h"
#include "mipcinfo.h"
#include "SYSFuncs.h"

#if 0
#define AUDIO_DEBUG printf
#else
#define AUDIO_DEBUG
#endif

static jv_thread_group_t audio_group;
static BOOL bAudioEnding = FALSE;
static BOOL bPreRunning = FALSE;

pthread_mutex_t soundmode_mutex;

static void _speak_process(void *param)
{
	char *fname=(char *)param;
	int ret=0;
	int nFrameCnt = 0;
	int minPlayTime = 0;		// 最小播放时间，ms

	pthreadinfo_add((char *)__func__);
	jv_ao_mute(0);//open speaker
	int fd = open(fname,O_RDONLY);
	if(fd < 0)
	{
		jv_ao_mute(1);
		printf("open audio file failed!\n");
		return;
	}

	int len = 0;
	jv_audio_frame_t frame;
	static BOOL bFlag = FALSE;
	jv_audio_attr_t aoAttr;
	jv_ai_get_attr(0, &aoAttr);
	jv_audio_enc_type_e encType;
	
	//check audio enc
	if(!bFlag && (strstr(fname,"pcm") == NULL))
	{
		encType = aoAttr.encType;
		if (aoAttr.encType != JV_AUDIO_ENC_G711_A)
		{
			aoAttr.encType = JV_AUDIO_ENC_G711_A;
			jv_ao_stop(0);
			jv_ao_start(0, &aoAttr);
		}

		bFlag = TRUE;
	}
	int Datalen = 0;
	Datalen = (strstr(fname,"pcm") == NULL) ? 320 : 882;

	while(audio_group.running)
	{
		len = read(fd,frame.aData,Datalen);
		if(len <= 0)
		{
			break;
		}
		frame.u32Len = len;
		++nFrameCnt;

		ret = jv_ao_send_frame(0, &frame);
		if(ret < 0)
		{
			break;
		}
	}
	close(fd);

	minPlayTime = nFrameCnt * ((strstr(fname, "pcm") == NULL) ? 40 : 10);
	/*printf("read finish, frame: %d, min play time: %d\n", nFrameCnt, minPlayTime);*/

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100) // 18EV200/16EV100 SDK有变更，需要特殊处理
	//adec需要告知一下音频送入完毕
	jv_audio_attr_t ao_attr;
	jv_ao_get_attr(0, &ao_attr);
	if (ao_attr.encType != JV_AUDIO_ENC_PCM)
	{
		jv_ao_adec_end();
	}
#endif

	/*等待音频输出播放完成或者超时*/
	int getAOStatusCnt = 0;
	jv_ao_status aoStatus;

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100) // 18EV200/16EV100 SDK有变更，需要特殊处理
	// 等待播放开始
	do
	{
		usleep(10 * 1000);
		jv_ao_get_status(0, &aoStatus);
		getAOStatusCnt++;
		if (getAOStatusCnt > 80)
			break;
	}
	while (aoStatus.aoBufBusyNum == 0);
#endif

	// sleep最短播放时间，确保音频能播放完整
	usleep(minPlayTime * 1000);

	// 等待播放结束
	getAOStatusCnt = 0;
	do
	{
		usleep(100 * 1000);
		jv_ao_get_status(0, &aoStatus);
		getAOStatusCnt++;
		if (getAOStatusCnt > 100)
		{
			AUDIO_DEBUG("jv_ao_get_status timeout\n");
			break;
		}
	}
	while (aoStatus.aoBufBusyNum != 0);

	if (speakerowerStatus != JV_SPEAKER_OWER_ALARMING)	//aoAttr.encType != encType && 
	{
		jv_ai_get_attr(0, &aoAttr);

		jv_ao_stop(0);
		jv_ao_start(0, &aoAttr);
		bFlag = FALSE;
	}

	//播放声音或者声波配置，都要关闭功放
	if(speakerowerStatus <= JV_SPEAKER_OWER_ALARMING || speakerowerStatus == JV_SPEAKER_OWER_VOICE)
		jv_ao_mute(1);//close speaker

	bAudioEnding = TRUE;
	bPreRunning  = FALSE;
	
	audio_group.running = FALSE;
	
	AUDIO_DEBUG("_speak_process exit\n");
}

void maudio_setDefault_gain(jv_audio_attr_t* ai_attr)
{
	if(PRODUCT_MATCH("H411") || 
		PRODUCT_MATCH("J2000IP-CmPTZ-111-V2.0") ||
		PRODUCT_MATCH("H411C") || 
		PRODUCT_MATCH("WHHT") || 
		PRODUCT_MATCH("H411KEDA") || 
		PRODUCT_MATCH("AT-15H2") || 
		PRODUCT_MATCH("AJL-H40610-S1") || 
		PRODUCT_MATCH("AJL-H40610-S2") || 
		PRODUCT_MATCH("JD-H40810") || 
		PRODUCT_MATCH("SW-H411V4"))
	{
		ai_attr->level = 0x02;
	}
	else if(PRODUCT_MATCH("HZD-600DM") ||
		PRODUCT_MATCH("HZD-600DN"))
	{
		ai_attr->level = 0x2;
	}
	else if (PRODUCT_MATCH("H301") || 
		PRODUCT_MATCH("H303") || 
		PRODUCT_MATCH("A1") || 
		PRODUCT_MATCH("HW-H21110"))
	{
		ai_attr->level = 0x07;
	}
	else if(PRODUCT_MATCH("HA320-H1") ||
			PRODUCT_MATCH("HA320-H1-A") ||
			PRODUCT_MATCH("HV120-H1") ||
			PRODUCT_MATCH("HA121-H2"))
	{
		ai_attr->level = 0x0D;
	}
	else if(PRODUCT_MATCH("H411S-H1") || 
			PRODUCT_MATCH("H411V2") ||
			PRODUCT_MATCH("HC420S-H2") || 
			PRODUCT_MATCH("HC420S-Q1") ||
			PRODUCT_MATCH("SW-H411V3"))
	{
		ai_attr->level = 0x0E;
	}
	else if(PRODUCT_MATCH("H411V1_1") ||
			PRODUCT_MATCH("VS-DPCW-122") ||
			PRODUCT_MATCH("H210-H1") ||
			PRODUCT_MATCH("H210V2") ||
			PRODUCT_MATCH("H210V2-BR") ||
			PRODUCT_MATCH("H51X") ||
			PRODUCT_MATCH("H411-H1") ||
			PRODUCT_MATCH("HA520D-H1") ||
			PRODUCT_MATCH("HC520D-H1"))
	{
		ai_attr->level = 0x02;
	}
	else if(PRODUCT_MATCH("HC420-H2"))
	{
		ai_attr->level = 0x09;
	}
	else if(PRODUCT_MATCH("H210V2-S") ||
			PRODUCT_MATCH("H210") ||
			PRODUCT_MATCH("VISTAWIFI") ||
			PRODUCT_MATCH("A14-PC7000-HD1") ||
			PRODUCT_MATCH("MF-H20510-S1") ||
			PRODUCT_MATCH("DR-H20910"))
	{
		ai_attr->level = 0x0c;
	}
	else if(PRODUCT_MATCH("HA220-H2") || 
			PRODUCT_MATCH("HA220-H2-A"))
	{
		ai_attr->level = 0x06;
	}
	else if (PRODUCT_MATCH(PRODUCT_C3A) ||
			PRODUCT_MATCH("HC530A"))
	{
		ai_attr->level = 0x0a;
	}
	else if (HWTYPE_MATCH(HW_TYPE_C5) ||
			HWTYPE_MATCH(HW_TYPE_V7) ||
			HWTYPE_MATCH(HW_TYPE_HA210) ||
			HWTYPE_MATCH(HW_TYPE_HA230) ||
			HWTYPE_MATCH(HW_TYPE_C3) ||
			HWTYPE_MATCH(HW_TYPE_C3W) ||
			HWTYPE_MATCH(HW_TYPE_C9))
	{
		ai_attr->level = 0x0D;
		ai_attr->soundfile_level = 0xf;
	}
	else if (HWTYPE_MATCH(HW_TYPE_A4))
	{
		ai_attr->level = 0x0a;
		ai_attr->soundfile_level = 0x0c;
	}
	else if (
			HWTYPE_MATCH(HW_TYPE_V8) ||
			HWTYPE_MATCH(HW_TYPE_C8A) ||
			HWTYPE_MATCH(HW_TYPE_C8S) ||
			HWTYPE_MATCH(HW_TYPE_C8H) ||
			HWTYPE_MATCH(HW_TYPE_C8)
			)
	{
		ai_attr->level = 0x0D;
		ai_attr->soundfile_level = 0x11;//17
	}
	else if(
			HWTYPE_MATCH(HW_TYPE_V3) ||
			HWTYPE_MATCH(HW_TYPE_V6)
			)
	{
	    ai_attr->level = 0x0f;
		ai_attr->soundfile_level = 0x11;//17
	}
	else if (PRODUCT_MATCH("HXBJRB"))
	{
		ai_attr->level = 0x00;
		ai_attr->soundfile_level = 0x00;//17
	}
	else
	{
		ai_attr->level = 0x02;
	}

	if(PRODUCT_MATCH("H411") || 
		PRODUCT_MATCH("J2000IP-CmPTZ-111-V2.0") ||
		PRODUCT_MATCH("H411KEDA") || 
		PRODUCT_MATCH("AT-15H2") || 
		PRODUCT_MATCH("HZD-600DM") || 
		PRODUCT_MATCH("HZD-600DN") || 
		PRODUCT_MATCH("AJL-H40610-S1") || 
		PRODUCT_MATCH("AJL-H40610-S2") || 
		PRODUCT_MATCH("JD-H40810") || 
		PRODUCT_MATCH("SW-H411V4"))
	{
		ai_attr->micGain = 23*3; 		//utl_system("himm 0x20050068 0x00025E5E");
	}
	else if(PRODUCT_MATCH("H411S-H1") || 
			PRODUCT_MATCH("H411V2") ||
			PRODUCT_MATCH("HC420S-H2") || 
			PRODUCT_MATCH("HC420S-Q1") ||
			PRODUCT_MATCH("HA520D-H1") ||
			PRODUCT_MATCH("HC520D-H1") ||
			PRODUCT_MATCH("HC420-H2") ||
			PRODUCT_MATCH("H411-H1") ||
			PRODUCT_MATCH("H411V1_1") || 
			PRODUCT_MATCH("VS-DPCW-122") ||
			PRODUCT_MATCH("SW-H411V3") ||
			PRODUCT_MATCH("HA220-H2") ||
			PRODUCT_MATCH("HA220-H2-A") ||
			PRODUCT_MATCH("H51X"))
	{
		ai_attr->micGain = 21*3; 		//utl_system("himm 0x20050068 0x00025656");
	}
	else if(PRODUCT_MATCH("YL") || 
		PRODUCT_MATCH("SML-H41210") || 
		PRODUCT_MATCH("JW-501DM") || 
		PRODUCT_MATCH("JW-502DM") || 
		PRODUCT_MATCH("JW-503DM"))
	{
		ai_attr->micGain = 0x1B*3;	 // 24db + 26db
	}
	else if (PRODUCT_MATCH("HA320-H1") ||
			PRODUCT_MATCH("HA320-H1-A") ||
			PRODUCT_MATCH("HV120-H1")  ||
			PRODUCT_MATCH("HA121-H2"))
	{
		ai_attr->micGain = 0x15*3;		//15db+26db	5656
	}
	else if(PRODUCT_MATCH("H210V2-S") ||
		PRODUCT_MATCH("H210-H1") ||
		PRODUCT_MATCH("H301") ||
		PRODUCT_MATCH("A1") ||
		PRODUCT_MATCH("H210") ||
		PRODUCT_MATCH("VISTAWIFI") ||
		PRODUCT_MATCH("A14-PC7000-HD1") ||
		PRODUCT_MATCH("H210V2") ||
		PRODUCT_MATCH("H210V2-BR") ||
		PRODUCT_MATCH("MF-H20510-S1") ||
		PRODUCT_MATCH("DR-H20910") ||
		PRODUCT_MATCH("HW-H21110"))
	{
		ai_attr->micGain = 0x11*3; 		//9db+26db     4646
	}
	else if (
			HWTYPE_MATCH(HW_TYPE_HA210) ||
			HWTYPE_MATCH(HW_TYPE_HA230) ||
			HWTYPE_MATCH(HW_TYPE_A4) ||
			HWTYPE_MATCH(HW_TYPE_C5) ||
			HWTYPE_MATCH(HW_TYPE_C8A) ||
			HWTYPE_MATCH(HW_TYPE_C8) ||
			HWTYPE_MATCH(HW_TYPE_C8S) ||
			HWTYPE_MATCH(HW_TYPE_C8H) ||
			HWTYPE_MATCH(HW_TYPE_C9) ||
			HWTYPE_MATCH(HW_TYPE_V3) ||
			HWTYPE_MATCH(HW_TYPE_V6) ||
			HWTYPE_MATCH(HW_TYPE_V7) ||
			HWTYPE_MATCH(HW_TYPE_V8)
			)
	{
		ai_attr->micGain = 0x0A;		//40db
		ai_attr->micGainTalk = 0x05;	//30db
	}
	else if(HWTYPE_MATCH(HW_TYPE_C3) ||
			HWTYPE_MATCH(HW_TYPE_C3W))
	{
		ai_attr->micGain = 0x0A;		//40db
		ai_attr->micGainTalk = 0x03;	//26db
	}
	else if (PRODUCT_MATCH(PRODUCT_C3A)
			|| PRODUCT_MATCH("HC530A"))
	{
		ai_attr->micGain = 0x0A;
	}
	else if (PRODUCT_MATCH("HXBJRB"))
	{
		ai_attr->micGain = 0x0A;
	}
	else
	{
		ai_attr->micGain = 21*3;
	}

	return;	
}

/**
*@brief 
*初始化，初始化线程锁,根据AI音频输入参数使能音频输入，
*根据AO音频输出参数使能音频输出
*/
int maudio_init()
{
	jv_audio_attr_t ai_attr;
	jv_audio_attr_t ao_attr;
	jv_ai_get_attr(0, &ai_attr);

	//请注意，音频编码类型可能在SYSFuncs.c中被修改
	ai_attr.encType = JV_AUDIO_ENC_G711_A;
	ai_attr.bitWidth = JV_AUDIO_BIT_WIDTH_16;
	ai_attr.sampleRate = JV_AUDIO_SAMPLE_RATE_8000;
	ao_attr.encType = JV_AUDIO_ENC_G711_A;
	ao_attr.bitWidth = JV_AUDIO_BIT_WIDTH_16;
	ao_attr.sampleRate = JV_AUDIO_SAMPLE_RATE_8000;
	
	maudio_setDefault_gain(&ai_attr);

	printf("Start AI and AO\n");
	memcpy(&ao_attr, &ai_attr, sizeof(jv_audio_attr_t));
	
	jv_ai_start(0, &ai_attr);
	jv_ao_start(0, &ao_attr);
	
	pthread_mutex_init(&audio_group.mutex, NULL);
	pthread_mutex_init(&soundmode_mutex, NULL);
	audio_group.running = FALSE;
	AUDIO_DEBUG("maudio_init ok\n");

	return 0;
}
/**
*@brief 结束，销毁线程锁,禁止音频输入输出
*/
int maudio_deinit(int aiChannelId, int aoChannelId)
{
	if(aiChannelId != -1)
		jv_ai_stop(aiChannelId);
	if(aoChannelId != -1)
		jv_ao_stop(aoChannelId);
	pthread_mutex_destroy(&audio_group.mutex);
	AUDIO_DEBUG("maudio_deinit ok\n");
	return 0;
}

/**
*@brief 根据语言管理提示音文件夹
*/

char*	voice_lan[LANGUAGE_MAX] = {"cn","en"};

void maudio_selFile_byLanguage(char *fname)
{
	char tempname1[128] = {0};
	char tempname2[128] = {0};
	
	char* pos = strrchr(fname,'/');

	strcpy(tempname2,pos+1);
	
	strncpy(tempname1,fname,pos-fname+1);

	ImportantCfg_t im;
	memset(&im, 0, sizeof(ImportantCfg_t));
	
	ReadImportantInfo(&im);

	sprintf(fname,"%s%s/%s",tempname1,voice_lan[im.nLanguage],tempname2);
}

/**
*@brief 播放PCM音频文件
*/
int maudio_speaker(const char *fname, BOOL bTransByLan,BOOL bPlayNow, BOOL bBlock)
{
	pthread_mutex_lock(&audio_group.mutex);
	jv_audio_attr_t ao_attr;
	jv_ao_get_attr(0, &ao_attr);
	static char tempFname[128] = {0};
	if(NULL == fname)
	{
		pthread_mutex_unlock(&audio_group.mutex);
		return -1;
	}
	strcpy(tempFname,fname);

	if(bTransByLan)
	{	
		maudio_selFile_byLanguage(tempFname);	
	}

	if(access(tempFname, 0) != 0)
	{
		pthread_mutex_unlock(&audio_group.mutex);
		return -1;
	}
	if(audio_group.running)
	{
//	 	if(bPlayNow)
	 	{
			AUDIO_DEBUG("stop maudio_speaker\n");
			audio_group.running = FALSE;
	 	}
		pthread_join(audio_group.thread, NULL);
	}
	
	if(ao_attr.soundfile_level != -1)
	{
		jv_ao_ctrl(ao_attr.soundfile_level);
	}
	AUDIO_DEBUG("start maudio_speaker\n");
	audio_group.running = TRUE;
	bAudioEnding = FALSE;
	pthread_create_normal(&audio_group.thread, NULL, (void *)_speak_process, (void*)tempFname);
	if (bBlock == FALSE)
	{
		pthread_detach(audio_group.thread);
	}
	else
	{
		pthread_join(audio_group.thread, NULL);
	}
	pthread_mutex_unlock(&audio_group.mutex);

	if(ao_attr.soundfile_level != -1)
	{
		jv_ao_ctrl(ao_attr.level);
	}
	return 0;
}

int maudio_speaker_stop()
{
	if(audio_group.running)
	{
		audio_group.running = FALSE;

		bPreRunning = TRUE;
		
		pthread_join(audio_group.thread, NULL);

	}

	return 0;
}

BOOL maudio_speaker_GetEndingFlag()
{
	if(!bPreRunning)
		return TRUE;
	
	return bAudioEnding;
}

void maudio_readfiletoao(char* FilePath)
{
	//return;				//声音文件还未准备好，暂时先不播放声音
	jv_audio_frame_t frame;

	int fd = -1;
	char chBuf[640] = {0};
	int len = 0;
	
	fd = open(FilePath,O_RDONLY);
	if(fd < 0)	
	{	 
		printf("open file %s failed!\n",FilePath);
		return;
	}

	jv_audio_attr_t ao_attr;
	jv_ao_get_attr(0, &ao_attr);
	if(ao_attr.soundfile_level != -1)
	{
		jv_ao_ctrl(ao_attr.soundfile_level);
	}

	while(1)
	{
		len = read(fd,chBuf, 320);
		if(len <= 0)
		{			
			Printf("file goes to end!! \n");
			break;
		}
		
		memcpy(frame.aData, chBuf, 320);
		frame.u32Len = 320;
		//播放接收到的音频帧
		jv_ao_send_frame(0,&frame);			
	}

	close(fd);

	/*等待音频输出播放完成或者超时*/
	int getAOStatusCnt = 0;
	jv_ao_status aoStatus;
	jv_ao_get_status(0, &aoStatus);
	while(aoStatus.aoBufBusyNum > 1)
	{
		jv_ao_get_status(0, &aoStatus);
		getAOStatusCnt++;
		if (getAOStatusCnt > 100)
		{
			AUDIO_DEBUG("jv_ao_get_status timeout\n");
			break;
		}
		usleep(100*1000);
	}

	if(ao_attr.soundfile_level != -1)
	{
		jv_ao_ctrl(ao_attr.level);
	}
}

/*******************************
*重新设置AI 和AO,因为声波配置跟声音播放的编码方式和采样率不一样
*mode: 1 (声波配置)  2 (声音播放)
********************************/
void maudio_resetAIAO_mode(int mode)
{
	if (!hwinfo.bSupportVoiceConf)
		return;

	pthread_mutex_lock(&soundmode_mutex);
	static int curMode = 0;
	if(curMode == mode)
	{
		pthread_mutex_unlock(&soundmode_mutex);
		return;
	}

	jv_ai_stop(0);
	jv_ao_stop(0);

	jv_audio_sample_rate_e wave_hz = JV_AUDIO_SAMPLE_RATE_24000;
#ifdef SOUND_WAVE_DEC_SUPPORT
	if(hwinfo.bNewVoiceDec)
		wave_hz = JV_AUDIO_SAMPLE_RATE_44100;
#endif
	
	jv_audio_attr_t aiAttr;
	jv_ai_get_attr(0,&aiAttr);
	if(1 == mode)
		aiAttr.encType = JV_AUDIO_ENC_PCM;
	else
		aiAttr.encType = JV_AUDIO_ENC_G711_A;
	aiAttr.bitWidth = JV_AUDIO_BIT_WIDTH_16;
	if(1 == mode)
		aiAttr.sampleRate = wave_hz;
	else
		aiAttr.sampleRate = JV_AUDIO_SAMPLE_RATE_8000;
	jv_ai_start(0, &aiAttr);
	
	jv_audio_attr_t aoAttr;
	jv_ai_get_attr(0,&aoAttr);
	if(1 == mode)
		aoAttr.encType = JV_AUDIO_ENC_PCM;
	else
		aoAttr.encType = JV_AUDIO_ENC_G711_A;
	aoAttr.bitWidth = JV_AUDIO_BIT_WIDTH_16;

	if(1 == mode)
		aoAttr.sampleRate = wave_hz;
	else
		aoAttr.sampleRate = JV_AUDIO_SAMPLE_RATE_8000;
	jv_ao_start(0, &aoAttr);

	curMode = mode;
    pthread_mutex_unlock(&soundmode_mutex);

}



