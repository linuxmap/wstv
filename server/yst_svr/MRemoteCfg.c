#include "jv_common.h"
#include <utl_filecfg.h>
#include "JvCDefines.h"
#include "JVNSDKDef.h"
#include "JvServer.h"
#include "SYSFuncs.h"
#include "mlog.h"
#include "mosd.h"
#include "mfirmup.h"
#include "mstream.h"
#include "MRemoteCfg.h"
#include "maccount.h"
#include "mtransmit.h"
#include "msnapshot.h"
#include "mipcinfo.h"
#include "mstorage.h"
#include "mprivacy.h"
#include "mdetect.h"
#include "malarmout.h"
#include "malarmin.h"
#include "sctrl.h"
#include "rcmodule.h"
#include "mrecord.h"
#include <webproxy.h>
#include <utl_ifconfig.h>
#include <utl_cmd.h>
#include <utl_queue.h>
#include <jv_ai.h>
#include <utl_net_lan.h>
#include "jv_ao.h"
#include "sgrpc.h"
#include "utl_iconv.h"
#include "mivp.h"
#include "utl_timer.h"
#include "mcloud.h"
#include "cgrpc.h"
#include "utl_iconv.h"
#include "muartcomm.h"
#include "mioctrl.h"
#include "mvoicedec.h"
#include "alarm_service.h"
#include "maudio.h"
#include "mptz.h"
#include "msensor.h"
#include "mdooralarm.h"
#include "mbizclient.h"
#include "utl_common.h"

static char acDll[IPCAM_MAX][MAX_PATH]=
{
	"/progs/res/IPCamCfg.dat.lzz",
	"/progs/res/IPCam_System.dat.lzz",
	"/progs/res/IPCam_Stream.dat.lzz",
	"/progs/res/IPCam_Storage.dat.lzz",
	"/progs/res/IPCam_Account.dat.lzz",
	"/progs/res/IPCam_Network.dat.lzz",
	"/progs/res/IPCam_LoginErr.dat.lzz",
	"/progs/res/IPCam_PTZ.dat.lzz",
	"/progs/res/IPCam_IVP.dat.lzz",
	"/progs/res/IPCam_Alarm.dat.lzz",
};

static jv_thread_group_t group;

VOID FirmupProc(REMOTECFG *cfg);
char *weber_extern(char *cmd, char *result);

static BOOL _CheckClientPower(U32 nClientID, U32 nPower)
{
	BOOL bPower	= FALSE;
	PNETLINKINFO pNetLinkInfo;
	NETLINKINFO info;

	pNetLinkInfo = __NETLINK_Get(nClientID, &info);
	if (NULL != pNetLinkInfo)
	{
		//判断权限
		if(pNetLinkInfo->stNetUser.nPower & nPower)
		{
			Printf("Verity, power ok..\n");
			bPower	= TRUE;
		}
	}
	return bPower;
}

//组合IPC参数
static VOID GetDVRParam(char *pData)
{
	char	acBuff[256]= {0};
	if(access(CONFIG_FILE, F_OK) || access(TUTK_FILE, F_OK))
	{
		//如果配置文件不存在则创建
		WriteConfigInfo();
	}
	pData[0] = '\0';
	//打开文件并组合发送字符串
	FILE *fIn	= fopen(CONFIG_FILE, "r");
	FILE *fp = fopen(TUTK_FILE, "r");
	if(fIn)
	{
		while (fgets(acBuff, 256, fIn))
		{
			//WriteConfigInfo并非每组参数独占一行进行存储，按照原来的处理，每调用一次fgets，就会误删两个字节，误增一个分号
			if(acBuff[strlen(acBuff)-1] == '\n')
				acBuff[strlen(acBuff)-1] = '\0';
			if(acBuff[strlen(acBuff)-1] == '\r')
				acBuff[strlen(acBuff)-1] = '\0';
			strcat(pData, acBuff);
			//strcat(pData, ";");
		}
		fclose(fIn);
	}

	if(fp)
	{
		while (fgets(acBuff, 256, fp))
		{
			if(acBuff[strlen(acBuff)-1] == '\n')
				acBuff[strlen(acBuff)-1] = '\0';
			if(acBuff[strlen(acBuff)-1] == '\r')
				acBuff[strlen(acBuff)-1] = '\0';
			strcat(pData, acBuff);
		}
		fclose(fp);
	}
}

//组合IPCAM数据
S32 LoadIPCamDlg(REMOTECFG *remotecfg, U32 nDll, BOOL bLoadDll, unsigned char *pData)
{
	S32 fdDlg, fdCfg, fdCfg_tutk;
	S32 nRead, nOffset=sizeof(S32);
	char acBuffer[256]= {0};
	int older;

	//此处作特殊处理，临时的代码，将来需要去掉,lck20120816
	if(-1 == remotecfg->nCh && IPCAM_NETWORK == nDll)
	{
		if(FALSE == _CheckClientPower(remotecfg->nClientID, POWER_ADMIN))
		{
			fdDlg = open(acDll[IPCAM_LOGINERR], O_RDONLY);
			if(fdDlg < 0)
			{
				Printf("Read dialog file %s failed.\n", acDll[nDll]);
				return 0;
			}
			while((nRead=read(fdDlg, pData+nOffset, 4096)) > 0)
			{
				nOffset += nRead;
			}
			close(fdDlg);
			memcpy(pData, &nOffset, sizeof(S32));

			//计算错误码
			sprintf(acBuffer, mlog_translate("Login failed [%X]"), maccount_get_errorcode());
			sprintf((char *)pData+nOffset, "%s", acBuffer);
			Printf("pData=%s\n", (char *)pData+nOffset);
			nOffset += strlen(acBuffer);

			Printf("%s\n", acBuffer);
			return nOffset;
		}
	}

	if(access(CONFIG_FILE, F_OK) || access(TUTK_FILE, F_OK))
	{
		//如果配置文件不存在则创建
		WriteConfigInfo();
	}

	if(bLoadDll)
	{
		fdDlg = open(acDll[nDll], O_RDONLY);
		if(fdDlg < 0)
		{
			Printf("Read dialog file %s failed.\n", acDll[nDll]);
			return nOffset;
		}
		while((nRead=read(fdDlg, pData+nOffset, 4096)) > 0)
		{
			nOffset += nRead;
		}
		close(fdDlg);
	}

	memcpy(pData, &nOffset, sizeof(S32));
	Printf("nOffset=%d\n", nOffset);

	older = nOffset;
	//打开文件并组合发送字符串
	fdCfg = open(CONFIG_FILE, O_RDONLY);
	if(fdCfg < 0)
	{
		Printf("Read config file %s failed.\n", CONFIG_FILE);
		return 0;
	}
	while((nRead=read(fdCfg, pData+nOffset, 4096)) > 0)
	{
		nOffset += nRead;
	}
	close(fdCfg);

	fdCfg_tutk = open(TUTK_FILE, O_RDONLY);
	if(fdCfg_tutk < 0)
	{
		Printf("Read config file %s failed.\n", TUTK_FILE);
		return 0;
	}
	while((nRead=read(fdCfg_tutk, pData+nOffset, 4096)) > 0)
	{
		nOffset += nRead;
	}
	close(fdCfg_tutk);
	//-------------------------追加的信息-------------------------
#if 1
	ACCOUNT stAccount;

	ACCOUNT *account = GetClientAccount(remotecfg->nClientID, &stAccount);
	if (account)
	{
		sprintf(acBuffer, "ClientPower=%d;", account->nPower);
		strcat((char *)pData+nOffset, acBuffer);
		nOffset += strlen(acBuffer);
	}
#endif

	YST stYST;
	GetYSTParam(&stYST);
	//添加版本信息
	sprintf(acBuffer, "Version=%s;" , IPCAM_VERSION);
	sprintf(acBuffer, "DEV_VERSION=%d;" , 1);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);
	//云视通分组
	sprintf(acBuffer, "YSTGROUP=%d;", ipcinfo_get_param(NULL)->nDeviceInfo[6]);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);
	//云视通号码，下面两项直接使用的全局变量访问，稍后进行修改,lck20120618
	sprintf(acBuffer, "YSTID=%d;", stYST.nID);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);
	//云视通在线状态
	sprintf(acBuffer, "YSTSTATUS=%d;", stYST.nStatus);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	//根据product中有芯片型号和软件特性，以便有不同默认值和功能
	sprintf(acBuffer, "IPCProduct=%s;", hwinfo.product);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	sprintf(acBuffer, "encryptCode=%d;", hwinfo.encryptCode);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	sprintf(acBuffer, "IPCIVPSupport=%d;", mivp_bsupport());	//智能分析的总开关
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	sprintf(acBuffer, "bIVPRALSupport=%d;", mivp_rl_bsupport());	//RAL:Regional And Line 区域入侵和绊线检测
	strcat((char *) pData + nOffset, acBuffer);
	nOffset += strlen(acBuffer);
	
	sprintf(acBuffer, "bIVPClimbSupport=%d;", mivp_climb_bsupport());
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);


	sprintf(acBuffer, "bIVPCOUNTSupport=%d;", mivp_count_bsupport());
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	sprintf(acBuffer, "bIVPHideSupport=%d;", mivp_hide_bsupport());
	strcat((char *) pData + nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	sprintf(acBuffer, "bIVPTLSupport=%d;", mivp_tl_bsupport());
	strcat((char *) pData + nOffset, acBuffer);
	nOffset += strlen(acBuffer);
	
	sprintf(acBuffer, "bIVPDetectSupport=%d;",mivp_detect_bsupport()); //移动目标侦测
	strcat((char *) pData + nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	/*是否支持云台，通过这个判断是否可以打开PTZ远程设置，暂时默认全部支持*/
	sprintf(acBuffer, "bPTZSupport=%d;", 1);
	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);

	strcat((char *)pData+nOffset, acBuffer);
	nOffset += strlen(acBuffer);
    
	//根据请求的dll追加相应的信息,lck20120412
	switch(nDll)
	{
	case IPCAM_SYSTEM:
		nOffset += build_storage_param((char *)pData+nOffset);
		nOffset += build_account_param((char *)pData+nOffset);
		break;
	case IPCAM_STREAM:
		//添加视频配置参数
		nOffset += build_stream_param((char *)pData+nOffset);
		nOffset += build_sensor_param((char *)pData+nOffset);
		break;
	case IPCAM_STORAGE:
		//添加存储器
		nOffset += build_storage_param((char *)pData+nOffset);
		break;
	case IPCAM_ACCOUNT:
		//添加帐户参数
		nOffset += build_account_param((char *)pData+nOffset);
		break;
	case IPCAM_NETWORK:
		//添加网络配置参数
		nOffset += build_network_param((char *)pData+nOffset);
		break;
	case IPCAM_ALARM:
		break;
	default:
		break;
	}

	Printf("load offset: %d\n", nOffset);

	return nOffset;
}

void _resolution_changed(int channelid)
{
	U8	pBuffer[256]= {0};
	JVS_FILE_HEADER_EX jHeaderEx;

	SctrlMakeHeader(channelid, &jHeaderEx);
	memcpy(pBuffer+2, &jHeaderEx, sizeof(jHeaderEx));
	if(gp.bNeedYST)
	{
		JVN_SendData(channelid+1, JVN_DATA_O, pBuffer, sizeof(JVS_FILE_HEADER_EX)+2, 0, 0);
	}

	U32 nChannel = SctrlGetMOChannel();
	if(nChannel == channelid)
	{
		PNETLINKINFO pNetLinkInfo;
		NETLINKINFO info;
		int i=0;
		while(1)
		{
			pNetLinkInfo = __NETLINK_GetByIndex(i, &info);
			if (pNetLinkInfo)
			{
				if(JVN_SCONNECTTYPE_MOCONNOK == pNetLinkInfo->nType)
				{
					if(gp.bNeedYST)
					{
						JVN_SendDataTo(pNetLinkInfo->nClientID, JVN_DATA_O, pBuffer, sizeof(JVS_FILE_HEADER_EX)+2, jHeaderEx.wVideoWidth, jHeaderEx.wVideoHeight);
					}
					Printf("Send resolution change to mobile...\n");
				}
			}
			else
			{
				break;
			}
			i++;
		}
	}
}

