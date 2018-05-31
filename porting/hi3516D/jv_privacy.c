#include "jv_common.h"
#include "hicommon.h"
#include "jv_privacy.h"
#include <mpi_region.h>

#define REGION_COLOR		0x0
#define MAX_PRIVACY_REGION_NUM 8
static U32 sCoverColor=0;

static jv_privacy_attr_t privacyAttr;
static int privacyID[MAX_PRIVACY_REGION_NUM] ;

/**
 *@brief do initialize 	//内存初始化，读取完全透明文件信息
 *@return 0 if success.
 *
 */
int jv_privacy_init(void)
{
	int i;
	
	memset(&privacyAttr, 0, sizeof(privacyAttr));
	for (i=0;i<MAX_PRIVACY_REGION_NUM;i++)
		privacyID[i] = -1;
	return 0;
}

/**
 *@brief do de-initialize
 *@return 0 if success.
 *
 */
int jv_privacy_deinit(void)
{
	jv_privacy_stop(0);
	return 0;
}

/**
 *@brief set attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of privacy. 
 *@return 0 if success.
 *
 */
int jv_privacy_set_attr(int channelid, jv_privacy_attr_t *attr)
{
	memcpy(&(privacyAttr) , attr, sizeof(jv_privacy_attr_t));
	return 0;
}

/**
 *@brief get attribute
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@param attr attribute of privacy. 
 *@return 0 if success.
 *
 */
int jv_privacy_get_attr(int channelid, jv_privacy_attr_t *attr)
{
	memcpy(attr, &(privacyAttr) , sizeof(jv_privacy_attr_t));
	return 0;
}

/**
 *@brief start privacy 
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_privacy_start(int channelid)
{
	U32 i;
	int ret;

	RGN_ATTR_S stCoverAttr;
	MPP_CHN_S coverChn;
	RGN_CHN_ATTR_S stCoverChnAttr;

	jv_privacy_stop(channelid);

	coverChn.enModId = HI_ID_VPSS;
	coverChn.s32ChnId = 0;
	coverChn.s32DevId = 0;

	for(i=0; i<MAX_PRIVACY_REGION_NUM; i++)
	{
		if(privacyAttr.rect[i].w>0 && privacyAttr.rect[i].h>0)
		{
			stCoverAttr.enType = COVER_RGN;
			stCoverChnAttr.enType = COVER_RGN;
			stCoverChnAttr.bShow = HI_TRUE;
			stCoverChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
			stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X = JV_ALIGN_CEILING(privacyAttr.rect[i].x, 2);
			stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y = JV_ALIGN_CEILING(privacyAttr.rect[i].y, 2);
			stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width = JV_ALIGN_CEILING(privacyAttr.rect[i].w, 2);
			stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height = JV_ALIGN_CEILING(privacyAttr.rect[i].h, 2);
			stCoverChnAttr.unChnAttr.stCoverChn.u32Color = sCoverColor;
			stCoverChnAttr.unChnAttr.stCoverChn.u32Layer = i;

			/*
			前8个，留给overlay
			*/
			privacyID[i] = i+MAX_OSD_WINDOW;
			ret = HI_MPI_RGN_Create(privacyID[i], &stCoverAttr);
			if (ret != 0)
			{
				Printf("ERROR: Failed HI_MPI_RGN_Create: %x, for chn: %d\n", ret, i);
				continue;
			}
			ret = HI_MPI_RGN_AttachToChn(privacyID[i], &coverChn, &stCoverChnAttr);
			if (ret != 0)
			{
				Printf("ERROR: Failed HI_MPI_RGN_AttachToChn: 0x%x\n", ret);
				Printf("Handle:%d, Set cover region rect:{%d,%d,%d,%d}\n",
					privacyID[i], privacyAttr.rect[i].x, privacyAttr.rect[i].y, privacyAttr.rect[i].w, privacyAttr.rect[i].h);
				continue;
			}

		}
		else
			privacyID[i] = -1;
	}
	return 0;
}


/**
 *@brief stop privacy
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_privacy_stop(int channelid)
{
	int ret;
	MPP_CHN_S coverChn;

	coverChn.enModId = HI_ID_VPSS;
	coverChn.s32ChnId = 0;
	coverChn.s32DevId = 0;

	int i;
	for(i=0; i<MAX_PRIVACY_REGION_NUM; i++)
	{
		if(privacyID[i] >= 0)
		{
			ret = HI_MPI_RGN_DetachFromChn(privacyID[i], &coverChn);
			if(ret!=HI_SUCCESS)
			{
				Printf("Error Here\n");
				//return RET_ERR;
			}
			ret = HI_MPI_RGN_Destroy(privacyID[i]);
			if(ret!=HI_SUCCESS)
			{
				Printf("Error Here\n");
				//return RET_ERR;
			}
			privacyID[i]	= -1;
		}
	}
	return 0;
}

