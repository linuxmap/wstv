#include <jv_common.h>
#include <jv_ai.h>
#include "hicommon.h"
#include <mpi_ai.h>
#include <mpi_aenc.h>
#include "acodec.h"
#include <utl_filecfg.h>
#include <jv_ao.h>
#include "utl_common.h"

#define DEV_AI	0
#define CHN_AI 0
#define ACODEC_FILE     "/dev/acodec"

BOOL bChatStatus = FALSE;

BOOL bGetFrameEnable = TRUE;

#define MAX_CALLBACK_COUNT 3

typedef struct{
	jv_ai_pcm_callback_t aipcm_callback_ptr[MAX_CALLBACK_COUNT];
	BOOL bThreadCreated;
	pthread_t threadid;
	BOOL bStarted;
}AIInfo_t;

static AIInfo_t sAIInfo = {
		.aipcm_callback_ptr = {NULL,NULL,NULL},
		.bThreadCreated = FALSE,
		.threadid = -1
};


/******************************************************************************
* function : Acodec config [ s32Samplerate(0:8k, 1:16k ) ]
******************************************************************************/
static HI_S32 SAMPLE_Acodec_CfgAudio(AUDIO_SAMPLE_RATE_E enSample, HI_BOOL bMicIn)
{
    HI_S32 fdAcodec = -1;
    unsigned int i2s_fs_sel = 0;
    unsigned int mixer_mic_ctrl = 0;
    unsigned int gain_mic = 0;

    fdAcodec = open(ACODEC_FILE,O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }

    if(ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL))
    {
        printf("Reset audio codec error\n");
        close(fdAcodec);
        return HI_FAILURE;
    }

    if ((AUDIO_SAMPLE_RATE_8000 == enSample)
        || (AUDIO_SAMPLE_RATE_11025 == enSample)
        || (AUDIO_SAMPLE_RATE_12000 == enSample))
    {
        i2s_fs_sel = 0x18;
    }
    else if ((AUDIO_SAMPLE_RATE_16000 == enSample)
        || (AUDIO_SAMPLE_RATE_22050 == enSample)
        || (AUDIO_SAMPLE_RATE_24000 == enSample))
    {
        i2s_fs_sel = 0x19;
    }
    else if ((AUDIO_SAMPLE_RATE_32000 == enSample)
        || (AUDIO_SAMPLE_RATE_44100 == enSample)
        || (AUDIO_SAMPLE_RATE_48000 == enSample))
    {
        i2s_fs_sel = 0x1a;
    }
    else
    {
        printf("%s: not support enSample:%d\n", __FUNCTION__, enSample);
        close(fdAcodec);
        return HI_FAILURE;
    }

    if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel))
    {
        printf("%s: set acodec sample rate failed\n", __FUNCTION__);
        close(fdAcodec);
        return HI_FAILURE;
    }

    if (HI_TRUE == bMicIn)
    {
//        mixer_mic_ctrl = 1;
//        if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
//        {
//            printf("%s: set acodec micin failed\n", __FUNCTION__);
//            close(fdAcodec);
//            return HI_FAILURE;
//        }

        /* set volume plus (0~0x1f,default 0) */
        gain_mic = 0xf;
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
        {
            printf("%s: set acodec micin volume failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
        {
            printf("%s: set acodec micin volume failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }
    }

    close(fdAcodec);
    return HI_SUCCESS;
}

/******************************************************************************
* function : Start Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
        AIO_ATTR_S *pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, HI_BOOL bResampleEn, AI_VQE_CONFIG_S *pstAiVqeAttr)
{
    HI_S32 i;
    HI_S32 s32Ret;

    s32Ret = HI_MPI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret)
    {
        printf("%s: HI_MPI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_AI_Enable(AiDevId);
    if (s32Ret)
    {
        printf("%s: HI_MPI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = HI_MPI_AI_EnableChn(AiDevId, i);
        if (s32Ret)
        {
            printf("%s: HI_MPI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
            return s32Ret;
        }

        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
            if (s32Ret)
            {
                printf("%s: HI_MPI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }

        if (NULL != pstAiVqeAttr)
        {
            s32Ret = HI_MPI_AI_SetVqeAttr(AiDevId, i, 0, i, pstAiVqeAttr);
            if (s32Ret)
            {
                printf("%s: HI_MPI_AI_SetVqeAttr(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }

            s32Ret = HI_MPI_AI_EnableVqe(AiDevId, i);
            if (s32Ret)
            {
                printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }
    }

	if (pstAioAttr->enSamplerate == 8000)
	{
		AI_VQE_CONFIG_S vqe_config;
		memset(&vqe_config, 0, sizeof(AI_VQE_CONFIG_S));

		vqe_config.enWorkstate = VQE_WORKSTATE_NOISY;
		vqe_config.s32WorkSampleRate = AUDIO_SAMPLE_RATE_8000;
		vqe_config.s32FrameSample = pstAioAttr->u32PtNumPerFrm;
		
		vqe_config.bAecOpen = HI_FALSE;
		vqe_config.bAgcOpen= HI_TRUE;
		vqe_config.bAnrOpen = HI_TRUE;
		vqe_config.bHpfOpen = HI_TRUE;

		vqe_config.stAnrCfg.bUsrMode = HI_FALSE;

		vqe_config.stAgcCfg.bUsrMode = HI_FALSE;
		vqe_config.stAgcCfg.s8TargetLevel = -5;
		vqe_config.stAgcCfg.s8NoiseFloor = -40;
		vqe_config.stAgcCfg.s8MaxGain = 10;
		vqe_config.stAgcCfg.s8AdjustSpeed = 10;
		vqe_config.stAgcCfg.s8ImproveSNR = 2;
		vqe_config.stAgcCfg.s8UseHighPassFilt = 0;
		vqe_config.stAgcCfg.s8OutputMode = 0;
		vqe_config.stAgcCfg.s16NoiseSupSwitch = 1;

		//memset(&vqe_config.stEqCfg.s8GaindB, 0, VQE_EQ_BAND_NUM);
		//vqe_config.stEqCfg.s8GaindB[0] = -20;
		//vqe_config.stEqCfg.s8GaindB[1] = -10;
		//vqe_config.stEqCfg.s8GaindB[2] = -10;
		//vqe_config.stEqCfg.s8GaindB[3] = -10;
		//vqe_config.stEqCfg.s8GaindB[4] = -10;
		//vqe_config.stEqCfg.s8GaindB[5] = -10;
		//vqe_config.stEqCfg.s8GaindB[6] = 2;
		//vqe_config.stEqCfg.s8GaindB[7] = -1;
		//vqe_config.stEqCfg.s8GaindB[8] = -3;
		//vqe_config.stEqCfg.s8GaindB[9] = 3;
		//vqe_config.stEqCfg.s32Reserved = 0;

		vqe_config.stHpfCfg.bUsrMode = HI_FALSE;

		vqe_config.stAecCfg.bUsrMode = HI_TRUE;
		vqe_config.stAecCfg.s8CngMode = 0;
		vqe_config.stAecCfg.s8NearAllPassEnergy = 1;
		vqe_config.stAecCfg.s8NearCleanSupEnergy = 2;
		vqe_config.stAecCfg.s16DTHnlSortQTh = 16384;
		vqe_config.stAecCfg.s16EchoBandLow = 10;
		vqe_config.stAecCfg.s16EchoBandHigh = 25;
		vqe_config.stAecCfg.s16EchoBandLow2 = 47;
		vqe_config.stAecCfg.s16EchoBandHigh2 = 63;
		vqe_config.stAecCfg.s16ERLBand[0] = 4;
		vqe_config.stAecCfg.s16ERLBand[1] = 6;
		vqe_config.stAecCfg.s16ERLBand[2] = 22;
		vqe_config.stAecCfg.s16ERLBand[3] = 49;
		vqe_config.stAecCfg.s16ERLBand[4] = 50;
		vqe_config.stAecCfg.s16ERLBand[5] = 51;
		vqe_config.stAecCfg.s16ERL[0] = 7;
		vqe_config.stAecCfg.s16ERL[1] = 10;
		vqe_config.stAecCfg.s16ERL[2] = 16;
		vqe_config.stAecCfg.s16ERL[3] = 18;
		vqe_config.stAecCfg.s16ERL[4] = 18;
		vqe_config.stAecCfg.s16ERL[5] = 18;
		vqe_config.stAecCfg.s16ERL[6] = 18;
		vqe_config.stAecCfg.s16VioceProtectFreqL = 3;
		vqe_config.stAecCfg.s16VioceProtectFreqL1 = 6;
		vqe_config.stAecCfg.s32Reserved = 0;
		
		
		s32Ret = HI_MPI_AI_SetVqeAttr(0, 0, 0, 0, &vqe_config);
		if (s32Ret)
		{
			printf("%s: HI_MPI_AI_SetVqeAttr(%d,%d) failed with %#x\n",
					__FUNCTION__, 0, 0, s32Ret);
			return HI_FAILURE;
		}
		if (HI_MPI_AI_EnableVqe(0, 0))
		{
			printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n",
					__FUNCTION__, 0, 0, s32Ret);
			return HI_FAILURE;
		}

	}

    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
        HI_BOOL bResampleEn, HI_BOOL bVqeEn)
{
    HI_S32 i; 
    HI_S32 s32Ret;

    for (i=0; i<s32AiChnCnt; i++)
    {
        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AI_DisableReSmp(AiDevId, i);
            if(HI_SUCCESS != s32Ret)
            {
                printf("[Func]:%s [Line]:%d [Info]: failed with %#x\n", __FUNCTION__, __LINE__, s32Ret);
                return s32Ret;
            }
        }

        if (HI_TRUE == bVqeEn)
        {
            s32Ret = HI_MPI_AI_DisableVqe(AiDevId, i);
            if(HI_SUCCESS != s32Ret)
            {
                printf("[Func]:%s [Line]:%d [Info]: failed with %#x\n", __FUNCTION__, __LINE__, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = HI_MPI_AI_DisableChn(AiDevId, i);
        if(HI_SUCCESS != s32Ret)
        {
            printf("[Func]:%s [Line]:%d [Info]: failed with %#x\n", __FUNCTION__, __LINE__, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = HI_MPI_AI_Disable(AiDevId);
    if(HI_SUCCESS != s32Ret)
    {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Start Aenc
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAenc(HI_S32 s32AencChnCnt, HI_U32 u32AencPtNumPerFrm, PAYLOAD_TYPE_E enType)
{
    AENC_CHN AeChn;
    HI_S32 s32Ret, i;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    AENC_ATTR_G711_S stAencG711;
    AENC_ATTR_G726_S stAencG726;
    AENC_ATTR_LPCM_S stAencLpcm;

    /* set AENC chn attr */
    stAencAttr.enType = enType;
    stAencAttr.u32BufSize = 30;
    stAencAttr.u32PtNumPerFrm = u32AencPtNumPerFrm;

    if (PT_ADPCMA == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = ADPCM_TYPE_DVI4;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = MEDIA_G726_40K;
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencLpcm;
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
        return HI_FAILURE;
    }

    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;

        /* create aenc chn*/
        s32Ret = HI_MPI_AENC_CreateChn(AeChn, &stAencAttr);
        if (s32Ret != HI_SUCCESS)
        {
            printf("%s: HI_MPI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,
                   AeChn, s32Ret);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Aenc
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAenc(HI_S32 s32AencChnCnt)
{
    HI_S32 i;
    HI_S32 s32Ret;

    for (i=0; i<s32AencChnCnt; i++)
    {
        s32Ret = HI_MPI_AENC_DestroyChn(i);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
            return s32Ret;
        }
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Aenc bind Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AencBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
* function : Aenc unbind Ai
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AencUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);      
}

static jv_audio_attr_t sAudioAttr[1] =
		{
			{
				JV_AUDIO_SAMPLE_RATE_8000,
				JV_AUDIO_BIT_WIDTH_16,
				JV_AUDIO_ENC_PCM,
				-1, 		//喇叭音量，初始默认系统值
				-1,			//播放声音文件音量
				-1,			//麦克增益
				-1			//全双工对讲使用的mic增益
			},
		};
/**
 *@brief 获取音频相关设置
 *
 *@param channelid 音频通道号
 *@param attr 音频通道的设置参数
 *
 *@return 0 成功
 *
 */
int jv_ai_get_attr(int channelid, jv_audio_attr_t *attr)
{
	memcpy(attr, &sAudioAttr[channelid], sizeof(jv_audio_attr_t));
	return 0;
}

int jv_ai_set_attr(int channelid, jv_audio_attr_t *attr)
{
	memcpy(&sAudioAttr[channelid], attr, sizeof(jv_audio_attr_t));
	return 0;
}

/******************************************************************************
* function : get frame from Ai, send it  to Aenc or Ao
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AiProc(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AiFd;
    AUDIO_FRAME_S stFrame;
	AEC_FRAME_S stAECFrame;//回声抵消参考帧
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;
	jv_audio_frame_t frame;
	//int flag = 0;

	pthreadinfo_add((char *)__func__);

    s32Ret = HI_MPI_AI_GetChnParam(DEV_AI, CHN_AI, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
        return NULL;
    }

    stAiChnPara.u32UsrFrmDepth = 30;

    s32Ret = HI_MPI_AI_SetChnParam(DEV_AI, CHN_AI, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return NULL;
    }

    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(DEV_AI, CHN_AI);
    FD_SET(AiFd,&read_fds);

    while (sAIInfo.bStarted)
    {
        TimeoutVal.tv_sec = 0;
        TimeoutVal.tv_usec = 200 * 1000;
      	//printf("getting...\n");
        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);

        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            printf("%s: get ai frame select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AiFd, &read_fds))
        {
            /* get frame from ai chn */
			if(sAudioAttr[0].sampleRate == JV_AUDIO_SAMPLE_RATE_8000)
            {
	            if( hwinfo.remoteAudioMode == AUDIO_MODE_TWO_WAY)
	            {
	            	if(bGetFrameEnable)
	            	{
	            		s32Ret = HI_MPI_AI_GetFrame(DEV_AI, CHN_AI, &stFrame, &stAECFrame, HI_FALSE);						
					}
	            }
	            else
	            {
	            	s32Ret = HI_MPI_AI_GetFrame(DEV_AI, CHN_AI, &stFrame, NULL, HI_FALSE);
	            }
			}
			else
			{
				s32Ret = HI_MPI_AI_GetFrame(DEV_AI, CHN_AI, &stFrame, NULL, HI_FALSE);
			}
			if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n",\
                       __FUNCTION__, DEV_AI, CHN_AI, s32Ret);
                // usleep(1000*1000);
                utl_WaitTimeout(!sAIInfo.bStarted, 1000);
                continue;
            }
			//printf("frame getted\n");

            /* send frame to encoder */
			if(sAudioAttr[0].sampleRate == JV_AUDIO_SAMPLE_RATE_8000)
            {
            	if (hwinfo.remoteAudioMode == AUDIO_MODE_TWO_WAY)
            	{
					if(jv_ai_GetChatStatus())
						HI_MPI_AENC_SendFrame(0, &stFrame, &stAECFrame);
					else
						HI_MPI_AENC_SendFrame(0, &stFrame, NULL);
            	}
            	else
            	{
            		HI_MPI_AENC_SendFrame(0, &stFrame, NULL);
            	}
            }
            if (stFrame.u32Len > 0)
            {
            	frame.u32Len = stFrame.u32Len;
            	frame.u32Seq = stFrame.u32Seq;
            	frame.u64TimeStamp = stFrame.u64TimeStamp;
            	memcpy(frame.aData, stFrame.pVirAddr[0], stFrame.u32Len);
            }

            /* finally you must release the stream */
            if(sAudioAttr[0].sampleRate == JV_AUDIO_SAMPLE_RATE_8000)
			{
				if (hwinfo.remoteAudioMode == AUDIO_MODE_TWO_WAY)
				{
					HI_MPI_AI_ReleaseFrame(DEV_AI, CHN_AI,&stFrame, &stAECFrame);
				}
				else
				{
					HI_MPI_AI_ReleaseFrame(DEV_AI, CHN_AI,&stFrame, NULL);
				}
			}
			else
			{
				HI_MPI_AI_ReleaseFrame(DEV_AI, CHN_AI,&stFrame,NULL);
			}
			
			// 移到最后，避免资源未释放，导致AI无法停止
            if (frame.u32Len > 0)
            {
				int m;

				for(m = 0;m < MAX_CALLBACK_COUNT; ++m)
				{
					if(NULL != sAIInfo.aipcm_callback_ptr[m])
						sAIInfo.aipcm_callback_ptr[m](0,&frame);
				}
            }
        }
    }
	sAIInfo.bThreadCreated = FALSE;
    return NULL;
}

