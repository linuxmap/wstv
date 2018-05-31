#include "jv_common.h"
#include "jv_stream.h"
#include "hicommon.h"
#include <jv_sensor.h>

#ifdef ZRTSP_SUPPORT
SPS_PPS jvstream_rtsp[3];
NALU_TYPEs rtsp_nalu_type;
int rtsp_offset;
pthread_mutex_t rtsp_mutex;
#endif

extern BOOL VI_CROP_ENABLE;

extern int VI_CROP_X;
extern int VI_CROP_Y;
extern int VI_CROP_W;
extern int VI_CROP_H;


typedef struct
{
	int fd;

	jv_stream_attr attr;
	BOOL bOpened;

} jvstream_status_t;

static jvstream_status_t ststatus[MAX_STREAM+1];
static pthread_mutex_t stream_mutex;

#define __VENC_SAMPLE__

#ifdef  __VENC_SAMPLE__
typedef enum sample_rc_e
{
    SAMPLE_RC_CBR = 0,
    SAMPLE_RC_VBR,
    SAMPLE_RC_FIXQP
} SAMPLE_RC_E;
VENC_GRP VencGrp[3] = {0,1,2};
VENC_CHN VencChn[3] = {0,1,2};
VPSS_CHN VpssChn[3] = {0,1,2};

//lck20121102
//将IPC的分辨率更改为以下几种，统一比例都是16:9

static resolution_t s_res_list_mmm[] =
{
	{2592, 1520},
	{2048, 1520},
	{1920, 1080},
	{1280,	960},
	{1280,	720},
	{960,	540},
	{768,	432},
	{720,	576},
	{720,   480},
	{704,	576},
	{640,	480},
//	{624,	352},
	{512,	288},
	{352,	288},
	{368,	208},
//	{176,	144},
	{0, 0}
};

static resolution_t *s_res_list;
static int s_res_cnt;

//以VI为基础，计算。因为通道0的VPSS不能缩放
static void __build_vpss_venc_size_vi(int vencw, int vench, SIZE_S *vpssSize, VENC_CROP_CFG_S *stGrpCropCfg)
{
	int expw, exph; //锁定比例时，期望的宽度和高度，以压缩的宽高为基准

	expw = VI_HEIGHT * vencw / vench;
	exph = VI_WIDTH * vench / vencw;

	if (expw - VI_WIDTH >= 32)
	{
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = 0;//JV_ALIGN_CEILING((expw - vencw)/2,16);
		stGrpCropCfg->stRect.s32Y = JV_ALIGN_CEILING((VI_HEIGHT - exph)/2, 2);
		stGrpCropCfg->stRect.u32Width = VI_WIDTH;
		stGrpCropCfg->stRect.u32Height = exph;
		vpssSize->u32Width = VI_WIDTH;//2 * stGrpCropCfg->stRect.s32X + vencw;//JV_ALIGN_CEILING(expw, 16);
		vpssSize->u32Height = VI_HEIGHT;
	}
	else if (exph - VI_HEIGHT > 10)
	{
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = JV_ALIGN_CEILING((VI_WIDTH-expw)/2,16);
		stGrpCropCfg->stRect.s32Y = 0;
		stGrpCropCfg->stRect.u32Width = expw;
		stGrpCropCfg->stRect.u32Height = VI_HEIGHT;
		vpssSize->u32Width = VI_WIDTH;
		vpssSize->u32Height = VI_HEIGHT;
	}
	else
	{
		memset(stGrpCropCfg, 0, sizeof(VENC_CROP_CFG_S));
		vpssSize->u32Width = VI_WIDTH;
		vpssSize->u32Height = VI_HEIGHT;
	}
	printf("0:x,y,w,h: %d,%d,%d,%d\n", stGrpCropCfg->stRect.s32X, stGrpCropCfg->stRect.s32Y, stGrpCropCfg->stRect.u32Width, stGrpCropCfg->stRect.u32Height);
	printf("0:vpss: %d,%d\n", vpssSize->u32Width, vpssSize->u32Height);
}

static void __build_vpss_venc_size(int vencw, int vench, int vencw_0,int vench_0, SIZE_S *vpssSize, VENC_CROP_CFG_S *stGrpCropCfg)
{
	SIZE_S tmpvpssSize;
	VENC_CROP_CFG_S tmpstGrpCropCfg;
	__build_vpss_venc_size_vi(vencw_0, vench_0, &tmpvpssSize, &tmpstGrpCropCfg);

	stGrpCropCfg->bEnable = FALSE;
	if(tmpstGrpCropCfg.stRect.s32X==0)
		vpssSize->u32Width = vencw;		//主码流场景宽未裁剪，次码流直接压缩为设置宽度
	else
	{	//宽被裁剪
		HI_S32 tmpX = (vencw*tmpstGrpCropCfg.stRect.s32X/tmpstGrpCropCfg.stRect.u32Width);
		tmpX = JV_ALIGN_CEILING(tmpX,16);
		vpssSize->u32Width = vencw + tmpX*2;
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = tmpX;
		stGrpCropCfg->stRect.s32Y = 0;
		stGrpCropCfg->stRect.u32Width = vencw;
		stGrpCropCfg->stRect.u32Height = vench;
	}

	if(tmpstGrpCropCfg.stRect.s32Y==0)
		vpssSize->u32Height = vench;		//主码流场景高未裁剪，次码流直接压缩为设置高度
	else
	{
		HI_S32 tmpY = (vench*tmpstGrpCropCfg.stRect.s32Y/tmpstGrpCropCfg.stRect.u32Height);
		tmpY = JV_ALIGN_CEILING(tmpY,16);
		vpssSize->u32Height = vench + tmpY*2;
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = 0;
		stGrpCropCfg->stRect.s32Y = tmpY;
		stGrpCropCfg->stRect.u32Width = vencw;
		stGrpCropCfg->stRect.u32Height = vench;
	}
#if 0
	int expw, exph; //锁定比例时，期望的宽度和高度，以压缩的宽高为基准

	expw = vench * VI_WIDTH / VI_HEIGHT;
	exph = vencw * VI_HEIGHT / VI_WIDTH;

	if (expw - vencw >= 32)
	{
		unsigned int x =  JV_ALIGN_CEILING((expw - vencw)/2, 16);
		vpssSize->u32Width = 2 * x + vencw;//JV_ALIGN_CEILING(expw, 16);
		vpssSize->u32Height = vench;
	}
	else if (exph - vench > 10)
	{
		unsigned int y = JV_ALIGN_CEILING((exph - vench)/2, 2);
		vpssSize->u32Width = vencw;
		vpssSize->u32Height = 2 * y + vench;
	}
	else
	{
		memset(stGrpCropCfg, 0, sizeof(VENC_CROP_CFG_S));
		vpssSize->u32Width = vencw;
		vpssSize->u32Height = vench;
	}

	int VPSS_W = vpssSize->u32Width;
	int VPSS_H = vpssSize->u32Height;

	int expw_crop,exph_crop;
	expw_crop = VPSS_H * vencw_0 / vench_0;
	exph_crop = VPSS_W * vench_0 / vencw_0;

	if (expw_crop - VPSS_W>= 32)
	{
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = 0;
		stGrpCropCfg->stRect.s32Y = JV_ALIGN_CEILING((vpssSize->u32Height - exph_crop)/2,2);
		stGrpCropCfg->stRect.u32Width = vpssSize->u32Width;
		stGrpCropCfg->stRect.u32Height = exph_crop;
	}
	else if (exph_crop - VPSS_H> 10)
	{
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = JV_ALIGN_CEILING((vpssSize->u32Width-expw_crop)/2,16);
		stGrpCropCfg->stRect.s32Y = 0;
		stGrpCropCfg->stRect.u32Width = expw_crop;
		stGrpCropCfg->stRect.u32Height = vpssSize->u32Height;
	}
	else
		stGrpCropCfg->bEnable = FALSE;
#endif

	printf("x,y,w,h: %d,%d,%d,%d\n", stGrpCropCfg->stRect.s32X, stGrpCropCfg->stRect.s32Y, stGrpCropCfg->stRect.u32Width, stGrpCropCfg->stRect.u32Height);
	printf("vpss: %d,%d\n", vpssSize->u32Width, vpssSize->u32Height);
}

