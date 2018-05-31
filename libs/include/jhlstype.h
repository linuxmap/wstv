/*
 * jhlstype.h
 *
 *  Created on: 2015年6月8日
 *      Author: LiuFengxiang
 *		 Email: lfx@jovision.com
 */

#ifndef JHLSTYPE_H_
#define JHLSTYPE_H_


typedef enum {

	JHLS_VIDEO_STREAMTYPE_H264,
	JHLS_VIDEO_STREAMTYPE_H265,
	JHLS_VIDEO_STREAMTYPE_MAX
} JHLSVideoStreamType_e;

enum{
	JHLS_TS_STREAM_TYPE_H264 = 0x1B,
	JHLS_TS_STREAM_TYPE_AAC = 0x0F,
	JHLS_TS_STREAM_TYPE_MP3 = 0x03,
	JHLS_TS_STREAM_TYPE_G711A = 0x31, //we defined it.
	JHLS_TS_STREAM_TYPE_G711U = 0x32, //we defined it.
};

typedef enum{
	JHLS_FRAME_TYPE_VIDEO,//Video,But not sure I or p
	JHLS_FRAME_TYPE_VIDEO_I,
	JHLS_FRAME_TYPE_VIDEO_P,
	JHLS_FRAME_TYPE_VIDEO_B,
	JHLS_FRAME_TYPE_AUDIO,
	JHLS_FRAME_TYPE_MAX,
}JHLSFrameType_e;

typedef enum{
	JHLS_AUDIO_STREAMTYPE_AAC,
	JHLS_AUDIO_STREAMTYPE_MP3,
	JHLS_AUDIO_STREAMTYPE_G711A, //we define 711a ts value:
	JHLS_AUDIO_STREAMTYPE_G711U,
	JHLS_AUDIO_STREAMTYPE_MAX
} JHLSAudioStreamType_e;

typedef struct{
	JHLSAudioStreamType_e type;
	union{
		struct{
			unsigned char id;
			unsigned char profile;
			unsigned char channel_configuration; //0~7，1: 1 channel: front-center，2: 2 channels: front-left, front-right，3: 3 channels: front-center, front-left, front-right，4: 4 channels: front-center, front-left, front-right, back-center
			int sampling_freq; //16000, 8000, ...
			int aac_frame_length; //包括adts头在内的音频数据总长度
		}aac;
	};
}JHLSAudioInfo_t;

#define MAX_M3U8_LINE_LEN 256

typedef struct{
	float second;
	char fname[MAX_M3U8_LINE_LEN];
} M3U8Node_t;

typedef void *JHLSHandle_t;

#endif /* JHLSTYPE_H_ */