int jv_ai_start(int channelid, jv_audio_attr_t *attr)
{
	AIO_ATTR_S	stAioAttr= {0};
	PAYLOAD_TYPE_E payloadType;

	memcpy(&sAudioAttr[0], attr, sizeof(jv_audio_attr_t));

	/* init stAio. all of cases will use it */
	stAioAttr.enSamplerate = attr->sampleRate; //AUDIO_SAMPLE_RATE_8000;
	//3518和3516C目前只支持16位宽
	stAioAttr.enBitwidth = attr->bitWidth; //AUDIO_BIT_WIDTH_16;
	stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	stAioAttr.u32EXFlag = 1;
	stAioAttr.u32FrmNum = 30;
	if( attr->sampleRate == JV_AUDIO_SAMPLE_RATE_24000)
		stAioAttr.u32PtNumPerFrm = 1024;
	else if(attr->sampleRate == JV_AUDIO_SAMPLE_RATE_44100)
		stAioAttr.u32PtNumPerFrm = 480;
	else
		stAioAttr.u32PtNumPerFrm = 320;

	stAioAttr.u32ChnCnt = 1;		//2;	//1;
	stAioAttr.u32ClkSel = 0;		//1;	//0;

	switch(attr->encType)
	{
	default:
	case JV_AUDIO_ENC_PCM:
		payloadType = PT_LPCM;
		break;
	case JV_AUDIO_ENC_ADPCM:
		payloadType = PT_ADPCMA;
		break;
	case JV_AUDIO_ENC_G711_A:
		payloadType = PT_G711A;
		break;
	case JV_AUDIO_ENC_G711_U:
		payloadType = PT_G711U;
		break;
	case JV_AUDIO_ENC_G726_16K:
	case JV_AUDIO_ENC_G726_24K:
	case JV_AUDIO_ENC_G726_32K:
	case JV_AUDIO_ENC_G726_40K:
		payloadType = PT_G726;
		break;
	}
	CHECK_RET(SAMPLE_Acodec_CfgAudio(stAioAttr.enSamplerate, HI_FALSE));
	CHECK_RET(SAMPLE_COMM_AUDIO_StartAi(DEV_AI, stAioAttr.u32ChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, NULL));
	CHECK_RET(SAMPLE_COMM_AUDIO_StartAenc(1, stAioAttr.u32PtNumPerFrm, payloadType));

	sAIInfo.bStarted = 1;
	if (!sAIInfo.bThreadCreated)
	{
		int rr_min_priority, rr_max_priority;	
		struct sched_param thread_dec_param;
		pthread_attr_t thread_dec_attr;
		pthread_attr_init(&thread_dec_attr);
		pthread_attr_getschedparam(&thread_dec_attr, &thread_dec_param);
		pthread_attr_setstacksize(&thread_dec_attr, LINUX_THREAD_STACK_SIZE);					//栈大小

		if(attr->sampleRate == JV_AUDIO_SAMPLE_RATE_44100)
		{
			rr_min_priority = sched_get_priority_min(SCHED_RR); 
			rr_max_priority = sched_get_priority_max(SCHED_RR);
			thread_dec_param.sched_priority = (rr_min_priority + rr_max_priority)/2 + 1; 

			pthread_attr_setschedpolicy(&thread_dec_attr, SCHED_RR);
			pthread_attr_setschedparam(&thread_dec_attr, &thread_dec_param);
			pthread_attr_setinheritsched(&thread_dec_attr, PTHREAD_EXPLICIT_SCHED); 		
			pthread_create(&sAIInfo.threadid, &thread_dec_attr, (void *)SAMPLE_COMM_AUDIO_AiProc, NULL);

		}
		else
		{
			thread_dec_param.sched_priority = 0;
			pthread_attr_setschedpolicy(&thread_dec_attr, SCHED_OTHER);
			pthread_attr_setschedparam(&thread_dec_attr, &thread_dec_param);
			pthread_attr_setinheritsched(&thread_dec_attr, PTHREAD_EXPLICIT_SCHED); 
			pthread_create(&sAIInfo.threadid, &thread_dec_attr, (void *)SAMPLE_COMM_AUDIO_AiProc, NULL);
		}
		
		sAIInfo.bThreadCreated = TRUE;
	}

	
	//CHECK_RET(SAMPLE_COMM_AUDIO_AencBindAi(DEV_AI, 0, 0));

	jv_ai_SetMicgain(attr->micGain);

	printf("jv_ai_start success\n");
	return 0;
}

