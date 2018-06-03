/*********************************************************************************
  *Copyright(C),2015-2020, 涂鸦科技 www.tuya.comm
  *FileName: tuya_ipc_media_utils.c
  *
  * 文件描述：
  * 1. 音视频工具API实现
  *
  * 本文件代码为工具代码，用户可以不用关心
  * 请勿随意修改该文件中的任何内容，如需修改请联系涂鸦产品经理！！
  *
**********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "es_to_ts.h"
#include "tuya_ipc_transfer.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_echo_show.h"
#include "tuya_ipc_media_utils.h"
#include "tuya_ipc_media_handler.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ring_buffer.h"

#define PR_ERR(fmt, ...)    printf("Err:"fmt"\r\n", ##__VA_ARGS__)
#define PR_DEBUG(fmt, ...)  printf("Dbg:"fmt"\r\n", ##__VA_ARGS__)
//#define PR_TRACE(fmt, ...)  printf("Trace:"fmt"\r\n", ##__VA_ARGS__)
#define PR_TRACE(fmt, ...)


STATIC IPC_MEDIA_INFO_S s_media_info = {{0}};
VOID IPC_APP_Refresh_Media_Info(VOID)
{
    IPC_APP_Register_Media_Info_CB(&s_media_info);

    PR_DEBUG("channel_enable:%d %d %d", s_media_info.channel_enable[0], s_media_info.channel_enable[1], s_media_info.channel_enable[2]);

    PR_DEBUG("fps:%u", s_media_info.video_fps);
    PR_DEBUG("gop:%u", s_media_info.video_gop);
    PR_DEBUG("bitrate:%u", s_media_info.video_bitrate);
    PR_DEBUG("video_main_width:%u", s_media_info.video_main_width);
    PR_DEBUG("video_main_height:%u", s_media_info.video_main_height);
    PR_DEBUG("video_freq:%u", s_media_info.video_freq);
    PR_DEBUG("video_codec:%d", s_media_info.video_codec);

    PR_DEBUG("audio_codec:%d", s_media_info.audio_codec);
    PR_DEBUG("audio_sample:%d", s_media_info.audio_sample);
    PR_DEBUG("audio_databits:%d", s_media_info.audio_databits);
    PR_DEBUG("audio_channel:%d", s_media_info.audio_channel);
}

/*
---------------------------------------------------------------------------------
Stream_Storage相关代码起始位置
---------------------------------------------------------------------------------
*/

typedef struct
{
    CHANNEL_E channel;
}STORAGE_THREAD_ARG;

STATIC STORAGE_THREAD_ARG s_video_thread_arg = {0};
STATIC STORAGE_THREAD_ARG s_audio_thread_arg = {0};

void *thread_stream_storage(void *arg)
{
    MEDIA_FRAME_S frame = {0};
    BOOL is_retry = FALSE;
    USER_INDEX_E user = E_USER_STREAM_STORAGE;
    OPERATE_RET ret;
    STORAGE_THREAD_ARG *thread_arg = (STORAGE_THREAD_ARG *)arg;
    CHANNEL_E channel = thread_arg->channel;

    PR_DEBUG("stream_storage thread start. channel:%d", channel);

    while(TRUE)
    {
        TUYA_APP_Get_Frame(channel, user, is_retry, &frame);
        if(channel == E_CHANNEL_VIDEO_MAIN || channel == E_CHANNEL_VIDEO_SUB)
        {
            ret = tuya_ipc_ss_put_video(&frame);
        }
        else if(channel == E_CHANNEL_AUDIO)
        {
            ret = tuya_ipc_ss_put_audio(&frame);
        }
        else
        {
            PR_ERR("Get Frame Channel ERR:%d", channel);
            break;
        }
        if(OPRT_OK == ret)
        {
            is_retry = FALSE;
        }
        else
        {
            is_retry = TRUE;
        }
    }

    PR_DEBUG("stream_storage thread finish. channel:%d", channel);

    pthread_exit(0);
}

