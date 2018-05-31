#include <jv_common.h>
#include "hicommon.h"
#include <mpi_ai.h>
#include <mpi_adec.h>
#include <jv_ai.h>
#include <jv_ao.h>
#include "jv_gpio.h"
#include "AW8733A.h"
#include "acodec.h"

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

#define DEV_AO	0
static jv_audio_attr_t gAudioAttr = {0};
/******************************************************************************
* function : Start Ao
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt,
        AIO_ATTR_S *pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, HI_BOOL bResampleEn)
{
    HI_S32 i;
    HI_S32 s32Ret;

    s32Ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
    if(HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_SetPubAttr(%d) failed with %#x!\n", __FUNCTION__, \
               AoDevId,s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_AO_Enable(AoDevId);
    if(HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_Enable(%d) failed with %#x!\n", __FUNCTION__, AoDevId, s32Ret);
        return HI_FAILURE;
    }

    for (i=0; i<s32AoChnCnt; i++)
    {
        s32Ret = HI_MPI_AO_EnableChn(AoDevId, i);
        if(HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AO_EnableChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
            return HI_FAILURE;
        }

        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
            s32Ret |= HI_MPI_AO_EnableReSmp(AoDevId, i, enInSampleRate);
            if(HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_AO_EnableReSmp(%d,%d) failed with %#x!\n", __FUNCTION__, AoDevId, i, s32Ret);
                return HI_FAILURE;
            }
        }
    }
    if (pstAioAttr->enSamplerate == 8000)
	{
		AO_VQE_CONFIG_S stAoVqeCfg;
		memset(&stAoVqeCfg, 0, sizeof(stAoVqeCfg));
	
		stAoVqeCfg.bAgcOpen = HI_TRUE;
		stAoVqeCfg.bAnrOpen = HI_TRUE;
		stAoVqeCfg.bHpfOpen = HI_TRUE;
		stAoVqeCfg.bEqOpen = HI_FALSE;
	
		stAoVqeCfg.s32FrameSample = pstAioAttr->u32PtNumPerFrm;
		stAoVqeCfg.enWorkstate = VQE_WORKSTATE_NOISY;
		stAoVqeCfg.s32WorkSampleRate = pstAioAttr->enSamplerate;	//AUDIO_SAMPLE_RATE_8000;
	
		stAoVqeCfg.stAgcCfg.bUsrMode = HI_TRUE;
		stAoVqeCfg.stAgcCfg.s8TargetLevel = -5;
		stAoVqeCfg.stAgcCfg.s8NoiseFloor = -30;
		stAoVqeCfg.stAgcCfg.s8MaxGain = 10;
		stAoVqeCfg.stAgcCfg.s8AdjustSpeed = 10;
		stAoVqeCfg.stAgcCfg.s8ImproveSNR = 2;
		stAoVqeCfg.stAgcCfg.s8UseHighPassFilt = 0;
		stAoVqeCfg.stAgcCfg.s8OutputMode = 1;
		stAoVqeCfg.stAgcCfg.s16NoiseSupSwitch = 1;
	
	
		stAoVqeCfg.stAnrCfg.bUsrMode = HI_FALSE;
	
		stAoVqeCfg.stHpfCfg.bUsrMode = HI_FALSE;

	/*
		memset(&stAoVqeCfg.stEqCfg.s8GaindB, 0, VQE_EQ_BAND_NUM);
		stAoVqeCfg.stEqCfg.s8GaindB[0] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[1] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[2] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[3] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[4] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[5] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[6] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[7] = -40;
		stAoVqeCfg.stEqCfg.s8GaindB[8] = 0;
		stAoVqeCfg.stEqCfg.s8GaindB[9] = 0;
		stAoVqeCfg.stEqCfg.s32Reserved = 0;
	*/
		s32Ret = HI_MPI_AO_SetVqeAttr(0, 0, &stAoVqeCfg);
		if (s32Ret)
		{
			printf("%s: HI_MPI_AO_SetVqe3Attr(%d,%d) failed with %#x\n", __FUNCTION__, AoDevId, 0, s32Ret);
			return s32Ret;
		}
	
		s32Ret = HI_MPI_AO_EnableVqe(0, 0);
		if (s32Ret)
		{
			printf("%s: HI_MPI_AO_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AoDevId, 0, s32Ret);
			return s32Ret;
		}
	}

    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Ao
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn)
{
    HI_S32 i;
    HI_S32 s32Ret;

    for (i=0; i<s32AoChnCnt; i++)
    {
        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_AO_DisableReSmp failed with %#x!\n", __FUNCTION__, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = HI_MPI_AO_DisableChn(AoDevId, i);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AO_DisableChn failed with %#x!\n", __FUNCTION__, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = HI_MPI_AO_Disable(AoDevId);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_Disable failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Start Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StartAdec(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    HI_S32 s32Ret;
    ADEC_CHN_ATTR_S stAdecAttr;
    ADEC_ATTR_ADPCM_S stAdpcm;
    ADEC_ATTR_G711_S stAdecG711;
    ADEC_ATTR_G726_S stAdecG726;
    ADEC_ATTR_LPCM_S stAdecLpcm;

    stAdecAttr.enType = enType;
    stAdecAttr.u32BufSize = 20;
    stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */

    if (PT_ADPCMA == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdpcm;
        stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE ;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG726;
        stAdecG726.enG726bps = G726_BPS ;
    }
    else if (PT_LPCM == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecLpcm;
        stAdecAttr.enMode = ADEC_MODE_PACK;/* lpcm must use pack mode */
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAdecAttr.enType);
        return HI_FAILURE;
    }

    /* create adec chn*/
    s32Ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ADEC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,\
               AdChn,s32Ret);
        return s32Ret;
    }
    return 0;
}

