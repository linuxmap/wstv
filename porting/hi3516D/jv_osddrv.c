#include "hicommon.h"
#include "jv_osddrv.h"
#include <mpi_region.h>
#include <jv_stream.h>

#define OVERLAY_ALIGN	16

static jv_osddrv_mapping_info_t osdinfo[MAX_OSD_WINDOW];

/**
 *@file NXP的OSD，是使用共享内存的方式实现的。
 *具体说明，可参考代码，以及jovision\porting\nxp88xx\nxp\others\venc程序修改说明.txt
 *
 */

static int _jv_osddrv_get_pix_bytes(jv_osddrv_color_type_t type)
{
	switch (type)
	{
	case OSDDRV_COLOR_TYPE_8_CLUT8 :return 1;
	case OSDDRV_COLOR_TYPE_16_ARGB1555:return 2;
	case OSDDRV_COLOR_TYPE_16_ARGB4444:return 2;
	case OSDDRV_COLOR_TYPE_16_RGB565:return 2;
	case OSDDRV_COLOR_TYPE_24_RGB888:return 3;
	default:
	case OSDDRV_COLOR_TYPE_32_ARGB8888:return 4;
	}
	return 0;
}

/**
 *@file NXP的OSD，是使用共享内存的方式实现的。
 *具体说明，可参考代码，以及jovision\porting\nxp88xx\nxp\others\venc程序修改说明.txt
 *
 */

static PIXEL_FORMAT_E _jv_osddrv_get_pix_type(jv_osddrv_color_type_t type)
{
	switch (type)
	{
	case OSDDRV_COLOR_TYPE_8_CLUT8 :return PIXEL_FORMAT_RGB_8BPP;
	case OSDDRV_COLOR_TYPE_16_ARGB1555:return PIXEL_FORMAT_RGB_1555;
	case OSDDRV_COLOR_TYPE_16_ARGB4444:return PIXEL_FORMAT_RGB_4444;
	case OSDDRV_COLOR_TYPE_16_RGB565:return PIXEL_FORMAT_RGB_565;
	case OSDDRV_COLOR_TYPE_24_RGB888:return PIXEL_FORMAT_RGB_888;
	default:
	case OSDDRV_COLOR_TYPE_32_ARGB8888:return PIXEL_FORMAT_RGB_8888;
	}
	return 0;
}

/**
 *@brief do initialize of osd driver
 *初始化
 *@return 0 if success.
 *
 */
int jv_osddrv_init(void)
{
	int i;
	memset(&osdinfo, 0, sizeof(osdinfo));
	for (i=0;i<MAX_OSD_WINDOW;i++)
	{
		osdinfo[i].channelid = -1;
	}
	return 0;
}

/**
 *@brief do de-initialize of osd driver
 *结束
 *@return 0 if success.
 *
 */
int jv_osddrv_deinit(void)
{
	return 0;
}


/**
 *@brief 修正XY的位置。
 *
 *@param rect  输入 原始位置和大小
 *@param os    输入 原始大小
 *@param ns    输入 新大小
 *@param xy    输出 新位置
 */
static int _fix_xy(RECT *rect, SIZE_S *os, SIZE_S *ns, POINT_S *xy)
{
	//直接改比例
#define XY_FIX(x, nw, ow)	((x)*(nw)/(ow))

//直接改比例，可能会导致超出显示范围
#define XY_FIX_WITH_NEW_WH(x, w, nw, ow)	((((x)+(w))*(nw)/(ow)) - (w))

	if (rect->x < os->u32Width/2)//在右边
	{
		xy->s32X = JV_ALIGN_CEILING(XY_FIX(rect->x, ns->u32Width, os->u32Width), OVERLAY_ALIGN);
	}
	else
	{
		xy->s32X = JV_ALIGN_CEILING(XY_FIX_WITH_NEW_WH(rect->x, rect->w, ns->u32Width, os->u32Width), OVERLAY_ALIGN);
	}
	if (rect->y < os->u32Height/2)//在右边
	{
		xy->s32Y = JV_ALIGN_CEILING(XY_FIX(rect->y, ns->u32Height, os->u32Height), OVERLAY_ALIGN);
	}
	else
	{
		xy->s32Y = JV_ALIGN_CEILING(XY_FIX_WITH_NEW_WH(rect->y, rect->h, ns->u32Height, os->u32Height), OVERLAY_ALIGN);
	}
	return 0;
}

/**
 *@brief 每个通道可以创建的最多区域数
 */
int jv_osddrv_max_region(void)
{
	return 2;
}

