#include "hicommon.h"
#include "jv_common.h"
#include "jv_mdetect.h"
#include "jv_stream.h"
#include "jv_osddrv.h"
#include "mpi_ive.h"
#include "jv_mva.h"

#include <math.h>


#undef MVA_PRT
#ifndef MVA_PRT
#define MVA_PRT(fmt...)  \
do{\
		printf("[%s]:%d ", __FUNCTION__, __LINE__);\
		printf(fmt); \
} while(0)
#endif
//#define MVA_MMZ_CACHE

#define MAX_MMZ_ALLOC_NUMS	8
#define MAX_MOTIONOBJ_NUM  	MAX_MMZ_ALLOC_NUMS

typedef struct
{
    int imgWidth;
    int imgHeight;
    U32 frames;
    U8 index;

    U32 imgAddr[MAX_MMZ_ALLOC_NUMS];
    void *virtAddr[MAX_MMZ_ALLOC_NUMS];
    U32 memSize[MAX_MMZ_ALLOC_NUMS];
    U8 *mapAddr[MAX_MMZ_ALLOC_NUMS];
}MVA_HANDLE;


static stMVAInfo_t  mva_info;
static MVA_HANDLE *IveHandle=NULL ;
//连通域分析
static  jv_mva_ccl_analysis 	f_ccl_analysis=NULL;
//挂起计算
static jv_mva_getsuspends	f_get_suspends=NULL;
//遮挡分析
static jv_mva_hide_analysis  f_mva_hide_analysis=NULL;

#ifdef DUMP_FILE
static int id=1000;

#endif

extern int utl_cmd_insert(char *cmd, char *help_general, char *help_detail, int (*func)(int argc, char *argv[]));


static U8 *jv_mva_map(U32 phyAddr, U32 size)
{
    return (U8 *) HI_MPI_SYS_Mmap(phyAddr, size);
}
static int jv_mva_mmz_alloc(MVA_HANDLE *IveHandle, int index, int w, int h)
{
	IveHandle->memSize[index] = w*h;
#ifdef MVA_MMZ_CACHE
	int ret = HI_MPI_SYS_MmzAlloc_Cached(&IveHandle->imgAddr[index], &IveHandle->virtAddr[index], "MVA", HI_NULL, w*h);
#else
	int ret = HI_MPI_SYS_MmzAlloc(&IveHandle->imgAddr[index], &IveHandle->virtAddr[index], "MVA", HI_NULL, w*h);
#endif   
	if(ret==HI_SUCCESS)
		IveHandle->mapAddr[index] = jv_mva_map(IveHandle->imgAddr[index], IveHandle->memSize[index]);
	else
		printf("[ERROR ] jv_mva_mmz_alloc  :%d, ret %x\n", index, ret);
    return ret;
}


static MVA_HANDLE *jv_mva_mmz_init( int width, int height)
{
    MVA_PRT(" width %d, height %d\n",  width, height);
    int i;
    MVA_HANDLE *IveHandle = (MVA_HANDLE *)calloc(1, sizeof(MVA_HANDLE));
    if(!IveHandle)
        return NULL;

    IveHandle->imgWidth = width;
    IveHandle->imgHeight = height;

    for(i = 0; i < MAX_MMZ_ALLOC_NUMS; i++)
    {
        jv_mva_mmz_alloc(IveHandle, i, width, height);
    }



    return IveHandle;
}


static void jv_mva_mmz_deinit(MVA_HANDLE *IveHandle )
{
	int i;
	int ret;
	if(IveHandle==NULL)
	    return ;
	for(i = 0; i < MAX_MMZ_ALLOC_NUMS; i++)
	{
		 CHECK_RET(HI_MPI_SYS_Munmap((HI_VOID *)IveHandle->imgAddr[i], (HI_U32)IveHandle->memSize[i]));
		 CHECK_RET(HI_MPI_SYS_MmzFree((HI_U32) IveHandle->imgAddr[i], (HI_VOID *)IveHandle->virtAddr[i]));
	}


}



