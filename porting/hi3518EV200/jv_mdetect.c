#include "hicommon.h"
#include "jv_common.h"
#include "jv_mdetect.h"
#include "jv_stream.h"
#include <mpi_vda.h>
#include <ivs_md.h>
#include <mpi_ive.h>
#include <mpi_vgs.h>
#include <sys/un.h>

#define MD_VI_W	720
#define MD_VI_H	576

#define MB_WIDTH	(MD_VI_W/16)
#define MB_HEIGHT	(MD_VI_H/16)

#define MD_VPSS_CHN	5

typedef struct
{
	BOOL bCallBackSlience;    /* 软开关,报警静默时使用 */
	jv_mdetect_attr_t attrlist[MAX_STREAM];

	jv_mdetect_callback_t callback_ptr;
	void *callback_param;

	//使用哪一个venc来进行移动检测,lck20120806
	int nVenc;

	char *szEventSnapshotPath;
	int fd;

	U32 	nMBNum; 			//有变化的宏块个数，超过这个值，则认为触发移动检测
	U8		acMB[MB_HEIGHT][MB_WIDTH];

} mdinfo_t;

static mdinfo_t mdinfo;

static jv_thread_group_t group;

#if 1
#define SAMPLE_IVE_MD_IMAGE_NUM 2
#define IVE_ALIGN 16
#define MDCHN  	0
#define VPSSGRP 0
#define VPSSCHN 3

typedef struct hiSAMPLE_IVE_RECT_S
{
	POINT_S astPoint[4];
    HI_U16 u16Thr;
}SAMPLE_IVE_RECT_S;

typedef struct hiSAMPLE_RECT_ARRAY_S
{
    HI_U16 u16Num;
    SAMPLE_IVE_RECT_S astRect[50];
}SAMPLE_RECT_ARRAY_S;

typedef struct hiSAMPLE_IVE_MD_S
{
	IVE_SRC_IMAGE_S astImg[SAMPLE_IVE_MD_IMAGE_NUM];
	IVE_DST_MEM_INFO_S stBlob;
	MD_ATTR_S stMdAttr;
	SAMPLE_RECT_ARRAY_S stRegion;
	VB_POOL hVbPool;
	HI_U16 u16BaseWitdh;
	HI_U16 u16BaseHeight;
}SAMPLE_IVE_MD_S;
static SAMPLE_IVE_MD_S stMd;


#define IVE_MMZ_FREE(phy,vir)\
do{\
	if ((0 != (phy)) && (NULL != (vir)))\
	{\
		 HI_MPI_SYS_MmzFree((phy),(vir));\
		 (phy) = 0;\
		 (vir) = NULL;\
	}\
}while(0)

#define SAMPLE_CHECK_EXPR_GOTO(expr, label, fmt...)\
do\
{\
	if(expr)\
	{\
		printf(fmt);\
		goto label;\
	}\
}while(0)

HI_U16 SAMPLE_COMM_IVE_CalcStride(HI_U16 u16Width, HI_U8 u8Align)
{
	return (u16Width + (u8Align - u16Width%u8Align)%u8Align);
}

