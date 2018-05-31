#include "hicommon.h"
#include "jv_common.h"
#include "jv_mdetect.h"
#include "jv_stream.h"
#include <mpi_vda.h>

#define MD_VI_W	720
#define MD_VI_H	576

#define MB_WIDTH	(MD_VI_W/16)
#define MB_HEIGHT	(MD_VI_H/16)

#define MD_VPSS_CHN	5

typedef struct
{
	jv_mdetect_attr_t attrlist[MAX_STREAM];
	//nxp_motion_detect_attr_t md_attr[MAX_STREAM];

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
#include <sys/un.h>

static void _jv_mdetect_process(void *param)
{
	S32	nRet, i, j, nResult;
	U8* pTmp = NULL;
	VDA_DATA_S stMDData;
	struct pollfd stPollMD;
	int nThreshold;

	//移动检测状态
	BOOL bMDStatus[]= {0, 0, 0, 0};

	while(group.running)
	{
		//如果移动检测未初始化或者灵敏度是0都不进行检测,lck20120807
		//灵敏度为0时，依然去检测，目的是把VDA的数据读出来。否则会导致VPSS阻塞，VI帧率降低
		if (mdinfo.fd == -1)// || 0 >= mdinfo.attrlist[0].nSensitivity)
		{
			usleep(100*1000);
			continue;
		}

		//阈值范围:5~39
		nThreshold = (100-mdinfo.attrlist[0].nSensitivity)/5+5;
		//Printf("nThreshold=%d\n", nThreshold);

		pthread_mutex_lock(&group.mutex);
		stPollMD.events = POLLIN;
		stPollMD.fd = mdinfo.fd;
		if (mdinfo.fd == -1)
		{
			pthread_mutex_unlock(&group.mutex);
			continue;
		}
		pthread_mutex_unlock(&group.mutex);
		//检查是否有移动检测结果
		if( poll(&stPollMD, MAX_MDCHN_NUM, 120) > 0)
		{
			nResult	= 0;
			//如果有视频信号并且有移动检测数据
			if (stPollMD.revents)
			{
				if((nRet=HI_MPI_VDA_GetData(mdinfo.nVenc, &stMDData, HI_IO_NOBLOCK)) != 0)
				{
					Printf("HI_MPI_MD_GetData err 0x%x\n", nRet);
					continue;
				}

				//Printf("stMDData.u16MBHeight: %d, %d\n", stMDData.u32MbWidth, stMDData.u32MbHeight);
				//处理移动检测数据
				for(i=0; i<stMDData.u32MbHeight; i++)
				{
					pTmp = (U8 *)((U32)stMDData.unData.stMdData.stMbSadData.pAddr + i*stMDData.unData.stMdData.stMbSadData.u32Stride);

					for(j=0; j<stMDData.u32MbWidth; j++)
					{
						if (mdinfo.acMB[i][j] && *pTmp > nThreshold)
						{
							nResult++;
						}
						//printf("%2x",*pTmp);
						pTmp++;
					}
					//printf("\n");
				}

				//连续发生两次移动检测才判断为时移动检测事件
				bMDStatus[1] = bMDStatus[0];
				bMDStatus[0] = (nResult > mdinfo.nMBNum);
				if(bMDStatus[0] && bMDStatus[1])	//开始报警
				{
					if (mdinfo.callback_ptr)
					{
						mdinfo.callback_ptr(0, mdinfo.callback_param);
					}
				}

				HI_MPI_VDA_ReleaseData(mdinfo.nVenc, &stMDData);
			}
		}
		usleep(1);
	}
}

//static void __ex_vichn()
//{
//	S32 s32Ret;
//	VI_CHN ViExtChn = 1;
//	VI_EXT_CHN_ATTR_S stViExtChnAttr;
//
//	stViExtChnAttr.s32BindChn           = 0;
//	stViExtChnAttr.stDestSize.u32Width  = MD_VI_W;
//	stViExtChnAttr.stDestSize.u32Height = MD_VI_H;
//	stViExtChnAttr.s32SrcFrameRate      = -1;
//	stViExtChnAttr.s32FrameRate         = -1;
//	stViExtChnAttr.enPixFormat          = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
//
//	s32Ret = HI_MPI_VI_SetExtChnAttr(ViExtChn, &stViExtChnAttr);
//	if (HI_SUCCESS != s32Ret)
//	{
//		SAMPLE_PRT("set vi  extchn failed!\n");
//		return ;
//	}
//
//	s32Ret = HI_MPI_VI_EnableChn(ViExtChn);
//	if (HI_SUCCESS != s32Ret)
//	{
//		SAMPLE_PRT("set vi  extchn failed!\n");
//		return ;
//	}
//
//}

/**
 *@brief do initialize of motion detection
 *@return 0 if success.
 *
 */
//porting中hi3507的第二码流，压缩通道ID,
//在porting中的MD模块需要此参数,lck20120806
#define VENC_SECOND		0
int jv_mdetect_init(void)
{
	memset(&mdinfo, 0, sizeof(mdinfo));

	mdinfo.fd		= -1;
	mdinfo.nVenc	= VENC_SECOND;

	group.running = TRUE;
	//pthread_mutex_init(&group.mutex, NULL);
	pthread_mutex_init(&group.mutex, NULL);
	pthread_create(&group.thread, NULL, (void *)_jv_mdetect_process, NULL);
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
	if (mdinfo.fd > 0)
		close(mdinfo.fd);
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
	VDA_CHN VdaChn;
	VDA_CHN_ATTR_S stVdaChnAttr;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;
	pthread_mutex_lock(&group.mutex);

	mdinfo.callback_ptr = callback;
	mdinfo.callback_param = param;

	VdaChn = mdinfo.nVenc;

	stVdaChnAttr.enWorkMode = VDA_WORK_MODE_MD;
	stVdaChnAttr.u32Width	= MD_VI_W;
	stVdaChnAttr.u32Height	= MD_VI_H;

	stVdaChnAttr.unAttr.stMdAttr.enVdaAlg	   = VDA_ALG_BG;
	stVdaChnAttr.unAttr.stMdAttr.enMbSize	   = VDA_MB_16PIXEL;
	stVdaChnAttr.unAttr.stMdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
	stVdaChnAttr.unAttr.stMdAttr.enRefMode	   = VDA_REF_MODE_DYNAMIC;
	stVdaChnAttr.unAttr.stMdAttr.u32VdaIntvl   = 12;
	stVdaChnAttr.unAttr.stMdAttr.u32BgUpSrcWgt = 128;
	stVdaChnAttr.unAttr.stMdAttr.u32MdBufNum   = 8;
	stVdaChnAttr.unAttr.stMdAttr.u32ObjNumMax  = 128;
	stVdaChnAttr.unAttr.stMdAttr.u32SadTh	   = 40;

	/*创建通道*/
	s32Ret = HI_MPI_VDA_CreateChn(VdaChn, &stVdaChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		Printf("Failed Create VDA Channel: 0x%x\n", s32Ret);
		pthread_mutex_unlock(&group.mutex);
		return s32Ret;
	}

	/*绑定VI*/
	stSrcChn.enModId = HI_ID_VPSS;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = MD_VPSS_CHN;

	stDestChn.enModId = HI_ID_VDA;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = VdaChn;

	CHECK_RET(HI_MPI_SYS_Bind(&stSrcChn, &stDestChn));

	/*启动接收图像*/
	CHECK_RET(HI_MPI_VDA_StartRecvPic(VdaChn));

	mdinfo.fd = HI_MPI_VDA_GetFd(VdaChn);
	Printf("MD fd=%d\n", mdinfo.fd);
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
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;
	VDA_CHN VdaChn;

	VdaChn = mdinfo.nVenc;

	pthread_mutex_lock(&group.mutex);
	if (mdinfo.fd != -1)
	{
		/*绑定VI*/
		stSrcChn.enModId = HI_ID_VPSS;
		stSrcChn.s32DevId = 0;
		stSrcChn.s32ChnId = MD_VPSS_CHN;

		stDestChn.enModId = HI_ID_VDA;
		stDestChn.s32DevId = 0;
		stDestChn.s32ChnId = VdaChn;
		/*停止接收图像*/
		CHECK_RET(HI_MPI_VDA_StopRecvPic(VdaChn));

		/*解绑定VI*/
		CHECK_RET(HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn));

		CHECK_RET(HI_MPI_VDA_DestroyChn(VdaChn));
		mdinfo.fd = -1;
	}
	pthread_mutex_unlock(&group.mutex);

	return 0;
}