int srcFramerate = 1;
#undef SAMPLE_PRT
#define SAMPLE_PRT CPrintf
/******************************************************************************
* funciton : Start venc stream mode (h264, mjpeg)
* note      : rate control parameter need adjust, according your case.
******************************************************************************/
HI_S32 SAMPLE_COMM_VENC_Start(jv_stream_attr *attr ,VENC_CHN VencChn, PAYLOAD_TYPE_E enType, VIDEO_NORM_E enNorm, SIZE_S *pstSize, SAMPLE_RC_E enRcMode, int bitrate, int framerate, int nGop)
{
	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stVencChnAttr;
	SIZE_S stPicSize;

	stPicSize = *pstSize;

	float fvif;
	int blow = jv_sensor_get_b_low_frame(0, &fvif, NULL);
	if (fvif < attr->framerate)
		fvif = attr->framerate;

	if (srcFramerate > 25 && fvif<srcFramerate && !blow)
		fvif = srcFramerate;

//	if (framerate > srcFramerate)
//		framerate = srcFramerate;

	/******************************************
	 step 1:  Create Venc Channel
	******************************************/
	stVencChnAttr.stVeAttr.enType = enType;
	switch(enType)
	{
	case PT_H264:
	{
		VENC_ATTR_H264_S stH264Attr;
		stH264Attr.u32MaxPicWidth = stPicSize.u32Width;
		stH264Attr.u32MaxPicHeight = stPicSize.u32Height;
		stH264Attr.u32PicWidth = stPicSize.u32Width;/*the picture width*/
		stH264Attr.u32PicHeight = stPicSize.u32Height;/*the picture height*/
		stH264Attr.u32BufSize  = stPicSize.u32Width * stPicSize.u32Height * 2;/*stream buffer size*/
		stH264Attr.u32Profile  = 1;/*0: baseline; 1:MP; 2:HP   ? */
		stH264Attr.bByFrame = HI_TRUE;/*get stream mode is slice mode or frame mode?*/
		stH264Attr.u32BFrameNum = 0;/* 0: not support B frame; >=1: number of B frames */
		stH264Attr.u32RefNum = 1;/* 0: default; number of refrence frame*/
		memcpy(&stVencChnAttr.stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));

		if(SAMPLE_RC_CBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
			VENC_ATTR_H264_CBR_S stH264Cbr;
			stH264Cbr.u32Gop            = nGop;
			stH264Cbr.u32StatTime       = (nGop+framerate-1)/framerate; /* stream rate statics time(s) */
			stH264Cbr.u32SrcFrmRate     = fvif;//(VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;/* input (vi) frame rate */
			stH264Cbr.fr32DstFrmRate = framerate;
			stH264Cbr.u32BitRate = bitrate;

			stH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
			memcpy(&stVencChnAttr.stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
		}
		else if (SAMPLE_RC_FIXQP == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
			VENC_ATTR_H264_FIXQP_S stH264FixQp;
			stH264FixQp.u32Gop = nGop;
			stH264FixQp.u32SrcFrmRate = fvif;//(VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264FixQp.fr32DstFrmRate = framerate;
			stH264FixQp.u32IQp = 20;
			stH264FixQp.u32PQp = 23;
			memcpy(&stVencChnAttr.stRcAttr.stAttrH264FixQp, &stH264FixQp,sizeof(VENC_ATTR_H264_FIXQP_S));
		}
		else if (SAMPLE_RC_VBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
			VENC_ATTR_H264_VBR_S stH264Vbr;
			stH264Vbr.u32Gop = nGop;
			stH264Vbr.u32StatTime = (nGop+framerate-1)/framerate;
			stH264Vbr.u32SrcFrmRate = fvif;//(VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264Vbr.fr32DstFrmRate = framerate;
			stH264Vbr.u32MinQp = attr->minQP;
			stH264Vbr.u32MaxQp = attr->maxQP;
			stH264Vbr.u32MaxBitRate = bitrate;
			memcpy(&stVencChnAttr.stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
		}
		else
		{
			return HI_FAILURE;
		}

		VENC_PARAM_H264_VUI_S pstH264Vui;
		HI_MPI_VENC_GetH264Vui(VencChn, &pstH264Vui);
		pstH264Vui.stVuiTimeInfo.timing_info_present_flag = 1;
		pstH264Vui.stVuiTimeInfo.num_units_in_tick = 1000;
		pstH264Vui.stVuiTimeInfo.time_scale = framerate * pstH264Vui.stVuiTimeInfo.num_units_in_tick * 2;
		pstH264Vui.stVuiTimeInfo.fixed_frame_rate_flag = 0;
		HI_MPI_VENC_SetH264Vui(VencChn, &pstH264Vui);
	}
	break;

	case PT_H265:
	{
		VENC_ATTR_H265_S stH265Attr;
		stH265Attr.u32MaxPicWidth = stPicSize.u32Width;
		stH265Attr.u32MaxPicHeight = stPicSize.u32Height;
		stH265Attr.u32PicWidth = stPicSize.u32Width;/*the picture width*/
		stH265Attr.u32PicHeight = stPicSize.u32Height;/*the picture height*/
		stH265Attr.u32BufSize  = stPicSize.u32Width * stPicSize.u32Height * 2;/*stream buffer size*/
		stH265Attr.u32Profile  = 0;/*0:MP; */
		stH265Attr.bByFrame = HI_TRUE;/*get stream mode is slice mode or frame mode?*/
		stH265Attr.u32BFrameNum = 0;/* 0: not support B frame; >=1: number of B frames */
		stH265Attr.u32RefNum = 1;/* 0: default; number of refrence frame*/
		memcpy(&stVencChnAttr.stVeAttr.stAttrH265e, &stH265Attr, sizeof(VENC_ATTR_H265_S));

		if(SAMPLE_RC_CBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
			VENC_ATTR_H265_CBR_S stH265Cbr;
			stH265Cbr.u32Gop            = nGop;
			stH265Cbr.u32StatTime       = (nGop+framerate-1)/framerate; /* stream rate statics time(s) */
			stH265Cbr.u32SrcFrmRate     = fvif;//(VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;/* input (vi) frame rate */
			stH265Cbr.fr32DstFrmRate = framerate;
			stH265Cbr.u32BitRate = bitrate;

			stH265Cbr.u32FluctuateLevel = 0; /* average bit rate */
			memcpy(&stVencChnAttr.stRcAttr.stAttrH265Cbr, &stH265Cbr, sizeof(VENC_ATTR_H265_CBR_S));
		}
		else if (SAMPLE_RC_FIXQP == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265FIXQP;
			VENC_ATTR_H265_FIXQP_S stH265FixQp;
			stH265FixQp.u32Gop = nGop;
			stH265FixQp.u32SrcFrmRate = fvif;//(VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH265FixQp.fr32DstFrmRate = framerate;
			stH265FixQp.u32IQp = 20;
			stH265FixQp.u32PQp = 23;
			memcpy(&stVencChnAttr.stRcAttr.stAttrH265FixQp, &stH265FixQp,sizeof(VENC_ATTR_H265_FIXQP_S));
		}
		else if (SAMPLE_RC_VBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
			VENC_ATTR_H265_VBR_S stH265Vbr;
			stH265Vbr.u32Gop = nGop;
			stH265Vbr.u32StatTime = (nGop+framerate-1)/framerate;
			stH265Vbr.u32SrcFrmRate = fvif;//(VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH265Vbr.fr32DstFrmRate = framerate;
			stH265Vbr.u32MinQp = attr->minQP;
			stH265Vbr.u32MaxQp = attr->maxQP;
			stH265Vbr.u32MaxBitRate = bitrate;
			memcpy(&stVencChnAttr.stRcAttr.stAttrH265Vbr, &stH265Vbr, sizeof(VENC_ATTR_H265_VBR_S));
		}
		else
		{
			return HI_FAILURE;
		}

		VENC_PARAM_H265_TIMING_S pstH265Timing;
		HI_MPI_VENC_GetH265Timing(VencChn, &pstH265Timing);
		pstH265Timing.timing_info_present_flag = 1;
		pstH265Timing.num_units_in_tick = 1000;
		pstH265Timing.time_scale = framerate * pstH265Timing.num_units_in_tick * 2;
		HI_MPI_VENC_SetH265Timing(VencChn, &pstH265Timing);

	}
	break;

	case PT_MJPEG:
	{
		VENC_ATTR_MJPEG_S stMjpegAttr;
		stMjpegAttr.u32MaxPicWidth = stPicSize.u32Width;
		stMjpegAttr.u32MaxPicHeight = stPicSize.u32Height;
		stMjpegAttr.u32PicWidth = stPicSize.u32Width;
		stMjpegAttr.u32PicHeight = stPicSize.u32Height;
		stMjpegAttr.u32BufSize = stPicSize.u32Width * stPicSize.u32Height * 2;
		stMjpegAttr.bByFrame = HI_TRUE;  /*get stream mode is field mode  or frame mode*/
		//stMjpegAttr.bMainStream = HI_TRUE;  /*main stream or minor stream types?*/
		//stMjpegAttr.bVIField = HI_FALSE;  /*the sign of the VI picture is field or frame?*/
		//stMjpegAttr.u32Priority = 0;/*channels precedence level*/
		memcpy(&stVencChnAttr.stVeAttr.stAttrMjpeg, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));

		if(SAMPLE_RC_FIXQP == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;
			VENC_ATTR_MJPEG_FIXQP_S stMjpegeFixQp;
			stMjpegeFixQp.u32Qfactor        = 90;
			stMjpegeFixQp.u32SrcFrmRate      = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stMjpegeFixQp.fr32DstFrmRate = framerate;
			memcpy(&stVencChnAttr.stRcAttr.stAttrMjpegeFixQp, &stMjpegeFixQp,
			       sizeof(VENC_ATTR_MJPEG_FIXQP_S));
		}
		else if (SAMPLE_RC_CBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
			stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32StatTime       = 1;
			stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32SrcFrmRate      = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stVencChnAttr.stRcAttr.stAttrMjpegeCbr.fr32DstFrmRate = framerate;
			stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32FluctuateLevel = 0;
			stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = bitrate;
		}
		else if (SAMPLE_RC_VBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGVBR;
			stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32StatTime = 1;
			stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == enNorm)?25:30;
			stVencChnAttr.stRcAttr.stAttrMjpegeVbr.fr32DstFrmRate = 5;
			stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MinQfactor = 50;
			stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxQfactor = 95;
			stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = bitrate;
		}
		else
		{
			SAMPLE_PRT("cann't support other mode in this version!\n");

			return HI_FAILURE;
		}
	}
	break;

	case PT_JPEG:
	{
		VENC_ATTR_JPEG_S stJpegAttr;
		stJpegAttr.u32PicWidth  = stPicSize.u32Width;
		stJpegAttr.u32PicHeight = stPicSize.u32Height;
		stJpegAttr.u32BufSize = stPicSize.u32Width * stPicSize.u32Height * 2;
		stJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
		stJpegAttr.bSupportDCF = HI_FALSE;
		memcpy(&stVencChnAttr.stVeAttr.stAttrJpeg, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));
	}
	break;
	default:
		return HI_ERR_VENC_NOT_SUPPORT;
	}

	s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n",\
		           VencChn, s32Ret);
		return s32Ret;
	}

	VENC_RC_PARAM_S rcPara;
	HI_MPI_VENC_GetRcParam(VencChn, &rcPara);
	if(enType == PT_H264)
	{
		HI_U32 thrdI[12] = {7,7,7,7,7,9,9,9,12,15,18,25};
		HI_U32 thrdP[12] = {7,7,7,7,7,9,9,9,12,15,18,25};
		memcpy(rcPara.u32ThrdI, thrdI, sizeof(rcPara.u32ThrdI));
		memcpy(rcPara.u32ThrdP, thrdP, sizeof(rcPara.u32ThrdP));
	}
	else if(enType == PT_H265)
	{
		HI_U32 thrdI[12] = {3,3,5,5,8,8,8,15,20,20,25,25};
		HI_U32 thrdP[12] = {3,3,5,5,8,8,8,15,20,20,25,25};
		memcpy(rcPara.u32ThrdI, thrdI, sizeof(rcPara.u32ThrdI));
		memcpy(rcPara.u32ThrdP, thrdP, sizeof(rcPara.u32ThrdP));
	}