NETWORKINFO  stNetworkInfo;

static void __ipset(unsigned int naddr, char addr[])
{
	unsigned char* pIP = (unsigned char*)&naddr;
	sprintf(addr, "%d.%d.%d.%d", pIP[3], pIP[2], pIP[1], pIP[0]);
}

//设置DVR参数
VOID SetDVRParam(REMOTECFG *remotecfg, char *pData)
{
	U32 nCh=0, nLen = 1;
	char acBuff[256]= {0};

	//记录哪个模块的参数发生了改变
	BOOL bWebServerModify	= FALSE;
	BOOL bPrivacyModify		= FALSE;
	BOOL bRSLModify			= FALSE;
	BOOL bAlarmModify		= FALSE;
	BOOL bAudioModify		= FALSE;
	BOOL bAlarmInModify  	= FALSE;
	BOOL bOSDModify  	= FALSE;
	BOOL bReboot			= FALSE;
    BOOL bDdnsModify		= FALSE;
	
	char *pItem;
	char *pValue;
	int value;
	YST stYST;
	REGION region;
	ALARMSET alarm;
	ipcinfo_t ipcinfo;
	int ipcinfoSetted = 0;
	mstream_attr_t stAttr[MAX_STREAM];
	jv_audio_attr_t audioInfo;				// audio info lk20131125
	MAlarmIn_t alarminInfo;					//alarmin info lk20131129
	int stAttrSetted[MAX_STREAM] = {0};
	int stream_default=0;
	mchnosd_attr osd;

#ifdef GB28181_SUPPORT

	GBRegInfo_t gbInfo;
	mgb28181_get_param(&gbInfo);

#endif
	mchnosd_get_param(0, &osd);
	for (nCh=0;nCh <HWINFO_STREAM_CNT;nCh++)
	{
		mstream_get_param(nCh, &stAttr[nCh]);
	}
	jv_payload_type_e oldType = stAttr[0].vencType;
	nCh = 0;
	ipcinfo_get_param(&ipcinfo);
	malarm_get_param(&alarm);
	mprivacy_get_param(0, &region);
	jv_ai_get_attr(0, &audioInfo);//audio get lk20131125
	malarmin_get_param(0,&alarminInfo);
	stNetworkInfo.bModify = FALSE;
	stNetworkInfo.bReboot = FALSE;

	GetYSTParam(&stYST);

	while ((nLen = ParseParam(acBuff, sizeof(acBuff), pData)) > 0)
	{
		acBuff[nLen]	= 0;	//把分号去掉

		pData += nLen+1;
		if (strncmp(acBuff, "[CH1]", 5) == 0)
			nCh = 0;
		else if (strncmp(acBuff, "[CH2]", 5) == 0)
			nCh = 1;
		else if (strncmp(acBuff, "[CH3]", 5) == 0)
			nCh = 2;
		else if (strncmp(acBuff, "[CH4]", 5) == 0)
			nCh = 3;
		else if (strncmp(acBuff, "[ALL]", 5) == 0)
			nCh = 0;
		else
		{
			Printf("nCh=%d\n", nCh);
			if(nCh >= MAX_STREAM)
			{
				return ;
			}
			pItem = strtok(acBuff, "=");
			pValue = strtok(NULL, "\r");


			Printf("%s=%s\n", pItem, pValue);	//lck20100810
			if (!pValue)
				pValue = "";
			//时间显示以及设置
			if (strncmp(pItem, "DevName", 7) == 0)
			{				
				//将新设置的设备名设置到云视通
				printf("hwinfo.bHomeIPC=%d ipcinfo_get_type2=%d",hwinfo.bHomeIPC,ipcinfo_get_type2());
				if(hwinfo.bHomeIPC==1&&ipcinfo_get_type2()!= 1&&pValue[0]!=0x20)
				continue;
				if(gp.bNeedYST)
				{
					JVN_SetDeviceName(pValue);
				}
				ipcinfoSetted = 1;
				strncpy(ipcinfo.acDevName, pValue, sizeof(ipcinfo.acDevName)-1);
			}
            else if(strncmp(pItem,"ProductType",9)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.type, pValue, sizeof(ipcinfo.type)-1);
            }
            else if(strncmp(pItem,"osdText0",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[0], pValue, sizeof(ipcinfo.osdText[0])-1);
            }
            else if(strncmp(pItem,"osdText1",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[1], pValue, sizeof(ipcinfo.osdText[1])-1);
            }
            else if(strncmp(pItem,"osdText2",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[2], pValue, sizeof(ipcinfo.osdText[2])-1);
            }
            else if(strncmp(pItem,"osdText3",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[3], pValue, sizeof(ipcinfo.osdText[3])-1);
            }
            else if(strncmp(pItem,"osdText4",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[4], pValue, sizeof(ipcinfo.osdText[4])-1);
            }
            else if(strncmp(pItem,"osdText5",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[5], pValue, sizeof(ipcinfo.osdText[5])-1);
            }
			else if(strncmp(pItem,"osdText6",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[6], pValue, sizeof(ipcinfo.osdText[6])-1);
            }
			else if(strncmp(pItem,"osdText7",8)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.osdText[7], pValue, sizeof(ipcinfo.osdText[7])-1);
            }
			else if(strcmp(pItem, "osdX") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.osdX = atoi(pValue);
			}
			else if(strcmp(pItem, "osdY") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.osdY = atoi(pValue);
			}
			else if(strcmp(pItem, "endX") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.endX = atoi(pValue);
			}
			else if(strcmp(pItem, "endY") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.endY = atoi(pValue);
			}
			else if(strcmp(pItem, "osdSize") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.osdSize = atoi(pValue);
			}
			else if(strcmp(pItem, "multiPosition") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.osdPosition = atoi(pValue);
			}
			else if(strcmp(pItem, "alignment") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.osdAlignment = atoi(pValue);
			}
            else if(strncmp(pItem,"lcmsServer",10)==0)
            {
				ipcinfoSetted = 1;
				strncpy(ipcinfo.lcmsServer, pValue, sizeof(ipcinfo.lcmsServer)-1);
            }
            else if(strncmp(pItem,"nPosition",9)==0)
            {
            	bOSDModify = TRUE;
            	osd.position = (mchnosd_pos_e) atoi(pValue);
            }
            else if(strcmp(pItem,"nTimePosition")==0)
            {
            	bOSDModify = TRUE;
				osd.timePos = (mchnosd_pos_e) atoi(pValue);
            }
            else if(strcmp(pItem,"bLargeOSD")==0)
            {
            	bOSDModify = TRUE;
				osd.bLargeOSD = atoi(pValue);
            }
			else if(strncmp(pItem,"tutkid",6)==0)
            {
            	ipcinfoSetted = 1;
				if(strcmp(pValue," ") == 0 || pValue[0] == '\0')
					memset(ipcinfo.tutkid,0,MAX_TUTK_UID_NUM);
				else
					strncpy(ipcinfo.tutkid, pValue, sizeof(ipcinfo.tutkid));
            }
            else if(strncmp(pItem,"nOSDbInvColEn",13)==0)
			{
            	mchnosd_set_be_invcol(-1, atoi(pValue));
			}
            else if (strncmp(pItem, "FocusValueSwitch", 16) == 0)
            {
            	mchnosd_display_focus_reference_value(atoi(pValue));
            }
			//系统语言
			else if (strncmp(pItem, "nLanguage", 9) == 0)
			{
				int i;
				mchnosd_attr osd;
				if(pValue)
				{
					ipcinfoSetted = 1;
					ipcinfo.nLanguage = atoi(pValue);
					for (i=0;i<HWINFO_STREAM_CNT;i++)
					{
						mchnosd_get_param(i, &osd);
						if(LANGUAGE_CN == ipcinfo.nLanguage)
						{
							gp.nTimeFormat = TF_YYYYMMDD;
							strcpy(osd.timeFormat, "YYYY-MM-DD hh:mm:ss");
						}
						else
						{
							gp.nTimeFormat = TF_MMDDYYYY;
							strcpy(osd.timeFormat, "MM/DD/YYYY hh:mm:ss");
						}
						mchnosd_set_param(i, &osd);
						mchnosd_flush(i);
					}
					maccount_fix_with_language(ipcinfo.nLanguage);
					mivp_restart(0);
				}
			}
			else if(strcmp(pItem, "bSntp") == 0)
			{
				if(ipcinfo.bSntp != atoi(pValue))
				{
					ipcinfoSetted = 1;
					ipcinfo.bSntp = atoi(pValue);
					if(1 == ipcinfo.bSntp)
					{
						mlog_write("Time auto synchronization Enabled");
					}
					else
					{
						mlog_write("Time auto synchronization Disabled");
					}
				}
			}
			else if(strcmp(pItem, "sntpInterval") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.sntpInterval = atoi(pValue);
			}
			else if(strncmp(pItem, "ntpServer", 9) == 0)
			{
				ipcinfoSetted = 1;
				strncpy(ipcinfo.ntpServer, pValue, sizeof(ipcinfo.ntpServer));
			}
			else if(strcmp(pItem, "rebootDay") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.rebootDay = (RebootTimer_e)atoi(pValue);
			}
			else if(strcmp(pItem, "rebootHour") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.rebootHour = atoi(pValue);
			}
			else if(strcmp(pItem, "LedControl") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.LedControl = (LedControl_e)atoi(pValue);
			}
			else if(strcmp(pItem, "bAudioEn") == 0)
			{
				int i;	//所有码流一个开关
				for(i = 0;i<HWINFO_STREAM_CNT;i++)
				{
					stAttrSetted[i] = 1;
					stAttr[i].bAudioEn = atoi(pValue);
				}
				mstream_audio_restart(0,stAttr[0].bAudioEn);
				bAudioModify = TRUE;
			}
			else if(strcmp(pItem, "vencType") == 0)
			{
				stAttrSetted[nCh] = 1;
				stAttr[nCh].vencType = abs(atoi(pValue));
			}
			else if (strncmp(pItem, "width", 5) == 0)
			{
#ifndef PLATFORM_hi3516EV100 
				//分辨率调节
				U32 nValue = atoi(pValue);
				if(nCh==0 && nValue!=stAttr[nCh].width)
				{
					int i;
					for(i=1; i<HWINFO_STREAM_CNT; i++)
					{
						stAttrSetted[i] = 1;
					}
				}
				stAttr[nCh].width = nValue;
				stAttrSetted[nCh] = 1;
#endif
			}
			else if (strncmp(pItem, "height", 6) == 0)
			{
#ifndef PLATFORM_hi3516EV100 
				//分辨率调节
				U32 nValue = atoi(pValue);
				if((PRODUCT_MATCH( "SW-H210V3") || 
					PRODUCT_MATCH("SW-H411V3")) &&
					(hwinfo.sensor == SENSOR_AR0130 || 
					hwinfo.sensor == SENSOR_OV9750))
				{
					if(nValue > 720)
						nValue = 720;
				}
				if(PRODUCT_MATCH("SW-H411V4"))
				{
					if(nValue > 720)
						nValue = 720;
				}
				if(nCh==0 && nValue!=stAttr[nCh].height)
				{
					int i;
					for(i=1; i<HWINFO_STREAM_CNT; i++)
					{
						stAttrSetted[i] = 1;
					}
				}
				stAttr[nCh].height = nValue;
				stAttrSetted[nCh] = 1;
#endif
			}
			else if (strncmp(pItem, "framerate", 9) == 0)
			{
				//帧率调节
				stAttr[nCh].framerate = atoi(pValue);
				stAttrSetted[nCh] = 1;
			}
			else if (strncmp(pItem, "nGOP_S", 4) == 0)
			{
				//帧率调节
				stAttr[nCh].nGOP_S = atoi(pValue);
				stAttrSetted[nCh] = 1;
			}
			else if (strncmp(pItem, "nMBPH", 5) == 0)
			{
				//码流调节
				U32 nValue = atoi(pValue);
                
				stAttr[nCh].bitrate = nValue * 1024 * 8 / 3600;
                //gyd 修改单位kbps
                stAttr[nCh].bitrate = nValue ;
				stAttrSetted[nCh] = 1;
			}
			else if (strncmp(pItem, "bitrate", 7) == 0)
			{
				U32 nValue = atoi(pValue);
				stAttr[nCh].bitrate = nValue ;
				stAttrSetted[nCh] = 1;
			}
			else if (strcmp(pItem, "rcMode") == 0)
			{
				//码率控制类型
				stAttr[nCh].rcMode = atoi(pValue);
				stAttrSetted[nCh] = 1;
			}
			else if (strcmp(pItem, "minQP") == 0)
			{
				stAttr[nCh].minQP = atoi(pValue);
				stAttrSetted[nCh] = 1;
			}
			else if (strcmp(pItem, "maxQP") == 0)
			{
				stAttr[nCh].maxQP = atoi(pValue);
				stAttrSetted[nCh] = 1;
			}
			else if (strcmp(pItem, "stream_default") == 0)
			{
				stream_default = atoi(pValue);
				//printf("***************************************.............%d\n",stream_default);
			}
			else if (strcmp(pItem, "MobileQuality") == 0)
			{
				mstream_attr_t attr;
				U32 nPictureType = atoi(pValue);
				if(SctrlGetParam(NULL)->nPictureType != nPictureType)
				{
					if (hwinfo.bHomeIPC)
					{
						if (nPictureType == MOBILE_QUALITY_HIGH)
						{
							mstream_get_param(0, &attr);
							attr.bitrate = 1024;
							mstream_set_param(0, &attr);
							mstream_flush(0);
						}
						else if(nPictureType == MOBILE_QUALITY_MIDDLE)
						{
							mstream_get_param(0, &attr);
							if (hwinfo.encryptCode == ENCRYPT_200W)	// 200W
							{
								attr.bitrate = 640;
							}
							else if (hwinfo.encryptCode == ENCRYPT_130W)	// 130W
							{
								attr.bitrate = 540;
							}
							else
							{
								attr.bitrate = 440;
							}
							mstream_set_param(0, &attr);
							mstream_flush(0);
						}
					}
					SctrlGetParam(NULL)->nPictureType = nPictureType;
					_resolution_changed(SctrlGetMOChannel());
					mrecord_pre_reinit();
				}
			}
			//音频设置,目前只写了编码类型的识别和修改 20131125lk
			else if(strcmp(pItem, "encType") == 0)
			{
				audioInfo.encType = atoi(pValue);
				bAudioModify = TRUE;
				printf("lk test audio enctyp: %d\n",atoi(pValue));
			}
			//视频遮挡
			else if(strncmp(pItem, "bCoverRgn", 9) == 0)
			{
				region.bEnable	= atoi(pValue);
				bPrivacyModify		= TRUE;
				Printf("region.bEnable=%d\n", region.bEnable);
			}
			else if(strncmp(pItem, "Region", 6) == 0)
			{
				RECT *pRect = NULL;
				U32 nIndex=0;			//第几个区域
				sscanf(pItem, "Region%d", &nIndex);
				pRect = &region.stRect[nIndex];
				sscanf(pValue, "%d,%d,%d,%d", &pRect->x, &pRect->y, &pRect->w, &pRect->h);
				bPrivacyModify		= TRUE;
				Printf("%s\n", pValue);
			}
			//web服务
			else if (strncmp(pItem, "WebServer", 9) == 0)
			{
				ipcinfo.bWebServer = atoi(pValue);
				bWebServerModify = TRUE;
			}
			else if (strncmp(pItem, "WebPort", 7) == 0)
			{
				ipcinfo.nWebPort = atoi(pValue);
				bWebServerModify = TRUE;
			}
			//云视通局域网连接模式lk20140222
//			else if(strncmp(pItem, "YSTLANModel", 11) == 0)
//			{
//				stYST.eLANModel = (LANModel_e)atoi(pValue);
//				SetYSTParam(&stYST);
//				FlushYSTCH();
//			}
			//云视通端口号
			else if (strncmp(pItem, "YSTPort", 7) == 0)
			{
				stYST.nPort = atoi(pValue);
				SetYSTParam(&stYST);
			}
			else if (strncmp(pItem, "bNeedRestart", 12) == 0)
			{
				if (atoi(pValue))
				{
				
					bReboot	= TRUE;
					//SYSFuncs_reboot();
				}
			}

			else if(strcmp(pItem, "CenterCorrection") == 0)
			{
			  	printf("fisheye centor correct!!!\n");
			}
			//警报设置
			else if(strncmp(pItem,"nAlarmDelay",11)==0)
			{
				//报警延时
				alarm.delay=atoi(pValue);
				bAlarmModify = TRUE;
			}
			else if(strncmp(pItem,"acMailSender",12)==0)
			{
				//发件人
				sprintf(alarm.sender,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if(strncmp(pItem,"acSMTPServer",12)==0)
			{
				//服务器
				bAlarmModify = TRUE;
				sprintf(alarm.server,"%s",pValue);
			}
			else if(strncmp(pItem,"acSMTPUser",10)==0)
			{
				//用户名
				sprintf(alarm.username,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"acSMTPPasswd",12)==0)
			{
				//密码
				sprintf(alarm.passwd,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"acReceiver0",11)==0)
			{
				//收件人1
				sprintf(alarm.receiver0,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"acReceiver1",11)==0)
			{
				//收件人2
				sprintf(alarm.receiver1,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"acReceiver2",11)==0)
			{
				//收件人3
				sprintf(alarm.receiver2,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"acReceiver3",11)==0)
			{
				//收件人4
				sprintf(alarm.receiver3,"%s",pValue);
				bAlarmModify = TRUE;
			}
			//add by xianlt at 20120628
			else if (strncmp(pItem,"acSMTPPort",10)==0)
			{
				//收件人3
				alarm.port=atoi(pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"acSMTPCrypto",12)==0)
			{
				//收件人4
				sprintf(alarm.crypto,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"vmsServerIp",11)==0)
			{
				
				sprintf(alarm.vmsServerIp,"%s",pValue);
				bAlarmModify = TRUE;
			}
			else if (strncmp(pItem,"vmsServerPort",13)==0)
			{
				
				alarm.vmsServerPort = atoi(pValue);
				bAlarmModify = TRUE;
			}
			else if(strncmp(pItem,"bDHCP",5)==0)
			{
				printf("Set DHCP %s\n", pValue);
				if(atoi(pValue)==1)
				{
					stNetworkInfo.dhcp=TRUE;
					//setdhcp(DHCP_ON);
					mlog_write("DHCP Enabled");
				}
				else
				{
					stNetworkInfo.dhcp=FALSE;
					//setdhcp(DHCP_OFF);
					mlog_write("DHCP Disabled");
				}
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"acETH_IP",8)==0)
			{
				printf("Set ETH_IP\n");
				strcpy(stNetworkInfo.eth_acip,pValue);
				//setip(pValue);
			}
			else if(strncmp(pItem,"acETH_NM",9)==0)
			{
				printf("Set ETH_NM\n");
				strcpy(stNetworkInfo.eth_acnm,pValue);
				//setnetmask(pValue);
			}
			else if(strncmp(pItem,"acETH_GW",8)==0)
			{
				printf("Set ETH_GW\n");
				strcpy(stNetworkInfo.eth_acgw,pValue);
				//setgatewy(pValue);
			}
			else if(strncmp(pItem,"acETH_DNS",9)==0)
			{
				printf("Set ETH_DNS\n");
				strcpy(stNetworkInfo.eth_acdns,pValue);
				//setdns(pValue);
			}
			else if(strncmp(pItem, "NW_ACTIVED", 10)==0)
			{
				stNetworkInfo.nActived = atoi(pValue);
			}
			//新的网络设置,lck20120516
			else if(strncmp(pItem,"nlIP", 4)==0)
			{
				Printf("Set nlIP, %s\n", pValue);
				__ipset(atoi(pValue), stNetworkInfo.eth_acip);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"nlNM", 4)==0)
			{
				Printf("Set nlNM, %s\n", pValue);
				//if(pValue) SetnlNM(atoi(pValue));
				__ipset(atoi(pValue), stNetworkInfo.eth_acnm);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"nlGW", 4)==0)
			{
				Printf("Set nlGW, %s\n", pValue);
				//if(pValue) SetnlGW(atoi(pValue));
				__ipset(atoi(pValue), stNetworkInfo.eth_acgw);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"nlDNS", 5)==0)
			{
				Printf("Set nlDNS, %s\n", pValue);
				//if(pValue) SetnlDNS(atoi(pValue));
				__ipset(atoi(pValue), stNetworkInfo.eth_acdns);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"ADSL_ID", 7)==0)
			{
				if(pValue) strcpy(stNetworkInfo.acID, pValue);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"ADSL_PW", 7)==0)
			{
				if(pValue) strcpy(stNetworkInfo.acPW, pValue);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"WIFI_AP", 7)==0)
			{
				if(pValue) stNetworkInfo.nAP = atoi(pValue);
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"WIFI_ID", 7)==0)
			{
				if(pValue) strncpy(stNetworkInfo.acID, pValue,sizeof(stNetworkInfo.acID));
				stNetworkInfo.bModify = TRUE;
			}
			else if(strncmp(pItem,"WIFI_PW", 7)==0)
			{
				if(pValue) strncpy(stNetworkInfo.acPW, pValue,sizeof(stNetworkInfo.acPW));
				stNetworkInfo.bModify = TRUE;
			}
            else if(strncmp(pItem,"WIFI_AUTH", 9)==0)
            {
                stNetworkInfo.wifiAuth = atoi(pValue);
				stNetworkInfo.bModify = TRUE;
            }
            else if(strncmp(pItem,"WIFI_ENC", 8)==0)
            {
                stNetworkInfo.wifiEncryp = atoi(pValue);
				stNetworkInfo.bModify = TRUE;
            }

			else if(strncmp(pItem, "ACTIVED", 7)==0)
			{
				if(pValue) 
				{
					if (stNetworkInfo.nActived != atoi(pValue))
					{
						stNetworkInfo.nActived = atoi(pValue);
						stNetworkInfo.bModify = TRUE;
					}
				}
			}
			else if(strncmp(pItem, "NW_RESTART", 10)==0)
			{
				stNetworkInfo.bReboot = TRUE;
				stNetworkInfo.bModify = TRUE;
			}
			else if (strncmp(pItem, "DDNS_Type",9) == 0)
			{
				bDdnsModify		= TRUE;
			}
			else if (strncmp(pItem, "DDNS_Host",9) == 0)
			{	
				bDdnsModify		= TRUE;
			}
			else if (strncmp(pItem, "DDNS_User",9) == 0)
			{
				bDdnsModify		= TRUE;
			}
			else if (strncmp(pItem, "DDNS_Pwd",8) == 0)
			{
				bDdnsModify		= TRUE;
			}

