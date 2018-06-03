#include <jv_common.h>
#include <jv_ai.h>
#include "hicommon.h"
#include <mpi_ai.h>
#include <mpi_aenc.h>
#include "acodec.h"

#define DEV_AI	0
#define AI_OFFSET			0		//AI与VI的对应关系，
#define ACODEC_FILE     "/dev/acodec"

static int s_ai_ref = 0;
static int s_ai_frame_len = 0;
static int stopFlag = 0;//停止动作时的标志。一但置为1，jv_ai_get_frame时就清空BUFFER

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
        mixer_mic_ctrl = 1;
        if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
        {
            printf("%s: set acodec micin failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }

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
		//回声抵消功能
		AI_AEC_CONFIG_S ace_config;
		memset(&ace_config, 0, sizeof(AI_AEC_CONFIG_S));

		AUDIO_ANR_CONFIG_S  stAnrCfg;
		memset(&stAnrCfg, 0, sizeof(AUDIO_ANR_CONFIG_S));
		stAnrCfg.bUsrMode = HI_TRUE;
		stAnrCfg.s16NrIntensity = 15;
		stAnrCfg.s16NoiseDbThr = 60;

		AI_RNR_CONFIG_S     stRnrCfg;
		memset(&stRnrCfg, 0, sizeof(AI_RNR_CONFIG_S));
		stRnrCfg.bUsrMode = HI_TRUE;
		stRnrCfg.s32MaxNrLevel = 25;
		stRnrCfg.s32NoiseThresh = -50;
		stRnrCfg.s32NrMode = HI_FALSE;

		AUDIO_HPF_CONFIG_S  stHpfCfg;
		stHpfCfg.bUsrMode = 1;
		stHpfCfg.enHpfFreq = AUDIO_HPF_FREQ_80;

		AI_VQE_CONFIG_S vqe_config;
		memset(&vqe_config, 0, sizeof(AI_VQE_CONFIG_S));

		vqe_config.bAecOpen = HI_FALSE; //回音抵消的功能去掉，占用CPU太高
		vqe_config.bAnrOpen = HI_TRUE;	//降噪还是打开，不然声音太差
		vqe_config.bRnrOpen = HI_FALSE;
		vqe_config.bHpfOpen = HI_FALSE;
		vqe_config.s32WorkSampleRate = 8000;
		vqe_config.s32FrameSample = 320;	//3
		vqe_config.enWorkstate = VQE_WORKSTATE_NOISY;
		vqe_config.stAecCfg = ace_config;
		vqe_config.stAnrCfg = stAnrCfg;
		vqe_config.stRnrCfg = stRnrCfg;
		vqe_config.stHpfCfg = stHpfCfg;
		if (HI_MPI_AI_SetVqeAttr(0, 0, 0, 0, &vqe_config))
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

    /* set AENC chn attr */

    stAencAttr.enType = enType;
    stAencAttr.u32BufSize = 30;
    stAencAttr.u32PtNumPerFrm = u32AencPtNumPerFrm;

    if (PT_ADPCMA == stAencAttr.enType)
    {
        AENC_ATTR_ADPCM_S stAdpcmAenc;
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = ADPCM_TYPE_DVI4;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        AENC_ATTR_G711_S stAencG711;
        stAencAttr.pValue       = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        AENC_ATTR_G726_S stAencG726;
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = MEDIA_G726_40K;
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        AENC_ATTR_LPCM_S stAencLpcm;
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
				1,  		//是否静音，默认静音
				20			//麦克增益
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

int jv_ai_start(int channelid, jv_audio_attr_t *attr)
{
	AIO_ATTR_S	stAioAttr= {0};
	PAYLOAD_TYPE_E payloadType;

	memcpy(&sAudioAttr[0], attr, sizeof(jv_audio_attr_t));

	/* init stAio. all of cases will use it */
	stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	//3518和3516C目前只支持16位宽
	stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	stAioAttr.u32EXFlag = 0;
	stAioAttr.u32FrmNum = 30;
	stAioAttr.u32PtNumPerFrm = 320;
	stAioAttr.u32ChnCnt = 1;
	stAioAttr.u32ClkSel = 0;

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
	CHECK_RET(SAMPLE_COMM_AUDIO_AencBindAi(DEV_AI, 0, 0));

	jv_ai_SetMicgain(attr->micGain);

	Printf("jv_ai_start success\n");
	return 0;
}

int jv_ai_stop(int channelid)
{
	//关音频通道
	//XDEBUG(HI_MPI_AI_DisableChn(DEV_AI, channelid+AI_OFFSET), "DisableAIChn");
	{
		//XDEBUG(HI_MPI_AI_Disable(DEV_AI), "DisableAI");
		//disable dev 之前应该关掉所有的打开的通道lk20131126详细见HiMPP媒体软件处理开发参考715页

		SAMPLE_COMM_AUDIO_AencUnbindAi(DEV_AI, 0, 0);
		SAMPLE_COMM_AUDIO_StopAenc(1);
		//因为jv_ai_start中AENC,AI都打开了两个通道所以要各关闭两个，如果start中有修改这里要修改
		SAMPLE_COMM_AUDIO_StopAi(DEV_AI, 1, HI_FALSE, HI_FALSE);
	}

	return 0;
}

int jv_ai_get_frame(int channelid, jv_audio_frame_t *frame)
{
	int ret;
    AUDIO_STREAM_S stStream;

	ret = HI_MPI_AENC_GetStream(channelid+AI_OFFSET, &stStream, 0);
	if (ret == HI_SUCCESS)
	{
		if (sAudioAttr[channelid].encType == JV_AUDIO_ENC_PCM)
		{
			int i;
			unsigned short *src = (unsigned short *)stStream.pStream;
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
						printf("n\n");
					}
				}
			}
		}
		else
		{
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
		HI_MPI_AENC_ReleaseStream(channelid+AI_OFFSET, &stStream);
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
	return JVERR_FUNC_NOT_SUPPORT;
}

void jv_ai_setChatStatus(BOOL bChat)
{
	return;
}

BOOL jv_ai_GetChatStatus()
{
	return TRUE;
}

int jv_ai_RestartAec(int chan,BOOL bSwitch)
{
	return 0;
}

int jv_ai_RestartAnr(int channel,BOOL bSwitch)
{
	return 0;
}

int jv_ai_SetMicgain(int gain)
{
	if(gain < 0)
		return -1;

	U16 gainValHigh = 0x0002;
	U16 gainValLow = 0x0;
	U32 micVal = 0x0;

	int gainval = ((gain/3)>30)?31:gain/3;

	gainValLow = ((gainval << 2 | 0x02 ) << 8) | (gainval << 2 | 0x02);
	micVal = (gainValHigh << 16) | gainValLow;
	printf("set mic gain 0x%08X(0x%x)\n", micVal, gainval);

	char micCmd[64] = {0};
	sprintf(micCmd, "himm 0x201200C8 0x%08X", micVal);
	utl_system(micCmd);

	return HI_SUCCESS;
}