//	printf("=================================================u32RowQpDelta:%d\n",rcPara.u32RowQpDelta);
	rcPara.u32RowQpDelta = 1;
	if (SAMPLE_RC_VBR == enRcMode)
	{
//		printf("=================================================stParamH264VBR_GET:%d,%d,%d,%d\n",rcPara.stParamH264VBR.s32IPQPDelta,rcPara.stParamH264VBR.u32MaxIprop,rcPara.stParamH264VBR.u32MinIprop,rcPara.stParamH264VBR.s32ChangePos);
#if 0
//		rcPara.stParamH264VBR.u32MinIQP = 3;
		if(enType == PT_H264)
		{
			rcPara.stParamH264VBR.s32IPQPDelta = 10;
			rcPara.stParamH264VBR.u32MinIprop = 5;
//			rcPara.stParamH264VBR.u32MaxIprop = 13;
			rcPara.stParamH264VBR.s32ChangePos = 70;
		}
		else if(enType == PT_H265)
		{
			rcPara.stParamH265Vbr.s32IPQPDelta = 10;
			rcPara.stParamH264VBR.u32MinIprop = 5;
			rcPara.stParamH265Vbr.u32MaxIprop = 13;
			rcPara.stParamH265Vbr.s32ChangePos = 70;
		}
#endif
	}
	else
	{
		//默认值如下：
		//     ID  MinIprop  MaxIprop   MaxQp   MaxStQp   MinQp  MaxPPDltQp  MaxIPDltQp   bLost     LostThr IPDltQp  QLevel  MaxReEncTimes
	    //  0         1       100      51        51      10           3           5       1    83886080       2       0              0
//		rcPara.stParamH264Cbr.u32MinIprop = 3;
//		rcPara.stParamH264Cbr.u32MaxIprop = 8;
//		rcPara.stParamH264Cbr.u32MaxIPDeltaQp = 6;
//		rcPara.stParamH264Cbr.u32MaxPPDeltaQp = 3;
//		rcPara.stParamH264Cbr.bLostFrmOpen = 0;

#if 0
		if(enType == PT_H264)
		{
			//rcPara.stParamH264Cbr.u32MinIprop = 3;
			//rcPara.stParamH264Cbr.u32MaxIprop = 8;
			rcPara.stParamH264Cbr.s32MaxReEncodeTimes = 2;
		}
		else if(enType == PT_H265)
		{
			//rcPara.stParamH265Cbr.u32MinIprop = 3;
			//rcPara.stParamH265Cbr.u32MaxIprop = 8;
			rcPara.stParamH265Cbr.s32MaxReEncodeTimes = 2;
		}

		VENC_PARAM_FRAMELOST_S stFrmLostParam;
		HI_MPI_VENC_GetFrameLostStrategy(VencChn, &stFrmLostParam);
		stFrmLostParam.bFrmLostOpen = HI_FALSE;
		CHECK_RET(HI_MPI_VENC_SetFrameLostStrategy(VencChn, &stFrmLostParam));

		VENC_SUPERFRAME_CFG_S stSuperFrmParam;
		HI_MPI_VENC_GetSuperFrameCfg(VencChn, &stSuperFrmParam);
		stSuperFrmParam.enSuperFrmMode = SUPERFRM_REENCODE;
		CHECK_RET(HI_MPI_VENC_SetSuperFrameCfg(VencChn, &stSuperFrmParam));
#endif
	}
	CHECK_RET(HI_MPI_VENC_SetRcParam(VencChn, &rcPara));

