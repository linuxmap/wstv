#include "jv_common.h"

#include "msensor.h"
#include "mstream.h"
#include "mtransmit.h"
#include "mrecord.h"
#include "msnapshot.h"
#include "mprivacy.h"
#include "mdetect.h"
#include "malarmout.h"
#include "malarmin.h"
#include "mipcinfo.h"
#include "mosd.h"
#include "sctrl.h"
#include <libPTZ.h>
#include <mptz.h>
#include "SYSFuncs.h"
#include <mlog.h>
#include <termios.h>
#include <jv_ai.h>
#include <jv_ao.h>
#include <utl_filecfg.h>
#include <utl_ifconfig.h>
#include "msoftptz.h"
#include "mivp.h"
#include "mcloud.h"
#include "mgb28181.h"
#include "maudio.h"
#include "mvoicedec.h"
#include "mdooralarm.h"
#include "JvServer.h"
#include <pthread.h>
/**
 *@brief 从给定字符串中提取key和value对。
 *处理字符串data，从中取出第一个key和value并存入key和value
 *@param data 要解析的字符串
 *@param key 保存提取到的KEY串
 *@param value 保存提取到的value串
 *@param maxlen key和value能容忍的最大字符串长度
 *@return 处理完字符串后，指针指向的位置
 *
 */
char *sysfunc_get_key_value(char *data, char *key, char *value, int maxlen)
{
	int i = 0;
	char *temp = data;
	while(*temp && *temp == ';')
		temp++;
	while(*temp && *temp != ';' && *temp != '=' 
		&& i < maxlen)
	{
		key[i++] = *temp++;
	}
	key[i] = '\0';
	if (*temp == ';' || *temp == '\0')
		value[0] = '\0';
	else if(*temp == '=')
	{
		i = 0;
		temp++;
		while(*temp && *temp != ';'
			&& i < maxlen)
		{
			value[i++] = *temp++;
		}
		value[i] = 0;
	}
	return temp;
}

//返回参数长度
U32 ParseParam(char *pParam, int nMaxLen, char *pBuffer)
{
	int nLen	= 0;
	while(pBuffer && *pBuffer && *pBuffer != ';')
	{
		*pParam++ = *pBuffer++;
		nLen++;
	}
	return nLen;
}

//从acData中，查找key的值，保存在value中
static char *__get_value(char *acData, char *key, char *value, int nMaxLen)
{
	char *pItem, *pValue;
	U32 nRead;
	char acBuff[256]={0};
	U32 nOffset = 0;
	int nCh = 0;
	while ((nRead = ParseParam(acBuff, 64, acData+nOffset)) > 0)
	{
		acBuff[nRead]	= 0;	//把分号去掉
		nOffset += (nRead+1);
		if (strncmp(acBuff, "[CH1]", 5) == 0)
		{
			nCh = 0;
		}
		else if (strncmp(acBuff, "[CH2]", 5) == 0)
		{
			nCh = 1;
		}
		else if (strncmp(acBuff, "[CH3]", 5) == 0)
		{
			nCh = 2;
		}
		else if (strncmp(acBuff, "[ALL]", 5) == 0)
		{
			nCh = 0;
		}
		else
		{
			pItem = strtok(acBuff, "=");
			pValue = strtok(NULL, "\r");
			//Printf("pItem: %s, pValue: %s\n", pItem, pValue);
			if (pValue == NULL)
				pValue = "";

			if (strcmp(pItem, key) == 0)
			{
				strncpy(value, pValue, nMaxLen);
				return value;
			}
		}
	}
	value[0] = '\0';
	return value;
}


static MD s_md;
static REGION s_region;
static msensor_attr_t s_stSensorAttr[MAX_SENSOR];
static mstream_attr_t s_stAttr[MAX_STREAM];
static mstream_roi_t s_stRoi;
static mrecord_attr_t s_recAttr[MAX_REC_TASK];
static ALARMSET s_alarm;
static ipcinfo_t s_ipcinfo;
static mchnosd_attr s_osd[MAX_STREAM];
static YST s_stYst;
static SCTRLINFO s_sctrl;
static jv_audio_attr_t s_AiAttr;
static MAlarmIn_t s_alarmin;
static MLE_t	stLocalexposure;
static MIVP_t s_mivp;
#ifdef  XW_MMVA_SUPPORT
#define  s_mivp_count  s_mivp
#else
static MIVP_t s_mivp_count;
#endif
static CLOUD s_cloud;

 /* 初始化读写锁 */
static pthread_rwlock_t  g_reboot_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * 单独的配置文件，保存语言、云视通号等，恢复出厂设置都不会影响的信息
 */
static int __read_important_cfg(ImportantCfg_t *cfg)
{
	U32	i = 0, nCh=0, nRead=0;
	char	acData[4096]={0}, acBuff[256]={0};
	S32 fdCfg;
	int j;

	//打开配置文件，设置系统信息
	fdCfg = open(IMPORTANT_CFG_FILE, O_RDONLY);
	if(fdCfg < 0)
	{
		Printf("Read config file %s failed.\n", CONFIG_FILE);
		return -1;
	}
	while((nRead=read(fdCfg, acData, 4096)) > 0)
	{
	}
	close(fdCfg);

	memset(cfg, 0, sizeof(ImportantCfg_t));
	cfg->nLanguage = atoi(__get_value(acData, "nLanguage", acBuff, sizeof(acBuff)));
	cfg->nTimeFormat = atoi(__get_value(acData, "nTimeFormat", acBuff, sizeof(acBuff)));
	cfg->ystID = atoi(__get_value(acData, "YSTID", acBuff, sizeof(acBuff)));
	return 0;
}

static void __write_important_cfg(ImportantCfg_t *cfg)
{
	FILE *fp;
	ImportantCfg_t oldCfg;
	__read_important_cfg(&oldCfg);
	if (0 == memcmp(&oldCfg, cfg, sizeof(oldCfg)))
	{
		//不需要写入
		return ;
	}
	pthread_rwlock_rdlock(&g_reboot_rwlock);
	fp = fopen(IMPORTANT_CFG_FILE, "wb");
	if (fp)
	{
		fprintf(fp, "nLanguage=%d;", cfg->nLanguage);
		fprintf(fp, "nTimeFormat=%d;", cfg->nTimeFormat);
		fprintf(fp, "YSTID=%d;", cfg->ystID);
		fclose(fp);
	}
	else
	{
		printf("ERROR: Failed open important cfg to write: %s\n", strerror(errno));
	}
	pthread_rwlock_unlock(&g_reboot_rwlock);

}

#define NGOP_FRAMERATE  4 //ngop与frame的倍数关系