static int jv_mva_LowThreshCut(MVA_HANDLE *IveHandle, HI_U32 pY, HI_U32 pOut, int threshold, void * pvY, void * pvOut)
{
    HI_S32 ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
 
	IVE_THRESH_CTRL_S stThrCtrl = {IVE_THRESH_MODE_TO_MINVAL, 40, 255, 0, 0, 255};
	IVE_SRC_IMAGE_S srcImg;
	IVE_DST_IMAGE_S dstImg;
	stThrCtrl.u8LowThr	=threshold;
	srcImg.enType=IVE_IMAGE_TYPE_U8C1;
	srcImg.pu8VirAddr[0]=pvY;
	srcImg.u32PhyAddr[0]= (HI_U32) pY;
	srcImg.u16Width = IveHandle->imgWidth;
	srcImg.u16Height= IveHandle->imgHeight;
	srcImg.u16Stride[0]= IveHandle->imgWidth;

	dstImg.enType=IVE_IMAGE_TYPE_U8C1;
	dstImg.pu8VirAddr[0]=pvOut;
	dstImg.u32PhyAddr[0]=(HI_U32) pOut;
	dstImg.u16Width = IveHandle->imgWidth;
	dstImg.u16Height= IveHandle->imgHeight;
	dstImg.u16Stride[0]= IveHandle->imgWidth;

	
    ret = HI_MPI_IVE_Thresh(&h, &srcImg,  &dstImg, &stThrCtrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}

static int jv_mva_LowThresh(MVA_HANDLE *IveHandle, HI_U32 pY, HI_U32 pOut, int threshold, void * pvY, void * pvOut)
{
    HI_S32 ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
    IVE_SRC_IMAGE_S src;
    IVE_THRESH_CTRL_S thresh;
    IVE_DST_IMAGE_S dst;

    src.enType = IVE_IMAGE_TYPE_U8C1;
    src.u32PhyAddr[0] = (HI_U32) pY;
    src.u16Stride[0] = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr[0] = pvY;

    dst.enType = IVE_IMAGE_TYPE_U8C1;
    dst.u32PhyAddr[0] = (HI_U32) pOut;
    dst.u16Stride[0] = IveHandle->imgWidth;
    dst.u16Width = IveHandle->imgWidth;
    dst.u16Height = IveHandle->imgHeight;
    dst.pu8VirAddr[0] = pvOut;
    
    thresh.enMode = IVE_THRESH_MODE_BINARY;
    thresh.u8MaxVal = 0xff;
    thresh.u8MinVal = 0;
    thresh.u8HighThr = threshold; 
    thresh.u8LowThr = threshold;
    ret = HI_MPI_IVE_Thresh(&h, &src,  &dst, &thresh, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}


static int jv_mva_Sub(MVA_HANDLE *IveHandle,HI_U32 pIn1, HI_U32 pIn2, HI_U32 pOut, void * pvIn1, void * pvIn2, void * pvOut)
{
    HI_S32 ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
    IVE_SRC_IMAGE_S src, src2;
    IVE_DST_IMAGE_S dst;
    IVE_SUB_CTRL_S ctrl = {IVE_SUB_MODE_ABS};

    src.enType = IVE_IMAGE_TYPE_U8C1;
    src.u32PhyAddr[0] = (HI_U32) pIn1;
    src.u16Stride[0] = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr[0] = pvIn1;

    src2.enType = IVE_IMAGE_TYPE_U8C1;
    src2.u32PhyAddr[0] = (HI_U32) pIn2;
    src2.u16Stride[0] = IveHandle->imgWidth;
    src2.u16Width = IveHandle->imgWidth;
    src2.u16Height = IveHandle->imgHeight;
    src2.pu8VirAddr[0] = pvIn2;

    dst.enType = IVE_IMAGE_TYPE_U8C1;
    dst.u32PhyAddr[0] = (HI_U32) pOut;
    dst.u16Stride[0] = IveHandle->imgWidth;
    dst.u16Width = IveHandle->imgWidth;
    dst.u16Height = IveHandle->imgHeight;
    dst.pu8VirAddr[0] = pvOut;

    ret = HI_MPI_IVE_Sub(&h, &src, &src2, &dst, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT("ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}

static int jv_mva_LPF(MVA_HANDLE *IveHandle, HI_U32 pIn, HI_U32 pOut, void * pvIn, void *pvOut)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;

	
	IVE_SRC_IMAGE_S srcImg;
	IVE_DST_IMAGE_S dstImg;


IVE_FILTER_CTRL_S ctrl = {
				{
					0,0,1,0,0,
					0,1,1,1,0,
					1,1,4,1,1,
					0,1,1,1,0,
					0,0,1,0,0}, 4};

	srcImg.enType=IVE_IMAGE_TYPE_U8C1;
	srcImg.pu8VirAddr[0]=pvIn;
	srcImg.u32PhyAddr[0]= (HI_U32) pIn;
	srcImg.u16Width = IveHandle->imgWidth;
	srcImg.u16Height= IveHandle->imgHeight;
	srcImg.u16Stride[0]= IveHandle->imgWidth;
	
	dstImg.enType=IVE_IMAGE_TYPE_U8C1;
	dstImg.pu8VirAddr[0]=pvOut;
	dstImg.u32PhyAddr[0]= (HI_U32) pOut;
	dstImg.u16Width = IveHandle->imgWidth;
	dstImg.u16Height= IveHandle->imgHeight;
	dstImg.u16Stride[0]= IveHandle->imgWidth;
	

    ret = HI_MPI_IVE_Filter(&h, &srcImg, &dstImg, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    
    return HI_MPI_IVE_Query(h, &bFinished, 1);
}
static int jv_mva_LPF2(MVA_HANDLE *IveHandle, HI_U32 pIn, HI_U32 pOut, void * pvIn, void *pvOut)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;

	
	IVE_SRC_IMAGE_S srcImg;
	IVE_DST_IMAGE_S dstImg;


IVE_FILTER_CTRL_S ctrl = {
				{
					1,1,1,1,1,
					1,1,2,1,1,
					1,2,4,2,1,
					1,1,2,1,1,
					1,1,1,1,1}, 5};

	srcImg.enType=IVE_IMAGE_TYPE_U8C1;
	srcImg.pu8VirAddr[0]=pvIn;
	srcImg.u32PhyAddr[0]= (HI_U32) pIn;
	srcImg.u16Width = IveHandle->imgWidth;
	srcImg.u16Height= IveHandle->imgHeight;
	srcImg.u16Stride[0]= IveHandle->imgWidth;
	
	dstImg.enType=IVE_IMAGE_TYPE_U8C1;
	dstImg.pu8VirAddr[0]=pvOut;
	dstImg.u32PhyAddr[0]= (HI_U32) pOut;
	dstImg.u16Width = IveHandle->imgWidth;
	dstImg.u16Height= IveHandle->imgHeight;
	dstImg.u16Stride[0]= IveHandle->imgWidth;
	

    ret = HI_MPI_IVE_Filter(&h, &srcImg, &dstImg, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    
    return HI_MPI_IVE_Query(h, &bFinished, 1);
}
static int jv_mva_LPF3(MVA_HANDLE *IveHandle, HI_U32 pIn, HI_U32 pOut, void * pvIn, void *pvOut)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;

	
	IVE_SRC_IMAGE_S srcImg;
	IVE_DST_IMAGE_S dstImg;


IVE_FILTER_CTRL_S ctrl = {
				{
					0,0,0,0,0,
					0,1,2,1,0,
					0,2,4,2,0,
					0,1,2,1,0,
					0,0,0,0,0}, 4};

	srcImg.enType=IVE_IMAGE_TYPE_U8C1;
	srcImg.pu8VirAddr[0]=pvIn;
	srcImg.u32PhyAddr[0]= (HI_U32) pIn;
	srcImg.u16Width = IveHandle->imgWidth;
	srcImg.u16Height= IveHandle->imgHeight;
	srcImg.u16Stride[0]= IveHandle->imgWidth;
	
	dstImg.enType=IVE_IMAGE_TYPE_U8C1;
	dstImg.pu8VirAddr[0]=pvOut;
	dstImg.u32PhyAddr[0]= (HI_U32) pOut;
	dstImg.u16Width = IveHandle->imgWidth;
	dstImg.u16Height= IveHandle->imgHeight;
	dstImg.u16Stride[0]= IveHandle->imgWidth;
	

    ret = HI_MPI_IVE_Filter(&h, &srcImg, &dstImg, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    
    return HI_MPI_IVE_Query(h, &bFinished, 1);
}

static int jv_mva_MPF(MVA_HANDLE *IveHandle, HI_U32 pIn, HI_U32 pOut, void * pvIn, void *pvOut)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;

	
	IVE_SRC_IMAGE_S srcImg;
	IVE_DST_IMAGE_S dstImg;

	IVE_ORD_STAT_FILTER_CTRL_S ctrl ;

	ctrl.enMode=IVE_ORD_STAT_FILTER_MODE_MAX;
	//ctrl.enMode=IVE_ORD_STAT_FILTER_MODE_MEDIAN;
	srcImg.enType=IVE_IMAGE_TYPE_U8C1;
	srcImg.pu8VirAddr[0]=pvIn;
	srcImg.u32PhyAddr[0]= (HI_U32) pIn;
	srcImg.u16Width = IveHandle->imgWidth;
	srcImg.u16Height= IveHandle->imgHeight;
	srcImg.u16Stride[0]= IveHandle->imgWidth;
	
	dstImg.enType=IVE_IMAGE_TYPE_U8C1;
	dstImg.pu8VirAddr[0]=pvOut;
	dstImg.u32PhyAddr[0]= (HI_U32) pOut;
	dstImg.u16Width = IveHandle->imgWidth;
	dstImg.u16Height= IveHandle->imgHeight;
	dstImg.u16Stride[0]= IveHandle->imgWidth;
	

    ret = HI_MPI_IVE_OrdStatFilter(&h, &srcImg, &dstImg, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    
    return HI_MPI_IVE_Query(h, &bFinished, 1);
}
static int jv_mva_HPF(MVA_HANDLE *IveHandle, HI_U32  pIn, HI_U32 pOut, void * pvIn, void *pvOut,int mode)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;

	
	IVE_SRC_IMAGE_S srcImg;
	IVE_DST_IMAGE_S dstImg;
	IVE_FILTER_CTRL_S * ctrl ;

	const IVE_FILTER_CTRL_S ctrl1= {
									{
										0,0,0,0,0,
										0,0,-1,0,0,
										0,-1,4,-1,0,
										0,0,-1,0,0,
										0,0,0,0,0}, 0};
	const IVE_FILTER_CTRL_S ctrl2 = {
									{
										0,0,0,0,0,
										0,1,0,1,0,
										0,0,-4,0,0,
										0,1,0,1,0,
										0,0,0,0,0}, 0};
										
	const IVE_FILTER_CTRL_S ctrl3= {
									{
										0,0,-1,0,0,
										0,0,0,0,0,
										-1,0,4,0,-1,
										0,0,0,0,0,
										0,0,-1,0,0}, 0};
	srcImg.enType=IVE_IMAGE_TYPE_U8C1;
	srcImg.pu8VirAddr[0]=pvIn;
	srcImg.u32PhyAddr[0]= (HI_U32) pIn;
	srcImg.u16Width = IveHandle->imgWidth;
	srcImg.u16Height= IveHandle->imgHeight;
	srcImg.u16Stride[0]= IveHandle->imgWidth;
	
	dstImg.enType=IVE_IMAGE_TYPE_U8C1;
	dstImg.pu8VirAddr[0]=pvOut;
	dstImg.u32PhyAddr[0]= (HI_U32) pOut;
	dstImg.u16Width = IveHandle->imgWidth;
	dstImg.u16Height= IveHandle->imgHeight;
	dstImg.u16Stride[0]= IveHandle->imgWidth;
	if(mode == 2)
	{
		ctrl = (IVE_FILTER_CTRL_S * )&ctrl3;
	}
	else	if(mode == 1)
	{
		ctrl = (IVE_FILTER_CTRL_S * )&ctrl2;
	}
		
	else
	{
		ctrl = (IVE_FILTER_CTRL_S * )&ctrl1;
	}

       ret = HI_MPI_IVE_Filter(&h, &srcImg, &dstImg, ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}
static int jv_mva_And(MVA_HANDLE *IveHandle,HI_U32 pIn1, HI_U32 pIn2,HI_U32 pOut, void * pvIn1, void * pvIn2, void * pvOut)
{
    HI_S32 ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
    IVE_SRC_IMAGE_S src, src2;
    IVE_DST_IMAGE_S dst;
    
    src.enType = IVE_IMAGE_TYPE_U8C1;
    src.u32PhyAddr[0] = (HI_U32) pIn1;
    src.u16Stride[0] = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr[0] = pvIn1;

    src2.enType = IVE_IMAGE_TYPE_U8C1;
    src2.u32PhyAddr[0] = (HI_U32) pIn2;
    src2.u16Stride[0] = IveHandle->imgWidth;
    src2.u16Width = IveHandle->imgWidth;
    src2.u16Height = IveHandle->imgHeight;
    src2.pu8VirAddr[0] = pvIn2;

    dst.enType = IVE_IMAGE_TYPE_U8C1;
    dst.u32PhyAddr[0] = (HI_U32) pOut;
    dst.u16Stride[0] = IveHandle->imgWidth;
    dst.u16Width = IveHandle->imgWidth;
    dst.u16Height = IveHandle->imgHeight;
    dst.pu8VirAddr[0] = pvOut;

    ret = HI_MPI_IVE_And(&h, &src, &src2, &dst, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}
static int jv_mva_Or(MVA_HANDLE *IveHandle,HI_U32 pIn1, HI_U32 pIn2,HI_U32 pOut, void * pvIn1, void * pvIn2, void * pvOut)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;
	IVE_SRC_IMAGE_S src, src2;
	IVE_DST_IMAGE_S dst;
	IVE_ADD_CTRL_S pstAddCtrl;
	src.enType = IVE_IMAGE_TYPE_U8C1;
	src.u32PhyAddr[0] = (HI_U32) pIn1;
	src.u16Stride[0] = IveHandle->imgWidth;
	src.u16Width = IveHandle->imgWidth;
	src.u16Height = IveHandle->imgHeight;
	src.pu8VirAddr[0] = pvIn1;

	src2.enType = IVE_IMAGE_TYPE_U8C1;
	src2.u32PhyAddr[0] = (HI_U32) pIn2;
	src2.u16Stride[0] = IveHandle->imgWidth;
	src2.u16Width = IveHandle->imgWidth;
	src2.u16Height = IveHandle->imgHeight;
	src2.pu8VirAddr[0] = pvIn2;

	dst.enType = IVE_IMAGE_TYPE_U8C1;
	dst.u32PhyAddr[0] = (HI_U32) pOut;
	dst.u16Stride[0] = IveHandle->imgWidth;
	dst.u16Width = IveHandle->imgWidth;
	dst.u16Height = IveHandle->imgHeight;
	dst.pu8VirAddr[0] = pvOut;
	pstAddCtrl.u0q16X=1;
	pstAddCtrl.u0q16Y=1;
	ret = HI_MPI_IVE_Or(&h, &src, &src2, &dst, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}
static int jv_mva_Erode(MVA_HANDLE *IveHandle, HI_U32  pIn, HI_U32 pOut, void * pvIn, void *pvOut)
{
    int ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_ERODE_CTRL_S ctrl = 
    {
    	{
        0, 0, 0, 0, 0,	
        0, 0xff, 0xff, 0xff, 0, 
        0, 0xff, 0xff, 0xff, 0, 
        0, 0xff, 0xff, 0xff, 0,
        0, 0, 0, 0, 0}
    };

    src.enType = IVE_IMAGE_TYPE_U8C1;
    src.u32PhyAddr[0] = pIn;
    src.u16Stride[0] = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr[0] = pvIn;

    dst.enType = IVE_IMAGE_TYPE_U8C1;
    dst.u32PhyAddr[0] = pOut;
    dst.u16Stride[0] = src.u16Stride[0];
    dst.u16Width = src.u16Width;
    dst.u16Height = src.u16Height;
    dst.pu8VirAddr[0] = pvOut;
    
    ret = HI_MPI_IVE_Erode(&h, &src, &dst, &ctrl, 0);    
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}

static int jv_mva_Dilate(MVA_HANDLE *IveHandle, HI_U32 pIn, HI_U32 pOut, void * pvIn, void *pvOut, int mode)
{
	HI_S32 ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;
	IVE_SRC_IMAGE_S src;
	IVE_DST_IMAGE_S dst;
	IVE_DILATE_CTRL_S ctrl;
  HI_U8 mask[25] = 
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff,
    };
  HI_U8 mask2[25] = 
    {
        0, 0, 0, 0, 0,
        0, 0xff, 0xff, 0xff, 0,
        0, 0xff, 0xff, 0xff, 0,
        0, 0xff, 0xff, 0xff, 0,
        0, 0, 0, 0, 0,
    };

    switch(mode)
    {
        defalut:
            memset(ctrl.au8Mask, 0xff, 25);
            break;
        case 1:
            memset(ctrl.au8Mask, 0, 25);
            memset(ctrl.au8Mask+10, 0xff, 5);
	case 2:
            memset(ctrl.au8Mask, 0, 25);
            memcpy(ctrl.au8Mask, mask, 25);
            break;
	case 3:
            memset(ctrl.au8Mask, 0, 25);
            memcpy(ctrl.au8Mask, mask,25);
            break;
    }

    src.enType = IVE_IMAGE_TYPE_U8C1;
    src.u32PhyAddr[0] =(HI_U32)  pIn;
    src.u16Stride[0] = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr[0] = pvIn;

    dst.enType = IVE_IMAGE_TYPE_U8C1;
    dst.u32PhyAddr[0] =(HI_U32)  pOut;
    dst.u16Stride[0] = src.u16Stride[0];
    dst.u16Width = src.u16Width;
    dst.u16Height = src.u16Height;
    dst.pu8VirAddr[0] = pvOut;
    
    ret = HI_MPI_IVE_Dilate(&h, &src, &dst, &ctrl, 0);    
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}

static int jv_mva_CCL(MVA_HANDLE *IveHandle, HI_U32 pIn,HI_U32 pTmp, void * pvIn, void * pvTmp, int area)
{
    HI_S32 i, ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
    IVE_IMAGE_S src;
    IVE_DST_MEM_INFO_S dst;
    IVE_CCL_CTRL_S ctrl;
    IVE_CCBLOB_S *blocks;

    src.enType = IVE_IMAGE_TYPE_U8C1;
    src.u32PhyAddr[0] =(HI_U32)  pIn;
    src.u16Stride[0] = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr[0] = pvIn;

    dst.u32PhyAddr = (HI_U32) pTmp;
    dst.pu8VirAddr = pvTmp;
    dst.u32Size = sizeof(IVE_CCBLOB_S);
    ctrl.enMode = IVE_CCL_MODE_8C;
    ctrl.u16InitAreaThr = area;
    ctrl.u16Step = 1;
    
    ret = HI_MPI_IVE_CCL(&h, &src, &dst, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

     HI_MPI_IVE_Query(h, &bFinished, 1);
/*
     blocks = (IVE_CCBLOB_S *)pvTmp;
     for(i = 0; i < IVE_MAX_REGION_NUM; i++)
     {
         if(blocks->astRegion[i].u32Area)
         {
             printf(("ccl%d: %d,[%d,%d,%d,%d]\n", i, blocks->astRegion[i].u32Area, blocks->astRegion[i].u16Left, blocks->astRegion[i].u16Top, blocks->astRegion[i].u16Right, blocks->astRegion[i].u16Bottom));
            // IveHandleAreaAdd(list, blocks->astRegion[i].u16Left, blocks->astRegion[i].u16Top, blocks->astRegion[i].u16Right, blocks->astRegion[i].u16Bottom);
         }
     }
*/
     return 0;
}

static int jv_mva_ImgCopy(MVA_HANDLE *IveHandle, HI_U32 pY, HI_U32 pOut, void *pvY, void *pvOut)
{
    HI_S32  ret;
    IVE_HANDLE h;
    HI_BOOL bFinished;
    IVE_DATA_S src;
    IVE_DST_DATA_S dst;
    IVE_DMA_CTRL_S ctrl = {IVE_DMA_MODE_DIRECT_COPY, 0, 0, 0};

    src.u32PhyAddr =(HI_U32) pY;
    src.u16Stride = IveHandle->imgWidth;
    src.u16Width = IveHandle->imgWidth;
    src.u16Height = IveHandle->imgHeight;
    src.pu8VirAddr = pvY;

    dst.u32PhyAddr =(HI_U32)  pOut;
    dst.u16Stride = IveHandle->imgWidth;
    dst.u16Width = IveHandle->imgWidth;
    dst.u16Height = IveHandle->imgHeight;
    dst.pu8VirAddr = pvOut;

    ret = HI_MPI_IVE_DMA(&h, &src, &dst, &ctrl, HI_FALSE);
	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}

static int jv_mva_Flush(U32 phyAddr, void *virtAddr, U32 size)
{
    return HI_MPI_SYS_MmzFlushCache(phyAddr, virtAddr, size);
}
static int jv_mva_EqualizeHist(MVA_HANDLE *IveHandle, HI_U32  pIn, HI_U32 pOut, void * pvIn, void *pvOut,HI_U32  pSctrl,void *pvSctrl)
{
	int ret;
	IVE_HANDLE h;
	HI_BOOL bFinished;
	IVE_SRC_IMAGE_S src;
	IVE_DST_IMAGE_S dst;
	IVE_EQUALIZE_HIST_CTRL_S stCtrl;

	src.enType = IVE_IMAGE_TYPE_U8C1;
	src.u32PhyAddr[0] = pIn;
	src.u16Stride[0] = IveHandle->imgWidth;
	src.u16Width = IveHandle->imgWidth;
	src.u16Height = IveHandle->imgHeight;
	src.pu8VirAddr[0] = pvIn;

	dst.enType = IVE_IMAGE_TYPE_U8C1;
	dst.u32PhyAddr[0] = pOut;
	dst.u16Stride[0] = src.u16Stride[0];
	dst.u16Width = src.u16Width;
	dst.u16Height = src.u16Height;
	dst.pu8VirAddr[0] = pvOut;
	
	stCtrl.stMem.u32Size = sizeof(IVE_EQUALIZE_HIST_CTRL_MEM_S);
	stCtrl.stMem.pu8VirAddr = (HI_U8 *) pSctrl;
	stCtrl.stMem.u32PhyAddr = (HI_U32 )pvSctrl;
		
	ret = HI_MPI_IVE_EqualizeHist(&h,&src,&dst,&stCtrl,HI_FALSE);

	if(ret!= HI_SUCCESS)
	{
	    MVA_PRT(" ret %x \n", ret);
	    return ret;
	}

    return HI_MPI_IVE_Query(h, &bFinished, 1);
}


#if DUMP_FILE
static int  save_result( U8 *p,char *name)
{
	static int cnt = 0;
	FILE *fOut;
	char fname[256] = {0};
	if(p==NULL)
		return 0;
/*	if(cnt > 0)
	{
		return 0;;
	}
	else
	{
		cnt++;
	}*/
	sprintf(fname, "/root/%s.y", name);
	fOut = fopen(fname, "wb+");
	if(fOut == NULL)
	{
		return -1;
	}
	int ret = fwrite(p, ATK_VI_W*ATK_VI_H, 1, fOut);

	fclose(fOut);
	return ret;
}
#endif





//根据连通域计算运动坐标
static int jv_mva_CCLResultProc( IVE_CCBLOB_S * stCclData,ObjStaticsInfo_t *stObj, int * objNum)
{
//	 struct  timeval start;
//	struct  timeval end;
//	unsigned  int diff;
	int i,j,k,index,index2;
	ObjStaticsInfo_t ObjStatics[IVE_MAX_REGION_NUM];
//	ObjStaticsInfo_t ObjStaticsResult[IVE_MAX_REGION_NUM];

	*objNum=0;

	if(stCclData==NULL)
		return -1;


	if(stCclData->s8LabelStatus!=0)
	{
		MVA_PRT("CCL failed\n");

		return -1;
	}
	if(stCclData->u8RegionNum<1)
	{
//		MVA_PRT("CCL u8RegionNum=0\n");
		return -1;
	}
	index=0;
	 for(j = 0; j < IVE_MAX_REGION_NUM; j++)
	 {
	     if(stCclData->astRegion[j].u32Area >0)
	     {

		#if 0
			ObjStatics[index].w=(stCclData->astRegion[j].u16Right-stCclData->astRegion[j].u16Left+1)*160/352;
			ObjStatics[index].h=(stCclData->astRegion[j].u16Bottom-stCclData->astRegion[j].u16Top+1)*120/288;
			ObjStatics[index].left =stCclData->astRegion[j].u16Left*160/352;
			ObjStatics[index].right =stCclData->astRegion[j].u16Right*160/352;
			ObjStatics[index].top=stCclData->astRegion[j].u16Top*120/288;
			ObjStatics[index].bottom=stCclData->astRegion[j].u16Bottom*120/288;
		#else
			ObjStatics[index].w=(stCclData->astRegion[j].u16Right-stCclData->astRegion[j].u16Left+1);
			ObjStatics[index].h=(stCclData->astRegion[j].u16Bottom-stCclData->astRegion[j].u16Top+1);
			ObjStatics[index].left =stCclData->astRegion[j].u16Left;
			ObjStatics[index].right =stCclData->astRegion[j].u16Right;
			ObjStatics[index].top=stCclData->astRegion[j].u16Top;
			ObjStatics[index].bottom=stCclData->astRegion[j].u16Bottom;		
		#endif

			ObjStatics[index].area=stCclData->astRegion[j].u32Area;
			if((ObjStatics[index].w)>0&& (ObjStatics[index].h>0))
				index++;
//		printf("ccl%d: %d,[%d,%d,%d,%d]\n", j, stCclData->astRegion[j].u32Area, stCclData->astRegion[j].u16Left, stCclData->astRegion[j].u16Top, stCclData->astRegion[j].u16Right, stCclData->astRegion[j].u16Bottom);
		}
	 }
//	printf("index=%d   ",index);
	if(index>0)
	{


		 for(i = 0; i < index; i++)
		 {
		 	for(j=0;j<index;j++)
		 	{
				if((ObjStatics[j].left==ObjStatics[i].left) && (ObjStatics[j].right==ObjStatics[i].right) && (ObjStatics[j].bottom==ObjStatics[i].bottom)&&(ObjStatics[j].top==ObjStatics[i].top))
				{
					continue;	//防止死循环
				}
				else if(__bNearObjs(&ObjStatics[j],&ObjStatics[i],OBJ_MERGE_LIMIT))
					
				{
					ccl_merge(&ObjStatics[j],&ObjStatics[i]);
					i=0;
					j=0;

				}
		 	}

		 }
	
		//防止出现重复的	
		for(i=0;i<index;i++)
	 	{
				
			for(j=i+1;j<index;j++)
			{
				
				if((ObjStatics[i].left==ObjStatics[j].left ) && ( ObjStatics[i].right==ObjStatics[j].right) && (ObjStatics[i].top==ObjStatics[j].top)  &&  (ObjStatics[i].bottom==ObjStatics[j].bottom) )
				{
					for(k=j+1;k<index;k++)
					{

						ObjStatics[k-1].left = ObjStatics[k].left;
						ObjStatics[k-1].right = ObjStatics[k].right;
						ObjStatics[k-1].top = ObjStatics[k].top;
						ObjStatics[k-1].bottom = ObjStatics[k].bottom;		

					}
					--index;
					--j;
				}



			}


		}
//		for(i=0;i<index;i++)
//		{
//			printf("i=%d pos [ %d %d %d %d] \n",i,ObjStatics[i].left,ObjStatics[i].top,ObjStatics[i].right,ObjStatics[i].bottom);
//		}
		index2 =0 ;
		for(i=0;i<index;i++)
		{
			stObj[index2].left = ObjStatics[i].left;
			stObj[index2].right = ObjStatics[i].right;
			stObj[index2].top = ObjStatics[i].top;
			stObj[index2].bottom = ObjStatics[i].bottom;
			stObj[index2].w = ObjStatics[i].right - ObjStatics[i].left + 1;
			stObj[index2].h = ObjStatics[i].bottom -ObjStatics[i].top + 1 ;
			if((ObjStatics[i].w > MIN_OBJ_SIZE) || (ObjStatics[i].h >MIN_OBJ_SIZE))
				if((ObjStatics[i].w < (ATK_VI_W-10)) ||  (ObjStatics[i].h < (ATK_VI_H-10)))
					index2++;
		}
		 *objNum = index2;

//		for(i=0;i<index2;i++)
//		{
//			printf("i=%d pos [ %d %d %d %d] \n",i,stObj[i].left,stObj[i].top,stObj[i].right,stObj[i].bottom);
//		}


	}

	
	return 0;
}


void jv_mva_dump( BOOL bEnOnes)
{
	if(bEnOnes)
	{
		id=0;
	}
}

void jv_mva_enable( BOOL bEn)
{
	mva_info.bIveProcRuning=bEn;

}


//3帧做差
#if 1
static int jv_mva_PatternProc(MVA_HANDLE *IveHandle,int index_current,int index_last1,int index_last2,IVE_CCBLOB_S * stCclData)
{
	U8 *p;
	
#if DUMP_FILE
	if(id==0)
	{
		id++;

	}
#endif	
	 jv_mva_LPF(IveHandle, IveHandle->imgAddr[index_current], IveHandle->imgAddr[3], IveHandle->virtAddr[index_current], IveHandle->virtAddr[3]);
	 jv_mva_LPF(IveHandle, IveHandle->imgAddr[index_last1], IveHandle->imgAddr[4], IveHandle->virtAddr[index_last1], IveHandle->virtAddr[4]);
	 jv_mva_LPF(IveHandle, IveHandle->imgAddr[index_last2], IveHandle->imgAddr[5], IveHandle->virtAddr[index_last2], IveHandle->virtAddr[5]);
#if DUMP_FILE
	if(id<2)
	{
		p = IveHandle->mapAddr[3];
		save_result(p,"1_LPF1");
		id++;
	}
	if(id<3)
	{
		p = IveHandle->mapAddr[4];
		save_result(p,"2_LPF2");
		id++;
	}
	if(id<4)
	{
		p = IveHandle->mapAddr[5];
		save_result(p,"3_LPF3");
		id++;
	}
#endif
	jv_mva_Sub(IveHandle, IveHandle->imgAddr[4], IveHandle->imgAddr[3], IveHandle->imgAddr[6], IveHandle->virtAddr[4], IveHandle->virtAddr[3],IveHandle->virtAddr[6]);
	jv_mva_Sub(IveHandle, IveHandle->imgAddr[4], IveHandle->imgAddr[5], IveHandle->imgAddr[7], IveHandle->virtAddr[4], IveHandle->virtAddr[5],IveHandle->virtAddr[7]);
#if DUMP_FILE
	if(id<5)
	{
		p = IveHandle->mapAddr[6];
		save_result(p,"4_SUB1");
		id++;
	}
	if(id<6)
	{
		p = IveHandle->mapAddr[7];
		save_result(p,"5_SUB2");
		id++;
	}
#endif
	
	jv_mva_LowThreshCut(IveHandle, IveHandle->imgAddr[6], IveHandle->imgAddr[3], 7, IveHandle->virtAddr[6], IveHandle->virtAddr[4]);
	jv_mva_LowThreshCut(IveHandle, IveHandle->imgAddr[7], IveHandle->imgAddr[4], 7, IveHandle->virtAddr[7], IveHandle->virtAddr[4]);
#if DUMP_FILE
	if(id<7)
	{
		p = IveHandle->mapAddr[3];
		save_result(p,"6_thresh1");
		id++;
	}
	if(id<8)
	{
		p = IveHandle->mapAddr[4];
		save_result(p,"7_thresh2");
		id++;
	}
#endif
	
	jv_mva_Dilate(IveHandle, IveHandle->imgAddr[3], IveHandle->imgAddr[5], IveHandle->virtAddr[3], IveHandle->virtAddr[5], 2);
	jv_mva_Dilate(IveHandle, IveHandle->imgAddr[4], IveHandle->imgAddr[6], IveHandle->virtAddr[4], IveHandle->virtAddr[6], 2);
#if DUMP_FILE
	if(id<9)
	{
		p = IveHandle->mapAddr[5];
		save_result(p,"8_dilate1");
		id++;
	}
	if(id<10)
	{
		p = IveHandle->mapAddr[6];
		save_result(p,"9_dilate2");
		id++;
	}
#endif
	jv_mva_And(IveHandle, IveHandle->imgAddr[5], IveHandle->imgAddr[6], IveHandle->imgAddr[4], IveHandle->virtAddr[5], IveHandle->virtAddr[6], IveHandle->virtAddr[4]);
#if DUMP_FILE
	if(id<11)
	{
		p = IveHandle->mapAddr[4];
		save_result(p,"10_and1");
		id++;
	}
#endif	
	jv_mva_LowThresh(IveHandle, IveHandle->imgAddr[4], IveHandle->imgAddr[5],8, IveHandle->virtAddr[4], IveHandle->virtAddr[5]);

//	jv_mva_LowThresh(IveHandle, IveHandle->imgAddr[4], IveHandle->imgAddr[5],20, IveHandle->virtAddr[4], IveHandle->virtAddr[5]);
#if DUMP_FILE
	if(id<12)
	{
		p = IveHandle->mapAddr[5];
		save_result(p,"11_2bin");
		id++;
	}
#endif
	jv_mva_CCL(IveHandle, IveHandle->imgAddr[5], IveHandle->imgAddr[6], IveHandle->virtAddr[5], IveHandle->virtAddr[6], 4);
	memcpy(stCclData,(IVE_CCBLOB_S  *)IveHandle->mapAddr[6],sizeof(IVE_CCBLOB_S));
	return 0;

}


#endif
static int jv_mva_HidePatternProc(MVA_HANDLE *IveHandle,int index_current,ObjHideStatics_t * stHide)
{
	int i,j,k,z;
//	U64 sum;
	U8 *p;
	static int  count=0;
	count++;
	if(count>4)
	{
		count =0;
	}
	else
	{
		return -1;
	}
	jv_mva_LPF3(IveHandle, IveHandle->imgAddr[index_current], IveHandle->imgAddr[3], IveHandle->virtAddr[index_current], IveHandle->virtAddr[3]);

//	jv_mva_LowThreshCut(IveHandle, IveHandle->imgAddr[index_current], IveHandle->imgAddr[3], 16, IveHandle->virtAddr[index_current], IveHandle->virtAddr[3]);
	jv_mva_HPF(IveHandle, IveHandle->imgAddr[3], IveHandle->imgAddr[4], IveHandle->virtAddr[3], IveHandle->virtAddr[4],0);
	jv_mva_HPF(IveHandle, IveHandle->imgAddr[4], IveHandle->imgAddr[5], IveHandle->virtAddr[4], IveHandle->virtAddr[5],1);
	jv_mva_HPF(IveHandle, IveHandle->imgAddr[5], IveHandle->imgAddr[6], IveHandle->virtAddr[5], IveHandle->virtAddr[6],2);

	jv_mva_LowThreshCut(IveHandle, IveHandle->imgAddr[6], IveHandle->imgAddr[3], 8, IveHandle->virtAddr[6], IveHandle->virtAddr[3]);
//	jv_mva_HPF(IveHandle, IveHandle->imgAddr[index_current], IveHandle->imgAddr[4], IveHandle->virtAddr[index_current], IveHandle->virtAddr[4]);
	p = IveHandle->mapAddr[3];
	memset(stHide,0,sizeof(ObjHideStatics_t));
	for(i=0;i<IveHandle->imgHeight;i++)
	{
		for(j=0;j<IveHandle->imgWidth;j++)
		{
			k=i*MAXWIN_H/IveHandle->imgHeight;
			z=j*MAXWIN_W/IveHandle->imgWidth;
			stHide->statics[k][z] += p[IveHandle->imgWidth*i+j]; 
		}
	
	}
	return 0;

}
static int jv_mva_ImageProc(MVA_HANDLE *IveHandle,VIDEO_FRAME_INFO_S * stFrame)
{
	int ret, i, j,size, n;
	U8 *p;
	int s32ret;
	HI_U32 *pYUV; 
	HI_U32 *pVirAddr;
	HI_U32 stride;
	int alarmed = 0; 
	ISP_VD_INFO_S vdinfo;
	IVE_CCBLOB_S stCclData;
	static BOOL motstattmp=0;
	static BOOL bFilled=0;
	int index_current,index_last1,index_last2;
	ObjStaticsInfo_t ObjStatics[IVE_MAX_REGION_NUM];
	int ObjNum;
	if( mva_info.bIveProcRuning > 0)
	{
		if(IveHandle==NULL)
			return -1;
		jv_mva_ImgCopy(IveHandle, stFrame->stVFrame.u32PhyAddr[0], IveHandle->imgAddr[IveHandle->index], stFrame->stVFrame.pVirAddr[0], IveHandle->virtAddr[IveHandle->index]);
		index_current = IveHandle->index;
		IveHandle->index++;
		if(IveHandle->index==3)
			IveHandle->index=0;
		IveHandle->frames++;
		if(IveHandle->frames > 10)
		{
			bFilled=1;
		}
		if(!bFilled)
			return -1;
		if(index_current==0)
		{
			index_last1=2;
			index_last2=1;
		}
		else if(index_current==1)
		{
			index_last1=0;
			index_last2=2;
		}
		else if(index_current==2)
		{
			index_last1=1;
			index_last2=0;
			
		}
		//形态学处理，取得连通域
		jv_mva_PatternProc(IveHandle,index_current,index_last1,index_last2,&stCclData);
		//连通域第一次合并等
		s32ret=jv_mva_CCLResultProc( &stCclData, &ObjStatics[0],&ObjNum);
		//回调进行越界侦测计算
		if(f_ccl_analysis)
			f_ccl_analysis(&ObjStatics[0],&ObjNum);
		//遮挡
		ObjHideStatics_t stHide;
		ret = jv_mva_HidePatternProc(IveHandle,index_current,&stHide);
		//开始统计值的二次分析
		if(ret == 0)
		{
			if(f_mva_hide_analysis)
				f_mva_hide_analysis(&stHide);
		}
		
	}
	
	
	return 0;
}


static int __debug_func(int argc, char *argv[])
{
	int channelid;
	printf("Here video analysis  setting\n");
	if (argc < 2)
	{
		printf("too little argc: %d\n",argc);
		return -1;
	}

	if (!strcmp(argv[1], "dump"))
	{
		printf("autotrack dump\n");
		jv_mva_dump(1);
	}
	else if (!strcmp(argv[1], "en"))
	{
		jv_mva_enable(1);

	}
	else if (!strcmp(argv[1], "den"))
	{
		jv_mva_enable(0);

	}
	else if (!strcmp(argv[1], "thresh"))
	{
		if (argc < 3)
		{
			printf("too little argc: %d\n",argc);
			return -1;
		}

	}
	
	
	return 0;
}






/**
 *@brief do initialize 
 *@return 0 if success.
 *
 */
 int jv_mva_init(void)
{
	memset(&mva_info, 0, sizeof(mva_info));
	pthread_mutex_init(&mva_info.group.mutex, NULL);
	
	utl_cmd_insert("va", "for line count",  "exp: va  dump\n",  __debug_func);

	return 0;
}
/**
 *@brief do de-initialize 
 *@return 0 if success.
 *
 */
int jv_mva_deinit(void)
{
	jv_mva_stop(0);
	pthread_mutex_destroy(&mva_info.group.mutex);

	return 0;
}


/**
 *@brief start motion track
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_mva_start(int channelid)
{
	MVA_PRT("Start xw mva module\n");
	pthread_mutex_lock(&mva_info.group.mutex);
	if(IveHandle==NULL)
	{
		IveHandle = jv_mva_mmz_init( ATK_VI_W, ATK_VI_H);
	}
	if(IveHandle==NULL)
	{
		printf("%s ERROR MMZ Init NULL Point\n",__func__);
	}
	jv_mva_enable(1);
	pthread_mutex_unlock(&mva_info.group.mutex);

	return 0;
}
/**
 *@brief stop 
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_mva_stop(int channelid)
{
	MVA_PRT("Stop xw mva module\n");

	pthread_mutex_lock(&mva_info.group.mutex);
//TODO free	
	jv_mva_enable(0);
	if(IveHandle)
	{
		jv_mva_mmz_deinit(IveHandle);
		free(IveHandle);
		IveHandle=NULL;
	}
	pthread_mutex_unlock(&mva_info.group.mutex);
	
	return 0;
}



int jv_mva_analysis( VIDEO_FRAME_INFO_S * srcImg)
{
	int ret=0;
	pthread_mutex_lock(&mva_info.group.mutex);
	if(IveHandle>0)
	{
		if( mva_info.bIveProcRuning > 0)
		{
			if(f_get_suspends !=NULL)
			{
				if( f_get_suspends()==TRUE)
				{
					pthread_mutex_unlock(&mva_info.group.mutex);
					return ret;
				}
			}
			ret=jv_mva_ImageProc(IveHandle, srcImg);
		}
	}
	pthread_mutex_unlock(&mva_info.group.mutex);
	return ret;

}
int  jv_mva_register_ccl_analysis_callback(jv_mva_ccl_analysis callback)
{


	f_ccl_analysis = callback;
	return 0;
}
int  jv_mva_register_getsuspends_callback(jv_mva_getsuspends callback)
{
	f_get_suspends = callback;
	return 0;
}

int  jv_mva_register_hide_analysis_callback(jv_mva_hide_analysis callback)
{

	f_mva_hide_analysis = callback;
	return 0;
}