#ifdef GB28181_SUPPORT
			else if (strcmp(pItem, "GB_bEnable") == 0)
				gbInfo.bEnable = atoi(pValue);
			else if (strcmp(pItem, "GB_devid") == 0)
				jv_strncpy(gbInfo.devid, pValue, sizeof(gbInfo.devid));
			else if (strcmp(pItem, "GB_devpasswd") == 0)
				jv_strncpy(gbInfo.devpasswd, pValue, sizeof(gbInfo.devpasswd));
			else if (strcmp(pItem, "GB_serverip") == 0)
				jv_strncpy(gbInfo.serverip, pValue, sizeof(gbInfo.serverip));
			else if (strcmp(pItem, "GB_serverport") == 0)
				gbInfo.serverport = atoi(pValue);
			else if (strcmp(pItem, "GB_localport") == 0)
				gbInfo.localport = atoi(pValue);
			else if (strcmp(pItem, "GB_expires") == 0)
				gbInfo.expires = atoi(pValue);
			else if (strcmp(pItem, "GB_alarminID") == 0)
				jv_strncpy(gbInfo.alarminID[0], pValue, sizeof(gbInfo.alarminID[0]));
			else if (strcmp(pItem, "GB_keepalive") == 0)
				gbInfo.keepalive = atoi(pValue);
			else if (strcmp(pItem, "GB_KA_outtimes") == 0)
				gbInfo.keepalive_outtimes = atoi(pValue);
			else if (strcmp(pItem, "GB_EX_refresh") == 0)
				gbInfo.expires_refresh = atoi(pValue);
