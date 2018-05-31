
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utl_filecfg.h>

#include <jv_isp.h>

#define ISP_CFG_PATH	CONFIG_PATH"/isp/"

/**
文件内容如下：
u32ExpLine=10
s32AGain=20
s32DGain=30
 */

static int __misp_read_cfg(char *cfg, jvISP_t * isp_cfg)
{
	keyvalue_t *kv;
	char path[256];
	int cur = 0;
	sprintf(path, ISP_CFG_PATH"%s", cfg);

	utl_fcfg_start_getnext(path);
	while((kv = utl_fcfg_get_next(path, &cur)))
	{
		//AE
		if (strcmp(kv->key, "u32ExpLine") == 0)
			isp_cfg->exposure.u32ExpLine = atoi(kv->value);
		else if (strcmp(kv->key, "s32AGain") == 0)
			isp_cfg->exposure.s32AGain = atoi(kv->value);
		else if (strcmp(kv->key, "s32DGain") == 0)
			isp_cfg->exposure.s32DGain = atoi(kv->value);
		else if (strcmp(kv->key, "u32ExpLine") == 0)
			isp_cfg->exposure.u32ExpLine = atoi(kv->value);
		else if (strcmp(kv->key, "bManualExpLineEnable") == 0)
			isp_cfg->exposure.bManualExpLineEnable = atoi(kv->value);
		else if (strcmp(kv->key, "bManualAGainEnable") == 0)
			isp_cfg->exposure.bManualAGainEnable = atoi(kv->value);
		else if (strcmp(kv->key, "bManualDGainEnable") == 0)
			isp_cfg->exposure.bManualDGainEnable = atoi(kv->value);
		else if (strcmp(kv->key, "enAEMode") == 0)
			isp_cfg->exposure.enAEMode = atoi(kv->value);
		else if (strcmp(kv->key, "u16ExpTimeMax") == 0)
			isp_cfg->exposure.u16ExpTimeMax = atoi(kv->value);
		else if (strcmp(kv->key, "u16ExpTimeMin") == 0)
			isp_cfg->exposure.u16ExpTimeMin = atoi(kv->value);
		else if (strcmp(kv->key, "u16DGainMax") == 0)
			isp_cfg->exposure.u16DGainMax = atoi(kv->value);
		else if (strcmp(kv->key, "u16DGainMin") == 0)
			isp_cfg->exposure.u16DGainMin = atoi(kv->value);
		else if (strcmp(kv->key, "u16AGainMax") == 0)
			isp_cfg->exposure.u16AGainMax = atoi(kv->value);
		else if (strcmp(kv->key, "u16AGainMin") == 0)
			isp_cfg->exposure.u16AGainMin = atoi(kv->value);
		else if (strcmp(kv->key, "u8ExpStep") == 0)
			isp_cfg->exposure.u8ExpStep = atoi(kv->value);
		else if (strcmp(kv->key, "s16ExpTolerance") == 0)
			isp_cfg->exposure.s16ExpTolerance = atoi(kv->value);
		else if (strcmp(kv->key, "u8ExpCompensation") == 0)
			isp_cfg->exposure.u8ExpCompensation = atoi(kv->value);
		else if (strcmp(kv->key, "enFrameEndUpdateMode") == 0)
			isp_cfg->exposure.enFrameEndUpdateMode = atoi(kv->value);
		
		//sharpen
		//else if (strcmp(kv->key, "bEnable") == 0)
		//	isp_cfg->gamma.bEnable = atoi(kv->value);
		//else if (strcmp(kv->key, "bManualEnable") == 0)
		//	isp_cfg->gamma.bManualEnable = atoi(kv->value);
		//else if (strcmp(kv->key, "u8StrengthMin") == 0)
		//	isp_cfg->gamma.u8StrengthMin = atoi(kv->value);
		//else if (strcmp(kv->key, "u8StrengthTarget") == 0)
		//	isp_cfg->gamma.u8StrengthTarget = atoi(kv->value);
		else if (strcmp(kv->key, "u8SharpenAltD") == 0)
			{
				char * value;
				int tmp_val[8];
				value=utl_fcfg_get_value(path,"u8SharpenAltD");
			//	sscanf(value,"{%d,%d,%d,%d,%d,%d,%d,%d}",&tmp_val[0],tmp_val[1],tmp_val[2],tmp_val[3],tmp_val[4],tmp_val[5],tmp_val[6],tmp_val[7]);
															
			}
		else if (strcmp(kv->key, "u8SharpenAltUd") == 0)
			{
				
			}
		
	}
	utl_fcfg_end_getnext(path);

	return 0;
}

int misp_change_cfg(char *cfg)
{
	jvISP_t isp;
	jv_isp_exposure_get(&(isp.exposure));
	jv_isp_ccm_get(&(isp.ccm));
	jv_isp_sharpen_get(&(isp.sharpen));
	jv_isp_wb_get(&(isp.wb));
	jv_isp_gamma_get(&(isp.gamma));
	__misp_read_cfg(cfg, &isp);
	
	jv_isp_exposure_set(&(isp.exposure));
	jv_isp_gamma_set(&(isp.gamma));
//	jv_isp_ccm_set(&(isp.ccm));
//	jv_isp_sharpen_set(&(isp.sharpen));
//	jv_isp_wb_set(&(isp.wb));
	
	printf("u32ExpLine = %d\n",isp.exposure.u32ExpLine);
	printf("s32AGain = %d\n",isp.exposure.s32AGain);
	printf("s32DGain = %d\n",isp.exposure.s32DGain);
	printf("bManualExpLineEnable = %d\n",isp.exposure.bManualExpLineEnable);
	printf("bManualAGainEnable = %d\n",isp.exposure.bManualAGainEnable);
	printf("bManualDGainEnable = %d\n",isp.exposure.bManualDGainEnable);
    printf("enAEMode = %d\n",isp.exposure.enAEMode);
	printf("u16ExpTimeMax = %d\n",isp.exposure.u16ExpTimeMax);
	printf("u16ExpTimeMin = %d\n",isp.exposure.u16ExpTimeMin);
	printf("u16DGainMax = %d\n",isp.exposure.u16DGainMax);
	printf("u16DGainMin = %d\n",isp.exposure.u16DGainMin);
	printf("u16AGainMax = %d\n",isp.exposure.u16AGainMax);
	
	printf("u16AGainMin = %d\n",isp.exposure.u16AGainMin);
	printf("u8ExpStep = %d\n",isp.exposure.u8ExpStep);
	printf("s16ExpTolerance = %d\n",isp.exposure.s16ExpTolerance);
	printf("u8ExpCompensation = %d\n",isp.exposure.u8ExpCompensation);
	printf("enFrameEndUpdateMode = %d\n",isp.exposure.enFrameEndUpdateMode);	
	
	
	
	return 0;
}
