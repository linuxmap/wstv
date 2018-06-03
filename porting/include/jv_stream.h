/**
 *@file jv_stream.h file about stream 
 * define the interface of stream.
 *@author Liu Fengxiang
 */

#ifndef _JV_STREAM_H_
#define _JV_STREAM_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "jv_common.h"

#define MAX_ROI_REGION	4

/** 
 *@brief resolution supported (copied from nxp, videocomm.c : 
 index: 0, wEncWidth: 176 x 120 rate: 1.466667   		// QSIF
index: 1, wEncWidth: 352 x 240 rate: 1.466667		// SIF
index: 2, wEncWidth: 704 x 240 rate: 2.933333		// 4SIF
index: 3, wEncWidth: 704 x 480 rate: 1.466667		// P4SIF
index: 4, wEncWidth: 88 x 60 rate: 1.466667		// MSIF
index: 5, wEncWidth: 176 x 144 rate: 1.222222		// QCIF
index: 6, wEncWidth: 352 x 288 rate: 1.222222		// CIF
index: 7, wEncWidth: 704 x 288 rate: 2.444444		// 4CIF
index: 8, wEncWidth: 704 x 576 rate: 1.222222		// P4CIF
index: 9, wEncWidth: 88 x 72 rate: 1.222222		// MCIF
index: 10, wEncWidth: 160 x 120 rate: 1.333333		// QMGA
index: 11, wEncWidth: 320 x 240 rate: 1.333333		// MGA
index: 12, wEncWidth: 640 x 240 rate: 2.666667		// VGA
index: 13, wEncWidth: 640 x 480 rate: 1.333333		// PVGA
index: 14, wEncWidth: 80 x 60 rate: 1.333333		// MVGA
index: 15, wEncWidth: 800 x 600 rate: 1.333333		// SVGA
index: 16, wEncWidth: 960 x 720 rate: 1.333333
index: 17, wEncWidth: 1024 x 768 rate: 1.333333
index: 18, wEncWidth: 1008 x 1008 rate: 1.000000
index: 19, wEncWidth: 1280 x 1024 rate: 1.250000		// SXGA
index: 20, wEncWidth: 1600 x 1200 rate: 1.333333		// UXGA
index: 21, wEncWidth: 2560 x 1920 rate: 1.333333		// UXGA
index: 22, wEncWidth: 1280 x 720 rate: 1.777778		// vf720p
index: 23, wEncWidth: 1280 x 960 rate: 1.333333		// vfxvga
index: 24, wEncWidth: 1280 x 800 rate: 1.600000		// wxga
index: 25, wEncWidth: 720 x 480 rate: 1.500000		// NTSC
index: 26, wEncWidth: 1920 x 1080 rate: 1.777778		// HD1080
index: 27, wEncWidth: 480 x 480 rate: 1.000000		// vfTest
index: 28, wEncWidth: 432 x 480 rate: 0.900000		// vfTest2
index: 29, wEncWidth: 240 x 160 rate: 1.500000		// vfTest3
index: 30, wEncWidth: 432 x 240 rate: 1.800000		// vfTest4
index: 31, wEncWidth: 360 x 480 rate: 0.750000		// vfTest5
index: 32, wEncWidth: 720 x 576 rate: 1.250000		//PAL
index: 33, wEncWidth: 638 x 480 rate: 1.329167		//vfTest6
index: 34, wEncWidth: 720 x 405 rate: 1.777778		//vfTest7
index: 35, wEncWidth: 180 x 120 rate: 1.500000		//vfTest8
index: 36, wEncWidth: 360 x 240 rate: 1.500000		//vfTest9
index: 37, wEncWidth: 2048 x 1536 rate: 1.333333		//vfTest10

*/

typedef enum{
	JV_PT_H264 = 0,
	JV_PT_H265 = 1,
	JV_PT_JPEG = 2,
	JV_PT_MJPEG = 3,
	JV_PT_MAX
}VENC_TYPE;

typedef enum
{
	JV_VENC_RC_MODE_CBR,
	JV_VENC_RC_MODE_VBR,
	JV_VENC_RC_MODE_FIXQP
}jv_venc_rc_mode_e;