#endif
			//lk20131129 报警输入
			else if (strcmp(pItem, "alarmin_bEnable") == 0)
			{
				alarminInfo.bEnable = atoi(pValue);
				bAlarmInModify = TRUE;
			}
			else if (strcmp(pItem, "alarmin_bNormallyClosed") == 0)
			{
				alarminInfo.bNormallyClosed = atoi(pValue);
				bAlarmInModify = TRUE;
			}
			else if (strcmp(pItem, "alarmin_bSendtoClient") == 0)
			{
				alarminInfo.bSendtoClient = atoi(pValue);
				bAlarmInModify = TRUE;
			}
			else if (strcmp(pItem, "alarmin_bSendtoVMS") == 0)
			{
				alarminInfo.bSendtoVMS = atoi(pValue);
				bAlarmInModify = TRUE;
			}
			else if (strcmp(pItem, "alarmin_bSendEmail") == 0)
			{
				alarminInfo.bSendEmail = atoi(pValue);
				bAlarmInModify = TRUE;
			}
			else if (strcmp(pItem, "alarmin_bEnRecord") == 0)
			{
				alarminInfo.bEnableRecord = atoi(pValue);
				bAlarmInModify = TRUE;
			}
			//timezone setting for homeipc
			else if (strcmp(pItem,"timezone") == 0)
			{
				ipcinfoSetted = 1;
				ipcinfo.tz = atoi(pValue);
			}
			else if(strcmp(pItem,"HWLedOn") == 0)
			{
				int ledOn = atoi(pValue);
				if(ledOn)
				{
					malarm_buzzing_open();
				}
				else
				{
					malarm_buzzing_close();
				}
			}
			else
			{
				//打印未应用的配置
				Printf("Other settings:%s=%s\n", pItem, pValue);
			}
		}
	}

	//应用WEB服务的新配置
	if(bWebServerModify)
	{
		ipcinfo_set_param(&ipcinfo);
		webserver_flush();
	}
	//设置遮挡区域，暂时放在这里,lck20120320
	if(bPrivacyModify)
	{
		mprivacy_set_param(0, &region);
		mprivacy_flush(0);
	}


	if (bAlarmModify)
	{
		malarm_set_param(&alarm);
	}

	//切换网络
	if(stNetworkInfo.bModify)
	{
		eth_t eth;
		switch(stNetworkInfo.nActived+ETH_NET)
		{
		default:
		case ETH_NET:
			if (utl_ifconfig_b_linkdown("eth0") == FALSE)
			{
				//system("rm data/pppoe.conf data/wpa_supplicant.conf;");
				strncpy(eth.addr, stNetworkInfo.eth_acip, sizeof(eth.addr));
				strncpy(eth.dns, stNetworkInfo.eth_acdns, sizeof(eth.dns));
				strncpy(eth.gateway, stNetworkInfo.eth_acgw, sizeof(eth.gateway));
				strncpy(eth.mask, stNetworkInfo.eth_acnm, sizeof(eth.mask));
				mlog_write("Network Actived Ethernet:%s",stNetworkInfo.dhcp?"DHCP":eth.addr);
				strncpy(eth.name, "eth0", sizeof(eth.name));
				eth.bDHCP = stNetworkInfo.dhcp;
				utl_ifconfig_eth_set(&eth);
	//			if(stNetworkInfo.bReboot)
				{
					net_deinit();
					//SYSFuncs_reboot();
				}
			}
			break;
		case PPPOE_NET:
			if (utl_ifconfig_b_linkdown("eth0") == TRUE)
			{
				mlog_write("Network Actived PPPOE");
				if(stNetworkInfo.bReboot)
				{
					pppoe_t pppoe;
					strncpy(pppoe.name, "eth0", sizeof(pppoe.name));
					strncpy(pppoe.username, stNetworkInfo.acID, sizeof(pppoe.username));
					strncpy(pppoe.passwd, stNetworkInfo.acPW, sizeof(pppoe.passwd));
					utl_ifconfig_ppp_set(&pppoe);
					net_deinit();
					bReboot	= TRUE;
					//SYSFuncs_reboot();
				}
			}
			break;
		case WIFI_NET:
			mlog_write("Network Actived WIFI");
//			if(stNetworkInfo.bReboot )
			{
				utl_ifconfig_wifi_start_sta();
				wifiap_t ap;
				strncpy(ap.name, stNetworkInfo.acID, sizeof(ap.name));
				strncpy(ap.passwd, stNetworkInfo.acPW, sizeof(ap.passwd));
				printf(".......wifi id : %s....wifi pwd: %s\n",ap.name,ap.passwd);
				net_deinit();
				ap.iestat[0] = stNetworkInfo.wifiAuth;
				ap.iestat[1] = stNetworkInfo.wifiEncryp;
				utl_ifconfig_wifi_connect(&ap);
			}
			break;
		}
	}

#ifdef GB28181_SUPPORT
	mgb28181_set_param(&gbInfo);
#endif

#include <jv_stream.h>
	//总最大编码能力，1码流的最大编码能力，实际每个码流设置的编码能力，
	int maxVencAbility,maxVencAbility0,setVencAbility[MAX_STREAM],setVencAbility0,setVencAbility1;
	int def_rate[MAX_STREAM],def_br[MAX_STREAM],def_w[MAX_STREAM],def_h[MAX_STREAM];
	int tmp_stream_id;
	jvstream_ability_t tmp_ab;
	jv_stream_get_ability(0,&tmp_ab);
	int venc_blimit=0;

//编码能力限制
//主码流做了设置则以主码流设置为依据，调整次码流帧率。否则以次码流为依据调整主码流帧率。
	int flag = 0;
	int tmpWidth = 0;
	int tmpHeight = 0;

#ifdef PLATFORM_hi3516D
	if(hwinfo.encryptCode == ENCRYPT_300W)
	{
		if(stAttrSetted[1])
		{
			if(stAttr[1].framerate > 28)
			{
				stAttr[1].framerate = 28;
				stAttr[1].bitrate = __CalcBitrate(stAttr[1].width, stAttr[1].height, stAttr[1].framerate, stAttr[1].vencType);
			}
		}
		if(stAttrSetted[2])
		{
			if(stAttr[2].framerate > 15)
			{
				stAttr[2].framerate = 15;
				stAttr[2].bitrate = __CalcBitrate(stAttr[2].width, stAttr[2].height, stAttr[2].framerate, stAttr[2].vencType);
			}
		}
	}
	if(hwinfo.sensor == SENSOR_OV4689 && strstr(hwinfo.type,"40"))	//400w的帧率
	{
		if(stAttrSetted[0])
		{
			if(tmp_ab.maxStreamRes[0]==stAttr[0].width*stAttr[0].height && 
				stAttr[0].framerate > 25)
			{
				stAttr[0].framerate = 25;
				stAttr[0].bitrate = __CalcBitrate(stAttr[0].width, stAttr[0].height, stAttr[0].framerate, stAttr[0].vencType);
			}
		}
		if(stAttrSetted[1])
		{
			if(stAttr[1].framerate > 25)
			{
				stAttr[1].framerate = 25;
				stAttr[1].bitrate = __CalcBitrate(stAttr[1].width, stAttr[1].height, stAttr[1].framerate, stAttr[1].vencType);
			}
		}
		if(stAttrSetted[2])
		{
			if(stAttr[2].framerate > 25)
			{
				stAttr[2].framerate = 25;
				stAttr[2].bitrate = __CalcBitrate(stAttr[2].width, stAttr[2].height, stAttr[2].framerate, stAttr[2].vencType);
			}
		}
	}
#endif

	printf("**********************\n");
	printf("chn0:bset:%d;w:%d;h:%d;frame:%d;br:%d\n",stAttrSetted[0],stAttr[0].width, stAttr[0].height,stAttr[0].framerate,stAttr[0].bitrate);
	printf("chn1:bset:%d;w:%d;h:%d;frame:%d;br:%d\n",stAttrSetted[1],stAttr[1].width, stAttr[1].height,stAttr[1].framerate,stAttr[1].bitrate);
	printf("**********************\n");

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100)
	if(hwinfo.encryptCode == ENCRYPT_200W)
	{
		if(stAttrSetted[0])
		{
			JVRotate_e rotate = msensor_get_rotate();
			if(rotate != JVSENSOR_ROTATE_NONE && stAttr[0].height == 1080)
			{
				stAttr[0].width = 1280;
				stAttr[0].height = 960;
			}
			if(stAttr[0].width*stAttr[0].height == 1920*1080)
				hwinfo.rotateBSupport = FALSE;
			else
				hwinfo.rotateBSupport = TRUE;
			
			// 200W支持到20帧
			if(tmp_ab.maxStreamRes[0]==stAttr[0].width*stAttr[0].height && stAttr[0].framerate > 20)
			{
				stAttr[0].framerate = 20;
				stAttr[0].bitrate = __CalcBitrate(stAttr[0].width, stAttr[0].height, stAttr[0].framerate, stAttr[0].vencType);
			}
		}
		if(stAttrSetted[1])
		{
			if(stAttr[1].width*stAttr[1].height > tmp_ab.maxStreamRes[1])
			{
				stAttr[1].width = 704;
				stAttr[1].height = 576;
			}
			if(stAttr[1].width*stAttr[1].height>640*480 && stAttr[1].framerate>20)
			{
				stAttr[1].framerate = 20;
				stAttr[1].bitrate = __CalcBitrate(stAttr[1].width, stAttr[1].height, stAttr[1].framerate, stAttr[1].vencType);
			}
		}
	}
#endif

	for (nCh=0; nCh<HWINFO_STREAM_CNT; nCh++)
	{
		if (stAttrSetted[nCh])
		{
			mstream_set_param(nCh, &stAttr[nCh]);
			mstream_flush(nCh);
		}
	}


	//音频设置lk20131125
	if(bAudioModify)
	{
		mstream_audio_set_param(0, &audioInfo);
		mrecord_stop(0);
        mrecord_flush(0);
	}

	if (bOSDModify)
	{
		ipcinfoSetted = 1;
		ipcinfo.needFlushOsd = TRUE;
		int i;
		//避免同时修改时，机器名不生效
		memcpy(osd.channelName, ipcinfo.acDevName, sizeof(osd.channelName));
		if(LANGUAGE_CN == ipcinfo.nLanguage)
		{
			gp.nTimeFormat = TF_YYYYMMDD;
			strcpy(osd.timeFormat, "YYYY-MM-DD hh:mm:ss");
		}
		else
		{
			gp.nTimeFormat = TF_MMDDYYYY;
			strcpy(osd.timeFormat, "MM/DD/YYYY hh:mm:ss");
		}
		for (i=0;i<HWINFO_STREAM_CNT;i++)
		{
			mchnosd_set_param(i, &osd);
			mchnosd_stop(i);
			mchnosd_flush(i);
		}
		mivp_restart(0);
	}
	
	if (ipcinfoSetted)
	{
		ipcinfo_set_param(&ipcinfo);
	}
	
   	WriteConfigInfo();


