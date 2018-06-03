#include "jv_common.h"
#include "jv_stream.h"
#include "hicommon.h"
#include <jv_sensor.h>
#include "../../server/sctrl/SYSFuncs.h"
#include "hi_comm_vgs.h"
#include "mpi_vgs.h"
#include "utl_common.h"
#ifdef ZRTSP_SUPPORT
SPS_PPS jvstream_rtsp[3];
NALU_TYPEs rtsp_nalu_type;
int rtsp_offset;
pthread_mutex_t rtsp_mutex;
#endif

typedef struct
{
	int fd;

	jv_stream_attr attr;
	BOOL bOpened;

} jvstream_status_t;

extern BOOL VI_CROP_ENABLE;

extern int VI_CROP_X;
extern int VI_CROP_Y;
extern int VI_CROP_W;
extern int VI_CROP_H;


static jvstream_status_t ststatus[MAX_STREAM+1];
static pthread_mutex_t stream_mutex;

#define __VENC_SAMPLE__

#ifdef  __VENC_SAMPLE__
typedef enum sample_rc_e
{
    SAMPLE_RC_CBR = 0,
    SAMPLE_RC_VBR,
    SAMPLE_RC_AVBR,
    SAMPLE_RC_FIXQP
} SAMPLE_RC_E;
VENC_GRP VencGrp[3] = {0,1,2};
VENC_CHN VencChn[3] = {0,1,2};
VPSS_CHN VpssChn[3] = {0,1,2};

//lck20121102
//将IPC的分辨率更改为以下几种，统一比例都是16:9