/******************************************************************************
* function : Stop Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_StopAdec(ADEC_CHN AdChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_ADEC_DestroyChn(AdChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ADEC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
               AdChn, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Ao bind Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
* function : Ao unbind Adec
******************************************************************************/
HI_S32 SAMPLE_COMM_AUDIO_AoUnbindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = AdChn;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn); 
}

int jv_ao_start(int channelid ,jv_audio_attr_t *attr)
{
	AIO_ATTR_S	stAioAttr= {0};
	memset(&gAudioAttr, 0, sizeof(jv_audio_attr_t));
	memcpy(&gAudioAttr, attr, sizeof(jv_audio_attr_t));
    /* init stAio. all of cases will use it */
    stAioAttr.enSamplerate = attr->sampleRate;//AUDIO_SAMPLE_RATE_8000;//
    stAioAttr.enBitwidth = attr->bitWidth;//AUDIO_BIT_WIDTH_16;//
    stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
	if( attr->sampleRate == JV_AUDIO_SAMPLE_RATE_24000)
		stAioAttr.u32PtNumPerFrm = 320 * 3;
	else if(attr->sampleRate == JV_AUDIO_SAMPLE_RATE_44100)
		stAioAttr.u32PtNumPerFrm = 441;
	else if(attr->sampleRate == JV_AUDIO_SAMPLE_RATE_48000)
		stAioAttr.u32PtNumPerFrm = 1024;
	else
		stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt = 1;
    stAioAttr.u32ClkSel = 0;
	PAYLOAD_TYPE_E payload;
	switch(attr->encType)
	{
	default:
	case JV_AUDIO_ENC_G711_A:
		payload = PT_G711A;
		break;
	case JV_AUDIO_ENC_G711_U:
		payload = PT_G711U;
		break;
	case JV_AUDIO_ENC_PCM:
		payload = PT_PCMU;
		break;
	}
//	payload = PT_LPCM;
	if(attr->encType != JV_AUDIO_ENC_PCM)
	{
	    
	    CHECK_RET( SAMPLE_COMM_AUDIO_StartAdec(0, payload));
		CHECK_RET( SAMPLE_COMM_AUDIO_StartAo(DEV_AO, stAioAttr.u32ChnCnt, &stAioAttr, 0, HI_FALSE));
	    CHECK_RET( SAMPLE_COMM_AUDIO_AoBindAdec(DEV_AO, 0, 0));
	}
	else
	{
	    CHECK_RET( SAMPLE_COMM_AUDIO_StartAo(DEV_AO, stAioAttr.u32ChnCnt, &stAioAttr, 0, HI_FALSE));
	    //CHECK_RET( SAMPLE_COMM_AUDIO_StartAdec(0, payload));
	    //CHECK_RET( SAMPLE_COMM_AUDIO_AoBindAdec(DEV_AO, 0, 0));
	}

	printf("...............ao start success..............\n");

	return 0;
}

int jv_ao_stop(int channelid)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = DEV_AO;
    stDestChn.s32ChnId = 0;

    HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
	HI_MPI_ADEC_DestroyChn(0);
	HI_MPI_AO_DisableChn(DEV_AO, 0);
	HI_MPI_AO_Disable(DEV_AO);
	
	HI_MPI_AO_ClrPubAttr(DEV_AO);
	return 0;
}

int jv_ao_send_frame(int channelid, jv_audio_frame_t *frame)
{
	int ret;
	if(gAudioAttr.encType == JV_AUDIO_ENC_PCM)
	{
		AUDIO_FRAME_S stFrame;
		if(gAudioAttr.sampleRate == JV_AUDIO_SAMPLE_RATE_8000)
			if(frame->u32Len != 640)
				return 0;
		if(gAudioAttr.sampleRate == JV_AUDIO_SAMPLE_RATE_24000)
			if(frame->u32Len != 640*3)
				return 0;
		if(gAudioAttr.sampleRate == JV_AUDIO_SAMPLE_RATE_48000)
			if(frame->u32Len != 2048)
				return 0;

		//memcpy(stFrame.pVirAddr[0], frame->aData, frame->u32Len);
		stFrame.pVirAddr[0] = frame->aData;
		stFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
		stFrame.enSoundmode = AUDIO_SOUND_MODE_MONO;
		stFrame.u32Len = frame->u32Len;
		stFrame.u32Seq = frame->u32Seq;
		ret = HI_MPI_AO_SendFrame(DEV_AO, 0, &stFrame, -1);
		if (ret != HI_SUCCESS)
		{
			printf("HI_MPI_AO_SendFrame failed: %x,frame->u32Len=%d\n", ret,frame->u32Len);
			return -1;
		}
	}
	else
	{
		AUDIO_STREAM_S stStream;
		unsigned char aData[JV_MAX_AUDIO_FRAME_LEN];

		stStream.pStream = aData;
		unsigned char header[4] = {0,1,0xa0, 0};
		header[3] = frame->u32Len/2;

		memcpy(aData, header, 4);
		memcpy(&aData[4], frame->aData, frame->u32Len);
		stStream.u32Len = frame->u32Len+4;
		stStream.u32Seq = frame->u32Seq;
		ret = HI_MPI_ADEC_SendStream(DEV_AO, &stStream, TRUE);
		if (ret != HI_SUCCESS)
		{
			printf("send faile: %x\n", ret);
			return -1;
		}
	}
	return 0;
}