int jv_ai_stop(int channelid)
{
	sAIInfo.bStarted = 0;
	pthread_join(sAIInfo.threadid,NULL);
	sAIInfo.bThreadCreated = FALSE;
	
	MPP_CHN_S stSrcChn, stDestChn;//AI--src,AENC--Dest

	stSrcChn.enModId = HI_ID_AI;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = 0;
	stDestChn.enModId = HI_ID_AENC;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = 0;

	//因为jv_ai_start中AENC,AI都打开了两个通道所以要各关闭两个，如果start中有修改这里要修改
	CHECK_RET(HI_MPI_AENC_DestroyChn(0));
	if(sAudioAttr[0].sampleRate == JV_AUDIO_SAMPLE_RATE_8000)
	{
		CHECK_RET(HI_MPI_AI_DisableVqe(0,0));
	}
	CHECK_RET(HI_MPI_AI_DisableChn(DEV_AI, 0));
	CHECK_RET(HI_MPI_AI_DisableChn(DEV_AI, 1));
	CHECK_RET(HI_MPI_AI_Disable(DEV_AI));
	CHECK_RET(HI_MPI_AI_ClrPubAttr(DEV_AI));

	return 0;
}

int jv_ai_get_frame(int channelid, jv_audio_frame_t *frame)
{
	int ret;
    AUDIO_STREAM_S stStream;

	ret = HI_MPI_AENC_GetStream(channelid, &stStream, 0);
//	if (channelid == 0)
//		Printf("chid: %d, ret: %d\n", channelid, ret);
	if (ret == HI_SUCCESS)
	{
		if (sAudioAttr[channelid].encType == JV_AUDIO_ENC_PCM)
		{
			int i;
			unsigned short *src = (unsigned short *)stStream.pStream;
//			printf("src: 0x%04x, 0x%04x,   ", src[0], src[1]);
			stStream.u32Len /= 2;
			for (i=0;i<stStream.u32Len;i++)
			{
				/*
				 * 这里为什么要加0x80呢？
				 * 因为：对于16位采样，安静时的值类似这样：0xFFF1, 0x0003, 0x0004, 0xFFF5, ...
				 *       当变成8位时，会变成这样：0xFF, 0x00, 0x00, 0xFF
				 *       可惜的是，对于8位值，0xFF和0x00虽然只差了1，但这个1代表的声音却比较大。就变成了噪声
				 *       因此，将0xFFF5 + 0x80， 就意味着将它们做了4舍5入的处理
				*/
#if 1
				//这里，取高9位（最高位再舍弃，实际是8位），提高音量
				frame->aData[i] = (0x40 + src[i]) >> 7;
				frame->aData[i] ^= 0x80;
#else
				frame->aData[i] = (0x80 + src[i]) >> 8;
				frame->aData[i] ^= 0x80;
#endif
			}
			frame->u32Len = stStream.u32Len;

//			printf("src: 0x%02x, 0x%02x\n", frame->aData[0], frame->aData[1]);
			if (0)
			{
				int j;
				static int cnt = 0;
				cnt++;
				if (cnt >= 25)
				{
					cnt = 0;
					for (i=0;i<64;i++)
					{
						printf("%d", (signed char)(frame->aData[i]+20-0x80));
						for (j=0;j<(signed char)(frame->aData[i]+20-0x80);j++)
						{
							printf(" ");
						}
						//i+=10;
						printf("n\n");
					}
				}
			}
		}
		else
		{
//			Printf("data: %p, len: %d\n", stStream.pStream, stStream.u32Len);
			//海思有4字节的帧头
			frame->u32Len = stStream.u32Len-4;
			memcpy(frame->aData, &stStream.pStream[4], frame->u32Len);

			if (0)
			{
				int i;

				for (i=0;i<16;i++)
					printf("%02x, ", frame->aData[i]);
				printf("\n");
			}
		}

		frame->u64TimeStamp = stStream.u64TimeStamp;
		frame->u32Seq = stStream.u32Seq;
		HI_MPI_AENC_ReleaseStream(channelid, &stStream);
		return 0;
	}
	return -1;
}