#if 1	//此处要重新考虑是否去掉,lck20120803
	//发送设置完成消息
	if (remotecfg != NULL)
	{
	   	char	acBuffer[20]={0};
		PACKET *pstPacket = (PACKET*)acBuffer;
		pstPacket->nPacketType	= RC_SETPARAMOK;
		pstPacket->nPacketLen	= ipcinfo_get_param(NULL)->nLanguage;
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)pstPacket, 4);
	}
#endif
	if(bReboot == TRUE)
		SYSFuncs_reboot();
	//将配置写入文件,lck20120109
	//WriteConfigInfo();
}

static VOID _remotecfg_send_devlog(REMOTECFG *pstRemoteCfg)
{
	U32	i;
	struct tm tmDate;
	if(pstRemoteCfg->stPacket.nPacketLen)//按日期
	{


	U32	nYear	= pstRemoteCfg->stPacket.nPacketLen;
	U32	nMonth	= pstRemoteCfg->stPacket.nPacketCount;
	U32	nDay	= pstRemoteCfg->stPacket.nPacketID;

	memset(&pstRemoteCfg->stPacket, 0, sizeof(PACKET));
	Printf("Date:%.4d-%.2d-%.2d\n", nYear, nMonth, nDay);

	char acBuffer[64]= {0};
	pstRemoteCfg->stPacket.nPacketType	= RC_GETDEVLOG;

#if 1 //以先后顺序为顺序

	if (stLog.head.nRecordTotal == LOG_MAX_RECORD)
	{
		for(i=stLog.head.nCurRecord+1; i<LOG_MAX_RECORD; i++)
		{
			localtime_r((time_t *)&stLog.item[i].nTime, &tmDate);
			if(tmDate.tm_year+1900 == nYear && tmDate.tm_mon+1 == nMonth && tmDate.tm_mday == nDay)
			{
				sprintf(acBuffer, "%.2d:%.2d:%.2d  %s\n", tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec, stLog.item[i].strLog);
				strcat((char *)pstRemoteCfg->stPacket.acData, acBuffer);
			}
		}
	}
	for(i=0; i<stLog.head.nCurRecord+1; i++)
	{
		localtime_r((time_t *)&stLog.item[i].nTime, &tmDate);
		if(tmDate.tm_year+1900 == nYear && tmDate.tm_mon+1 == nMonth && tmDate.tm_mday == nDay)
		{
			sprintf(acBuffer, "%.2d:%.2d:%.2d  %s\n", tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec, stLog.item[i].strLog);
			strcat((char *)pstRemoteCfg->stPacket.acData, acBuffer);
		}
	}

#else

	for(i=0; i<stLog.head.nRecordTotal; i++)
	{
		localtime_r((time_t *)&stLog.item[i].nTime, &tmDate);
		if(tmDate.tm_year+1900 == nYear && tmDate.tm_mon+1 == nMonth && tmDate.tm_mday == nDay)
		{
			sprintf(acBuffer, "%.2d:%.2d:%.2d  %s\n", tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec, stLog.item[i].strLog);
			strcat(pstRemoteCfg->stPacket.acData, acBuffer);
		}
	}
#endif

	}
	else//按页码lk20140103
	{
		U32 nPage = pstRemoteCfg->stPacket.nPacketCount;
		//printf("******************LK test log page:%d*********************\n",nPage);
		memset(&pstRemoteCfg->stPacket, 0, sizeof(PACKET));
		char acBuffer[64] ={ 0 };
		pstRemoteCfg->stPacket.nPacketType = RC_GETDEVLOG;
		pstRemoteCfg->stPacket.nPacketCount = (stLog.head.nRecordTotal+99)/100;
		int p = stLog.head.nCurRecord - nPage*100;
		if(p < 0)
			p += LOG_MAX_RECORD;
		//printf("******************LK test log start item:%d*********************\n",p);
		for(i = 0; i < 100; i++,p--)
		{
			if(p < 0)
				p += LOG_MAX_RECORD;
			localtime_r((time_t *)&stLog.item[p].nTime, &tmDate);
			if(strcmp(stLog.item[p].strLog,"\0")==0)
				continue;
			sprintf(acBuffer, "%d/%d/%d %.2d:%.2d:%.2d  %s\n",tmDate.tm_year+1900,tmDate.tm_mon+1, tmDate.tm_mday, tmDate.tm_hour,tmDate.tm_min, tmDate.tm_sec, stLog.item[p].strLog);
			//printf("******log:%s\n",acBuffer);
			strcat((char *)pstRemoteCfg->stPacket.acData, acBuffer);
		}
		//printf("******log:%d,%s\n",stLog.head.nCurRecord,stLog.item[stLog.head.nCurRecord].strLog);


	}
	//Printf("%s\n", pstRemoteCfg->stPacket.acData);
	MT_TRY_SendChatData(pstRemoteCfg->nCh, pstRemoteCfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&pstRemoteCfg->stPacket, strlen((char *)pstRemoteCfg->stPacket.acData)+4);
}

void remotecfg_send_self_data(int channelid,char *data,int nSize)
{
	if(!gp.bNeedYST)
		return;
	JVN_SendData(1, JVN_DATA_O, (U8*)data, nSize, 0, 0);
}

/*
 * 报警信息定义：
 *   0  1 报警信息
 *   1  1 == 发生报警 ; 0 == 报警解除
 *   2  n 附加信息下标。其值 表示，附加信息的偏移地址。当前应为4。当取0时，表示无附加信息
 *   3  报警类型  0：未知  1：移动检测 2：报警输入
 *   .  可增加其它信息的定义
 *   n  附加信息。 当前定义的图片信息，其内容应为：jpeg\0 后面跟上图片数据。类似这样{'j', 'p', 'e', 'g', 0, DATA...}
 */

//通知分控关闭或打开报警
void remotecfg_alarm_on(int channelid, BOOL bOn, alarm_type_e type)
{
	U8 u8Data[4]= {0};
	u8Data[0]	= 1;
	u8Data[1]	= bOn?1:0;
	u8Data[2] = 0;
	u8Data[3] = type;
	if(!gp.bNeedYST)
		return;
	JVN_SendData(1, JVN_DATA_O, u8Data, 4, 0, 0);
	JVN_SendData(2, JVN_DATA_O, u8Data, 4, 0, 0);

	if (bOn)
	{
		//check if send jpeg
		static int timesended = 0;
		int timenow;
		BOOL ignore_flag = 0;

		timenow = time(NULL);
		if (timenow - timesended < 10)
		{
			//Printf("Now no need to send again.\n");
			ignore_flag = 1;
		}
		timesended = timenow;

		if (!ignore_flag)
		{
			int len = 0;
			U8 *acData = malloc(256*1024);
			U8 *p = acData;
			*p++ = 1;
			*p++ = 1; //on
			*p++ = 4; //data offset
			*p++ = type;

			*p++ = 'j';
			*p++ = 'p';
			*p++ = 'e';
			*p++ = 'g';
			*p++ = 0;
			len = p-acData;
			if(hwinfo.bHomeIPC)
			{
				stSnapSize size;
				size.nWith = 400;
				size.nHeight = 224;
				len += msnapshot_get_data(0, p,256*1024-10,&size);
			}
			else
			{
				stSnapSize size;
				size.nWith = 720;
				size.nHeight = 576;
				len += msnapshot_get_data(0, p,256*1024-10,&size);
			}
			JVN_SendData(1, JVN_DATA_O, acData, len, 0, 0);
			free(acData);
//			printf("now send jpeg...: %d\n", len);
		}
	}
}

static int _type = 0;
static REMOTECFG _remoteCfg = {0};

void DoorAlarmInsert(signed char result, signed char index)
{
	Printf("DoorAlarmInsert: result=%d, index=%d\n", result, index);
	char *pdata = (char *)_remoteCfg.stPacket.acData;

	pdata[0] = '\0';

	switch(result)
	{
		case DOOR_INSERT_OK:
			pdata += sprintf(pdata, "type=%d;", _type);
			pdata += sprintf(pdata, "guid=%d;", index);
			pdata += sprintf(pdata, "res=1;");
			break;
		case DOOR_INSERT_TIMEDOUT:
			pdata += sprintf(pdata, "type=%d;", _type);
			pdata += sprintf(pdata, "res=0;");
			break;
		case DOOR_INSERT_FULL:
			pdata += sprintf(pdata, "type=%d;", _type);
			pdata += sprintf(pdata, "res=2;");
			break;
		case DOOR_REINSERT:
			pdata += sprintf(pdata, "type=%d;", _type);
			pdata += sprintf(pdata, "guid=%d;", index);
			pdata += sprintf(pdata, "res=3;");
			break;
		default:
			break;
	}

	Printf("acData:%s\n", (char *)_remoteCfg.stPacket.acData);
	MT_TRY_SendChatData(_remoteCfg.nCh, _remoteCfg.nClientID, JVN_RSP_TEXTDATA, (U8*)&_remoteCfg.stPacket, strlen((char *)_remoteCfg.stPacket.acData)+4);
	memset(&_remoteCfg, 0, sizeof(REMOTECFG));
}

static int dooralarm_timer = -1;
static int doorAlarmType = 0;

static BOOL _mdooralarm_timer(int id, void* param)
{
	MDoorAlarm_t *doorAlarmCfg = mdooralarm_get_param(NULL);
	static U32 alarmOutType = 0;
	ALARMSET stAlarm;
	malarm_get_param(&stAlarm);

	if (param == NULL)
	{
		if(doorAlarmCfg->bSendtoClient)
		{
			remotecfg_alarm_on(0, TRUE, ALARM_TYPE_WIRELESS/*, doorAlarmType */);
			alarmOutType |= ALARM_OUT_CLIENT;
		}

		#if 0
		if(doorAlarmCfg->bBuzzing && (stAlarm.bEnable || 2 == doorAlarmType))
		{
			MAlarmIn_t malarmin;
			malarmin_get_param(0, &malarmin);
			if(malarmin.nGuardChn == 0)
			{
				ipcinfo_t info;
				ipcinfo_get_param(&info);
				if(!info.bAlarmService || stAlarm.alarmLinkOut[NET_ALARM_TYPE_WIRELESS_UNKNOWN+doorAlarmType] & 0x00000001)
				{
					malarm_buzzing_open();
					alarmOutType |= ALARM_OUT_BUZZ;
				}
			}
		}
		#endif

		if(stAlarm.bEnable && stAlarm.bAlarmSoundEnable)
		{
			printf("_mdooralarm_timer\n");
			malarm_sound_start();
			alarmOutType |= ALARM_OUT_SOUND;
		}

		utl_timer_reset(dooralarm_timer, stAlarm.delay*1000, _mdooralarm_timer, (void *) 1);
	}
	else
	{
		if(alarmOutType & ALARM_OUT_CLIENT)
			remotecfg_alarm_on(0, FALSE, ALARM_TYPE_WIRELESS/* , doorAlarmType */);

		#if 0
		if(alarmOutType & ALARM_OUT_BUZZ)
		{
			MAlarmIn_t malarmin;
			malarmin_get_param(0, &malarmin);
			if(malarmin.nGuardChn == 0)
			{
				malarm_buzzing_close();
			}
		}
		#endif
		
		if(alarmOutType & ALARM_OUT_SOUND)
		{
			malarm_sound_stop();
		}

		alarmOutType = 0;
	}

	return FALSE;
}

void DoorAlarmStop()
{
	if (dooralarm_timer != -1)
	{
		utl_timer_reset(dooralarm_timer, 0, _mdooralarm_timer, (void *) 1);
	}
	
}