HI_S32 SAMPLE_COMM_VGS_FillRect(VIDEO_FRAME_INFO_S *pstFrmInfo, SAMPLE_RECT_ARRAY_S *pstRect,HI_U32 u32Color)
{
    VGS_HANDLE VgsHandle;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U16 i;
    VGS_TASK_ATTR_S stVgsTask;
    VGS_ADD_COVER_S stVgsAddCover;

    if(0 == pstRect->u16Num)
    {
        return s32Ret;
    }
    s32Ret = HI_MPI_VGS_BeginJob(&VgsHandle);
    if(s32Ret != HI_SUCCESS)
    {
        printf("Vgs begin job fail,Error(%#x)\n",s32Ret);
        return s32Ret;
    }

    memcpy(&stVgsTask.stImgIn,pstFrmInfo,sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTask.stImgOut,pstFrmInfo,sizeof(VIDEO_FRAME_INFO_S));

    stVgsAddCover.enCoverType = COVER_QUAD_RANGLE;
    stVgsAddCover.u32Color = u32Color;
    for(i = 0;i < pstRect->u16Num;i++)
    {
        stVgsAddCover.stQuadRangle.bSolid = HI_FALSE;
        stVgsAddCover.stQuadRangle.u32Thick = 2;
        memcpy(stVgsAddCover.stQuadRangle.stPoint,pstRect->astRect[i].astPoint,sizeof(pstRect->astRect[i].astPoint));
        s32Ret = HI_MPI_VGS_AddCoverTask(VgsHandle, &stVgsTask, &stVgsAddCover);
        if(s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VGS_AddCoverTask fail,Error(%#x)\n",s32Ret);
            HI_MPI_VGS_CancelJob(VgsHandle);
            return s32Ret;
        }
    }

    s32Ret = HI_MPI_VGS_EndJob(VgsHandle);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VGS_EndJob fail,Error(%#x)\n",s32Ret);
        HI_MPI_VGS_CancelJob(VgsHandle);
        return s32Ret;
    }
    return s32Ret;
}


HI_VOID SAMPLE_COMM_IVE_BlobToRect(IVE_CCBLOB_S *pstBlob, SAMPLE_RECT_ARRAY_S *pstRect,
                                            HI_U16 u16RectMaxNum,HI_U16 u16AreaThrBase,HI_U16 u16AreaThrStep,
                                            HI_U16 u16SrcWidth, HI_U16 u16SrcHeight,
                                            HI_U16 u16DstWidth,HI_U16 u16DstHeight)
{
    HI_U16 u16Num;
    HI_U16 i,j,k;
    HI_U16 u16Thr= u16AreaThrBase;
	HI_BOOL bValid;

//	printf("===========================================%d\n",pstBlob->u16CurAreaThr);

    if(pstBlob->u8RegionNum > u16RectMaxNum)
    {

		u16Thr = pstBlob->u16CurAreaThr;
		do
		{
			u16Num = 0;
			u16Thr += u16AreaThrStep;
			for(i = 0;i < 254;i++)
			{
				if(pstBlob->astRegion[i].u32Area > u16Thr)
				{
					u16Num++;
				}
			}
		}while(u16Num > u16RectMaxNum);
    }

   u16Num = 0;

   for(i = 0;i < 254;i++)
    {

        if(pstBlob->astRegion[i].u32Area > u16Thr)
        {
            pstRect->astRect[u16Num].astPoint[0].s32X = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Left / (HI_FLOAT)u16SrcWidth * (HI_FLOAT)u16DstWidth) & (~1) ;
			pstRect->astRect[u16Num].astPoint[0].s32Y = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Top / (HI_FLOAT)u16SrcHeight * (HI_FLOAT)u16DstHeight) & (~1);

			pstRect->astRect[u16Num].astPoint[1].s32X = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Right/ (HI_FLOAT)u16SrcWidth * (HI_FLOAT)u16DstWidth) & (~1);
			pstRect->astRect[u16Num].astPoint[1].s32Y = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Top / (HI_FLOAT)u16SrcHeight * (HI_FLOAT)u16DstHeight) & (~1);

			pstRect->astRect[u16Num].astPoint[2].s32X = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Right / (HI_FLOAT)u16SrcWidth * (HI_FLOAT)u16DstWidth) & (~1);
			pstRect->astRect[u16Num].astPoint[2].s32Y = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Bottom / (HI_FLOAT)u16SrcHeight * (HI_FLOAT)u16DstHeight) & (~1);

			pstRect->astRect[u16Num].astPoint[3].s32X = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Left / (HI_FLOAT)u16SrcWidth * (HI_FLOAT)u16DstWidth) & (~1);
			pstRect->astRect[u16Num].astPoint[3].s32Y = (HI_U16)((HI_FLOAT)pstBlob->astRegion[i].u16Bottom / (HI_FLOAT)u16SrcHeight * (HI_FLOAT)u16DstHeight) & (~1);

			pstRect->astRect[u16Num].u16Thr = pstBlob->astRegion[i].u32Area;
			bValid = HI_TRUE;
			for(j = 0; j < 3;j++)
			{
			  for (k = j + 1; k < 4;k++)
			  {
				  if ((pstRect->astRect[u16Num].astPoint[j].s32X == pstRect->astRect[u16Num].astPoint[k].s32X)
				  	 &&(pstRect->astRect[u16Num].astPoint[j].s32Y == pstRect->astRect[u16Num].astPoint[k].s32Y))
				  	{
				  	    bValid = HI_FALSE;
						break;
				     }
			  }
			}
			if (HI_TRUE == bValid)
			{
				u16Num++;
			}
        }
    }

	pstRect->u16Num = u16Num;
}