/**
 *@brief 注册回调函数，提供音频的PCM帧给上层应用
 *
 *@param channelid 音频通道号
 *@param callback_ptr 回调函数指针
 */
int jv_ai_set_pcm_callback( int channelid, jv_ai_pcm_callback_t callback_ptr)
{
	static int i = 0;
	if (i >= MAX_CALLBACK_COUNT)
	{
		printf("error : ai callback full!!\n");
		return -1;
	}
	sAIInfo.aipcm_callback_ptr[i++] = callback_ptr;
	return 0;
}

void jv_ai_setChatStatus(BOOL bChat)
{
	bGetFrameEnable = FALSE;
	if(bChat != bChatStatus)
	{
		jv_ai_RestartAec(0,bChat);
		bChatStatus = bChat;
	}
	bGetFrameEnable = TRUE;
}

BOOL jv_ai_GetChatStatus()
{
	return bChatStatus;
}

int jv_ai_RestartAec(int chan,BOOL bSwitch)
{
	if(hwinfo.remoteAudioMode == AUDIO_MODE_TWO_WAY)
	{
		HI_S32 s32Ret;

		AI_VQE_CONFIG_S vqe_config;
		memset(&vqe_config, 0, sizeof(AI_VQE_CONFIG_S));

		HI_S32 ret = HI_MPI_AI_GetVqeAttr(0,chan,&vqe_config);
		if(0 != ret)
		{
			printf("##############  HI_MPI_AI_GetVqeAttr failed!!! %x \n",ret);
		}

		s32Ret = HI_MPI_AI_DisableVqe(0,chan);
		if(0 != s32Ret)
		{
			printf("##############  HI_MPI_AI_GetVqeAttr failed!!! %x \n",s32Ret);
		}
		

		if(bSwitch)
		{
			vqe_config.bAecOpen = HI_TRUE;
			vqe_config.stAgcCfg.bUsrMode = HI_TRUE;
			jv_ai_SetMicgain(sAudioAttr[0].micGainTalk);
		}
		else
		{
			vqe_config.bAecOpen = HI_FALSE;
			vqe_config.stAgcCfg.bUsrMode = HI_FALSE;
			jv_ai_SetMicgain(sAudioAttr[0].micGain);
		}

		if(HI_MPI_AI_SetVqeAttr(0,chan,0,0,&vqe_config))
		{
			printf("%s: HI_MPI_AI_SetVqeAttr(%d,%d) failed with %#x\n",\
						__FUNCTION__, 0, 0, s32Ret);
			return HI_FAILURE;
		}
		if(HI_MPI_AI_EnableVqe(0,chan))
		{
			printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n",\
						__FUNCTION__, 0, 0, s32Ret);
			return HI_FAILURE;
		}			

	}
	return 0;
}
int jv_ai_SetMicMute(int mute)
{
	int ret;
	HI_S32 fdAcodec = -1;
    fdAcodec = open(ACODEC_FILE,O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open acodec,%s : %s\n", __FUNCTION__, ACODEC_FILE,strerror(errno));
        return -1;
    }	
	
	if(ioctl(fdAcodec, ACODEC_SET_MICL_MUTE, &mute))
	{
	 printf("setmic mute error \n");
	}
	if(ioctl(fdAcodec, ACODEC_SET_MICR_MUTE, &mute))
	{
	 printf("setmic mute error \n");
	}
	close(fdAcodec);
	return 0;
	
}