static resolution_t s_res_list_mmm[] =
{
	{1920,  1080},
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

static void __build_vpss_venc_size(int bRotate, int vencw, int vench, int vencw_0,int vench_0, SIZE_S *vpssSize, VENC_CROP_CFG_S *stGrpCropCfg)
{
	int expw, exph; //锁定比例时，期望的宽度和高度，以压缩的宽高为基准

	expw = vench * VI_WIDTH / VI_HEIGHT;
	exph = vencw * VI_HEIGHT / VI_WIDTH;

	if(bRotate)//走廊模式，旋转90度或者270度的时候，VB不够用，直接有VPSS压缩，VPSS的rotate W和H不需要手动翻转这里再反过来
	{
		vpssSize->u32Width = JV_ALIGN_CEILING(vench, 4);
		vpssSize->u32Height = JV_ALIGN_CEILING(vencw, 4);
		return ;
	}

	if (expw - vencw >= 32)
	{
		unsigned int x =  JV_ALIGN_CEILING((expw - vencw)/2, 16);
		vpssSize->u32Width = 2 * x + vencw;
		vpssSize->u32Width = JV_ALIGN_CEILING(vpssSize->u32Width, 4);
		vpssSize->u32Height = JV_ALIGN_CEILING(vench, 4);
	}
	else if (exph - vench > 10)
	{
		unsigned int y = JV_ALIGN_CEILING((exph - vench)/2, 2);
		vpssSize->u32Width = JV_ALIGN_CEILING(vencw, 4);
		vpssSize->u32Height = 2 * y + vench;
		vpssSize->u32Height = JV_ALIGN_CEILING(vpssSize->u32Height, 4);
	}
	else
	{
		vpssSize->u32Width = JV_ALIGN_CEILING(vencw, 4);
		vpssSize->u32Height = JV_ALIGN_CEILING(vench, 4);
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

	printf("x,y,w,h: %d,%d,%d,%d\n", stGrpCropCfg->stRect.s32X, stGrpCropCfg->stRect.s32Y, stGrpCropCfg->stRect.u32Width, stGrpCropCfg->stRect.u32Height);
	printf("vpss: %d,%d\n", vpssSize->u32Width, vpssSize->u32Height);
}

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
		stGrpCropCfg->stRect.u32Height = JV_ALIGN_CEILING(exph, 2);
		vpssSize->u32Width = VI_WIDTH;//2 * stGrpCropCfg->stRect.s32X + vencw;//JV_ALIGN_CEILING(expw, 16);
		vpssSize->u32Height = VI_HEIGHT;
	}
	else if (exph - VI_HEIGHT > 10)
	{
		stGrpCropCfg->bEnable = TRUE;
		stGrpCropCfg->stRect.s32X = JV_ALIGN_CEILING((VI_WIDTH-expw)/2,16);
		stGrpCropCfg->stRect.s32Y = 0;
		stGrpCropCfg->stRect.u32Width = JV_ALIGN_CEILING(expw, 2);
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
//	printf("x,y,w,h: %d,%d,%d,%d\n", stGrpCropCfg->stRect.s32X, stGrpCropCfg->stRect.s32Y, stGrpCropCfg->stRect.u32Width, stGrpCropCfg->stRect.u32Height);
//	printf("vpss: %d,%d\n", vpssSize->u32Width, vpssSize->u32Height);
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
	
	if (fvif < attr->framerate && VI_WIDTH*VI_HEIGHT != 1920*1080)
		fvif = attr->framerate;

	if (srcFramerate > 25 && fvif < srcFramerate && !blow)
		fvif = srcFramerate;
	if(VI_WIDTH*VI_HEIGHT == 1920*1080)		//200W的在VPSS修改帧率
	{
		VPSS_GRP VpssGrp = 0;
		VPSS_CHN_ATTR_S vpssChnAttr;
		HI_U32 rcSrcFrameRate;
		HI_FR32 rcDstFrameRate;

		HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn[VencChn], &vpssChnAttr);
		rcSrcFrameRate = vpssChnAttr.s32DstFrameRate;
		rcDstFrameRate = rcSrcFrameRate;
		if(rcSrcFrameRate != framerate)
		{
			nGop = nGop / framerate * rcSrcFrameRate;
		}

		fvif = rcSrcFrameRate;
		framerate = rcDstFrameRate;
		printf("[%s, %d]: vencChn=%d, w=%d, h=%d, rcSrcFr=%d, rcDstFr=%d, gop=%d\n", __func__, __LINE__, 
			VencChn, stPicSize.u32Width, stPicSize.u32Height, rcSrcFrameRate, rcDstFrameRate, nGop);
	}
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
		// 1.0.4.0版SDK需要多分配10多K缓存，否则创建VDEC通道会失败
		stH264Attr.u32BufSize  = JV_ALIGN_CEILING(stPicSize.u32Width * stPicSize.u32Height * 3 / 4 + 16 * 1024, 64);/*stream buffer size*/
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
		else if (SAMPLE_RC_AVBR == enRcMode)
		{
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264AVBR;
			VENC_ATTR_H264_AVBR_S stH264AVbr;
			stH264AVbr.u32Gop = nGop;
			stH264AVbr.u32StatTime = (nGop+framerate-1)/framerate;
			stH264AVbr.u32SrcFrmRate = fvif;
			stH264AVbr.fr32DstFrmRate = framerate;
			stH264AVbr.u32MaxBitRate = bitrate;
			memcpy(&stVencChnAttr.stRcAttr.stAttrH264AVbr, &stH264AVbr, sizeof(VENC_ATTR_H264_AVBR_S));
		}
		else
		{
			return HI_FAILURE;
		}
	}
	break;

	case PT_H265:
	{
		VENC_ATTR_H265_S stH265Attr;
		stH265Attr.u32MaxPicWidth = stPicSize.u32Width;
		stH265Attr.u32MaxPicHeight = stPicSize.u32Height;
		stH265Attr.u32PicWidth = stPicSize.u32Width;/*the picture width*/
		stH265Attr.u32PicHeight = stPicSize.u32Height;/*the picture height*/
		stH265Attr.u32BufSize  = stPicSize.u32Width * stPicSize.u32Height * 1.5;/*stream buffer size*/
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
		SAMPLE_PRT("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n", VencChn, s32Ret);
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

	rcPara.u32RowQpDelta = 3;
	if (SAMPLE_RC_VBR == enRcMode)
	{
		rcPara.stParamH264VBR.s32IPQPDelta = 2;
#if 0
//		rcPara.stParamH264VBR.u32MinIQP = 3;
		if(enType == PT_H264)
		{
			rcPara.stParamH264VBR.u32MaxIprop = 8;
			rcPara.stParamH264VBR.s32ChangePos = 70;
		}
		else if(enType == PT_H265)
		{
			rcPara.stParamH265Vbr.u32MaxIprop = 8;
			rcPara.stParamH265Vbr.s32ChangePos = 70;
		}
#endif
	}
	else if (SAMPLE_RC_AVBR == enRcMode)
	{
		// rcPara.stParamH264AVbr.s32ChangePos = 50;
		// rcPara.stParamH264AVbr.s32IPQPDelta = 3;
		// rcPara.stParamH264AVbr.s32MinStillPercent = 20;
		rcPara.stParamH264AVbr.u32MinIQp = attr->minQP;
		rcPara.stParamH264AVbr.u32MaxIQp = attr->maxQP;
		rcPara.stParamH264AVbr.u32MinQp = attr->minQP;
		rcPara.stParamH264AVbr.u32MaxQp = attr->maxQP;
		rcPara.stParamH264AVbr.u32MaxStillQP = 40;
		rcPara.stParamH264AVbr.s32MinStillPercent = 50;
		// rcPara.stParamH264AVbr.u32MaxIprop = 50;
		rcPara.stParamH264AVbr.s32IPQPDelta = 0;
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
/*	VENC_PARAM_REF_S refParam;
	memset(&refParam,0,sizeof(refParam));

	refParam.u32Base = 25;
	refParam.u32Enhance = 1;
	refParam.bEnablePred = FALSE;
	
	HI_MPI_VENC_SetRefParam(VencChn,&refParam);*/
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

#define ALIGN_2(x)			((x) & 0xFFFFFFFE)

struct VpssVencParam
{
	pthread_t	pid;
	BOOL		bRun;
	VPSS_GRP	VpssGrp;
	VPSS_CHN	VpssChn;
	VENC_CHN	VencChn;
};

static struct VpssVencParam Args[5];
static jv_stream_get_graph	s_GetGraphCallback;

static inline void _vgs_set_line(VGS_DRAW_LINE_S* VgsLine, int x1, int y1, int x2, int y2, int linew, int color)
{
	VgsLine->stStartPoint.s32X = ALIGN_2(x1);
	VgsLine->stStartPoint.s32Y = ALIGN_2(y1);
	VgsLine->stEndPoint.s32X = ALIGN_2(x2);
	VgsLine->stEndPoint.s32Y = ALIGN_2(y2);
	VgsLine->u32Thick = ALIGN_2(linew);
	VgsLine->u32Color = color;
}

static inline void _vgs_set_rect(VGS_DRAW_LINE_S VgsLine[4], int x, int y, int w, int h, int linew, int color)
{
	_vgs_set_line(&VgsLine[0], x,		y,		x + w,	y,		linew, color);	// 上
	_vgs_set_line(&VgsLine[1], x,		y,		x,		y + h,	linew, color);	// 左
	_vgs_set_line(&VgsLine[2], x,		y + h - linew,	x + w,	y + h - linew,	linew, color);	// 下
	_vgs_set_line(&VgsLine[3], x + w - linew,	y,		x + w - linew,	y + h,	linew, color);	// 右
}

//全能的画线,画多边形, 要求count >= 2
static inline int  _vgs_set_polygon(int count, POINT_s * point, VGS_DRAW_LINE_S *VgsLine, int linew, int color)
{
	int i,next;
	
	if(count < 2)
		return 0;
	if(count == 2)
	{
		i= 0 ;
		next = 1;
		_vgs_set_line(&VgsLine[i], point[i].x, point[i].y,  point[next].x, point[next].y, linew, color);   // 上
//		printf("i=%d x1= %d y1=%d   x2= %d y2=%d \n",point[i].x,point[i].y,point[next].x,point[next].y);
		return 1;

	}
	else
	{
		for(i=0;i<count;i++)
		{
			next = (i+1)%count;
			_vgs_set_line(&VgsLine[i], point[i].x, point[i].y,  point[next].x, point[next].y, linew, color);   // 上

		}
		return count;
	}
}

static inline void _vgs_set_words(VGS_ADD_OSD_S* VgsOsd, RECT* rect, unsigned int phyaddr, int color)
{
	VgsOsd->stRect.s32X = ALIGN_2(rect->x);
	VgsOsd->stRect.s32Y = ALIGN_2(rect->y);
	VgsOsd->stRect.u32Width = ALIGN_2(rect->w);
	VgsOsd->stRect.u32Height = ALIGN_2(rect->h);
	VgsOsd->u32BgColor = 0xFFFF;
	VgsOsd->enPixelFmt = PIXEL_FORMAT_RGB_1555;
	VgsOsd->u32PhyAddr = phyaddr;
	VgsOsd->u32Stride = rect->w * 2;
	VgsOsd->u32BgAlpha = 0x00;
	VgsOsd->u32FgAlpha = 0xFF;
}

static void* _Vpss_To_Venc(void* arg)
{
	struct VpssVencParam* pParam = (struct VpssVencParam*)arg;
	VPSS_GRP	VpssGrp = pParam->VpssGrp;
	VPSS_CHN	VpssChn = pParam->VpssChn;
	VENC_CHN	VencChn = pParam->VencChn;
	VENC_CHN_ATTR_S VencChnAttr;
	VGS_HANDLE	VgsHandle = 0;
	VGS_TASK_ATTR_S VgsTask;
	VGS_DRAW_LINE_S	VgsLine[34 * 4 + 4*10];
	VGS_ADD_OSD_S	VgsOsd[10];
	int linecnt, osdcnt;
	GRAPH_t graph[34+4];
	int cnt = 0;
	int newCnt = 0;
	int i = 0;
	int w = VI_WIDTH;
	int h = VI_HEIGHT;
	BOOL bOK;
	HI_S32 ret;
	char *pMapAddr;
	U64 t1,t2;
	U32 diff, max;
	struct timespec tv;
	BOOL bNeedGet=0;
	int oldcnt;
	pthreadinfo_add(__func__);


	CHECK_RET(HI_MPI_VENC_GetChnAttr(VencChn, &VencChnAttr));
	printf("%s VencChn=%d,w=%d,h=%d\n",__func__, VencChn,VencChnAttr.stVeAttr.stAttrH264e.u32PicWidth,VencChnAttr.stVeAttr.stAttrH264e.u32PicHeight);
	w	= VencChnAttr.stVeAttr.stAttrH264e.u32PicWidth;
	h	= VencChnAttr.stVeAttr.stAttrH264e.u32PicHeight;

	ret = HI_MPI_VPSS_SetDepth(VpssGrp, VpssChn, 1);
	if (ret != 0)
	{
		printf("HI_MPI_VPSS_SetDepth failed: %#x, grp: %d, ch: %d\n", ret, VpssGrp, VpssChn);
	}

	while (pParam->bRun)
	{
		ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, VpssChn, &VgsTask.stImgIn, 500);
		if (ret != 0)
		{
			printf("HI_MPI_VPSS_GetChnFrame failed: %#x, grp: %d, ch: %d\n", ret, VpssGrp, VpssChn);
			usleep(20 * 1000);
			continue;
		}
		bOK = FALSE;
		if (s_GetGraphCallback == NULL)
		{
			goto End;
		}
		newCnt = ARRAY_SIZE(graph);
		bNeedGet=!bNeedGet;
		if(bNeedGet)
		{
			s_GetGraphCallback(VencChn, graph, &newCnt, w, h);
			oldcnt = newCnt;
		}
		else
		{
			newCnt = oldcnt;
		}
			
//		printf("newCnt=%d\n",newCnt);
		
		if (newCnt > 0)
		{
			GRAPH_t* pGraph = graph;

			linecnt = 0;
			osdcnt = 0;

			for (i = 0; i < newCnt; ++i)
			{
				switch (pGraph->graph_type)
				{
				case GRAPH_TYPE_RECT:
					_vgs_set_rect(&VgsLine[linecnt], pGraph->Rect.x, pGraph->Rect.y, pGraph->Rect.w, pGraph->Rect.h, 
									pGraph->linew, pGraph->color);
					linecnt += 4;
					break;
				case GRAPH_TYPE_LINE:
					_vgs_set_line(&VgsLine[linecnt], pGraph->Line.x1, pGraph->Line.y1, pGraph->Line.x2, pGraph->Line.y2, 
									pGraph->linew, pGraph->color);
					++linecnt;
					break;
				case GRAPH_TYPE_WORDS:
					_vgs_set_words(&VgsOsd[osdcnt], &pGraph->Bitmap.rect, pGraph->Bitmap.phyaddr, pGraph->color);
					++osdcnt;
					break;
				case GRAPH_TYPE_POLYGON:
					linecnt += _vgs_set_polygon(pGraph->Polygon.nCnt, &pGraph->Polygon.stPoints[0],&VgsLine[linecnt],pGraph->linew, pGraph->color );
					break;
				case GRAPH_TYPE_NONE:
				default:
					break;
				}

				++pGraph;
			}

		}

		if (newCnt > 0 )
		{
			ret = HI_MPI_VGS_BeginJob(&VgsHandle);
			if (ret != 0)
			{
				printf("HI_MPI_VGS_BeginJob failed: %#x\n", ret);
				goto End;
			}

			VgsTask.stImgOut = VgsTask.stImgIn;

			if (linecnt > 0)
			{
				ret = HI_MPI_VGS_AddDrawLineTaskArray(VgsHandle, &VgsTask, (VGS_DRAW_LINE_S*)VgsLine, linecnt);
				if (ret != 0)
				{
					printf("HI_MPI_VGS_AddDrawLineTaskArray failed: %#x\n", ret);
					goto End;
				}
				bOK = TRUE;
			}

			if (osdcnt > 0)
			{
				ret = HI_MPI_VGS_AddOsdTaskArray(VgsHandle, &VgsTask, VgsOsd, osdcnt);
				if (ret != 0)
				{
					printf("HI_MPI_VGS_AddOsdTask failed: %#x\n", ret);
					goto End;
				}
			}

		End:
//		clock_gettime(CLOCK_MONOTONIC, &tv);
//		t1 =  tv.tv_sec * 1000 + tv.tv_nsec / 1000000;

			if (bOK)
			{
				ret = HI_MPI_VGS_EndJob(VgsHandle);
				if (ret != 0)
				{
					printf("HI_MPI_VGS_EndJob failed: %#x\n", ret);
					goto End;
				}
				VgsHandle = 0;
				bOK = TRUE;
			}
			else
			{
				if (VgsHandle)
				{
					HI_MPI_VGS_CancelJob(VgsHandle);
					VgsHandle = 0;
				}
			}
		}
//		clock_gettime(CLOCK_MONOTONIC, &tv);
//		t2 =  tv.tv_sec * 1000 + tv.tv_nsec / 1000000;

//		ret = HI_MPI_VENC_SendFrame(VencChn, bOK ? &VgsTask.stImgOut : &VgsTask.stImgIn, 500);

		ret = HI_MPI_VENC_SendFrame(VencChn, &VgsTask.stImgIn, 500);
		if (ret != 0)
		{
			printf("HI_MPI_VENC_SendFrame failed: %#x, chn: %d\n", ret, VencChn);
		}
		do
		{
			ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &VgsTask.stImgIn);
			if(ret !=0)
			{
				printf("HI_MPI_VPSS_ReleaseChnFrame  Error=%d\n",ret);
			}
		}while(ret!=0);

//		diff = t2-t1;
//		if(max< diff)
//			max =diff;
//		printf("CH=%d  ; diff= %d ms max=%d ms\n",VpssChn,diff,max);	
	}

	return NULL;
}

