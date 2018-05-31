/*
 * sp_audio.cpp
 *
 *  Created on: 2013年12月17日
 *      Author: lfx  20451250@qq.com
 */

#include "sp_define.h"
#include "sp_audio.h"
#include <jv_ai.h>
#include "jv_ao.h"
#include <SYSFuncs.h>
#include "mstream.h"

int sp_audio_get_param(int channelid, SPAudioAttr_t *attr)
{
	jv_audio_attr_t ja;
	jv_ai_get_attr(channelid, &ja);
	switch(ja.encType)
	{
	default:
		Printf("ERROR: audio format not support: %d\n", attr->encType);
		break;
	case JV_AUDIO_ENC_PCM:
		attr->encType = SP_AUDIO_ENC_PCM;
		break;
	case JV_AUDIO_ENC_ADPCM:
		attr->encType = SP_AUDIO_ENC_ADPCM;
		break;
	case JV_AUDIO_ENC_G711_A:
		attr->encType = SP_AUDIO_ENC_G711_A;
		break;
	case JV_AUDIO_ENC_G711_U:
		attr->encType = SP_AUDIO_ENC_G711_U;
		break;
	case JV_AUDIO_ENC_G726_16K:
		attr->encType = SP_AUDIO_ENC_G726_16K;
		break;
	case JV_AUDIO_ENC_G726_24K:
		attr->encType = SP_AUDIO_ENC_G726_24K;
		break;
	case JV_AUDIO_ENC_G726_32K:
		attr->encType = SP_AUDIO_ENC_G726_32K;
		break;
	case JV_AUDIO_ENC_G726_40K:
		attr->encType = SP_AUDIO_ENC_G726_40K;
		break;
	}

	switch(ja.sampleRate)
	{
	default:
	case JV_AUDIO_SAMPLE_RATE_8000:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_8000;
		break;
	case JV_AUDIO_SAMPLE_RATE_11025:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_11025;
		break;
	case JV_AUDIO_SAMPLE_RATE_16000:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_16000;
		break;
	case JV_AUDIO_SAMPLE_RATE_22050:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_22050;
		break;
	case JV_AUDIO_SAMPLE_RATE_24000:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_24000;
		break;
	case JV_AUDIO_SAMPLE_RATE_32000:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_32000;
		break;
	case JV_AUDIO_SAMPLE_RATE_44100:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_44100;
		break;
	case JV_AUDIO_SAMPLE_RATE_48000:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_48000;
		break;
	case JV_AUDIO_SAMPLE_RATE_BUTT:
		attr->sampleRate = SP_AUDIO_SAMPLE_RATE_BUTT;
		break;
	}

	switch (ja.bitWidth)
	{
	default:
	case JV_AUDIO_BIT_WIDTH_32:
		attr->bitWidth = SP_AUDIO_BIT_WIDTH_32;
		break;
	case JV_AUDIO_BIT_WIDTH_16:
		attr->bitWidth = SP_AUDIO_BIT_WIDTH_16;
		break;
	case JV_AUDIO_BIT_WIDTH_8:
		attr->bitWidth = SP_AUDIO_BIT_WIDTH_8;
		break;
	}
	return 0;
}

int sp_audio_set_param(int channelid, SPAudioAttr_t *attr)
{
	jv_audio_attr_t ja;
	jv_ai_get_attr(channelid, &ja);
	switch(attr->encType)
	{
	default:
		Printf("ERROR: audio format not support: %d\n", attr->encType);
		break;
	case SP_AUDIO_ENC_PCM:
		ja.encType = JV_AUDIO_ENC_PCM;
		break;
	case SP_AUDIO_ENC_ADPCM:
		ja.encType = JV_AUDIO_ENC_ADPCM;
		break;
	case SP_AUDIO_ENC_G711_A:
		ja.encType = JV_AUDIO_ENC_G711_A;
		break;
	case SP_AUDIO_ENC_G711_U:
		ja.encType = JV_AUDIO_ENC_G711_U;
		break;
	case SP_AUDIO_ENC_G726_16K:
		ja.encType = JV_AUDIO_ENC_G726_16K;
		break;
	case SP_AUDIO_ENC_G726_24K:
		ja.encType = JV_AUDIO_ENC_G726_24K;
		break;
	case SP_AUDIO_ENC_G726_32K:
		ja.encType = JV_AUDIO_ENC_G726_32K;
		break;
	case SP_AUDIO_ENC_G726_40K:
		ja.encType = JV_AUDIO_ENC_G726_40K;
		break;
	}

	switch(attr->sampleRate)
	{
	default:
	case SP_AUDIO_SAMPLE_RATE_8000:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_8000;
		break;
	case SP_AUDIO_SAMPLE_RATE_11025:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_11025;
		break;
	case SP_AUDIO_SAMPLE_RATE_16000:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_16000;
		break;
	case SP_AUDIO_SAMPLE_RATE_22050:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_22050;
		break;
	case SP_AUDIO_SAMPLE_RATE_24000:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_24000;
		break;
	case SP_AUDIO_SAMPLE_RATE_32000:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_32000;
		break;
	case SP_AUDIO_SAMPLE_RATE_44100:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_44100;
		break;
	case SP_AUDIO_SAMPLE_RATE_48000:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_48000;
		break;
	case SP_AUDIO_SAMPLE_RATE_BUTT:
		ja.sampleRate = JV_AUDIO_SAMPLE_RATE_BUTT;
		break;
	}

	switch (attr->bitWidth)
	{
	default:
	case SP_AUDIO_BIT_WIDTH_32:
		ja.bitWidth = JV_AUDIO_BIT_WIDTH_32;
		break;
	case SP_AUDIO_BIT_WIDTH_16:
		ja.bitWidth = JV_AUDIO_BIT_WIDTH_16;
		break;
	case SP_AUDIO_BIT_WIDTH_8:
		ja.bitWidth = JV_AUDIO_BIT_WIDTH_8;
		break;
	}

	mstream_audio_set_param(channelid, &ja);
	WriteConfigInfo();
	return 0;
}

void sp_audio_send_out_frame(unsigned char *data, unsigned int len)
{
	jv_audio_frame_t frame;
	memcpy(frame.aData, data, len);
	frame.u32Len = len;
	//播放接收到的音频帧
	jv_ao_send_frame(0,&frame);
}

void sp_audio_ao_mute(unsigned char bmute)
{
	jv_ao_mute(bmute);
}

void sp_audio_StopAi()
{
	jv_ai_stop(0);
}

void sp_audio_Stopao()
{
	jv_ao_stop(0);
}

void sp_audio_AiSetVolume(int vol)
{
	//暂时没有设置AI音量的接口
	return;
}

void sp_audio_AoSetVolume(int vol)
{
	jv_ao_ctrl(vol);
	return ;
}

int  sp_audio_AecSwitch(int bSwitch)
{
	return jv_ai_RestartAec(0,bSwitch);
}

int sp_audio_SetaiGain(int gainval)
{
	return jv_ai_SetMicgain(gainval);
}