OPERATE_RET TUYA_APP_Init_Stream_Storage(IN CONST CHAR *p_sd_base_path)
{
    STATIC BOOL s_stream_storage_inited = FALSE;
    if(s_stream_storage_inited == TRUE)
    {
        PR_DEBUG("The Stream Storage Is Already Inited");
        return OPRT_OK;
    }

    PR_DEBUG("Init Stream_Storage SD:%s", p_sd_base_path);
    PR_DEBUG("Init Stream_Storage max_key_num:%d", s_media_info.max_key_frame_in_one_file);
    INT ret;
    pthread_t stream_storage_thread;
    STREAM_STORAGE_INFO_S storage_info;
    memset(&storage_info, 0x00, sizeof(STREAM_STORAGE_INFO_S));
    strncpy(storage_info.base_path, p_sd_base_path, strlen(p_sd_base_path));
    storage_info.video_codec = s_media_info.video_codec;
    storage_info.audio_codec = s_media_info.audio_codec;
    storage_info.max_key_num = s_media_info.max_key_frame_in_one_file;

    ret = tuya_ipc_ss_init(&storage_info,SS_WRITE_MODE_ALL);
    if(ret != 0)
    {
        PR_ERR("Init Main Video Stream_Storage Fail. %d", ret);
        return OPRT_COM_ERROR;
    }

    //本地存储只保存高清版本视频和音频
    if(s_media_info.channel_enable[E_CHANNEL_VIDEO_MAIN] == TRUE)
    {
        PR_DEBUG("Init Main Video Stream_Storage");
        s_video_thread_arg.channel = E_CHANNEL_VIDEO_MAIN;
        pthread_create(&stream_storage_thread, NULL, thread_stream_storage, (VOID *)&s_video_thread_arg);
        pthread_detach(stream_storage_thread);
    }

    if(s_media_info.channel_enable[E_CHANNEL_AUDIO] == TRUE)
    {
        PR_DEBUG("Init Audio Stream_Storage");
        s_audio_thread_arg.channel = E_CHANNEL_AUDIO;
        pthread_create(&stream_storage_thread, NULL, thread_stream_storage, (VOID *)&s_audio_thread_arg);
        pthread_detach(stream_storage_thread);
    }

    s_stream_storage_inited = TRUE;

    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
Stream_Storage相关代码结束位置
---------------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------------
RingBuffer相关代码起始位置
---------------------------------------------------------------------------------
*/

OPERATE_RET TUYA_APP_Init_Ring_Buffer(VOID)
{
    STATIC BOOL s_ring_buffer_inited = FALSE;
    if(s_ring_buffer_inited == TRUE)
    {
        PR_DEBUG("The Ring Buffer Is Already Inited");
        return OPRT_OK;
    }

    CHANNEL_E channel;
    OPERATE_RET ret;
    for( channel = E_CHANNEL_VIDEO_MAIN; channel < E_CHANNEL_MAX; channel++ )
    {
        PR_DEBUG("Init Ring_Buffer. Channel:%d Enable:%d", channel, s_media_info.channel_enable[channel]);
        if(s_media_info.channel_enable[channel] == TRUE)
        {
            UINT buffer_size = s_media_info.video_bitrate * 1024/8*4; //4-seconds data buffer
            ret = tuya_ipc_ring_buffer_init(channel, buffer_size);
            if(ret != 0)
            {
                PR_ERR("init ring buffer fails. %d %u %d", channel, buffer_size, ret);
                return OPRT_MALLOC_FAILED;
            }
            PR_DEBUG("init ring buffer success. channel:%d buffer_size:%u", channel, buffer_size);
        }
    }

    s_ring_buffer_inited = TRUE;

    return OPRT_OK;
}

OPERATE_RET TUYA_APP_Put_Frame(IN CONST CHANNEL_E channel, IN CONST MEDIA_FRAME_S *p_frame)
{
    PR_TRACE("Put Frame. Channel:%d type:%d size:%u pts:%ull ts:%ull",
             channel, p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);

    OPERATE_RET ret = tuya_ipc_ring_buffer_append_data(channel,p_frame->p_buf, p_frame->size,p_frame->type,p_frame->pts);

    if(ret != OPRT_OK)
    {
        /*PR_ERR("Put Frame Fail.%d Channel:%d type:%d size:%u pts:%ull ts:%ull",ret,*/
                 /*channel, p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);*/
    }
    return ret;
}

OPERATE_RET TUYA_APP_Get_Frame_Backwards(IN CONST CHANNEL_E channel,
                                                  IN CONST USER_INDEX_E user_index,
                                                  IN CONST UINT backward_frame_num,
                                                  INOUT MEDIA_FRAME_S *p_frame)
{
    if(p_frame == NULL)
    {
        PR_ERR("input is null");
        return OPRT_INVALID_PARM;
    }

    UINT real_backward_frame_num;
    if(channel == E_CHANNEL_VIDEO_MAIN || channel == E_CHANNEL_VIDEO_SUB)
        real_backward_frame_num = tuya_ipc_ring_buffer_update_user_info_to_pre_video_frames(channel,user_index,backward_frame_num);
    else
        real_backward_frame_num = tuya_ipc_ring_buffer_update_user_info_to_pre_audio_frames(channel,user_index,backward_frame_num);
    if(real_backward_frame_num < 0)
    {
        PR_ERR("Fail to re-locate for user %d backward %d frames",user_index,backward_frame_num);
        return OPRT_INVALID_PARM;
    }

    return TUYA_APP_Get_Frame(channel, user_index, TRUE, p_frame);;
}

OPERATE_RET TUYA_APP_Get_Frame(IN CONST CHANNEL_E channel, IN CONST USER_INDEX_E user_index, IN CONST BOOL isRetry, INOUT MEDIA_FRAME_S *p_frame)
{
    if(p_frame == NULL)
    {
        PR_ERR("input is null");
        return OPRT_INVALID_PARM;
    }
    PR_TRACE("Get Frame Called. channel:%d user:%d retry:%d", channel, user_index, isRetry);

    Ring_Buffer_Node_S *node = NULL;
    while(node == NULL)
    {
        if(channel == E_CHANNEL_VIDEO_MAIN || channel == E_CHANNEL_VIDEO_SUB)
        {
            node = tuya_ipc_ring_buffer_get_video_frame(channel,user_index,isRetry);
        }
        else if(channel == E_CHANNEL_AUDIO)
        {
            node = tuya_ipc_ring_buffer_get_audio_frame(channel,user_index,isRetry);
        }
        if(NULL == node)
        {
            usleep(10*1000);
        }
    }
    p_frame->p_buf = node->rawData;
    p_frame->size = node->size;
    p_frame->timestamp = node->timestamp;
    p_frame->type = node->type;
    p_frame->pts = node->pts;

    PR_TRACE("Get Frame Success. channel:%d user:%d retry:%d size:%u ts:%ull type:%d pts:%llu",
             channel, user_index, isRetry, p_frame->size, p_frame->timestamp, p_frame->type, p_frame->pts);
    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
RingBuffer相关代码结束位置
---------------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------------
TUTK P2P相关代码起始位置
---------------------------------------------------------------------------------
*/

typedef struct
{
    BOOL enabled;
    BOOL enable_live_video;
    BOOL enable_live_audio;
    TRANSFER_VIDEO_CLARITY_TYPE_E live_clarity;
    UINT max_users;
    TRANSFER_AUDIO_CODEC_E p2p_audio_codec;
}TUYA_APP_P2P_MGR;

STATIC TUYA_APP_P2P_MGR s_p2p_mgr = {0};

STATIC VOID __TUYA_APP_media_frame_TO_trans_video(IN CONST MEDIA_FRAME_S *p_in, INOUT TRANSFER_VIDEO_FRAME_S *p_out)
{
    switch (s_media_info.video_codec)
    {
        case CODEC_VIDEO_H264:
        {
            p_out->video_codec = TY_VIDEO_CODEC_H264;
            break;
        }
        default:
        {
            PR_ERR("curr video codec not supported. %d", s_media_info.video_codec);
            p_out->video_codec = TY_VIDEO_CODEC_H264;
            break;
        }
    }

    p_out->video_frame_type = p_in->type == E_VIDEO_PB_FRAME ? TY_VIDEO_FRAME_PBFRAME:TY_VIDEO_FRAME_IFRAME;
    p_out->p_video_buf = p_in->p_buf;
    p_out->buf_len = p_in->size;
    p_out->timestamp = p_in->timestamp;
}

STATIC VOID __TUYA_APP_media_frame_TO_trans_audio(IN CONST MEDIA_FRAME_S *p_in, INOUT TRANSFER_AUDIO_FRAME_S *p_out)
{
    switch (s_media_info.audio_codec)
    {
        case CODEC_AUDIO_PCM:
        {
            p_out->audio_codec = TY_AUDIO_CODEC_PCM;
            break;
        }
        default:
        {
            PR_ERR("curr audio codec not supported. %d", s_media_info.audio_codec);
            p_out->audio_codec = TY_AUDIO_CODEC_PCM;
            break;
        }
    }

    switch (s_media_info.audio_sample)
    {
        case AUDIO_SAMPLE_8K:
        {
            p_out->audio_sample = TY_AUDIO_SAMPLE_8K;
            break;
        }
        default:
        {
            PR_ERR("curr audio sample not supported. %d", s_media_info.audio_sample);
            p_out->audio_sample = TY_AUDIO_SAMPLE_8K;
            break;
        }
    }

    switch (s_media_info.audio_databits)
    {
        case AUDIO_DATABITS_8:
        {
            p_out->audio_databits = TY_AUDIO_DATABITS_8;
            break;
        }
        case AUDIO_DATABITS_16:
        {
            p_out->audio_databits = TY_AUDIO_DATABITS_16;
            break;
        }
        default:
        {
            PR_ERR("curr audio db not supported. %d", s_media_info.audio_databits);
            p_out->audio_databits = TY_AUDIO_DATABITS_16;
            break;
        }
    }

    switch (s_media_info.audio_channel)
    {
        case AUDIO_CHANNEL_MONO:
        {
            p_out->audio_channel = TY_AUDIO_CHANNEL_MONO;
            break;
        }
        case AUDIO_CHANNEL_STERO:
        {
            p_out->audio_channel = TY_AUDIO_CHANNEL_STERO;
            break;
        }
        default:
        {
            PR_ERR("curr audio channel not supported. %d", s_media_info.audio_channel);
            p_out->audio_channel = TY_AUDIO_CHANNEL_MONO;
            break;
        }
    }
    p_out->p_audio_buf = p_in->p_buf;
    p_out->buf_len = p_in->size;
    p_out->timestamp = p_in->timestamp;
}

STATIC VOID *__TUYA_APP_ss_pb_event_cb(IN UINT pb_idx, IN SS_PB_EVENT_E pb_event, IN PVOID args)
{
    PR_DEBUG("ss pb rev event: %u %d", pb_idx, pb_event);
    if(pb_event == SS_PB_FINISH)
    {
        tuya_ipc_playback_send_finish(pb_idx);
    }
	return NULL;
}

STATIC VOID *__TUYA_APP_ss_pb_get_video_cb(IN UINT pb_idx, IN CONST MEDIA_FRAME_S *p_frame)
{
    TRANSFER_VIDEO_FRAME_S video_frame = {0};
    __TUYA_APP_media_frame_TO_trans_video(p_frame, &video_frame);
    tuya_ipc_playback_send_video_frame(pb_idx, &video_frame);
	return NULL;
}

STATIC VOID *__TUYA_APP_ss_pb_get_audio_cb(IN UINT pb_idx, IN CONST MEDIA_FRAME_S *p_frame)
{
    TRANSFER_AUDIO_FRAME_S audio_frame = {0};
    __TUYA_APP_media_frame_TO_trans_audio(p_frame, &audio_frame);
    tuya_ipc_playback_send_audio_frame(pb_idx, &audio_frame);
	return NULL;
}

STATIC VOID __depereated_online_cb(IN TRANSFER_ONLINE_E status)
{

}

STATIC VOID *__TUYA_APP_P2P_live_video(void *arg)
{
    MEDIA_FRAME_S h264_frame = {0};
    BOOL is_retry = FALSE;
    OPERATE_RET ret;

    PR_DEBUG("tutk p2p live video thread start");

    while(s_p2p_mgr.enabled == TRUE)
    {
        if(s_p2p_mgr.enable_live_video == TRUE)
        {
            PR_TRACE("live video enable and clarity:%d", s_p2p_mgr.live_clarity);

            if( (s_p2p_mgr.live_clarity == TY_VIDEO_CLARITY_STANDARD) &&
                    (s_media_info.channel_enable[E_CHANNEL_VIDEO_SUB] == TRUE) )
            {
                TUYA_APP_Get_Frame(E_CHANNEL_VIDEO_SUB, E_USER_TUTK_P2P, is_retry, &h264_frame);
            }
            else if( (s_p2p_mgr.live_clarity == TY_VIDEO_CLARITY_HIGH) &&
                    (s_media_info.channel_enable[E_CHANNEL_VIDEO_MAIN] == TRUE) )
            {
                TUYA_APP_Get_Frame(E_CHANNEL_VIDEO_MAIN, E_USER_TUTK_P2P, is_retry, &h264_frame);
            }
            else
            {
                PR_DEBUG("live_clarity and channel not match.No Live Video. %d %d %d",
                         s_p2p_mgr.live_clarity, s_media_info.channel_enable[0], s_media_info.channel_enable[1]);
                sleep(2);
                continue;
            }

            TRANSFER_VIDEO_FRAME_S p2p_video_frm = {0};
            __TUYA_APP_media_frame_TO_trans_video(&h264_frame, &p2p_video_frm);
            ret = tuya_ipc_live_send_video_frame( &p2p_video_frm );
            if(OPRT_OK != ret)
            {
                PR_ERR("p2p send live video fail. %d", ret);
            }
        }
        else
        {
            sleep(2);
        }
    }

    PR_DEBUG("tutk p2p live video thread stop");

    pthread_exit(0);
}

STATIC VOID *__TUYA_APP_P2P_live_audio(void *arg)
{
    MEDIA_FRAME_S audio_frame = {0};
    BOOL is_retry = FALSE;
    OPERATE_RET ret;

    PR_DEBUG("tutk p2p live audio thread start");

    while(s_p2p_mgr.enabled == TRUE)
    {
        if(s_p2p_mgr.enable_live_audio == TRUE)
        {
            if(s_media_info.channel_enable[E_CHANNEL_AUDIO] == TRUE)
            {
                TUYA_APP_Get_Frame(E_CHANNEL_AUDIO, E_USER_TUTK_P2P, is_retry, &audio_frame);
            }else
            {
                PR_DEBUG("Audio Channel Not Enable. No Live Audio");
                sleep(2);
                continue;
            }

            TRANSFER_AUDIO_FRAME_S p2p_audio_frm = {0};
            __TUYA_APP_media_frame_TO_trans_audio(&audio_frame, &p2p_audio_frm);
            ret = tuya_ipc_live_send_audio_frame( &p2p_audio_frm );
            if(OPRT_OK != ret)
            {
                PR_ERR("p2p send live audio fail. %d", ret);
            }
        }else
        {
            sleep(2);
        }
    }

    PR_DEBUG("tutk p2p live audio thread stop");

    pthread_exit(0);
}

/* 传输事件回调 */
STATIC VOID __TUYA_APP_p2p_event_cb(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID args)
{
    PR_DEBUG("p2p rev event cb=[%d] ", event);
    switch (event)
    {
        case TRANS_LIVE_VIDEO_START:
        {
            PR_DEBUG("live video start");
            s_p2p_mgr.enable_live_video = TRUE;
            break;
        }
        case TRANS_LIVE_VIDEO_STOP:
        {
            PR_DEBUG("live video stop");
            s_p2p_mgr.enable_live_video = FALSE;
            break;
        }
        case TRANS_LIVE_AUDIO_START:
        {
            PR_DEBUG("live audio start");
            s_p2p_mgr.enable_live_audio = TRUE;
            break;
        }
        case TRANS_LIVE_AUDIO_STOP:
        {
            PR_DEBUG("live audio stop");
            s_p2p_mgr.enable_live_audio = FALSE;
            break;
        }
        case TRANS_SPEAKER_START:
        {
            PR_DEBUG("enbale audio speaker");
            TUYA_APP_Enable_Speaker_CB(TRUE);
            break;
        }
        case TRANS_SPEAKER_STOP:
        {
            PR_DEBUG("disable audio speaker");
            TUYA_APP_Enable_Speaker_CB(FALSE);
            break;
        }
        case TRANS_LIVE_LOAD_ADJUST:
        {
            EVENT_LIVE_LOAD_PARAM_S *quality = (EVENT_LIVE_LOAD_PARAM_S *)args;
            PR_DEBUG("live quality %d -> %d", quality->curr_load_level, quality->new_load_level);
            break;
        }
        case TRANS_PLAYBACK_LOAD_ADJUST:
        {
            EVENT_PLAYBACK_LOAD_PARAM_S *quality= (EVENT_PLAYBACK_LOAD_PARAM_S *)args;
            PR_DEBUG("pb idx:%d quality %d -> %d", quality->client_index, quality->curr_load_level, quality->new_load_level);
            break;
        }
        case TRANS_PLAYBACK_QUERY_MONTH_SIMPLIFY:
        {
            EVENT_PLAYBACK_QUERY_MONTH_SIMPLIFY_PARAM_S *p = (EVENT_PLAYBACK_QUERY_MONTH_SIMPLIFY_PARAM_S *)args;
            PR_DEBUG("pb query by month: %d-%d", p->query_year, p->query_month);

            OPERATE_RET ret = tuya_ipc_pb_query_by_month(p->query_year, p->query_month, &(p->return_days));
            if (OPRT_OK != ret)
            {
                PR_ERR("pb query by month: %d-%d ret:%d", p->query_year, p->query_month, ret);
            }

            break;
        }
        case TRANS_PLAYBACK_QUERY_DAY_TS:
        {
            EVENT_PLAYBACK_QUERY_DAY_TS_PARAM_S *pquery = (EVENT_PLAYBACK_QUERY_DAY_TS_PARAM_S *)args;
            PR_DEBUG("pb_ts query by day: %d-%d-%d", pquery->query_year, pquery->query_month, pquery->query_day);
            SS_QUERY_DAY_TS_ARR_S *p_day_ts = NULL;
            OPERATE_RET ret = tuya_ipc_pb_query_by_day(pquery->query_year, pquery->query_month, pquery->query_day, &p_day_ts);
            if (OPRT_OK != ret)
            {
                PR_ERR("pb_ts query by day: %d-%d-%d Fail", pquery->query_year, pquery->query_month, pquery->query_day);
                break;
            }
            pquery->p_query_ts_arr = (PB_QUERY_DAY_TS_ARR_S *)p_day_ts;
            break;
        }
        case TRANS_PLAYBACK_START_TS:
        {
            /* 开始回放时client会带上开始时间点，这里简单起见，只进行了日志打印 */
            EVENT_PLAYBACK_START_TS_PARAM_S *pParam = (EVENT_PLAYBACK_START_TS_PARAM_S *)args;
            PR_DEBUG("PB StartTS idx:%d %u [%u %u]", pParam->client_index, pParam->play_timestamp, pParam->pb_file_info.start_timestamp, pParam->pb_file_info.end_timestamp);

            tuya_ipc_ss_pb_start(pParam->client_index, (SS_PB_EVENT_CB)__TUYA_APP_ss_pb_event_cb, (SS_PB_GET_MEDIA_CB)__TUYA_APP_ss_pb_get_video_cb, (SS_PB_GET_MEDIA_CB)__TUYA_APP_ss_pb_get_audio_cb);
            tuya_ipc_ss_pb_seek(pParam->client_index, (SS_FILE_TIME_TS_S *)&(pParam->pb_file_info), pParam->play_timestamp);

            break;
        }
        case TRANS_PLAYBACK_PAUSE:
        {
            EVENT_PLAYBACK_PAUSE_PARAM_S *pParam = (EVENT_PLAYBACK_PAUSE_PARAM_S *)args;
            PR_DEBUG("PB Pause idx:%d", pParam->client_index);

            tuya_ipc_ss_pb_set_status(pParam->client_index, SS_PB_PAUSE);
            break;
        }
        case TRANS_PLAYBACK_RESUME:
        {
            EVENT_PLAYBACK_RESUME_PARAM_S *pParam = (EVENT_PLAYBACK_RESUME_PARAM_S *)args;
            PR_DEBUG("PB Resume idx:%d", pParam->client_index);

            tuya_ipc_ss_pb_set_status(pParam->client_index, SS_PB_RESUME);
            break;
        }
        case TRANS_PLAYBACK_MUTED:
        {
            EVENT_PLAYBACK_MUTED_PARAM_S *pParam = (EVENT_PLAYBACK_MUTED_PARAM_S *)args;
            PR_DEBUG("PB idx:%d muted:%d", pParam->client_index, pParam->muted);

            tuya_ipc_ss_pb_set_status(pParam->client_index, pParam->muted == 1 ? SS_PB_MUTE:SS_PB_UN_MUTE );
            break;
        }
        case TRANS_PLAYBACK_STOP:
        {
            EVENT_PLAYBACK_STOP_PARAM_S *pParam = (EVENT_PLAYBACK_STOP_PARAM_S *)args;
            PR_DEBUG("PB Stop idx:%d", pParam->client_index);

            tuya_ipc_ss_pb_stop(pParam->client_index);
            break;
        }
        case TRANS_LIVE_VIDEO_CLARITY_SET:
        {
            EVENT_VIDEO_CLARITY_PARAM_S *pParam = (EVENT_VIDEO_CLARITY_PARAM_S *)args;
            PR_DEBUG("set clarity:%d", pParam->clarity);
            if((pParam->clarity == TY_VIDEO_CLARITY_STANDARD)||(pParam->clarity == TY_VIDEO_CLARITY_HIGH))
            {
                s_p2p_mgr.live_clarity = pParam->clarity;
            }
            break;
        }
        case TRANS_LIVE_VIDEO_CLARITY_QUERY:
        {
            EVENT_VIDEO_CLARITY_PARAM_S *pParam = (EVENT_VIDEO_CLARITY_PARAM_S *)args;
            pParam->clarity = s_p2p_mgr.live_clarity;
            PR_DEBUG("query larity:%d", pParam->clarity);
            break;
        }
        default:
            break;
    }
}

STATIC VOID __TUYA_APP_rev_audio_cb(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT time_stamp, IN CONST UINT frame_no)
{
    MEDIA_FRAME_S audio_frame = {0};
    audio_frame.p_buf = p_audio_frame->p_audio_buf;
    audio_frame.size = p_audio_frame->buf_len;

    PR_TRACE("Rev Audio. size:%u audio_codec:%d audio_sample:%d audio_databits:%d audio_channel:%d",p_audio_frame->buf_len,
             p_audio_frame->audio_codec, p_audio_frame->audio_sample, p_audio_frame->audio_databits, p_audio_frame->audio_channel);

    TUYA_APP_Rev_Audio_CB( &audio_frame, AUDIO_SAMPLE_8K, AUDIO_DATABITS_16, AUDIO_CHANNEL_MONO);
}

OPERATE_RET TUYA_APP_Enable_P2PTransfer(IN BOOL enable, IN UINT max_users)
{
    if(s_p2p_mgr.enabled == enable)
    {
        PR_DEBUG("The Tutk P2P Is Already Inited");
        return OPRT_OK;
    }

    if(enable != TRUE)
    {
        PR_DEBUG("The Tutk P2P Is Only Support Enable %d", enable);
        return OPRT_INVALID_PARM;
    }

    PR_DEBUG("Init Tutk With Max Users:%u", max_users);

    s_p2p_mgr.enabled = TRUE;
    s_p2p_mgr.max_users = max_users;

    s_p2p_mgr.p2p_audio_codec = TY_AUDIO_CODEC_PCM;

    TUYA_IPC_TRANSFER_VAR_S p2p_var = {0};
    p2p_var.online_cb = __depereated_online_cb;
    p2p_var.on_rev_audio_cb = __TUYA_APP_rev_audio_cb;
    p2p_var.rev_audio_codec = s_p2p_mgr.p2p_audio_codec;
    p2p_var.on_event_cb = __TUYA_APP_p2p_event_cb;
    p2p_var.live_quality = TRANS_LIVE_QUALITY_MAX;
    p2p_var.max_client_num = max_users;
    tuya_ipc_tranfser_init(&p2p_var);

    pthread_t live_video_thread;
    pthread_create(&live_video_thread, NULL, __TUYA_APP_P2P_live_video, NULL);
    pthread_detach(live_video_thread);

    pthread_t live_audio_thread;
    pthread_create(&live_audio_thread, NULL, __TUYA_APP_P2P_live_audio, NULL);
    pthread_detach(live_audio_thread);


    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
TUTK P2P相关代码结束位置
---------------------------------------------------------------------------------
*/



/*
---------------------------------------------------------------------------------
EchoShow相关代码起始位置
---------------------------------------------------------------------------------
*/

OPERATE_RET TUYA_APP_Enable_EchoShow(IN BOOL enable)
{
    if(enable != TRUE)
    {
        PR_ERR("EchoShow Currently Not Support Disable");
        return OPRT_COM_ERROR;
    }

    STATIC BOOL s_echoshow_inited = FALSE;
    if(s_echoshow_inited == TRUE)
    {
        PR_DEBUG("The EchoShow Is Already Inited");
        return OPRT_OK;
    }

    PR_DEBUG("Init EchoShow");
    TUYA_APP_Init_Ring_Buffer();
    tuya_ipc_echo_show_init(s_media_info);

    s_echoshow_inited = TRUE;

    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
EchoShow相关代码结束位置
---------------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------------
云储存相关代码起始位置
---------------------------------------------------------------------------------
*/

void *thread_cloud_storage(void *arg)
{
    MEDIA_FRAME_S h264_frame = {0};
    BOOL is_retry = FALSE;
    BOOL is_1st_start = TRUE;
    OPERATE_RET ret;
    UINT retLen;

    UINT ts_buffer_size = 120*1024;
    char *ts_buffer = malloc(ts_buffer_size);
    if(ts_buffer == NULL)
    {
        PR_ERR("Fail to alloc ts buffer");
        return NULL;
    }

    PR_DEBUG("Cloud storage task start fps %d gop %d",s_media_info.video_fps,s_media_info.video_gop);
    tuya_ipc_cloud_storage_init(s_media_info.video_fps,s_media_info.video_gop);

    StreamInfo stream_info;
    memset((void *)&stream_info, 0, sizeof(StreamInfo));
    stream_info.iProgramInfo[0].iMuxConfig = MPEGTS_MUX_CONFIG_VIDEO_ONLY;
    stream_info.iProgramInfo[0].iStreamType = MEDIA_TYPE_VIDEO;
    stream_info.patinfo.iChannelId[0] = 0;
    stream_info.patinfo.iTotalProgramNum = 1;
    stream_info.iCurrentChannelId = 0;

    while(TRUE)
    {
        if(is_1st_start)
        {
            TUYA_APP_Get_Frame_Backwards(E_CHANNEL_VIDEO_MAIN, E_USER_COULD_STORAGE, s_media_info.video_fps, &h264_frame); //回找1秒视频
            is_1st_start = FALSE;
        }
        else
        {
            TUYA_APP_Get_Frame(E_CHANNEL_VIDEO_MAIN, E_USER_COULD_STORAGE, is_retry, &h264_frame);
        }

        stream_info.iProgramInfo[0].iPtsTime = h264_frame.pts;
        stream_info.iProgramInfo[0].iFrameSize = h264_frame.size;
        stream_info.iProgramInfo[0].iNaluType = h264_frame.type;

        if(h264_frame.size > ts_buffer_size)
        {
            PR_DEBUG("frame size too big %d > %d",h264_frame.size,ts_buffer_size);
            continue;
        }
        retLen = ConvertEStoTS(h264_frame.p_buf, &stream_info, (unsigned char *)ts_buffer);

        ret = tuya_ipc_upload_h264_video_frame(ts_buffer, retLen, h264_frame.type);
        if(OPRT_OK == ret)
        {
            is_retry = FALSE;
        }
        else
        {
            is_retry = TRUE;
        }
    }
}

OPERATE_RET TUYA_APP_Enable_CloudStorage(IN BOOL enable)
{
    pthread_t cloud_storage_thread;
    pthread_create(&cloud_storage_thread, NULL, thread_cloud_storage, NULL);
    pthread_detach(cloud_storage_thread);
    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
云储存相关代码结束位置
---------------------------------------------------------------------------------
*/

#if 0
/*
---------------------------------------------------------------------------------
ChromeCast相关代码起始位置
---------------------------------------------------------------------------------
*/

OPERATE_RET TUYA_APP_Enable_ChromeCast(IN BOOL enable)
{
    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
ChromeCast相关代码结束位置
---------------------------------------------------------------------------------
*/
#endif




OPERATE_RET TUYA_APP_Trigger_Event_Storage(VOID)
{
    PR_DEBUG("Event Triggerd");
    tuya_ipc_ss_trigger_event();
	return 0;
}