/******************************************************************************
* function : Dma frame info to  ive image
******************************************************************************/
HI_S32 SAMPLE_COMM_DmaImage(VIDEO_FRAME_INFO_S *pstFrameInfo,IVE_DST_IMAGE_S *pstDst,HI_BOOL bInstant)
{
	HI_S32 s32Ret;
	IVE_HANDLE hIveHandle;
	IVE_SRC_DATA_S stSrcData;
	IVE_DST_DATA_S stDstData;
	IVE_DMA_CTRL_S stCtrl = {IVE_DMA_MODE_DIRECT_COPY,0};
	HI_BOOL bFinish = HI_FALSE;
	HI_BOOL bBlock = HI_TRUE;

	//fill src
	stSrcData.pu8VirAddr = (HI_U8*)pstFrameInfo->stVFrame.pVirAddr[0];
	stSrcData.u32PhyAddr = pstFrameInfo->stVFrame.u32PhyAddr[0];
	stSrcData.u16Width   = (HI_U16)pstFrameInfo->stVFrame.u32Width;
	stSrcData.u16Height  = (HI_U16)pstFrameInfo->stVFrame.u32Height;
	stSrcData.u16Stride  = (HI_U16)pstFrameInfo->stVFrame.u32Stride[0];

	//fill dst
	stDstData.pu8VirAddr = pstDst->pu8VirAddr[0];
	stDstData.u32PhyAddr = pstDst->u32PhyAddr[0];
	stDstData.u16Width   = pstDst->u16Width;
	stDstData.u16Height  = pstDst->u16Height;
	stDstData.u16Stride  = pstDst->u16Stride[0];

	s32Ret = HI_MPI_IVE_DMA(&hIveHandle,&stSrcData,&stDstData,&stCtrl,bInstant);
	if (HI_SUCCESS != s32Ret)
	{
        printf("HI_MPI_IVE_DMA fail,Error(%#x)\n",s32Ret);
       return s32Ret;
    }

	if (HI_TRUE == bInstant)
	{
		s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);
		while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
		{
			usleep(100);
			s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);
		}
		if (HI_SUCCESS != s32Ret)
		{
			printf("HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
		   return s32Ret;
		}
	}

	return HI_SUCCESS;
}