int jv_ai_SetMicgain(int gain)
{
#if 0
	if(gain < 0)
		return -1;
	
    HI_S32 fdAcodec = -1;
	unsigned int gain_mic = 0;
	
    fdAcodec = open(ACODEC_FILE,O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }

    gain_mic = gain > 15 ? 15 : gain;
    if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
    {
        printf("%s: set acodec micin volume failed\n", __FUNCTION__);
        close(fdAcodec);
        return HI_FAILURE;
    }
    if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
    {
        printf("%s: set acodec micin volume failed\n", __FUNCTION__);
        close(fdAcodec);
        return HI_FAILURE;
    }	
	close(fdAcodec);
	printf("================>>>>> set micgain is %d , actuly : %d\n",gain,gain_mic);
#else
	U16 gainValHigh = 0x0002;
	U16 gainValLow = 0x0;
	U32 micVal = 0x0;

	//gain = gain > 45 ? 45 : gain;

	int gainval = gain;		//((gain/3)>30)?31:gain/3;

	if(gain == -1)
	{
		return HI_FAILURE;
	}
	gainValLow = ((gainval << 2 | 0x02 ) << 8) | (gainval << 2 | 0x02);
	micVal = (gainValHigh << 16) | gainValLow;
	printf("set mic gain 0x%08X(0x%x)\n", micVal, gainval);

	char micCmd[64] = {0};
	sprintf(micCmd, "himm 0x201200C8 0x%08X", micVal);
	utl_system(micCmd);
#endif 
	return HI_SUCCESS;
}

