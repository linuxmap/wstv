/***********************************************************
*libzrtsp.h - libzrtsp head file
*
* Copyright(c) 2014~  , 
*
*$Date: $ 
*$Author: $
*$Revision: $
*
*-----------------------
*$Log: $
*
*14-08-13, zhushuchao created
************************************************************/
#ifndef _LIBZRTSP_H_
#define _LIBZRTSP_H_
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MNG_NAME_LEN         16
#define MAX_MNG_DESCRIPTION_LEN 16   
#define MAX_MNG_PWD_LEN           16

typedef enum
{
    e_DEV_INFO_VER,
    e_DEV_INFO_VER_NUM,
    e_DEV_INFO_MODEL_TYPE,
    e_DEV_INFO_MODEL_VER,
    e_DEV_INFO_DEV_TYPE,
    e_DEV_INFO_WORK_MODE,
    e_DEV_INFO_BASIC,
    e_DEV_INFO_DEV_ID,
    e_DEV_INFO_ADVANCED,
    e_DEV_INFO_DVR,
    e_DEV_INFO_IP,
    e_DEV_INFO_LOCAL_ADDR,
    e_DEV_INFO_NET,
    e_DEV_INFO_DYNAMIC_NET,
    e_DEV_INFO_WIFI,
    e_DEV_INFO_WIFI_SEARCH,
    e_DEV_INFO_INFO,
    e_DEV_INFO_USER,
    e_DEV_INFO_REG,
    e_DEV_INFO_USER_NUM,
    e_DEV_INFO_USER_INFO,
    e_DEV_INFO_ADD_USER,
    e_DEV_INFO_DEL_USER,
    e_DEV_INFO_SERIAL_PORT,
    e_DEV_INFO_INOUT_DEV,
    e_DEV_INFO_OUT_DEV,
    e_DEV_INFO_TIME,
    e_DEV_INFO_MDEIA,
    e_DEV_INFO_STREAM,
    e_DEV_INFO_ENCODE_PARA,
    e_DEV_INFO_VIDEO_NUM,
    e_DEV_INFO_VIDOE_FORMAT,
    e_DEV_INFO_VIDEO_MIRROR,
    e_DEV_INFO_VIDEO_FLIP,
    e_DEV_INFO_IMAGE_DISPLAY,
    e_DEV_INFO_IMAGE_TEXT,
    e_DEV_INFO_IMAGE_TEXT_CHN,
    e_DEV_INFO_IMAGE_TEXT_EX,
    e_DEV_INFO_IMAGE_TEXT_EX_CHN,
    e_DEV_INFO_IMAGE_HIDE_AREA,
    e_DEV_INFO_ALARM_PLAN,
    e_DEV_INFO_ALARM_BASIC,
    e_DEV_INFO_ALARM_ALL,
    e_DEV_INFO_GPIN_NUM,
    e_DEV_INFO_GPIN_STATUS,
    e_DEV_INFO_GPIN_ALARM,
    e_DEV_INFO_GOUT_ALARM,
    e_DEV_INFO_GOUT_MODE,
    e_DEV_INFO_GOUT_EX,
    e_DEV_INFO_GPIN_ALARM_EX,
    e_DEV_INFO_VIDEO_ALARM,
    e_DEV_INFO_VIDEO_LOSE,
    e_DEV_INFO_HIDE_DETECTION,
    e_DEV_INFO_MOTION_DETECTION,
    e_DEV_INFO_ALERT,
    e_DEV_INFO_SERVICE,
    e_DEV_INFO_STATE,
    e_DEV_INFO_NET_MANAGE,
    e_DEV_INFO_PTZ,
    e_DEV_INFO_PTZ_PRESET,
    e_DEV_INFO_PTZ_ADD_PRESET,
    e_DEV_INFO_PTZ_DEL_PRESET,
    e_DEV_INFO_PTZ_CTRL,
    e_DEV_INFO_CRUISE_TRACK,
    e_DEV_INFO_CRUISE_TRACK_DEL,
    e_DEV_INFO_DEV_CTRL,
    e_DEV_INFO_TRANSPARENT,
    e_DEV_INFO_STORAGE,
    e_DEV_INFO_STORAGE_TASK,
    e_DEV_INFO_STORAGE_TASK_NEW,
    e_DEV_INFO_STORAGE_TASK_CLEAR,
    e_DEV_INFO_STORAGE_TASK_CHN,
    e_DEV_INFO_STORAGE_FILE,
    e_DEV_INFO_STORAGE_STREAM_TYPE,
    e_DEV_INFO_STORAGE_FILE_OPEN,
    e_DEV_INFO_STORAGE_FILE_OPEN_BY_TIME,
    e_DEV_INFO_STORAGE_FILE_CLOSE,
    e_DEV_INFO_STORAGE_FILE_SEEK,
    e_DEV_INFO_STORAGE_FILE_READ,
    e_DEV_INFO_STORAGE_FILE_READ_SAMPLE,
    e_DEV_INFO_STORAGE_FILE_READ_AUDIO,
    e_DEV_INFO_DEL_FILE,
    e_DEV_INFO_REPORT_ALARM,
    e_DEV_INFO_CLEAR_ALARM,
    e_DEV_INFO_VIDEO_INFO,
    e_DEV_INFO_VIDEO_SUB,
    e_DEV_INFO_VIDEO_INFO_EX,
    e_DEV_INFO_DEC_VIDOE_CTRL,
    e_DEV_INFO_DEC_CTRL,
    e_DEV_INFO_DECODER_INFO,
    e_DEV_INFO_GPS,
    e_DEV_INFO_DISK_INFO,
    e_DEV_INFO_DISK_STATE,
    e_DEV_INFO_DISK_FORMAT,
    e_DEV_INFO_DISK_FORMAT_STATE,
    e_DEV_INFO_SD_STATE,
    e_DEV_INFO_SD_FORMAT,
    e_DEV_INFO_FILE_OPEN,
    e_DEV_INFO_FILE_CHN,
    e_DEV_INFO_SHOOT_FILE_OPEN,
    e_DEV_INFO_ALARM_IPC,
    e_DEV_INFO_MNG_PORT,
    e_DEV_INFO_VIDEO_PORT,
    e_DEV_INFO_UDP_TRANS_DELAY,
    e_DEV_INFO_START_TRARS,
    e_DEV_INFO_DDNS,
    e_DEV_INFO_RESET_PARA,
    e_DEV_INFO_SET_DEFAULT_PARA,
    e_DEV_INFO_EARSE_DEFAULT_PARA,
    e_DEV_INFO_RESET_3G,
    e_DEV_INFO_VOLUM,
    e_DEV_INFO_PLATEFORM,
    e_DEV_INFO_CPU_MEM,
    e_DEV_INFO_PROCESS_NUM,
    e_DEV_INFO_JPEG_SNAP,
    e_DEV_INFO_VIDEO_WH,
    e_DEV_INFO_VIDEO_SIZE,
    e_DEV_INFO_VIDEO_CODEC,
    e_DEV_INFO_VIDEO_SPS,
    e_DEV_INFO_VIDEO_PPS,
    e_DEV_INFO_VIDEO_SPS_PPS,
    e_DEV_INFO_VIDEO_FRAME,
    e_DEV_INFO_VIDEO_QP,
    e_DEV_INFO_VIDEO_BITRATE,
    e_DEV_INFO_VIDEO_CAMERA,
    e_DEV_INFO_CAMERA_ID,
    e_DEV_INFO_AUDIO_CODEC,
    e_DEV_INFO_SECRIT_STATE,
    e_DEV_INFO_MAX_USER,
    e_DEV_INFO_UPGRADE,
    e_DEV_INFO_VIDEO_CTRL,
    e_DEV_INFO_AUDIO_CTRL,
    e_DEV_INFO_LINK_MODE,
    e_DEV_INFO_TCP_BUF_LEVEL,
    e_DEV_INFO_UDP_DELAY,
    e_DEV_INFO_RTP_OVERFLAG,
    e_DEV_INFO_MULTICAST_INFO,
    e_DEV_INFO_DEV_NAME,
    e_DEV_INFO_DEV_CAP,
    e_DEV_INFO_CALL_FAILED,
    e_DEV_INFO_LANGUAGE,
    e_DEV_INFO_YST_INFO,
    e_DEV_INFO_PRODUCT_INFO,
    e_DEV_INFO_ALARM_DEV_ID,
    e_DEV_INFO_IVP_RULE,
    e_DEV_INFO_IVP_BASIC,
    e_DEV_INFO_USER_STATE,   //携带DEV_USER_STATE
    e_DEV_INFO_MEDIA_URL_INFO,
    e_DEV_INFO_USER_PWD, //传出用户账号获取密码,返回密码长度,密码放入缓冲区
	e_DEV_INFO_IPPOWER,
}DEV_INFO_TYPEs;