/******************************************************************************
* function : venc bind vpss
******************************************************************************/
HI_S32 SAMPLE_COMM_VENC_BindVpss(VENC_CHN VencChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
	HI_S32 s32Ret = HI_SUCCESS;

#ifdef  XW_MMVA_SUPPORT
	if (hwinfo.bVgsDrawLine)
	{

		if (VencChn < 0 || VencChn >= 5)
		{
			printf("%s, ================Invalid chn: %d\n", __func__, VencChn);
			return -1;
		}

		Args[VencChn].VpssGrp	= VpssGrp;
		Args[VencChn].VpssChn	= VpssChn;
		Args[VencChn].VencChn	= VencChn;
		Args[VencChn].bRun		= TRUE;
		int rr_min_priority, rr_max_priority;	
		struct sched_param thread_dec_param;
		pthread_attr_t thread_dec_attr;
		pthread_attr_init(&thread_dec_attr);
		pthread_attr_getschedparam(&thread_dec_attr, &thread_dec_param);
		pthread_attr_setstacksize(&thread_dec_attr, LINUX_THREAD_STACK_SIZE);					//栈大小
		
		rr_min_priority = sched_get_priority_min(SCHED_RR); 
		rr_max_priority = sched_get_priority_max(SCHED_RR);
		thread_dec_param.sched_priority = (rr_min_priority + rr_max_priority)/2 + 3; 

		pthread_attr_setschedpolicy(&thread_dec_attr, SCHED_RR);
		pthread_attr_setschedparam(&thread_dec_attr, &thread_dec_param);
		pthread_attr_setinheritsched(&thread_dec_attr, PTHREAD_EXPLICIT_SCHED); 	
		
		pthread_create_normal(&Args[VencChn].pid, &thread_dec_attr, _Vpss_To_Venc, &Args[VencChn]);
	}
	else
	{
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
	}
#else
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
#endif
	return s32Ret;
}