//	__start_trip(VencGrp, stPicSize.u32Width, stPicSize.u32Height);
	/******************************************
	 step 2:  Start Recv Venc Pictures
	******************************************/
	s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;

}

/******************************************************************************
* funciton : Stop venc ( stream mode -- H264, MJPEG )
******************************************************************************/
HI_S32 SAMPLE_COMM_VENC_Stop(VENC_CHN VencChn)
{
	HI_S32 s32Ret;

	/******************************************
	 step 1:  Stop Recv Pictures
	******************************************/
	s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!\n",\
		           VencChn, s32Ret);
		return HI_FAILURE;
	}
	{
		VENC_CROP_CFG_S stGrpCropCfg;
		memset(&stGrpCropCfg, 0, sizeof(stGrpCropCfg));
		stGrpCropCfg.bEnable = FALSE;
		HI_MPI_VENC_SetCrop(VencChn, &stGrpCropCfg);
	}

#if 0
	/******************************************
	 step 2:  UnRegist Venc Channel
	******************************************/
	s32Ret = HI_MPI_VENC_UnRegisterChn(VencChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VENC_UnRegisterChn vechn[%d] failed with %#x!\n",\
		           VencChn, s32Ret);
		return HI_FAILURE;
	}
#endif

	/******************************************
	 step 3:  Distroy Venc Channel
	******************************************/
	s32Ret = HI_MPI_VENC_DestroyChn(VencChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VENC_DestroyChn vechn[%d] failed with %#x!\n",\
		           VencChn, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/******************************************************************************
* function : venc bind vpss
******************************************************************************/
HI_S32 SAMPLE_COMM_VENC_BindVpss(VENC_CHN VencChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
	HI_S32 s32Ret = HI_SUCCESS;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	stSrcChn.enModId = HI_ID_VPSS;
	stSrcChn.s32DevId = VpssGrp;
	stSrcChn.s32ChnId = VpssChn;

	stDestChn.enModId = HI_ID_VENC;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = VencChn;

	s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return s32Ret;
}

/******************************************************************************
* function : venc unbind vpss
******************************************************************************/
HI_S32 SAMPLE_COMM_VENC_UnBindVpss(VENC_CHN VencChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
	HI_S32 s32Ret = HI_SUCCESS;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	stSrcChn.enModId = HI_ID_VPSS;
	stSrcChn.s32DevId = VpssGrp;
	stSrcChn.s32ChnId = VpssChn;

	stDestChn.enModId = HI_ID_VENC;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = VencChn;

	s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return s32Ret;
}

#endif

/**
 *@brief do initialize of stream
 *@return 0 if success.
 *
 */
int jv_stream_init(void)
{
	memset(&ststatus, 0, sizeof(ststatus));

	//对抓拍码流4单独设置
	ststatus[3].attr.width = 720;
	ststatus[3].attr.height = 405;
	ststatus[3].attr.framerate = 10;
	ststatus[3].attr.nGOP = ststatus[3].attr.framerate;
	ststatus[3].attr.bitrate = 10;
	if(VI_CROP_ENABLE)//一码流缩放走这里
		VpssChn[0] = 6;

//	s_res_list_mmm[0].height = VI_HEIGHT;
//	s_res_list_mmm[1].height = VI_HEIGHT;

#if 0
	jv_stream_set_attr(3, &ststatus[3].attr);
	jv_stream_start(3);
#endif
	pthread_mutex_init(&stream_mutex, NULL);

#ifdef ZRTSP_SUPPORT
	pthread_mutex_init(&rtsp_mutex, NULL);
#endif

	s_res_cnt = sizeof(s_res_list_mmm)/sizeof(s_res_list_mmm[0]);
	if (VI_WIDTH==2048)	//300W
	{
		s_res_list = &s_res_list_mmm[1];
		s_res_cnt--;
	}
	else if (VI_HEIGHT == 1080)	//200W
	{
		s_res_list = &s_res_list_mmm[2];
		s_res_cnt -= 2;
	}
	else
	{
		s_res_list_mmm[1].width = 2304;
		s_res_list_mmm[1].height = 1296;
		s_res_list = s_res_list_mmm;
	}
	return 0;
}

/**
 *@brief do de-initialize of stream
 *@return 0 if success.
 *
 */
int jv_stream_deinit(void)
{
	int i;
	//for (i=0;i<HWINFO_STREAM_CNT+1;i++)
	for (i=0; i<HWINFO_STREAM_CNT; i++)
	{
		jv_stream_stop(i);
		ststatus[i].fd = -1;
	}
	pthread_mutex_destroy(&stream_mutex);
#ifdef ZRTSP_SUPPORT
	pthread_mutex_destroy(&rtsp_mutex);
#endif
	return 0;
}


static int __set_isp_framerate(int framerate)
{
	int rf;
//	return 30;
	if (framerate > 25)
		rf = 30;
	else 
		rf = 25;
	//else
		//rf=20;
	
	if (rf > MAX_FRAME_RATE)
		rf = MAX_FRAME_RATE;
	jv_sensor_set_fps(rf);
    return rf;
}

/**
 *@brief start a stream
 *@param channelid id of the stream
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_start(int channelid)
{

	HI_S32 s32Ret = HI_SUCCESS;
	VPSS_GRP VpssGrp = 0;
	SAMPLE_RC_E enRcMode;
	VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
	PAYLOAD_TYPE_E vencType = PT_BUTT;

	SIZE_S size;
	jv_stream_attr *attr;
	int bNeedRestartChannel23 = 0;

	pthread_mutex_lock(&stream_mutex);
	attr = &ststatus[channelid].attr;
	size.u32Width = attr->width;
	size.u32Height = attr->height;
	if (channelid == 0)
	{
		int nf = __set_isp_framerate(ststatus[channelid].attr.framerate);
		if (srcFramerate != nf)
		{
			bNeedRestartChannel23 = 1;
			srcFramerate = nf;
		}
	}
	if (attr->rcMode == JV_VENC_RC_MODE_CBR)
		enRcMode = SAMPLE_RC_CBR;
	else
		enRcMode= SAMPLE_RC_VBR;

	switch(attr->vencType)
	{
		case JV_PT_H264:
			vencType = PT_H264;
			break;
		case JV_PT_H265:
			vencType = PT_H265;
			break;
		case JV_PT_JPEG:
			vencType = PT_JPEG;
			break;
		case JV_PT_MJPEG:
			vencType = PT_MJPEG;
			break;
		default:
			break;
	}

	Printf("======== ststatus[%d].attr.width: %d, height: %d, framerate: %d\n", channelid, attr->width, attr->height, attr->framerate);
	s32Ret = SAMPLE_COMM_VENC_Start(attr, VencChn[channelid], vencType,\
	                                gs_enNorm, &size, enRcMode, attr->bitrate, attr->framerate,attr->nGOP);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("Start Venc failed!\n");
		pthread_mutex_unlock(&stream_mutex);
		return -1;
	}

	jv_stream_set_roi(channelid, &ststatus[channelid].attr.roiInfo);
	ststatus[channelid].attr.bRectStretch = 0;
	if (!ststatus[channelid].attr.bRectStretch)
	{
		VENC_CROP_CFG_S stGrpCropCfg = {0};
		if (channelid == 0)
			__build_vpss_venc_size_vi(attr->width, attr->height, &size, &stGrpCropCfg);
		else
			__build_vpss_venc_size(attr->width, attr->height,ststatus[0].attr.width,ststatus[0].attr.height, &size, &stGrpCropCfg);
		if(attr->width >= attr->height)//走廊模式，旋转90度或者270度的时候，有几个分辨率裁剪会重启，不再裁剪
			CHECK_RET(HI_MPI_VENC_SetCrop(VencChn[channelid], &stGrpCropCfg));
	}
	if (VpssChn[channelid] <= 3)
	{
		VPSS_CHN_MODE_S vpssMode;
		CHECK_RET(HI_MPI_VPSS_GetChnMode(VpssGrp, VpssChn[channelid], &vpssMode));
		vpssMode.u32Width = size.u32Width;
		vpssMode.u32Height = size.u32Height;
		if(attr->width <= attr->height && VpssChn[channelid]>0)
		{
			vpssMode.u32Width = attr->height;
			vpssMode.u32Height = attr->width;
		}
		CHECK_RET(HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn[channelid], &vpssMode));
	}
	else
	{	//实际走不到这里
		VPSS_EXT_CHN_ATTR_S vpssExtChnAttr;
		CHECK_RET(HI_MPI_VPSS_GetExtChnAttr(VpssGrp, VpssChn[channelid], &vpssExtChnAttr));
		vpssExtChnAttr.u32Width = size.u32Width;
		vpssExtChnAttr.u32Height = size.u32Height;
		if (VpssChn[channelid] > 3)
		{
			ROTATE_E rotate = ROTATE_NONE;
			HI_MPI_VPSS_GetRotate(VpssGrp, 0, &rotate);
			if (rotate == ROTATE_90 || rotate == ROTATE_270)
			{
				vpssExtChnAttr.u32Width = size.u32Height;
				vpssExtChnAttr.u32Height = size.u32Width;
			}
		}
		CHECK_RET(HI_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn[channelid], &vpssExtChnAttr));
	}

	s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn[channelid], VpssGrp, VpssChn[channelid]);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("venc bind vpss failed!\n");
		pthread_mutex_unlock(&stream_mutex);
		return -1;
	}

	S32	tmpfd = 0;

	tmpfd = HI_MPI_VENC_GetFd(channelid);
	Printf("Chn:%d, Venc:%d, GetFd=%d\n", channelid, VencChn[channelid], tmpfd);

	ststatus[channelid].fd = tmpfd;
	ststatus[channelid].bOpened = TRUE;
	pthread_mutex_unlock(&stream_mutex);

	if (bNeedRestartChannel23 && channelid == 0)
	{
		int i;
		for (i=1;i<HWINFO_STREAM_CNT;i++)
		{
			if (ststatus[i].fd > 0)
			{
				jv_stream_stop(i);
				jv_stream_start(i);
			}
		}
	}
	return 0;
}

/**
 *@brief stop a stream
 *@param channelid id of the stream
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_stop(int channelid)
{
	pthread_mutex_lock(&stream_mutex);

	SAMPLE_COMM_VENC_UnBindVpss(VencChn[channelid],0,VpssChn[channelid]);
	SAMPLE_COMM_VENC_Stop(VencChn[channelid]);
	
	ststatus[channelid].fd = -1;
	ststatus[channelid].bOpened = FALSE;
	pthread_mutex_unlock(&stream_mutex);
	return 0;
}

/**
 *@brief check if data received
 *@param fd a list of channelid, when data received, fd[n].received will be set TRUE
 *@param cnt count of fd to check received data
 *@param timeout an upper limit the func will block, in milliseconds.  -1 means no timeout
 *
 */
int jv_stream_poll(jv_stream_pollfd fd[], int cnt, int timeout)
{
	int maxfd;
	fd_set readset;
	int i;
	struct timeval tv;
	int n;

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	maxfd = -1;
	FD_ZERO(&readset);
	pthread_mutex_lock(&stream_mutex);
	for (i=0; i<cnt; i++)
	{
		if (ststatus[fd[i].channelid].fd > 0)
		{
			FD_SET(ststatus[fd[i].channelid].fd, &readset);
			if (maxfd < ststatus[fd[i].channelid].fd)
				maxfd = ststatus[fd[i].channelid].fd;
		}
	}
	pthread_mutex_unlock(&stream_mutex);
	if (maxfd == -1)
		return 0;
	maxfd++;
	n = select(maxfd, &readset, NULL, NULL, &tv);
	pthread_mutex_lock(&stream_mutex);
	if (n <= 0)
	{
		pthread_mutex_unlock(&stream_mutex);
		return 0;
	}
	for (i=0; i<cnt; i++)
	{
		if (ststatus[fd[i].channelid].fd > 0
		        && FD_ISSET(ststatus[fd[i].channelid].fd, &readset))
			fd[i].received = 1;
	}
	pthread_mutex_unlock(&stream_mutex);

	return n;
}


/**
 *@brief Read one frame data from the channel
 *@param channelid Id of the stream
 *@param Info frame info
 *
 *@note The function will block untill data is comming or #jv_stream_stop is called
 *@note After parsed the data, #jv_stream_release should be call to release the source
 *
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_read(int channelid, jv_frame_info_t *info)
{
	unsigned char *acFrameBuffer ;
	int nLength = 0;
	S32	nRet, i;
	VENC_CHN_STAT_S	stStat;
	VENC_STREAM_S	stStream;
	VENC_PACK_S		stVencPack[8];
	stStream.pstPack = stVencPack;
	int maxlen;
	int tlen;

	//循环中调用此memset导致cpu占用率过高,lck20120619
	//memset(info,0,sizeof(jv_frame_info_t));

	pthread_mutex_lock(&stream_mutex);
	nRet = HI_MPI_VENC_Query(channelid, &stStat);
	if (0 != nRet)
	{
		Printf("Channel:%d, HI_MPI_VENC_Query:0x%x\n", channelid, nRet);
		pthread_mutex_unlock(&stream_mutex);
		return -1;
	}
	stStream.u32PackCount = stStat.u32CurPacks; //指定数据流的包个数
	nRet = HI_MPI_VENC_GetStream(channelid, &stStream, -1);	//获取视频数据流
	if (0 != nRet)
	{
		Printf("Channel:%d, HI_MPI_VENC_GetStream:0x%x\n", channelid, nRet);
		pthread_mutex_unlock(&stream_mutex);
		return -1;
	}

	nLength = 0;
	acFrameBuffer = info->buffer;
	maxlen = sizeof(info->buffer);
	info->streamid = channelid;
	info->frametype = JV_FRAME_TYPE_P;
	for (i=0; i< stStream.u32PackCount; i++)
	{
#ifdef ZRTSP_SUPPORT
		pthread_mutex_lock(&rtsp_mutex);
		int DataLen = stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset - 4;
		char *t = (char *)stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset;

		if(ststatus[channelid].attr.vencType == JV_PT_H264)
		{
			if((t[4]&0x1F) == H264E_NALU_SPS)
			{
				jvstream_rtsp[channelid].sps[0] = DataLen;
				memcpy(jvstream_rtsp[channelid].sps+1, t+4, DataLen);
			}
			if((t[4]&0x1F) == H264E_NALU_PPS)
			{
				jvstream_rtsp[channelid].pps[0][0] = DataLen;
				memcpy(jvstream_rtsp[channelid].pps[0]+1, t+4, DataLen);
			}
			rtsp_nalu_type = (NALU_TYPEs)stStream.pstPack[i].DataType.enH264EType;
		}
		else if(ststatus[channelid].attr.vencType == JV_PT_H265)
		{
			if(((t[4]>>1)&0x3F) == H265E_NALU_SPS)
			{
				jvstream_rtsp[channelid].sps[0] = DataLen;
				memcpy(jvstream_rtsp[channelid].sps+1, t+4, DataLen);
			}
			if(((t[4]>>1)&0x3F) == H265E_NALU_PPS)
			{
				jvstream_rtsp[channelid].pps[0][0] = DataLen;
				memcpy(jvstream_rtsp[channelid].pps[0]+1, t+4, DataLen);
			}
			rtsp_nalu_type = stStream.pstPack[i].DataType.enH265EType;
		}
//		printf("------:%x,%x,%x,%x,%x\n",t[0],t[1],t[2],t[3],t[4]);

		rtsp_offset = stStream.pstPack[i].u32Offset;
		pthread_mutex_unlock(&rtsp_mutex);
#endif
		tlen = stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
		if (tlen + nLength > maxlen)
			tlen = maxlen - nLength;
		memcpy(&acFrameBuffer[nLength], stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset, tlen);
		nLength += tlen;
		if(ststatus[channelid].attr.vencType == JV_PT_H264)
		{
			if(stStream.pstPack[i].DataType.enH264EType == H264E_NALU_ISLICE)
			{
				info->frametype = JV_FRAME_TYPE_I;
			}
		}
		else if(ststatus[channelid].attr.vencType == JV_PT_H265)
		{
			if(stStream.pstPack[i].DataType.enH265EType == H265E_NALU_ISLICE)
			{
				info->frametype = JV_FRAME_TYPE_I;
			}
		}
		info->timestamp = stStream.pstPack[i].u64PTS/1000;
	}
	info->len = nLength;

	//释放视频数据流
	HI_MPI_VENC_ReleaseStream(channelid , &stStream);

	pthread_mutex_unlock(&stream_mutex);
	return 0;
}

#ifdef ZRTSP_SUPPORT
	int jv_stream_get_spspps(int chn,SPS_PPS *rtsp_sps_pps)
	{
		if(!rtsp_sps_pps)
			return -1;
		pthread_mutex_lock(&rtsp_mutex);
		memcpy(rtsp_sps_pps,&jvstream_rtsp[chn],sizeof(SPS_PPS));
		pthread_mutex_unlock(&rtsp_mutex);
		return 0;
	}
	NALU_TYPEs jv_stream_get_nalu_tpye()
	{
		pthread_mutex_lock(&rtsp_mutex);
		NALU_TYPEs rtspNaluType = rtsp_nalu_type;
		pthread_mutex_unlock(&rtsp_mutex);
		return rtspNaluType;
	}
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
int jv_stream_release(int channelid, jv_frame_info_t *info)
{
	return 0;
}

//#endif

#if 1
/**
 *@brief set stream attribute
 *@param channelid id of the stream
 *@param attr pointer of attribution
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_set_attr(int channelid, jv_stream_attr *attr)
{
	VENC_CHN_ATTR_S		stAttr;
	int rc = 0;

	pthread_mutex_lock(&stream_mutex);
	if (attr->framerate > MAX_FRAME_RATE)
		attr->framerate = MAX_FRAME_RATE;
	if(hwinfo.sensor == SENSOR_OV4689)
	{
		if(attr->width == 2592 && attr->height == 1520)
		{	
			attr->width = VI_WIDTH;
			attr->height = VI_HEIGHT;
		}
		else if(attr->width == 1520 && attr->height == 2592)
		{
			attr->width = VI_HEIGHT;
			attr->height = VI_WIDTH;
		}
	}
	if(hwinfo.sensor == SENSOR_OV4689&&(attr->vencType ==JV_PT_H265) && attr->width==VI_WIDTH && attr->height==VI_HEIGHT)
	{
		if (attr->minQP == 20 && attr->maxQP == 40)
		{
			attr->minQP = 18;
			attr->maxQP = 38;
		}
		else if (attr->minQP == 22 && attr->maxQP == 42)
		{
			attr->minQP = 19;
			attr->maxQP = 39;
		}
		else if (attr->minQP == 24 && attr->maxQP == 45)
		{
			attr->minQP = 20;
			attr->maxQP = 40;
		}
		else if (attr->minQP == 26 && attr->maxQP == 48)
		{
			attr->minQP = 24;
			attr->maxQP = 45;
		}
	}


	memcpy(&ststatus[channelid].attr, attr, sizeof(jv_stream_attr));

	//if (ststatus[channelid].attr.framerate > 29)
	//	ststatus[channelid].attr.framerate = 29;
	if (ststatus[channelid].bOpened)
	{
		if (channelid == 0)
			rc = __set_isp_framerate(ststatus[channelid].attr.framerate);
		HI_MPI_VENC_GetChnAttr(channelid, &stAttr);
		if (ststatus[channelid].attr.rcMode == JV_VENC_RC_MODE_VBR)
		{
			stAttr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = ststatus[channelid].attr.framerate;
			stAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = ststatus[channelid].attr.bitrate;
			stAttr.stRcAttr.stAttrH264Vbr.u32Gop = ststatus[channelid].attr.nGOP;
			stAttr.stRcAttr.stAttrH264Vbr.u32MaxQp =  ststatus[channelid].attr.maxQP;
			stAttr.stRcAttr.stAttrH264Vbr.u32MinQp =  ststatus[channelid].attr.minQP;
			if (rc != 0)
				stAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = rc;
		}
		else if (ststatus[channelid].attr.rcMode == JV_VENC_RC_MODE_CBR)
		{
			stAttr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = ststatus[channelid].attr.framerate;
			stAttr.stRcAttr.stAttrH264Cbr.u32BitRate = ststatus[channelid].attr.bitrate;
			stAttr.stRcAttr.stAttrH264Cbr.u32Gop = ststatus[channelid].attr.nGOP;
			if (rc != 0)
				stAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = rc;
			stAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
		}
		else
		{
			Printf("ERROR: Unsupport stAttr.stRcAttr.enRcMode: %d\n", stAttr.stRcAttr.enRcMode);
		}

		CHECK_RET(HI_MPI_VENC_SetChnAttr(channelid, &stAttr));

	}
	pthread_mutex_unlock(&stream_mutex);

	return 0;
}

/**
 *@brief get stream attribute
 *@param channelid id of the stream
 *@param attr pointer of attribution
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_stream_get_attr(int channelid, jv_stream_attr *attr)
{
	pthread_mutex_lock(&stream_mutex);
	memcpy(attr, &ststatus[channelid].attr, sizeof(jv_stream_attr));
	pthread_mutex_unlock(&stream_mutex);
	return 0;
}


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

/**
 *@brief 获取编码相关的各项指标
 *@param ability 输出 编码相关的各项指标
 *
 *@return 0
 *
 */
int jv_stream_get_ability(int channelid, jvstream_ability_t *ability)
{
	if (ability == NULL)
	{
		Printf("param error!\n");
		return JVERR_BADPARAM;
	}
	/*

	resolution_t *resList;//输出分辨率的列表
	int resListCnt;//输出分辨率的可选个数
	int maxNGOP;//最大nGOP
	int minNGOP;//最小nGOP
	int maxFramerate;//最大帧率
	int minFramerate;//最小帧率
	int maxKBitrate;//最大码率，单位为KBit
	int minKBitrate;//最小码率
	resolution_t inputRes;//输入分辨率
	*/
	memset(ability, 0, sizeof(jvstream_ability_t));
	ability->resList = s_res_list;
	ability->resListCnt = s_res_cnt;
	ability->maxNGOP = 360;
	ability->minNGOP = 1;
	ability->maxFramerate = MAX_FRAME_RATE;
	ability->minFramerate = 5;
	ability->maxKBitrate = 10 * 1024;
	ability->minKBitrate = 10;
	ability->inputRes.width = VI_WIDTH;
	ability->inputRes.height = VI_HEIGHT;
	if(VI_CROP_ENABLE)
	{
		ability->inputRes.width = VI_CROP_W;
		ability->inputRes.height = VI_CROP_H;
	}
	ability->maxStreamRes[0] = VI_WIDTH*VI_HEIGHT;
	if(hwinfo.sensor == SENSOR_OV4689 && VI_WIDTH != 2048)
	{
		ability->inputRes.width = 2560;
		ability->inputRes.height = 1440;
		ability->maxStreamRes[0] = 2592*1520;	
	}
	ability->maxStreamRes[1] = 720*576;
	ability->maxStreamRes[2] = 352*288;

	ability->vencTypeNum = 2;
	ability->maxRoi = MAX_ROI_REGION;
	return 0;
}

/**
 *@brief 向解码器申请一帧关键帧
 *在录像开始或者有客户端连接时调用此函数
 *
 *@param channelid 通道号，表示要申请此通道输出关键帧
 *
 */
void jv_stream_request_idr(int channelid)
{
	pthread_mutex_lock(&stream_mutex);
	HI_MPI_VENC_RequestIDR(channelid, HI_FALSE);
	pthread_mutex_unlock(&stream_mutex);
}

/**
 *@获取码流的输入端的宽度和高度
 *@param channelid 输入 通道号
 *@param width 输出：宽度指针
 *@param height 输出：高度指针
 *
 *@return 0 if success
 *
 */
int jv_stream_get_vi_resolution(int channelid, unsigned int *width, unsigned int *height)
{
	if (width)
		*width = VI_WIDTH;
	if (height)
		*height = VI_HEIGHT;

	return 0;
}
/**
 * 获取感兴趣区域信息
 */
int jv_stream_get_roi(int channelid, jv_stream_roi *roi)
{
	memcpy(roi,&(ststatus[channelid].attr.roiInfo),sizeof(jv_stream_roi));
	return 0;
}
/**
 * 设置感兴趣区域信息
 */
int jv_stream_set_roi(int channelid, jv_stream_roi *roi)
{
	BOOL bSetted = FALSE;
	memcpy(&(ststatus[channelid].attr.roiInfo),roi,sizeof(jv_stream_roi));
	int i;
	VENC_ROI_CFG_S hi_roi;

	int inQP, outQP;//区域内和区域外的QP值

	if (!roi->ior_reverse)
	{
		inQP = -1;
		outQP = ststatus[channelid].attr.roiInfo.roiWeight;
	}
	else
	{
		inQP = ststatus[channelid].attr.roiInfo.roiWeight;
		outQP = -1;
	}

	//注意：ROI的0通道，作为普遍通道，1，2，3，4对应实际的0，1，2，3
	hi_roi.bAbsQp = HI_FALSE;
	for(i=0;i<MAX_ROI_REGION;i++)
	{
		HI_MPI_VENC_GetRoiCfg(channelid, i+1,&hi_roi);

		hi_roi.stRect.s32X = ststatus[channelid].attr.roiInfo.roi[i].x;
		hi_roi.stRect.s32Y = ststatus[channelid].attr.roiInfo.roi[i].y;
		hi_roi.stRect.u32Width = ststatus[channelid].attr.roiInfo.roi[i].w;
		hi_roi.stRect.u32Height = ststatus[channelid].attr.roiInfo.roi[i].h;
		hi_roi.u32Index = i+1;
		if(hi_roi.stRect.u32Width)
		{
			hi_roi.s32Qp = inQP;//ststatus[channelid].attr.roiInfo.roiWeight;
			hi_roi.bEnable = HI_TRUE;
			bSetted = TRUE;
		}
		else
		{
			hi_roi.s32Qp = 0;
			hi_roi.bEnable = HI_FALSE;
		}
//		printf("QP:%d;X:%d,Y:%d,W:%d,H:%d;I:%d;EN:%d;\n",
//				hi_roi.s32Qp,hi_roi.stRect.s32X,hi_roi.stRect.s32Y,hi_roi.stRect.u32Width,
//				hi_roi.stRect.u32Height,hi_roi.u32Index,hi_roi.bEnable);
		CHECK_RET(HI_MPI_VENC_SetRoiCfg(channelid,&hi_roi));
//		printf("channelid: %d, index: %d, xy: %d,%d, width: %d, height: %d, hi_roi.s32Qp: %d, hi_roi.bEnable: %d\n"
//				, channelid
//				, hi_roi.u32Index
//				, hi_roi.stRect.s32X
//				, hi_roi.stRect.s32Y
//				, hi_roi.stRect.u32Width
//				, hi_roi.stRect.u32Height
//				, hi_roi.s32Qp
//				, hi_roi.bEnable);
//		printf(">>>>>>>>>>>>>>>>>%d\n",ret);
	}
	if (bSetted)
	{
		i = 0;
		HI_MPI_VENC_GetRoiCfg(channelid, i,&hi_roi);
		hi_roi.u32Index = i;
		hi_roi.s32Qp = outQP;//ststatus[channelid].attr.roiInfo.roiWeight;
		hi_roi.bEnable = HI_TRUE;
		hi_roi.stRect.s32X = 0;
		hi_roi.stRect.s32Y = 0;
		hi_roi.stRect.u32Width = ststatus[channelid].attr.width;
		hi_roi.stRect.u32Height = ststatus[channelid].attr.height;
		hi_roi.stRect.u32Width = JV_ALIGN_FLOOR(hi_roi.stRect.u32Width, 16);
		hi_roi.stRect.u32Height = JV_ALIGN_FLOOR(hi_roi.stRect.u32Height, 16);
		CHECK_RET(HI_MPI_VENC_SetRoiCfg(channelid,&hi_roi));
//		printf("channelid: %d, index: %d, width: %d, height: %d, hi_roi.s32Qp: %d\n", channelid, hi_roi.u32Index, hi_roi.stRect.u32Width, hi_roi.stRect.u32Height, hi_roi.s32Qp);
	}

	return 0;
}

