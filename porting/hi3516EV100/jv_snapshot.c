#include <jv_common.h>
#include "jv_snapshot.h"

#include <mpi_region.h>
#include "hicommon.h"

#define SNAP_W	400
#define SNAP_H	224

#define SNAP_CHN	3
#define SNAP_VPSS	3

static void __jv_snapshot_osd(int group)
{
	RGN_CHN_ATTR_S stChnAttr;
	MPP_CHN_S stChn;

	int handle[4];
	int hCnt = 0;
	RGN_CHN_ATTR_S hRgn[4];
	//用最后一个码流的OSD Region
	{
		int chnid = HWINFO_STREAM_CNT-1;
		stChn.enModId = HI_ID_VENC;
		stChn.s32DevId = 0;
		stChn.s32ChnId = chnid;
		int i;
		for (i=0;i<10;i++)
		{
			int ret = HI_MPI_RGN_GetDisplayAttr(i, &stChn, &stChnAttr);
			if (ret == 0)
			{
				//printf("handle[%d]: %d\n", i, stChnAttr.enType);
				hRgn[hCnt] = stChnAttr;
				handle[hCnt++] = i;
				if (hCnt >= sizeof(handle)/sizeof(handle[0]))
					break;
			}
		}
	}

	stChn.enModId = HI_ID_VENC;
	stChn.s32DevId = 0;
	stChn.s32ChnId = SNAP_CHN;

	memset(&stChnAttr,0,sizeof(stChnAttr));
	stChnAttr.bShow = HI_TRUE;
	stChnAttr.enType = OVERLAY_RGN;

	int i;
	int offset = 0;
	for (i=0;i<hCnt;i++)
	{
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 4;
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 4 + offset;
		stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0; //alpha 为0 的透明度
		stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128; //alpha 为1 的透明度
		stChnAttr.unChnAttr.stOverlayChn.u32Layer = 1;

		stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
		stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

		CHECK_RET(HI_MPI_RGN_AttachToChn(handle[i], &stChn, &stChnAttr));

		RGN_ATTR_S region;
		HI_MPI_RGN_GetAttr(handle[i], &region);
		if (region.enType == OVERLAY_RGN)
			offset += region.unAttr.stOverlay.stSize.u32Height;
		else if (region.enType == OVERLAYEX_RGN)
			offset += region.unAttr.stOverlayEx.stSize.u32Height;
	}
}

/**
 *@brief snap shot
 *@param handle handle of the stream
 *@param channelid channel id
 *@param w width of photo wantted
 *@param h height of photo wantted
 *@param quality quality of photo, value in [1,5]
 *@param data buffer for photo data
 *@retval len if success
 *@retval 0 if failed. JV_ERR_XXX
 */
