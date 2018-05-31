#include "mrecord.h"

#include "m_mp4.h"

//打开MP4读文件
//strFile			:文件名
//pInfo 			:文件的信息
//return NULL 失败，>0 成功
void *MP4_Open_Read(char *strFile, MP4_READ_INFO *pInfo)
{
    #if SD_RECORD_SUPPORT
	//MP4_CHECK mp4Check={0};
	//int ret;
	void *handle;
	char strIndex[128];

	memset(pInfo, 0, sizeof(MP4_READ_INFO));
	
	//获取索引文件名
	MP4_GetIndexFile(strFile, strIndex);
	//正常关闭的mp4文件
	MP4_INFO stMp4Info;
	handle = JP_OpenUnpkg(strFile, &stMp4Info, _FOPEN_BUF_SIZE);

	if(handle)
	{
		pInfo->iDataStart = 0;							//0表示封装完毕的文件，>0表示正在封装的文件
		pInfo->bNormal = 1;//mp4Check.bNormal;		
		pInfo->iFrameWidth = stMp4Info.iFrameWidth;		// 视频宽度
		pInfo->iFrameHeight = stMp4Info.iFrameHeight;	// 视频高度
		pInfo->dFrameRate = stMp4Info.dFrameRate;		// 帧速
		//memcpy(&pInfo->avcC, &stMp4Info.avcC, sizeof(MP4_AVCC));			// avcc
		pInfo->iNumVideoSamples = stMp4Info.iNumVideoSamples;			// VideoSample个数
		pInfo->iNumAudioSamples = stMp4Info.iNumAudioSamples;			// AudioSample个数
		strcpy(pInfo->szVideoMediaDataName, stMp4Info.szVideoMediaDataName);	// 视频编码名字 "avc1"
		strcpy(pInfo->szAudioMediaDataName, stMp4Info.szAudioMediaDataName);	// 音频编码名字 "samr" "alaw" "ulaw"	

		if(pInfo->iNumVideoSamples)						// 是否有视频
			pInfo->bHasVideo = 1;			
		if(pInfo->iNumAudioSamples)						// 是否有音频
			pInfo->bHasAudio = 1;
	}
	else
	{
		JP_MP4_INFO jp_mp4Info;
		handle = JP_OpenFile(strFile, 0, strIndex, &jp_mp4Info, _FOPEN_BUF_SIZE);
		
		if(handle)
		{
			pInfo->iDataStart = 0;//mp4Check.iDataStart;		//0表示封装完毕的文件，>0表示正在封装的文件
			pInfo->bNormal = 0;//mp4Check.bNormal;		
			pInfo->bHasVideo = jp_mp4Info.bHasVideo;		// 是否有视频
			pInfo->bHasAudio = jp_mp4Info.bHasAudio;		// 是否有音频
			pInfo->iFrameWidth = jp_mp4Info.iFrameWidth;	// 视频宽度
			pInfo->iFrameHeight = jp_mp4Info.iFrameHeight;	// 视频高度
			pInfo->dFrameRate = jp_mp4Info.dFrameRate;		// 帧速
			pInfo->iNumVideoSamples = jp_mp4Info.iNumVideoSamples;
			pInfo->iNumAudioSamples = jp_mp4Info.iNumAudioSamples;
			strcpy(pInfo->szVideoMediaDataName, jp_mp4Info.szVideoMediaDataName);	// 视频编码名字 "avc1"
			strcpy(pInfo->szAudioMediaDataName, jp_mp4Info.szAudioMediaDataName);	// 音频编码名字 "samr" "alaw" "ulaw"	
			//memcpy(&pInfo->avcC, &jp_mp4Info.avcC, sizeof(MP4_AVCC));// avcc
		}
	}

	//视频类型
	if(strcmp(pInfo->szVideoMediaDataName, "avc1") == 0)
	{
		pInfo->enMp4VideoType = VIDEO_TYPE_H264;
	}
	else if(strcmp(pInfo->szVideoMediaDataName, "hev1") == 0 || strcmp(pInfo->szVideoMediaDataName, "hvc1") == 0)
	{
		pInfo->enMp4VideoType = VIDEO_TYPE_H265;
	}
	else
	{
		pInfo->enMp4VideoType = VIDEO_TYPE_UNKNOWN;
	}

	//音频类型
	if(strcmp(pInfo->szAudioMediaDataName, "alaw") == 0)
	{
		pInfo->nMp4AudioType = AUDIO_TYPE_G711_A;
	}
	else if(strcmp(pInfo->szAudioMediaDataName, "ulaw") == 0)
	{
		pInfo->nMp4AudioType = AUDIO_TYPE_G711_U;
	}
	else
	{
		pInfo->nMp4AudioType = AUDIO_TYPE_UNKNOWN;
	}

	/*printf("MP4_Open_Read:%s, ret %x	Video=%d	Audio=%d\n",strFile,(U32)handle, pInfo->enMp4VideoType, pInfo->nMp4AudioType);*/
	//printf("video:%s,audio:%s\n",pInfo->szVideoMediaDataName, pInfo->szAudioMediaDataName);
	/*printf("iDataStart=%d,w%d h%d fr%f v_nr%d a_nr%d\n",pInfo->iDataStart,pInfo->iFrameWidth,pInfo->iFrameHeight,pInfo->dFrameRate,pInfo->iNumVideoSamples,pInfo->iNumAudioSamples);*/
	return handle;
    #else
    return 0;
    #endif
}

