/*
 * sp_audio.h
 *
 *  Created on: 2013Äê12ÔÂ17ÈÕ
 *      Author: lfx  20451250@qq.com
 */
#ifndef SP_AUDIO_H_
#define SP_AUDIO_H_


typedef enum
{
    SP_AUDIO_SAMPLE_RATE_8000   = 8000,    /* 8K samplerate*/
    SP_AUDIO_SAMPLE_RATE_11025  = 11025,   /* 11.025K samplerate*/
    SP_AUDIO_SAMPLE_RATE_16000  = 16000,   /* 16K samplerate*/
    SP_AUDIO_SAMPLE_RATE_22050  = 22050,   /* 22.050K samplerate*/
    SP_AUDIO_SAMPLE_RATE_24000  = 24000,   /* 24K samplerate*/
    SP_AUDIO_SAMPLE_RATE_32000  = 32000,   /* 32K samplerate*/
    SP_AUDIO_SAMPLE_RATE_44100  = 44100,   /* 44.1K samplerate*/
    SP_AUDIO_SAMPLE_RATE_48000  = 48000,   /* 48K samplerate*/
    SP_AUDIO_SAMPLE_RATE_BUTT,
} SPAudioSamplerate_e;

typedef enum
{
    SP_AUDIO_BIT_WIDTH_8   = 0,   /* 8bit width */
    SP_AUDIO_BIT_WIDTH_16  = 1,   /* 16bit width*/
    SP_AUDIO_BIT_WIDTH_32  = 2,   /* 32bit width*/
    SP_AUDIO_BIT_WIDTH_BUTT,
} SPAudioBitwidth_e;

typedef enum
{
	SP_AUDIO_ENC_PCM,
	SP_AUDIO_ENC_G711_A,
	SP_AUDIO_ENC_G711_U,
	SP_AUDIO_ENC_G726_16K, // not support
	SP_AUDIO_ENC_G726_24K, // not support
	SP_AUDIO_ENC_G726_32K, // not support
	SP_AUDIO_ENC_G726_40K,
	SP_AUDIO_ENC_AAC,
	SP_AUDIO_ENC_ADPCM,
	SP_AUDIO_ENC_AC3
}SPAudioEnctype_e;

typedef struct
{
	SPAudioSamplerate_e sampleRate;
	SPAudioBitwidth_e bitWidth;
	SPAudioEnctype_e encType;
}SPAudioAttr_t;


#ifdef __cplusplus
extern "C" {
#endif

int sp_audio_get_param(int channelid, SPAudioAttr_t *attr);

int sp_audio_set_param(int channelid, SPAudioAttr_t *attr);

void sp_audio_send_out_frame(unsigned char *data, unsigned int len);

void sp_audio_ao_mute(unsigned char bmute);

void sp_audio_StopAi();

void sp_audio_Stopao();

void sp_audio_AiStartCtrl(int ctrlMode);

void sp_audio_AiSetVolume(int vol);

void sp_audio_AoSetVolume(int vol);

int sp_audio_AecSwitch(int bSwitch);

int sp_audio_SetaiGain(int gainval);


#ifdef __cplusplus
}
#endif

#endif /* SP_AUDIO_H_ */