int jv_snapshot_get_ex(int channelid, unsigned char *pData,unsigned int len,stSnapSize *snap)
{

	stSnapSize size;
	if(!snap || snap->nWith>SNAP_W||snap->nHeight>SNAP_H)
	{
		//printf("snap shot size is too lager!\n");
		size.nWith = SNAP_W;
		size.nHeight = SNAP_H;
	}
	else
	{
		size.nWith = snap->nWith;
		size.nHeight = snap->nHeight;
	}

	VENC_CHN VencChn = SNAP_CHN;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	if(1)
	{
		HI_S32 s32Ret;
		VENC_CHN_ATTR_S stVencChnAttr;
		VENC_ATTR_JPEG_S stJpegAttr;

		//Create
		stVencChnAttr.stVeAttr.enType = PT_JPEG;
		stJpegAttr.u32MaxPicWidth  = size.nWith;
		stJpegAttr.u32MaxPicHeight = size.nHeight;
		stJpegAttr.u32PicWidth  = size.nWith;
		stJpegAttr.u32PicHeight = size.nHeight;
		stJpegAttr.u32BufSize = JV_ALIGN_CEILING(size.nWith,16) * JV_ALIGN_CEILING(size.nHeight,16);
		stJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
		stJpegAttr.bSupportDCF = HI_FALSE;
		memcpy(&stVencChnAttr.stVeAttr.stAttrJpege, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

		s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
		if (HI_SUCCESS != s32Ret)
		{
			printf("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n", VencChn, s32Ret);
			return HI_FAILURE;
		}

		//Bind
		stSrcChn.enModId = HI_ID_VPSS;
		stSrcChn.s32DevId = 0;
		stSrcChn.s32ChnId = SNAP_VPSS;

		stDestChn.enModId = HI_ID_VENC;
		stDestChn.s32DevId = 0;
		stDestChn.s32ChnId = SNAP_CHN;

		s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
		if (s32Ret != HI_SUCCESS)
		{
			printf("HI_MPI_SYS_Bind failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}

		//__jv_snapshot_osd(SNAP_GRP);
	}

	struct pollfd	stPoll;
	stPoll.fd		= HI_MPI_VENC_GetFd(VencChn);
	stPoll.events	= POLLIN;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S	stStream;
	VENC_PACK_S *stPack = NULL;
	stStream.pstPack = NULL;
	XDEBUG(HI_MPI_VENC_StartRecvPic(VencChn), "HI_MPI_VENC_StartRecvPic");

	U32 i, u32Length = 0;

	//获取压缩图像数据流
	if (poll(&stPoll, 1, 500) > 0)
	{
		//前2个字节为前缀
		//3518把前2个字节已经加上了
//		pData[0] = 0xFF;
//		pData[1] = 0xD8;
//		u32Length = 2;

		XDEBUG(HI_MPI_VENC_Query(VencChn, &stStat), "HI_MPI_VENC_Query");
		//根据查询情况设定数据流的包大小
		stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
		if (NULL == stStream.pstPack)
		{
			printf("WARNING: malloc Failed...\n");
			//停止接受图像
			XDEBUG(HI_MPI_VENC_StopRecvPic(VencChn), "HI_MPI_VENC_StopRecvPic");
			XDEBUG(HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn), "HI_MPI_SYS_UnBind");
			XDEBUG(HI_MPI_VENC_DestroyChn(VencChn), "HI_MPI_VENC_DestroyChn");
			return HI_FAILURE;
		}
		stStream.u32PackCount = stStat.u32CurPacks;
		//获取视频数据流
		XDEBUG(HI_MPI_VENC_GetStream(VencChn, &stStream, -1), "HI_MPI_VENC_GetStream");
		//组合数据
		for (i=0; i< stStream.u32PackCount; i++)
		{
			stPack = stStream.pstPack + i;
			if(u32Length>=len || u32Length+stPack->u32Len-stPack->u32Offset>=len)
			{
				printf("pData's length is too smaller\n");
				u32Length = 0;
				break;
			}
			memcpy(&pData[u32Length], stPack->pu8Addr+stPack->u32Offset, stPack->u32Len-stPack->u32Offset);
			u32Length += stPack->u32Len-stPack->u32Offset;
			Printf("u32Offset=%d, u32Len=%d, u32Length=%d\n", stPack->u32Offset, stPack->u32Len, u32Length);
		}
		//添加后缀
//		pData[u32Length++] = 0xFF;
//		pData[u32Length++] = 0xD9;
		//释放视频数据流
		XDEBUG(HI_MPI_VENC_ReleaseStream(VencChn, &stStream), "HI_MPI_VENC_ReleaseStream");
	}
	else
	{
		printf(">>>>>>>>>>>>>>>>>>>>>>>ERROR: poll Failed...\n\n");
		//exit(0);
		u32Length = 0;
	}

	if(stStream.pstPack)
		free(stStream.pstPack);

	//停止接受图像
	XDEBUG(HI_MPI_VENC_StopRecvPic(VencChn), "HI_MPI_VENC_StopRecvPic");

	//解绑定通道
	XDEBUG(HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn), "HI_MPI_SYS_UnBind");
	//销毁通道
	XDEBUG(HI_MPI_VENC_DestroyChn(VencChn), "HI_MPI_VENC_DestroyChn");


	return u32Length;
}

//报警邮件使用固定大小D1
int jv_snapshot_get(int channelid, unsigned char *pData,unsigned int len)
{
	stSnapSize snap;
	snap.nWith = SNAP_W;
	snap.nHeight = SNAP_H;
	return jv_snapshot_get_ex(channelid,pData,len,&snap);
}
int jv_snapshot_get_def_size(stSnapSize *snap)
{
	snap->nWith = SNAP_W;
	snap->nHeight = SNAP_H;
	return 0;
}
