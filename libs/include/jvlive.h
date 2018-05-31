#ifndef _JVLIVE_H_
#define _JVLIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	JVLIVE_CMD_TYPE_CONNECTED,
	JVLIVE_CMD_TYPE_DISCONNECTED,
}JVLiveCmdType_e;

typedef struct{
	JVLiveCmdType_e type;
	struct{
		char addr[16];
		unsigned int sessionID;
	}connect;
}jvlive_info_t;

typedef struct{
	void (*callback)(jvlive_info_t *info);
	int max_frame_len;
}jvliveParam_t;

void jvlive_rtsp_init(jvliveParam_t *param);

//准备RTSP服务
//param nameFmt 类似live%d.264
//cnt 提供的视频的路数。
void jvlive_rtsp_start(char *nameFmt, unsigned short port, int *bufferSizeList, int *frameList, int cnt);

//audioType: "g726-40"/"PCMU"/"PCMA" ...
//PCMU，PCMA即对应的G711-U,-A. 目前，在8K,16BIT情况下，测试可用
void jvlive_rtsp_start_ex(char *nameFmt, unsigned short port, int *bufferSizeList, int *frameList, int cnt, char *audioType, int timeStampFrequency);

typedef struct{
	char audioType[32];
	int timeStampFrequency;
}JVLiveAudioInfo_t;

//修改音频参数
int jvlive_rtsp_set_audio_info(int channelid, JVLiveAudioInfo_t *audioInfo);

//获取音频参数
int jvlive_rtsp_get_audio_info(int channelid, JVLiveAudioInfo_t *audioInfo);

//设置当前帧率
int jvlive_rtsp_set_framerate(int index, int framerate);

//写入数据
int jvlive_rtsp_write(int index, unsigned char *data, int len);

//写入音频数据
int jvlive_rtsp_write_audio(int index, unsigned char *data, int len);


//添加用户名、密码
//必须调用jvlive_rtsp_user_add_finished 才能生效
void jvlive_rtsp_user_add_start(char *user, char *passwd);

//完成添加用户
void jvlive_rtsp_user_add_finished(void);

#ifdef __cplusplus
}
#endif

#endif