typedef struct
{
    char sps[32];
    char pps[2][8];
}SPS_PPS;

typedef struct
{
    int level;
    char name[MAX_MNG_NAME_LEN];
    char description[MAX_MNG_DESCRIPTION_LEN];
    char pwd[MAX_MNG_PWD_LEN];
}MNG_ACC;

typedef enum
{
    e_NALU_TYPE_PSLICE = 1, /*PSLICE types*/
    e_NALU_TYPE_ISLICE = 5, /*ISLICE types*/
    e_NALU_TYPE_SEI    = 6, /*SEI types*/
    e_NALU_TYPE_SPS    = 7, /*SPS types*/
    e_NALU_TYPE_PPS    = 8, /*PPS types*/
    e_NALU_TYPE_AUDIO = 100,   
}NALU_TYPEs;

typedef enum
{
    e_H265_NALU_TYPE_PSLICE = 1, /*PSLICE types*/
    e_H265_NALU_TYPE_ISLICE = 19, /*ISLICE types*/
    e_H265_NALU_TYPE_VPS	= 32,
    e_H265_NALU_TYPE_SPS    = 33, /*SPS types*/
    e_H265_NALU_TYPE_PPS    = 34, /*PPS types*/
    e_H265_NALU_TYPE_SEI    = 39, /*SEI types*/
}H265_NALU_TYPEs;