typedef struct
{
	int ior_reverse;				//是否反选
	RECT roi[MAX_ROI_REGION];
	int roiWeight;				///< 权重。其值介于  0 ~ 51之间，一般在0～18之间即可
}jv_stream_roi;

/**
 *@brief stream info for create
 *
 */
typedef struct 
{
	unsigned int	width;		///< resolution width
	unsigned int	height;		///< resolution height
	unsigned int	framerate;	///< framerate such as 25, 30 ...
	unsigned int	nGOP;		///< interval between two key frame

	jv_venc_rc_mode_e rcMode;
	unsigned int	bitrate;		///< KBit per second
	int maxQP;	//VBR时maxQP和minQP
	int minQP;

	BOOL bRectStretch; //对于16：9与4：3之间的差异，是拉伸，还是裁剪。为真时表示拉伸
	//感 兴趣区域编码
	jv_stream_roi roiInfo;
	VENC_TYPE vencType;		// 视频编码协议类型
}jv_stream_attr;

/**
 *@brief frame type definiton
 *
 */
typedef enum
{
	JV_FRAME_TYPE_I,
	JV_FRAME_TYPE_P,
	JV_FRAME_TYPE_B,
	JV_FRAME_TYPE_A,	///< 音频帧
	JV_FRAME_TYPE_VI,       //virtual I
	JV_FRAME_TYPE_MAX
}jv_frame_type_e;
#if (defined PLATFORM_hi3518EV200)
#define MAX_FRAME_LEN		(300*1024)
#else
#define MAX_FRAME_LEN		(512*1024)

#endif
/**
 *@brief frame info definition
 */
typedef struct 
{
	unsigned char	buffer[MAX_FRAME_LEN];
	int				len;
	jv_frame_type_e	frametype;
	unsigned long long timestamp;
	int streamid;
}jv_frame_info_t;

typedef struct {
	int width;
	int height;
}resolution_t;

typedef struct
{
	BOOL bEnable;
	int crop_x;
	int crop_y;
	int crop_w;
	int crop_h;
}venc_crop_t;
//VGS画线类型
enum
{
	GRAPH_TYPE_NONE,
	GRAPH_TYPE_LINE,
	GRAPH_TYPE_RECT,
	GRAPH_TYPE_WORDS,
	GRAPH_TYPE_POLYGON,//多边形
};
#define MAX_POINT_NUM 10
//点结构体
typedef struct
{
    int x;
    int y;
}POINT_s;
//多边形结构体定义
typedef  struct 
{
	int						nCnt;						//点的个数>=2,=2时是绊线
	POINT_s					stPoints[MAX_POINT_NUM];		//图形的点的坐标
}Polygon_s;
//VGS 图形结构体定义
typedef struct
{
	unsigned int	graph_type;
	unsigned int	color;			// rgb888
	unsigned int	linew;
	union
	{
		struct
		{
			unsigned int	x1;
			unsigned int	y1;
			unsigned int	x2;
			unsigned int	y2;
		}Line;
		RECT	Rect;
		struct
		{
			RECT			rect;
			unsigned int	phyaddr;
		}Bitmap;
		Polygon_s Polygon;


		
	};
}GRAPH_t;
//VGS获取图形回调
typedef void (*jv_stream_get_graph)(int channelid, GRAPH_t* graph, int* cnt, int basew, int baseh);

/**
 *@brief callback when data received
 *@param channelid id of the stream
 *@param frame_info frame information
 *@param callback_param the value jv_stream_open setted 
 *
 */
typedef void (*jv_stream_callback_t)(int channelid, jv_frame_info_t *frame_info, void *callback_param);

/**
 *@brief do initialize of stream
 *@return 0 if success.
 *
 */
int jv_stream_init(void);

/**
 *@brief do de-initialize of stream
 *@return 0 if success.
 *
 */
int jv_stream_deinit(void);

/**
 *@brief start a stream 
 *@param channelid id of the stream
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_start(int channelid);

/**
 *@brief stop a stream 
 *@param channelid id of the stream
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_stop(int channelid);

#if 1
typedef struct
{
	int channelid;
	int received;
}jv_stream_pollfd;

/**
 *@brief check if data received
 *@param fd a list of channelid, when data received, fd[n].received will be set TRUE
 *@param cnt count of fd to check received data
 *@param timeout an upper limit the func will block, in milliseconds.  -1 means no timeout
 *
 */
