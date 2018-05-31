#include <stdio.h>
#include <string.h>
#include <hi_common.h>
#include "3518_isp.h"
#include <jv_isp.h>

#define ISP_3518_CFG_FILE	CONFIG_PATH"/3518-isp.cfg"
static int sensor = 0xffffffff;

int jv_isp_get_sensor(int *value)
{
    if(sensor == 0xffffffff)
	{ 
        isp_ioctl(0,GET_ID,(unsigned long)&sensor);
    } 
    *value = sensor;
	return 0;
}

/*设置曝光设置*/

int jv_isp_exposure_set(jvExposure_t *exposure)
{
	EXPOSURE_DATA data;
	int i,j;
	
	isp_exposure_get(&data);
	data.type = exposure->type;
	
	if(exposure->type == 1)
	{                                              //手动模式
		data.bManualExpLineEnable = exposure->bManualExpLineEnable;
		if(exposure->bManualExpLineEnable)
		{
			data.u32ExpLine = exposure->u32ExpLine;
		}
		data.bManualAGainEnable = exposure->bManualAGainEnable;
		if(exposure->bManualAGainEnable)
		{
			data.s32AGain = exposure->s32AGain;
		}
		data.bManualDGainEnable = exposure->bManualDGainEnable;
		if(exposure->bManualDGainEnable)
		{
			data.s32DGain = exposure->s32DGain;
		}
	}
	else{                                                            //自动模式
		data.enAEMode = exposure->enAEMode;
		data.u16ExpTimeMax = exposure->u16ExpTimeMax;
		if((exposure->u16ExpTimeMax)%65536 <= (exposure->u16ExpTimeMin)%65536)
			{
				data.u16ExpTimeMin = (exposure->u16ExpTimeMax)%65536 - 1;
			}
		else
			{
				data.u16ExpTimeMin = (exposure->u16ExpTimeMin)%65536;
			}
			
		data.u16DGainMax = exposure->u16DGainMax;
		if((exposure->u16DGainMax)%256 <= (exposure->u16DGainMin)%256)
			{
				data.u16DGainMin = (exposure->u16DGainMax)%256 - 1;
			}
		else
			{
				data.u16DGainMin = (exposure->u16DGainMin)%256;
			}
		
		data.u16AGainMax = exposure->u16AGainMax;
		if((exposure->u16AGainMax)%256 <= (exposure->u16AGainMin)%256)
			{
				data.u16AGainMin = (exposure->u16AGainMax)%256 - 1;
			}
		else
			{
				data.u16AGainMin = (exposure->u16AGainMin)%256;
			}
		
		data.u8ExpStep = exposure->u8ExpStep;
		data.s16ExpTolerance = exposure->s16ExpTolerance;
		data.u8ExpCompensation = exposure->u8ExpCompensation;
		//data.enFrameEndUpdateMode = exposure->enFrameEndUpdateMode;
		data.bByPassAE = exposure->bByPassAE;
		/*for(i=0;i<WEIGHT_ZONE_ROW;i++)
			{
			for(j=0;j<WEIGHT_ZONE_COLUMN;j++)
				{
				data.u8Weight[i][j] = exposure->u8WeightLine[i*WEIGHT_ZONE_COLUMN+j];
			}
		}*/
		memcpy(&(data.u8Weight),&(exposure->u8WeightLine),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
		
	}
	
	
	isp_exposure_set(&data);
	return 0;
}

/*获取曝光设置*/
int jv_isp_exposure_get(jvExposure_t *exposure)
{
	EXPOSURE_DATA data;
	int i,j;
	
	isp_exposure_get(&data);
	exposure->type = data.type;
	exposure->s32AGain = data.s32AGain;
	exposure->s32DGain = data.s32DGain;
	exposure->u32ExpLine = data.u32ExpLine;
	exposure->bManualExpLineEnable = data.bManualExpLineEnable;
	exposure->bManualAGainEnable = data.bManualAGainEnable;
	exposure->bManualDGainEnable = data.bManualDGainEnable;
	exposure->enAEMode = data.enAEMode;
	exposure->u16ExpTimeMax = data.u16ExpTimeMax;
	exposure->u16ExpTimeMin = data.u16ExpTimeMin;
	exposure->u16DGainMax = data.u16DGainMax;
	exposure->u16DGainMin = data.u16DGainMin;
	exposure->u16AGainMax = data.u16AGainMax;
	exposure->u16AGainMin = data.u16AGainMin;
	exposure->u8ExpStep = data.u8ExpStep;
	exposure->s16ExpTolerance = data.s16ExpTolerance;
	exposure->u8ExpCompensation = data.u8ExpCompensation;
	//exposure->enFrameEndUpdateMode = data.enFrameEndUpdateMode;
	exposure->bByPassAE = data.bByPassAE;
	/*for(i=0;i<WEIGHT_ZONE_ROW;i++)
		{
		for(j=0;j<WEIGHT_ZONE_COLUMN;j++)
			{
			exposure->u8WeightLine[i*WEIGHT_ZONE_COLUMN+j] = data.u8Weight[i][j];
		}
	}*/
	memcpy(&(exposure->u8WeightLine),&(data.u8Weight),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
	return 0;
}

/*设置白平衡设置*/

int jv_isp_wb_set(jvWb_t *wb)
{
	WB_DATA data;
	int i,j;
	
	isp_wb_get(&data);
	data.type = wb->type;
	
	if(wb->type == 1)
	{                        //手动模式
		data.u16Rgain = wb->u16Rgain;
		data.u16Ggain = wb->u16Ggain;
		data.u16Bgain = wb->u16Bgain;
	}
	else
	{                                   //自动模式
		data.u8RGStrength = wb->u8RGStrength;
		data.u8BGStrength = wb->u8BGStrength;
		data.u8ZoneSel = wb->u8ZoneSel;
		data.u8HighColorTemp = wb->u8HighColorTemp;
		
		if((wb->u8HighColorTemp)%256 <= (wb->u8LowColorTemp)%256)
		{
			data.u8LowColorTemp = (wb->u8HighColorTemp)%256 - 1;
		}
		else
		{
			data.u8LowColorTemp = (wb->u8LowColorTemp)%256;
		}

		memcpy(&(data.u8Weight),&(wb->u8WeightLine),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
	}
	
	isp_wb_set(&data);
	return 0;
}

/*获取白平衡设置*/

int jv_isp_wb_get(jvWb_t *wb)
{
	WB_DATA data;
	int i,j;

	isp_wb_get(&data);
	wb->type = data.type;
	wb->u16Rgain = data.u16Rgain;
	wb->u16Ggain = data.u16Ggain;
	wb->u16Bgain = data.u16Bgain;
	wb->u8RGStrength = data.u8RGStrength;
	wb->u8BGStrength = data.u8BGStrength;
	wb->u8ZoneSel = data.u8ZoneSel;
	wb->u8HighColorTemp = data.u8HighColorTemp;
	wb->u8LowColorTemp = data.u8LowColorTemp;
	/*for(i=0;i<WEIGHT_ZONE_ROW;i++)
		{
		for(j=0;j<WEIGHT_ZONE_COLUMN;j++)
			{
			wb->u8WeightLine[i*WEIGHT_ZONE_COLUMN+j] = data.u8Weight[i][j];
		}
	}*/
	memcpy(&(wb->u8WeightLine),&(data.u8Weight),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
	return 0;
	
}

/*设置伽马设置*/

int jv_isp_gamma_set(jvGamma_t *gamma)
{
	GAMMA_DATA data;
	int i;

	isp_gamma_get(&data);
	
	switch(gamma->bEnable.bEnable)
	{
		case JV_HI_FALSE:
			data.bEnable.bEnable = HI_FALSE;
			break;
		case JV_HI_TRUE:
			data.bEnable.bEnable = HI_TRUE;
			break;
		default : break;
	}
	if(gamma->bEnable.bEnable)
	{	
		switch(gamma->enGammaCurve)
		{
			case JV_ISP_GAMMA_CURVE_DEFAULT:
				data.enGammaCurve = ISP_GAMMA_CURVE_DEFAULT;
				break;
			case JV_ISP_GAMMA_CURVE_SRGB:
				data.enGammaCurve = ISP_GAMMA_CURVE_SRGB;
				break;
			case JV_ISP_GAMMA_CURVE_USER_DEFINE:
				data.enGammaCurve = ISP_GAMMA_CURVE_USER_DEFINE;
				break;
			default:break ;
		}
	}	
	for(i=0;i<257;i++)
	{
		data.u16Gamma[i] = gamma->u16Gamma[i];
	}
	for(i=0;i<257;i++)
	{
		data.u16Gamma_FE[i] = gamma->u16Gamma_FE[i];
	}

	isp_gamma_set(&data);
	
	return 0;
}

/*获取伽马设置*/

int jv_isp_gamma_get(jvGamma_t *gamma)
{
	GAMMA_DATA data;
	int i;

	isp_gamma_get(&data);

	switch(data.bEnable.bEnable)
	{
		case HI_TRUE :
			gamma->bEnable.bEnable = JV_HI_TRUE;
			break;
		case HI_FALSE :
			gamma->bEnable.bEnable = JV_HI_FALSE;
			break;
		default: break;
	}

	switch(data.enGammaCurve)
	{
		case ISP_GAMMA_CURVE_DEFAULT:
			gamma->enGammaCurve = JV_ISP_GAMMA_CURVE_DEFAULT;
			break;
		case ISP_GAMMA_CURVE_SRGB:
			gamma->enGammaCurve = JV_ISP_GAMMA_CURVE_SRGB;
			break;
		case ISP_GAMMA_CURVE_USER_DEFINE:
			gamma->enGammaCurve = JV_ISP_GAMMA_CURVE_USER_DEFINE;
			break;
		default:break ;
	}

	
	for(i=0;i<257;i++)
	{
		gamma->u16Gamma[i] = data.u16Gamma[i];
	}
	for(i=0;i<257;i++)
	{
		gamma->u16Gamma_FE[i] = data.u16Gamma_FE[i];
	}		

	return 0;
}


/*设置锐度设置*/

int jv_isp_sharpen_set(jvSharpen_t *sharpen)
{
	SHARPEN_DATA data;
	int i;
	
	isp_sharpen_get(&data);
	data.bEnable = sharpen->bEnable;
	if(data.bEnable == 1)
	{
		data.bManualEnable = sharpen->bManualEnable;
		data.u8StrengthTarget = sharpen->u8StrengthTarget;
		data.u8StrengthMin = sharpen->u8StrengthMin;
		for(i = 0;i < 8;i++)
		{
			data.u8SharpenAltD[i] = sharpen->u8SharpenAltD[i];
		}
		for(i = 0;i < 8;i++)
		{
			data.u8SharpenAltUd[i] = sharpen->u8SharpenAltUd[i];
		}
	}
	
	isp_sharpen_set(&data);
	return 0;
}


/*获得锐度设置*/

int jv_isp_sharpen_get(jvSharpen_t*sharpen)
{
	SHARPEN_DATA data;
	int i;
	
	isp_sharpen_get(&data);
	sharpen->bEnable = data.bEnable;
	sharpen->bManualEnable = data.bManualEnable;
	sharpen->u8StrengthMin = data.u8StrengthMin;
	sharpen->u8StrengthTarget = data.u8StrengthTarget;
	
	for(i = 0;i < 8;i++)
	{
		sharpen->u8SharpenAltD[i] = data.u8SharpenAltD[i];
	}
	for(i = 0;i < 8;i++)
	{
		sharpen->u8SharpenAltUd[i] = data.u8SharpenAltUd[i];
	}
	return 0;
}

/*设置饱和度设置*/

int jv_isp_ccm_set(jvCcm_t * ccm)
{
	CCM_DATA data;
	int i;
	
	isp_ccm_get(&data);
	data.bSatManualEnable = ccm->bSatManualEnable;
	
	data.u8SatTarget = ccm->u8SatTarget;
			
	for(i = 0;i < 8;i++)
	{
		data.au8Sat[i] = ccm->au8Sat[i];
	}
			
	data.pu32Value = ccm->pu32Value;
	data.u16HighColorTemp = ccm->u16HighColorTemp;
			
	for(i = 0;i < 9;i++)
	{
		data.au16HighCCM[i] = ccm->au16HighCCM[i];
	}
	data.u16MidColorTemp = ccm->u16MidColorTemp;
	for(i = 0;i < 9;i++)
	{
		data.au16MidCCM[i] = ccm->au16MidCCM[i];
	}
	data.u16LowColorTemp = ccm->u16LowColorTemp;
	for(i = 0;i < 9;i++)
	{
		data.au16LowCCM[i] = ccm->au16LowCCM[i];
	}
	
	isp_ccm_set(&data);
	
	return 0;
}

/*获取饱和度设置*/

int jv_isp_ccm_get(jvCcm_t * ccm)
{
	CCM_DATA data;
	int i;
	
	isp_ccm_get(&data);
	ccm->bSatManualEnable = data.bSatManualEnable;
	ccm->u8SatTarget = data.u8SatTarget;
	
	for(i = 0;i < 8;i++)
	{
		ccm->au8Sat[i] = data.au8Sat[i];
	}
	
	ccm->pu32Value = data.pu32Value;
	
	ccm->u16HighColorTemp = data.u16HighColorTemp;
	for(i = 0;i < 9;i++)
	{
		ccm->au16HighCCM[i] = data.au16HighCCM[i];
	}
	
	ccm->u16MidColorTemp = data.u16MidColorTemp;
	for(i = 0;i < 9;i++)
	{
		ccm->au16MidCCM[i] = data.au16MidCCM[i];
	}
	
	ccm->u16LowColorTemp = data.u16LowColorTemp;
	for(i = 0;i < 9;i++)
	{
		ccm->au16LowCCM[i] = data.au16LowCCM[i];
	}

	return 0;
}

/* 设置key和其对应的value*/
int jv_isp_cfg_set(char *key, int value)
{
	char str[32];
	sprintf(str,"%d",value);

	return utl_fcfg_set_value(ISP_3518_CFG_FILE, key ,str);
}

/* 获取key对应的值*/
char *jv_isp_cfg_get(char *key)
{
	return utl_fcfg_get_value(ISP_3518_CFG_FILE, key);
}

/*刷新ISP 文件*/
int jv_isp_cfg_flush()
{
	return utl_fcfg_flush(ISP_3518_CFG_FILE);
}


/* 保存ISP 初始配置*/
void jv_isp_cfg_init()
{
	printf("Isp setting saved... \n");
	int i,j;
	char *p;
	char str[32];
	jvExposure_t exposure;
	jvCcm_t ccm;
	jvGamma_t gamma;
	jvSharpen_t sharpen;
	jvWb_t wb;

	//获取并保存曝光设置
	jv_isp_exposure_get(&exposure);
	jv_isp_cfg_set("Exptype",exposure.type);	
	jv_isp_cfg_set("Exps32Again",exposure.s32AGain);
	jv_isp_cfg_set("Exps32DGain",exposure.s32DGain);
	jv_isp_cfg_set("Expu32ExpLine",exposure.u32ExpLine);
	jv_isp_cfg_set("ExpbManualExpLineEnable",exposure.bManualExpLineEnable);
	jv_isp_cfg_set("ExpbManualAGainEnable",exposure.bManualAGainEnable);
	jv_isp_cfg_set("ExpbManualDGainEnable",exposure.bManualDGainEnable);
	jv_isp_cfg_set("Expu16ExpTimeMax",(int)exposure.u16ExpTimeMax);
	jv_isp_cfg_set("Expu16ExpTimeMin",(int)exposure.u16ExpTimeMin);
	jv_isp_cfg_set("Expu16DGainMax",(int)exposure.u16DGainMax);
	jv_isp_cfg_set("Expu16DGainMin",(int)exposure.u16DGainMin);
	jv_isp_cfg_set("Expu16AGainMax",(int)exposure.u16AGainMax);
	jv_isp_cfg_set("Expu16AGainMin",(int)exposure.u16AGainMin);
	jv_isp_cfg_set("Expu8ExpStep",(int)exposure.u8ExpStep);
	jv_isp_cfg_set("Exps16ExpTolerance",(int)exposure.s16ExpTolerance);
	jv_isp_cfg_set("Expu8ExpCompensation",(int)exposure.u8ExpCompensation);
	//jv_isp_cfg_set("ExpenFrameEndUpdateMode",exposure.enFrameEndUpdateMode);
	jv_isp_cfg_set("ExpbByPassAE",exposure.bByPassAE);

	for(i = 0;i < WEIGHT_ZONE_ROW;i++)
	{
		for(j = 0;j < WEIGHT_ZONE_COLUMN;j++)
		{
			memset(str,0,sizeof(str));
			if(exposure.u8WeightLine[i * WEIGHT_ZONE_COLUMN + j] == '\0')
			{
				printf("null, break!!!!\n");
				break;
			}
			sprintf(str,"Expu8WeightLine[%d]",i * WEIGHT_ZONE_COLUMN + j);
			jv_isp_cfg_set(str,(int)exposure.u8WeightLine[i * WEIGHT_ZONE_COLUMN + j]);
		}
	}

	//获取白平衡设置并保存
	jv_isp_wb_get(&wb);
	jv_isp_cfg_set("Wbtype",wb.type);
	jv_isp_cfg_set("Wbu16Rgain",(int)wb.u16Rgain);
	jv_isp_cfg_set("Wbu16Ggain",(int)wb.u16Ggain);
	jv_isp_cfg_set("Wbu16Bgain",(int)wb.u16Bgain);
	jv_isp_cfg_set("Wbu8RGStrength",(int)wb.u8RGStrength);
	jv_isp_cfg_set("Wbu8BGStrength",(int)wb.u8BGStrength);
	jv_isp_cfg_set("Wbu8ZoneSel",(int)wb.u8ZoneSel);
	jv_isp_cfg_set("Wbu8HighColorTemp",(int)wb.u8HighColorTemp);
	jv_isp_cfg_set("Wbu8LowColorTemp",(int)wb.u8LowColorTemp);

	for(i = 0;i < WEIGHT_ZONE_ROW;i++)
	{
		for(j = 0;j < WEIGHT_ZONE_COLUMN;j++)
		{	
			memset(str,0,sizeof(str));
			if(wb.u8WeightLine[i * WEIGHT_ZONE_COLUMN + j] == '\0')
			{
				printf("null, break!!!!\n");
				break;
			}
			sprintf(str,"Wbu8WeightLine[%d]",i * WEIGHT_ZONE_COLUMN + j);
			jv_isp_cfg_set(str,(int)wb.u8WeightLine[i * WEIGHT_ZONE_COLUMN + j]);
		}
	}
	
	//获取伽马设置并保存
	jv_isp_gamma_get(&gamma);
	jv_isp_cfg_set("GambEnable",gamma.bEnable.bEnable);
	jv_isp_cfg_set("GamenGammaCurve",gamma.enGammaCurve);

	for(i = 0;i < 257; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Gamu16Gamma[%d]",i);
		jv_isp_cfg_set(str,(int)gamma.u16Gamma[i]);
	}
	for(i = 0;i < 257; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Gamu16Gamma_FE[%d]",i);
		jv_isp_cfg_set(str,(int)gamma.u16Gamma_FE[i]);
	}

	//获取锐度设置并保存
	jv_isp_sharpen_get(&sharpen);
	
	jv_isp_cfg_set("ShabEnable",sharpen.bEnable);
	jv_isp_cfg_set("ShabManualEnable",sharpen.bManualEnable);
	jv_isp_cfg_set("Shau8StrengthTarget",(int)sharpen.u8StrengthTarget);
	jv_isp_cfg_set("Shau8StrengthMin",(int)sharpen.u8StrengthMin);
	for(i = 0;i < 8; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Shau8SharpenAltD[%d]",i);
		jv_isp_cfg_set(str,(int)sharpen.u8SharpenAltD[i]);
	}
	for(i = 0;i < 8; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Shau8SharpenAltUd[%d]",i);
		jv_isp_cfg_set(str,(int)sharpen.u8SharpenAltUd[i]);
	}

	//获取饱和度设置并保存
	jv_isp_ccm_get(&ccm);
	
	jv_isp_cfg_set("CcmbSatManualEnable",ccm.bSatManualEnable);
	jv_isp_cfg_set("Ccmu8SatTarget",(int)ccm.u8SatTarget);
	jv_isp_cfg_set("Ccmpu32Value",ccm.pu32Value);
	jv_isp_cfg_set("Ccmu16HighColorTemp",(int)ccm.u16HighColorTemp);
	jv_isp_cfg_set("Ccmu16MidColorTemp",(int)ccm.u16MidColorTemp);
	jv_isp_cfg_set("Ccmu16LowColorTemp",(int)ccm.u16LowColorTemp);
	for(i = 0;i < 8; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Ccmau8Sat[%d]",i);
		jv_isp_cfg_set(str,(int)ccm.au8Sat[i]);
	}
	for(i = 0;i < 9; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Ccmau16HighCCM[%d]",i);
		jv_isp_cfg_set(str,(int)ccm.au16HighCCM[i]);
	}
	for(i = 0;i < 9; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Ccmau16MidCCM[%d]",i);
		jv_isp_cfg_set(str,(int)ccm.au16MidCCM[i]);
	}
	for(i = 0;i < 9; i++)
	{	
		memset(str,0,sizeof(str));
		sprintf(str,"Ccmau16LowCCM[%d]",i);
		jv_isp_cfg_set(str,(int)ccm.au16LowCCM[i]);
	}

	i = jv_isp_cfg_flush(); 
	p = jv_isp_cfg_get("Ccmau16LowCCM[8]");
	printf("p = %s\n",p); 

	printf("Isp setting saved done...\n");
	
	
}