/******************************************************************************
* function : Create memory info
******************************************************************************/
HI_S32 SAMPLE_COMM_IVE_CreateMemInfo(IVE_MEM_INFO_S*pstMemInfo,HI_U32 u32Size)
{
	HI_S32 s32Ret;

	if (NULL == pstMemInfo)
	{
		printf("pstMemInfo is null\n");
		return HI_FAILURE;
	}
	pstMemInfo->u32Size = u32Size;
	s32Ret = HI_MPI_SYS_MmzAlloc(&pstMemInfo->u32PhyAddr, (void**)&pstMemInfo->pu8VirAddr, NULL, HI_NULL, u32Size);
	if(s32Ret != HI_SUCCESS)
	{
		printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/******************************************************************************
* function : Create ive image
******************************************************************************/
HI_S32 SAMPLE_COMM_IVE_CreateImage(IVE_IMAGE_S *pstImg,IVE_IMAGE_TYPE_E enType,HI_U16 u16Width,HI_U16 u16Height)
{
	HI_U32 u32Size = 0;
	HI_S32 s32Ret;
	if (NULL == pstImg)
	{
		printf("pstImg is null\n");
		return HI_FAILURE;
	}

	pstImg->enType = enType;
	pstImg->u16Width = u16Width;
	pstImg->u16Height = u16Height;
	pstImg->u16Stride[0] = SAMPLE_COMM_IVE_CalcStride(pstImg->u16Width,IVE_ALIGN);

	switch(enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		{
			u32Size = pstImg->u16Stride[0] * pstImg->u16Height;
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		{
			u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 3 / 2;
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
			pstImg->u16Stride[1] = pstImg->u16Stride[0];
			pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;
			pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;

		}
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		{
			u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 2;
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
			pstImg->u16Stride[1] = pstImg->u16Stride[0];
			pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;
			pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;

		}
		break;
	case IVE_IMAGE_TYPE_YUV420P:
		break;
	case IVE_IMAGE_TYPE_YUV422P:
		break;
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
		break;
	case IVE_IMAGE_TYPE_S8C2_PLANAR:
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		{

			u32Size = pstImg->u16Stride[0] * pstImg->u16Height * sizeof(HI_U16);
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		{
			u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 3;
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
			pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] +1;
			pstImg->pu8VirAddr[2] = pstImg->pu8VirAddr[1] + 1;
			pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + 1;
			pstImg->u32PhyAddr[2] = pstImg->u32PhyAddr[1] + 1;
			pstImg->u16Stride[1] = pstImg->u16Stride[0];
			pstImg->u16Stride[2] = pstImg->u16Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		break;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
		{
			u32Size = pstImg->u16Stride[0] * pstImg->u16Height * sizeof(HI_U32);
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
		}
		break;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		{

			u32Size = pstImg->u16Stride[0] * pstImg->u16Height * sizeof(HI_U64);
			s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], NULL, HI_NULL, u32Size);
			if(s32Ret != HI_SUCCESS)
			{
				printf("Mmz Alloc fail,Error(%#x)\n",s32Ret);
				return s32Ret;
			}
		}
		break;
	default:
		break;

	}

	return HI_SUCCESS;
}

static HI_VOID SAMPLE_IVE_Md_Uninit(SAMPLE_IVE_MD_S *pstMd)
{
	HI_S32 i;
	HI_S32 s32Ret = HI_SUCCESS;

	for (i = 0; i < SAMPLE_IVE_MD_IMAGE_NUM; i++)
	{
    	IVE_MMZ_FREE(pstMd->astImg[i].u32PhyAddr[0],pstMd->astImg[i].pu8VirAddr[0]);
	}

    IVE_MMZ_FREE(pstMd->stBlob.u32PhyAddr,pstMd->stBlob.pu8VirAddr);

	s32Ret = HI_IVS_MD_Exit();
	if(s32Ret != HI_SUCCESS)
	{
	   printf("HI_IVS_MD_Exit fail,Error(%#x)\n",s32Ret);
	   return ;
	}

}


static HI_S32 SAMPLE_IVE_Md_Init(SAMPLE_IVE_MD_S *pstMd,HI_U16 u16ExtWidth,HI_U16 u16ExtHeight,
			HI_U16 u16BaseWidth,HI_U16 u16BaseHeight,HI_CHAR *pchFileName)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 i ;
	HI_U32 u32Size;
	HI_U8 u8WndSz;

   	memset(pstMd,0,sizeof(SAMPLE_IVE_MD_S));
	for (i = 0;i < SAMPLE_IVE_MD_IMAGE_NUM;i++)
	{
		s32Ret = SAMPLE_COMM_IVE_CreateImage(&pstMd->astImg[i],IVE_IMAGE_TYPE_U8C1,u16ExtWidth,u16ExtHeight);
		if(s32Ret != HI_SUCCESS)
		{
			printf("SAMPLE_COMM_IVE_CreateImage fail\n");
		   goto MD_INIT_FAIL;
		}
	}
	u32Size = sizeof(IVE_CCBLOB_S);
	s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&pstMd->stBlob,u32Size);
    if(s32Ret != HI_SUCCESS)
	{
	   printf("SAMPLE_COMM_IVE_CreateMemInfo fail\n");
	   goto MD_INIT_FAIL;
	}

	pstMd->u16BaseWitdh = u16BaseWidth;
	pstMd->u16BaseHeight = u16BaseHeight;
	pstMd->stMdAttr.enAlgMode = MD_ALG_MODE_BG;
	pstMd->stMdAttr.enSadMode = IVE_SAD_MODE_MB_4X4;
	pstMd->stMdAttr.enSadOutCtrl = IVE_SAD_OUT_CTRL_THRESH;
	pstMd->stMdAttr.u16SadThr = 100;//100 * (1 << 2);
	pstMd->stMdAttr.u16Width = u16ExtWidth;
	pstMd->stMdAttr.u16Height = u16ExtHeight;
	pstMd->stMdAttr.stAddCtrl.u0q16X = 32768;
	pstMd->stMdAttr.stAddCtrl.u0q16Y = 32768;
	u8WndSz = ( 1 << (2 + pstMd->stMdAttr.enSadMode));
	pstMd->stMdAttr.stCclCtrl.u16InitAreaThr = 4;//u8WndSz * u8WndSz;
	pstMd->stMdAttr.stCclCtrl.u16Step = 2;

	s32Ret = HI_IVS_MD_Init();
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_IVS_MD_Init fail,Error(%#x)\n",s32Ret);
		goto MD_INIT_FAIL;
	}

MD_INIT_FAIL:

    if(HI_SUCCESS != s32Ret)
	{
        SAMPLE_IVE_Md_Uninit(pstMd);
	}
	return s32Ret;

}

