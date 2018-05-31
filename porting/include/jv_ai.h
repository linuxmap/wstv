#ifndef _JV_AI_H_
#define _JV_AI_H_
#include <jv_common.h>
#define JV_MAX_AUDIO_FRAME_LEN		(10*1024)


typedef enum
{
    JV_AUDIO_SAMPLE_RATE_8000   = 8000,    /* 8K samplerate*/
    JV_AUDIO_SAMPLE_RATE_11025  = 11025,   /* 11.025K samplerate*/
    JV_AUDIO_SAMPLE_RATE_16000  = 16000,   /* 16K samplerate*/
    JV_AUDIO_SAMPLE_RATE_22050  = 22050,   /* 22.050K samplerate*/
    JV_AUDIO_SAMPLE_RATE_24000  = 24000,   /* 24K samplerate*/
    JV_AUDIO_SAMPLE_RATE_32000  = 32000,   /* 32K samplerate*/
    JV_AUDIO_SAMPLE_RATE_44100  = 44100,   /* 44.1K samplerate*/
    JV_AUDIO_SAMPLE_RATE_48000  = 48000,   /* 48K samplerate*/
    JV_AUDIO_SAMPLE_RATE_BUTT,
} jv_audio_sample_rate_e;

typedef enum 
{
    JV_AUDIO_BIT_WIDTH_8   = 0,   /* 8bit width */
    JV_AUDIO_BIT_WIDTH_16  = 1,   /* 16bit width*/
    JV_AUDIO_BIT_WIDTH_32  = 2,   /* 32bit width*/
    JV_AUDIO_BIT_WIDTH_BUTT,
} jv_audio_bit_width_e;

typedef enum
{
	JV_AUDIO_ENC_PCM,
	JV_AUDIO_ENC_G711_A,
	JV_AUDIO_ENC_G711_U,
	JV_AUDIO_ENC_G726_16K, // not support
	JV_AUDIO_ENC_G726_24K, // not support
	JV_AUDIO_ENC_G726_32K, // not support
	JV_AUDIO_ENC_G726_40K,
	JV_AUDIO_ENC_ADPCM,
	JV_AUDIO_ENC_AMR,
	JV_AUDIO_ENC_AAC,
}jv_audio_enc_type_e;

typedef enum
{
	JV_SPEAKER_OWER_NONE,
	JV_SPEAKER_OWER_ALARMING,
	JV_SPEAKER_OWER_CHAT,
	JV_SPEAKER_OWER_VOICE,
}jv_speaker_ower_e;

jv_speaker_ower_e speakerowerStatus;

typedef struct
{
	jv_audio_sample_rate_e sampleRate;
	jv_audio_bit_width_e bitWidth;
	jv_audio_enc_type_e encType;
	//在原有结构的基础上，加上客户想要的功能
	//BOOL echo;  			//回音消除 (1:enabled; 0:disabled)
	int level;  			//喇叭音量 (0-100)
	int soundfile_level;    //播放声音文件音量
	int micGain;		  	//麦克风增益Microphone gain(0-100)	监听和声波使用
	int micGainTalk;		//麦克风增益，全双工对讲使用
}jv_audio_attr_t;

typedef struct
{
	unsigned char aData[JV_MAX_AUDIO_FRAME_LEN];
	U64 u64TimeStamp;                /*audio frame timestamp*/
	U32 u32Seq;                      /*audio frame seq*/
	U32 u32Len;                      /*data lenth per channel in frame*/
} jv_audio_frame_t;

#define BUF_SZ (320*2*10+1)
typedef struct
{
    unsigned char buf[BUF_SZ];
    unsigned int readPos;
    unsigned int writePos;
    //unsigned int size;
}BUF_RING;

/**
 *@brief 打开音频采集
 *
 *@param channelid 音频通道号
 *@param attr 音频通道的设置参数
 *
 *@return 0 成功
 *
 */
int jv_ai_start(int channelid, jv_audio_attr_t *attr);

/**
 *@brief 获取音频相关设置
 *
 *@param channelid 音频通道号
 *@param attr 音频通道的设置参数
 *
 *@return 0 成功
 *
 */
int jv_ai_get_attr(int channelid, jv_audio_attr_t *attr);

/**
 *@brief 设置音频参数
 *
 *@param channelid 音频通道号
 *@param attr 音频通道的设置参数
 *
 *@return 0 成功
 *
 */
int jv_ai_set_attr(int channelid, jv_audio_attr_t *attr);

/**
 *@brief 关闭音频采集
 *
 *@param channelid 音频通道号
 *
 *@return 0 成功
 *
 */
int jv_ai_stop(int channelid);

/**
 *@brief 获取一帧音频数据
 *
 *@param channelid 音频通道号
 *@param frame 音频帧
 *
 *@return 0 正确的获取到了数据， -1，无数据
 *
 *@note 对于IPC来说，只有一路音频。但却有多路视频。
 * 为了应对这种情况，本函数每个通道都可以获取到一份数据
 *
 */
int jv_ai_get_frame(int channelid, jv_audio_frame_t *frame);

typedef void (*jv_ai_pcm_callback_t)(int channelid, jv_audio_frame_t *frame);

/**
 *@brief 注册回调函数，提供音频的PCM帧给上层应用
 *
 *@param channelid 音频通道号
 *@param callback_ptr 回调函数指针
 */
int jv_ai_set_pcm_callback( int channelid, jv_ai_pcm_callback_t callback_ptr);

 /*
  * 设置语音对讲状态，指明当先是否在对讲状态
  */
void jv_ai_setChatStatus(BOOL bChat);
BOOL jv_ai_GetChatStatus();

int jv_ai_RestartAec(int chan,BOOL bSwitch);
int jv_ai_RestartAnr(int channel,BOOL bSwitch);

int jv_ai_SetMicgain(int gainval);
int jv_ai_SetMicMute(int mute);

#endif