/******************************************************************************
* function : venc unbind vpss
******************************************************************************/
HI_S32 SAMPLE_COMM_VENC_UnBindVpss(VENC_CHN VencChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
	HI_S32 s32Ret = HI_SUCCESS;

#ifdef  XW_MMVA_SUPPORT
	if (hwinfo.bVgsDrawLine)
	{
		if (VencChn < 0 || VencChn >= 5)
		{
			printf("%s, ================Invalid chn: %d\n", __func__, VencChn);
			return -1;
		}

		Args[VencChn].bRun = FALSE;

		if (Args[VencChn].pid)
		{
			pthread_join(Args[VencChn].pid, NULL);
			Args[VencChn].pid = 0;
		}
	}
	else
	{
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

	}
#else
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


#endif

	return s32Ret;
}

#endif

int jv_get_vi_maxframrate()		//获取vi帧率，解决20帧时卡顿问题
{
	int fvif_max = 25;
	if((hwinfo.encryptCode==ENCRYPT_200W && (ststatus[0].attr.width*ststatus[0].attr.height >1280*960))||hwinfo.sensor==SENSOR_SC2235)
		fvif_max = 20;
	else
		fvif_max = 25;

	return fvif_max;
}

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

#if 0
	jv_stream_set_attr(3, &ststatus[3].attr);
	jv_stream_start(3);