#endif

static int __bRectAndRect(RECT *pRect0,SAMPLE_IVE_RECT_S *pRect1)
{
	int zx = abs(pRect0->x+pRect0->x+pRect0->w - pRect1->astPoint[0].s32X - pRect1->astPoint[1].s32X); //两个矩形重心在x轴上的距离的两倍
	int x = pRect0->w+abs(pRect1->astPoint[0].s32X-pRect1->astPoint[1].s32X); //两矩形在x方向的边长的和
	int zy = abs(pRect0->y+pRect0->y+pRect0->h - pRect1->astPoint[0].s32Y - pRect1->astPoint[3].s32Y); //重心在y轴上距离的两倍
	int y = pRect0->h + abs(pRect1->astPoint[0].s32Y - pRect1->astPoint[3].s32Y); //y方向边长的和
	if (zx <= x && zy <= y)
		return 1;
	return 0;
}

static void _jv_mdetect_process(void *param)
{

	S32	s32Ret, i, j, nResult;
	SAMPLE_IVE_MD_S *pstMd;
	MD_ATTR_S *pstMdAttr;
	VIDEO_FRAME_INFO_S stFrmInfo;

	HI_S32 s32GetFrameMilliSec = 2000;
	HI_S32 s32SetFrameMilliSec = 2000;

	HI_BOOL bInstant = TRUE;
	HI_S32 s32CurIdx = 0;
	HI_BOOL bFirstFrm = TRUE;

	BOOL bMDStatus[]= {0, 0, 0, 0};

	int nThreshold;

	pthreadinfo_add((char *)__func__);

	pstMd = (SAMPLE_IVE_MD_S *) (param);
	pstMdAttr = &(pstMd->stMdAttr);

	while (group.running)
	{
		if (mdinfo.fd == -1)
		{
			usleep(100 * 1000);
			continue;
		}

//		nThreshold = (100-mdinfo.attrlist[0].nSensitivity)*21+176;	//352X288
		nThreshold = (100-mdinfo.attrlist[0].nSensitivity)*2+60;	//160X120
		pthread_mutex_lock(&group.mutex);
		s32Ret = HI_MPI_VPSS_GetChnFrame(VPSSGRP, VPSSCHN, &stFrmInfo, s32GetFrameMilliSec);
		if (HI_SUCCESS != s32Ret)
		{
			pthread_mutex_unlock(&group.mutex);
			printf("HI_MPI_VPSS_GetChnFrame chn(%d) fail,Error(%#x)\n", VPSSCHN, s32Ret);
			usleep(50*1000);
			continue;
		}
#ifdef XW_MMVA_SUPPORT
		extern int jv_mva_analysis( VIDEO_FRAME_INFO_S * srcImg);

		jv_mva_analysis(&stFrmInfo);
#endif
		if (TRUE != bFirstFrm)
		{
			s32Ret = SAMPLE_COMM_DmaImage(&stFrmInfo, &pstMd->astImg[s32CurIdx], bInstant);
			SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, EXT_RELEASE, "SAMPLE_COMM_DmaImage fail,Error(%#x)\n", s32Ret);
		}
		else
		{
			s32Ret = SAMPLE_COMM_DmaImage(&stFrmInfo,
					&pstMd->astImg[1 - s32CurIdx], bInstant);
			SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, EXT_RELEASE,
					"SAMPLE_COMM_DmaImage fail,Error(%#x)\n", s32Ret);

			bFirstFrm = FALSE;
			goto CHANGE_IDX;
			//first frame just init reference frame
		}

		s32Ret = HI_IVS_MD_Process(MDCHN, &pstMd->astImg[s32CurIdx], &pstMd->astImg[1-s32CurIdx],NULL ,&pstMd->stBlob);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, EXT_RELEASE, "HI_IVS_MD_Process fail,Error(%#x)\n", s32Ret);

		SAMPLE_COMM_IVE_BlobToRect((IVE_CCBLOB_S *) pstMd->stBlob.pu8VirAddr, &(pstMd->stRegion), 50, nThreshold, 8, pstMdAttr->u16Width,
				pstMdAttr->u16Height, pstMd->u16BaseWitdh, pstMd->u16BaseHeight);

		nResult	= 0;
		if(pstMd->stRegion.u16Num>0)
		{
			if(mdinfo.attrlist[0].cnt > 0)
			{
				for(i=0;i<mdinfo.attrlist[0].cnt;i++)
				{
					for(j=0;j<pstMd->stRegion.u16Num;j++)
					{
						if(__bRectAndRect(&mdinfo.attrlist[0].rect[i],&pstMd->stRegion.astRect[j]))
							nResult++;
					}
				}
			}
			else
				nResult++;
		}

		bMDStatus[1] = bMDStatus[0];
		bMDStatus[0] = (nResult > 0);
		if (bMDStatus[0] && bMDStatus[1])	//开始报警
		{
#if 0
			printf("==========================MD AlarmIng:%d\n",pstMd->stRegion.u16Num);
			for(i=0;i<pstMd->stRegion.u16Num;i++)
			{
				printf("===========================MD u16Thr%d:%d\n",i,pstMd->stRegion.astRect[i].u16Thr);
			}
#endif
			if (mdinfo.callback_ptr && mdinfo.bCallBackSlience == FALSE)
			{
				mdinfo.callback_ptr(0, mdinfo.callback_param);
			}
		}

		//Draw rect
		//s32Ret = SAMPLE_COMM_VGS_FillRect(&stBaseFrmInfo, &pstMd->stRegion, 0x0000FF00);
		//SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, BASE_RELEASE, "SAMPLE_COMM_VGS_FillRect fail,Error(%#x)\n",s32Ret);

		CHANGE_IDX: //Change reference and current frame index
		s32CurIdx = 1 - s32CurIdx;

		EXT_RELEASE:
		s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VPSSGRP, VPSSCHN, &stFrmInfo);
		if (HI_SUCCESS != s32Ret)
		{
			printf("HI_MPI_VPSS_ReleaseChnFrame fail, chn(%d),Error(%#x)\n", VPSSCHN, s32Ret);
		}
		pthread_mutex_unlock(&group.mutex);
	}

	return ;
}