//关闭MP4读文件
void MP4_Close_Read(void *handle, MP4_READ_INFO *pInfo)
{
 #if SD_RECORD_SUPPORT
	if(!pInfo->bNormal)
	{
		JP_CloseFile(handle);
	}
	else
	{
		JP_CloseUnpkg(handle);
	}
#endif
}

//读取MP4文件I帧
//handle  MP4文件打开的句柄
//pInfo   文件信息
//pPack   解封h264或amr帧数据结构体 
//bForword  表示查找方向, TRUE向前(往右), FALSE, 向后(往左) 
//return    FALSE失败  TRUE 成功
BOOL MP4_ReadIFrame(void *handle, MP4_READ_INFO *pInfo, PAV_UNPKT pPack, BOOL bForword)
{
	//使用mp4库的读关键帧接口
	BOOL ret = 0;
#if SD_RECORD_SUPPORT
	int nIFrame=0;
	pPack->iType = JVS_UPKT_VIDEO;
	pPack->iSize = 0;

	if(pPack->iSampleId < 0)
		return FALSE;
	
	if(!pInfo->bNormal)
	{
		nIFrame = JP_ReadKeyFrame(handle, pPack->iSampleId, bForword);
		if(nIFrame >= 0)
		{
			pPack->iSampleId = nIFrame;
			ret = JP_ReadFile(handle, pPack);
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		nIFrame = JP_UnpkgKeyFrame(handle, pPack->iSampleId, bForword);
		if(nIFrame >= 0)
		{
			pPack->iSampleId = nIFrame;
			ret = JP_UnpkgOneFrame(handle, pPack);
		}
		else
		{
			return FALSE;
		}
	}
#endif
	return ret;
}
//读取MP4文件一帧数据
//handle  MP4文件打开的句柄
//pInfo   文件信息
//pPack   解封h264或amr帧数据结构体 
//return    正在进行封装的 返回0表示无数据可读，否则表示有数据读，对于完成封装的 FALSE失败  TRUE 成功
BOOL MP4_ReadOneFrame(void *handle, MP4_READ_INFO *pInfo, PAV_UNPKT pPack)
{
	BOOL ret;
    #if SD_RECORD_SUPPORT
	pPack->iSize = 0;
	if(!pInfo->bNormal)
	{
		ret = JP_ReadFile(handle, pPack);
	}
	else
	{
		ret = JP_UnpkgOneFrame(handle, pPack);
	}

	//if(pPack->iType == JVS_UPKT_AUDIO)
	//	printf("r ret%d,type%d,len%d, %x %x %x %x\n", ret, pPack->iType, pPack->iSize, pPack->pData[0], pPack->pData[1], pPack->pData[2], pPack->pData[3]);
#else
	ret = FALSE;
    #endif
    return ret;
}

//获取和iFrameVideo同步的音频帧号
int MP4_GetSyncAudioFrame(void *handle, MP4_READ_INFO *pInfo, int iFrameVideo)
{
	#if SD_RECORD_SUPPORT
    int iFrameAudio=0;
	if(pInfo->bNormal)
	{
        iFrameAudio = JP_PkgGetAudioSampleId(handle, iFrameVideo, NULL, NULL);
	}
    else
    {
        iFrameAudio = JP_JdxGetAudioSampleId(handle, iFrameVideo, NULL, NULL);
    }
    return iFrameAudio;
	#endif
	return 0;
}