#endif
	pthread_mutex_init(&stream_mutex, NULL);

#ifdef ZRTSP_SUPPORT
	pthread_mutex_init(&rtsp_mutex, NULL);
#endif

	s_res_cnt = sizeof(s_res_list_mmm)/sizeof(s_res_list_mmm[0]);
	if (VI_HEIGHT == 960)
 	{
 		s_res_list = &s_res_list_mmm[1];
		s_res_cnt--;
 	}
	else if (VI_HEIGHT == 720)
	{
		s_res_list = &s_res_list_mmm[2];
		s_res_cnt -= 2;
	}
	else if (VI_HEIGHT == 1080)
	{
		s_res_list = s_res_list_mmm;
		VpssChn[0] = 1;
		VpssChn[1] = 2;
		VpssChn[2] = 3;
	}
	else
	{
		s_res_list = &s_res_list_mmm[2];
		s_res_cnt -= 2;
	}

	if (VI_CROP_ENABLE)
	{
		s_res_list = s_res_list_mmm;
		VpssChn[0] = 6;

		if (VI_WIDTH == 1440)
		{
			s_res_list_mmm[1].width = VI_WIDTH;
			s_res_list_mmm[1].height = VI_HEIGHT;
			s_res_list = &s_res_list_mmm[1];
			s_res_cnt--;
		}
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
		if (ststatus[i].fd >= 0)
		{
			jv_stream_stop(i);
		}
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
	if(hwinfo.sensor ==SENSOR_SC2235)
		if( rf > 20)
			rf = 20;
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
	{
		// 针对主码流判断是否开启AVBR
		if (0 == channelid)
		{
			enRcMode= hwinfo.bSupportAVBR ? SAMPLE_RC_AVBR : SAMPLE_RC_VBR;
		}
		else
		{
			enRcMode= SAMPLE_RC_VBR;
		}
	}

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
	if(hwinfo.sensor == SENSOR_OV9750)
	{
		if(channelid == 0 && VI_WIDTH*VI_HEIGHT==ststatus[0].attr.width*ststatus[0].attr.height && 
							ststatus[0].attr.framerate > 26)
		{
			ststatus[0].attr.framerate = 26;
			attr->bitrate = __CalcBitrate(attr->width, attr->height, attr->framerate, attr->vencType);
		}
		if(channelid == 1 && VI_WIDTH*VI_HEIGHT==ststatus[0].attr.width*ststatus[0].attr.height && 
							ststatus[1].attr.framerate > 20)
		{
			ststatus[1].attr.framerate = 20;
			attr->bitrate = __CalcBitrate(attr->width, attr->height, attr->framerate, attr->vencType);
		}
	}
	if(VI_WIDTH*VI_HEIGHT == 1920*1080)			//200W的在VPSS修改帧率
	{
		VPSS_CHN_ATTR_S vpssChnAttr;
		int s32SrcFrameRate, s32DstFrameRate;
		float fvif;
		int vif_max = jv_get_vi_maxframrate();
		BOOL blow = jv_sensor_get_b_low_frame(0, &fvif, NULL);
		if(blow)
		{
			s32SrcFrameRate = fvif;
		}
		else
		{
			s32SrcFrameRate = vif_max;
			jv_sensor_set_fps(s32SrcFrameRate);
		}
		printf("blow:%d, s32SrcFrameRate=%d, vif_max:%d\n",blow, s32SrcFrameRate,vif_max);
		s32DstFrameRate = (attr->framerate < s32SrcFrameRate) ? attr->framerate : s32SrcFrameRate;

		HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn[channelid], &vpssChnAttr);
		if(vpssChnAttr.s32SrcFrameRate!=s32SrcFrameRate || 
			vpssChnAttr.s32DstFrameRate!=s32DstFrameRate)
		{
			vpssChnAttr.s32SrcFrameRate = s32SrcFrameRate;
			vpssChnAttr.s32DstFrameRate = s32DstFrameRate;
			HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn[channelid], &vpssChnAttr);
			printf("[%s, %d]: vpssChn=%d, s32SrcFrameRate=%d, s32DstFrameRate=%d\n", __func__, __LINE__, 
				VpssChn[channelid], vpssChnAttr.s32SrcFrameRate, vpssChnAttr.s32DstFrameRate);

			if(channelid == 1)
			{
				VPSS_CHN jpegVpssChn = 4;
				VPSS_EXT_CHN_ATTR_S vpssExtChnAttr;
				HI_MPI_VPSS_GetExtChnAttr(VpssGrp, jpegVpssChn, &vpssExtChnAttr);
				vpssExtChnAttr.s32SrcFrameRate = s32DstFrameRate;
				vpssExtChnAttr.s32DstFrameRate = s32DstFrameRate>5 ? 5:s32DstFrameRate;
				HI_MPI_VPSS_SetExtChnAttr(VpssGrp, jpegVpssChn, &vpssExtChnAttr);
				printf("[%s, %d]: vpssChn=%d, s32SrcFrameRate=%d, s32DstFrameRate=%d\n", __func__, __LINE__, 
					jpegVpssChn, vpssExtChnAttr.s32SrcFrameRate, vpssExtChnAttr.s32DstFrameRate);
			}
		}
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

	ROTATE_E rotate = ROTATE_NONE;
	int bRotate = 0;
	HI_MPI_VPSS_GetRotate(VpssGrp, VpssChn[channelid], &rotate);
	if (rotate == ROTATE_90 || rotate == ROTATE_270)
		bRotate = 1;

	jv_stream_set_roi(channelid, &ststatus[channelid].attr.roiInfo);
	if (!ststatus[channelid].attr.bRectStretch)
	{
		VENC_CROP_CFG_S stGrpCropCfg = {0};
		if (channelid == 0)
			__build_vpss_venc_size_vi(attr->width, attr->height, &size, &stGrpCropCfg);
		else
			__build_vpss_venc_size(bRotate, attr->width, attr->height,ststatus[0].attr.width,ststatus[0].attr.height, &size, &stGrpCropCfg);
		if(attr->width >= attr->height)//走廊模式，旋转90度或者270度的时候，有几个分辨率裁剪会重启，不再裁剪
			CHECK_RET(HI_MPI_VENC_SetCrop(VencChn[channelid], &stGrpCropCfg));
	}
	else		//拉伸的时候处理
	{
		if(bRotate)	//走廊模式，分辨率不需要翻转，这里再翻转回去
		{
			size.u32Width = attr->height;
			size.u32Height = attr->width;
		}
	}
	if (VpssChn[channelid] <= 3)
	{
		VPSS_CHN_MODE_S vpssMode;
		CHECK_RET(HI_MPI_VPSS_GetChnMode(VpssGrp, VpssChn[channelid], &vpssMode));
		vpssMode.u32Width = size.u32Width;
		vpssMode.u32Height = size.u32Height;
		CHECK_RET(HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn[channelid], &vpssMode));
	}
	else
	{
		VPSS_EXT_CHN_ATTR_S vpssExtChnAttr;
		CHECK_RET(HI_MPI_VPSS_GetExtChnAttr(VpssGrp, VpssChn[channelid], &vpssExtChnAttr));
		vpssExtChnAttr.u32Width = size.u32Width;
		vpssExtChnAttr.u32Height = size.u32Height;
		if(VpssChn[channelid] > 3)
		{
			if (bRotate)
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
		if (gp.bNeedZrtsp)
		{
			pthread_mutex_lock(&rtsp_mutex);
			int DataLen = stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset - 4;
			char *t = (char *)stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset;
			if(t[4] == 0x67)
			{
				//printf("SPS0获得,通道%d\n", channelid);
				jvstream_rtsp[channelid].sps[0] = DataLen;
				memcpy(jvstream_rtsp[channelid].sps+1, t+4, DataLen);
			}
			else if(t[4] == 0x68)
			{
				//printf("PPS0获得,通道%d\n", channelid);
				jvstream_rtsp[channelid].pps[0][0] = DataLen;
				memcpy(jvstream_rtsp[channelid].pps[0]+1, t+4, DataLen);
			}
			if(ststatus[channelid].attr.vencType == JV_PT_H264)
			{
				rtsp_nalu_type = (NALU_TYPEs)stStream.pstPack[i].DataType.enH264EType;
			}
			else if(ststatus[channelid].attr.vencType == JV_PT_H265)
			{
				rtsp_nalu_type = (NALU_TYPEs)stStream.pstPack[i].DataType.enH265EType;
			}
			rtsp_offset = stStream.pstPack[i].u32Offset;
			pthread_mutex_unlock(&rtsp_mutex);
		}
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
	int oldWidth, oldHeight;
	jv_venc_rc_mode_e oldRcMode;
	VENC_TYPE oldVencType;
	int oldFramerate = 0;

	pthread_mutex_lock(&stream_mutex);

	oldWidth = ststatus[channelid].attr.width;
	oldHeight = ststatus[channelid].attr.height;
	oldRcMode = ststatus[channelid].attr.rcMode;
	oldVencType = ststatus[channelid].attr.vencType;

	if (attr->framerate > MAX_FRAME_RATE)
		attr->framerate = MAX_FRAME_RATE;
	if(VI_CROP_ENABLE && attr->width == 1920 && attr->height == 1080)
	{
		attr->width = VI_CROP_W;
		attr->height = VI_CROP_H;
	}
	memcpy(&ststatus[channelid].attr, attr, sizeof(jv_stream_attr));

	if (ststatus[channelid].bOpened)
	{
		if(oldWidth!=attr->width || 
			oldHeight!=attr->height || 
			oldRcMode!=attr->rcMode || 
			oldVencType!=attr->vencType)
		{
			//这些参数不支持动态设置，通过重启venc设置生效
			pthread_mutex_unlock(&stream_mutex);
			return 0;
		}

		int rcSrcFrameRate, rcDstFrameRate;
		float fvif;
		int blow = jv_sensor_get_b_low_frame(0, &fvif, NULL);
		int framerate = ststatus[channelid].attr.framerate;
		int nGop = ststatus[channelid].attr.nGOP;

		if(VI_WIDTH*VI_HEIGHT == 1920*1080)			//200W的在VPSS修改帧率
		{
			VPSS_GRP VpssGrp = 0;
			VPSS_CHN_ATTR_S vpssChnAttr;
			int s32SrcFrameRate, s32DstFrameRate;
			s32SrcFrameRate = fvif;
			s32DstFrameRate = (attr->framerate < s32SrcFrameRate) ? attr->framerate : s32SrcFrameRate;

			HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn[channelid], &vpssChnAttr);
			if(vpssChnAttr.s32SrcFrameRate!=s32SrcFrameRate || 
				vpssChnAttr.s32DstFrameRate!=s32DstFrameRate)
			{
				vpssChnAttr.s32SrcFrameRate = s32SrcFrameRate;
				vpssChnAttr.s32DstFrameRate = s32DstFrameRate;
				HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn[channelid], &vpssChnAttr);
				printf("[%s, %d]: vpssChn=%d, s32SrcFrameRate=%d, s32DstFrameRate=%d\n", __func__, __LINE__, 
					VpssChn[channelid], vpssChnAttr.s32SrcFrameRate, vpssChnAttr.s32DstFrameRate);

				if(channelid == 1)
				{
					VPSS_CHN jpegVpssChn = 4;
					VPSS_EXT_CHN_ATTR_S vpssExtChnAttr;
					HI_MPI_VPSS_GetExtChnAttr(VpssGrp, jpegVpssChn, &vpssExtChnAttr);
					vpssExtChnAttr.s32SrcFrameRate = s32DstFrameRate;
					vpssExtChnAttr.s32DstFrameRate = s32DstFrameRate>5 ? 5:s32DstFrameRate;
					HI_MPI_VPSS_SetExtChnAttr(VpssGrp, jpegVpssChn, &vpssExtChnAttr);
					printf("[%s, %d]: vpssChn=%d, s32SrcFrameRate=%d, s32DstFrameRate=%d\n", __func__, __LINE__, 
						jpegVpssChn, vpssExtChnAttr.s32SrcFrameRate, vpssExtChnAttr.s32DstFrameRate);
				}
			}

			rcSrcFrameRate = vpssChnAttr.s32DstFrameRate;
			rcDstFrameRate = rcSrcFrameRate;
			if(framerate != rcSrcFrameRate)
			{
				nGop = nGop / framerate * rcSrcFrameRate;
				framerate = rcSrcFrameRate;
			}
		}
		else
		{
			rcSrcFrameRate = fvif;
			if(framerate <= rcSrcFrameRate)
			{
				rcDstFrameRate = framerate;
			}
			else
			{
				rcDstFrameRate = rcSrcFrameRate;
				nGop = nGop / framerate * rcSrcFrameRate;
				framerate = rcSrcFrameRate;
			}
		}

		HI_MPI_VENC_GetChnAttr(channelid, &stAttr);
		if (ststatus[channelid].attr.rcMode == JV_VENC_RC_MODE_VBR)
		{
			if (hwinfo.bSupportAVBR)
			{
				stAttr.stRcAttr.stAttrH264AVbr.u32MaxBitRate = ststatus[channelid].attr.bitrate;
				stAttr.stRcAttr.stAttrH264AVbr.u32Gop = nGop;
				stAttr.stRcAttr.stAttrH264AVbr.u32StatTime = (nGop+framerate-1)/framerate;
				stAttr.stRcAttr.stAttrH264AVbr.u32SrcFrmRate = rcSrcFrameRate;
				stAttr.stRcAttr.stAttrH264AVbr.fr32DstFrmRate = rcDstFrameRate;
			}
			else
			{
				stAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = ststatus[channelid].attr.bitrate;
				stAttr.stRcAttr.stAttrH264Vbr.u32Gop = nGop;
				stAttr.stRcAttr.stAttrH264Vbr.u32StatTime = (nGop+framerate-1)/framerate;
				stAttr.stRcAttr.stAttrH264Vbr.u32MaxQp =  ststatus[channelid].attr.maxQP;
				stAttr.stRcAttr.stAttrH264Vbr.u32MinQp =  ststatus[channelid].attr.minQP;
				stAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = rcSrcFrameRate;
				stAttr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = rcDstFrameRate;
			}
		}
		else if (ststatus[channelid].attr.rcMode == JV_VENC_RC_MODE_CBR)
		{
			stAttr.stRcAttr.stAttrH264Cbr.u32BitRate = ststatus[channelid].attr.bitrate;
			stAttr.stRcAttr.stAttrH264Cbr.u32Gop = nGop;
			stAttr.stRcAttr.stAttrH264Cbr.u32StatTime = (nGop+framerate-1)/framerate;
			stAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
			stAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = rcSrcFrameRate;
			stAttr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = rcDstFrameRate;
		}
		else
		{
			Printf("ERROR: Unsupport stAttr.stRcAttr.enRcMode: %d\n", stAttr.stRcAttr.enRcMode);
		}

		CHECK_RET(HI_MPI_VENC_SetChnAttr(channelid, &stAttr));
		printf("[%s, %d]: vencChn=%d, rcSrcFrameRate=%d, rcDstFrameRate=%d, gop=%d\n", __func__, __LINE__, 
			channelid, rcSrcFrameRate, rcDstFrameRate, nGop);

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
	// ability->maxStreamRes[1] = 720*576;
	ability->maxStreamRes[1] = 512*288;
	ability->maxStreamRes[2] = 352*288;

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
	CHECK_RET(HI_MPI_VENC_RequestIDR(channelid, HI_FALSE));
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

static BOOL s_bLowBitRate = FALSE;

/**
 * 降低码率
 */
int jv_stream_switch_lowbitrate(int channelid, unsigned int bitrate)
{
	if (s_bLowBitRate)
	{
		return 0;
	}
	// 只在主码流降码率，次码流本身码率已经很低，降码率效果不明显
	if (channelid != 0)
	{
		return -1;
	}

	VENC_RC_PARAM_S rcPara;

	if (hwinfo.bSupportAVBR && (JV_VENC_RC_MODE_VBR == ststatus[channelid].attr.rcMode))
	{
		HI_MPI_VENC_GetRcParam(channelid, &rcPara);
		rcPara.stParamH264AVbr.u32MinIQp = 31;
		rcPara.stParamH264AVbr.u32MaxIQp = 51;
		rcPara.stParamH264AVbr.u32MinQp = 33;
		rcPara.stParamH264AVbr.u32MaxQp = 51;
		rcPara.stParamH264AVbr.u32MaxStillQP = 44;
		rcPara.stParamH264AVbr.s32MinStillPercent = 25;
		// rcPara.stParamH264AVbr.u32MaxIprop = 100;
		rcPara.stParamH264AVbr.s32IPQPDelta = 2;
		CHECK_RET(HI_MPI_VENC_SetRcParam(channelid, &rcPara));
	}
	else
	{
		printf("%s, only valid when avbr, now mode: %d, support avbr: %d\n", __func__, ststatus[channelid].attr.rcMode, hwinfo.bSupportAVBR);
		return -1;
	}

	s_bLowBitRate = TRUE;

	printf("%s, set low bitrate, ch: %d\n", __func__, channelid);

	return 0;
}

/**
 * 恢复码率
 */
int jv_stream_revert_bitrate(int channelid)
{
	if (!s_bLowBitRate)
	{
		return 0;
	}
	if (channelid != 0)
	{
		return -1;
	}

	VENC_RC_PARAM_S rcPara;

	if (hwinfo.bSupportAVBR && (JV_VENC_RC_MODE_VBR == ststatus[channelid].attr.rcMode))
	{
		HI_MPI_VENC_GetRcParam(channelid, &rcPara);
		rcPara.stParamH264AVbr.u32MinIQp =  ststatus[channelid].attr.minQP;
		rcPara.stParamH264AVbr.u32MaxIQp = ststatus[channelid].attr.maxQP;
		rcPara.stParamH264AVbr.u32MinQp = ststatus[channelid].attr.minQP;
		rcPara.stParamH264AVbr.u32MaxQp = ststatus[channelid].attr.maxQP;
		rcPara.stParamH264AVbr.u32MaxStillQP = 40;
		rcPara.stParamH264AVbr.s32MinStillPercent = 50;
		// rcPara.stParamH264AVbr.u32MaxIprop = 50;
		rcPara.stParamH264AVbr.s32IPQPDelta = 0;
		CHECK_RET(HI_MPI_VENC_SetRcParam(channelid, &rcPara));
	}
	else
	{
		printf("%s, only valid when avbr, now mode: %d, support avbr: %d\n", __func__, ststatus[channelid].attr.rcMode, hwinfo.bSupportAVBR);
		return -1;
	}

	jv_stream_request_idr(channelid);

	s_bLowBitRate = FALSE;

	printf("%s, revert bitrate, ch: %d\n", __func__, channelid);

	return 0;
}

int jv_stream_set_graph_callback(jv_stream_get_graph callback)
{
	s_GetGraphCallback = callback;
	return 0;
}