/**
 *@brief do initialize of motion detection
 *@return 0 if success.
 *
 */
int jv_mdetect_init(void)
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U16 u16ExtWidth = 160;
	HI_U16 u16ExtHeight = 120;
	HI_U16 u16BaseWidth = (HI_U16)VI_WIDTH;
	HI_U16 u16BaseHeight = (HI_U16)VI_HEIGHT;
	s32Ret = SAMPLE_IVE_Md_Init(&stMd, u16ExtWidth, u16ExtHeight, u16BaseWidth, u16BaseHeight, NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("SAMPLE_IVE_Md_Init fail\n");
		return 0;
	}
//	pthread_create(&group.thread, NULL, SAMPLE_IVE_MdProc, (HI_VOID*) &stMd);
	memset(&mdinfo, 0, sizeof(mdinfo));
	group.running = TRUE;
	mdinfo.fd = -1;
	mdinfo.bCallBackSlience = FALSE;
	pthread_mutex_init(&group.mutex, NULL);
	pthread_create(&group.thread, NULL, (void *)_jv_mdetect_process, (HI_VOID*) &stMd);
	return 0;
}

/**
 *@brief do de-initialize of motion detection
 *@return 0 if success.
 *
 */
int jv_mdetect_deinit(void)
{
	group.running = FALSE;
	pthread_join(group.thread, NULL);
	pthread_mutex_destroy(&group.mutex);
	return 0;
}

/**
 *@brief set attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of motion detect.
 *@return 0 if success.
 *
 */