int jv_ao_get_status(int channelid, jv_ao_status *aoStatus)
{
	AO_CHN_STATE_S stStatus;
	int ret = HI_MPI_AO_QueryChnStat(DEV_AO, channelid, &stStatus);
	if (ret != HI_SUCCESS)
	{
		printf("HI_MPI_AO_QueryChnStat failed: %x\n", ret);
		return -1;
	}
	aoStatus->aoBufBusyNum = stStatus.u32ChnBusyNum;
	aoStatus->aoBufFreeNum = stStatus.u32ChnFreeNum;
	aoStatus->aoBufTotalNum = stStatus.u32ChnTotalNum;
	return 0;
}

void jv_ao_adec_end()
{
	int ret = -1;
	ret = HI_MPI_ADEC_SendEndOfStream(0, FALSE);
    if (0 != ret)    
    {    
        printf("%s: HI MPI ADEC SendEndOfStream failed: %#x!\n", __FUNCTION__, ret);
    }
}

int jv_ao_mute(BOOL bMute)
{
	if (strcmp(hwinfo.devName,"HXBJRB") == 0)
	{
		if (higpios.audioOutMute.group != -1)
		{
			jv_gpio_write(higpios.audioOutMute.group, higpios.audioOutMute.bit, bMute?1:0);
		}
	}
	else
	{
		if (access("/dev/AW8733A", F_OK) != 0)
		{
			if (
				HWTYPE_MATCH(HW_TYPE_C8A)
				)
			{
				utl_system("insmod /home/ipc_drv/extdrv/AW8733A.ko Group=7 Index=1 MuxReg=0x200F00E4 MuxVal=0 OnVal=1");
			}
			else
			{
				utl_system("insmod /home/ipc_drv/extdrv/AW8733A.ko Group=6 Index=1 MuxReg=0x200F00C4 MuxVal=0 OnVal=1");
			}
			usleep(1000*1000);
			
			if (access("/dev/AW8733A", F_OK) != 0)
			{
				utl_system("mknod /dev/AW8733A c 10 `cat /proc/misc | grep AW8733A | awk '{print $1}'`");
				usleep(100*1000);
			}
		}

		int fd = -1;
		fd = open("/dev/AW8733A", 0);
		if (fd == -1)
		{
			printf("\n\n open AW8733A error !! \n\n");
			return 0;
		}
		if (bMute == 1)
			ioctl(fd, AW8733A_SET_PA, 0);
		else
		{
			if (HWTYPE_MATCH(HW_TYPE_A4))
			{
				ioctl(fd, AW8733A_SET_PA, 2);
			}
			else
			{
				ioctl(fd, AW8733A_SET_PA, 4);
			}
		}
		
		close(fd);
	}
	
	return 0;
}

#define ACODEC_FILE     "/dev/acodec"
int jv_ao_ctrl(int volCtrl)
{
	int mode = 0;
	int ret;
	HI_S32 fdAcodec = -1;

	printf("=============> jv_ao_ctrl : 0x%x \n",volCtrl);
    fdAcodec = open(ACODEC_FILE,O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open acodec,%s : %s\n", __FUNCTION__, ACODEC_FILE,strerror(errno));
        return HI_FAILURE;
    }	

	mode =0;
	ret = ioctl(fdAcodec, ACODEC_SET_PD_DACL,  &mode);
    ret = ioctl(fdAcodec, ACODEC_SET_PD_DACR,  &mode);
	
	ACODEC_VOL_CTRL vol_ctrl;
	vol_ctrl.vol_ctrl_mute = 0;
	vol_ctrl.vol_ctrl = volCtrl;	//0x11;
	ioctl(fdAcodec, ACODEC_SET_DACL_VOL, &vol_ctrl);
	ioctl(fdAcodec, ACODEC_SET_DACR_VOL, &vol_ctrl);

	close(fdAcodec);

	gAudioAttr.level = volCtrl;	
	return 0;
}

int jv_ao_get_attr(int channelid, jv_audio_attr_t *attr)
{
	memcpy(attr, &gAudioAttr, sizeof(jv_audio_attr_t));
	return 0;
}

