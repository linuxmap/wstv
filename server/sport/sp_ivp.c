#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <jv_common.h>
#include "sp_define.h"
#include "sp_ivp.h"
#include "mivp.h"
#include <SYSFuncs.h>

/**
 *@brief 布防
 *
 *@param channelid 智能分析通道
 */
int sp_ivp_start(int channelid)
{
	__FUNC_DBG__();
	MIVP_t ivp;
	
	mivp_get_param(channelid, &ivp);
	ivp.st_rl_attr.bEnable = TRUE;
	mivp_set_param(channelid, &ivp);
	WriteConfigInfo();
	mivp_restart(channelid);
	return 0;
}

/**
 *@brief 撤防
 *
 *@param channelid 智能分析通道
 */
int sp_ivp_stop(int channelid)
{
	__FUNC_DBG__();
	MIVP_t ivp;
	
	mivp_get_param(channelid, &ivp);
	ivp.st_rl_attr.bEnable = FALSE;
	mivp_set_param(channelid, &ivp);
	WriteConfigInfo();
	mivp_restart(channelid);
	return 0;
}
int sp_ivp_get_param(int channelid, SPIVP_t *param)
{
	MIVP_t ivp;
	
	mivp_get_param(channelid, &ivp);
	param->bEnable = ivp.st_rl_attr.bEnable;
	param->nDelay = ivp.st_rl_attr.stAlarmOutRL.nDelay;
	param->bStarting = ivp.st_rl_attr.stAlarmOutRL.bStarting;
	param->nRgnCnt = ivp.st_rl_attr.nRgnCnt;
	memcpy(param->stRegion, ivp.st_rl_attr.stRegion, sizeof(param->stRegion));
	param->bDrawFrame = ivp.st_rl_attr.bDrawFrame;
	param->bFlushFrame = ivp.st_rl_attr.bFlushFrame;
	param->bMarkObject = ivp.st_rl_attr.bMarkObject;
	param->bMarkAll = ivp.st_rl_attr.bMarkAll;
	param->bOpenCount = ivp.st_count_attr.bOpenCount;
	param->bShowCount = ivp.st_count_attr.bShowCount;
	param->bPlateSnap = ivp.bPlateSnap;
	strcpy(param->sSnapRes, ivp.sSnapRes);
	param->nAlpha = ivp.st_rl_attr.nAlpha;
	param->nSen = ivp.st_rl_attr.nSen;
	param->nThreshold = ivp.st_rl_attr.nThreshold;
	param->nStayTime = ivp.st_rl_attr.nStayTime;
	param->bEnableRecord = ivp.st_rl_attr.stAlarmOutRL.bEnableRecord;
	param->bOutAlarm1 = ivp.st_rl_attr.stAlarmOutRL.bOutAlarm1;
	param->bOutClient = ivp.st_rl_attr.stAlarmOutRL.bOutClient;
	param->bOutEMail = ivp.st_rl_attr.stAlarmOutRL.bOutEMail;
	param->bOutVMS = ivp.st_rl_attr.stAlarmOutRL.bOutVMS;
	param->bNeedRestart = ivp.bNeedRestart;
	param->eCountOSDPos = ivp.st_count_attr.eCountOSDPos;
	param->nCountOSDColor = ivp.st_count_attr.nCountOSDColor;
	param->nCountSaveDays = ivp.st_count_attr.nCountSaveDays;
	return 0;
}
int sp_ivp_set_param(int channelid, SPIVP_t *param)
{
	MIVP_t ivp;
	
	mivp_get_param(channelid, &ivp);
	ivp.st_rl_attr.bEnable = param->bEnable;
	ivp.st_rl_attr.stAlarmOutRL.nDelay = param->nDelay;
	ivp.st_rl_attr.stAlarmOutRL.bStarting = param->bStarting;
	ivp.st_rl_attr.nRgnCnt = param->nRgnCnt;
	memcpy(ivp.st_rl_attr.stRegion, param->stRegion, sizeof(ivp.st_rl_attr.stRegion));
	ivp.st_rl_attr.bDrawFrame = param->bDrawFrame;
	ivp.st_rl_attr.bFlushFrame = param->bFlushFrame;
	ivp.st_rl_attr.bMarkObject = param->bMarkObject;
	ivp.st_rl_attr.bMarkAll = param->bMarkAll;
	ivp.st_count_attr.bOpenCount = param->bOpenCount;
	ivp.st_count_attr.bShowCount = param->bShowCount;
	ivp.bPlateSnap = param->bPlateSnap;
	strcpy(ivp.sSnapRes, param->sSnapRes);
	ivp.st_rl_attr.nAlpha = param->nAlpha;
	ivp.st_rl_attr.nSen = param->nSen;
	ivp.st_rl_attr.nThreshold = param->nThreshold;
	ivp.st_rl_attr.nStayTime = param->nStayTime;
	ivp.st_rl_attr.stAlarmOutRL.bEnableRecord = param->bEnableRecord;
	ivp.st_rl_attr.stAlarmOutRL.bOutAlarm1 = param->bOutAlarm1;
	ivp.st_rl_attr.stAlarmOutRL.bOutClient = param->bOutClient;
	ivp.st_rl_attr.stAlarmOutRL.bOutEMail = param->bOutEMail;
	ivp.st_rl_attr.stAlarmOutRL.bOutVMS = param->bOutVMS;
	ivp.bNeedRestart = param->bNeedRestart;
	ivp.st_count_attr.eCountOSDPos = param->eCountOSDPos;
	ivp.st_count_attr.nCountOSDColor = param->nCountOSDColor;
	ivp.st_count_attr.nCountSaveDays = param->nCountSaveDays;
	mivp_set_param(channelid, &ivp);
	WriteConfigInfo();
	mivp_restart(channelid);
	return 0;
}
