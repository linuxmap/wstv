/*********************************************************************************
  *Copyright(C),2015-2020, 涂鸦科技 www.tuya.comm
  *FileName: tuya_ipc_media_handler.c
  *
  * 文件描述：
  * 1. 音视频函数API实现
  *
  * 开发者工作：
  * 1. 设置本地音视频属性。
  * 2. 实现对讲模式所需要的API。
  *
**********************************************************************************/
#include <string.h>
#include <stdio.h>

#include "tuya_ipc_media_handler.h"
#include "mstream.h"


/* 设置本地音视频属性 */
VOID IPC_APP_Register_Media_Info_CB(INOUT IPC_MEDIA_INFO_S *p_media_info)
{
	mstream_attr_t stAttr;
	mstream_get_param(0, &stAttr);
    if(p_media_info == NULL)
    {
        return;
    }
    memset(p_media_info, 0 , sizeof(IPC_MEDIA_INFO_S));

    p_media_info->channel_enable[E_CHANNEL_VIDEO_MAIN] = TRUE;    /* 是否开启本地高清视频流 */
    p_media_info->channel_enable[E_CHANNEL_VIDEO_SUB] = TRUE;     /* 是否开启本地标清视频流 */
    p_media_info->channel_enable[E_CHANNEL_AUDIO] = TRUE;         /* 是否开启本地声音采集 */

    p_media_info->video_fps = stAttr.framerate;  /* 视频FPS */
    p_media_info->video_gop = stAttr.nGOP_S;  /* 视频GOP */
    p_media_info->video_bitrate = stAttr.bitrate; /* 高清视频流 码率 */
    p_media_info->video_main_width = 640; /* 高清视频流 宽 */
    p_media_info->video_main_height = 360;/* 高清视频流 高 */
    p_media_info->video_freq = 90000; /* 视频流 时钟频率 */
    p_media_info->video_codec = CODEC_VIDEO_H264; /* 视频流编码格式 */


    p_media_info->audio_codec = CODEC_AUDIO_PCM;/* 音频流编码格式 */
    p_media_info->audio_sample = AUDIO_SAMPLE_8K;/* 音频流采样率 */
    p_media_info->audio_databits = AUDIO_DATABITS_16;/* 音频流位宽 */
    p_media_info->audio_channel = AUDIO_CHANNEL_MONO;/* 音频流信道 */

    p_media_info->max_key_frame_in_one_file = 900;/* 分割文件时文件内最大关键帧个数 */
}

/* 对讲模式声音回调，开启关闭扬声器硬件 */
VOID TUYA_APP_Enable_Speaker_CB(BOOL enabled)
{
    printf("enable speaker %d \r\n", enabled);
    //TODO
    /* 开发者需要实现开启关闭扬声器硬件操作，如果IPC硬件特性无需显式开启，该函数留空即可 */
}

/* 对讲模式声音回调，开启关闭扬声器硬件 */
VOID TUYA_APP_Rev_Audio_CB(IN CONST MEDIA_FRAME_S *p_audio_frame,
                           AUDIO_SAMPLE_E audio_sample,
                           AUDIO_DATABITS_E audio_databits,
                           AUDIO_CHANNEL_E audio_channel)
{
    printf("rev audio cb len:%u sample:%d db:%d channel:%d\r\n", p_audio_frame->size, audio_sample, audio_databits, audio_channel);
    //PCM-Format 8K 16Bit MONO
    //TODO
    /* 开发者需要实现扬声器播放声音操作 */

}