/**
 *@brief create osd region 
 *@param channelid The id of the channel.
 *@param attr osd region attribute fro create a osd region
 *@retval <0 error happened
 *@retval >=0 handle of osd
 *
 */
int jv_osddrv_create(int channelid, jv_osddrv_region_attr *attr)
{
	int s32Ret;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	int handle;

	SIZE_S mainSize,minorSize,viSize;
	POINT_S xy;

	jv_assert(attr, return JVERR_BADPARAM);
#if 0
	handle = channelid;
#else
	for (handle=0; handle<MAX_OSD_WINDOW; handle++)
	{
		if (osdinfo[handle].channelid == -1)
			break;
	}
	jv_assert(handle < MAX_OSD_WINDOW, return JVERR_NO_FREE_RESOURCE);
#endif
	//REGION，当使能OSD反色时，必须16字节对齐
	attr->rect.w = JV_ALIGN_CEILING(attr->rect.w, OVERLAY_ALIGN);
	attr->rect.h = JV_ALIGN_CEILING(attr->rect.h, OVERLAY_ALIGN);
	attr->rect.x = JV_ALIGN_FLOOR(attr->rect.x, OVERLAY_ALIGN);
	attr->rect.y = JV_ALIGN_FLOOR(attr->rect.y, OVERLAY_ALIGN);
	osdinfo[handle].rect = attr->rect;
	osdinfo[handle].type = attr->type;
	osdinfo[handle].pitch = attr->rect.w * _jv_osddrv_get_pix_bytes(attr->type);
	osdinfo[handle].len = osdinfo[handle].pitch * osdinfo[handle].rect.h;
	osdinfo[handle].buffer = malloc(osdinfo[handle].len);
	if (osdinfo[handle].buffer == NULL)
	{
		Printf("Failed get buffer!\n");
		return JVERR_NO_FREE_MEMORY;
	}
	memset(osdinfo[handle].buffer, 0x0, osdinfo[handle].len);
	osdinfo[handle].channelid = channelid;
//printf("x,y,w,h: %d,%d,%d,%d\n", osdinfo[handle].rect.x, osdinfo[handle].rect.y, osdinfo[handle].rect.w, osdinfo[handle].rect.h);
	//创建REGION，并将它绑定到指定的通道
	stRgnAttr.enType = OVERLAY_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = _jv_osddrv_get_pix_type(attr->type);
	stRgnAttr.unAttr.stOverlay.stSize.u32Width	= osdinfo[handle].rect.w;
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = osdinfo[handle].rect.h;
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00;

	s32Ret = HI_MPI_RGN_Create(handle, &stRgnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		Printf("HI_MPI_RGN_Create (%d) failed with %#x!\n", \
			   handle, s32Ret);
		free(osdinfo[handle].buffer);
		osdinfo[handle].buffer = NULL;
		HI_MPI_RGN_Destroy(handle);
		osdinfo[handle].channelid = -1;
		return JVERR_UNKNOWN;
	}


	stChn.enModId = HI_ID_VENC;
	stChn.s32DevId = 0;
	stChn.s32ChnId = channelid;
	jv_stream_get_vi_resolution(channelid, &viSize.u32Width, &viSize.u32Height);

	memset(&stChnAttr,0,sizeof(stChnAttr));
	stChnAttr.bShow = HI_TRUE;
	stChnAttr.enType = OVERLAY_RGN;

//	jv_stream_attr stAttr;
	//jv_stream_get_attr(channelid, &stAttr);
	//mainSize.u32Width = stAttr.width;
	//mainSize.u32Height = stAttr.height;
	//_fix_xy(&osdinfo[handle].rect, &viSize, &mainSize, &xy);
	xy.s32X = osdinfo[handle].rect.x;
	xy.s32Y = osdinfo[handle].rect.y;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = xy.s32X;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = xy.s32Y;
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0; //alpha 为0 的透明度
	stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128; //alpha 为1 的透明度
	stChnAttr.unChnAttr.stOverlayChn.u32Layer = 1;

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

//	printf("create channelid: %d, x: %d,%d, bInvcolEn: %d, layer: %d\n",
//			channelid,
//			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X,
//			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y,
//			stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn,
//			stChnAttr.unChnAttr.stOverlayChn.u32Layer
//			);
	CHECK_RET(HI_MPI_RGN_AttachToChn(handle, &stChn, &stChnAttr));
	return handle;
}