void DoorAlarmSend(unsigned char* name, int arg)
{
	Printf("DoorAlarmSend: name=%s\n", name);
	MDoorAlarm_t *doorAlarmCfg = NULL;
	BOOL bAlarmReportEnable = FALSE; // 安全防护开关
	BOOL bCloudRecEnable = FALSE; // 是否开通云存储功能
	BOOL bInValidTime = FALSE; // 是否在安全防护时间段内
	static unsigned int lasttime = 0;
	JV_ALARM alarm;
	
	bInValidTime = malarm_check_validtime();
	if(!bInValidTime)
	{
		return;
	}
	
	doorAlarmType = arg;

	bAlarmReportEnable = malarm_check_enable();
	bCloudRecEnable = mcloud_check_enable();

	ALARMSET stAlarm;
	malarm_get_param(&stAlarm);

	doorAlarmCfg = mdooralarm_get_param(NULL);

	if(hwinfo.bHomeIPC && bAlarmReportEnable)
	{
		malarm_build_info(&alarm, ALARM_DOOR);
		if(bCloudRecEnable)
			alarm.pushType = ALARM_PUSH_YST_CLOUD;
	}

	if(doorAlarmCfg->bEnableRecord)
	{
		mrecord_set(0, alarming, TRUE);
		if(hwinfo.bHomeIPC && bAlarmReportEnable && bCloudRecEnable)
		{
			alarm.cmd[1] = ALARM_VIDEO;
			mrecord_alarming(0, ALARM_TYPE_ALARM, &alarm);
		}
		else
		{
			mrecord_alarming(0, ALARM_TYPE_ALARM, NULL);
		}
	}

	if(hwinfo.bHomeIPC && bAlarmReportEnable)
	{
		mrecord_alarm_get_attachfile(REC_ALARM, &alarm);
		if (alarm.cloudPicName[0])
		{
			printf("alarmJpgFile: %s\n", alarm.cloudPicName);
			if (msnapshot_get_file(0, alarm.cloudPicName) != 0)
			{
				alarm.cloudPicName[0] = '\0';
				alarm.PicName[0] = '\0';
			}
		}
		else if(alarm.PicName[0])
		{
			printf("alarmJpgFile: %s\n", alarm.PicName);
			if(msnapshot_get_file(0, alarm.PicName) != 0)
			{
				alarm.PicName[0] = '\0';
			}
		}
		else
			printf("MD cloudPicName and PicName are none\n");
#ifdef BIZ_CLIENT_SUPPORT
		if (hwinfo.bXWNewServer)
		{
			int ret = -1;
			char *ppic = "";
			char tmppic[64] = {0};
			char *pvedio = "";
			if (alarm.pushType == ALARM_PUSH_YST_CLOUD)
			{
				if (alarm.cloudPicName[0])
				{
					ppic = strrchr(alarm.cloudPicName, '/');
					struct tm tmDate;
					localtime_r(&alarm.time, &tmDate);
					sprintf(tmppic,"%s/%s/%.4d%.2d%.2d%s",obss_info.days,alarm.ystNo,tmDate.tm_year + 1900,
							tmDate.tm_mon + 1, tmDate.tm_mday, ppic);	
					ppic = tmppic;
				}
				if (alarm.cloudVideoName[0])
				{
					pvedio = alarm.cloudVideoName;
				}
			}
			else
			{
				if (alarm.PicName[0])
				{
					ppic = alarm.PicName;
				}
				if (alarm.VideoName[0])
				{
					pvedio = alarm.VideoName;
				}
			}
			ret = mbizclient_PushAlarm("DOOR",alarm.ystNo,alarm.nChannel,ALARM_TEXT,alarm.uid,alarm.alarmType,alarm.time,ppic,pvedio);
			printf("MD mbizclient_PushAlarm successful!!!! %d\n\n", ret);
		}

		if(bCloudRecEnable && alarm.cloudPicName[0])
		{
			alarm.cmd[1] = ALARM_PIC;
			mcloud_upload_alarm_file(&alarm); 
		}
#endif
	}

	if(doorAlarmCfg->bSendEmail)
	{
		//Printf("Now malarm_sendmail\n");
		malarm_sendmail(0, mlog_translate("MDoorAlarm: DoorAlarm Warning")); //借用mlog的多语言管理
		mlog_write("MDoorAlarm: Mail Sended");
	}

	ipcinfo_t info;
	ipcinfo_get_param(&info);
/*	if(doorAlarmCfg->bSendtoVMS)
	{
		//printf("send door alarm to vms\n");
		AlarmInfo_t alarmInfo;
		alarmInfo.channel = 0;
		jv_ystNum_parse(alarmInfo.dev_id, info.nDeviceInfo[6], info.ystID);
		strcpy(alarmInfo.dev_type, "ipc");
		strcpy(alarmInfo.detector_id, "12345");
		strcpy(alarmInfo.type, "io");
		strcpy(alarmInfo.subtype, "doorAlarm");
		//strcpy(alarmInfo.content, "door alarm");
		utl_iconv_gb2312toutf8("门磁报警", alarmInfo.pir_code, sizeof(alarmInfo.pir_code));
		cgrpc_alarm_report(&alarmInfo);

	}*/

	if(doorAlarmCfg->bSendtoClient || doorAlarmCfg->bBuzzing || stAlarm.bAlarmSoundEnable)
	{
		if (dooralarm_timer == -1)
		{
			dooralarm_timer = utl_timer_create("mdooralarm", 0, _mdooralarm_timer, NULL);
		}
		else
		{
			utl_timer_reset(dooralarm_timer, 0, _mdooralarm_timer, NULL);
		}
	}
}

static int _doorAlarmConfig(PACKET *packet)
{
	char *buf = (char *)packet->acData;
	char *pdata = buf;
	char type = 0;
	char guid = 0;
	char enable = 0;
	unsigned char name[32] = {0};
	int ret;

	Printf("jvGpinConfig: %s\n", buf);

	type = GetIntValueWithDefault(buf, "type", 0);
	guid = GetIntValueWithDefault(buf, "guid", 0);
	enable = GetIntValueWithDefault(buf, "enable", 0);
	GetKeyValue(buf, "name", (char *)name, sizeof(name));

	printf("type=%d, guid=%d, enable=%d, name=%s\n", type, guid, enable, name);

	switch(packet->nPacketType)
	{
		case RC_GPIN_ADD:
			_type = type;
			mdooralarm_insert(type);
			break;
		case RC_GPIN_SET:
			ret = mdooralarm_set(guid, name, enable);
			printf("mdooralarm_set: ret=%d\n", ret);
			if(ret == 0)
			{
				pdata += sprintf(pdata,  "type=%d;", type);
				pdata += sprintf(pdata,  "guid=%d;", guid);
				pdata += sprintf(pdata,  "res=%d;", 1);
			}
			else
			{
				pdata += sprintf(pdata,  "type=%d;", type);
				pdata += sprintf(pdata,  "guid=%d;", guid);
				pdata += sprintf(pdata,  "res=%d;", 0);
			}
			break;
		case RC_GPIN_SELECT:
			ret = mdooralarm_select(buf);
			printf("mdooralarm_select: ret=%d, buf=%s\n", ret, buf);
			break;
		case RC_GPIN_DEL:
			ret = mdooralarm_del(guid);
			printf("mdooralarm_del: ret=%d, guid=%d\n", ret, guid);
			if(ret == 0)
			{
				pdata += sprintf(pdata,  "type=%d;", type);
				pdata += sprintf(pdata,  "guid=%d;", guid);
				pdata += sprintf(pdata,  "res=%d;", 1);
			}
			else
			{
				pdata += sprintf(pdata,  "type=%d;", type);
				pdata += sprintf(pdata,  "guid=%d;", guid);
				pdata += sprintf(pdata,  "res=%d;", 0);
			}
			break;
		default:
			break;
	}

	return ret;
}

static int _get_wifi_cfg_type(char *pData)
{
	U32 nSize = 0;
	char acItem[256] = {0};
	char value[256] = {0};

	pData[0] = '\0';

	//sprintf(acItem, "{");
	//nSize += strlen(acItem);
	//strcat(pData, acItem);

	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	char ystID[32] = {0};
	jv_ystNum_parse(value ,ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
	sprintf(acItem, "I=%s;", value);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	sprintf(value, "C=");
	switch(utl_ifconfig_wifi_get_model())
	{
		case WIFI_MODEL_REALTEK8188:
		case WIFI_MODEL_REALTEK8192:
			strcat(value, "A");
			if(strcmp(hwinfo.devName, "H211") &&
				strcmp(hwinfo.devName, "HXBJRB"))
			{
				strcat(value, ",");
				strcat(value, "V");
			}
			else
			{
				strcat(value, ",");
				strcat(value, "N");
			}	
			break;
		case WIFI_MODEL_RALINK7601:
			strcat(value, "Z");
			if(strcmp(hwinfo.devName, "AY"))
			{
				strcat(value, ",");
				strcat(value, "V");
			}
			else
			{
				strcat(value, ",");
				strcat(value, "N");
			}
			break;
		default:
			break;
	}
	strcat(value, ";");

	sprintf(acItem, "%s", value);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	//sprintf(acItem, "}");
	//nSize += strlen(acItem);
	//strcat(pData, acItem);

	printf("pData=%s, nSize=%d\n", pData, nSize);
	return nSize;
}

static void _remotecfg_data_proc(REMOTECFG *remotecfg)
{
	int len;

	char *js = (char *)&remotecfg->stPacket;
	if (strncmp("jsset:", js, 6) == 0)
	{
		//这里的格式为:jsset:%d;%s，其中%d为版本号，用于避免认错了回复内容。%s为实际内容
		char *oldjs = js;
		js = strchr(js, ';');
		if (js)
		{
			js++;
			printf("RC_JSPARAM cmd: %d\n%s\n", remotecfg->nSize, js);
#ifdef WEB_SUPPORT
			if (gp.bNeedWeb)
			{
				weber_extern(js, js);
			}
#endif
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)oldjs, strlen(oldjs)+1);
			printf("RC_JSPARAM result: %s\n", js);
		}
		return ;
	}
	else if (strncmp("grpc:", js, 5) == 0)
	{
		char tmpBuf[128*1024];
		memset(tmpBuf, 0, sizeof(tmpBuf));
		utl_iconv_utf8togb2312(js+5, tmpBuf, sizeof(tmpBuf));
		char *p = sgrpc_parse_yst(tmpBuf);
		if (p)
		{
			memset(tmpBuf, 0, sizeof(tmpBuf));
			len = utl_iconv_gb2312toutf8(p, tmpBuf, sizeof(tmpBuf));
			tmpBuf[len] = '\0';
			strcpy(js+5, tmpBuf);
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)js, strlen(js)+1);
			sgrpc_free_yst(p);
		}
	}

	Printf("stPacket.nPacketType=%d\n", remotecfg->stPacket.nPacketType);
	switch(remotecfg->stPacket.nPacketType)
	{
	case RC_WEB_PROXY:
		len = strlen((char *)remotecfg->stPacket.acData) < sizeof(remotecfg->stPacket.acData)? strlen((char *)remotecfg->stPacket.acData) : sizeof(remotecfg->stPacket.acData);
		if (strncmp((char *)remotecfg->stPacket.acData, "(null)", 6) == 0)
			proxy_close_socket();
		else
			proxy_send2server(remotecfg->stPacket.acData, len, remotecfg);
		break;
	case RC_GETPARAM:
		Printf("Client get param...\n");
		memset(&remotecfg->stPacket, 0, sizeof(PACKET));
		remotecfg->stPacket.nPacketType	= RC_GETPARAM;
		remotecfg->stPacket.nPacketCount	= HWINFO_STREAM_CNT/2;	//向分控发送自己是几路DVR，不发送，默认为四路
		GetDVRParam((char *)remotecfg->stPacket.acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, strlen((char *)remotecfg->stPacket.acData)+4);
		Printf("%s\n", remotecfg->stPacket.acData);
		break;
	case RC_LOADDLG:
		;U32 nDll = remotecfg->stPacket.nPacketID;
		U32 nVer = *((U32*)remotecfg->stPacket.acData), nDataLen = 0;
		Printf("nDll:%d, version:0x%x, chn:%d,Client load ipcam's dialog, ver:0x%x...\n", nDll, gp.nIPCamCfg,remotecfg->nCh, nVer);
		memset(&remotecfg->stPacket, 0, sizeof(PACKET));
		remotecfg->stPacket.nPacketID		= nDll;
		remotecfg->stPacket.nPacketType		= RC_LOADDLG;
		remotecfg->stPacket.nPacketCount	= HWINFO_STREAM_CNT/2;	//向分控发送自己时几路DVR，不发送，默认为四路
		remotecfg->stPacket.nPacketLen		= ipcinfo_get_param(NULL)->nLanguage;//语言
		if(nVer > 0 && nVer == gp.nIPCamCfg)
		{
			nDataLen = LoadIPCamDlg(remotecfg, nDll, FALSE, remotecfg->stPacket.acData);
		}
		else if(1==nVer)//如果版本号是1则不加载对话框,lck20120924
		{
			nDataLen = LoadIPCamDlg(remotecfg, nDll, FALSE, remotecfg->stPacket.acData);
		}
		else
		{
			nDataLen = LoadIPCamDlg(remotecfg, nDll, TRUE, remotecfg->stPacket.acData);
		}
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, nDataLen+4);
		break;
	case RC_SETPARAM:
		Printf("Client SetParams:%s\n", remotecfg->stPacket.acData);
		SetDVRParam(remotecfg, (char *)remotecfg->stPacket.acData);
		malarm_flush();
		break;
	case RC_GETSYSTIME:
		Printf("Client get system time...\n");
		memset(&remotecfg->stPacket, 0, sizeof(PACKET));
		remotecfg->stPacket.nPacketType	= RC_GETSYSTIME;
		struct tm	*pTm	= localtime(&gp.ttNow);
		sprintf((char *)remotecfg->stPacket.acData, "%d:%.4d-%.2d-%.2d %.2d:%.2d:%.2d", gp.nTimeFormat, pTm->tm_year+1900, pTm->tm_mon+1, pTm->tm_mday,
		        pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, strlen((char *)remotecfg->stPacket.acData)+4);
		break;
	case RC_SETSYSTIME:
		Printf("Client set system time:%s\n", remotecfg->stPacket.acData);
		struct tm	stTm, stOldTm;
		int nTimeFormat;
