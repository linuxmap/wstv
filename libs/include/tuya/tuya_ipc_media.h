#ifndef _TUYA_IPC_MEDIA_H_
#define _TUYA_IPC_MEDIA_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"
#include "tuya_cloud_types.h"


typedef enum
{
    E_CHANNEL_VIDEO_MAIN = 0,    
    E_CHANNEL_VIDEO_SUB,    
    E_CHANNEL_AUDIO,
    E_CHANNEL_MAX
}CHANNEL_E;

typedef enum
{
    E_VIDEO_PB_FRAME = 0,
    E_VIDEO_I_FRAME,
    E_VIDEO_TS_FRAME,
    E_AUDIO_FRAME,
    E_MEDIA_FRAME_TYPE_MAX
}MEDIA_FRAME_TYPE_E;


typedef enum
{
    CODEC_VIDEO_MPEG4 = 0,
    CODEC_VIDEO_H263,
    CODEC_VIDEO_H264,
    CODEC_VIDEO_MJPEG,
    CODEC_VIDEO_H265,

    CODEC_AUDIO_ADPCM,
    CODEC_AUDIO_PCM,
    CODEC_AUDIO_AAC_RAW,
    CODEC_AUDIO_AAC_ADTS,
    CODEC_AUDIO_AAC_LATM,
    CODEC_AUDIO_G711U,
    CODEC_AUDIO_G711A,
    CODEC_AUDIO_G726,
    CODEC_AUDIO_SPEEX,
    CODEC_AUDIO_MP3,

    CODEC_INVALID
}TUYA_CODEC_ID;

typedef enum
{
    AUDIO_SAMPLE_8K,
}AUDIO_SAMPLE_E;

typedef enum
{
    AUDIO_DATABITS_8,
    AUDIO_DATABITS_16,
}AUDIO_DATABITS_E;

typedef enum
{
    AUDIO_CHANNEL_MONO,
    AUDIO_CHANNEL_STERO,
}AUDIO_CHANNEL_E;


typedef struct
{
    BOOL channel_enable[E_CHANNEL_MAX];

    UINT video_fps;
    UINT video_gop;
    UINT video_bitrate;
    UINT video_main_width;
    UINT video_main_height;
    UINT video_freq;
    TUYA_CODEC_ID video_codec;

    TUYA_CODEC_ID audio_codec;
    AUDIO_SAMPLE_E audio_sample;
    AUDIO_DATABITS_E audio_databits;
    AUDIO_CHANNEL_E audio_channel;

    UINT max_key_frame_in_one_file;
}IPC_MEDIA_INFO_S;

typedef struct
{
    MEDIA_FRAME_TYPE_E type;
    BYTE    *p_buf;
    UINT    size;
    UINT64  pts;
    UINT64  timestamp;
}MEDIA_FRAME_S;



#ifdef __cplusplus
}
#endif

#endif /*_TUYA_IPC_MEDIA_H_*/