int jv_mdetect_set_attr(int channelid, jv_mdetect_attr_t *attr)
{
	U32 i, j, k, x, y, w, h, n=0;
	int ratio;

	unsigned int viW, viH;
	jv_assert(channelid >=0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);

	memcpy(&mdinfo.attrlist[channelid], attr, sizeof(jv_mdetect_attr_t));

	RECT *pRect;

	jv_stream_get_vi_resolution(0, &viW, &viH);
	memset(mdinfo.acMB, 0, MB_WIDTH * MB_HEIGHT);
	for (i=0; i<mdinfo.attrlist[channelid].cnt; i++)
	{
		pRect = &mdinfo.attrlist[channelid].rect[i];
		x = pRect->x / 16;
		y = pRect->y / 16;
		w = pRect->w / 16;
		h = pRect->h / 16;
		if (w<=0 || h<=0)
		{
			Printf("Error area:x:%d, y:%d, w:%d, h:%d\n", x, y, w, h);
			continue;
		}

		x = x * MD_VI_W / viW;
		y = y * MD_VI_H / viH;
		w = w * MD_VI_W / viW;
		h = h * MD_VI_H / viH;

		//计算要检测的宏块和宏块个数
		for (j=0; j<h; j++)
		{
			for (k=0; k<w; k++)
			{
				mdinfo.acMB[j+y][k+x] = 1;
				n++;
			}
		}
		Printf("Rect%d:{%d,%d,%d,%d}\n", i, pRect->x, pRect->y, pRect->w, pRect->h);
	}

	//根据灵敏度，调整宏块范围，从2个到30个
#define MAX_MODIFIED	30
	ratio = (100-mdinfo.attrlist[channelid].nSensitivity);
	mdinfo.nMBNum = MAX_MODIFIED * ratio/100;
	if (mdinfo.nMBNum < 2)
		mdinfo.nMBNum = 2;

	if (mdinfo.nMBNum >= n/3)
		mdinfo.nMBNum = n/3;

	Printf("nRectMB=%d, nMDMB=%d\n", n, mdinfo.nMBNum);

	return 0;
}

/**
 *@brief get attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of motion detect.
 *@return 0 if success.
 *
 */
int jv_mdetect_get_attr(int channelid, jv_mdetect_attr_t *attr)
{
	jv_assert(channelid >=0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	memcpy(attr, &mdinfo.attrlist[channelid], sizeof(jv_mdetect_attr_t));
	return 0;
}

/**
 *@brief start motion detection
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param callback funcptr will be called when motion detected.
 *@param param when callback occured, param is filled in the parameter
 *@return 0 if success.
 *
 */
int jv_mdetect_start(int channelid, jv_mdetect_callback_t callback, void *param)
{

	HI_S32 s32Ret = HI_SUCCESS;

	mdinfo.callback_ptr = callback;
	mdinfo.callback_param = param;
	mdinfo.bCallBackSlience = FALSE;

	pthread_mutex_lock(&group.mutex);
	s32Ret = HI_IVS_MD_CreateChn(MDCHN, &(stMd.stMdAttr));
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_IVS_MD_CreateChn fail,Error(%#x)\n", s32Ret);
		return -1;
	}
	HI_MPI_VPSS_SetDepth(VPSSGRP, VPSSCHN, 1);
	mdinfo.fd = 1;
	pthread_mutex_unlock(&group.mutex);

	return 0;
}

/**
 *@brief stop motion detection
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_mdetect_stop(int channelid)
{
	HI_S32 s32Ret = HI_SUCCESS;
	pthread_mutex_lock(&group.mutex);
	if (mdinfo.fd != -1)
	{
		s32Ret = HI_IVS_MD_DestroyChn(MDCHN);
		if (HI_SUCCESS != s32Ret)
		{
			printf("HI_IVS_MD_DestroyChn fail,Error(%#x)\n", s32Ret);
		}
		mdinfo.fd = -1;
	}
	pthread_mutex_unlock(&group.mutex);
	return 0;
}

void jv_mdetect_set_sensitivity(int channelid, int sens)
{
	mdinfo.attrlist[channelid].nSensitivity = sens;

	//根据灵敏度，调整门槛
	MD_ATTR_S md_attr;
	memset((char *)&md_attr, 0, sizeof(md_attr));
	HI_IVS_MD_GetChnAttr(MDCHN, &md_attr);
	//1-100->400-50=>y=-3.5x+403.5
	md_attr.u16SadThr = 403.5 - 3.5 * sens;
	HI_IVS_MD_SetChnAttr(MDCHN, &md_attr);
	printf("MD thr=%d\n", md_attr.u16SadThr);
}

void jv_mdetect_silence_callback(BOOL flag)
{
	mdinfo.bCallBackSlience = flag;
}