static void __read_config_file(char *cfgName)
{
	char *pItem, *pValue;
	U32	i = 0, nCh=0, nRead=0, nOffset=0;
	char acData[16*1024]={0}, acBuff[256]={0};
	S32 fdCfg;
	int j;
	int ibuf[3];
	PTZ *pPtz = PTZ_GetInfo();
	PTZ_PRESET_INFO *preInfo = PTZ_GetPreset();
	PTZ_PATROL_INFO *patrol = PTZ_GetPatrol();
	PTZ_GUARD_T *guard = PTZ_GetGuard();
	PTZ_SCHEDULE_INFO *schedule = PTZ_GetSchedule();
	
	//打开配置文件，设置系统信息
	fdCfg = open(cfgName, O_RDONLY);
	if(fdCfg < 0)
	{
		Printf("Read config file %s failed.\n", cfgName);
		return ;
	}
	while((nRead=read(fdCfg, acData+nOffset, 4096)) > 0)
	{
		nOffset += nRead;
	}
	close(fdCfg);
	nOffset = 0;
	
	//memset(&s_stSensorAttr[0],0,sizeof(msensor_attr_t));
	
	Printf("__read_config_file:\nconfigInfo: %s\n", acData);

	while ((nRead = ParseParam(acBuff, 64, acData+nOffset)) > 0)
	{
		acBuff[nRead]	= 0;	//把分号去掉
		nOffset += (nRead+1);
		if (strncmp(acBuff, "[CH1]", 5) == 0)
		{
			nCh = 0;
		}
		else if (strncmp(acBuff, "[CH2]", 5) == 0)
		{
			nCh = 1;
		}
		else if (strncmp(acBuff, "[CH3]", 5) == 0)
		{
			nCh = 2;
		}
		else if (strncmp(acBuff, "[ALL]", 5) == 0)
		{
			nCh = 0;
		}
		else
		{
			pItem = strtok(acBuff, "=");
			pValue = strtok(NULL, "\r");
			//Printf("pItem: %s, pValue: %s\n", pItem, pValue);
			if (pValue == NULL)
				pValue = "";

			//设备号
			if (strncmp(pItem, "YSTID", 5) == 0)
			{
				s_stYst.nID = atoi(pValue);
				Printf("YSTID=%d\n", s_stYst.nID);
				s_ipcinfo.ystID = s_stYst.nID;
			}
			else if (strcmp(pItem, "nTimeFormat") == 0)
			{
				gp.nTimeFormat = atoi(pValue);
			}
			else if(strncmp(pItem, "DevName", 7) == 0)
			{
				sprintf(s_ipcinfo.acDevName, "%s", pValue);
				if(hwinfo.bHomeIPC==1&&ipcinfo_get_type2()!= 1)
				{
					char*m_acDevName=	"此机器工作异常，请联系返修";
					strcpy(s_ipcinfo.acDevName, m_acDevName);
				}
			}
			else if(strncmp(pItem, "nickName", 8) == 0)
			{
				sprintf(s_ipcinfo.nickName, "%s", pValue);
			}
			else if(strncmp(pItem, "ProductType", 11) == 0)
			{
				if(strlen(pValue)>0 && 0==access(RESTRICTION_FILE, F_OK))
					sprintf(s_ipcinfo.type, "%s", pValue);
				else if(strlen(pValue) == 0 && 0 == access(RESTRICTION_FILE, F_OK))
				{
					int fd = open(RESTRICTION_FILE, O_RDONLY);
					if(fd != -1)
					{
						char buf[32];
						int bytes = read(fd, buf, 32);
						if(bytes > 0)
							sprintf(s_ipcinfo.type, "%s", buf);
						else
							sprintf(s_ipcinfo.type, "%s", hwinfo.type);
						close(fd);
					}
				}
				else
					sprintf(s_ipcinfo.type, "%s", hwinfo.type);
			}
			else if(strncmp(pItem, "nLanguage", 9) == 0)
			{//语言
				s_ipcinfo.nLanguage = atoi(pValue);
			}
			else if(strcmp(pItem, "YSTPort") == 0)
			{
				s_stYst.nPort = atoi(pValue);
			}
			else if(strcmp(pItem, "bSntp") == 0)
			{
				s_ipcinfo.bSntp = atoi(pValue);
			}
			else if(strcmp(pItem, "sntpInterval") == 0)
			{
				s_ipcinfo.sntpInterval = atoi(pValue);
			}
			else if(strncmp(pItem, "ntpServer", 9) == 0)
			{
				strncpy(s_ipcinfo.ntpServer, pValue, sizeof(s_ipcinfo.ntpServer));
			}
			else if(strncmp(pItem, "tutkid", 6) == 0)
			{
				if(strcmp(pValue," ") == 0 || pValue[0] == '\0')
					memset(s_ipcinfo.tutkid,0,MAX_TUTK_UID_NUM);
				else
					strncpy(s_ipcinfo.tutkid, pValue, sizeof(s_ipcinfo.tutkid));
			}
			else if(strcmp(pItem, "rebootHour") == 0)
			{
				s_ipcinfo.rebootHour = atoi(pValue);
			}
			else if(strcmp(pItem, "rebootDay") == 0)
			{
				s_ipcinfo.rebootDay = atoi(pValue);
			}
			else if(strcmp(pItem, "LedControl") == 0)
			{
				s_ipcinfo.LedControl = atoi(pValue);
			}
			else if(strcmp(pItem, "timezone") == 0)
			{
				s_ipcinfo.tz = atoi(pValue);
			}
			else if(strcmp(pItem, "osdText0") == 0)
			{
				sprintf(s_ipcinfo.osdText[0], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText1") == 0)
			{
				sprintf(s_ipcinfo.osdText[1], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText2") == 0)
			{
				sprintf(s_ipcinfo.osdText[2], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText3") == 0)
			{
				sprintf(s_ipcinfo.osdText[3], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText4") == 0)
			{
				sprintf(s_ipcinfo.osdText[4], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText5") == 0)
			{
				sprintf(s_ipcinfo.osdText[5], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText6") == 0)
			{
				sprintf(s_ipcinfo.osdText[6], "%s", pValue);
			}
			else if(strcmp(pItem, "osdText7") == 0)
			{
				sprintf(s_ipcinfo.osdText[7], "%s", pValue);
			}
			else if(strcmp(pItem, "osdX") == 0)
			{
				s_ipcinfo.osdX = atoi(pValue);
			}
			else if(strcmp(pItem, "osdY") == 0)
			{
				s_ipcinfo.osdY = atoi(pValue);
			}
			else if(strcmp(pItem, "endX") == 0)
			{
				s_ipcinfo.endX = atoi(pValue);
			}
			else if(strcmp(pItem, "endY") == 0)
			{
				s_ipcinfo.endY = atoi(pValue);
			}
			else if(strcmp(pItem, "osdSize") == 0)
			{
				s_ipcinfo.osdSize = atoi(pValue);
			}
			else if(strcmp(pItem,"multiPosition") == 0)
			{
				s_ipcinfo.osdPosition = atoi(pValue);
			}
			else if(strcmp(pItem,"alignment") == 0)
			{
				s_ipcinfo.osdAlignment= atoi(pValue);
			}
			else if(strncmp(pItem, "lcmsServer", 10) == 0)
			{
				sprintf(s_ipcinfo.lcmsServer, "%s", pValue);
			}
			//sensor参数
			else if(strcmp(pItem, "bAEME") == 0)
			{
				s_stSensorAttr[0].ae.bAEME = atoi(pValue);
			}
			else if(strcmp(pItem, "bByPassAE") == 0)
			{
				s_stSensorAttr[0].ae.bByPassAE = atoi(pValue);
			}
			else if(strcmp(pItem, "exposureMax") == 0)
			{
				s_stSensorAttr[0].ae.exposureMax= atoi(pValue);
			}
			else if(strcmp(pItem, "exposureMin") == 0)
			{
				s_stSensorAttr[0].ae.exposureMin= atoi(pValue);
			}
			else if(strcmp(pItem, "u16DGainMax") == 0)
			{
				s_stSensorAttr[0].ae.u16DGainMax = atoi(pValue);
			}
			else if(strcmp(pItem, "u16DGainMin") == 0)
			{
				s_stSensorAttr[0].ae.u16DGainMin = atoi(pValue);
			}
			else if(strcmp(pItem, "u16AGainMax") == 0)
			{
				s_stSensorAttr[0].ae.u16AGainMax = atoi(pValue);
			}
			else if(strcmp(pItem, "u16AGainMin") == 0)
			{
				s_stSensorAttr[0].ae.u16AGainMin = atoi(pValue);
			}
			else if(strcmp(pItem, "u32ISPDGainMax") == 0)
			{
				s_stSensorAttr[0].ae.u32ISPDGainMax = atoi(pValue);
			}
			else if(strcmp(pItem, "u32SystemGainMax") == 0)
			{
				s_stSensorAttr[0].ae.u32SystemGainMax = atoi(pValue);
			}
			else if(strcmp(pItem, "u32ISPDGain") == 0)
			{
				s_stSensorAttr[0].ae.u32ISPDGain = atoi(pValue);
			}
			else if(strcmp(pItem, "u32ExpLine") == 0)
			{
				s_stSensorAttr[0].ae.u32ExpLine = atoi(pValue);
			}
			else if(strcmp(pItem, "s32DGain") == 0)
			{
				s_stSensorAttr[0].ae.s32DGain= atoi(pValue);
			}
			else if(strcmp(pItem, "s32AGain") == 0)
			{
				s_stSensorAttr[0].ae.s32AGain= atoi(pValue);
			}
			else if(strcmp(pItem, "bManualAGainEnable") == 0)
			{
				s_stSensorAttr[0].ae.bManualAGainEnable= atoi(pValue);
			}
			else if(strcmp(pItem, "bManualDGainEnable") == 0)
			{
				s_stSensorAttr[0].ae.bManualDGainEnable= atoi(pValue);
			}
			else if(strcmp(pItem, "bManualExpLineEnable") == 0)
			{
				s_stSensorAttr[0].ae.bManualExpLineEnable = atoi(pValue);
			}
			else if(strcmp(pItem, "bManualISPGainEnable") == 0)
			{
				s_stSensorAttr[0].ae.bManualISPGainEnable = atoi(pValue);
			}
			else if(strcmp(pItem, "bDRCEnable") == 0)
			{
				s_stSensorAttr[0].drc.bDRCEnable= atoi(pValue);
			}
			else if(strncmp(pItem, "brightness", 10) == 0)
			{
				s_stSensorAttr[0].brightness = atoi(pValue);
			}
			else if(strncmp(pItem, "exposure", 8) == 0)
			{
				s_stSensorAttr[0].exposure = atoi(pValue);
			}
			else if(strncmp(pItem, "saturation", 10) == 0)
			{
				s_stSensorAttr[0].saturation = atoi(pValue);
			}
            else if(strcmp(pItem, "contrast") == 0)
            {
                s_stSensorAttr[0].contrast= atoi(pValue);
            }
            else if(strcmp(pItem, "sharpness") == 0)
            {
                s_stSensorAttr[0].sharpness= atoi(pValue);
            }
            else if(strcmp(pItem, "antifog") == 0)
            {
                s_stSensorAttr[0].antifog= atoi(pValue);
            }
			else if(strcmp(pItem, "light") == 0)
            {
                s_stSensorAttr[0].light= atoi(pValue);
            }
            else if(strcmp(pItem, "bSupportWDr") == 0)
            {
                s_stSensorAttr[0].bSupportWdr= atoi(pValue);
            }
            else if(strcmp(pItem, "bEnableWdr") == 0)
            {
                s_stSensorAttr[0].bEnableWdr= atoi(pValue);
            }
			else if(strcmp(pItem, "bSupportSl") == 0)
            {
                s_stSensorAttr[0].bSupportSl= atoi(pValue);
            }
            else if(strcmp(pItem, "bEnableSl") == 0)
            {
                s_stSensorAttr[0].bEnableSl= atoi(pValue);
            }
            else if(strcmp(pItem, "daynightMode") == 0)
            {
                s_stSensorAttr[0].daynightMode= atoi(pValue);
            }
            else if(strcmp(pItem, "bRedWhiteCtrlEnabled") == 0)
            {
				msensor_set_whitelight_function(atoi(pValue));
            }
            else if(strcmp(pItem, "dayStart") == 0)
            {
            	int hour, minute;
            	sscanf(pValue, "%d:%d", &hour, &minute);
            	s_stSensorAttr[0].dayStart.hour = hour;
            	s_stSensorAttr[0].dayStart.minute = minute;
            }
            else if(strcmp(pItem, "dayEnd") == 0)
            {
            	int hour, minute;
            	sscanf(pValue, "%d:%d", &hour, &minute);
            	s_stSensorAttr[0].dayEnd.hour = hour;
            	s_stSensorAttr[0].dayEnd.minute = minute;
            }
            else if(strcmp(pItem, "effect_flag") == 0)
            {
                s_stSensorAttr[0].effect_flag=atoi(pValue);
            }
			else if(strncmp(pItem, "AutoLowFrameEn",14) == 0)
            {
            	s_stSensorAttr[i].AutoLowFrameEn = atoi(pValue);
            }
            else if(strcmp(pItem,"sw_cut")==0)
            {
                s_stSensorAttr[0].sw_cut=atoi(pValue);
            }
            else if(strcmp(pItem,"cut_rate")==0)
            {
                s_stSensorAttr[0].cut_rate = atoi(pValue);
            }
            else if(strcmp(pItem,"sence")==0)
            {
                s_stSensorAttr[0].sence=atoi(pValue);
            }
            else if(strcmp(pItem,"cutDelay")==0)
            {
                s_stSensorAttr[0].cutDelay=atoi(pValue);
            }
            else if(strcmp(pItem,"rotate")==0)
            {
                s_stSensorAttr[0].rotate=atoi(pValue);
            }
			else if(strcmp(pItem,"exp_mode")==0)
            {
                s_stSensorAttr[0].exp_mode=atoi(pValue);
            }
			//add for fisheye
			else if (strcmp(pItem, "fishEyeView") == 0)
			{
			}
			else if (strcmp(pItem, "fishEyeCheckMode") == 0)
			{
			}
            else if (strncmp(pItem, "LEReverse", 9) == 0)
			{
            	stLocalexposure.bLE_Reverse = atoi(pValue);
			}
			else if (strncmp(pItem, "LEWeight", 8) == 0)
			{
				stLocalexposure.nLE_Weight = atoi(pValue);
			}
			else if(strncmp(pItem, "LERegion", 8) == 0)
			{
				RECT *pRect = NULL;
				U32 nIndex = 0;			//第几个区域
				sscanf(pItem, "LERegion%d", &nIndex);
				pRect = &stLocalexposure.stLE_Rect[nIndex];
				sscanf(pValue, "%d,%d,%d,%d", &pRect->x, &pRect->y, &pRect->w,
						&pRect->h);
			}

			//
			else if(strcmp(pItem, "bShowOSD") == 0)
			{
				s_osd[nCh].bShowOSD = atoi(pValue);
			}
			else if(strcmp(pItem, "nPosition") == 0)
			{
				s_osd[nCh].position = atoi(pValue);
			}
			else if(strcmp(pItem, "nTimePosition") == 0)
			{
				s_osd[nCh].timePos = atoi(pValue);
			}
			else if(strcmp(pItem, "bLargeOSD") == 0)
			{
				s_osd[nCh].bLargeOSD = atoi(pValue);
			}
			else if(strcmp(pItem,"nOSDbInvColEn")==0)
			{
				s_osd[nCh].osdbInvColEn = atoi(pValue);
			}
			else if(strcmp(pItem, "timeFormat") == 0)
			{
				strcpy(s_osd[nCh].timeFormat, pValue);
			}
			else if(strcmp(pItem, "channelName") == 0)
			{
				strcpy(s_osd[nCh].channelName, pValue);
				if(hwinfo.bHomeIPC==1&&ipcinfo_get_type2()!= 1)
				{
					char*m_channelName=	"此机器工作异常，请联系返修";
					strcpy(s_osd[nCh].channelName, m_channelName);
				}
			}
			else if(strncmp(pItem, "bAudioEn", 8) == 0)
			{
				s_stAttr[nCh].bAudioEn = atoi(pValue);
			}
			else if(strncmp(pItem, "width", 5) == 0)
			{
				s_stAttr[nCh].width = atoi(pValue);
			}
			else if(strncmp(pItem, "height", 6) == 0)
			{
				s_stAttr[nCh].height = atoi(pValue);
			}
			else if(strcmp(pItem, "framerate") == 0)
			{
				s_stAttr[nCh].framerate = atoi(pValue);
				#if (!SUPPORT_3RD_1080P)
				if(s_stAttr[nCh].framerate > 30)
					HWINFO_STREAM_CNT = 2;
				#endif
			}
			else if(strcmp(pItem, "nGOP_S") == 0)
			{
				s_stAttr[nCh].nGOP_S = atoi(pValue);
			}
			else if(strcmp(pItem, "bRectStretch") == 0)
			{
				s_stAttr[nCh].bRectStretch = atoi(pValue);
			}
			else if(strcmp(pItem, "vencType") == 0)
			{
				s_stAttr[nCh].vencType = atoi(pValue);
			}
			else if(strcmp(pItem,"bLDCEnable") == 0)
			{
				s_stAttr[nCh].bLDCEnable = atoi(pValue);
			}
			else if(strcmp(pItem,"ldcRatio") == 0)
			{
				s_stAttr[nCh].nLDCRatio = atoi(pValue);
			}
			else if(strncmp(pItem, "bitrate", 5) == 0)
			{
				s_stAttr[nCh].bitrate = atoi(pValue);
			}
			else if(strncmp(pItem, "rcMode", 5) == 0)
			{
				s_stAttr[nCh].rcMode = atoi(pValue);
			}
			else if(strncmp(pItem, "minQP", 5) == 0)
			{
				s_stAttr[nCh].minQP = atoi(pValue);
			}
			else if(strncmp(pItem, "maxQP", 5) == 0)
			{
				s_stAttr[nCh].maxQP = atoi(pValue);
			}
			else if(strncmp(pItem, "roiRegion", 9) == 0)
			{
				RECT *pRect = NULL;
				U32 nIndex=0;			//第几个区域
				sscanf(pItem, "roiRegion%d", &nIndex);
				pRect = &s_stRoi.roi[nIndex];
				sscanf(pValue, "%d,%d,%d,%d", &pRect->x, &pRect->y, &pRect->w, &pRect->h);
			}

			else if (strncmp(pItem, "roiReverse", 10) == 0)
			{
				s_stRoi.ior_reverse = atoi(pValue);
			}
			else if (strncmp(pItem, "roiWeight", 9) == 0)
			{
				s_stRoi.roiWeight = atoi(pValue);
			}


			else if(strcmp(pItem, "encType") == 0)
			{
				s_AiAttr.encType = atoi(pValue);
			}

			//audio部分读取新增参数的配置信息
			else if(strcmp(pItem, "level") == 0)
			{
				s_AiAttr.level = atoi(pValue);
			}
			else if(strcmp(pItem, "micGain") == 0)
			{
				s_AiAttr.micGain = atoi(pValue);
			}
			
			//区域遮挡
			else if(strncmp(pItem, "bCoverRgn", 9) == 0)
			{
				s_region.bEnable	= atoi(pValue);
			}
			else if(strncmp(pItem, "Region", 6) == 0)
			{
				RECT *pRect = NULL;
				U32 nIndex=0;			//第几个区域
				sscanf(pItem, "Region%d", &nIndex);
				pRect = &s_region.stRect[nIndex];
				sscanf(pValue, "%d,%d,%d,%d", &pRect->x, &pRect->y, &pRect->w, &pRect->h);
			}
			//移动检测
			else if(strncmp(pItem, "bMDEnable", 9) == 0)
			{
				s_md.bEnable = atoi(pValue);
			}
			else if(strncmp(pItem, "nMDSensitivity", 12) == 0)
			{
				s_md.nSensitivity = atoi(pValue);
				//根据灵敏度计算阈值等
				s_md.nThreshold = (100-s_md.nSensitivity)/5+5;
				//md.nRatio 	= (100-md.nSensitivity)/5+5;
				//...
			}
			else if(strncmp(pItem,"detect_bEnRecord",16) == 0)
			{
				s_md.bEnableRecord = atoi(pValue);
			}
			else if(strncmp(pItem, "nMDDelay", 8) == 0)
			{
				s_md.nDelay = atoi(pValue);
			}
			else if(strncmp(pItem, "MDRegion", 8) == 0)
			{
				RECT *pRect = NULL;
				U32 nIndex=0;			//第几个区域
				sscanf(pItem, "MDRegion%d", &nIndex);
				pRect = &s_md.stRect[nIndex];
				sscanf(pValue, "%d,%d,%d,%d", &pRect->x, &pRect->y, &pRect->w, &pRect->h);
			}
			else if(strncmp(pItem, "nMDOutClient", 12) == 0)
			{
				s_md.bOutClient = atoi(pValue);
			}
			else if(strncmp(pItem, "nMDOutEMail", 11) == 0)
			{
				s_md.bOutEMail = atoi(pValue);
			}
			else if(strncmp(pItem, "nMDOutVMS", 9) == 0)
			{
				s_md.bOutVMS= atoi(pValue);
			}
			else if(strncmp(pItem, "nMDOutBuzzing", 13) == 0)
			{
				s_md.bBuzzing = atoi(pValue);
			}
			else if(strncmp(pItem, "MovGridW", 8) == 0)
			{
				s_md.nColumn = atoi(pValue);
			}
			else if(strncmp(pItem, "MovGridH", 8) == 0)
			{
				s_md.nRow = atoi(pValue);
			}
			else if(strncmp(pItem, "MovGrid", 7) == 0)
			{
				int iter = 0;
				for (; iter < s_md.nRow; iter++)
				{
					sscanf(pValue + 12 * iter, "%11d,", &s_md.nRegion[iter]);
				}
			}
			else if(strcmp(pItem, "bRecEnable") == 0)
			{
				s_recAttr[nCh].bEnable = atoi(pValue);
			}
			else if(strcmp(pItem, "RecFileLength") == 0)
			{
				s_recAttr[nCh].file_length = atoi(pValue);
			}
			else if(strcmp(pItem, "bRecTimingEnable") == 0)
			{
				s_recAttr[nCh].timing_enable = atoi(pValue);
			}
			else if(strcmp(pItem, "bRecDisconEnable") == 0)
			{
				s_recAttr[nCh].discon_enable = atoi(pValue);
			}
			else if(strcmp(pItem, "bRecAlarmEnable") == 0)
			{
				s_recAttr[nCh].alarm_enable = atoi(pValue);
			}
			else if(strcmp(pItem, "RecTimingStart") == 0)
			{
				s_recAttr[nCh].timing_start = atoi(pValue);
			}
			else if(strcmp(pItem, "RecTimingStop") == 0)
			{
				s_recAttr[nCh].timing_stop = atoi(pValue);
			}
			else if(strcmp(pItem, "detecting") == 0)
			{
				s_recAttr[nCh].detecting = atoi(pValue);
			}
			else if(strcmp(pItem, "alarming") == 0)
			{
				s_recAttr[nCh].alarming = atoi(pValue);
			}
			else if(strcmp(pItem, "ivping") == 0)
			{
				s_recAttr[nCh].ivping= atoi(pValue);
			}
			else if(strcmp(pItem,"bRecChFrameEnable") == 0)
			{
				s_recAttr[nCh].chFrame_enable = atoi(pValue);
			}
			else if(strcmp(pItem,"chFrameSec") == 0)
			{
				s_recAttr[nCh].chFrameSec = atoi(pValue);
			}
			else if(strcmp(pItem, "alarm_pre_record") == 0)
			{
				s_recAttr[nCh].alarm_pre_record = atoi(pValue);
			}
			else if(strcmp(pItem, "alarm_duration") == 0)
			{
				s_recAttr[nCh].alarm_duration = atoi(pValue);
			}
			//web服务
			else if (strncmp(pItem, "WebServer", 9) == 0)
			{
				s_ipcinfo.bWebServer = atoi(pValue);
			}
			else if (strncmp(pItem, "WebPort", 7) == 0)
			{
				s_ipcinfo.nWebPort = atoi(pValue);
			}
			else if (strncmp(pItem, "PictureType", 11) == 0)
			{
				s_sctrl.nPictureType = atoi(pValue);
			}
            //gyd20120323 //添加报警设置部分
           	else if(strncmp(pItem,"nAlarmDelay",11)==0)
			{//报警延时
				s_alarm.delay=atoi(pValue);
			}
			else if(strncmp(pItem,"acMailSender",12)==0)
			{//发件人
				sprintf(s_alarm.sender, "%s", pValue);
			}
			else if(strncmp(pItem,"acSMTPServer",12)==0)
			{//服务器
				sprintf(s_alarm.server, "%s", pValue);
			}
			else if(strncmp(pItem,"acSMTPUser",10)==0)
			{//用户名
				sprintf(s_alarm.username, "%s", pValue);
			}
			else if (strncmp(pItem,"acSMTPPasswd",12)==0)
			{//密码
				sprintf(s_alarm.passwd, "%s", pValue);
			}
			else if (strncmp(pItem,"acReceiver0",11)==0)
			{//收件人1
				sprintf(s_alarm.receiver0, "%s", pValue);
			}
			else if (strncmp(pItem,"acReceiver1",11)==0)
			{//收件人2
				sprintf(s_alarm.receiver1, "%s", pValue);
			}
			else if (strncmp(pItem,"acReceiver2",11)==0)
			{//收件人3
				sprintf(s_alarm.receiver2, "%s", pValue);
			}
			else if (strncmp(pItem,"acReceiver3",11)==0)
			{//收件人4
				sprintf(s_alarm.receiver3, "%s", pValue);
			}
			//add by xianlt at 20120628
			else if (strncmp(pItem,"acSMTPPort",11)==0)
			{//收件人3
				s_alarm.port=atoi(pValue);
			}
			else if (strncmp(pItem,"acSMTPCrypto",11)==0)
			{//收件人4
				sprintf(s_alarm.crypto, "%s", pValue);
			}
			//add by xianlt at 20120628
			else if (strncmp(pItem, "bAlarmEnable", 12) == 0)
			{//安全防护开关
				s_alarm.bEnable = atoi(pValue);
			}
			else if(strncmp(pItem,"bAlarmSound",11) == 0)
			{
				s_alarm.bAlarmSoundEnable = atoi(pValue);
			}
			else if(strncmp(pItem,"bAlarmLight",11) == 0)
			{
				s_alarm.bAlarmLightEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "vmsServerIp", 11) == 0)
			{//VMS服务器IP
				sprintf(s_alarm.vmsServerIp, "%s", pValue);
			}
			else if (strncmp(pItem, "vmsServerPort", 13) == 0)
			{//VMS服务器端口
				s_alarm.vmsServerPort = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmTime0", 10) == 0)
			{//安全防护时间段1
				if(pValue)
				{
					char *tmp = strstr(pValue, "-");
					if(tmp)
					{
						*tmp = '\0';
						strcpy(s_alarm.alarmTime[0].tStart, pValue);
						strcpy(s_alarm.alarmTime[0].tEnd, tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "alarmTime1", 10) == 0)
			{//安全防护时间段2
				if(pValue)
				{
					char *tmp = strstr(pValue, "-");
					if(tmp)
					{
						*tmp = '\0';
						strcpy(s_alarm.alarmTime[1].tStart, pValue);
						strcpy(s_alarm.alarmTime[1].tEnd, tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "alarmTime2", 10) == 0)
			{//安全防护时间段3
				if(pValue)
				{
					char *tmp = strstr(pValue, "-");
					if(tmp)
					{
						*tmp = '\0';
						strcpy(s_alarm.alarmTime[2].tStart, pValue);
						strcpy(s_alarm.alarmTime[2].tEnd, tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "Schedule_bEn1", 13) == 0)
			{
				s_alarm.m_Schedule[0].bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_time1", 14) == 0)
			{
				if(pValue)
				{
					char *tmp = strstr(pValue, ":");
					if(tmp)
					{
					 	*tmp = '\0';
						s_alarm.m_Schedule[0].Schedule_time_H= atoi(pValue);
						s_alarm.m_Schedule[0].Schedule_time_M=atoi(tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "Schedule_num1", 13) == 0)
			{
				s_alarm.m_Schedule[0].num = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_interval1", 18) == 0)
			{
				s_alarm.m_Schedule[0].Interval = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_bEn2", 13) == 0)
			{
				s_alarm.m_Schedule[1].bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_time2", 14) == 0)
			{
				if(pValue)
				{
					char *tmp = strstr(pValue, ":");
					if(tmp)
					{
					 	*tmp = '\0';
						s_alarm.m_Schedule[1].Schedule_time_H= atoi(pValue);
						s_alarm.m_Schedule[1].Schedule_time_M=atoi(tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "Schedule_num2", 13) == 0)
			{
				s_alarm.m_Schedule[1].num = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_interval2", 18) == 0)
			{
				s_alarm.m_Schedule[1].Interval = atoi(pValue);
			}
				else if (strncmp(pItem, "Schedule_bEn3", 13) == 0)
			{
				s_alarm.m_Schedule[2].bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_time3", 14) == 0)
			{
				if(pValue)
				{
					char *tmp = strstr(pValue, ":");
					if(tmp)
					{
					 	*tmp = '\0';
						s_alarm.m_Schedule[2].Schedule_time_H= atoi(pValue);
						s_alarm.m_Schedule[2].Schedule_time_M=atoi(tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "Schedule_num3", 13) == 0)
			{
				s_alarm.m_Schedule[2].num = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_interval3", 18) == 0)
			{
				s_alarm.m_Schedule[2].Interval = atoi(pValue);
			}
				else if (strncmp(pItem, "Schedule_bEn4", 13) == 0)
			{
				s_alarm.m_Schedule[3].bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_time4", 14) == 0)
			{
				if(pValue)
				{
					char *tmp = strstr(pValue, ":");
					if(tmp)
					{
					 	*tmp = '\0';
						s_alarm.m_Schedule[3].Schedule_time_H= atoi(pValue);
						s_alarm.m_Schedule[3].Schedule_time_M=atoi(tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "Schedule_num4", 13) == 0)
			{
				s_alarm.m_Schedule[3].num = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_interval4", 18) == 0)
			{
				s_alarm.m_Schedule[3].Interval = atoi(pValue);
			}
				else if (strncmp(pItem, "Schedule_bEn5", 13) == 0)
			{
				s_alarm.m_Schedule[4].bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_time5", 14) == 0)
			{
				if(pValue)
				{
					char *tmp = strstr(pValue, ":");
					if(tmp)
					{
					 	*tmp = '\0';
						s_alarm.m_Schedule[4].Schedule_time_H= atoi(pValue);
						s_alarm.m_Schedule[4].Schedule_time_M=atoi(tmp+1);
					}
				}
			}
			else if (strncmp(pItem, "Schedule_num5", 13) == 0)
			{
				s_alarm.m_Schedule[4].num = atoi(pValue);
			}
			else if (strncmp(pItem, "Schedule_interval5", 18) == 0)
			{
				s_alarm.m_Schedule[4].Interval = atoi(pValue);
			}
	
			//云台,zwq20100513
			else if (strncmp(pItem, "nPTZAddr", 8) == 0)
			{
				pPtz[nCh].nAddr = atoi(pValue);
			}
			else if (strncmp(pItem, "nprotocol", 9) == 0)
			{
				pPtz[nCh].nProtocol = atoi(pValue);
			}
			else if (strncmp(pItem, "nBaud", 5) == 0)
			{
				pPtz[nCh].nBaudRate = atoi(pValue);
				pPtz[nCh].nHwParams.nBaudRate = pPtz[nCh].nBaudRate;
			}
			else if(strncmp(pItem, "nCharSize", 9) == 0)
			{
				pPtz[i].nHwParams.nCharSize	= atoi(pValue);
			}
			else if(strncmp(pItem, "nStopBit", 8) == 0)
			{
				pPtz[i].nHwParams.nStopBit	= atoi(pValue);
			}
			else if(strncmp(pItem, "nParityBit", 10) == 0)
			{
				pPtz[i].nHwParams.nParityBit	= atoi(pValue);
			}
			else if(strncmp(pItem, "nFlowCtl", 8) == 0)
			{
				pPtz[i].nHwParams.nFlowCtl	= atoi(pValue);
			}
			else if (strncmp(pItem, "bLRSW", 5) == 0)
			{
				pPtz[nCh].bLeftRightSwitch = atoi(pValue);
			}
			else if (strncmp(pItem, "bUDSW", 5) == 0)
			{
				pPtz[nCh].bUpDownSwitch = atoi(pValue);
			}
			else if (strncmp(pItem, "bIZSW", 5) == 0)
			{
				pPtz[nCh].bIrisZoomSwitch= atoi(pValue);
			}
			else if (strncmp(pItem, "bFZSW", 5) == 0)
			{
				pPtz[nCh].bFocusZoomSwitch = atoi(pValue);
			}
			else if (strncmp(pItem, "bZSW", 4) == 0)
			{
				pPtz[nCh].bZoomSwitch = atoi(pValue);
			}
			else if (strncmp(pItem, "scanSpeed", 9) == 0)
			{
				pPtz[nCh].scanSpeed = atoi(pValue);
			}
			else if (strncmp(pItem, "preset", 6) == 0)
			{
				j = preInfo[nCh].nPresetCt;
				sscanf(pValue, "%02x%s", &preInfo[nCh].pPreset[j], preInfo[nCh].name[j]);
				strcpy(preInfo[nCh].name[j], pValue+2);
				//printf("read:%s\n",preInfo[nCh].name[j]);
				preInfo[nCh].nPresetCt++;
			}
//			else if (strncmp(pItem, "lenpreset", 9) == 0)
//			{
//
//				sscanf(pValue, "%02x%04x%04x",&j,&ibuf[1],&ibuf[2]);
//				j--;
//				j=j>0?j:0;
//				preInfo[nCh].lenzoompos[j]=ibuf[1];
//				preInfo[nCh].lenfocuspos[j]=ibuf[2];
//			}
			else if (strncmp(pItem, "patrolSpeed", 11) == 0)
			{
				patrol[0].nPatrolSpeed = atoi(pValue);
			}
			else if (strncmp(pItem, "aPatrol", 7) == 0)
			{
				PATROL_NOD *pPatrol;
				pPatrol = patrol[0].aPatrol;	 //如果宏MAX_PATROL_NOD修改，此处也需要修改
				j = patrol[0].nPatrolSize;
				sscanf( pValue,"%02x%08x",&pPatrol[j].nPreset, &pPatrol[j].nStayTime);
				patrol[0].nPatrolSize++;
			}
			else if (strncmp(pItem, "2patrolSpeed", 12) == 0)
			{
				patrol[1].nPatrolSpeed = atoi(pValue);
			}
			else if (strncmp(pItem, "2aPatrol", 8) == 0)
			{
				PATROL_NOD *pPatrol;
				pPatrol = patrol[1].aPatrol;	 //如果宏MAX_PATROL_NOD修改，此处也需要修改
				j = patrol[1].nPatrolSize;
				sscanf( pValue,"%02x%08x",&pPatrol[j].nPreset, &pPatrol[j].nStayTime);
				patrol[1].nPatrolSize++;
			}
			else if (strncmp(pItem, "guardTime", 9) == 0)
			{
				guard[nCh].guardTime = atoi(pValue);
			}
			else if (strncmp(pItem, "guardType", 9) == 0)
			{
				guard[nCh].guardType = atoi(pValue);
			}
			else if (strncmp(pItem, "guardValue", 10) == 0)
			{
				guard[nCh].nRreset = atoi(pValue);
			}
			else if (strncmp(pItem, "bootItem", 8) == 0)
			{

				PTZ_SetBootConfigItem(nCh, atoi(pValue));				
			}
			else if (strncmp(pItem, "bSch1En", 7) == 0)
			{
				schedule[nCh].bSchEn[0] = atoi(pValue);
//				printf("bSch1En  %d\n", schedule[nCh].bSchEn[0]);
			}
			else if (strncmp(pItem, "bSch2En", 7) == 0)
			{
				schedule[nCh].bSchEn[1] = atoi(pValue);
//				printf("bSch2En  %d\n", schedule[nCh].bSchEn[1]);
			}
			else if (strncmp(pItem, "schedule1", 9) == 0)
			{
				schedule[nCh].schedule[0] = atoi(pValue);
				printf("schedule1  %d\n", schedule[nCh].schedule[0]);
			}
			else if (strncmp(pItem, "schedule2", 9) == 0)
			{
				schedule[nCh].schedule[1] = atoi(pValue);
				printf("schedule2  %d\n", schedule[nCh].schedule[1]);
			}
			else if (strncmp(pItem, "sch1TimeStart", 13) == 0)
			{
				int hour, minute;
            	sscanf(pValue, "%d:%d", &hour, &minute);
				schedule[nCh].schTimeStart[0].hour = hour;
				schedule[nCh].schTimeStart[0].minute = minute;	
				printf("sch1TimeStart  %d:%d\n", schedule[nCh].schTimeStart[0].hour,
					schedule[nCh].schTimeStart[0].minute);
			}
			else if (strncmp(pItem, "sch1TimeEnd", 11) == 0)
			{
				int hour, minute;
            	sscanf(pValue, "%d:%d", &hour, &minute);
				schedule[nCh].schTimeEnd[0].hour = hour;
				schedule[nCh].schTimeEnd[0].minute = minute;
				printf("sch1TimeEnd  %d:%d\n", schedule[nCh].schTimeEnd[0].hour,
					schedule[nCh].schTimeEnd[0].minute);
			}
			else if (strncmp(pItem, "sch2TimeStart", 13) == 0)
			{
				int hour, minute;
            	sscanf(pValue, "%d:%d", &hour, &minute);
				schedule[nCh].schTimeStart[1].hour = hour;
				schedule[nCh].schTimeStart[1].minute = minute;	
				printf("sch2TimeStart  %d:%d\n", schedule[nCh].schTimeStart[1].hour,
					schedule[nCh].schTimeStart[1].minute);
			}
			else if (strncmp(pItem, "sch2TimeEnd", 11) == 0)
			{
				int hour, minute;
            	sscanf(pValue, "%d:%d", &hour, &minute);
				schedule[nCh].schTimeEnd[1].hour = hour;
				schedule[nCh].schTimeEnd[1].minute = minute;
				printf("sch2TimeEnd  %d:%d\n", schedule[nCh].schTimeEnd[1].hour,
					schedule[nCh].schTimeEnd[1].minute);
			}
			//报警输入lk20131130
			else if (strncmp(pItem, "alarmin_bEnable", 15) == 0)
			{
				s_alarmin.bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_bNormallyClosed", 25) == 0)
			{
				s_alarmin.bNormallyClosed = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_bSendtoClient", 21) == 0)
			{
				s_alarmin.bSendtoClient = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_bSendtoVMS", 18) == 0)
			{
				s_alarmin.bSendtoVMS= atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_bSendEmail", 18) == 0)
			{
				s_alarmin.bSendEmail = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_bEnRecord", 17) == 0)
			{
				s_alarmin.bEnableRecord = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_u8Num", 13) == 0)
			{
				s_alarmin.u8AlarmNum = atoi(pValue);
			}
			else if (strncmp(pItem, "alarmin_bBuzzing", 17) == 0)
			{
				s_alarmin.bBuzzing = atoi(pValue);
			}
			/**************************************智能分析**********************************************************/

			if (strncmp(pItem, "IVPEnable", 9) == 0)
			{
				s_mivp.st_rl_attr.bEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPRgnCnt", 9) == 0)
			{
				s_mivp.st_rl_attr.nRgnCnt = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPRegion", 9) == 0)
			{
				int n = 0;
				char point[128];
				sscanf(pItem, "IVPRegion%d", &n);
				sscanf(pValue, "%d:%d:%s", &s_mivp.st_rl_attr.stRegion[n].nIvpCheckMode,&s_mivp.st_rl_attr.stRegion[n].nCnt, point);
				for (i = 0; i < s_mivp.st_rl_attr.stRegion[n].nCnt; i++)
				{
					if (i == s_mivp.st_rl_attr.stRegion[n].nCnt - 1)
						sscanf(point, "%d,%d", &s_mivp.st_rl_attr.stRegion[n].stPoints[i].x,&s_mivp.st_rl_attr.stRegion[n].stPoints[i].y);
					else
						sscanf(point, "%d,%d-%s",&s_mivp.st_rl_attr.stRegion[n].stPoints[i].x,&s_mivp.st_rl_attr.stRegion[n].stPoints[i].y, point);
				}
			}
			
			else if (strncmp(pItem, "nClimbPoints", 12) == 0)
			{
				MIVP_RL_t  sRlClimb;
				memcpy(&sRlClimb,&s_mivp.st_rl_attr,sizeof(MIVP_RL_t));
				int n=0;
				char point[128];
				sscanf(pValue, "%d:%s", &n,point);
				if( n != 2 )
					n=0;
				s_mivp.st_rl_attr.stClimb.nPoints = n;

				for(i=0;i<s_mivp.st_rl_attr.stClimb.nPoints;i++)
				{
					if(i==s_mivp.st_rl_attr.stClimb.nPoints-1)
						sscanf(point, "%d,%d", &s_mivp.st_rl_attr.stClimb.stPoints[i].x,&s_mivp.st_rl_attr.stClimb.stPoints[i].y);
					else
						sscanf(point, "%d,%d-%s", &s_mivp.st_rl_attr.stClimb.stPoints[i].x,&s_mivp.st_rl_attr.stClimb.stPoints[i].y,point);
				}
			}
			else if (strncmp(pItem, "bDrawFrame", 10) == 0)
			{
				s_mivp.st_rl_attr.bDrawFrame = atoi(pValue);
			}
			else if (strncmp(pItem, "bFlushFrame", 11) == 0)
			{
				s_mivp.st_rl_attr.bFlushFrame = atoi(pValue);
			}
			else if (strncmp(pItem, "bMarkAll", 8) == 0)
			{
				s_mivp.st_rl_attr.bMarkAll = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPAlpha", 8) == 0)
			{
				s_mivp.st_rl_attr.nAlpha = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPRecEnable", 12) == 0)
			{
				s_mivp.st_rl_attr.stAlarmOutRL.bEnableRecord = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPOutAlarm1", 12) == 0)
			{
				s_mivp.st_rl_attr.stAlarmOutRL.bOutAlarm1 = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPAlarmSound", 13) == 0)
			{
				s_mivp.st_rl_attr.stAlarmOutRL.bOutSound= atoi(pValue);
			}
			else if (strncmp(pItem, "IVPOutClient", 12) == 0)
			{
				s_mivp.st_rl_attr.stAlarmOutRL.bOutClient = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPOutEMail", 11) == 0)
			{
				s_mivp.st_rl_attr.stAlarmOutRL.bOutEMail = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPOutVMS", 9) == 0)
			{
				s_mivp.st_rl_attr.stAlarmOutRL.bOutVMS= atoi(pValue);
			}
			else if (strncmp(pItem, "IVPSen", 6) == 0)
			{
				s_mivp.st_rl_attr.nSen = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPThreshold", 12) == 0)
			{
				s_mivp.st_rl_attr.nThreshold = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPStayTime", 11) == 0)
			{
				s_mivp.st_rl_attr.nStayTime = atoi(pValue);
			}
			
			else if (strncmp(pItem, "IVPMarkObject", 13) == 0)
			{
				s_mivp.st_rl_attr.bMarkObject = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPPlateSnap", 12) == 0)
			{
				s_mivp.bPlateSnap = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPSnapRes", 10) == 0)
			{
				strcpy(s_mivp.sSnapRes, pValue);
			}
			
			//占有率分析部分
			if (strncmp(pItem, "bVREnable", 10) == 0)
			{
				s_mivp.st_vr_attr.bVREnable = atoi(pValue);
			}
			else if (strncmp(pItem, "nVRThreshold", 12) == 0)
			{
				s_mivp.st_vr_attr.nVRThreshold = atoi(pValue);
			}
			else if (strncmp(pItem, "nVRSen", 6) == 0)
			{
				s_mivp.st_vr_attr.nSen = atoi(pValue);
			}
			
			//人数统计部分
			else if (strncmp(pItem, "bMarkObject", 11) == 0)
			{
				s_mivp_count.st_count_attr.bMarkObject = atoi(pValue);
			}
			else if (strncmp(pItem, "bOpenCount", 10) == 0)
			{
				s_mivp_count.st_count_attr.bOpenCount = atoi(pValue);
			}
			else if (strncmp(pItem, "bShowCount", 10) == 0)
			{
				s_mivp_count.st_count_attr.bShowCount = atoi(pValue);
			}
			else if (strncmp(pItem, "bDrawCountLine", 14) == 0)
			{
				s_mivp_count.st_count_attr.bDrawFrame = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPCountOSDPos", 14) == 0)
			{
				s_mivp_count.st_count_attr.eCountOSDPos =	(mivpcountosd_pos_e) atoi(pValue);
			}
			else if (strncmp(pItem, "IVPCountOSDColor", 16) == 0)
			{
				s_mivp_count.st_count_attr.nCountOSDColor = atoi(pValue);
			}
			else if (strncmp(pItem, "nCountSaveDays", 14) == 0)
			{
				s_mivp_count.st_count_attr.nCountSaveDays = atoi(pValue);
			}
			//人数统计新增检测模式和检测点
			else if (strncmp(pItem, "CheckCountMode", 14) == 0)
			{
				s_mivp_count.st_count_attr.nCheckMode = atoi(pValue);
			}
			else if (strncmp(pItem, "nCountPoints", 12) == 0)
			{
				int n=0;
				char point[128];
				sscanf(pValue, "%d:%s", &n,point);
				if( n != 2 )
					n=0;
				s_mivp_count.st_count_attr.nPoints = n;

				for(i=0;i<s_mivp_count.st_count_attr.nPoints;i++)
				{
					if(i==s_mivp_count.st_count_attr.nPoints-1)
						sscanf(point, "%d,%d", &s_mivp_count.st_count_attr.stPoints[i].x,&s_mivp_count.st_count_attr.stPoints[i].y);
					else
						sscanf(point, "%d,%d-%s", &s_mivp_count.st_count_attr.stPoints[i].x,&s_mivp_count.st_count_attr.stPoints[i].y,point);
				}

			}
			
			//移动目标侦测
			else if (strncmp(pItem, "bIVPDetectEn", 12) == 0)
			{
				s_mivp.st_detect_attr.bDetectEn= atoi(pValue);
			}
			//遮挡报警
			else if (strncmp(pItem, "bIVPHideAlarmEn", 16) == 0)
			{
				s_mivp.st_hide_attr.bHideEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideThreshold", 17) == 0)
			{
				s_mivp.st_hide_attr.nThreshold = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideAlarmRecEn", 18) == 0)
			{
				s_mivp.st_hide_attr.stHideAlarmOut.bEnableRecord = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideAlarmOut1", 17) == 0)
			{
				s_mivp.st_hide_attr.stHideAlarmOut.bOutAlarm1 = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideAlarm2Client", 20) == 0)
			{
				s_mivp.st_hide_attr.stHideAlarmOut.bOutClient = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideAlarm2Email", 19) == 0)
			{
				s_mivp.st_hide_attr.stHideAlarmOut.bOutEMail = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideAlarm2VMS", 17) == 0)
			{
				s_mivp.st_hide_attr.stHideAlarmOut.bOutVMS = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPHideSound", 12) == 0)
			{
				s_mivp.st_hide_attr.stHideAlarmOut.bOutSound = atoi(pValue);
			}
			//遗留拿取报警
			else if (strncmp(pItem, "bIVPTLEnable", 12) == 0)
			{
				s_mivp.st_tl_attr.bTLEnable = atoi(pValue);
			}
			else if (strncmp(pItem, "bIVPTLMode", 12) == 0)
			{
				s_mivp.st_tl_attr.nTLMode = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLRgnCnt", 11) == 0)
			{
				s_mivp.st_tl_attr.nTLRgnCnt = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLRegion", 11) == 0)
			{
				int n = 0;
				char point[128];
				sscanf(pItem, "IVPRegion%d", &n);
				sscanf(pValue, "%d:%d:%s", &s_mivp.st_tl_attr.stTLRegion[n].nIvpCheckMode,&s_mivp.st_tl_attr.stTLRegion[n].nCnt, point);
				for (i = 0; i < s_mivp.st_tl_attr.stTLRegion[n].nCnt; i++)
				{
					if (i == s_mivp.st_tl_attr.stTLRegion[n].nCnt - 1)
						sscanf(point, "%d,%d", &s_mivp.st_tl_attr.stTLRegion[n].stPoints[i].x,&s_mivp.st_tl_attr.stTLRegion[n].stPoints[i].y);
					else
						sscanf(point, "%d,%d-%s",&s_mivp.st_tl_attr.stTLRegion[n].stPoints[i].x,&s_mivp.st_tl_attr.stTLRegion[n].stPoints[i].y, point);
				}
			}
			else if (strncmp(pItem, "IVPTLRecEnable", 14) == 0)
			{
				s_mivp.st_tl_attr.stTLAlarmOut.bEnableRecord = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLOutAlarm1", 14) == 0)
			{
				s_mivp.st_tl_attr.stTLAlarmOut.bOutAlarm1 = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLOutClient", 14) == 0)
			{
				s_mivp.st_tl_attr.stTLAlarmOut.bOutClient = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLOutEMail", 13) == 0)
			{
				s_mivp.st_tl_attr.stTLAlarmOut.bOutEMail = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLOutVMS", 11) == 0)
			{
				s_mivp.st_tl_attr.stTLAlarmOut.bOutVMS = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLSen", 8) == 0)
			{
				s_mivp.st_tl_attr.nTLSen = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLAlarmDuration", 18) == 0)
			{
				s_mivp.st_tl_attr.nTLAlarmDuration = atoi(pValue);
			}
			else if (strncmp(pItem, "IVPTLSuspectTime", 18) == 0)
			{
				s_mivp.st_tl_attr.nTLSuspectTime = atoi(pValue);
			}
			/*************************************智能分析*******************************************************/
		}
	}

}

int __CalcBitrate(int w, int h, int framerate, int vencType)
{
	int no = w*h;
	int defKB;
	double dkb ;
	{
		//这里有个特别之处
		if (no == 1280*960)//960仍然按720的码率来设计
		{
			no = 1280*720;
		}
		if (no >= 2592*1944)
		{
			dkb = (double)4608.0 * no / (2592*1944);
//			if(ipcinfo_get_type()==IPCTYPE_JOVISION)
//				dkb = (double)5120.0 * no / (2592*1520);
		}
		else if (no >= 2592*1520)
		{
			dkb = (double)4096.0 * no / (2592*1520);
//			if(ipcinfo_get_type()==IPCTYPE_JOVISION)
//				dkb = (double)4736.0 * no / (2592*1520);
		}
		else if (no >= 2048*1520)
		{
			dkb = (double)3584.0 * no / (2048*1520);
//			if(ipcinfo_get_type()==IPCTYPE_JOVISION)
//				dkb = (double)4096.0 * no / (2592*1520);
		}
		else if (no >= 1920*1080)
		{
			dkb = (double)3072.0 * no / (1920*1080);
		}
		else
		{
			dkb = (double)2048.0 * no / (1280*720);
		}
		if (no < 640*480)
		{
			dkb *= 1.8;
		}
		else if (no <= 720*576)
		{
			dkb *= 1.2;
			if(dkb>1024)
				dkb=1024;
		}
	}

	double IF = 1.0/4;
	dkb //= dkb * IF + dkb * (1-IF)*framerate/25
			= dkb * (IF + (1-IF)*framerate/25);

	if(vencType == JV_PT_H265)
		dkb = dkb * 2 / 3;


	defKB = (int)dkb;
	return defKB;
}

void __check_valid_res(int channelid, unsigned int *w, unsigned int *h)
{
	jvstream_ability_t stAbility;
	int res = *w * *h;
	int i;

	jv_stream_get_ability(channelid, &stAbility);
	if (res > 0 && res <= stAbility.maxStreamRes[channelid])
	{
		return ;
	}

	for (i=0;i<stAbility.resListCnt;i++)
	{
		if (stAbility.resList[i].width * stAbility.resList[i].height <= stAbility.maxStreamRes[channelid])
		{
			*w = stAbility.resList[i].width;
			*h = stAbility.resList[i].height;
			break;
		}
	}
	return ;
}

int ReadImportantInfo(ImportantCfg_t* im)
{
	return __read_important_cfg(im);
}

VOID ReadConfigInfo()
{
	char *pItem, *pValue;
	U32	i = 0, nCh=0, nRead=0, nOffset=0;
	char	acData[16*1024]={0}, acBuff[256]={0};
	char acBuffer[128] = {0};
	S32 fdCfg;
	int j;
	PTZ *pPtz = PTZ_GetInfo();
	PTZ_PRESET_INFO *preInfo = PTZ_GetPreset();
	PTZ_PATROL_INFO *patrol = PTZ_GetPatrol();
	PTZ_GUARD_T *guard = PTZ_GetGuard();
	PTZ_SCHEDULE_INFO *schedule = PTZ_GetSchedule();
	jvstream_ability_t stAbility;
	
	if(gp.bFactoryFlag)	//检测到工厂测试标识文件，先删除配置文件
	{
		printf("checked the factory test falg file,and rm the config files !!! \n");
		sprintf(acBuffer, "rm %s %s %s",CONFIG_FILE, FIRSTPOS_FILE, PATROL_FLAG_FLAG);
		utl_system(acBuffer);	
	}

	//ptz
	pPtz[i].nAddr		= i+1;
	pPtz[i].nProtocol	= hwinfo.ptzprotocol;	  //pelco-d
	pPtz[i].nBaudRate	= hwinfo.ptzbaudrate;
	pPtz[i].nHwParams.nBaudRate = pPtz[i].nBaudRate;
	pPtz[i].nHwParams.nCharSize	=	8;
	pPtz[i].nHwParams.nStopBit 	=	1;
	pPtz[i].nHwParams.nParityBit	=	PAR_NONE;
	pPtz[i].nHwParams.nFlowCtl 	=	PTZ_DATAFLOW_NONE;
	pPtz[i].bLeftRightSwitch =0;
	pPtz[i].bUpDownSwitch = 0;
	pPtz[i].bFocusZoomSwitch = 0;
	pPtz[i].bIrisZoomSwitch = 0;
	pPtz[i].bZoomSwitch = 0;
	pPtz[i].scanSpeed = 55;
	preInfo[i].nPresetCt = 0;
	patrol[i].nPatrolSize = 0;
	patrol[i].nPatrolSpeed = 0;
	patrol[i+1].nPatrolSize = 0;
	patrol[i+1].nPatrolSpeed = 0;
	guard[i].guardTime = 0;
	guard[i].guardType = GUARD_NO;
	guard[i].nRreset = 0;
	schedule[i].bSchEn[0] = FALSE;
	schedule[i].bSchEn[1] = FALSE;
	schedule[i].schedule[0] = 0;
	schedule[i].schedule[1] = 0;
	schedule[i].schTimeStart[0].hour = 6;
	schedule[i].schTimeStart[0].minute = 0;
	schedule[i].schTimeEnd[0].hour = 18;
	schedule[i].schTimeEnd[0].minute = 0;
	schedule[i].schTimeStart[1].hour = 18;
	schedule[i].schTimeStart[1].minute = 0;
	schedule[i].schTimeEnd[1].hour = 6;
	schedule[i].schTimeEnd[1].minute = 0;
	jv_ai_get_attr(0, &s_AiAttr);
	ipcinfo_get_param(&s_ipcinfo);
	//摄像机默认名称
	sprintf(s_ipcinfo.acDevName, "%s", "HD IPC");
#ifdef PLATFORM_hi3516D
	if(hwinfo.encryptCode == ENCRYPT_300W)
	{
		sprintf(s_ipcinfo.acDevName, "%s", "H.265 300W IPC");
		BOOL bRd = FALSE;
		char *rd = utl_fcfg_get_value(CONFIG_HWCONFIG_FILE,"400wto300w");	//400w降300w
		if(rd)
		{
			if (atoi(rd))
			{
				bRd = TRUE;
			}
		}
		if(hwinfo.sensor == SENSOR_OV4689 && !bRd)
			sprintf(s_ipcinfo.acDevName, "%s", "H.265 400W IPC");
	}
	else if(hwinfo.encryptCode == ENCRYPT_200W)
		sprintf(s_ipcinfo.acDevName, "%s", "H.265 200W IPC");
#endif
	if(hwinfo.bHomeIPC==1&&ipcinfo_get_type2()!= 1)
	{
	  char*m_channelName=	"此机器工作异常，请联系返修";
	  strcpy(s_ipcinfo.acDevName, m_channelName);
	}
	if(0 == access(RESTRICTION_FILE, F_OK))
	{
		int fd = open(RESTRICTION_FILE, O_RDONLY);
		if(fd != -1)
		{
			char buf[32];
			int bytes = read(fd, buf, 32);
			if(bytes > 0)
				sprintf(s_ipcinfo.type, "%s", buf);
			else
				sprintf(s_ipcinfo.type, "%s", hwinfo.type);
			close(fd);
		}
	}
	else
		sprintf(s_ipcinfo.type, "%s", hwinfo.type);
	s_ipcinfo.bSntp = TRUE;
	s_ipcinfo.sntpInterval = 24;

	s_ipcinfo.nLanguage = LANGUAGE_CN;
	if (hwinfo.bInternational)		// 国际版
		s_ipcinfo.nLanguage = LANGUAGE_EN;
	strcpy(s_ipcinfo.ntpServer, "cn.pool.ntp.org");
	s_ipcinfo.rebootDay = REBOOT_TIMER_NEVER;
	s_ipcinfo.rebootHour = 1;
	s_ipcinfo.LedControl = LED_CONTROL_AUTO;
	s_ipcinfo.bRestriction= TRUE;
	s_ipcinfo.osdSize = 32;
	s_ipcinfo.osdPosition = MULTIOSD_POS_HIDE;
	s_ipcinfo.osdAlignment = MULTIOSD_RIGHT;
	s_ipcinfo.bWebServer	= TRUE;
	s_ipcinfo.nWebPort		= 80;

	//时间格式默认为年月日,lck20120130
	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		s_osd[i].bShowOSD = TRUE;
		s_osd[i].position = MCHNOSD_POS_LEFT_BOTTOM;
		if (ipcinfo.nDeviceInfo[6] == 'H')
			s_osd[i].timePos = MCHNOSD_POS_LEFT_TOP;
		else
			s_osd[i].timePos = MCHNOSD_POS_RIGHT_TOP;

		s_osd[i].bLargeOSD = 1;
		//OSD反色默认参数。。中维产品打开，尚维产品关闭lk20131218
		if (ipcinfo_get_type() != IPCTYPE_SW)
			s_osd[i].osdbInvColEn = TRUE;
		else
			s_osd[i].osdbInvColEn = FALSE;
		strcpy(s_osd[i].channelName, s_ipcinfo.acDevName);
		if (s_ipcinfo.nLanguage == LANGUAGE_CN)
		{
			gp.nTimeFormat = TF_YYYYMMDD;
			strcpy(s_osd[i].timeFormat, "YYYY-MM-DD hh:mm:ss");
		}
		else
		{
			gp.nTimeFormat = TF_MMDDYYYY;
			strcpy(s_osd[i].timeFormat, "MM/DD/YYYY hh:mm:ss");
		}
	}
	s_ipcinfo.tz = 8;

#ifdef YST_SVR_SUPPORT
	//云视通
	GetYSTParam(&s_stYst);
	memcpy(s_stYst.strGroup, &s_ipcinfo.nDeviceInfo[6], 4);//云视通组号"A"
	s_stYst.nID			= 0;
	s_stYst.nPort			= 9101;
	s_stYst.nYSTPeriod	= 10; //云视通状态检查周期,秒
#endif

	s_sctrl.nPictureType	= MOBILE_QUALITY_LOW;


	//区域遮挡,测试
	s_region.bEnable	= FALSE;
	for(i=0; i<MAX_PYRGN_NUM; i++)
	{
		s_region.stRect[i].x	= 0;
		s_region.stRect[i].y	= 0;
		s_region.stRect[i].w	= 0;
		s_region.stRect[i].h	= 0;
	}

	//sensor参数
	i = 0;
	memset(&s_stSensorAttr[i],0,sizeof(msensor_attr_t));
	s_stSensorAttr[i].brightness = 128;
	s_stSensorAttr[i].exposure = 128;
	s_stSensorAttr[i].saturation = 128;
	/*
	1,0
	0x1fe0,0x400,0x5a30,0x400
	0x1000,0xb3aba
	0x0,0x400d2158,0x11106c,0x0
	3018240,517724,516676,-1093323524

	*/
	s_stSensorAttr[i].drc.bDRCEnable = FALSE;
	s_stSensorAttr[i].ae.exposureMax = 3;
	s_stSensorAttr[i].ae.exposureMin = 100000;
	
	s_stSensorAttr[i].ae.bAEME = TRUE;
	s_stSensorAttr[i].ae.bByPassAE = FALSE;
	s_stSensorAttr[i].ae.u16DGainMax = 0x1fe0;
	s_stSensorAttr[i].ae.u16DGainMin = 0x400;
	s_stSensorAttr[i].ae.u16AGainMax = 0x5a30;
	s_stSensorAttr[i].ae.u16AGainMin = 0x400;
	s_stSensorAttr[i].ae.u32ISPDGainMax = 0x1000;
	s_stSensorAttr[i].ae.u32SystemGainMax = 0xb3aba;
	
	s_stSensorAttr[i].ae.s32AGain = 0x400;
	s_stSensorAttr[i].ae.s32DGain = 0x400;
	s_stSensorAttr[i].ae.u32ISPDGain = 0x400;
	s_stSensorAttr[i].ae.u32ExpLine = 0x400;
	s_stSensorAttr[i].ae.bManualExpLineEnable = FALSE;
	s_stSensorAttr[i].ae.bManualAGainEnable = FALSE;
	s_stSensorAttr[i].ae.bManualDGainEnable = FALSE;
	s_stSensorAttr[i].ae.bManualISPGainEnable = FALSE;
	
    s_stSensorAttr[i].contrast=128;
    s_stSensorAttr[i].sharpness=128;

    s_stSensorAttr[i].antifog = 0;
	s_stSensorAttr[i].light= 4;
    s_stSensorAttr[i].bSupportWdr=hwinfo.wdrBsupport;
	if(hwinfo.sensor == SENSOR_AR0230)
	{
#if (defined PLATFORM_hi3516D)
		s_stSensorAttr[i].bEnableWdr=FALSE;
#else
    	s_stSensorAttr[i].bEnableWdr=TRUE;
#endif
	}
	else
		s_stSensorAttr[i].bEnableWdr=FALSE;
    s_stSensorAttr[i].daynightMode=MSENSOR_DAYNIGHT_AUTO;
	if(hwinfo.sensor == SENSOR_BT601)
		s_stSensorAttr[i].daynightMode=MSENSOR_DAYNIGHT_ALWAYS_DAY;
	
	//低照度模式是否降帧开关初始化
	s_stSensorAttr[i].bSupportSl = FALSE;
	s_stSensorAttr[i].bEnableSl = FALSE;
#if (defined PLATFORM_hi3516D)
	if(hwinfo.sensor==SENSOR_IMX123)
		s_stSensorAttr[0].bEnableSl = TRUE;
#endif

    s_stSensorAttr[i].dayStart.hour = 6;
    s_stSensorAttr[i].dayStart.minute = 0;
    s_stSensorAttr[i].dayEnd.hour = 18;
    s_stSensorAttr[i].dayEnd.minute = 0;

    s_stSensorAttr[i].effect_flag=1<<EFFECT_AWB;
    
    if (hwinfo.sensor == SENSOR_OV9712
    		|| hwinfo.sensor == SENSOR_AR0330|| hwinfo.sensor == SENSOR_OV2710
    		|| hwinfo.sensor == SENSOR_AR0130
    	 	|| hwinfo.sensor == SENSOR_OV9750 ||hwinfo.sensor==SENSOR_OV9732
    	 	|| hwinfo.sensor == SENSOR_AR0230 || hwinfo.sensor == SENSOR_IMX123 || hwinfo.sensor == SENSOR_IMX178
    	 	|| hwinfo.sensor == SENSOR_OV4689
    	 	|| hwinfo.sensor == SENSOR_BG0701 || hwinfo.sensor == SENSOR_IMX290
    	 	|| hwinfo.sensor == SENSOR_AR0237 || hwinfo.sensor == SENSOR_AR0237DC
    	 	|| hwinfo.sensor == SENSOR_SC2045
    	 	|| hwinfo.sensor == SENSOR_SC2135
    	 	|| hwinfo.sensor == SENSOR_OV2735 || hwinfo.sensor == SENSOR_MN34227 || hwinfo.sensor == SENSOR_SC2235
    		)
    {
		s_stSensorAttr[i].effect_flag |= (1 << (EFFECT_LOW_FRAME));
    }
    s_stSensorAttr[i].sw_cut =0;
    s_stSensorAttr[i].cut_rate =30;
	if(hwinfo.bHomeIPC)
		s_stSensorAttr[i].sence=SENCE_INDOOR;
	else
	    s_stSensorAttr[i].sence=SENCE_OUTDOOR;
    s_stSensorAttr[i].cutDelay = 3;
    s_stSensorAttr[i].rotate = JVSENSOR_ROTATE_NONE;
	
	s_stSensorAttr[i].imageD = D_NORMAL;
	s_stSensorAttr[i].AutoLowFrameEn = FALSE; 
	if(ipcinfo_get_type() == IPCTYPE_SW)
	{
		s_stSensorAttr[i].imageD = D_HIGH; 
//		s_stSensorAttr[i].AutoLowFrameEn = TRUE;
	}
	s_stSensorAttr[i].exp_mode = JV_EXP_AUTO_MODE;
	
    msensor_get_local_exposure(0,&stLocalexposure);
	stLocalexposure.bLE_Reverse = 0;
	stLocalexposure.nLE_RectNum = 0;
	stLocalexposure.nLE_Weight = 146;
	memset(&(stLocalexposure.stLE_Rect),0,RECT_MAX_CNT*sizeof(RECT));

	jv_stream_get_ability(0, &stAbility);

	int dft_framerate = 25;
	BOOL STREAM_RECT_B_STRETCH = 1;

	s_stRoi.ior_reverse = 0;
	s_stRoi.roiWeight = 72;		//14
	memset(&(s_stRoi.roi),0,sizeof(s_stRoi.roi));

	//默认参数，适合1080p的摄像机
	i=0;//码流1，默认全高清，主要供NVR连接，进行录像
	s_stAttr[i].bEnable = TRUE;
	s_stAttr[i].vencType = JV_PT_H264;
#if (defined PLATFORM_hi3516D) || (defined PLATFORM_hi3516EV100)
	s_stAttr[i].vencType = JV_PT_H265;
#endif
	s_stAttr[i].bAudioEn = 1;
	s_stAttr[i].width = VI_WIDTH;
	s_stAttr[i].height = VI_HEIGHT;
	s_stAttr[i].framerate = dft_framerate;//s_stAbility.maxFramerate;
	s_stAttr[i].rcMode = JV_VENC_RC_MODE_VBR;
	s_stAttr[i].maxQP = 46;
	s_stAttr[i].minQP = 24;
	s_stAttr[i].nGOP_S = NGOP_FRAMERATE;
	s_stAttr[i].bitrate = __CalcBitrate(s_stAttr[i].width, s_stAttr[i].height, s_stAttr[i].framerate, s_stAttr[i].vencType);
	//主码流默认为不拉伸，避免因缩放导致的图像质量下降
	s_stAttr[i].bRectStretch = 0;//STREAM_RECT_B_STRETCH;
	s_stAttr[i].bLDCEnable = FALSE; //默认关闭畸变校正
	s_stAttr[i].nLDCRatio   = 455; //畸变校正强度

	if(hwinfo.bHomeIPC)
	{
		s_stAttr[i].nGOP_S = 4;
		if(strcmp(hwinfo.devName,"BMDQ") == 0 || strcmp(hwinfo.devName,"H411BMDQ") == 0)
		{
			s_stAttr[i].framerate = 22;
			s_stAttr[i].bitrate = 2048;
		}
		else if(strcmp(hwinfo.devName,"YL") == 0)
		{
			s_stAttr[i].framerate = 15;
			s_stAttr[i].bitrate = 800;
		}
		else if (strcmp(hwinfo.devName,"HC421S-H1") == 0)
		{
			s_stAttr[i].framerate = 20;
			s_stAttr[i].bitrate = 640;
		}
#if (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100)
		else if (hwinfo.encryptCode == ENCRYPT_200W)	// 200W
		{
			// 200W提高到20帧
			s_stAttr[i].framerate = 20;
			s_stAttr[i].bitrate = 640;
		}
		else if (hwinfo.encryptCode == ENCRYPT_130W)	// 130W
		{
			s_stAttr[i].framerate = 25;
			s_stAttr[i].bitrate = 540;
		}
#endif
		else
		{
			s_stAttr[i].framerate = 20;
			s_stAttr[i].bitrate = 440;
		}
	}

	i++;//码流2，默认一个适合网传的分辨率
	s_stAttr[i].bEnable = TRUE;
	s_stAttr[i].vencType = JV_PT_H264;
#if (defined PLATFORM_hi3516D) || (defined PLATFORM_hi3516EV100)
	s_stAttr[i].vencType = JV_PT_H265;
#endif
	s_stAttr[i].bAudioEn = 1;
	s_stAttr[i].width = 704;
	s_stAttr[i].height = 576;

	__check_valid_res(i, &s_stAttr[i].width, &s_stAttr[i].height);
	s_stAttr[i].framerate = dft_framerate;//s_stAbility.maxFramerate;
	s_stAttr[i].rcMode = JV_VENC_RC_MODE_VBR;
	s_stAttr[i].maxQP = 46;
	s_stAttr[i].minQP = 24;
	s_stAttr[i].bitrate = __CalcBitrate(s_stAttr[i].width, s_stAttr[i].height, s_stAttr[i].framerate, s_stAttr[i].vencType);
	s_stAttr[i].nGOP_S = NGOP_FRAMERATE;
	s_stAttr[i].bRectStretch = 0;  //STREAM_RECT_B_STRETCH;
	s_stAttr[i].bLDCEnable = FALSE; //默认关闭畸变校正
	s_stAttr[i].nLDCRatio   = 255; //畸变校正强度	
	if (hwinfo.bHomeIPC)
	{
		s_stAttr[i].width = 512;
		s_stAttr[i].height = 288;
		s_stAttr[i].nGOP_S = 4;
		if(strcmp(hwinfo.devName,"YL") == 0)
		{
			s_stAttr[i].framerate = 20;
			s_stAttr[i].bitrate = 300;
		}
		else if (strcmp(hwinfo.devName,"HC421S-H1") == 0)
		{
			s_stAttr[i].framerate = 15;
			s_stAttr[i].bitrate = 168;
		}
		else
		{
			s_stAttr[i].framerate = 15;
			//stAttr[i].rcMode = JV_VENC_RC_MODE_CBR;
			s_stAttr[i].bitrate = 128;
		}
	}

	i++;//码流3，默认一个适合手机的分辨率
	s_stAttr[i].bEnable	= TRUE;
	s_stAttr[i].vencType = JV_PT_H264;
#if (defined PLATFORM_hi3516D) || (defined PLATFORM_hi3516EV100)
	s_stAttr[i].vencType = JV_PT_H265;
#endif
	s_stAttr[i].bAudioEn = 1;
	s_stAttr[i].width		= 352;
	s_stAttr[i].height	= 288;
	__check_valid_res(i, &s_stAttr[i].width, &s_stAttr[i].height);
	s_stAttr[i].framerate	= 15;
	s_stAttr[i].rcMode = JV_VENC_RC_MODE_VBR;
	s_stAttr[i].maxQP = 46;
	s_stAttr[i].minQP = 24;
	s_stAttr[i].bitrate	= __CalcBitrate(s_stAttr[i].width, s_stAttr[i].height, s_stAttr[i].framerate, s_stAttr[i].vencType);
	if(hwinfo.bHomeIPC)
	{
		s_stAttr[i].framerate	= 25;
		s_stAttr[i].bitrate = 512;
	}
	s_stAttr[i].nGOP_S		= NGOP_FRAMERATE;
	s_stAttr[i].bRectStretch = 0;  //STREAM_RECT_B_STRETCH;
	s_stAttr[i].bLDCEnable = FALSE; //默认关闭畸变校正
	s_stAttr[i].nLDCRatio   = 255; //畸变校正强度

    if (strncmp(hwinfo.type, "nxp", 3) == 0)
    {
    	s_stAttr[0].bitrate	 =s_stAttr[0].bitrate*0.9;
       	s_stAttr[1].bitrate	 = s_stAttr[1].bitrate*0.9;
    	s_stAttr[2].bitrate	= 256;
        s_stAttr[2].width	= 624;
        s_stAttr[2].height	= 352;
        s_stAttr[2].bitrate	= 256;
    }

    s_AiAttr.encType = JV_AUDIO_ENC_G711_A;
	//移动检测参数
	memset(&s_md, 0, sizeof(s_md));

	s_md.bEnable		= FALSE;
	s_md.nSensitivity	= 60;
	s_md.bEnableRecord = TRUE;
	s_md.nDelay		= 10;
	s_md.bBuzzing = TRUE;
	for(i=0; i<MAX_MDRGN_NUM; i++)
	{
		s_md.stRect[i].x = 0;
		s_md.stRect[i].y = 0;
		s_md.stRect[i].w = 0;
		s_md.stRect[i].h = 0;
	}
	// 鹏博士定制默认开启移动侦测
	if (PRODUCT_MATCH(PRODUCT_PBS))
	{
		s_md.bEnable = TRUE;
	}
#ifdef MD_GRID_SET
	s_md.nColumn=22;
	s_md.nRow=18;
#endif


	//录像参数
	for(i=0; i<MAX_REC_TASK; i++)
	{
		memset(&s_recAttr[i], 0, sizeof(mrecord_attr_t));
		s_recAttr[i].bEnable = TRUE;
		s_recAttr[i].file_length = 600;
		Printf("---recAttr[i].file_length:%d\n", s_recAttr[i].file_length);
		s_recAttr[i].timing_enable = FALSE;
		s_recAttr[i].discon_enable = FALSE;
		s_recAttr[i].alarm_enable = FALSE;
		s_recAttr[i].timing_start = 0;
		s_recAttr[i].timing_stop = 0;
		s_recAttr[i].detecting = 0;
		s_recAttr[i].ivping= 0;
		s_recAttr[i].alarming = 0;
		s_recAttr[i].alarm_pre_record = 8;
		s_recAttr[i].alarm_duration = 10;
		s_recAttr[i].chFrame_enable = FALSE;
		s_recAttr[i].chFrameSec = 4;

		// 鹏博士默认开启报警录像模式
		if (PRODUCT_MATCH(PRODUCT_PBS))
		{
			s_recAttr[i].bEnable = FALSE;
			s_recAttr[i].alarm_enable = TRUE;
		}
	}

    //gyd20120323  添加报警部分
    memset(&s_alarm,0,sizeof(s_alarm));
    s_alarm.delay=10;
	sprintf(s_alarm.sender, "ipcmail@163.com");
	sprintf(s_alarm.server, "smtp.163.com");
	sprintf(s_alarm.username, "ipcmail");
	sprintf(s_alarm.passwd, "ipcam71a");
	sprintf(s_alarm.receiver0, "(null)");
	sprintf(s_alarm.receiver1, "(null)");
	sprintf(s_alarm.receiver2, "(null)");
	sprintf(s_alarm.receiver3, "(null)");
	sprintf(s_alarm.vmsServerIp, "0.0.0.0");
	s_alarm.vmsServerPort = 10000;

	s_alarm.bEnable = FALSE;
	s_alarm.bAlarmSoundEnable = FALSE;
	s_alarm.bAlarmLightEnable = FALSE;
	s_alarm.port = 25;
	sprintf(s_alarm.crypto, "none");

	//lk20131129 报警输入
	s_alarmin.bEnable = FALSE;
	s_alarmin.bNormallyClosed = FALSE;
	s_alarmin.bSendEmail = TRUE;
	s_alarmin.bSendtoClient = TRUE;
	s_alarmin.bSendtoVMS= TRUE;
	s_alarmin.bEnableRecord = TRUE;
	s_alarmin.u8AlarmNum = 0x1;//默认开启第一路报警..140415
	s_alarmin.bBuzzing = TRUE;

	/*-----------------------------------------------智能分析------------------------------------------------------*/

	mivp_get_param(0,&s_mivp);
	memset(&s_mivp,0,sizeof(MIVP_t));
	mivp_count_get(0, &s_mivp_count);
	memset(&s_mivp_count, 0, sizeof(MIVP_t));
	s_mivp.st_rl_attr.nSen=90;
	s_mivp.st_rl_attr.bDrawFrame = 0;			//默认关闭区域画线
	s_mivp.st_rl_attr.bFlushFrame = 1;
	s_mivp.st_rl_attr.stAlarmOutRL.bOutClient = 1;
	s_mivp.st_rl_attr.stAlarmOutRL.bOutEMail = 1;
	s_mivp.st_rl_attr.stAlarmOutRL.bOutVMS= 1;
	s_mivp.st_rl_attr.stAlarmOutRL.bOutSound=0;
	s_mivp.st_rl_attr.stAlarmOutRL.bEnableRecord=TRUE;
	
	s_mivp.st_rl_attr.nStayTime = 0;
	s_mivp.st_rl_attr.nThreshold = 40;
	s_mivp.st_rl_attr.bMarkObject = 1;			//默认标记告警物体
	strcpy(s_mivp.sSnapRes, "1280*720");
	//人群密度
	s_mivp.st_cde_attr.nCDEThreshold = 90;
	s_mivp.st_cde_attr.bDrawFrame = 1;
	s_mivp.st_cde_attr.stCDEAlarmOut.bOutClient = 1;
	s_mivp.st_cde_attr.stCDEAlarmOut.bOutEMail = 1;
    //超员检测
	s_mivp.st_ocl_attr.bDrawFrame = 1;
    s_mivp.st_ocl_attr.stOCLAlarmOut.bOutClient = 1;
    s_mivp.st_ocl_attr.stOCLAlarmOut.bOutEMail = 1;
	//快速移动
	s_mivp.st_fm_attr.nSen = 90;
	s_mivp.st_fm_attr.bDrawFrame = 1;
	s_mivp.st_fm_attr.bFlushFrame = 1;
	s_mivp.st_fm_attr.stAlarmOutRL.bOutClient = 1;
	s_mivp.st_fm_attr.stAlarmOutRL.bOutEMail = 1;
	s_mivp.st_fm_attr.stAlarmOutRL.bOutVMS = 1;
	s_mivp.st_fm_attr.nStayTime = 0;
	s_mivp.st_fm_attr.nThreshold = 40;
	s_mivp.st_fm_attr.bMarkObject = 1;
	s_mivp.st_fm_attr.nSpeedLevel = 3;
	//车牌识别
	s_mivp.st_lpr_attr.work_mode = 2;
	s_mivp.st_lpr_attr.def_dir = 0;
	s_mivp.st_lpr_attr.display = 0;
	
	//占有率部分
	s_mivp.st_vr_attr.bVREnable = 0;
	s_mivp.st_vr_attr.nVRThreshold = 20;
	s_mivp.st_vr_attr.nSen=35;
	
	//客流统计部分
	s_mivp_count.st_count_attr.nTimeIntervalReport = 300;
	s_mivp_count.st_count_attr.bShowCount = TRUE;
	s_mivp_count.st_count_attr.nLinePosY = 20;
	//拿取遗留报警
	s_mivp.st_tl_attr.nTLAlarmDuration = 10;
	s_mivp.st_tl_attr.nTLSen = 60;
	s_mivp.st_tl_attr.stTLAlarmOut.bOutClient = 1;
	s_mivp.st_tl_attr.stTLAlarmOut.bOutEMail = 1;

	//徘徊
	s_mivp.st_hover_attr.stAlarmOut.bOutClient = 1;
	s_mivp.st_hover_attr.stAlarmOut.bOutEMail = 1;

	//遮挡
	s_mivp.st_hide_attr.nThreshold = 70;
	//虚焦
	s_mivp.st_vf_attr.nThreshold = 70;
	//烟火
	s_mivp.st_fire_attr.stAlarmOut.bOutClient = 1;
	s_mivp.st_fire_attr.stAlarmOut.bOutEMail = 1;
	s_mivp.st_fire_attr.sensitivity = 70;

	s_mivp.st_sc_attr.bEnable = FALSE;
	s_mivp.st_sc_attr.nThreshold = 50;
	s_mivp.st_sc_attr.duration = 10;
	/*-----------------------------------------------智能分析------------------------------------------------------*/

	//DDNS
	// 云存储
	memset(&s_cloud, 0, sizeof(CLOUD));

	//读取对话框的版本号
	fdCfg = open(VER_IPCAMCFG, O_RDONLY);
	if(fdCfg < 0)
	{
		printf("Read file %s failed.\n", VER_IPCAMCFG);
	}
	read(fdCfg, &gp.nIPCamCfg, sizeof(gp.nIPCamCfg));
	close(fdCfg);
	Printf("IPCamCfg.dll, version=%d\n", gp.nIPCamCfg);

	ImportantCfg_t im;
	memset(&im, 0, sizeof(ImportantCfg_t));
	if(__read_important_cfg(&im)==0)
	{
		s_ipcinfo.nLanguage = im.nLanguage;
		if(s_ipcinfo.nLanguage == LANGUAGE_CN)
		{
			gp.nTimeFormat = TF_YYYYMMDD;
		}
		else
		{
			gp.nTimeFormat = TF_MMDDYYYY;
		}
		
	}
	//根据重要配置文件的时间格式，初始化时间设置的时间格式
	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		if(s_ipcinfo.nLanguage == LANGUAGE_CN)
		{
			strcpy(s_osd[i].timeFormat, "YYYY-MM-DD hh:mm:ss");
		}
		else
		{
			strcpy(s_osd[i].timeFormat, "MM/DD/YYYY hh:mm:ss");
		}
	}

	if (0 == access(DEFAULT_FORCE_FILE, F_OK))
	{
		//强制初始化
		printf("=========>> force update\n");
		unlink(DEFAULT_FORCE_FILE);
		unlink(CONFIG_FILE);
	}
	if (0 == access(RESTRICTION_FILE, F_OK))
	{
		s_ipcinfo.bRestriction = FALSE;
	}
	__read_config_file(DEFAULT_CFG_FILE);
	__read_config_file(CONFIG_FILE);
	__read_config_file(TUTK_FILE);
	//加密芯片是否内置云视通号
	if(s_ipcinfo.nDeviceInfo[9] == 0xB567709F || s_ipcinfo.nDeviceInfo[9] == 0xB56881B0)
	{
		im.ystID = s_ipcinfo.nDeviceInfo[10];
	}
	s_ipcinfo.ystID = im.ystID;
	s_stYst.nID = s_ipcinfo.ystID;

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100)
	if(stAbility.inputRes.height == 1080)
	{
		for(i = 0;i < HWINFO_STREAM_CNT;i++)
			s_stAttr[i].bRectStretch = STREAM_RECT_B_STRETCH;
		if(s_stAttr[0].width*s_stAttr[0].height > 1280*960)
			hwinfo.rotateBSupport = FALSE;
		else
			hwinfo.rotateBSupport = TRUE;
	}
#endif

#if (defined PLATFORM_hi3516D)
		if(hwinfo.sensor==SENSOR_OV2710)
			s_stSensorAttr[0].bSupportSl = TRUE;
		else if(hwinfo.sensor==SENSOR_IMX123)
			s_stSensorAttr[0].bSupportSl = FALSE;
#endif

	//避免原始版本升级后，IPC的QP质量未更新
	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		if (s_stAttr[i].maxQP == 32)
			s_stAttr[i].maxQP = 46;
	}

	//为避免升级后，内容还没写进去。这里再写入一遍
	//__write_important_cfg 内部有判断，可避免重复写入
	{
		im.nLanguage = s_ipcinfo.nLanguage;
		im.nTimeFormat = gp.nTimeFormat;
		im.ystID = s_ipcinfo.ystID;
		__write_important_cfg(&im);
	}

SETTING:
	mlog_enable(FALSE);
#ifdef YST_SVR_SUPPORT
	SetYSTParam(&s_stYst);
#endif
	SctrlSetParam(&s_sctrl);
	msensor_setparam(&s_stSensorAttr[0]);

	int flag = 0;
	int tmpWidth,tmpHeight;
	
	if((hwinfo.sensor == SENSOR_AR0130 && s_stAttr[0].height > 960))
	{
		tmpWidth = s_stAttr[0].width;
		tmpHeight = s_stAttr[0].height;
		flag = 1;
	}

	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		__check_valid_res(i, &s_stAttr[i].width, &s_stAttr[i].height);
		mstream_set_param(i, &s_stAttr[i]);
		mchnosd_set_param(i, &s_osd[i]);
	}
	mstream_set_roi(&s_stRoi);
	ipcinfo_set_param(&s_ipcinfo);
	malarm_set_param(&s_alarm);
	mdetect_set_param(0, &s_md);
	mprivacy_set_param(0, &s_region);
	for(i=0; i<MAX_REC_TASK; i++)
		mrecord_set_param(i, &s_recAttr[i]);
	mstream_audio_set_param(0, &s_AiAttr);
	malarmin_set_param(0, &s_alarmin);
	mlog_enable(TRUE);
	msensor_set_local_exposure(0,&stLocalexposure);
	mivp_set_param(0,&s_mivp);
	mivp_count_set(0,&s_mivp_count);
	mcloud_set_param(&s_cloud);
}

static char *__date_str_2_yyyymmdd(char *str, char *dst)
{
	char date[32];
	strncpy(date, str, sizeof(date));

	//month
	char *month = date;
    while(month[0] == ' ')
    {
        month[0] = '\0';
        month++;
    }

	//day
	char *day = strchr(month, ' ');
	if (!day)
		return str;
    do{
        day[0] = '\0';
        day++;
    }while(day[0] == ' ');
    
	//year
	char *year = strchr(day, ' ');
	if (!year)
		return str;
    do{
        year[0] = '\0';
        year++;
    }while(year[0] == ' ');
	year[4] = '\0';

	const char *monstr[] = {
			"Jan",  "Feb",  "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	int i;
	int monInt = -1;
	for (i=0;i<sizeof(monstr)/sizeof(monstr[0]);i++)
	{
		if (0 == strcmp(monstr[i], month))
		{
			monInt = i+1;
			break;
		}
	}

	if (monInt == -1)
		return str;
    if(day[1] == '\0')
    {
        sprintf(dst, "%s%02d0%s", year, monInt, day);
    }    
    else
    {
        sprintf(dst, "%s%02d%s", year, monInt, day);
    }
    
	return dst;
}

//将配置写入配置文件时，每一项使用分号分割,lck20120302
//[ALL]选项在前，[CH*]选项在后
VOID WriteConfigInfo()
{
	U32	i;
	FILE * fOut = NULL;
	MD md;
	REGION region;
	msensor_attr_t stSensorAttr;
	mstream_attr_t stAttr;
	mrecord_attr_t recAttr;
	ALARMSET alarm;
	ipcinfo_t ipcinfo;
	mchnosd_attr osd;
	jvstream_ability_t ability;
	mstream_roi_t roi;
	YST stYST;
	SCTRLINFO sctrl;
	FILE * fp = NULL;
	char acItem[512];
	char tmp[128];
	acItem[0]='\0';
  /* 读加锁 ,当reboot写锁时，阻塞*/
       pthread_rwlock_rdlock(&g_reboot_rwlock);
  	fOut = fopen(CONFIG_FILE, "w+");
	if (fOut == NULL)
	{
		Printf("Write To %s Failed\n", CONFIG_FILE);
		pthread_rwlock_unlock(&g_reboot_rwlock);
		return;
	}

#ifdef YST_SVR_SUPPORT
	GetYSTParam(&stYST);
#endif

	SctrlGetParam(&sctrl);

	//版本号
	fprintf(fOut, "CONFIG_VER=%d;", CONFIG_VER);
	//[ALL]不是针对通道的设置
	fprintf(fOut, "[ALL];");
	fprintf(fOut,"YSTID=%d;" , s_ipcinfo.ystID);
	fprintf(fOut,"YSTGROUP=%d;",ipcinfo_get_param(NULL)->nDeviceInfo[6]);
	fprintf(fOut,"nTimeFormat=%d;" , gp.nTimeFormat);

	ipcinfo_get_param(&ipcinfo);
	char datestr[32];
    memset(datestr,0,sizeof(datestr));
	fprintf(fOut,"Version=%s - %s     %s;" , IPCAM_VERSION, __date_str_2_yyyymmdd(__DATE__, datestr), __TIME__);
	fprintf(fOut,"Softverion=%d;",atoi(strrchr(IPCAM_VERSION,'.') + 1));	//手机端要通过版本号区分一些功能
    if (IPCTYPE_IPDOME == ipcinfo_get_type())
    {
	    Printf("<><><><> IPCTYPE_IP     %s\n", ipcinfo.type);
		fprintf(fOut,"ProductType=%s;" , ipcinfo.type);
    }
	else if (IPCTYPE_SW == ipcinfo_get_type())
	{
	    Printf("<><><><> IPCTYPE_SW     IPC\n");
		//if(0 == access(RESTRICTION_FILE, F_OK))
			fprintf(fOut,"ProductType=%s;" , ipcinfo.type);
		//else
		//	fprintf(fOut,"ProductType=%s;" , "IPC");
    }
	else
	{
        Printf("<><><><> IPCTYPE_ZW     %s\n", ipcinfo.type);
        fprintf(fOut,"ProductType=%s;" , ipcinfo.type);
	}
	if(0 == access(RESTRICTION_FILE, F_OK))
	{
		int fd = open(RESTRICTION_FILE, O_WRONLY | O_TRUNC);
		if(fd != -1)
		{
			write(fd, ipcinfo.type, strlen(ipcinfo.type)+1);
			close(fd);
		}
	}
		
	fprintf(fOut,"DevName=%s;" , ipcinfo.acDevName);
	fprintf(fOut,"nickName=%s;" , ipcinfo.nickName);
	fprintf(fOut,"nLanguage=%d;", ipcinfo.nLanguage);
	fprintf(fOut,"SN=%u;" , ipcinfo.sn);			//设备识别号

	fprintf(fOut,"bSntp=%d;", ipcinfo.bSntp);
	fprintf(fOut,"sntpInterval=%d;", ipcinfo.sntpInterval);
	fprintf(fOut,"ntpServer=%s;", ipcinfo.ntpServer);
	fprintf(fOut,"timezone=%d;", ipcinfo.tz);
	fprintf(fOut,"rebootDay=%d;", ipcinfo.rebootDay);
	fprintf(fOut,"rebootHour=%d;", ipcinfo.rebootHour);
	fprintf(fOut,"LedControl=%d;", ipcinfo.LedControl);
	fprintf(fOut,"bRestriction=%d;", ipcinfo.bRestriction);
	fprintf(fOut, "PortUsed=%s;", ipcinfo.portUsed);
	fprintf(fOut,"osdText0=%s;", ipcinfo.osdText[0]);
	fprintf(fOut,"osdText1=%s;", ipcinfo.osdText[1]);
	fprintf(fOut,"osdText2=%s;", ipcinfo.osdText[2]);
	fprintf(fOut,"osdText3=%s;", ipcinfo.osdText[3]);
	fprintf(fOut,"osdText4=%s;", ipcinfo.osdText[4]);
	fprintf(fOut,"osdText5=%s;", ipcinfo.osdText[5]);
	fprintf(fOut,"osdText6=%s;", ipcinfo.osdText[6]);
	fprintf(fOut,"osdText7=%s;", ipcinfo.osdText[7]);
	fprintf(fOut,"osdX=%d;", ipcinfo.osdX);
	fprintf(fOut,"osdY=%d;", ipcinfo.osdY);
	fprintf(fOut,"endX=%d;", ipcinfo.endX);
	fprintf(fOut,"endY=%d;", ipcinfo.endY);
	fprintf(fOut,"osdSize=%d;", ipcinfo.osdSize);
	fprintf(fOut,"multiPosition=%d;", ipcinfo.osdPosition);
	fprintf(fOut,"alignment=%d;", ipcinfo.osdAlignment);
	fprintf(fOut,"lcmsServer=%s;" , ipcinfo.lcmsServer);

	//区分中维和尚维。lk20131218
	fprintf(fOut,"ipcinfoType=%d;",ipcinfo_get_type());

	fprintf(fOut, "WebServer=%d;", ipcinfo.bWebServer);
	fprintf(fOut, "WebPort=%u;", ipcinfo.nWebPort);

	jv_stream_get_ability(0, &ability);
	fprintf(fOut, "viWidth=%d;", ability.inputRes.width);
	fprintf(fOut, "viHeight=%d;", ability.inputRes.height);
	fprintf(fOut, "maxFramerate=%d;", ability.maxFramerate);
	fprintf(fOut, "resListCnt=%d;", ability.resListCnt);
	for (i=0;i<ability.resListCnt;i++)
	{
		fprintf(fOut, "resList=%dx%d;", ability.resList[i].width, ability.resList[i].height);
	}
	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		fprintf(fOut, "[CH%d];", i+1);
		fprintf(fOut, "maxStreamRes=%d;", ability.maxStreamRes[i]);
	}

	fprintf(fOut,"YSTPort=%d;", stYST.nPort);

	fprintf(fOut, "PictureType=%d;", sctrl.nPictureType);

    //gyd20120323 报警
    malarm_get_param(&alarm);
    fprintf(fOut,"nAlarmDelay=%d;",alarm.delay);
    fprintf(fOut,"acMailSender=%s;",alarm.sender);
    fprintf(fOut,"acSMTPServer=%s;",alarm.server);
    fprintf(fOut,"acSMTPUser=%s;",alarm.username);
    fprintf(fOut,"acSMTPPasswd=%s;",alarm.passwd);
    fprintf(fOut,"acReceiver0=%s;",alarm.receiver0);
    fprintf(fOut,"acReceiver1=%s;",alarm.receiver1);
    fprintf(fOut,"acReceiver2=%s;",alarm.receiver2);
    fprintf(fOut,"acReceiver3=%s;",alarm.receiver3);
	fprintf(fOut,"acSMTPPort=%d;",alarm.port);
	fprintf(fOut,"acSMTPCrypto=%s;",alarm.crypto);

	fprintf(fOut, "bAlarmEnable=%d;", alarm.bEnable);
	fprintf(fOut, "bAlarmSound=%d;", alarm.bAlarmSoundEnable);
	fprintf(fOut, "bAlarmLight=%d;", alarm.bAlarmLightEnable);
    fprintf(fOut,"vmsServerIp=%s;",alarm.vmsServerIp);
    fprintf(fOut,"vmsServerPort=%d;",alarm.vmsServerPort);
	for(i=0; i<MAX_ALATM_TIME_NUM; i++)
	{
		if(strlen(alarm.alarmTime[i].tStart) > 0)
			fprintf(fOut, "alarmTime%d=%s-%s;", i, alarm.alarmTime[i].tStart, alarm.alarmTime[i].tEnd);
	}

	for(i=0; i<5; i++)
	{
		fprintf(fOut, "Schedule_bEn%d=%d;", i+1, alarm.m_Schedule[i].bEnable);
		fprintf(fOut, "Schedule_time%d=%d:%d;", i+1, alarm.m_Schedule[i].Schedule_time_H,alarm.m_Schedule[i].Schedule_time_M);
		fprintf(fOut, "Schedule_num%d=%d;", i+1, alarm.m_Schedule[i].num);
		fprintf(fOut, "Schedule_interval%d=%d;", i+1, alarm.m_Schedule[i].Interval);
	}

	//lk20131129报警输入
	MAlarmIn_t alarmin;
	malarmin_get_param(0, &alarmin);
	fprintf(fOut, "alarmin_bEnable=%d;",alarmin.bEnable);
	fprintf(fOut, "alarmin_bNormallyClosed=%d;",alarmin.bNormallyClosed);
	fprintf(fOut, "alarmin_bSendtoClient=%d;",alarmin.bSendtoClient);
	fprintf(fOut, "alarmin_bSendtoVMS=%d;",alarmin.bSendtoVMS);
	fprintf(fOut, "alarmin_bSendEmail=%d;",alarmin.bSendEmail);
	fprintf(fOut, "alarmin_bEnRecord=%d;",alarmin.bEnableRecord);
	fprintf(fOut, "alarmin_u8Num=%d;",alarmin.u8AlarmNum);
	fprintf(fOut, "alarmin_bBuzzing=%d;",alarmin.bBuzzing);

	//报警传感器
	MDoorAlarm_t *doorAlarmCfg = mdooralarm_get_param(NULL);
	fprintf(fOut, "bDoorASupport=%d;", hwinfo.bSupportMCU433);
	fprintf(fOut, "dooralarm_bEnable=%d;", doorAlarmCfg->bEnable);
	fprintf(fOut, "dooralarm_bSendtoClient=%d;", doorAlarmCfg->bSendtoClient);
	fprintf(fOut, "dooralarm_bSendtoVMS=%d;", doorAlarmCfg->bSendtoVMS);
	fprintf(fOut, "dooralarm_bSendEmail=%d;", doorAlarmCfg->bSendEmail);
	fprintf(fOut, "dooralarm_bEnRecord=%d;", doorAlarmCfg->bEnableRecord);
	fprintf(fOut, "dooralarm_bBuzzing=%d;", doorAlarmCfg->bBuzzing);

    //视频遮挡
    mprivacy_get_param(0, &region);
    fprintf(fOut, "bCoverRgn=%d;", region.bEnable);
	for(i=0; i<MAX_PYRGN_NUM; i++)
	{
		RECT *pRect=&region.stRect[i];
		fprintf(fOut, "Region%d=%d,%d,%d,%d;", i, pRect->x, pRect->y, pRect->w, pRect->h);
	}

	//??????
	mdetect_get_param(0, &md);
	fprintf(fOut, "bMDEnable=%d;", md.bEnable);
	fprintf(fOut, "nMDSensitivity=%d;", md.nSensitivity);
	fprintf(fOut, "detect_bEnRecord=%d;",md.bEnableRecord);
	fprintf(fOut, "nMDDelay=%d;", md.nDelay);
	for(i=0; i<MAX_MDRGN_NUM; i++)
	{
		RECT *pRect=&md.stRect[i];
		fprintf(fOut, "MDRegion%d=%d,%d,%d,%d;", i, pRect->x, pRect->y, pRect->w, pRect->h);
	}
	fprintf(fOut, "nMDOutClient=%d;", md.bOutClient);
	fprintf(fOut, "nMDOutEMail=%d;", md.bOutEMail);
	fprintf(fOut, "nMDOutVMS=%d;", md.bOutVMS);
	fprintf(fOut, "nMDOutBuzzing=%d;", md.bBuzzing);
	
#ifdef MD_GRID_SET
	fprintf(fOut, "MovGridW=%d;", md.nColumn);
	fprintf(fOut, "MovGridH=%d;", md.nRow);
	int md_index=0;
	fprintf(fOut, "MovGrid=%11d,", md.nRegion[md_index++]);
	for(;md_index<md.nRow-1;md_index++)
	{
		fprintf(fOut, "%11d,", md.nRegion[md_index]);
	}
	fprintf(fOut, "%11d;", md.nRegion[md_index++]);
#endif
	
	//sensor,目前只有一个sensor,lck20120726
	msensor_getparam(&stSensorAttr);
	fprintf(fOut, "sensorType=%d;", hwinfo.sensor);
	fprintf(fOut, "rotateBSupport=%d;", hwinfo.rotateBSupport);
	fprintf(fOut, "bAEME=%d;", stSensorAttr.ae.bAEME);
	fprintf(fOut, "bByPassAE=%d;", stSensorAttr.ae.bByPassAE);
	fprintf(fOut, "exposureMin=%d;", stSensorAttr.ae.exposureMin);
	fprintf(fOut, "exposureMax=%d;", stSensorAttr.ae.exposureMax);
	fprintf(fOut, "u16DGainMax=%d;", stSensorAttr.ae.u16DGainMax);
	fprintf(fOut, "u16DGainMin=%d;", stSensorAttr.ae.u16DGainMin);
	fprintf(fOut, "u16AGainMax=%d;", stSensorAttr.ae.u16AGainMax);
	fprintf(fOut, "u16AGainMin=%d;", stSensorAttr.ae.u16AGainMin);
	fprintf(fOut, "u32ISPDGainMax=%d;", stSensorAttr.ae.u32ISPDGainMax);
	fprintf(fOut, "u32SystemGainMax=%d;", stSensorAttr.ae.u32SystemGainMax);
	fprintf(fOut, "s32AGain=%d;", stSensorAttr.ae.s32AGain);
	fprintf(fOut, "s32DGain=%d;", stSensorAttr.ae.s32DGain);
	fprintf(fOut, "u32ISPDGain=%d;", stSensorAttr.ae.u32ISPDGain);
	fprintf(fOut, "u32ExpLine=%d;", stSensorAttr.ae.u32ExpLine);
	fprintf(fOut, "bManualAGainEnable=%d;", stSensorAttr.ae.bManualAGainEnable);
	fprintf(fOut, "bManualDGainEnable=%d;", stSensorAttr.ae.bManualDGainEnable);
	fprintf(fOut, "bManualExpLineEnable=%d;", stSensorAttr.ae.bManualExpLineEnable);
	fprintf(fOut, "bManualISPGainEnable=%d;", stSensorAttr.ae.bManualISPGainEnable);
	fprintf(fOut, "bDRCEnable=%d;", stSensorAttr.drc.bDRCEnable);
	fprintf(fOut, "brightness=%d;", stSensorAttr.brightness);
	fprintf(fOut, "exposure=%d;", stSensorAttr.exposure);
	fprintf(fOut, "saturation=%d;", stSensorAttr.saturation);
    fprintf(fOut, "contrast=%d;", stSensorAttr.contrast);
    fprintf(fOut, "sharpness=%d;", stSensorAttr.sharpness);
    fprintf(fOut, "antifog=%d;", stSensorAttr.antifog);
	fprintf(fOut, "light=%d;", stSensorAttr.light);
    fprintf(fOut, "bSupportWdr=%d;", stSensorAttr.bSupportWdr);
    fprintf(fOut, "bEnableWdr=%d;", stSensorAttr.bEnableWdr);
	fprintf(fOut, "bSupportSl=%d;", stSensorAttr.bSupportSl);
    fprintf(fOut, "bEnableSl=%d;", stSensorAttr.bEnableSl);
    fprintf(fOut, "daynightMode=%d;", stSensorAttr.daynightMode);
    fprintf(fOut, "dayStart=%02d:%02d;", stSensorAttr.dayStart.hour, stSensorAttr.dayStart.minute);
    fprintf(fOut, "dayEnd=%02d:%02d;", stSensorAttr.dayEnd.hour, stSensorAttr.dayEnd.minute);
    fprintf(fOut, "effect_flag=%d;", stSensorAttr.effect_flag);
    fprintf(fOut, "sw_cut=%d;", stSensorAttr.sw_cut);
    fprintf(fOut, "cut_rate=%d;", stSensorAttr.cut_rate);
    fprintf(fOut, "sence=%d;", stSensorAttr.sence);
    fprintf(fOut, "cutDelay=%d;", stSensorAttr.cutDelay);
    fprintf(fOut, "rotate=%d;", stSensorAttr.rotate);
    fprintf(fOut, "AutoLowFrameEn=%d;", stSensorAttr.AutoLowFrameEn);
	fprintf(fOut, "exp_mode=%d;", stSensorAttr.exp_mode);
	fprintf(fOut, "bSupportRedWhiteCtrl=%d;", hwinfo.bSupportRedWhiteCtrl);
	fprintf(fOut, "bRedWhiteCtrlEnabled=%d;", msensor_get_whitelight_function());
	
    {
		MLE_t	stLocalexposure;
		msensor_get_local_exposure(0,&stLocalexposure);
		int j;
		for (j = 0; j < stLocalexposure.nLE_RectNum; j++)
		{
			fprintf(fOut, "LERegion%d=%d,%d,%d,%d;", j,
					stLocalexposure.stLE_Rect[j].x, stLocalexposure.stLE_Rect[j].y,
					stLocalexposure.stLE_Rect[j].w, stLocalexposure.stLE_Rect[j].h);
		}
		fprintf(fOut, "LEReverse=%d;", stLocalexposure.bLE_Reverse);
		fprintf(fOut, "LEWeight=%d;", stLocalexposure.nLE_Weight);
    }
	//[CH*],以下为针对每个通道的配置
	//OSD
	for (i=0;i<HWINFO_STREAM_CNT;i++)
	{
		mchnosd_get_param(i, &osd);
		fprintf(fOut, "[CH%d];", i+1);
		fprintf(fOut, "bShowOSD=%d;", osd.bShowOSD);
		fprintf(fOut, "nPosition=%d;", osd.position);
		fprintf(fOut, "nTimePosition=%d;", osd.timePos);
		fprintf(fOut, "bLargeOSD=%d;", osd.bLargeOSD);
		fprintf(fOut, "nOSDbInvColEn=%d;", osd.osdbInvColEn);
		fprintf(fOut, "timeFormat=%s;", osd.timeFormat);
		fprintf(fOut, "channelName=%s;", osd.channelName);
	}

	//录制
	/*for (i=0;i<MAX_REC_TASK;i++)*/
	{
		/* 录像设置只有一个通道 */
		mrecord_get_param(0, &recAttr);
		fprintf(fOut, "[CH%d];", 1);
		fprintf(fOut, "bRecEnable=%d;", recAttr.bEnable);
		fprintf(fOut, "RecFileLength=%d;",recAttr.file_length);
		fprintf(fOut, "bRecDisconEnable=%d;", recAttr.discon_enable);
		fprintf(fOut, "bRecAlarmEnable=%d;", recAttr.alarm_enable);
		fprintf(fOut, "bRecTimingEnable=%d;", recAttr.timing_enable);
		fprintf(fOut, "RecTimingStart=%d;", recAttr.timing_start);
		fprintf(fOut, "RecTimingStop=%d;", recAttr.timing_stop);
		fprintf(fOut, "detecting=%d;", recAttr.detecting);
		fprintf(fOut, "alarming=%d;", recAttr.alarming);
		fprintf(fOut, "ivping=%d;", recAttr.ivping);
		fprintf(fOut, "bRecChFrameEnable=%d;",recAttr.chFrame_enable);
		fprintf(fOut, "alarm_pre_record=%d;", recAttr.alarm_pre_record);
		fprintf(fOut, "alarm_duration=%d;", recAttr.alarm_duration);
		// 录像模式
		if(hwinfo.bHomeIPC)
		{
			if(recAttr.bEnable)
				fprintf(fOut, "storageMode=%d;", RECORD_MODE_NORMAL);	// 手动录像
			else if(recAttr.chFrame_enable)
				fprintf(fOut,"storageMode=%d;", RECORD_MODE_CHFRAME); // 抽帧录像
			else if(recAttr.alarm_enable || recAttr.alarming || recAttr.detecting||recAttr.ivping)
				fprintf(fOut, "storageMode=%d;", RECORD_MODE_ALARM);	// 报警录像
			else
				fprintf(fOut,"storageMode=%d;", RECORD_MODE_STOP);		//停止录像
			fprintf(fOut,"chFrameSec=%d;", recAttr.chFrameSec);
		}
	}

	//数据流
    for(i=0; i<HWINFO_STREAM_CNT; i++)
    {
		mstream_get_param(i,&stAttr);
		fprintf(fOut, "[CH%d];", i+1);
		fprintf(fOut, "bAudioEn=%d;",stAttr.bAudioEn);
		fprintf(fOut, "width=%d;", stAttr.width);
		fprintf(fOut, "height=%d;", stAttr.height);
		fprintf(fOut, "framerate=%d;", stAttr.framerate);
		fprintf(fOut, "nGOP_S=%d;", stAttr.nGOP_S);
		fprintf(fOut, "bitrate=%d;", stAttr.bitrate);
		fprintf(fOut, "rcMode=%d;", stAttr.rcMode);
		fprintf(fOut, "minQP=%d;", stAttr.minQP);
		fprintf(fOut, "maxQP=%d;", stAttr.maxQP);
		fprintf(fOut, "bRectStretch=%d;", stAttr.bRectStretch);
		fprintf(fOut, "vencType=%d;", stAttr.vencType);
		if(hwinfo.bSupportLDC)
		{
			fprintf(fOut,"bSupportLDC=%d;",hwinfo.bSupportLDC);
			fprintf(fOut,"bLDCEnable=%d;",stAttr.bLDCEnable);
			fprintf(fOut,"ldcRatio=%d;",stAttr.nLDCRatio);
		}

    }
    mstream_get_roi(&roi);
	for (i=0;i<MAX_ROI_REGION;i++)
	{
		fprintf(fOut, "roiRegion%d=%d,%d,%d,%d;", i, roi.roi[i].x,roi.roi[i].y,roi.roi[i].w,roi.roi[i].h);
	}
	fprintf(fOut,"roiReverse=%d;",roi.ior_reverse);
	fprintf(fOut,"roiWeight=%d;",roi.roiWeight);

	fprintf(fOut,"MobileQuality=%d;", SctrlGetParam(NULL)->nPictureType);

    jv_audio_attr_t aiAttr, aoAttr;
    jv_ai_get_attr(0, &aiAttr);
    jv_ao_get_attr(0, &aoAttr);
	fprintf(fOut, "encType=%d;", isVoiceSetting() ? 2 : aiAttr.encType);  //声波配置时，配置文件不能保存成pcm类型，防止插网线启动时，声音异常

	//PTZ
	i=0;
	fprintf(fOut, "[CH%d];", i+1);
	fprintf(fOut,"moveSpeed=%d;",abs(msoftptz_speed_get(0)));
	PTZ *pPtz = PTZ_GetInfo();
	fprintf(fOut, "nPTZAddr=%d;", pPtz[i].nAddr);
	fprintf(fOut, "nprotocol=%d;", pPtz[i].nProtocol);
	fprintf(fOut, "nBaud=%d;",  pPtz[i].nBaudRate );
	fprintf(fOut, "nCharSize=%d;", pPtz[i].nHwParams.nCharSize );
	fprintf(fOut, "nStopBit=%d;", pPtz[i].nHwParams.nStopBit );
	fprintf(fOut, "nParityBit=%d;", pPtz[i].nHwParams.nParityBit );
	fprintf(fOut, "nFlowCtl=%d;", pPtz[i].nHwParams.nFlowCtl );
	fprintf(fOut, "bLRSW=%d;", pPtz[i].bLeftRightSwitch);
	fprintf(fOut, "bUDSW=%d;", pPtz[i].bUpDownSwitch);
	fprintf(fOut, "bIZSW=%d;", pPtz[i].bIrisZoomSwitch);
	fprintf(fOut, "bFZSW=%d;", pPtz[i].bFocusZoomSwitch);
	fprintf(fOut, "bZSW=%d;", pPtz[i].bZoomSwitch);
	fprintf(fOut, "scanSpeed=%d;", pPtz[i].scanSpeed);

	PTZ_PRESET_INFO *preInfo = PTZ_GetPreset();
	int j;
	for(j=0; j<preInfo[i].nPresetCt; j++)
	{
		//printf("write:%s\n",preInfo[i].name[j]);
		fprintf(fOut, "preset=%02x%s;",preInfo[i].pPreset[j], preInfo[i].name[j]);
//		fprintf(fOut, "lenpreset=%02x%04x%04x;",preInfo[i].pPreset[j],preInfo[i].lenzoompos[j], preInfo[i].lenfocuspos[j]);
	}

	PTZ_PATROL_INFO *patrol = PTZ_GetPatrol();
	fprintf(fOut, "patrolSpeed=%d;", patrol[i].nPatrolSpeed);
	for (j=0;j<patrol[i].nPatrolSize;j++)
	{
		fprintf(fOut, "aPatrol=%02x%08x;", patrol[i].aPatrol[j].nPreset, patrol[i].aPatrol[j].nStayTime);
	}
	fprintf(fOut, "2patrolSpeed=%d;", patrol[i+1].nPatrolSpeed);
	for (j=0;j<patrol[i+1].nPatrolSize;j++)
	{
		fprintf(fOut, "2aPatrol=%02x%08x;", patrol[i+1].aPatrol[j].nPreset, patrol[i+1].aPatrol[j].nStayTime);
	}

	PTZ_GUARD_T *guard = PTZ_GetGuard();
	Printf("%s	%d:		guard time:%d  preset:%d  type:%d\n",__FUNCTION__, __LINE__, guard[i].guardTime, 
		guard[i].nRreset,guard[i].guardType);
	fprintf(fOut, "guardTime=%d;", guard[i].guardTime);
	fprintf(fOut, "guardType=%d;", guard[i].guardType);
	fprintf(fOut, "guardValue=%d;", guard[i].nRreset);

	int item = PTZ_GetBootConfigItem();
	fprintf(fOut, "bootItem=%d;", item);
	
	PTZ_SCHEDULE_INFO *schdule = PTZ_GetSchedule();
	fprintf(fOut, "bSch1En=%d;", schdule[i].bSchEn[0]);
	fprintf(fOut, "bSch2En=%d;", schdule[i].bSchEn[1]);
	fprintf(fOut, "schedule1=%d;", schdule[i].schedule[0]);
	fprintf(fOut, "schedule2=%d;", schdule[i].schedule[1]);
	fprintf(fOut, "sch1TimeStart=%d:%d;", schdule[i].schTimeStart[0].hour, schdule[i].schTimeStart[0].minute);
	fprintf(fOut, "sch1TimeEnd=%d:%d;", schdule[i].schTimeEnd[0].hour, schdule[i].schTimeEnd[0].minute);
	fprintf(fOut, "sch2TimeStart=%d:%d;", schdule[i].schTimeStart[1].hour, schdule[i].schTimeStart[1].minute);
	fprintf(fOut, "sch2TimeEnd=%d:%d;", schdule[i].schTimeEnd[1].hour, schdule[i].schTimeEnd[1].minute);

	//add for Homeipc
	fprintf(fOut, "HomeIPCFlag=%d;", hwinfo.bHomeIPC);
	fprintf(fOut, "bSupportAESens=%d;", hwinfo.bSupportAESens);		// 软光敏灵敏度调节
	fprintf(fOut,"chatMode=%d;",hwinfo.remoteAudioMode);			// 对讲模式
	fprintf(fOut,"softptzsupport=%d;",hwinfo.bSoftPTZ);				// 软云台
	fprintf(fOut,"bSupport3DLocate=%d;",hwinfo.bSupport3DLocate);	// 3D定位
	fprintf(fOut,"bSupportTimePointPlay=%d;", 1);	//是否支持时间轴形式回放。目前都支持

	//add for check whether has eth0 network
	fprintf(fOut,"HasEth=%d;",utl_ifconfig_bSupport_ETH_check());

	MIVP_t mivp;
	mivp_get_param(0,&mivp);
	fprintf(fOut, "IVPEnable=%d;", mivp.st_rl_attr.bEnable);
	fprintf(fOut, "IVPRgnCnt=%d;", mivp.st_rl_attr.nRgnCnt);
	acItem[0]='\0';
	for(i = 0; i<mivp.st_rl_attr.nRgnCnt; i++)
	{
		sprintf(tmp, "IVPRegion%d=%d:%d:", i, mivp.st_rl_attr.stRegion[i].nIvpCheckMode, mivp.st_rl_attr.stRegion[i].nCnt);
		strcat(acItem, tmp);
		for (j = 0; j < mivp.st_rl_attr.stRegion[i].nCnt; j++)
		{
			sprintf(tmp, "%d,%d-", mivp.st_rl_attr.stRegion[i].stPoints[j].x,
					mivp.st_rl_attr.stRegion[i].stPoints[j].y);
			strcat(acItem, tmp);
		}
		acItem[strlen(acItem) - 1] = ';';
	}
	fprintf(fOut, "%s", acItem);
	/*******攀爬***************/
	acItem[0]='\0';
	sprintf(tmp, "nClimbPoints=%d:", mivp.st_rl_attr.stClimb.nPoints);
	strcat(acItem, tmp);
	for(j = 0; j<mivp.st_rl_attr.stClimb.nPoints; j++)
	{
		int x = mivp.st_rl_attr.stClimb.stPoints[j].x ;
		int y = mivp.st_rl_attr.stClimb.stPoints[j].y ;
		sprintf(tmp, "%d,%d-",x,y);
		strcat(acItem, tmp);
	}
//	if(mivp.st_rl_attr.stClimb.nPoints > 0)
		acItem[strlen(acItem) - 1] = ';';
//	else
//		acItem[strlen(acItem) ] = ';';
	fprintf(fOut, "%s", acItem);
	fprintf(fOut, "bDrawFrame=%d;", mivp.st_rl_attr.bDrawFrame);
	fprintf(fOut, "bFlushFrame=%d;", mivp.st_rl_attr.bFlushFrame);
	fprintf(fOut, "bMarkAll=%d;", mivp.st_rl_attr.bMarkAll);

	fprintf(fOut, "IVPAlpha=%d;", mivp.st_rl_attr.nAlpha);
	fprintf(fOut, "IVPRecEnable=%d;", mivp.st_rl_attr.stAlarmOutRL.bEnableRecord);
	fprintf(fOut, "IVPOutAlarm1=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutAlarm1);
	fprintf(fOut, "IVPAlarmSound=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutSound);
	fprintf(fOut, "IVPOutClient=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutClient);
	fprintf(fOut, "IVPOutEMail=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutEMail);
	fprintf(fOut, "IVPOutVMS=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutVMS);
	fprintf(fOut, "IVPSen=%d;", mivp.st_rl_attr.nSen);
	fprintf(fOut, "IVPThreshold=%d;", mivp.st_rl_attr.nThreshold);
	fprintf(fOut, "IVPStayTime=%d;", mivp.st_rl_attr.nStayTime);
	
	fprintf(fOut, "IVPMarkObject=%d;", mivp.st_rl_attr.bMarkObject);
	fprintf(fOut, "IVPPlateSnap=%d;", mivp.bPlateSnap);
	fprintf(fOut, "IVPSnapRes=%s;", mivp.sSnapRes);
	
	//占有率分析
	fprintf(fOut, "bVREnable=%d;", mivp.st_vr_attr.bVREnable);
	fprintf(fOut, "nVRThreshold=%d;", mivp.st_vr_attr.nVRThreshold);
	fprintf(fOut, "nVRSen=%d;", mivp.st_vr_attr.nSen);
	
	//客流统计
	MIVP_t mivp_count;
	mivp_count_get(0,&mivp_count);
	fprintf(fOut, "bMarkObject=%d;", mivp_count.st_count_attr.bMarkObject);
	fprintf(fOut, "bOpenCount=%d;", mivp_count.st_count_attr.bOpenCount);
	fprintf(fOut, "bShowCount=%d;", mivp_count.st_count_attr.bShowCount);
	fprintf(fOut, "bDrawCountLine=%d;", mivp_count.st_count_attr.bDrawFrame);
	fprintf(fOut, "IVPCountOSDPos=%d;", mivp_count.st_count_attr.eCountOSDPos);
	fprintf(fOut, "IVPCountOSDColor=%d;", mivp_count.st_count_attr.nCountOSDColor);
	fprintf(fOut, "nCountSaveDays=%d;", mivp_count.st_count_attr.nCountSaveDays);
	fprintf(fOut, "CheckCountMode=%d;", mivp_count.st_count_attr.nCheckMode);
	acItem[0]='\0';
	sprintf(tmp, "nCountPoints=%d:", mivp_count.st_count_attr.nPoints);
	strcat(acItem, tmp);
	for(j = 0; j<mivp_count.st_count_attr.nPoints; j++)
	{
		sprintf(tmp, "%d,%d-",mivp_count.st_count_attr.stPoints[j].x,mivp_count.st_count_attr.stPoints[j].y);
		strcat(acItem, tmp);
	}	
	if(mivp_count.st_count_attr.nPoints > 0)
		acItem[strlen(acItem) - 1] = ';';
	else
		acItem[strlen(acItem) ] = ';';
	fprintf(fOut, "%s", acItem);
	printf("%s line %d rl_ben %d count_ben %d  nCnt %d\n  ",__func__,__LINE__, mivp.st_rl_attr.bEnable,mivp_count.st_count_attr.bOpenCount,mivp.st_rl_attr.nRgnCnt);
	//移动目标检测
	fprintf(fOut, "bIVPDetectEn=%d;",mivp.st_detect_attr.bDetectEn);
	//遮挡报警
	fprintf(fOut, "bIVPHideAlarmEn=%d;",mivp.st_hide_attr.bHideEnable);
	fprintf(fOut, "bIVPHideThreshold=%d;",mivp.st_hide_attr.nThreshold);
	fprintf(fOut, "bIVPHideAlarmRecEn=%d;", mivp.st_hide_attr.stHideAlarmOut.bEnableRecord);
	fprintf(fOut, "bIVPHideAlarmOut1=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutAlarm1);
	fprintf(fOut, "bIVPHideAlarm2Client=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutClient);
	fprintf(fOut, "bIVPHideAlarm2Email=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutEMail);
	fprintf(fOut, "bIVPHideAlarm2VMS=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutVMS);
	fprintf(fOut, "bIVPHideSound=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutSound);
	
	//拿取遗留报警
	fprintf(fOut, "bIVPTLEnable=%d;", mivp.st_tl_attr.bTLEnable);
	fprintf(fOut, "bIVPTLMode=%d;", mivp.st_tl_attr.nTLMode);
	fprintf(fOut, "IVPTLRgnCnt=%d;", mivp.st_tl_attr.nTLRgnCnt);
	{
		acItem[0]='\0';
		for (i = 0; i < mivp.st_tl_attr.nTLRgnCnt; i++)
		{
			sprintf(tmp, "IVPTLRegion%d=%d:%d:", i, mivp.st_tl_attr.stTLRegion[i].nIvpCheckMode,mivp.st_tl_attr.stTLRegion[i].nCnt);
			strcat(acItem, tmp);
			for (j = 0; j < mivp.st_tl_attr.stTLRegion[i].nCnt; j++)
			{
				sprintf(tmp, "%d,%d-", mivp.st_tl_attr.stTLRegion[i].stPoints[j].x,
						mivp.st_tl_attr.stTLRegion[i].stPoints[j].y);
				strcat(acItem, tmp);
			}
			acItem[strlen(acItem) - 1] = ';';
		}
		fprintf(fOut, "%s", acItem);
	}
	fprintf(fOut, "IVPTLRecEnable=%d;", mivp.st_tl_attr.stTLAlarmOut.bEnableRecord);
	fprintf(fOut, "IVPTLOutAlarm1=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutAlarm1);
	fprintf(fOut, "IVPTLOutClient=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutClient);
	fprintf(fOut, "IVPTLOutEMail=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutEMail);
	fprintf(fOut, "IVPTLOutVMS=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutVMS);
	fprintf(fOut, "IVPTLSen=%d;", mivp.st_tl_attr.nTLSen);
	fprintf(fOut, "IVPTLAlarmDuration=%d;", mivp.st_tl_attr.nTLAlarmDuration);
	fprintf(fOut, "IVPTLSuspectTime=%d;", mivp.st_tl_attr.nTLSuspectTime);
	
	/*******************************************************************************智能分析*************************/

	// 云存储
	fprintf(fOut, "CloudSupport=%d;", hwinfo.bSupportXWCloud);
	CLOUD cloud;
	mcloud_get_param(&cloud);
	fprintf(fOut, "bCloudEnable=%d;", cloud.bEnable);
	fprintf(fOut, "CloudID=%s;", cloud.acID);
	fprintf(fOut, "CloudPwd=%s;", cloud.acPwd);
	fprintf(fOut, "CloudHost=%s;", cloud.host);
	fprintf(fOut, "CloudBkt=%s;", cloud.bucket);
	fprintf(fOut, "CloudType=%d;", cloud.type);
	fprintf(fOut, "CloudExp=%s;", cloud.expireTime);

	fclose(fOut);
	fp = fopen(TUTK_FILE, "w+");
	if (fp == NULL)
	{
		printf("Write To %s Failed\n", TUTK_FILE);
		pthread_rwlock_unlock(&g_reboot_rwlock);
		return;
	}

	fprintf(fp,"bSupportTUTK=%d;", 0);
	memset(ipcinfo.tutkid,0,MAX_TUTK_UID_NUM);
	fprintf(fp,"tutkid=%s;", ipcinfo.tutkid);

	fclose(fp);
	pthread_rwlock_unlock(&g_reboot_rwlock);
	ImportantCfg_t im;
	im.nLanguage = ipcinfo.nLanguage;
	im.nTimeFormat = gp.nTimeFormat;
	im.ystID = ipcinfo.ystID;
	__write_important_cfg(&im);
	 
}

char *SYSFuncs_GetKeyValue(const char *pBuffer, const char *key, char *valueBuf, int maxValueBuf)
{
	int i = 0;
	char *ptmpItem;
	char *pValue = NULL;  //gyd
	int klen = strlen(key);
	while (pBuffer && *pBuffer)     //gyd
	{
		if (strncmp(pBuffer, key, klen) == 0)
		{
			if (pBuffer[klen] == '=')
			{
				int i = 0;
				pBuffer += klen+1;
				while(*pBuffer && *pBuffer != ';' && i < maxValueBuf)
					valueBuf[i++] = *pBuffer++;
				valueBuf[i] = '\0';
//				printf("find key: %s, value: %s\n", key, valueBuf);
				return valueBuf;
			}
		}
		pBuffer = strchr(pBuffer, ';');
		if (pBuffer && pBuffer[0])
			pBuffer++;
	}

	return NULL;
}

/**
 *@brief 获取指定的参数
 */
char *SYSFuncs_GetValue(const char *fname, const char *key, char *value, int maxValueLen)
{
	char *pItem, *pValue;
	U32	i = 0, nCh=0, nRead=0, nOffset=0;
	char	acData[16*1024]={0};
	S32 fdCfg;
	int j;

	//打开配置文件，设置系统信息
	fdCfg = open(fname, O_RDONLY);
	if(fdCfg < 0)
	{
		Printf("Read config file %s failed.\n", fname);
		return NULL;
	}
	while((nRead=read(fdCfg, acData+nOffset, 4096)) > 0)
	{
		nOffset += nRead;
	}
	close(fdCfg);
	nOffset = 0;

	return SYSFuncs_GetKeyValue(acData, key, value, maxValueLen);
}

void clear_bootupdate_flag()
{
	char buffer[128];
	char mod[64];
	unsigned int phy_addr;
	int ret = utl_system_ex("fw_printenv UPDATE", buffer, sizeof(buffer));
	//UPDATE=netupdate JVS-HI3516C8M 0x84400000
	int n = sscanf(buffer, "UPDATE=netupdate %s 0x%x", mod, &phy_addr);

	printf("mod: %s, phy_addr: 0x%x, n: %d\n", mod, phy_addr, n);
	if (n != 2)
	{
		printf("Failed get uboot info\n");
		return ;
	}
#define PAGE_SIZE 0x1000
#define PAGE_SIZE_MASK 0xfffff000
	unsigned int size = 1024;
	unsigned int phy_addr_in_page;
	unsigned int page_diff;

	unsigned int size_in_page;
	int fd = open ("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0)
	{
		printf("memmap():open %s error!\n", "/dev/mem");
		return ;
	}
	phy_addr_in_page = phy_addr & PAGE_SIZE_MASK;
	page_diff = phy_addr - phy_addr_in_page;

	/* size in page_size */
	size_in_page =((size + page_diff - 1) & PAGE_SIZE_MASK) + PAGE_SIZE;
	void *addr = mmap ((void *)0, size_in_page, PROT_READ|PROT_WRITE, MAP_SHARED, fd, phy_addr_in_page);
	if (addr == MAP_FAILED)
	{
		printf("memmap():mmap @ 0x%x error!\n", phy_addr_in_page);
		return ;
	}
	unsigned int *wanted = (unsigned int *)(addr+page_diff);
	printf("*wanted: %x\n", *wanted);
	*wanted = 0xFFFFFFFF;
	munmap(addr, size_in_page);
	close(fd);
}

/**
 *@brief 重启系统
 */
int SYSFuncs_reboot(void)
{
	clear_bootupdate_flag();
	mlog_write("Client Restart Device");
	pthread_rwlock_wrlock(&g_reboot_rwlock);
	gp.nExit	= EXIT_REBOOT;
	gp.bRunning	= FALSE;
	return 0;
}

/**
 *@brief 恢复系统默认值
 */
int SYSFuncs_factory_default(int bHardReset)
{
	mlog_write("Client Recover Device");
	pthread_rwlock_wrlock(&g_reboot_rwlock);
	gp.nExit	= bHardReset ? EXIT_RECOVERY : EXIT_SOFT_RECOVERY;
	gp.bRunning	= FALSE;
	return 0;
}

int SYSFuncs_factory_default_without_reboot()
{
	int i = 0;
	char acBuffer[512]={0};
	unsigned int imac[6] = {0};
	char inet[32];
	int ret = 0;
	int wifisetted = 0;

	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);

	wifisetted = utl_ifconfig_wifi_STA_configured();
	utl_system("rm -rf "CONFIG_PATH"network/*");
	sprintf(acBuffer, "rm %s %s %s %s %s",
		CONFIG_PATH"account.dat", CONFIG_FILE, GB28181_CFG_FILE, FIRSTPOS_FILE, PATROL_FLAG_FLAG);
	utl_system(acBuffer);
#ifdef IVP_COUNT_SUPPORT
	if(mivp_count_bsupport())
		utl_system("rm /etc/conf.d/jovision/ivpCount.cfg");
#endif


	printf("SYSFuncs_factory_default ReadConfigInfo, time=%ld\n", time(NULL));
	ReadConfigInfo();						//拿到前面来，防止声波配置过快，这个函数里面又改了 采样率

	jv_audio_attr_t ai_attr;
	jv_ai_get_attr(0, &ai_attr);
	ai_attr.level = -1;
	ai_attr.soundfile_level = -1;
	ai_attr.micGain= -1;
	ai_attr.micGainTalk = -1;
	
	maudio_setDefault_gain(&ai_attr);

	jv_ai_set_attr(0,&ai_attr);

	speakerowerStatus = JV_SPEAKER_OWER_NONE;
	
	if(!utl_ifconfig_b_linkdown("eth0"))
	{
		printf("SYSFuncs_factory_default VOICE_RESET, time=%ld\n", time(NULL));
		maudio_speaker(VOICE_RESET, TRUE,TRUE, TRUE);
		net_check_destroy();
		net_init(ipcinfo.nDeviceInfo[0]);
	}
	else
	{	
		printf("SYSFuncs_factory_default wifisetted is %d net_init, time=%ld\n", wifisetted,time(NULL));
		if (wifisetted == 1)
		{
			utl_system("sync");
			utl_system("echo 3 > /proc/sys/vm/drop_caches");	//8188模块reset时搜索不全，此处清空一下缓存
			Printf("sync finished!, time=%ld\n", time(NULL));
#ifdef SOUND_WAVE_DEC_SUPPORT
			if(hwinfo.bNewVoiceDec)
			{
				maudio_resetAIAO_mode(1);
				maudio_speaker(VOICE_NEW_WAITSET,TRUE,TRUE,FALSE);
			}
			else
			{
				maudio_resetAIAO_mode(2);
				maudio_speaker(VOICD_WAITSET, TRUE, TRUE, TRUE);
				maudio_resetAIAO_mode(1);
			}
#else
			maudio_resetAIAO_mode(2);
			maudio_speaker(VOICD_WAITSET, TRUE, TRUE, TRUE);
			maudio_resetAIAO_mode(1);
#endif
			net_check_destroy();
			net_init(ipcinfo.nDeviceInfo[0]);
		}
		else
		{
			maudio_resetAIAO_mode(2);
			maudio_speaker(VOICD_WAITSET, TRUE, TRUE, TRUE);
			maudio_resetAIAO_mode(1);

			if (utl_ifconfig_bsupport_smartlink(utl_ifconfig_wifi_get_model()))
			{
				// 重启智联路由线程，彻底清除之前的智联配置
				utl_ifconfig_wifi_smart_connect_close(TRUE);
				utl_ifconfig_wifi_smart_connect();
			}
		}
	}
		
	printf("WriteConfigInfo, time=%ld\n", time(NULL));
	WriteConfigInfo();
	printf("PTZ_Release, time=%ld\n", time(NULL));
	PTZ_Release();
	printf("PTZ_Init, time=%ld\n", time(NULL));
	PTZ_Init();
	printf("maccount_init, time=%ld\n", time(NULL));
	maccount_init();
	for(i=0; i<HWINFO_STREAM_CNT; i++)
	{
		printf("mstream_flush, time=%ld\n", time(NULL));
		mstream_flush(i);
		printf("mchnosd_flush, time=%ld\n", time(NULL));
		mchnosd_flush(i);
	}
	printf("msensor_flush, time=%ld\n", time(NULL));
	msensor_flush(0);
	printf("mprivacy_flush, time=%ld\n", time(NULL));
	mprivacy_flush(0);
	printf("mrecord_flush, time=%ld\n", time(NULL));
	mrecord_flush(0);
	printf("mdetect_flush, time=%ld\n", time(NULL));
	mdetect_flush(0);
	printf("malarmin_flush, time=%ld\n", time(NULL));
	malarmin_flush(0);
	printf("malarm_flush, time=%ld\n", time(NULL));
	malarm_flush();

#ifdef GB28181_SUPPORT
	mgb28181_deinit();
	mgb28181_init();
#endif

#ifdef YST_SVR_SUPPORT
	if (gp.bNeedYST)
	{
		remotecfg_deinit();
		remotecfg_init();
	}
#endif

	return 0;
}


