#include "tuya.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_dp_handler.h"
#include "tuya_ipc_mgr_utils.h"
#include "tuya_ipc_media_utils.h"
#include "tuya_ipc_cloud_storage.h"


#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
STATIC CHAR s_raw_path[128] = {0};

static void *sync_time_func(void *arg)
{
    while(1)
    {
        sleep(60);

        OPERATE_RET ret = OPRT_OK;

        /* 调用本API同步时间。如果返回OK，说明时间同步成功 */
		ret = IPC_APP_Sync_Utc_Time();
        if(ret != OPRT_OK)
        {
            continue;
        }
        break;
    }

    pthread_exit(0);
}

/*
 * 示例代码采用文件读写方式来模拟音视频请求，文件在rawfiles.tar.gz中
 */
#define AUDIO_FRAME_SIZE 640
#define AUDIO_FPS 25

#define VIDEO_BUF_SIZE	(1024 * 400)

static int live_clarity = 0;/* 0为标清 */

/*
使用读写文件的方式来模拟音频输出。
*/
void *thread_live_audio(void *arg)
{
    char fullpath[128] = {0};
    sprintf(fullpath, "%s/rawfiles/jupiter_8k_16bit_mono.raw", s_raw_path);

    FILE *aFp = fopen(fullpath, "rb");
    if(aFp == NULL)
    {
        printf("can't read live audio files\n");
        pthread_exit(0);
    }
    unsigned char audioBuf[AUDIO_FRAME_SIZE];
    MEDIA_FRAME_S pcm_frame = {0};
    pcm_frame.type = E_AUDIO_FRAME;

    while(1)
    {
        int size = fread(audioBuf, 1, AUDIO_FRAME_SIZE, aFp);
        if(size < AUDIO_FRAME_SIZE)
        {
            rewind(aFp);
            continue;
        }
        pcm_frame.size = size;
        pcm_frame.p_buf = audioBuf;

        TUYA_APP_Put_Frame(E_CHANNEL_AUDIO,&pcm_frame);

        int frameRate = AUDIO_FPS;
        int sleepTick = 1000000/frameRate;
        usleep(sleepTick);
    }

    pthread_exit(0);
}

/* 使用读写文件的方式来模拟直播视频。*/
void *thread_live_video(void *arg)
{
    char raw_fullpath[128] = {0};
    char info_fullpath[128] = {0};

    sprintf(raw_fullpath, "%s/rawfiles/video_multi/beethoven_240.multi/frames.bin", s_raw_path);
    sprintf(info_fullpath, "%s/rawfiles/video_multi/beethoven_240.multi/frames.info", s_raw_path);

    FILE *streamBin_fp = fopen(raw_fullpath, "rb");
    FILE *streamInfo_fp = fopen(info_fullpath, "rb");
    if((streamBin_fp == NULL)||(streamInfo_fp == NULL))
    {
        printf("can't read live video files\n");
        pthread_exit(0);
    }

    char line[128] = {0}, *read = NULL;
    INT fps = 30;
    read = fgets(line, sizeof(line), streamInfo_fp);
    sscanf(line, "FPS %d\n", &fps);

    unsigned char videoBuf[VIDEO_BUF_SIZE];

    MEDIA_FRAME_S h264_frame = {0};
    while(1)
    {
        read = fgets(line, sizeof(line), streamInfo_fp);
        if(read == NULL)
        {
            rewind(streamBin_fp);
            rewind(streamInfo_fp);
            read = fgets(line, sizeof(line), streamInfo_fp);

            continue;
        }

        char frame_type[2] = {0};
        int frame_pos = 0, frame_size = 0, nRet = 0;
        sscanf(line, "%c %d %d\n", frame_type, &frame_pos, &frame_size);

        fseek(streamBin_fp, frame_pos*sizeof(char), SEEK_SET);
        nRet = fread(videoBuf, 1, frame_size, streamBin_fp);
        if(nRet < frame_size)
        {
            rewind(streamBin_fp);
            rewind(streamInfo_fp);
            read = fgets(line, sizeof(line), streamInfo_fp);
            continue;
        }

        h264_frame.type = (strcmp(frame_type, "I") == 0 ? E_VIDEO_I_FRAME: E_VIDEO_PB_FRAME);
        h264_frame.p_buf = videoBuf;
        h264_frame.size = nRet;
        h264_frame.pts = 0;

        /* 将高清视频数据送入SDK */
        TUYA_APP_Put_Frame(E_CHANNEL_VIDEO_MAIN, &h264_frame);
        /* 将标清视频数据送入SDK */
        TUYA_APP_Put_Frame(E_CHANNEL_VIDEO_SUB, &h264_frame);

        int frameRate = fps;
        int sleepTick = 1000000/frameRate;
        usleep(sleepTick);
    }

    pthread_exit(0);
}

int tuya_init()
{
    INT res = -1;
    CHAR *token = "AYUMUIc20qTxQK";;
    WIFI_INIT_MODE_E mode = WIFI_INIT_AUTO;

	printf("===================%s %d\n", __func__, __LINE__);
	printf("===================%s %d\n", __func__, __LINE__);
    /* 启动SDK */
    IPC_APP_Init_SDK(mode, token);
	printf("===================%s %d\n", __func__, __LINE__);
	printf("===================%s %d\n", __func__, __LINE__);

    /* 判断SDK是否连接到MQTT */
    while(IPC_APP_Get_Mqtt_Status() != 1)
    {
        sleep(1);
    }
	printf("===================%s %d\n", __func__, __LINE__);
	printf("===================%s %d\n", __func__, __LINE__);
    /* 当MQTT连接成功后，上传本地所有状态 */
    IPC_APP_upload_all_status();
	printf("===================%s %d\n", __func__, __LINE__);
	printf("===================%s %d\n", __func__, __LINE__);

    /* 开启线层，同步服务器和本地时间 */
    pthread_t sync_time_thread;
    pthread_create(&sync_time_thread, NULL, sync_time_func, NULL);
    pthread_detach(sync_time_thread);

	/*tuya_ipc_reset();*/

    return 0;
}

int tuya_deInit()
{
	return 0;
}