//		ipcinfo_t mipcinfo;
		sscanf((char *)remotecfg->stPacket.acData, "%d:%d-%d-%d %d:%d:%d", &nTimeFormat, &stTm.tm_year, &stTm.tm_mon, &stTm.tm_mday,
		       &stTm.tm_hour, &stTm.tm_min, &stTm.tm_sec);
//		ipcinfo_get_param(&mipcinfo);
//		if(mipcinfo.bSntp==0 )// 没有选中才允许校时的格式修改
		{
		if(nTimeFormat != gp.nTimeFormat)
		{
			gp.nTimeFormat = nTimeFormat;
			WriteConfigInfo();
			if (gp.nTimeFormat == TF_YYYYMMDD)
			{
				mchnosd_set_time_format(-1, "YYYY-MM-DD hh:mm:ss");
				mlog_write("set time format：[%s]", "YYYY-MM-DD hh:mm:ss");
			}
			else if(gp.nTimeFormat == TF_MMDDYYYY)
			{
				mchnosd_set_time_format(-1, "MM/DD/YYYY hh:mm:ss");
				mlog_write("set time format：[%s]", "MM/DD/YYYY hh:mm:ss");
			}
			else
			{
				mchnosd_set_time_format(-1, "DD/MM/YYYY hh:mm:ss");
				mlog_write("set time format：[%s]", "DD/MM/YYYY hh:mm:ss");
			}
		}
		}
		stTm.tm_year -= 1900;
		stTm.tm_mon	-= 1;
		//验证输入的日期是否正确
		memcpy(&stOldTm, &stTm, sizeof(stTm));
		time_t old = time(0);
		time_t now = mktime(&stTm);
		if (   stTm.tm_year != stOldTm.tm_year
		        || stTm.tm_mon  != stOldTm.tm_mon
		        || stTm.tm_mday != stOldTm.tm_mday
		        || stTm.tm_hour != stOldTm.tm_hour
		        || stTm.tm_min  != stOldTm.tm_min
		        || stTm.tm_sec  != stOldTm.tm_sec
		        || now == -1
		   )
		{
			return;
		}

		ipcinfo_set_time_ex(&now);//设置时间
		break;
	case RC_GETDEVLOG:
		_remotecfg_send_devlog(remotecfg);
		break;
	case RC_EXTEND:
		Printf("RC_EXTEND->%d\n", remotecfg->stPacket.nPacketCount);
		switch(remotecfg->stPacket.nPacketCount)
		{
		case RC_EX_FIRMUP:
			FirmupProc(remotecfg);
			break;
		case RC_EX_NETWORK:
			NetworkProc(remotecfg);
			break;
		case RC_EX_STORAGE:
			StorageProc(remotecfg);
			break;
		case RC_EX_ACCOUNT:
			AccountProc(remotecfg);
			break;
		case RC_EX_PRIVACY:
			PrivacyProc(remotecfg);
			break;
		case RC_EX_MD:
			MDProc(remotecfg);
			break;
		case RC_EX_ALARM:
			MailProc(remotecfg);
			break;
		case RC_EX_SENSOR:
			SensorProc(remotecfg);
			break;
		case RC_EX_PTZ:
			PTZProc(remotecfg);
			break;
		case RC_EX_ALARMIN:
			MAlarmInProc(remotecfg);
			break;
		case RC_EX_REGISTER:
			MRegisterProc(remotecfg);
			break;
		case RC_EX_EXPOSURE:
			ROIProc(remotecfg);
			break;
		case RC_EX_QRCODE:
			QRCodeProc(remotecfg);
			break;
		case RC_EX_IVP:
			IVPProc(remotecfg);
			break;
		case RC_EX_DOORALARM:
			MDoorAlarmProc(remotecfg);
			break;
		case RC_EX_NICKNAME:
			YSTNickNameProc(remotecfg);
			break;
		case RC_EX_RESTRICTION:
			RestrictionProc(remotecfg);
			break;
		case RC_EX_PTZUPDATE:
			break;
		case RC_EX_COMTRANS:
			ComTransProc(remotecfg);
			break;
		case RC_EX_REQ_IDR:
			ReqIDRProc(remotecfg);
			break;
		case RC_EX_CHAT:
			ChatProc(remotecfg);
			break;
		case RC_EX_ALARMOUT:
			AlarmOutProc(remotecfg);
			break;
		case RC_EX_CLOUD:
			CloudProc(remotecfg);
			break;
		case RC_EX_STREAM:
			StreamProc(remotecfg);
			break;
		case RC_EX_PLAY_REMOTE:
			PlayRemoteProc(remotecfg);
			break;
		case RC_EX_HEARTBEAT:
			break;
		case RC_EX_DEBUG:
			DebugProc(remotecfg);
			break;
		default:
			break;
		}
		break;
	case RC_GPIN_ADD:
		_doorAlarmConfig(&remotecfg->stPacket);
		if(!_remoteCfg.nClientID)
			memcpy(&_remoteCfg, remotecfg, sizeof(REMOTECFG));
		break;
	case RC_GPIN_SET:
	case RC_GPIN_SELECT:
	case RC_GPIN_DEL:
	{
		int ret = 0;
		ret = _doorAlarmConfig(&remotecfg->stPacket);
		if (ret < 0)
		{
			Printf("dooralarm not support!!\n");
			break;	//此处返回，手机端接收超时认为不支持门磁报警
		}
		Printf("acData:%s\n", remotecfg->stPacket.acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, strlen((char *)remotecfg->stPacket.acData)+4);
		break;
	}
	case RC_BOARDEXT_CHECK:
	{
		int redBoardSta = 0;
		int lightBoardSta = 0;
		
		Printf("Client get board extend device param...\n");
		
		redBoardSta = mio_getboardext_status(0);	
		lightBoardSta = mio_getboardext_status(1);
		
		memset(&remotecfg->stPacket, 0, sizeof(PACKET));
		remotecfg->stPacket.nPacketType	= RC_BOARDEXT_CHECK;
		
		sprintf((char*)(remotecfg->stPacket.acData),"redboard=%d;lightboard=%d;",redBoardSta,lightBoardSta);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, strlen((char *)remotecfg->stPacket.acData)+4);
		Printf("board extend : type : %d , count : %d , %s\n", remotecfg->stPacket.nPacketType,remotecfg->stPacket.nPacketCount,remotecfg->stPacket.acData);		
		break;
	}
	case RC_GET_WIFI_CFG_TYPE:
		_get_wifi_cfg_type((char *)remotecfg->stPacket.acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, strlen((char *)remotecfg->stPacket.acData)+4);
		break;
	default:
		break;
	}
}

//处理客户端发送来的JVN_DATA_O消息,lck20121105
static void _jvn_data_o_process(int nLinkID, int nClientID, U8 *pBuffer, int nSize)
{
	PACKET_O stOPacket;
	memcpy(&stOPacket, pBuffer, nSize);

	Printf("nLinkID=%d, nClientID=%d, strData=:%s, nSize=%d\n",
		nLinkID, nClientID, stOPacket.acData, nSize);

	switch(stOPacket.nDataType)
	{
	case RC_GET_MOPIC:
		{
			memset(stOPacket.acData, 0, sizeof(stOPacket.acData));
			sprintf(stOPacket.acData, "Enabled=1;nPictureType=%d;", SctrlGetParam(NULL)->nPictureType);
			if(gp.bNeedYST)
			{
				JVN_SendDataTo(nClientID, JVN_DATA_O, (unsigned char *)&stOPacket, sizeof(stOPacket), 0, 0);
			}
		}
		break;
	case RC_SET_MOPIC:
		{
			//手机端可以设置IPC三种模式，
			U32 nPictureType=0;
			sscanf(stOPacket.acData, "nPictureType=%d", &nPictureType);
			if(SctrlGetParam(NULL)->nPictureType != nPictureType)
			{
				SctrlGetParam(NULL)->nPictureType = nPictureType;
				U32 nChannel = SctrlGetMOChannel();

				mstream_attr_t attrStream;
				mstream_get_param(nChannel, &attrStream);
				switch(nPictureType)
				{
				case 0://高清
					attrStream.width = 624;
					attrStream.height = 352;
					attrStream.framerate = 15;
					attrStream.bitrate = 318;
					break;
				case 1://标清
					attrStream.width = 368;
					attrStream.height = 208;
					attrStream.framerate = 15;
					attrStream.bitrate = 163;
					break;
				case 2://流畅
					attrStream.width = 368;
					attrStream.height = 208;
					attrStream.framerate = 15;
					attrStream.bitrate = 136;
					break;
				default:
					break;
				}
				mstream_set_param(nChannel, &attrStream);
				mstream_flush(nChannel);
				_resolution_changed(nChannel);
			}

			memset(stOPacket.acData, 0, sizeof(stOPacket.acData));
			sprintf(stOPacket.acData, "Enabled=1;nPictureType=%d;", SctrlGetParam(NULL)->nPictureType);
			if(gp.bNeedYST)
			{
				JVN_SendDataTo(nClientID, JVN_DATA_O, (unsigned char *)&stOPacket, sizeof(stOPacket), 0, 0);
			}
		}
		break;
	}
}