typedef enum
{
    AC_NONE,
    AC_G711A,
    AC_G711U,
    AC_ADPCM,
    AC_G726,
    AC_AMR,
    AC_AMRDTX,
    AC_AAC,    
    AC_G722,
    AC_G7231,
    AC_G728,
    AC_G729,
    AC_MP1,
    AC_MP2,
    AC_MP3,
    AC_MPEG2,
    AC_AC3,
    AC_TOTAL
}AUDIO_CODEC;

typedef enum
{
    VC_H261,
    VC_H263,
    VC_MPEG2,
    VC_MPEG4,
    VC_H264,
    VC_MJPEG,
    VC_AVS,
    VC_H265,
    VC_TOTAL,
}VIDEO_CODEC;

typedef enum
{
    e_DEV_USER_STATE_CONNECTED,
    e_DEV_USER_STATE_AUTH_FAILED,
    e_DEV_USER_STATE_SETUP,
    e_DEV_USER_STATE_PLAY,
    e_DEV_USER_STATE_PAUSE,
    e_DEV_USER_STATE_STOP,
}DEV_USER_STATEs;

typedef struct
{
    int type;  //0=rtsp user
    int st;   //DEV_USER_STATEs
    int chn; 
    int stream; 
    int media; //0-audio, 1-video
    long ip;
    long id;
    char acc[32];//
}DEV_USER_STATE;

typedef struct
{
    int chn;
    int cmd;
    int stream;
}DEV_VIDOE_CTRL;

typedef enum
{
    e_MEDIA_CTRL_START,
    e_MEDIA_CTRL_STOP,
    e_MDEIA_CTRL_NEED_IFRAME,
}VIDEO_CTRL_CMDs;

typedef struct
{
    char *url;
    int chn;
    int media;
    int stream;
}DEV_MEDIA_URL_INFO;

typedef struct
{
	int sampleRate;
 	int MicDev;
 }AUDIO_CODEC_ATTR;


typedef int (*DEV_GET_INFO)(DEV_INFO_TYPEs type, char *info, int chn, int platformId);
typedef int (*DEV_SET_INFO)(DEV_INFO_TYPEs type, char *info, int paraLen);

/*****************************************************************************************************
int serviceStart(int rtspPort, int sipPort, DEV_GET_INFO get, DEV_SET_INFO set, void *data);
功能:
        启动rtsp sever,注册参数获取设置回调
参数:
rtspPort: rtsp 服务器端口
sipPort: 保留
get: 注册参数获取回调
set: 注册参数设置回调
data:保留

int serviceStop();
功能:
    释放rtsp server

int transStream(int chn, unsigned char *data, int dataLen, long pts, int naluType, int streamType,  long pktInfo);
功能:
    发送码流数据
参数:
chn:通道号,主次码流使用同一个通道号，streamtype区分
data:数据缓冲区地址
dataLen:数据长度
naluType: nalu类型NALU_TYPEs
streamType:码流类型，0-主码流，1-子码流
pktInfo:
*/
int serviceStart(int rtspPort, int sipPort, DEV_GET_INFO get, DEV_SET_INFO set, void *data);
int serviceStop();
int transStream(int chn, unsigned char *data, int dataLen, long pts, int naluType, int streamType,  long pktInfo);


int transStream_ex(int channel, char *data, int dataLen, long pts, int streamType,  long pktInfo);

int TransExternData(char channel, char *data, int dataLen ,long pts);

#ifdef __cplusplus
}
#endif
#endif