/**
 *@brief destroy osd region 
 *@param handle It is the retval of jv_osddrv_create
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_destroy(int handle)
{
	MPP_CHN_S stChn;
	jv_assert(handle < MAX_OSD_WINDOW, return JVERR_BADPARAM);
	if (handle < 0 || osdinfo[handle].channelid<0)
		return 0;
	if (osdinfo[handle].buffer)
		free(osdinfo[handle].buffer);
	osdinfo[handle].buffer = NULL;

	stChn.enModId = HI_ID_VENC;
	stChn.s32DevId = 0;
	stChn.s32ChnId = osdinfo[handle].channelid;

//	CPrintf("detach chn: %d, %d\n", handle, osdinfo[handle].channelid);
	CHECK_RET(HI_MPI_RGN_DetachFromChn(handle,&stChn));

	CHECK_RET(HI_MPI_RGN_Destroy(handle));
	osdinfo[handle].channelid = -1;
	return 0;
}

/**
 *@brief clear the draw buffer. erase all
 *
 *@param handle It is the retval of #jv_osddrv_create
 */
int jv_osddrv_clear(int handle)
{
	if (handle < 0 || handle > MAX_OSD_WINDOW)
		return -1;
	CommonColor_t cc;
	jv_osddrv_get_common_color(&cc);
	unsigned char high,low;
	high = (cc.clear&0xFF00) >> 8;
	low = cc.clear&0xFF;
	if (high == low)
	{
		memset(osdinfo[handle].buffer, high, osdinfo[handle].len);
	}
	else
	{
		int i;
		int len = osdinfo[handle].len/2;
		unsigned short *ptr = (unsigned short *)osdinfo[handle].buffer;
		for (i=0;i<len;i++)
		{
			*ptr++ = cc.clear;
		}
	}
	return 0;
}

/**
 *@brief draw bitmap 
 *@param handle It is the retval of jv_osddrv_create
 *@param rect region to draw 
 *@param data pointer to data buffer, and It's length depend on the colortype of the osd handle
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_drawbitmap(int handle, RECT *rect, unsigned char *data)
{
	int i,j;
	int w,h;
	unsigned short *ptr = (unsigned short *)osdinfo[handle].buffer;
	unsigned short *src = (unsigned short *)data;

	ptr += (rect->y * osdinfo[handle].pitch/2) + rect->x;

	w = MIN(rect->w, osdinfo[handle].rect.w);
	h = MIN(rect->h, osdinfo[handle].rect.h);
//	printf("w: %d, rect->w: %d, drv w: %d\n", w, rect->w, osdinfo[handle].rect.w);
	for (i=0; i<h; i++)
	{
		for (j=0; j<w; j++)
		{
			ptr[j] = src[j];
		}
		ptr += osdinfo[handle].pitch/2;
		src += rect->w;
	}

	return JVERR_NO;
}

/**
 *@brief get map address of the osd
 *@note with mapped address, you should #jv_osddrv_flush to enable your drawing
 *@param handle It is the retval of jv_osddrv_create
 *@param map output, the mapping info
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_get_mapping(int handle, jv_osddrv_mapping_info_t *map)
{
	jv_assert(handle < MAX_OSD_WINDOW, return JVERR_BADPARAM);
	memcpy(map, &osdinfo[handle], sizeof(jv_osddrv_mapping_info_t));
	return 0;
}

/**
 *@brief flush the region
 * sometimes, the osd need to flush to enable the drawing.
 *@param handle It is the retval of jv_osddrv_create
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_flush(int handle)
{
	BITMAP_S bitmap;
	bitmap.enPixelFormat = _jv_osddrv_get_pix_type(osdinfo[handle].type);
	bitmap.pData = osdinfo[handle].buffer;
	bitmap.u32Width = osdinfo[handle].rect.w;
	bitmap.u32Height = osdinfo[handle].rect.h;

	CHECK_RET(HI_MPI_RGN_SetBitMap(handle, &bitmap));
	return 0;
}

/**
 *@brief color convert
 * convert color from structure to unsigned int value
 *@param color color structure pointer
 *@retval <0 error happened
 *@retval 0 success
 *
 */
unsigned int jv_osddrv_color2uint(jv_osddrv_color_t *color)
{
	return 0;
}

/**
 *@brief color convert
 * convert color from unsigned int value to structure 
 *@param value It is the value of the color in unsigned int
 *@param type the type of color to convert
 *@param color output, the structure you wantted
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_uint2color(unsigned int value, jv_osddrv_color_type_t type, jv_osddrv_color_t *color)
{
	return 0;
}

int jv_osddrv_get_common_color(CommonColor_t *cc)
{
	cc->clear = 0;
	cc->white = 0xFFFF;
	cc->black = 0x8000;
	cc->gray = 0x9CE7;
	return 0;
}