void remote_start_chat(int nLocalChannel, int clientid)
{
	static int pre_clientid = 0;
	jv_audio_attr_t ai_attr;
	
	if (!gp.ChatInfo.bTalking && (speakerowerStatus < JV_SPEAKER_OWER_CHAT))
	{
		//如果没有开对讲，则硬件上打开对讲
		mio_chat_start();
		jv_ao_mute(0);
		jv_ai_get_attr(0, &ai_attr);
		
		if(utl_ifconfig_wifi_get_mode() == WIFI_MODE_AP)
		{
			maudio_resetAIAO_mode(2);
			speakerowerStatus = JV_SPEAKER_OWER_NONE;
		}
	
		gp.ChatInfo.bTalking = TRUE;
		gp.ChatInfo.nClientID = clientid;
		gp.ChatInfo.nLocalChannel = nLocalChannel;
		
		speakerowerStatus = JV_SPEAKER_OWER_CHAT;
		
		malarm_sound_stop();
		while(!malarm_get_speakerFlag())
		{
			usleep(100*1000);
		}

		jv_ai_setChatStatus(TRUE);

		//修改一下逻辑，加个判断条件，因为jv_audio的结构中增加了静音和音量的设置
		//先判断结构中的值，如果不合法，再按照下面的默认处理
		if(ai_attr.level != -1)
		{
			if (PRODUCT_MATCH(PRODUCT_C3A)
				|| PRODUCT_MATCH("HC530A"))
			{
				ai_attr.level = 0x06;
			}
			jv_ao_ctrl(ai_attr.level);
		}
		else
		{
			if( PRODUCT_MATCH("H411") ||
				PRODUCT_MATCH("J2000IP-CmPTZ-111-V2.0") ||
				PRODUCT_MATCH("H411KEDA") ||
				PRODUCT_MATCH("H411C") ||
				PRODUCT_MATCH("WHHT") ||
				PRODUCT_MATCH("AT-15H2") || 
				PRODUCT_MATCH("HZD-600DM") || 
				PRODUCT_MATCH("HZD-600DN") || 
				PRODUCT_MATCH("AJL-H40610-S1") || 
				PRODUCT_MATCH("AJL-H40610-S2") || 
				PRODUCT_MATCH("JD-H40810") || 
				PRODUCT_MATCH("SW-H411V4"))
				jv_ao_ctrl(0x02);
			else if (PRODUCT_MATCH("H301") || 
				PRODUCT_MATCH("H303") || 
				PRODUCT_MATCH("A1") ||
				PRODUCT_MATCH("HW-H21110"))
				jv_ao_ctrl(0x07);
			else if(
				    PRODUCT_MATCH("HA320-H1") ||
					PRODUCT_MATCH("HA320-H1-A") || 
					PRODUCT_MATCH("HV120-H1") ||
					PRODUCT_MATCH("HA121-H2") ||
					HWTYPE_MATCH(HW_TYPE_C3) ||
					HWTYPE_MATCH(HW_TYPE_C3W) ||
					HWTYPE_MATCH(HW_TYPE_V3) ||
					HWTYPE_MATCH(HW_TYPE_V6)
					)
				jv_ao_ctrl(0x09);
			else if(PRODUCT_MATCH("H411S-H1") || 
					PRODUCT_MATCH("H411V2") || 
					PRODUCT_MATCH("HC420S-H2") || 
					PRODUCT_MATCH("HC420S-Q1") ||
					PRODUCT_MATCH("HA520D-H1") ||
					PRODUCT_MATCH("HC520D-H1") || 
					PRODUCT_MATCH("HC420-H2") ||
					PRODUCT_MATCH("H411-H1") ||
					PRODUCT_MATCH("SW-H411V3"))
				jv_ao_ctrl(0x0E);
			else if(PRODUCT_MATCH("H411V1_1") ||
					PRODUCT_MATCH("VS-DPCW-122"))
				jv_ao_ctrl(0x08);
			else
				;			//调过值的设备按调试值赋值，其他设备默认吧
		}
		pre_clientid = clientid;
	}

	if(pre_clientid == clientid)
	{
		//相同分控，单双向都发同意
		maudio_readfiletoao(VOICE_CHATSTARTTIP);
		if(gp.bNeedYST)
		{
			JVN_SendChatData(nLocalChannel, clientid, JVN_RSP_CHATACCEPT, 0, 0);
		}
	}
	else
	{
		// jv_ao_ctrl(0x09);
		if(gp.bNeedYST)
		{
			JVN_SendChatData(nLocalChannel, clientid, JVN_CMD_CHATSTOP, 0, 0);
		}
	}
}

//nLocalChannel值为-1时，不发送STOP消息出去。因为那代表着连接断开
void remote_stop_chat(int nLocalChannel, int clientid)
{
	if (gp.ChatInfo.nClientID == clientid && gp.ChatInfo.bTalking)
	{
		gp.ChatInfo.bTalking = FALSE;

		jv_ai_setChatStatus(FALSE);	
		
		mio_chat_stop();
		if(speakerowerStatus == JV_SPEAKER_OWER_CHAT)	
			speakerowerStatus = JV_SPEAKER_OWER_NONE;
		maudio_readfiletoao(VOICE_CHATCLOSETIP);
		usleep(1000*1000);
		if(PRODUCT_MATCH("HA220-H2-A"))
			jv_ao_ctrl(0x02);
		else if(PRODUCT_MATCH("HC420-H2"))
			jv_ao_ctrl(0x04);
		else if(PRODUCT_MATCH("HZD-600DM") || PRODUCT_MATCH("HZD-600DN"))
			jv_ao_ctrl(0x2);
		else if (PRODUCT_MATCH(PRODUCT_C3A)
			|| PRODUCT_MATCH("HC530A"))
			jv_ao_ctrl(0xa);
		else
			jv_ao_ctrl(0x09);
		jv_ao_mute(1);
	}
}

//发送对讲数据
void remote_send_chatdata(int nLocalChannel, char *buffer, int len)
{
	if(!gp.bNeedYST)
		return;
	//printf("talking: %d, %d== %d, len: %d\n", gp.ChatInfo.bTalking, nLocalChannel, gp.ChatInfo.nLocalChannel, len);
	if (gp.ChatInfo.bTalking)
	{
		if (gp.ChatInfo.nLocalChannel == nLocalChannel)
			JVN_SendChatData(nLocalChannel, gp.ChatInfo.nClientID, JVN_RSP_CHATDATA, (U8 *)buffer, len);
	}
}

static void _remotecfg_process(void *param)
{
	int len;
	REMOTECFG *remotecfg = NULL;
	REMOTECFG *Item = NULL;

	pthreadinfo_add((char *)__func__);

	remotecfg = calloc(1, sizeof(REMOTECFG));
	if (!remotecfg)
	{
		printf("%s, calloc failed!\n", __func__);
		return;
	}

	while(group.running)
	{
		//放在前边，是为了避免哪天有人在循环中添加一个continue，导致内存泄漏
		if (Item != NULL)
		{
			free(Item);
			Item = NULL;
		}
		len = utl_queue_recv(group.iMqHandle, (char *)&Item, -1);
		if (len != 0)
		{
			Printf("timeout happened when mq_receive data\n");
			usleep(1);
			continue;
		}
		if (Item == NULL)
			continue;

		// 取出实际有效数据
		// REMOTE_CFG_SIZE计算时多加了一些额外字节，有效数据后，会有一段空数据
		// 因此，这里不用再清空整个remotecfg，提高效率
		memcpy(remotecfg, Item, REMOTE_CFG_SIZE(Item->nSize));

		U32 bPower	= FALSE;

		U32 nCmd	= remotecfg->nCmd;
		Printf("********** remotecfg->nCmd:%x\n", nCmd);
		//先清空事件类型，以求最大限度减少线程冲突
		switch (nCmd)
		{
		case JVN_REQ_TEXT:
			Printf("Req text, verity, nCh=%d, ...\n", remotecfg->nCh);
			bPower	= _CheckClientPower(remotecfg->nClientID, POWER_USER|POWER_ADMIN);	//判断连接是否有远程设置权限
			if(-1 == remotecfg->nCh)
			{
				bPower = TRUE;
			}
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, bPower?JVN_RSP_TEXTACCEPT:JVN_CMD_TEXTSTOP, NULL, 0);
			break;
		case JVN_RSP_TEXTDATA:
			//Printf("TextData...\n");
			_remotecfg_data_proc(remotecfg);
			break;
		case JVN_CMD_TEXTSTOP:
			Printf("ok, text stop...\n");
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_CMD_TEXTSTOP, NULL, 0);
			break;
		case JVN_DATA_O:
			_jvn_data_o_process(remotecfg->nCh, remotecfg->nClientID, (U8*)&remotecfg->stPacket, remotecfg->nSize);
			break;

		case JVN_REQ_CHAT:
			remote_start_chat(remotecfg->nCh, remotecfg->nClientID);
			break;
		case JVN_CMD_CHATSTOP:
			remote_stop_chat(remotecfg->nCh, remotecfg->nClientID);
			if(gp.bNeedYST)
			{
				JVN_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_CMD_CHATSTOP, 0, 0);
			}
			break;
		default:
			break;
		}
	}
	Printf("out of _remotecfg_process\n");
}

static void _remote_webproxy_callback(unsigned char *buffer, int len, void *param)
{
	static unsigned char index = 0;
	REMOTECFG *remotecfg = (REMOTECFG *)param;

	if (buffer)
	{
		memcpy(remotecfg->stPacket.acData, buffer, len);
	}
	else
	{
		strcpy((char *)remotecfg->stPacket.acData, "(null)");
		len = strlen("(null)");
	}
	if (len < 80)
	{
		remotecfg->stPacket.acData[len] = '\0';
		len = 80;
	}
	remotecfg->stPacket.nPacketID = index++;
	remotecfg->stPacket.nPacketLen = len;
	//printf(" ==== %d ====\n", remotecfg->stPacket.nPacketID );
	//printf("%s\n", buffer);
	MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, len + 4);
	//usleep(100*1000);
}

/**
 *@brief 初始化
 *
 */
int remotecfg_init(void)
{
	group.iMqHandle = utl_queue_create("remote_cfg_mq", 4, 64);
	if (group.iMqHandle < 0)
	{
		return -1;
	}

	pthread_mutex_init(&group.mutex, NULL);

	group.running = TRUE;
	pthread_create(&group.thread, NULL, (void *)_remotecfg_process, NULL);

	proxy_init(_remote_webproxy_callback, "127.0.0.1", 80);
	return 0;
}

/**
 *@brief 结束
 *
 */
int remotecfg_deinit(void)
{
	group.running = FALSE;

	//发送空消息让remotecfg退出,lck20120808
	unsigned int iNULL = 0;
	utl_queue_send(group.iMqHandle, &iNULL);

	pthread_join(group.thread, NULL);
	utl_queue_destroy(group.iMqHandle);

	pthread_mutex_destroy(&group.mutex);

	Printf("remotecfg_deinit over\n");
	return 0;
}

int remotecfg_sendmsg(REMOTECFG *cfg)
{
	return utl_queue_send(group.iMqHandle, (char *)&cfg);
}

void remotecfg_alarmin_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_ALARM);
}

void remotecfg_mdetect_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_MOTION);
}

void remotecfg_mbabycry_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_BABYCRY);
}

void remotecfg_mpir_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_PIR);
}

void remotecfg_mivp_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_IVP);
}

void remotecfg_mivp_vr_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_IVP_VR);
}

void remotecfg_mivp_hide_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_IVP_HIDE);
}

//物品遗留报警
void remotecfg_mivp_left_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_IVP_LEFT);
}

//物品取走报警
void remotecfg_mivp_removed_callback(int channelid, BOOL bAlarmOn)
{
	remotecfg_alarm_on(channelid, bAlarmOn, ALARM_TYPE_IVP_REMOVED);
}