int jv_stream_poll(jv_stream_pollfd fd[], int cnt, int timeout);

/**
 *@brief Read one frame data from the channel 
 *@param channelid Id of the stream
 *@param Info frame info
 *
 *@note The function will block untill data is comming or #jv_stream_stop is called
 *@note After parsed the data, #jv_stream_release should be call to release the source
 *@note some times, info->len maybe 0, and that means no data received. You can just ignore it
 *
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_read(int channelid, jv_frame_info_t *info);


#ifdef ZRTSP_SUPPORT
#include "libzrtsp.h"
int jv_stream_get_spspps(int chn,SPS_PPS *rtsp_sps_pps);
NALU_TYPEs jv_stream_get_nalu_tpye();
#endif

/**
 *@brief Read one frame data from the channel 
 *@param channelid Id of the stream
 *@param Info frame info
 *
 *@note The function will block untill data is comming or #jv_stream_stop is called
 *
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_release(int channelid, jv_frame_info_t *info);

#endif

#if 1
/**
 *@brief set stream attribute
 *@param channelid id of the stream
 *@param attr pointer of attribution
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_set_attr(int channelid, jv_stream_attr *attr);

/**
 *@brief get stream attribute
 *@param channelid id of the stream
 *@param attr pointer of attribution
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_get_attr(int channelid, jv_stream_attr *attr);

#else
/**
 *@brief set framerate of the stream 
 *@param channelid id of the stream
 *@param framerate framerate of the stream, value in {30, 25, 20, 15, 10, 5}
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_set_framerate(int channelid, int framerate);

/**
 *@brief set framerate of the stream 
 *@param channelid id of the stream
 *@param bitrate bits per second
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_set_bitrate(int channelid, int framerate);

/**
 *@brief set resolution of the stream 
 *@param channelid id of the stream
 *@param width resoultion width
 *@param height resolution height
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_set_resolution(int channelid, int width, int height);
#endif

typedef struct{
	resolution_t *resList;//输出分辨率的列表
	int resListCnt;//输出分辨率的可选个数
	int maxNGOP;//最大nGOP
	int minNGOP;//最小nGOP
	int maxFramerate;//最大帧率
	int minFramerate;//最小帧率
	int maxKBitrate;//最大码率，单位为KBit
	int minKBitrate;//最小码率
	resolution_t inputRes;//输入分辨率
	int maxStreamRes[MAX_STREAM]; //1，2，3码流的最大分辨率。用于从resList中查找各自码流能够适用的分辨率
							//其取值类似这样：{1280*720, 720*576, 352*288}

	int vencTypeNum;	//编码类型代码：1.264;2.264 265;...
	int maxRoi;//感兴趣区域编码所支持的个数。0表示不支持
	venc_crop_t vencCrop;
}jvstream_ability_t;

/**
 *@brief 获取编码相关的各项指标
 *@param ability 输出 编码相关的各项指标
 *
 *@return 0
 *
 */
int jv_stream_get_ability(int channelid, jvstream_ability_t *ability);

/**
 *@brief 向解码器申请一帧关键帧
 *在录像开始或者有客户端连接时调用此函数
 *
 *@param channelid 通道号，表示要申请此通道输出关键帧
 *
 */
void jv_stream_request_idr(int channelid); 

/**
 *@获取码流的输入端的宽度和高度
 *@param channelid 输入 通道号
 *@param width 输出：宽度指针
 *@param height 输出：高度指针
 *
 *@return 0 if success
 *
 */
int jv_stream_get_vi_resolution(int channelid, unsigned int *width, unsigned int *height);

/**
 * 获取感兴趣区域信息
 */
int jv_stream_get_roi(int channelid, jv_stream_roi *roi);
/**
 * 设置感兴趣区域信息
 */
int jv_stream_set_roi(int channelid, jv_stream_roi *roi);

/**
 * 降低码率
 */
int jv_stream_switch_lowbitrate(int channelid, unsigned int bitrate);

/**
 * 恢复码率
 */
int jv_stream_revert_bitrate(int channelid);
//设置VGS图形回调
int jv_stream_set_graph_callback(jv_stream_get_graph callback);

#ifdef __cplusplus
}
#endif
#endif

