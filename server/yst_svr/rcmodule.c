#include "jv_common.h"
#include "sctrl.h"
#include "MRemoteCfg.h"
#include "msensor.h"
#include "mstream.h"
#include "msnapshot.h"
#include "JvServer.h"
#include "malarmout.h"
#include "mtransmit.h"
#include "mdetect.h"
#include "malarmin.h"
#include "SYSFuncs.h"
#include "rcmodule.h"
#include "mprivacy.h"
#include "mstorage.h"
#include "mipcinfo.h"
#include "utl_ifconfig.h"
#include "mptz.h"
#include <mlog.h>
#include <mrecord.h>
#include "mivp.h"
#include "mosd.h"
#include "muartcomm.h"
#include "utl_timer.h"
#include "mvoicedec.h"
#include <msoftptz.h>
#include "mcloud.h"
#include "alarm_service.h"
#include "mplay_remote.h"
#include "mioctrl.h"
#include "mdebug.h"
#include "mdooralarm.h"


U32 GetIntValueWithDefault(char *pBuffer, char *pItem, U32 defVal)
{
	int i = 0;
	char *ptmpItem;
	char *pValue = NULL;  //gyd
	char acBuff[256] =
	{ 0 };

	while (*pBuffer)     //gyd
	{
		i = 0;
		memset(acBuff, 0, sizeof(acBuff));
		while (*pBuffer != ';')
		{
			acBuff[i++] = *pBuffer++;
		}
		ptmpItem = strtok(acBuff, "=");
		pValue = strtok(NULL, "\r");

		if (strcmp(ptmpItem, pItem) == 0)
		{
			break;
		}
		pValue = NULL;
		pBuffer++;
	}
	if (pValue != NULL)
	{
		Printf("%s=%s\n", pItem, pValue);

		//返回获取到的值
		return atoi(pValue);
	}
	return defVal;
}

//返回参数长度
U32 GetIntValue(char *pBuffer, char *pItem)
{
	return GetIntValueWithDefault(pBuffer, pItem, 0xFFFF);
}

char *GetKeyValue(const char *pBuffer, const char *key, char *valueBuf, int maxValueBuf)
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


//---------------------网络远程设置------------------
//添加网络配置参数,lck20120215
U32 build_network_param(char *pData)
{
	U32 nSize = 0;
	char acItem[256] = { 0 };
	ipcinfo_t ipcinfo;
	eth_t eth;
	eth_t current;
	pppoe_t pppoe;
	wifiap_t wifiap;
	char curIface[32] = {0};
	int net_alive=ETH_NET;

	pData[0] = '\0';
	utl_ifconfig_eth_get(&eth);
	utl_ifconfig_ppp_get(&pppoe);

	//获取当前网络
	//utl_ifconfig_get_inet(acItem);
	int bLinkDown = utl_ifconfig_b_linkdown("eth0");
	if (bLinkDown == 0)
	{
		utl_ifconfig_get_inet(acItem);
		if (strcmp(acItem, "ppp") == 0)
		{
			snprintf(curIface, sizeof(curIface), "%s", "ppp0");
			net_alive = PPPOE_NET;
		}	
		else
		{
			snprintf(curIface, sizeof(curIface), "%s", "eth0");
			net_alive = ETH_NET;
		}
	}
	if(bLinkDown && utl_ifconfig_wifi_bsupport())
	{
		utl_ifconfig_get_iface(curIface);
		net_alive = WIFI_NET;
	}

	utl_ifconfig_build_attr(curIface, &current, FALSE);

	ipcinfo_get_param(&ipcinfo);
	//refresh_net_info(0);
	//添加默认连接信息
	sprintf(acItem, "ACTIVED=%d;", (net_alive - ETH_NET));
	stNetworkInfo.nActived = (net_alive - ETH_NET);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//---------------以太网参数---------------------
	//添加ETH获取IP方式
	sprintf(acItem, "bDHCP=%d;", eth.bDHCP);
	stNetworkInfo.dhcp = eth.bDHCP;
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加IP地址信息
	sprintf(acItem, "ETH_IP=%s;", current.addr);
	strcpy(stNetworkInfo.eth_acip, eth.addr);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加子网掩码信息
	sprintf(acItem, "ETH_NM=%s;", current.mask);
	strcpy(stNetworkInfo.eth_acnm, eth.mask);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加网关信息
	sprintf(acItem, "ETH_GW=%s;", current.gateway);
	strcpy(stNetworkInfo.eth_acgw, eth.gateway);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加DNS信息
	sprintf(acItem, "ETH_DNS=%s;", current.dns);
	strcpy(stNetworkInfo.eth_acdns, eth.dns);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加MAC信息
	sprintf(acItem, "ETH_MAC=%s;", current.mac);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//---------------ADSL网参数---------------------
	if (net_alive == PPPOE_NET)
	{
		utl_ifconfig_build_attr("ppp0", &current, FALSE);
		//添加IP地址信息
		sprintf(acItem, "ADSL_IP=%s;", current.addr);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加子网掩码信息
		sprintf(acItem, "ADSL_NM=%s;", current.mask);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加网关信息
		sprintf(acItem, "ADSL_GW=%s;", current.gateway);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加DNS信息
		sprintf(acItem, "ADSL_DNS=%s;", current.dns);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加MAC信息
		sprintf(acItem, "ADSL_MAC=%s;", current.mac);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加ADSL帐号
		sprintf(acItem, "ADSL_ID=%s;", pppoe.username);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加ADSL密码
		sprintf(acItem, "ADSL_PW=%s;", pppoe.passwd);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//网络是否连接
		sprintf(acItem, "ADSL_ON=%d;", utl_ifconfig_check_status("ppp0"));
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
	//---------------WIFI网参数---------------------
	if (net_alive == WIFI_NET)
	{
		//添加IP地址信息
		sprintf(acItem, "WIFI_IP=%s;", current.addr);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加子网掩码信息
		sprintf(acItem, "WIFI_NM=%s;", current.mask);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加网关信息
		sprintf(acItem, "WIFI_GW=%s;", current.gateway);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加DNS信息
		sprintf(acItem, "WIFI_DNS=%s;", current.dns);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//添加MAC信息
		sprintf(acItem, "WIFI_MAC=%s;", current.mac);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//wifi是否已经连接
		sprintf(acItem, "WIFI_ON=%d;", utl_ifconfig_check_status(curIface));
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
	else
	{
		sprintf(acItem, "WIFI_ON=0;");
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}

	//--------添加当前连接的AP的信息-----
	//如果连接成功则不用扫描节点，直接从文件获取信息
	//直接从文件获取不再重新扫描
	utl_ifconfig_wifi_info_get(&wifiap);
	//AP名称
	sprintf(acItem, "WIFI_ID=%s;", wifiap.name);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加AP密码
	sprintf(acItem, "WIFI_PW=%s;", wifiap.passwd);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加AP信号
	sprintf(acItem, "WIFI_Q=%d;", wifiap.quality);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加认证方式
	sprintf(acItem, "WIFI_AUTH=%d;", wifiap.iestat[0]);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//添加加密类型
	sprintf(acItem, "WIFI_ENC=%d;", wifiap.iestat[1]);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//wifi模式
	sprintf(acItem, "WIFI_MODE=%d;", utl_ifconfig_wifi_get_mode());
	nSize += strlen(acItem);
	strcat(pData, acItem);

	YST stYST;
	GetYSTParam(&stYST);
	//云视通分组
	sprintf(acItem, "YSTGROUP=%d;", ipcinfo.nDeviceInfo[6]);
	strcat(pData + nSize, acItem);
	nSize += strlen(acItem);
	//云视通号码，下面两项直接使用的全局变量访问，稍后进行修改,lck20120618
	sprintf(acItem, "YSTID=%d;", stYST.nID);
	strcat(pData + nSize, acItem);
	nSize += strlen(acItem);
	//云视通在线状态
	sprintf(acItem, "YSTSTATUS=%d;", stYST.nStatus);
	strcat(pData + nSize, acItem);
	nSize += strlen(acItem);

#ifdef GB28181_SUPPORT
	if (1)	//GB28181相关
	{
		GBRegInfo_t info;
		mgb28181_get_param(&info);
		sprintf(acItem, "GB_bSupport=%d;", hwinfo.bHomeIPC?0:1); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_bEnable=%d;", info.bEnable); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_devid=%s;", info.devid); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_devpasswd=%s;", info.devpasswd); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_serverip=%s;", info.serverip); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_serverport=%d;", info.serverport); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_localport=%d;", info.localport); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_expires=%d;", info.expires); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_alarminID=%s;", info.alarminID[0]); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_keepalive=%d;", info.keepalive); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_KA_outtimes=%d;", info.keepalive_outtimes); strcat(pData+nSize, acItem); nSize += strlen(acItem);
		sprintf(acItem, "GB_EX_refresh=%d;", info.expires_refresh); strcat(pData+nSize, acItem); nSize += strlen(acItem);
	}
#endif
	Printf("Network's param:%s\n", pData);
	return nSize;
}

static void _executeCMD(const char *cmd, char *result)
{    
	char buf_ps[1024];
	char ps[1024]={0};
	FILE *ptr;
	strcpy(ps, cmd);
	if((ptr=popen(ps, "r"))!=NULL)
	{       
		while(fgets(buf_ps, 1024, ptr)!=NULL)
		{           
			strcat(result, buf_ps);
			if(strlen(result)>1024)
				break;        
		}

		pclose(ptr); 
		ptr = NULL;
	} 
	else 
	{  
		printf("popen %s error\n", ps);
	}
}

static void _praseandShowRssid(char* buf)
{
	char* tmps = NULL;
	char* tmpe = NULL;
	char sigLevel[32] = {0};
	
	tmps = strstr(buf,"Signal level");
	tmpe = strstr(tmps,"Noise");
	
	strncpy(sigLevel,tmps,tmpe-tmps);
	
	mchnosd_debug_mode(TRUE,sigLevel);
	
}

static void _show_wifi_rssid(void* param)
{

    char result[1024] = {0};
	pthreadinfo_add((char *)__func__);
	while(1)
	{
		if(!utl_ifconfig_b_linkdown("eth0"))
		{
			usleep(3*1000*1000);
			continue;
		}
		
		if(access("/tmp/iwconfig",F_OK) == 0)
			_executeCMD( "/tmp/iwconfig wlan0", result);	
		
		if(strlen(result) != 0)
			_praseandShowRssid(result);

		result[0] = '\0';
		
		usleep(500 * 1000);
	}

}


void _rcmodule_get_wifi_list_func(REMOTECFG *remotecfg)
{
	char *pData = (char *)remotecfg->stPacket.acData;
	S32 nSize = remotecfg->nSize - 4;
	EXTEND *pstEx = (EXTEND*) remotecfg->stPacket.acData;
	
	if (utl_ifconfig_wifi_bsupport())
	{
		wifiap_t *list = utl_ifconfig_wifi_get_ap();
		pstEx->nType = EX_WIFI_AP;
		pstEx->nParam1 = utl_ifconfig_wifi_list_cnt(list);
		printf(">>>lk test<<< wifi count :%d\n",pstEx->nParam1);
		pstEx->nParam2 = sizeof(wifiapOld_t) * pstEx->nParam1;
#if 0//DEBUG
		int i;
		for(i = 0;i< pstEx->nParam1;i++)
		{
			printf("~~~~~~~~lk test wifi out~~~~%s\n",list[i].name);
		}
#endif

		wifiapOld_t wifiapOld;
		wifiap_t *wifiap = NULL;
		int i;
		for(i=0; i<pstEx->nParam1; i++)
		{
			wifiap = list + i;
			snprintf(wifiapOld.name, sizeof(wifiapOld.name), "%s", wifiap->name);
			snprintf(wifiapOld.passwd, sizeof(wifiapOld.passwd), "%s", wifiap->passwd);
			wifiapOld.quality = wifiap->quality;
			wifiapOld.keystat = wifiap->keystat;
			snprintf(wifiapOld.iestat, sizeof(wifiapOld.iestat), "%s", wifiap->iestat);
			memcpy(pstEx->acData+sizeof(wifiapOld_t)*i, &wifiapOld, sizeof(wifiapOld_t));
		}
	}
	else
	{
		pstEx->nType = EX_WIFI_AP;
		pstEx->nParam1 = 0;
		pstEx->nParam2 = 0;
	}
	MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
			JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket,
			20 + pstEx->nParam2);
}

void* _rcmodule_get_wifi_list_thread(void* p)
{
	REMOTECFG* remotecfg = (REMOTECFG*)p;

	pthreadinfo_add((char *)__func__);
	printf("get wifi list in thread\n");
	_rcmodule_get_wifi_list_func(remotecfg);
	free(remotecfg);
	return 0;
}

//远程设置消息处理函数
VOID NetworkProc(REMOTECFG *remotecfg)
{
	pppoe_t pppoe;
	char *pData = (char *)remotecfg->stPacket.acData;
	S32 nSize = remotecfg->nSize - 4;
	EXTEND *pstEx = (EXTEND*) remotecfg->stPacket.acData;
	char acID[32] = { 0 };
	char acPW[64] = { 0 };
	ACCOUNT *account;
	ACCOUNT stAccount;

	account = GetClientAccount(remotecfg->nClientID, &stAccount);

	if (account == NULL)
	{
		Printf("ERROR: account is NULL, remotecfg->nClientID: %d\n", remotecfg->nClientID);
		return;
	}

	if (!(account->nPower & POWER_ADMIN))
	{
		Printf("User have no permision to update the devices, pstEx->nType: %d\n", pstEx->nType);
		pstEx->nParam1 = COMM_NO_PERMISSION;	// 4. permision denied
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20);
		return;
	}

	Printf("RC_EXTEND, nSize=%d\n", nSize);
	if (nSize <= 0)
		return;

	Printf("**  type:%x\n",pstEx->nType);
	switch (pstEx->nType)
	{
	case EX_ADSL_ON:
		memcpy(pppoe.username, pstEx->acData, pstEx->nParam1);
		memcpy(pppoe.passwd, pstEx->acData + pstEx->nParam1, pstEx->nParam2);
		utl_ifconfig_ppp_set(&pppoe);
		break;
	case EX_ADSL_OFF:
		Printf("EX_ADSL_OFF\n");
		//disconn_pppoe();
		utl_system("pppoe-stop");
		break;
	case EX_WIFI_AP:
		Printf("EX_WIFI_AP\n");
		if (strcmp(hwinfo.devName, "HXBJRB") == 0)
		{
			pthread_t wifi_list_thread;
			REMOTECFG* recfg = (REMOTECFG*)malloc(sizeof(REMOTECFG));	//free in thread
			*recfg = *remotecfg;
			pthread_create(&wifi_list_thread, NULL, _rcmodule_get_wifi_list_thread, (void*)recfg);
			pthread_detach(wifi_list_thread);
		}
		else
		{
			printf("get wifi list in func\n");
			_rcmodule_get_wifi_list_func(remotecfg);
		}
		break;
	case EX_WIFI_AP_DLL:
		printf("dll get ap list\n");
		if (utl_ifconfig_wifi_bsupport())
		{
			wifiap_t *list = utl_ifconfig_wifi_get_ap();
			pstEx->nType = EX_WIFI_AP_DLL;
			pstEx->nParam1 = utl_ifconfig_wifi_list_cnt(list);
			printf(">>>lk test<<< wifi count :%d\n",pstEx->nParam1);
			pstEx->nParam2 = sizeof(wifiap_t) * pstEx->nParam1;
#if 0//DEBUG
			int i;
			for(i = 0;i< pstEx->nParam1;i++)
			{
				printf("~~~~~~~~lk test wifi out~~~~%s\n",list[i].name);
			}
#endif
			memcpy(pstEx->acData, list, pstEx->nParam2);
		}
		else
		{
			pstEx->nType = EX_WIFI_AP_DLL;
			pstEx->nParam1 = 0;
			pstEx->nParam2 = 0;
		}
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket,
				20 + pstEx->nParam2);
		break;
	case EX_START_AP:
		Printf("*******************AP Start %d*********************\n", pstEx->nParam1);
		if(utl_ifconfig_wifi_get_model() == WIFI_MODEL_RALINK7601)	
		{
			break;
		}
		int bap=0;
		pstEx->nType = EX_START_AP;
		if(pstEx->nParam1==0)
		{
			if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_AP)
			{
				bap=1;
			}
			else
			{
				bap=2;
			}
			pstEx->nParam1 = bap;
			if (!utl_ifconfig_wifi_bsupport())
				pstEx->nParam1 = -1;
			Printf("*******************AP Start %d\n", pstEx->nParam1);
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20);
		}
		else if(pstEx->nParam1 == 1)//开启AP
		{
			if(strcmp(hwinfo.devName,"HXBJRB") == 0)
			{
				net_deinit();
				utl_ifconfig_wifi_start_ap();
			}
			else
			{
				utl_ifconfig_wifi_save_remove();
				SYSFuncs_reboot();
			}
		}
		break;
	case EX_START_STA:
		Printf("*******************STA Start %d*********************\n", pstEx->nParam1);
		int bSTA=0;
		pstEx->nType = EX_START_STA;
		if(pstEx->nParam1==0)
		{
			if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
			{
				bSTA=1;
			}
			else
			{
				if(utl_ifconfig_wifi_STA_configured())
					bSTA=2;
			}
			pstEx->nParam1 = bSTA;
			if (!utl_ifconfig_wifi_bsupport())
				pstEx->nParam1 = -1;
			Printf("*******************STA Start %d\n", pstEx->nParam1);
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20);
		}
		else if(pstEx->nParam1 == 1)//开启STA
		{
			if(strcmp(hwinfo.devName,"HXBJRB") == 0)
			{
				wifiap_t wifiap;
				utl_ifconfig_wifi_start_sta();
				utl_ifconfig_wifi_info_get(&wifiap);
				net_deinit();
				utl_ifconfig_wifi_connect(&wifiap);
			}
		}
		break;
	case EX_WIFI_ON:
		Printf("EX_WIFI_ON\n");
		memcpy(acID, pstEx->acData, pstEx->nParam1);
		memcpy(acPW, pstEx->acData + pstEx->nParam1, pstEx->nParam2);
		printf("EX_WIFI_ON, acID=%s, acPW=%s\n", acID, acPW);
		wifiap_t *wifiap;
		wifiap = utl_ifconfig_wifi_get_by_ssid(acID);
		if (wifiap == NULL)
			break;
		strncpy(wifiap->passwd, acPW, sizeof(wifiap->passwd));
		utl_ifconfig_wifi_connect(wifiap);
		break;
	case EX_WIFI_OFF:
		Printf("EX_WIFI_OFF\n");
		utl_system("killall wpa_supplicant");
		utl_system("killall udhcpc ");
		break;
	case EX_NETWORK_OK:
		Printf("EX_NETWORK_OK\n");
		break;
	case EX_UPDATE_ETH:
		Printf("EX_UPDATE_ETH, %s\n", pstEx->acData);
		SetDVRParam(remotecfg, (char *)pstEx->acData);
		break;
	case EX_NW_REFRESH:
		Printf("EX_NW_REFRESH\n");
		pstEx->nParam1 = 0;
		memset(pstEx->acData, 0, RC_DATA_SIZE - 16);
		pstEx->nParam2 = build_network_param((char *)pstEx->acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket,
				20 + pstEx->nParam2);
		break;
	case EX_NW_SUBMIT:
		printf("Network params:%s\n", pstEx->acData);
		SetDVRParam(remotecfg, (char *)pstEx->acData);
		break;
	case EX_WIFI_AP_CONFIG:
		printf("EX_WIFI_AP_CONFIG\n");
		wifiap_t ap;
		SctrlGetKeyValue((char *)pstEx->acData, "wifi_ssid", ap.name, sizeof(ap.name));
		SctrlGetKeyValue((char *)pstEx->acData, "wifi_pwd", ap.passwd, sizeof(ap.passwd));
		memset(ap.iestat, -1, sizeof(ap.iestat));
		printf("ID:%s,PWD:%s\n", ap.name,ap.passwd);

		pstEx->nParam1=0;
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20);

		sleep(2);

		utl_ifconfig_wifi_connect_ap(&ap, TRUE);
		break;
	case EX_WIFI_SHOW_RSSID:
		if(gp.bFactoryFlag)
		{
			utl_system("mount_as_tmpfs /home/libs");
			utl_system("cp /progs/rec/00/libiw.so.29 /home/libs");

			utl_system("cp /progs/rec/00/iwconfig /tmp");

			pthread_t pid;
			pthread_create(&pid, NULL, (void *)_show_wifi_rssid, NULL);	
			pthread_detach(pid);		
		}
		break;
	default:
		break;
	}
}

static int _get_stream_def_param(int chn, int *w, int *h, int *fr, int *br, int vencType)
{
	jvstream_ability_t tmp_ab;

	jv_stream_get_ability(0,&tmp_ab);

	if(w)
	{
		*w = tmp_ab.inputRes.width;
		if(chn == 1)
			*w = 704;
		if(chn == 2)
			*w = 352;
	}

	if(h)
	{
		*h = tmp_ab.inputRes.height;
		if (chn == 1)
			*h = 576;
		if (chn == 2)
			*h = 288;
	}

	if(fr)
		*fr = 25;

#ifdef PLATFORM_hi3516D
	if(tmp_ab.maxStreamRes[0]==2592*1520)
		if (chn == 0)
			*fr = 20;
#endif

#ifdef PLATFORM_hi3518C
	if(tmp_ab.maxStreamRes[0]==1920*1080)
	{
		*fr = 19;
	}
#endif

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100)
	if(chn == 1)
	{
		*fr = 20;
	}
	if(chn == 0)
	{		
		if(tmp_ab.maxStreamRes[0] == 1920*1080)
		{
			*fr = 20;
		}
	}
#endif

	*br = __CalcBitrate(*w, *h, *fr, vencType);

	return 0;
}

//添加视频配置参数,lck20120221
U32 build_stream_param(char *pData)
{
	U32 i, nSize = 0;
	mstream_attr_t stAttr;
	char acItem[256] =
	{ 0 };
	int ttt;
	pData[0] = '\0';
	for (i = 0; i < HWINFO_STREAM_CNT; i++)
	{
		mstream_get_param(i, &stAttr);
//		printf("-----------------------------------------ff-bitrate:%d\n",stAttr.bitrate);

		sprintf(acItem, "[CH%d];", i + 1);
		nSize += strlen(acItem);
		strcat(pData, acItem);

		if((!strcmp(hwinfo.devName, "SW-H210V3") || 
			!strcmp(hwinfo.devName, "SW-H411V3")) &&
			(hwinfo.sensor == SENSOR_AR0130 || 
			hwinfo.sensor == SENSOR_OV9750))
		{
			if(stAttr.height == 720)
				stAttr.height = 960;
		}
		if(!strcmp(hwinfo.devName, "SW-H411V4"))
		{
			if(stAttr.height == 720)
				stAttr.height = 960;
		}
		sprintf(acItem, "width=%d;height=%d;", stAttr.width, stAttr.height);
		nSize += strlen(acItem);
		strcat(pData, acItem);
//		printf("------------------------------------chn:%d,framerate:%d\n",i,stAttr.framerate);
		sprintf(acItem, "framerate=%d;", stAttr.framerate);
		nSize += strlen(acItem);
		strcat(pData, acItem);

		int w,h,fr,br;
		_get_stream_def_param(i, &w, &h, &fr, &br, stAttr.vencType);
//		printf("----------------------------------chn:%d;w:%d;h:%d;fr:%d;br:%d\n",i,w,h,fr,br);
		sprintf(acItem, "def_width=%d;def_height=%d;def_framerate=%d;def_bitrate=%d;", w,h,fr,br);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//因为精度问题，所以再往回换算时，需要加一个除数
		ttt = ((1024 * 8) / 3600);
		if (((1024 * 8) % 3600) == 0)
			ttt--;
		//sprintf(acItem, "nMBPH=%d;", (stAttr.bitrate + ttt)*3600/(1024*8));
		sprintf(acItem, "nMBPH=%d;", stAttr.bitrate);  //gyd
		//Printf("framerate: %d, %d\n",stAttr.framerate,stAttr.framerate*3600/(1024*8));
		nSize += strlen(acItem);
		strcat(pData, acItem);

		if(hwinfo.bSupportLDC)		//支持畸变校正的设备会显示此参数
		{
			sprintf(acItem, "bSupportLDC=%d;", hwinfo.bSupportLDC);  //gyd
			nSize += strlen(acItem);
			strcat(pData, acItem);
			sprintf(acItem, "bLDCEnable=%d;", stAttr.bLDCEnable);  //gyd
			nSize += strlen(acItem);
			strcat(pData, acItem);
			sprintf(acItem, "ldcRatio=%d;", stAttr.nLDCRatio);	//gyd
			nSize += strlen(acItem);
			strcat(pData, acItem);
		}

		snprintf(acItem, sizeof(acItem), "vencType=%d;", stAttr.vencType);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}

	snprintf(acItem, sizeof(acItem), "MobileQuality=%d;", SctrlGetParam(NULL)->nPictureType);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	jvstream_ability_t ability;
	jv_stream_get_ability(0, &ability);
	if(ability.vencTypeNum == 0)
		ability.vencTypeNum = 1;
	snprintf(acItem, sizeof(acItem), "vencTypeNum=%d;", ability.vencTypeNum);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	int vencTypeDef = 0;	//0 264
#if (defined PLATFORM_hi3516D) || (defined PLATFORM_hi3516EV100)
	vencTypeDef = 1;	//1 	//1 265
#endif
	snprintf(acItem, sizeof(acItem), "vencTypeDef=%d;", vencTypeDef);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	Printf("build_stream_param:%s\n", pData);

	return nSize;
}

/**
 *@brief 把结构体转化为字符串
 *@param pData 保存转化后字符串的内存地址
 *@return 转化后字符串的长度
 *
 */
U32 build_mdetect_param(int channelid, char *pData)
{
	U32 i, nSize = 0;
	char acItem[256] =
	{ 0 };
	MD md1;
	MD *md = &md1;
	mdetect_get_param(channelid, md);

	pData[0] = '\0';
	//是否开启
	sprintf(acItem, "bMDEnable=%d;", md->bEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//灵敏度
	sprintf(acItem, "nMDSensitivity=%d;", md->nSensitivity);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//阈值
	sprintf(acItem, "nMDThreshold=%d;", md->nThreshold);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//比例
//	sprintf(acItem, "nMDRatio=%d;", md->nRatio);
//	nSize += strlen(acItem);
//	strcat(pData, acItem);
	//四个区域
	for (i = 0; i < MAX_MDRGN_NUM; i++)
	{
		RECT *pRect = &md->stRect[i];
		sprintf(acItem, "MDRegion%d=%d,%d,%d,%d;", i, pRect->x, pRect->y,
				pRect->w, pRect->h);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}

	//分控报警
	sprintf(acItem, "nMDOutClient=%d;", md->bOutClient);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//邮件报警
	sprintf(acItem, "nMDOutEMail=%d;", md->bOutEMail);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//蜂鸣器报警
	sprintf(acItem, "nMDOutBuzzing=%d;", md->bBuzzing);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	sprintf(acItem, "nMDOutVMS=%d;", md->bOutVMS);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	Printf("MD:%s\n", pData);
#ifdef MD_GRID_SET
	sprintf(acItem, "MovGridW=%d;", md->nColumn);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	Printf("MD:%s\n", pData);

	sprintf(acItem, "MovGridH=%d;", md->nRow);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	Printf("MD:%s\n", pData);


	int index = 0;
	sprintf(acItem, "MovGrid=%11d,", md->nRegion[index++]);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	for(;index<md->nRow;index++)
	{
		sprintf(acItem, "%11d,", md->nRegion[index]);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
	pData[nSize-1]=';';
#endif

	//追加智能算法的开启状态，用于客户端进行互斥提示
	MIVP_t mivp;
	mivp_get_param(0,&mivp);
	sprintf(acItem, "IPCIVPSupport=%d;", mivp_bsupport());
	nSize += strlen(acItem);
	strcat(pData, acItem);

	sprintf(acItem, "IVPEnable=%d;", mivp.st_rl_attr.bEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	sprintf(acItem, "bIVPCOUNTSupport=%d;", mivp_count_bsupport());
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bOpenCount=%d;", mivp.st_count_attr.bOpenCount);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideSupport=%d;", mivp_hide_bsupport());
	nSize += strlen(acItem);
	strcat(pData, acItem);	
	sprintf(acItem, "bIVPHideAlarmEn=%d;",mivp.st_hide_attr.bHideEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPClimbSupport=%d;",mivp_climb_bsupport());
	nSize += strlen(acItem);
	strcat(pData, acItem);
//	printf("build_mdetect_param: %s \n ",pData);
	return nSize;
}

/**
 *@brief 把字符串参数转化为结构体
 *@param pData INPUT 字符串的内存地址
 *@param md OUTPUT 移动检测的结构体
 *@return 无
 *
 */
static void _mdetect_string2param(char *pData, MD *md)
{
	char key[30];
	//char value[30];
	char value[385];
	while (*pData != '\0')
	{
		//pData = sysfunc_get_key_value(pData, key, value, 30);
		pData = sysfunc_get_key_value(pData, key, value, strlen(pData)+1);
		//移动检测
		if (strncmp(key, "bMDEnable", 9) == 0)
		{
			md->bEnable = atoi(value);
			//bMDModify = TRUE;
		}
		else if (strncmp(key, "nMDSensitivity", 12) == 0)
		{
			md->nSensitivity = atoi(value);
			//根据灵敏度计算阈值等
			md->nThreshold = (100 - md->nSensitivity) / 5 + 5;
			//md->nRatio 	= (100-md->nSensitivity)/5+5;
			//...
			//bMDModify = TRUE;
		}
		else if (strncmp(key, "MDRegion", 8) == 0)
		{
			RECT *pRect = NULL;
			U32 nIndex = 0;			//第几个区域
			sscanf(key, "MDRegion%d", &nIndex);
			pRect = &md->stRect[nIndex];
			sscanf(value, "%d,%d,%d,%d", &pRect->x, &pRect->y, &pRect->w,
					&pRect->h);
			Printf(
					"nIndex:%d, ----%d,%d,%d,%d\n", nIndex, pRect->x, pRect->y, pRect->w, pRect->h);
			//bMDModify = TRUE;
		}
		else if (strncmp(key, "nMDOutClient", 12) == 0)
		{
			md->bOutClient = atoi(value);
		}
		else if (strncmp(key, "nMDOutEMail", 11) == 0)
		{
			md->bOutEMail = atoi(value);
		}
		else if (strncmp(key, "nMDOutBuzzing", 13) == 0)
		{
			md->bBuzzing = atoi(value);
		}
		else if (strncmp(key, "nMDOutVMS", 9) == 0)
		{
			md->bOutVMS= atoi(value);
		}
#ifdef MD_GRID_SET
		else if (strncmp(key, "MovGridW", 8) == 0)
		{
			md->nColumn = atoi(value);
		}
		else if (strncmp(key, "MovGridH", 8) == 0)
		{
			md->nRow = atoi(value);
		}
		else if (strcmp(key, "MovGrid") == 0)
		{
			int index = 0;
			//char strRow[512];
			//sscanf(value, "%d,%s", &md->nRegion[index++],strRow);
			//for(;index<md->nRow-1;index++)
			//{
			//	sscanf(strRow, "%d,%s", &md->nRegion[index],strRow);
			//}
			//sscanf(strRow, "%d", &md->nRegion[index]);
			for (; index < md->nRow; index++)
			{
				sscanf(value + 12 * index, "%11d,", &md->nRegion[index]);
			}
		}
#endif
	}
}

/**
 *@brief 把字符串参数转化为结构体
 *@param pData INPUT 字符串的内存地址
 *@param md OUTPUT 移动检测的结构体
 *@return 无
 *
 */
static void _malarm_string2param(char *pData, ALARMSET *alarm)
{
	char key[30];
	char value[30];
	char *tmp = NULL;
	while (*pData != '\0')
	{
		pData = sysfunc_get_key_value(pData, key, value, 30);
		//移动检测
		if (strncmp(key, "nAlarmDelay", 11) == 0)
		{
			alarm->delay = atoi(value);
			//bMDModify = TRUE;
		}
		else if (strncmp(key, "acMailSender", 12) == 0)
		{
			strcpy(alarm->sender, value);
			Printf("-------sender:%s\n", alarm->sender);
		}
		else if (strncmp(key, "acSMTPServer", 12) == 0)
		{
			strcpy(alarm->server, value);
		}
		else if (strncmp(key, "acSMTPUser", 10) == 0)
		{
			strcpy(alarm->username, value);
			Printf("-------username:%s\n", alarm->username);
		}
		else if (strncmp(key, "acSMTPPasswd", 12) == 0)
		{
			strcpy(alarm->passwd, value);
			Printf("-------passwd:%s\n", alarm->passwd);
		}
		else if (strncmp(key, "acSMTPPort", 10) == 0)
		{
			alarm->port = atoi(value);
		}
		else if (strncmp(key, "acSMTPCrypto", 12) == 0)
		{
			strcpy(alarm->crypto, value);
		}
		else if (strncmp(key, "acReceiver0", 11) == 0)
		{
			strcpy(alarm->receiver0, value);
		}
		else if (strncmp(key, "acReceiver1", 11) == 0)
		{
			strcpy(alarm->receiver1, value);
		}
		else if (strncmp(key, "acReceiver2", 11) == 0)
		{
			strcpy(alarm->receiver2, value);
		}
		else if (strncmp(key, "acReceiver3", 11) == 0)
		{
			strcpy(alarm->receiver3, value);
		}
		else if (strncmp(key, "bAlarmEnable", 12) == 0)
		{
			alarm->bEnable = atoi(value);
		}
		else if(strncmp(key,"bAlarmSound",11) == 0)
		{
			alarm->bAlarmSoundEnable = atoi(value);
			if(!atoi(value))
				malarm_sound_stop();	
		}
		else if(strncmp(key,"bAlarmLight",11) == 0)
		{
			alarm->bAlarmLightEnable = atoi(value);
			if(!atoi(value))
				malarm_light_stop();
		}
		else if(strncmp(key,"alarmLightStatus",11) == 0)
		{
			/* 只有关,没有开 */
			if(!atoi(value))
				malarm_light_stop();
		}
		else if (strncmp(key, "vmsServerPort", 13) == 0)
		{
			alarm->vmsServerPort = atoi(value);
		}
		else if (strncmp(key, "vmsServerIp", 11) == 0)
		{
			strcpy(alarm->vmsServerIp, value);
		}
		else if (strncmp(key, "alarmTime0", 10) == 0)
		{
			tmp = strstr(value, "-");
			if(tmp)
			{
				*tmp = '\0';
				strcpy(alarm->alarmTime[0].tStart, value);
				strcpy(alarm->alarmTime[0].tEnd, tmp+1);
			}
		}
		else if (strncmp(key, "alarmTime1", 10) == 0)
		{
			tmp = strstr(value, "-");
			if(tmp)
			{
				*tmp = '\0';
				strcpy(alarm->alarmTime[1].tStart, value);
				strcpy(alarm->alarmTime[1].tEnd, tmp+1);
			}
		}
		else if (strncmp(key, "alarmTime2", 10) == 0)
		{
			tmp = strstr(value, "-");
			if(tmp)
			{
				*tmp = '\0';
				strcpy(alarm->alarmTime[2].tStart, value);
				strcpy(alarm->alarmTime[2].tEnd, tmp+1);
			}
		}
	    else if (strncmp(key, "Schedule_bEn1", 13) == 0)
		{
			alarm->m_Schedule[0].bEnable = atoi(value);
		}
		else if (strncmp(key, "Schedule_time1", 14) == 0)
		{
			tmp = strstr(value, ":");
			if(tmp)
			{
				*tmp = '\0';
				alarm->m_Schedule[0].Schedule_time_H= atoi(value);
				alarm->m_Schedule[0].Schedule_time_M=atoi(tmp+1);
			}
		}
		else if (strncmp(key, "Schedule_num1", 13) == 0)
		{
			alarm->m_Schedule[0].num = atoi(value);
		}
	    else if (strncmp(key, "Schedule_interval1", 18) == 0)
		{
			alarm->m_Schedule[0].Interval = atoi(value);
		}
		else if (strncmp(key, "Schedule_bEn2", 13) == 0)
		{
			alarm->m_Schedule[1].bEnable = atoi(value);
		}
		else if (strncmp(key, "Schedule_time2", 14) == 0)
		{
			tmp = strstr(value, ":");
			if(tmp)
			{
				*tmp = '\0';
				alarm->m_Schedule[1].Schedule_time_H= atoi(value);
				alarm->m_Schedule[1].Schedule_time_M=atoi(tmp+1);
			}
		}
		else if (strncmp(key, "Schedule_num2", 13) == 0)
		{
			alarm->m_Schedule[1].num = atoi(value);
		}
	    else if (strncmp(key, "Schedule_interval2", 18) == 0)
		{
			alarm->m_Schedule[1].Interval = atoi(value);
		}
		else if (strncmp(key, "Schedule_bEn3", 13) == 0)
		{
			alarm->m_Schedule[2].bEnable = atoi(value);
		}
		else if (strncmp(key, "Schedule_time3", 14) == 0)
		{
			tmp = strstr(value, ":");
			if(tmp)
			{
				*tmp = '\0';
				alarm->m_Schedule[2].Schedule_time_H= atoi(value);
				alarm->m_Schedule[2].Schedule_time_M=atoi(tmp+1);
			}
		}
		else if (strncmp(key, "Schedule_num3", 13) == 0)
		{
			alarm->m_Schedule[2].num = atoi(value);
		}
	    else if (strncmp(key, "Schedule_interval3", 18) == 0)
		{
			alarm->m_Schedule[2].Interval = atoi(value);
		}
		else if (strncmp(key, "Schedule_bEn4", 13) == 0)
		{
			alarm->m_Schedule[3].bEnable = atoi(value);
		}
		else if (strncmp(key, "Schedule_time4", 14) == 0)
		{
			tmp = strstr(value, ":");
			if(tmp)
			{
				*tmp = '\0';
				alarm->m_Schedule[3].Schedule_time_H= atoi(value);
				alarm->m_Schedule[3].Schedule_time_M=atoi(tmp+1);
			}
		}
		else if (strncmp(key, "Schedule_num4", 13) == 0)
		{
			alarm->m_Schedule[3].num = atoi(value);
		}
	    else if (strncmp(key, "Schedule_interval4", 18) == 0)
		{
			alarm->m_Schedule[3].Interval = atoi(value);
		}
		else if (strncmp(key, "Schedule_bEn5", 13) == 0)
		{
			alarm->m_Schedule[4].bEnable = atoi(value);
		}
		else if (strncmp(key, "Schedule_time5", 14) == 0)
		{
			tmp = strstr(value, ":");
			if(tmp)
			{
				*tmp = '\0';
				alarm->m_Schedule[4].Schedule_time_H= atoi(value);
				alarm->m_Schedule[4].Schedule_time_M=atoi(tmp+1);
			}
		}
		else if (strncmp(key, "Schedule_num5", 13) == 0)
		{
			alarm->m_Schedule[4].num = atoi(value);
		}
	    else if (strncmp(key, "Schedule_interval5", 18) == 0)
		{
			alarm->m_Schedule[4].Interval = atoi(value);
		}
		
	}
}

//远程设置处理
VOID MailProc(REMOTECFG *cfg)
{
	EXTEND *pstExt = (EXTEND*) (cfg->stPacket.acData);
	ALARMSET alarm, alarm_bak;
	int time_tmp;
	int time_tmpMax;
	int i;
	time_t tt;
	struct tm tm;
	int now;
	int cnt;
	
	printf("acData=%s\n", pstExt->acData);
	malarm_get_param(&alarm);
	malarm_get_param(&alarm_bak);
	_malarm_string2param((char *)pstExt->acData, &alarm);
	malarm_set_param(&alarm);
	WriteConfigInfo();

	switch (pstExt->nType)
	{
	case EX_MAIL_TEST:
		Printf("EX_MAIL_TEST acData=%s\n", pstExt->acData);
		pstExt->nType = EX_MAIL_TEST;
		pstExt->nParam1 = mail_test((char *)pstExt->acData);
		Printf(
				"---------EX_MAIL_TEST acData=%s, length:%d, ch:%d, nClientID:%d\n", pstExt->acData, pstExt->nParam1, cfg->nCh, cfg->nClientID);
		Printf(
				"type=%d, subtype:%d\n", cfg->stPacket.nPacketType, cfg->stPacket.nPacketCount);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstExt->nParam1);
		break;
	case EX_ALARM_SUBMIT:
		Printf("EX_ALARM_SUBMIT acData=%s\n", pstExt->acData);
		pstExt->nType = EX_ALARM_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	case EX_SCHEDULE_SUBMIT:
		tt = time(NULL);
		localtime_r(&tt, &tm);
		now = tm.tm_hour*60 + tm.tm_min;
		for(i=0;i<5;i++)
		{
			if(memcmp(&alarm.m_Schedule[i],&alarm_bak.m_Schedule[i],sizeof(alarm_bak.m_Schedule[i])))
			{
			    if(alarm.m_Schedule[i].bEnable==1)
			    {
				  	
					time_tmp=alarm.m_Schedule[i].Schedule_time_H*60+alarm.m_Schedule[i].Schedule_time_M;	
					time_tmpMax=alarm.m_Schedule[i].Schedule_time_H*60+alarm.m_Schedule[i].Schedule_time_M+(alarm.m_Schedule[i].num-1)*alarm.m_Schedule[i].Interval;
					if(now>time_tmp && now <= time_tmpMax)
					{
							for(cnt = 1;cnt < alarm.m_Schedule[i].num;cnt++)
							{
								time_tmp=alarm.m_Schedule[i].Schedule_time_H*60+alarm.m_Schedule[i].Schedule_time_M+(cnt)*alarm.m_Schedule[i].Interval;
								if(now <= time_tmp)
									break;
							}
						
					}
					utl_schedule_Enable(schedule_id[i],time_tmp);			
			    }
				else
				{
					utl_schedule_disable(schedule_id[i]);
				}
			}
		}
		
		break;
	case EX_ALARM_REFRESH:
		Printf("EX_ALARM_REFRESH\n");
		pstExt->nParam1 = 0;
		memset(pstExt->acData, 0, RC_DATA_SIZE - 16);
		pstExt->nParam2 = build_alarm_param((char *)pstExt->acData);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
				20 + pstExt->nParam2);		
		break;
	default:
		break;
	}
}


//远程设置处理
VOID MDProc(REMOTECFG *cfg)
{
	MD md;
	EXTEND *pstMDExt = (EXTEND*) (cfg->stPacket.acData);

	stSnapSize size;
	size.nWith = 400;
	size.nHeight = 224;

	switch (pstMDExt->nType)
	{
	case EX_MD_UPDATE:
		Printf("EX_MD_UPDATE\n");
		pstMDExt->nType = EX_MD_UPDATE;
		pstMDExt->nParam1 = msnapshot_get_data(0, pstMDExt->acData,RC_DATA_SIZE-16,&size);
		pstMDExt->nParam2 = build_mdetect_param(0,
				(char *)pstMDExt->acData + pstMDExt->nParam1);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket,
				20 + pstMDExt->nParam1 + pstMDExt->nParam2);
		break;
	case EX_MD_REFRESH:
		Printf("EX_MD_REFRESH\n");
		pstMDExt->nType = EX_MD_REFRESH;
		pstMDExt->nParam1 = msnapshot_get_data(0, pstMDExt->acData,RC_DATA_SIZE-16,&size);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstMDExt->nParam1);
		break;
	case EX_MD_SUBMIT:
		Printf("EX_MD_SUBMIT acData=%s\n", pstMDExt->acData);
		mdetect_get_param(0, &md);
		_mdetect_string2param((char *)pstMDExt->acData, &md);
		mdetect_set_param(0, &md);
		WriteConfigInfo();
		mdetect_flush(0);
		pstMDExt->nType = EX_MD_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	default:
		break;
	}
}

//添加区域遮挡参数
U32 build_privacy_param(char *pData)
{
	U32 i, nSize = 0;
	char acItem[256] =
	{ 0 };
	REGION region;

	pData[0] = '\0';
	mprivacy_get_param(0, &region);
	//是否开启
	Printf("pData:%s\n", pData);
	sprintf(acItem, "bCoverRgn=%d;", region.bEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	Printf("pData:%s\n", pData);
	//四个区域
	for (i = 0; i < MAX_PYRGN_NUM; i++)
	{
		RECT *pRect = &region.stRect[i];
		sprintf(acItem, "Region%d=%d,%d,%d,%d;", i, pRect->x, pRect->y,
				pRect->w, pRect->h);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}

	Printf("Privacy:%s\n", pData);

	return nSize;
}

//远程设置处理
VOID PrivacyProc(REMOTECFG *cfg)			//U8 *pData
{
	EXTEND *pstPExt = (EXTEND*) (cfg->stPacket.acData);

	stSnapSize size;
	size.nWith = 400;
	size.nHeight = 224;

	//memset(pstPExt, 0, sizeof(EXTEND));
	switch (pstPExt->nType)
	{
	case EX_COVERRGN_UPDATE:
		pstPExt->nParam1 = msnapshot_get_data(0, pstPExt->acData,RC_DATA_SIZE-16,&size);
		pstPExt->nParam2 = build_privacy_param(
				(char *)pstPExt->acData + pstPExt->nParam1);
		//Printf("xian ch:%d:  client:%d\n", cfg->nCh, cfg->nClientID);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstPExt->nParam1 + pstPExt->nParam2);
		break;
	case EX_COVERRGN_REFRESH:
		pstPExt->nParam1 = msnapshot_get_data(0, pstPExt->acData,RC_DATA_SIZE-16,&size);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstPExt->nParam1);
		break;
	case EX_COVERRGN_SUBMIT:
		Printf("acData=%s\n", pstPExt->acData);
		SetDVRParam(cfg, (char *)pstPExt->acData);
		Printf("xian SetDVRParam over\n");
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	default:
		break;
	}
}

//添加sensor参数
U32 build_sensor_param(char *pData)
{
	U32 nSize = 0;
	char acItem[256] =
	{ 0 };

	msensor_attr_t stSensorAttr;

	msensor_getparam(&stSensorAttr);

	sprintf(acItem, "ir_cut=%d;", stSensorAttr.ir_cut);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "brightness=%d;", stSensorAttr.brightness);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "saturation=%d;", stSensorAttr.saturation);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "exposure=%d;", stSensorAttr.exposure);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "contrast=%d;", stSensorAttr.contrast);
	nSize += strlen(acItem);
	sprintf(acItem, "sharpness=%d;", stSensorAttr.sharpness);
	nSize += strlen(acItem);
	sprintf(acItem, "antifog=%d;", stSensorAttr.antifog);
	nSize += strlen(acItem);
	sprintf(acItem, "bSupportWdr=%d;", stSensorAttr.bSupportWdr);
	nSize += strlen(acItem);
	sprintf(acItem, "bEnableWdr=%d;", stSensorAttr.bEnableWdr);
	nSize += strlen(acItem);
	sprintf(acItem, "bSupportSl=%d;", stSensorAttr.bSupportSl);
	nSize += strlen(acItem);
	sprintf(acItem, "bEnableSl=%d;", stSensorAttr.bEnableSl);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "effect_flag=%d;", stSensorAttr.effect_flag);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "sw_cut=%d;", stSensorAttr.sw_cut);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "cut_rate=%d;", stSensorAttr.cut_rate);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "exp_mode=%d;", stSensorAttr.exp_mode);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "rotate=%d;", stSensorAttr.rotate);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "daynightMode=%d;", stSensorAttr.daynightMode);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bRedWhiteCtrlEnabled=%d;", msensor_get_whitelight_function());
	nSize += strlen(acItem);
	strcat(pData, acItem);

	// 支持硬光敏
	//if ((IRCUT_SW_BY_ADC0 == hwinfo.ir_sw_mode)
	//	|| (IRCUT_SW_BY_ADC1 == hwinfo.ir_sw_mode)
	//	|| (IRCUT_SW_BY_ADC2 == hwinfo.ir_sw_mode))
	{
		sprintf(acItem, "adcHValue=%d;", jv_sensor_get_adc_low_val());
		nSize += strlen(acItem);
		strcat(pData, acItem);
		sprintf(acItem, "adcLValue=%d;", jv_sensor_get_adc_high_val());
		nSize += strlen(acItem);
		strcat(pData, acItem);
		sprintf(acItem, "adcRValue=%d;", jv_sensor_get_adc_val());
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}

	Printf("pData:%s\n", pData);

	return nSize;
}

//远程设置处理
VOID SensorProc(REMOTECFG *cfg)
{
	Printf("SensorProc...\n");
	EXTEND *pstPExt = (EXTEND*) (cfg->stPacket.acData);
	msensor_attr_t stAttr;
	msensor_getparam(&stAttr);
	int len=strlen((char *)pstPExt->acData);//防止最后没有;而挂掉
	if(len>0 && pstPExt->acData[len-1]!=';')
		pstPExt->acData[strlen((char *)pstPExt->acData)]=';';
	printf("%s\n", pstPExt->acData);
	switch (pstPExt->nType)
	{
	case EX_SENSOR_SAVE:
	case EX_SENSOR_SUBMIT:	//处理完提交的数据再执行EX_SENSOR_REFRESH分支的代码
		stAttr.ae.exposureMin = GetIntValueWithDefault((char *)pstPExt->acData, "exposureMin", stAttr.ae.exposureMin);
		stAttr.ae.exposureMax = GetIntValueWithDefault((char *)pstPExt->acData, "exposureMax", stAttr.ae.exposureMax);
		stAttr.ae.bAEME= GetIntValueWithDefault((char *)pstPExt->acData, "bAEME", stAttr.ae.bAEME);
		stAttr.ae.bByPassAE= GetIntValueWithDefault((char *)pstPExt->acData, "bByPassAE", stAttr.ae.bByPassAE);
		stAttr.ae.u16AGainMax= GetIntValueWithDefault((char *)pstPExt->acData, "u16AGainMax", stAttr.ae.u16AGainMax);
		stAttr.ae.u16AGainMin= GetIntValueWithDefault((char *)pstPExt->acData, "u16AGainMin", stAttr.ae.u16AGainMin);
		stAttr.ae.u16DGainMax= GetIntValueWithDefault((char *)pstPExt->acData, "u16DGainMax", stAttr.ae.u16DGainMax);
		stAttr.ae.u16DGainMin= GetIntValueWithDefault((char *)pstPExt->acData, "u16DGainMin", stAttr.ae.u16DGainMin);
		stAttr.ae.u32ISPDGainMax= GetIntValueWithDefault((char *)pstPExt->acData, "u32ISPDGainMax", stAttr.ae.u32ISPDGainMax);
		stAttr.ae.u32SystemGainMax= GetIntValueWithDefault((char *)pstPExt->acData, "u32SystemGainMax", stAttr.ae.u32SystemGainMax);
		stAttr.ae.u32ExpLine= GetIntValueWithDefault((char *)pstPExt->acData, "u32ExpLine", stAttr.ae.u32ExpLine);
		stAttr.ae.u32ISPDGain= GetIntValueWithDefault((char *)pstPExt->acData, "u32ISPDGain", stAttr.ae.u32ISPDGain);
		stAttr.ae.s32AGain= GetIntValueWithDefault((char *)pstPExt->acData, "s32AGain", stAttr.ae.s32AGain);
		stAttr.ae.s32DGain= GetIntValueWithDefault((char *)pstPExt->acData, "s32DGain", stAttr.ae.s32DGain);
		stAttr.ae.bManualAGainEnable= GetIntValueWithDefault((char *)pstPExt->acData, "bManualAGainEnable", stAttr.ae.bManualAGainEnable);
		stAttr.ae.bManualDGainEnable= GetIntValueWithDefault((char *)pstPExt->acData, "bManualDGainEnable", stAttr.ae.bManualDGainEnable);
		stAttr.ae.bManualExpLineEnable= GetIntValueWithDefault((char *)pstPExt->acData, "bManualExpLineEnable", stAttr.ae.bManualExpLineEnable);
		stAttr.ae.bManualISPGainEnable= GetIntValueWithDefault((char *)pstPExt->acData, "bManualISPGainEnable", stAttr.ae.bManualISPGainEnable);
		stAttr.drc.bDRCEnable= GetIntValueWithDefault((char *)pstPExt->acData, "bDRCEnable", stAttr.drc.bDRCEnable);
		char temp[64];
		if (GetKeyValue((char *)pstPExt->acData, "dayStart", temp, 32))
		{
			int hour, minute;
			sscanf(temp, "%d:%d", &hour, &minute);
			stAttr.dayStart.hour = hour;
			stAttr.dayStart.minute = minute;
		}
		if (GetKeyValue((char *)pstPExt->acData, "dayEnd", temp, 32))
		{
			int hour, minute;
			sscanf(temp, "%d:%d", &hour, &minute);
			stAttr.dayEnd.hour = hour;
			stAttr.dayEnd.minute = minute;
		}
		if (GetKeyValue((char *)pstPExt->acData, "bRedWhiteCtrlEnabled", temp, 32))
		{
			msensor_set_whitelight_function(atoi(temp));
			WriteConfigInfo();
		}

		stAttr.brightness = GetIntValueWithDefault((char *)pstPExt->acData, "brightness", stAttr.brightness);
		stAttr.saturation = GetIntValueWithDefault((char *)pstPExt->acData, "saturation", stAttr.saturation);
		stAttr.contrast = GetIntValueWithDefault((char *)pstPExt->acData, "contrast", stAttr.contrast);
		stAttr.sharpness = GetIntValueWithDefault((char *)pstPExt->acData, "sharpness", stAttr.sharpness);
		stAttr.exposure = GetIntValueWithDefault((char *)pstPExt->acData, "exposure", stAttr.exposure);
		stAttr.antifog = GetIntValueWithDefault((char *)pstPExt->acData, "antifog", stAttr.antifog);
		stAttr.light= GetIntValueWithDefault((char *)pstPExt->acData, "light", stAttr.light);
		stAttr.bEnableWdr = GetIntValueWithDefault((char *)pstPExt->acData, "bEnableWdr", stAttr.bEnableWdr);
		stAttr.bEnableSl = GetIntValueWithDefault((char *)pstPExt->acData, "bEnableSl", stAttr.bEnableSl);
		stAttr.effect_flag = GetIntValueWithDefault((char *)pstPExt->acData, "effect_flag", stAttr.effect_flag);
		stAttr.sw_cut = GetIntValueWithDefault((char *)pstPExt->acData, "sw_cut", stAttr.sw_cut);
		stAttr.cut_rate = GetIntValueWithDefault((char *)pstPExt->acData, "cut_rate", stAttr.cut_rate);
		stAttr.sence = GetIntValueWithDefault((char *)pstPExt->acData, "sence", stAttr.sence);
		stAttr.daynightMode = GetIntValueWithDefault((char *)pstPExt->acData, "daynightMode", stAttr.daynightMode);
		stAttr.bDISOpen = GetIntValueWithDefault((char *)pstPExt->acData, "bDISOpen", stAttr.bDISOpen);
		stAttr.rotate = GetIntValueWithDefault((char *)pstPExt->acData, "rotate", stAttr.rotate);
		stAttr.AutoLowFrameEn =GetIntValueWithDefault((char *)pstPExt->acData, "AutoLowFrameEn", stAttr.AutoLowFrameEn);
		stAttr.cutDelay=GetIntValueWithDefault((char *)pstPExt->acData, "cutDelay", stAttr.cutDelay);
		stAttr.exp_mode = GetIntValueWithDefault((char *)pstPExt->acData, "exp_mode", stAttr.exp_mode);
		stAttr.stCutTest.bOpenCutTest = GetIntValueWithDefault((char *)pstPExt->acData, "bOpenCutTest",0);
		stAttr.stCutTest.nCountTimes = GetIntValueWithDefault((char *)pstPExt->acData, "nCountTimes",0);
		stAttr.stCutTest.nInterval = GetIntValueWithDefault((char *)pstPExt->acData, "nCutInterval",0);
		msensor_setparam(&stAttr);
		msensor_flush(0);

		if (EX_SENSOR_SAVE == pstPExt->nType)
			WriteConfigInfo();
		break;
	case EX_SENSOR_REFRESH:
		;stSnapSize size;
		size.nWith = 400;
		size.nHeight = 224;
		pstPExt->nParam1 = msnapshot_get_data(0, pstPExt->acData,RC_DATA_SIZE-16,&size);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstPExt->nParam1);
		break;
	case EX_SENSOR_GETPARAM:
		pstPExt->nParam1 = 0;
		memset(pstPExt->acData, 0, RC_DATA_SIZE - 16);
		pstPExt->nParam2 = build_sensor_param((char*)pstPExt->acData);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
				20 + pstPExt->nParam2);
		break;
	default:
		break;
	}
}

//远程设置处理
VOID PTZProc(REMOTECFG *cfg)
{
	Printf("PTZProc...\n");
	EXTEND *pstPExt = (EXTEND*) (cfg->stPacket.acData);
	PTZ_PATROL_INFO *info;
	PTZ_GUARD_T guard;
	PTZ_GUARD_T *guardinfo;
	PTZ *mpPtz;
	PTZ_SCHEDULE_INFO *schedule;
	ZONE_INFO zone;
	mstream_attr_t resol;
	Printf("%s\n", pstPExt->acData);

	switch (pstPExt->nType)
	{
	//预置点
	case EX_PTZ_PRESET_ADD:
		PTZ_AddPreset(pstPExt->nParam1, pstPExt->nParam2, (char *)pstPExt->acData);
		mlog_write("Add Preset: %d with name: %s", pstPExt->nParam2,
				pstPExt->acData);
		break;
	case EX_PTZ_PRESET_DEL:
		PTZ_DelPreset(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Delete Preset: %d", pstPExt->nParam2);
		break;
	case EX_PTZ_PRESET_CALL:
		PtzLocatePreset(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Call Preset: %d", pstPExt->nParam2);
		break;
	case EX_PTZ_PRESET_SET:
		PtzSetPreset(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Set Preset: %d", pstPExt->nParam2);
		break;
	case EX_PTZ_PRESET_CLEAR:
		break;
	//定时任务
	case EX_PTZ_SCHEDULE_SAVE:
		schedule = PTZ_GetSchedule();
		int schEn1 = GetIntValue((char *)pstPExt->acData, "bSch1En");
		int schEn2 = GetIntValue((char *)pstPExt->acData, "bSch2En");
		if(schedule->bSchEn[0] != schEn1)
		{
			schedule->bSchEn[0] = schEn1;
			if(schedule->bSchEn[0] == TRUE)
				mlog_write("Start Schedule-%d", 1);
			else
				mlog_write("Stop Schedule-%d", 1);
		}
		if(schedule->bSchEn[1] != schEn2)
		{
			schedule->bSchEn[1] = schEn2;
			if(schedule->bSchEn[1] == TRUE)
				mlog_write("Start Schedule-%d", 2);
			else
				mlog_write("Stop Schedule-%d", 2);
		}
		schedule->schedule[0] = GetIntValue((char *)pstPExt->acData, "schedule1");
		schedule->schedule[1] = GetIntValue((char *)pstPExt->acData, "schedule2");
		schedule->schTimeStart[0].hour = GetIntValue((char *)pstPExt->acData, "sch1StartHour");
		schedule->schTimeStart[0].minute = GetIntValue((char *)pstPExt->acData, "sch1StartMinute");
		schedule->schTimeEnd[0].hour = GetIntValue((char *)pstPExt->acData, "sch1EndHour");
		schedule->schTimeEnd[0].minute = GetIntValue((char *)pstPExt->acData, "sch1EndMinute");
		schedule->schTimeStart[1].hour = GetIntValue((char *)pstPExt->acData, "sch2StartHour");
		schedule->schTimeStart[1].minute = GetIntValue((char *)pstPExt->acData, "sch2StartMinute");
		schedule->schTimeEnd[1].hour = GetIntValue((char *)pstPExt->acData, "sch2EndHour");
		schedule->schTimeEnd[1].minute = GetIntValue((char *)pstPExt->acData, "sch2EndMinute");
//		printf("schedule1:%d-%d  schedule2:%d-%d\n",schedule->schedule[0], schedule->bSchEn[0], schedule->schedule[1], schedule->bSchEn[1]);
//		printf("%d:%d  %d:%d  \n",schedule->schTimeStart[0].hour,schedule->schTimeStart[0].minute,
//			schedule->schTimeEnd[0].hour,schedule->schTimeEnd[0].minute);
//		printf("%d:%d  %d:%d  \n",schedule->schTimeStart[1].hour,schedule->schTimeStart[1].minute,
//			schedule->schTimeEnd[1].hour,schedule->schTimeEnd[1].minute);
		break;
	//巡航
	case EX_PTZ_PATROL_ADD:
		info = PTZ_GetPatrol();
		AddPatrolNod(&info[pstPExt->nParam1], pstPExt->nParam2, pstPExt->nParam3);
		mlog_write("Add Patrol-%d: %d Stay: %d Second", (pstPExt->nParam1+1), pstPExt->nParam2,
				pstPExt->nParam3);
		break;
	case EX_PTZ_PATROL_DEL:
		info = PTZ_GetPatrol();
		if (pstPExt->nParam2 >= info[pstPExt->nParam1].nPatrolSize)
		{
			printf("Failed find preset: %d\n", pstPExt->nParam2);
			break;
		}
		mlog_write("Delete Patrol-%d: %d", (pstPExt->nParam1+1), info[pstPExt->nParam1].aPatrol[pstPExt->nParam2].nPreset);
		DelPatrolNod(&info[pstPExt->nParam1], pstPExt->nParam2);
		break;
	case EX_PTZ_PATROL_START:
		PTZ_StartPatrol(pstPExt->nParam1, pstPExt->nParam3);
		mlog_write("Start Patrol-%d", (pstPExt->nParam3+1));
		break;
	case EX_PTZ_PATROL_STOP:
		PTZ_StopPatrol(pstPExt->nParam1);
		mlog_write("Stop Patrol");
		break;
	case EX_PTZ_PATROL_SETSPEED:
		info = PTZ_GetPatrol();
		info[pstPExt->nParam3].nPatrolSpeed = pstPExt->nParam2;
		break;
		//轨迹
	case EX_PTZ_TRAIL_REC_START:
		PtzTrailStartRec(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Start Record Trail-%d", (pstPExt->nParam2+1));
		break;
	case EX_PTZ_TRAIL_REC_STOP:
		PtzTrailStopRec(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Stop Record Trail-%d", (pstPExt->nParam2+1));
		break;
	case EX_PTZ_TRAIL_START:
		PtzTrailStart(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Start Trail-%d", (pstPExt->nParam2+1));
		break;
	case EX_PTZ_TRAIL_STOP:
		PtzTrailStop(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Stop Trail-%d", (pstPExt->nParam2+1));
		break;

		//守望
	case EX_PTZ_GUARD_SAVE:
		guardinfo = PTZ_GetGuard();
		guardinfo->guardTime = pstPExt->nParam3;
		guardinfo->nRreset = pstPExt->nParam2;
		//printf("save: time=%d, preset=%d\n", guardinfo->guardTime, guardinfo->nRreset);
		break;
	case EX_PTZ_GUARD_START:
		guardinfo = PTZ_GetGuard();
		guard.guardTime = guardinfo->guardTime;
		guard.nRreset = guardinfo->nRreset;
		guard.guardType = GUARD_PRESET;
		//printf("start: time=%d, preset=%d, type:%d\n", guard.guardTime, guard.nRreset, guard.guardType);
		PtzGuardSet(pstPExt->nParam1, &guard);
		mlog_write("Start Guard: Goto Preset: %d after %d Seconds",
				guard.nRreset, guard.guardTime);
		break;
	case EX_PTZ_GUARD_STOP:
		guard.guardTime = 0;
		guard.guardType = GUARD_NO;
		//printf("stop: time=%d, type:%d\n", guard.guardTime, guard.guardType);
		PtzGuardSet(pstPExt->nParam1, &guard);
		mlog_write("Stop Guard");
		break;

		//扫描
	case EX_PTZ_SCAN_LEFT:
		PtzLimitScanLeft(pstPExt->nParam1);
		mlog_write("Record Left Limitation of Scan");
		break;
	case EX_PTZ_SCAN_RIGHT:
		PtzLimitScanRight(pstPExt->nParam1);
		mlog_write("Record Right Limitation of Scan");
		break;
	case EX_PTZ_SCAN_START:
		PtzLimitScanSpeed(pstPExt->nParam1, 0, pstPExt->nParam2);
		PtzLimitScanStart(pstPExt->nParam1, 0);
		mlog_write("Start Limited Scan");
		break;
	case EX_PTZ_SCAN_UP:
		PtzLimitScanUp(pstPExt->nParam1);
		mlog_write("Record Up Limitation of Scan");
		break;
	case EX_PTZ_SCAN_DOWN:
		PtzLimitScanDown(pstPExt->nParam1);
		mlog_write("Record Down Limitation of Scan");
		break;
	case EX_PTZ_VERT_SCAN_START:
		PtzLimitScanSpeed(pstPExt->nParam1, 0, pstPExt->nParam2);
		PtzVertScanStart(pstPExt->nParam1, 0);
		mlog_write("Start Vertical Scan");
		break;
	case EX_PTZ_RANDOM_SCAN_START:
		PtzLimitScanSpeed(pstPExt->nParam1, 0, pstPExt->nParam2);
		PtzRandomScanStart(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Start Random Scan");
		break;
	case EX_PTZ_FRAME_SCAN_START:
		PtzLimitScanSpeed(pstPExt->nParam1, 0, pstPExt->nParam2);
		PtzFrameScanStart(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Start Frame Scan");
		break;
	case EX_PTZ_SCAN_STOP:
		PtzLimitScanStop(pstPExt->nParam1, 0);
		mlog_write("Stop Scan");
		break;
	case EX_PTZ_WAVE_SCAN_START:
		PtzLimitScanSpeed(pstPExt->nParam1, 0, pstPExt->nParam2);
		PtzWaveScanStart(pstPExt->nParam1, pstPExt->nParam2);
		mlog_write("Start Wave Scan");
		break;
	case EX_PTZ_COM_SETUP:
		mpPtz = PTZ_GetInfo();
		mpPtz->nProtocol = GetIntValue((char *)pstPExt->acData, "nprotocol");
		mpPtz->nAddr = GetIntValue((char *)pstPExt->acData, "nPTZAddr");
		mpPtz->nBaudRate = GetIntValue((char *)pstPExt->acData, "nBaud");
		mpPtz->nHwParams.nBaudRate = mpPtz->nBaudRate;
		mpPtz->nHwParams.nCharSize = GetIntValue((char *)pstPExt->acData, "nCharSize");
		mpPtz->nHwParams.nStopBit = GetIntValue((char *)pstPExt->acData, "nStopBit");
		mpPtz->nHwParams.nParityBit = GetIntValue((char *)pstPExt->acData,
				"nParityBit");
		mpPtz->nHwParams.nFlowCtl = GetIntValue((char *)pstPExt->acData, "nFlowCtl");
		PtzReSetup(mpPtz->nHwParams);
		break;
	case EX_PTZ232_COM_SETUP:
		break;
	case EX_AF_CALIBRATION:
		break;
	case EX_PTZ_PRESETS_PATROL_START:
		PTZ_PresetsPatrolStart();
		mlog_write("Add presets and start patrol");
		break;
	case EX_PTZ_PRESETS_PATROL_STOP:
		PTZ_PresetsPatrolStop();
		mlog_write("Stop patrol and remove presets");
		break;
	case EX_PTZ_BOOT_ITEM:
		PTZ_SetBootConfigItem(pstPExt->nParam1,pstPExt->nParam2);
		mlog_write("Boot config item: %d", pstPExt->nParam2);
		break;
	case EX_PTZ_ZOOM_ZONE:
		zone.x = GetIntValue((char *)pstPExt->acData, "m_x");
		zone.y = GetIntValue((char *)pstPExt->acData, "m_y");
		zone.w = GetIntValue((char *)pstPExt->acData, "m_w");
		zone.h = GetIntValue((char *)pstPExt->acData, "m_h");
		int chn = GetIntValue((char *)pstPExt->acData, "m_ch");
		int ret = mstream_get_param(chn, &resol);
		if(ret != 0)
		{
			printf("Invalid chn=%d\n", chn);
			return;
		}
		//printf("[%s]:%d	ch%d	res:%4d*%4d\n", __FUNCTION__, __LINE__, chn, resol.width, resol.height);
		PtzZoomZone(chn, &zone, resol.width, resol.height,pstPExt->nParam2);
		break;	
	case EX_PTZ_GETSPEED:
		pstPExt->nParam1 = 0;
		memset(pstPExt->acData, 0, RC_DATA_SIZE - 16);
		sprintf((char*)pstPExt->acData,"moveSpeed=%d;",abs(msoftptz_speed_get(0)));
		pstPExt->nParam2 = strlen((char*)pstPExt->acData);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
				20 + pstPExt->nParam2);		
		break;
	default:
		break;
	}
	WriteConfigInfo();
}

U32 build_storageMode(char *pData)
{
	pData[0] = '\0';
	U32 nSize = 0;
	char acItem[256] =
	{ 0 };

	mrecord_attr_t record;
	mrecord_get_param(0, &record);
	
	if(record.bEnable)
		sprintf(acItem, "storageMode=%d;chFrameSec=%d;", 1,0); // 手动录像
	else if(record.alarming || record.detecting)
		sprintf(acItem, "storageMode=%d;chFrameSec=%d;", 2,0); // 报警录像 
	else if(record.chFrame_enable)
	{
		sprintf(acItem, "storageMode=%d;chFrameSec=%d;", 3,record.chFrameSec); // 手动录像
	}
	else 
	{
		sprintf(acItem, "storageMode=%d;chFrameSec=%d;", 0,0); // 报警录像
	}
		
	nSize += strlen(acItem);
	strcat(pData, acItem);	

	Printf("Storages Mode' param:%s\n", pData);

	return nSize;	
}

U32 build_storage_param(char *pData)
{
	STORAGE storage;
	memset(&storage, 0, sizeof(STORAGE));

	pData[0] = '\0';
	if (0 != mstorage_get_info(&storage))
	{
		Printf("mstorage_get_info error\n");
		return 0;
	}
	
	U32 nSize = 0;
	char acItem[256] =
	{ 0 };

	//添加存储设备信息
	if (storage.nSize)
	{
		//存储器个数
		sprintf(acItem, "nStorage=%d;", 1);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//当前正在使用的分区
		sprintf(acItem, "nCurStorage=%d;", 0);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//第一个存储设备
		sprintf(acItem, "[STORAGE%d];", 1);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//总容量
		sprintf(acItem, "nTotalSize=%d;", storage.nSize);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//使用容量
		sprintf(acItem, "nUsedSize=%d;",
				(storage.nSize - storage.nFreeSpace[0]));
		nSize += strlen(acItem);
		strcat(pData, acItem);
		//存储器状态
		sprintf(acItem, "nStatus=%d;", storage.nStatus);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}

	printf("Storages param:%s\n", pData);

	return nSize;
}

//MStorage模块，远程指令处理,lck20120306
VOID StorageProc(REMOTECFG *remotecfg)
{
	S32 nSize = remotecfg->nSize - 4;
	char *pData = (char *)remotecfg->stPacket.acData;
	EXTEND *stEx = (EXTEND *)pData;
	Printf("RC_EXTEND, nSize=%d\n", nSize);
	if (nSize <= 0)
		return;

	STORAGE storage;
	switch (stEx->nType)
	{
	case EX_STORAGE_REC:
	{
		mrecord_attr_t record;
		mrecord_get_param(0, &record);
		char temp[32];
		if (GetKeyValue((char *)stEx->acData, "bRecEnable", temp, sizeof(temp)))
		{
			record.bEnable = atoi(temp);
		}
		if (GetKeyValue((char *)stEx->acData, "RecFileLength", temp, sizeof(temp)))
		{
			record.file_length = atoi(temp);
		}
		if (GetKeyValue((char *)stEx->acData, "bRecTimingEnable", temp, sizeof(temp)))
		{
			record.timing_enable = atoi(temp);
		}
		if (GetKeyValue((char *)stEx->acData, "RecTimingStart", temp, sizeof(temp)))
		{
			record.timing_start = atoi(temp);
		}
		if (GetKeyValue((char *)stEx->acData, "RecTimingStop", temp, sizeof(temp)))
		{
			record.timing_stop = atoi(temp);
		}
		if (GetKeyValue((char *)stEx->acData, "bRecDisconEnable", temp, sizeof(temp)))
		{
			record.discon_enable = atoi(temp);
		}
		if (GetKeyValue((char *)stEx->acData, "bRecAlarmEnable", temp, sizeof(temp)))
		{
			record.alarm_enable = atoi(temp);
			if(record.timing_enable || record.discon_enable)
			{
				record.alarm_enable = TRUE;
			}
		}
		if (GetKeyValue((char *)stEx->acData, "bRecChFrameEnable", temp, sizeof(temp)))
		{
			record.chFrame_enable = atoi(temp);
		}
		if(GetKeyValue((char*)stEx->acData,"chFrameSec",temp,sizeof(temp)))
		{
			record.chFrameSec = atoi(temp);
		}
		
		printf("[%s]\n", stEx->acData);
		printf("record.bEnable: %d\n, recdiscon: %d\n", record.bEnable, record.discon_enable);
		mrecord_set_param(0, &record);
		mrecord_flush(0);
		WriteConfigInfo();
	}
		break;
	case EX_STORAGE_REFRESH:
refresh:
		stEx->nParam1 = build_storage_param((char *)stEx->acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam1);
		break;
	case EX_STORAGE_FORMAT:
		{
			mrecord_attr_t Mrecord,Mrecord_back;
			mrecord_get_param(0, &Mrecord);
			memcpy(&Mrecord_back,&Mrecord,sizeof(mrecord_attr_t));
			Mrecord.bEnable = 0;
			Mrecord.discon_enable = FALSE;
			Mrecord.disconnected = FALSE;
			Mrecord.timing_enable = FALSE;
			Mrecord.alarm_enable = FALSE;			
			mrecord_set_param(0, &Mrecord);
		    mrecord_flush(0);
			if (SUCCESS == mstorage_format(stEx->nParam3))
			{
				stEx->nParam1 = EX_STORAGE_OK;
				Mrecord.bEnable = 1;
			}
		
			if (SUCCESS != mstorage_mount())
			{
				stEx->nParam1 = EX_STORAGE_ERR;
			}

			mrecord_set_param(0, &Mrecord_back);
			mrecord_flush(0);

			stEx->nParam2 = build_storage_param((char *)stEx->acData);
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam2);
		}
		break;
	case EX_STORAGE_SWITCH:
		{
			int bMode = 0;
			char temp[32] = {0};
			MAlarmIn_t ma;
			MD md;
			if(GetKeyValue((char *)stEx->acData, "storageMode", temp, sizeof(temp)))
			{
				mrecord_attr_t record;
				mrecord_get_param(0, &record);
				bMode = atoi(temp);
				if (hwinfo.bCloudSee == TRUE && bMode == 2)
				{
					//对应cloudsee的设备，2代表报警录像
					bMode = RECORD_MODE_ALARM;
				}
				if(RECORD_MODE_NORMAL == bMode)		//手动录像
				{
					record.bEnable = TRUE;
					record.discon_enable = FALSE;
					record.disconnected = FALSE;
					record.timing_enable = FALSE;
					record.alarm_enable = FALSE;
					record.chFrame_enable = FALSE;
				}
				else if(RECORD_MODE_ALARM == bMode)//报警录像
				{
					record.bEnable = FALSE;
					record.discon_enable = FALSE;
					record.disconnected = FALSE;
					record.timing_enable = FALSE;
					record.alarm_enable = TRUE;
					record.chFrame_enable = FALSE;
				}
				else if(RECORD_MODE_CHFRAME == bMode)
				{
					record.bEnable = FALSE;
					record.discon_enable = FALSE;
					record.disconnected = FALSE;
					record.timing_enable = FALSE;
					record.alarm_enable = FALSE;	
					record.chFrame_enable = TRUE;

					if(GetKeyValue((char *)stEx->acData, "chFrameSec", temp, sizeof(temp)))
					{
						record.chFrameSec = atoi(temp);
					}
					
				}
				else
				{
					record.bEnable = FALSE;
					record.discon_enable = FALSE;
					record.disconnected = FALSE;
					record.timing_enable = FALSE;
					record.alarm_enable = FALSE;	
					record.chFrame_enable = FALSE;
				}
				mrecord_set_param(0, &record);
				mrecord_flush(0);
				WriteConfigInfo();
			}
			Printf("[%s]\n", stEx->acData);
			Printf("recode mode change,the mode is %d \n", bMode);
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, 
				JVN_RSP_TEXTDATA, (U8*)&remotecfg->stPacket, 20);
		}
		break;
	case EX_STORAGE_GETRECMODE:
		{
			stEx->nParam1 = build_storageMode((char *)stEx->acData);
			MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam1);
			break;
		}

	default:
		break;
	}
}

U32 build_alarm_param(char *pData)
{
	char temp[256] =
	{ 0 };
	ALARMSET alarm;
	int nSize = 0;
	int i=0;
	
	malarm_get_param(&alarm);
	pData[0] = '\0';
	sprintf(temp, "nAlarmDelay=%d;", alarm.delay);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acMailSender=%s;", alarm.sender);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acSMTPServer=%s;", alarm.server);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acSMTPUser=%s;", alarm.username);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acSMTPPasswd=%s", alarm.passwd);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acReceiver0=%s;", alarm.receiver0);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acReceiver1=%s;", alarm.receiver1);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acReceiver2=%s;", alarm.receiver2);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acReceiver3=%s;", alarm.receiver3);
	nSize += strlen(temp);
	strcat(pData, temp);
	//add by xianlt at 20120628
	sprintf(temp, "acSMTPPort=%d;", alarm.port);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "acSMTPCrypto=%s;", alarm.crypto);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "vmsServerPort=%d;", alarm.vmsServerPort);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "vmsServerIp=%s;", alarm.vmsServerIp);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "bAlarmEnable=%d;", alarm.bEnable);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "bAlarmSound=%d;", alarm.bAlarmSoundEnable);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "bAlarmLight=%d;", alarm.bAlarmLightEnable);
	nSize += strlen(temp);
	strcat(pData, temp);
	sprintf(temp, "alarmLightStatus=%d;", mio_get_light_alarm_st());
	nSize += strlen(temp);
	strcat(pData, temp);

	for(i=0; i<MAX_ALATM_TIME_NUM; i++)
	{
		if(strlen(alarm.alarmTime[i].tStart) > 0)
		{
			sprintf(temp, "alarmTime%d=%s-%s;",i, alarm.alarmTime[i].tStart, alarm.alarmTime[i].tEnd);
			nSize += strlen(temp);
			strcat(pData, temp);	
		}
	}

	//add by xianlt at 20120628
	return nSize;
}

VOID AlarmOutProc(REMOTECFG* cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_ALARMOUT;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;
	switch (pstEx->nType)
	{
		case EX_ALARMOUT_REFRESH:
			Printf("EX_ALARMOUT_REFRESH\n");
			pstEx->nParam1 = 0;
			memset(pstEx->acData, 0, RC_DATA_SIZE - 16);
			pstEx->nParam2 = build_alarm_param((char *)pstEx->acData);
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
					20 + pstEx->nParam2);

			break;

		case EX_ALARMOUT_SUBMIT:
			break;

		default:
			break;
	}
}

/**
 *@brief 将用户信息按指定格式放到字符串中
 *@param pData 用来存储用户信息的BUFFER
 *@return 用户信息的长度
 *
 */
U32 build_account_param(char *pData)
{
	U32 i, nSize = 0;
	char acItem[256] =
	{ 0 };

	int cnt;
	ACCOUNT *act;
	pData[0] = '\0';
	cnt = maccount_get_cnt();

	for (i = 0; i < cnt; i++)
	{
		act = maccount_get(i);
		if (act->nIndex >= 0)
		{
			sprintf(acItem, "ID=%s;POWER=%d;DESCRIPT=%s;", act->acID,
					act->nPower, act->acDescript);
			nSize += strlen(acItem);
			strcat(pData, acItem);
		}
	}

	Printf("Accounts:%s\n", pData);

	return nSize;
}

/**
 *@brief 远程设置处理
 *@param pData 要解析的数据
 *@param nSize 要解析数据的长度
 */
VOID AccountProc(REMOTECFG *remotecfg)
{
	char *pData = (char *)remotecfg->stPacket.acData;
	S32 nSize = remotecfg->nSize - 4;
	EXTEND *stEx = (EXTEND *)pData;
	ACCOUNT stTmp;
	ACCOUNT *account;

	Printf("RC_EXTEND, nSize=%d\n", nSize);
	if (nSize <= 0)
		return;

	ACCOUNT stAccount;
	account = GetClientAccount(remotecfg->nClientID, &stAccount);
	if (account == NULL)
	{
		Printf(
				"ERROR: account is NULL, remotecfg->nClientID: %d\n", remotecfg->nClientID);
		return;
	}

	switch (stEx->nType)
	{
	case EX_ACCOUNT_REFRESH:
		stEx->nParam1 = build_account_param((char *)stEx->acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam1);
		break;
	case EX_ACCOUNT_ADD:
		memcpy(stTmp.acID, stEx->acData, SIZE_ID);
		memcpy(stTmp.acPW, stEx->acData + SIZE_ID, SIZE_PW);
		memcpy(stTmp.acDescript, stEx->acData + SIZE_ID + SIZE_PW,
				SIZE_DESCRIPT);
		stTmp.nPower = stEx->nParam1;
		if (account->nPower & POWER_ADMIN)
			stEx->nParam1 = maccount_add(&stTmp);
		else
			stEx->nParam1 = ERR_PERMISION_DENIED;
		stEx->nParam2 = build_account_param((char *)stEx->acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam2);
		break;
	case EX_ACCOUNT_DEL:
		stEx->nType = EX_ACCOUNT_DEL;
		memcpy(stTmp.acID, stEx->acData, stEx->nParam1);
		memcpy(stTmp.acPW, stEx->acData + stEx->nParam1, stEx->nParam2);
		stTmp.nPower = stEx->nParam3;
		if (account->nPower & POWER_ADMIN
				&& (0 != strcmp(account->acID, stTmp.acID))) //必须是管理员而且不是自己
			stEx->nParam1 = maccount_remove(&stTmp);
		else
			stEx->nParam1 = ERR_PERMISION_DENIED;
		stEx->nParam2 = build_account_param((char *)stEx->acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam2);
		break;
	case EX_ACCOUNT_MODIFY:
		memcpy(stTmp.acID, stEx->acData, SIZE_ID);
		memcpy(stTmp.acPW, stEx->acData + SIZE_ID, SIZE_PW);
		memcpy(stTmp.acDescript, stEx->acData + SIZE_ID + SIZE_PW,
				SIZE_DESCRIPT);
		stTmp.nPower = stEx->nParam1;

		//admin和自己，可以修改用户信息
		if (account->nPower & POWER_ADMIN)
			stEx->nParam1 = maccount_modify(&stTmp);
		else if (0 == strcmp(account->acID, stTmp.acID))
		{
			if (stTmp.nPower & POWER_ADMIN)
				stEx->nParam1 = ERR_PERMISION_DENIED;
			else
				stEx->nParam1 = maccount_modify(&stTmp);
		}
		else
			stEx->nParam1 = ERR_PERMISION_DENIED;

		stEx->nParam2 = build_account_param((char *)stEx->acData);
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket, 20 + stEx->nParam2);
		break;
	default:
		break;
	}
}

/**
 *@brief 把字符串参数转化为结构体
 *@param pData INPUT 字符串的内存地址
 *@param md OUTPUT 报警输入的结构体
 *@return 无
 *
 */
static void _malarmin_string2param(char *pData, MAlarmIn_t *ma)
{
	char key[30];
	char value[30];
	while (*pData != '\0')
	{
		pData = sysfunc_get_key_value(pData, key, value, 30);
		//移动检测
		if (strncmp(key, "alarmin_bEnable", 15) == 0)
		{
			ma->bEnable = atoi(value);
		}
		else if (strncmp(key, "alarmin_bNormallyClosed", 25) == 0)
		{
			ma->bNormallyClosed = atoi(value);
		}
		else if (strncmp(key, "alarmin_bSendtoClient", 21) == 0)
		{
			ma->bSendtoClient = atoi(value);
		}
		else if (strncmp(key, "alarmin_bSendtoVMS", 18) == 0)
		{
			ma->bSendtoVMS = atoi(value);
		}
		else if (strncmp(key, "alarmin_bSendEmail", 18) == 0)
		{
			ma->bSendEmail = atoi(value);
		}
		else if (strncmp(key, "alarmin_bEnRecord", 17) == 0)
		{
			ma->bEnableRecord = atoi(value);
		}
		else if (strncmp(key, "alarmin_u8Num", 13) == 0)
		{
			ma->u8AlarmNum = atoi(value);
		}
		else if (strncmp(key, "alarmin_bBuzzing", 17) == 0)
		{
			ma->bBuzzing = atoi(value);
		}
	}
}
//远程设置处理,报警输入处理
VOID MAlarmInProc(REMOTECFG *cfg)
{
	MAlarmIn_t ma;
	EXTEND *pstMAExt = (EXTEND*) (cfg->stPacket.acData);

	switch (pstMAExt->nType)
	{
	case EX_MALARMIN_SUBMIT:
		Printf("EX_MD_SUBMIT acData=%s\n", pstMAExt->acData);
		malarmin_get_param(0, &ma);
		_malarmin_string2param((char *)pstMAExt->acData, &ma);
		malarmin_set_param(0, &ma);
		WriteConfigInfo();
		malarmin_flush(0);
		pstMAExt->nType = EX_MALARMIN_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	default:
		break;
	}
}

static void _doorAlarm_string2param(char *pData, MDoorAlarm_t *da)
{
	char key[30];
	char value[30];
	while (*pData != '\0')
	{
		pData = sysfunc_get_key_value(pData, key, value, 30);
		if (strncmp(key, "dooralarm_bEnable", strlen("dooralarm_bEnable")) == 0)
		{
			da->bEnable = atoi(value);
		}
		if (strncmp(key, "dooralarm_bEnRecord", strlen("dooralarm_bEnRecord")) == 0)
		{
			da->bEnableRecord = atoi(value);
		}
		if (strncmp(key, "dooralarm_bSendtoClient", strlen("dooralarm_bSendtoClient")) == 0)
		{
			da->bSendtoClient = atoi(value);
		}
		if (strncmp(key, "dooralarm_bSendtoVMS", strlen("dooralarm_bSendtoVMS")) == 0)
		{
			da->bSendtoVMS = atoi(value);
		}
		if (strncmp(key, "dooralarm_bSendEmail", strlen("dooralarm_bSendEmail")) == 0)
		{
			da->bSendEmail = atoi(value);
		}
		if (strncmp(key, "dooralarm_bBuzzing", strlen("dooralarm_bBuzzing")) == 0)
		{
			da->bBuzzing = atoi(value);
		}
	}
}
//远程设置处理,门磁报警处理
VOID MDoorAlarmProc(REMOTECFG *cfg)
{
	MDoorAlarm_t doorAlarm;
	EXTEND *pstMAExt = (EXTEND*) (cfg->stPacket.acData);

	switch (pstMAExt->nType)
	{
	case EX_MDOORALARM_SUBMIT:
		Printf("EX_MDOORALARM_SUBMIT acData=%s\n", pstMAExt->acData);
		mdooralarm_get_param(&doorAlarm);
		_doorAlarm_string2param((char *)pstMAExt->acData, &doorAlarm);
		mdooralarm_set_param(&doorAlarm);
		WriteConfigInfo();
		pstMAExt->nType = EX_MDOORALARM_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	default:
		break;
	}
}

//远程设置处理，云视通注册处理
VOID MRegisterProc(REMOTECFG *remotecfg)
{
	//printf("********************LK test Register***********************\n");
	remotecfg->stPacket.nPacketType = RC_EXTEND;
	remotecfg->stPacket.nPacketCount = RC_EX_REGISTER;
	EXTEND *pstEx = (EXTEND*) remotecfg->stPacket.acData;
	YST		stYST;
	GetYSTParam(&stYST);
	printf("register,bOnline=%d\n",stYST.nStatus);
	if (JVN_SONLINETYPE_ONLINE == stYST.nStatus)
	{
		//云视通已上线，无需注册，走下过场
		pstEx->nType = EX_REGISTER_ONLINE;
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
						JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket,
						20);
	}
	else
	{
		int ret;
		STGetNum *pkGetNum;
		unsigned char cBuff[256];
		int * pTemp;
		ipcinfo_t ipcinfo;

		ipcinfo_get_param(&ipcinfo);

		pTemp = (int *) cBuff;
		pTemp[0] = JVN_CMD_REGCARD;
		pTemp[1] = sizeof(STGetNum);

		pkGetNum = (STGetNum *) &cBuff[8];
		memcpy(pkGetNum->chGroup, &ipcinfo.nDeviceInfo[6], 4);
		pkGetNum->nCardType = ipcinfo.nDeviceInfo[7];
		pkGetNum->nDate = ipcinfo.nDeviceInfo[5];
		pkGetNum->nSerial = ipcinfo.nDeviceInfo[4];
		memcpy(&(pkGetNum->guid), &ipcinfo.nDeviceInfo[0], sizeof(GUID));

		//通过云视通注册产品，多出8个字节的数据以防意外
		if(gp.bNeedYST)
		{
			ret = JVN_RegCard((char *) &ipcinfo.nDeviceInfo[6], cBuff, sizeof(STGetNum)+ 8 + 8);
			if(ret)
				pstEx->nType = EX_REGISTER_SUCCESS;
			else
				pstEx->nType = EX_REGISTER_FAILE;
		}
		MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID,
				JVN_RSP_TEXTDATA, (U8*) &remotecfg->stPacket,
				20);
	}
}


//远程设置，局部曝光
static void _roi_string2param(char *pData, roiInf *roi_info)
{
	char key[30];
	char value[30];
	int i= 0;
	while (*pData != '\0')
	{
		pData = sysfunc_get_key_value(pData, key, value, 30);
		//移动检测
//		if (strncmp(key, "bROI_Exposure_Enable", 20) == 0)
//		{
//			roi_info->bROIExposureEN = atoi(value);
//		}
//		if (strncmp(key, "bROI_QP_Enable", 14) == 0)
//		{
//			roi_info->bROIQPEN = atoi(value);
//		}
		if (strncmp(key, "bROI_Reverse", 12) == 0)
		{
			roi_info->bROIReverse = atoi(value);
		}
		else if (strncmp(key, "nROI_Weight_QP", 15) == 0)
		{
			roi_info->nWeightQP = atoi(value);
		}
		else if (strncmp(key, "nROI_Weight_Exposure", 20) == 0)
		{
			roi_info->nWeightExposure = atoi(value);
		}
		else if (strncmp(key, "ROI_Region", 10) == 0)
		{
			sscanf(value,"%d,%d,%d,%d",&roi_info->roiRect[i].x,&roi_info->roiRect[i].y,&roi_info->roiRect[i].w,&roi_info->roiRect[i].h);
			i++;
		}
	}
}
U32 build_roi_param(int channelid, char *pData)
{
	U32 i, nSize = 0;
	char acItem[256] ={ 0 };
	MLE_t mle1;
	MLE_t *mle = &mle1;
	mstream_roi_t streamroi;
	mstream_get_roi(&streamroi);
	msensor_get_local_exposure(0,mle);

	pData[0] = '\0';

	/*//是否开启
//	sprintf(acItem, "bROI_Exposure_Enable=%d;", 1);
//	nSize += strlen(acItem);
//	strcat(pData, acItem);
//	sprintf(acItem, "bROI_QP_Enable=%d;", 0);
//	nSize += strlen(acItem);
//	strcat(pData, acItem);
    */

//	sprintf(acItem, "bROI_Reverse=%d;", streamroi.ior_reverse);
	sprintf(acItem, "bROI_Reverse=%d;", mle1.bLE_Reverse);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//灵敏度
	sprintf(acItem, "nROI_Weight_QP=%d;", streamroi.roiWeight);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "nROI_Weight_Exposure=%d;", mle1.nLE_Weight);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//四个区域
	for (i = 0; i < RECT_MAX_CNT; i++)
	{
//		RECT *pRect = &streamroi.jv_roi.roi[i];
		RECT *pRect = &mle1.stLE_Rect[i];
		sprintf(acItem, "ROI_Region%d=%d,%d,%d,%d;", i, pRect->x, pRect->y,
				pRect->w, pRect->h);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
	printf("ROI:%s\n", pData);

	return nSize;
}

VOID ROIProc(REMOTECFG *cfg)
{
	MLE_t mle;					//局部曝光信息
	mstream_roi_t streamroi;	//感兴趣区域信息
	roiInf roi_info;			//中间传递参数用
	EXTEND *pstMDExt = (EXTEND*) (cfg->stPacket.acData);
	int i;
	stSnapSize size;
	size.nWith = 400;
	size.nHeight = 224;
	switch (pstMDExt->nType)
	{
	case EX_ROI_UPDATE:
		Printf("EX_ROI_UPDATE\n");
		pstMDExt->nType = EX_ROI_UPDATE;
		pstMDExt->nParam1 = msnapshot_get_data(0, pstMDExt->acData,RC_DATA_SIZE-16,&size);
		pstMDExt->nParam2 = build_roi_param(0,
				(char *)pstMDExt->acData + pstMDExt->nParam1);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket,
				20 + pstMDExt->nParam1 + pstMDExt->nParam2);
		break;
	case EX_ROI_REFRESH:
		Printf("EX_ROI_REFRESH\n");
		pstMDExt->nType = EX_ROI_REFRESH;
		pstMDExt->nParam1 = msnapshot_get_data(0, pstMDExt->acData,RC_DATA_SIZE-16,&size);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstMDExt->nParam1);
		break;
	case EX_ROI_SUBMIT:
		printf("EX_ROI_SUBMIT acData=%s\n", pstMDExt->acData);
		_roi_string2param((char *)pstMDExt->acData, &roi_info);
		mstream_get_roi(&streamroi);
		streamroi.ior_reverse = roi_info.bROIReverse;
		streamroi.roiWeight = roi_info.nWeightQP;
		memcpy(&streamroi.roi, &roi_info.roiRect,sizeof(RECT)*MAX_ROI_REGION);
		mstream_set_roi(&streamroi);
//		int i;
//		for (i=0;i<HWINFO_STREAM_CNT;i++)
//		{
//			mstream_flush(i);
//		}

		mstream_attr_t attr;
		mstream_get_param(0,&attr);
//		printf("+++++++++++++++%d,%d\n",attr.mroi.ior_reverse,attr.mroi.jv_roi.roiWeight);
//		printf("+++++++++++++++%d,%d\n",streamroi.ior_reverse,streamroi.jv_roi.roiWeight);
//		if(roi_info.nWeightExposure > 0)
		{
			msensor_get_local_exposure(0,&mle);
			mle.bLE_Reverse = roi_info.bROIReverse;
			mle.nLE_Weight = roi_info.nWeightExposure;
			memcpy(&mle.stLE_Rect,&roi_info.roiRect,sizeof(RECT)*ROI_MAX_RECT_CNT);
			msensor_set_local_exposure(0,&mle);
		}

		WriteConfigInfo();
//		mdetect_flush(0);
		pstMDExt->nType = EX_ROI_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	default:
		break;
	}
}

VOID QRCodeProc(REMOTECFG *remotecfg)
{
	YST yst;
	int W, H;
	IPCType_e tmptype;
	char tmphttpipa[100],tmphttpapk[100];
	remotecfg->stPacket.nPacketType = RC_EXTEND;
	remotecfg->stPacket.nPacketCount = RC_EX_QRCODE;
	EXTEND *pstEx = (EXTEND*) remotecfg->stPacket.acData;

	tmptype = ipcinfo_get_type();
	switch (tmptype)
	{
	case IPCTYPE_JOVISION:
	case IPCTYPE_IPDOME:
	default:
		strcpy(tmphttpapk,"http://www.jovetech.com/UpLoadFiles/file/JVCloudSeeClient.apk");
		strcpy(tmphttpipa,"http://www.jovetech.com/UpLoadFiles/file/CloudSEE for iPhone v4.9.5.ipa");
		break;
	case IPCTYPE_SW:
		strcpy(tmphttpapk, "http://www.sunywo.com/userfiles/nvsip.apk");
		strcpy(tmphttpipa, "http://www.sunywo.com/userfiles/nvsipv.ipa");
		break;
	}

	pstEx->nType = EX_QRCODE_EX;
	GetYSTParam(&yst);
	char ystnum[20];
	sprintf(ystnum, "%s%d", yst.strGroup, yst.nID);
	memcpy(pstEx->acData,ystnum,strlen(ystnum));
	pstEx->nParam1 = strlen(ystnum);
	memcpy(pstEx->acData+pstEx->nParam1,tmphttpapk,strlen(tmphttpapk));
	pstEx->nParam2 = strlen(tmphttpapk);
	memcpy(pstEx->acData+pstEx->nParam1+pstEx->nParam2,tmphttpipa,strlen(tmphttpipa));
	pstEx->nParam3 = strlen(tmphttpipa);
	MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA,
			(U8*) &remotecfg->stPacket, 20 + pstEx->nParam1+pstEx->nParam2+pstEx->nParam3);


/*	pstEx->nType = EX_QRCODE_YST;
	GetYSTParam(&yst);
	char ystnum[20];
	sprintf(ystnum, "%s%d", yst.strGroup, yst.nID);
	memcpy(pstEx->acData,ystnum,strlen(ystnum));
	pstEx->nParam3 = strlen(ystnum);
	MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &remotecfg->stPacket, 20 + pstEx->nParam3);
	usleep(1000*100);

	pstEx->nType = EX_QRCODE_AND;
	memcpy(pstEx->acData,tmphttpapk,strlen(tmphttpapk));
	pstEx->nParam3 = strlen(tmphttpapk);
	MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA,
					(U8*) &remotecfg->stPacket, 20 + pstEx->nParam3);
	usleep(1000*100);

	pstEx->nType = EX_QRCODE_IOS;
	memcpy(pstEx->acData,tmphttpipa,strlen(tmphttpipa));
	pstEx->nParam3 = strlen(tmphttpipa);
	MT_TRY_SendChatData(remotecfg->nCh, remotecfg->nClientID, JVN_RSP_TEXTDATA,
					(U8*) &remotecfg->stPacket, 20 + pstEx->nParam3);*/

}

VOID YSTNickNameProc(REMOTECFG *remotecfg)
{
	
}

static void _ivp_string2param(char *pData, MIVP_t *mivp)
{
	char key[30];
	char value[128];
	int i= 0;
	mivp->bNeedRestart = FALSE;
	while (*pData != '\0')
	{
		pData = sysfunc_get_key_value(pData, key, value, 128);
		if (strncmp(key, "IVPEnable", 9) == 0)
		{
			if(mivp->st_rl_attr.bEnable != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_rl_attr.bEnable = atoi(value);
		}
		else if (strncmp(key, "IVPRgnCnt", 9) == 0)
		{
			if(mivp->st_rl_attr.nRgnCnt != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_rl_attr.nRgnCnt = atoi(value);
		}
		else if (strncmp(key, "IVPRegion", 9) == 0)
		{
			MIVPREGION_t stRegion[MAX_IVP_REGION_NUM];
			memcpy(&stRegion,&mivp->st_rl_attr.stRegion,sizeof(stRegion));
			int n=0;
			char point[128];
			sscanf(key, "IVPRegion%d", &n);
			if(n>=MAX_IVP_REGION_NUM)
				break;
			sscanf(value, "%d:%d:%s", &mivp->st_rl_attr.stRegion[n].nIvpCheckMode,&mivp->st_rl_attr.stRegion[n].nCnt,point);
			if(mivp->st_rl_attr.stRegion[n].nCnt <= 0)
				break;
			for(i=0;i<mivp->st_rl_attr.stRegion[n].nCnt;i++)
			{
				if(i==mivp->st_rl_attr.stRegion[n].nCnt-1)
					sscanf(point, "%d,%d", &mivp->st_rl_attr.stRegion[n].stPoints[i].x,&mivp->st_rl_attr.stRegion[n].stPoints[i].y);
				else
					sscanf(point, "%d,%d-%s", &mivp->st_rl_attr.stRegion[n].stPoints[i].x,&mivp->st_rl_attr.stRegion[n].stPoints[i].y,point);
			}
			if(memcmp(&stRegion,&mivp->st_rl_attr.stRegion,sizeof(stRegion))!=0)
				mivp->bNeedRestart = TRUE;
		}
		else if (strncmp(key, "nClimbPoints", 12) == 0)
		{
			MIVP_RL_t  sRlClimb;
			memcpy(&sRlClimb,&mivp->st_rl_attr,sizeof(MIVP_RL_t));
			int n=0;
			char point[128];
			sscanf(value, "%d:%s", &n,point);
			if( n != 2 )
				n=0;
			mivp->st_rl_attr.stClimb.nPoints = n;

			for(i=0;i<mivp->st_rl_attr.stClimb.nPoints;i++)
			{
				if(i==mivp->st_rl_attr.stClimb.nPoints-1)
					sscanf(point, "%d,%d", &mivp->st_rl_attr.stClimb.stPoints[i].x,&mivp->st_rl_attr.stClimb.stPoints[i].y);
				else
					sscanf(point, "%d,%d-%s", &mivp->st_rl_attr.stClimb.stPoints[i].x,&mivp->st_rl_attr.stClimb.stPoints[i].y,point);
			}
			if(memcmp(&sRlClimb,&mivp->st_rl_attr,sizeof(MIVP_RL_t))!=0)
				mivp->bNeedRestart = TRUE;
		}		
		else if (strncmp(key, "bDrawFrame", 10) == 0)
		{
			if(mivp->st_rl_attr.bDrawFrame != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_rl_attr.bDrawFrame = atoi(value);
		}
		else if (strncmp(key, "bFlushFrame", 11) == 0)
		{
			if(mivp->st_rl_attr.bFlushFrame != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_rl_attr.bFlushFrame = atoi(value);
		}
		else if (strncmp(key, "bMarkAll", 8) == 0)
		{
			if(mivp->st_rl_attr.bMarkAll != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_rl_attr.bMarkAll = atoi(value);
		}
		else if (strncmp(key, "IVPAlpha", 8) == 0)
		{
			mivp->st_rl_attr.nAlpha = atoi(value);
		}
		else if (strncmp(key, "IVPRecEnable", 12) == 0)
		{
			mivp->st_rl_attr.stAlarmOutRL.bEnableRecord = atoi(value);
		}
		else if (strncmp(key, "IVPOutAlarm1", 12) == 0)
		{
			mivp->st_rl_attr.stAlarmOutRL.bOutAlarm1 = atoi(value);
		}
		else if (strncmp(key, "IVPAlarmSound", 13) == 0)
		{
			mivp->st_rl_attr.stAlarmOutRL.bOutSound= atoi(value);
		}		
		else if (strncmp(key, "IVPOutClient", 12) == 0)
		{
			mivp->st_rl_attr.stAlarmOutRL.bOutClient = atoi(value);
		}
		else if (strncmp(key, "IVPOutEMail", 11) == 0)
		{
			mivp->st_rl_attr.stAlarmOutRL.bOutEMail = atoi(value);
		}
		else if (strncmp(key, "IVPOutVMS", 9) == 0)
		{
			mivp->st_rl_attr.stAlarmOutRL.bOutVMS= atoi(value);
		}
		else if (strncmp(key, "IVPSen", 6) == 0)
		{
			mivp->st_rl_attr.nSen = atoi(value);
		}
		else if (strncmp(key, "IVPThreshold", 12) == 0)
		{
			mivp->st_rl_attr.nThreshold = atoi(value);
		}
		else if (strncmp(key, "IVPStayTime", 11) == 0)
		{
			mivp->st_rl_attr.nStayTime = atoi(value);
		}

		else if (strncmp(key, "IVPMarkObject", 13) == 0)
		{
			if(mivp->st_rl_attr.bMarkObject != atoi(value))
			{
				mivp->bNeedRestart = TRUE;
			}
			mivp->st_rl_attr.bMarkObject = atoi(value);
		}
		else if (strncmp(key, "IVPPlateSnap", 12) == 0)
		{
			mivp->bPlateSnap = atoi(value);
		}
		else if (strncmp(key, "IVPSnapRes", 10) == 0)
		{
			strcpy(mivp->sSnapRes, value);
		}

		//人数统计部分
		else if (strncmp(key, "bMarkObject", 11) == 0)
		{
			mivp->st_count_attr.bMarkObject = atoi(value);
		}
		else if (strncmp(key, "bOpenCount", 10) == 0)
		{
			if(mivp->st_count_attr.bOpenCount != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_count_attr.bOpenCount = atoi(value);
		}
		else if (strncmp(key, "bShowCount", 10) == 0)
		{
			mivp->st_count_attr.bShowCount = atoi(value);
		}
		else if (strncmp(key, "bDrawCountLine", 11) == 0)
		{
			if(mivp->st_count_attr.bDrawFrame != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_count_attr.bDrawFrame = atoi(value);
		}
		else if (strncmp(key, "IVPCountOSDPos", 14) == 0)
		{
			mivp->st_count_attr.eCountOSDPos =	(mivpcountosd_pos_e) atoi(value);
		}
		else if (strncmp(key, "IVPCountOSDColor", 16) == 0)
		{
			mivp->st_count_attr.nCountOSDColor = atoi(value);
		}
		else if (strncmp(key, "nCountSaveDays", 16) == 0)
		{
			mivp->st_count_attr.nCountSaveDays = atoi(value);
		}
		//计数新增设置进入的方向,以及检测线
		else if (strncmp(key, "CheckCountMode", 14) == 0)
		{
			if(mivp->st_count_attr.nCheckMode != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_count_attr.nCheckMode = atoi(value);
		}	
		else if (strncmp(key, "nCountPoints", 12) == 0)
		{
			MIVP_Count_t stLineCount;
			memcpy(&stLineCount,&mivp->st_count_attr,sizeof(MIVP_Count_t));
			int n=0;
			char point[128];
			sscanf(value, "%d:%s", &n,point);
			if( n != 2 )
				n=0;
			mivp->st_count_attr.nPoints = n;

			for(i=0;i<mivp->st_count_attr.nPoints;i++)
			{
				if(i==mivp->st_count_attr.nPoints-1)
					sscanf(point, "%d,%d", &mivp->st_count_attr.stPoints[i].x,&mivp->st_count_attr.stPoints[i].y);
				else
					sscanf(point, "%d,%d-%s", &mivp->st_count_attr.stPoints[i].x,&mivp->st_count_attr.stPoints[i].y,point);
			}
			if(memcmp(&stLineCount,&mivp->st_count_attr,sizeof(MIVP_Count_t))!=0)
				mivp->bNeedRestart = TRUE;
		}
		
		//移动目标侦测
		else if (strncmp(key, "bIVPDetectEn", 12) == 0)
		{
			if(mivp->st_detect_attr.bDetectEn!=atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_detect_attr.bDetectEn = atoi(value);
		}
		//遮挡报警
		else if (strncmp(key, "bIVPHideAlarmEn", 16) == 0)
		{
			if(mivp->st_hide_attr.bHideEnable!=atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_hide_attr.bHideEnable = atoi(value);
		}
		else if (strncmp(key, "bIVPHideThreshold", 17) == 0)
		{
			mivp->st_hide_attr.nThreshold = atoi(value);
		}
		else if (strncmp(key, "bIVPHideAlarmRecEn", 18) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bEnableRecord = atoi(value);
		}		
		else if (strncmp(key, "bIVPHideAlarmRecEn", 18) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bEnableRecord = atoi(value);
		}
		else if (strncmp(key, "bIVPHideAlarmOut1", 17) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bOutAlarm1 = atoi(value);
		}
		else if (strncmp(key, "bIVPHideAlarm2Client", 20) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bOutClient = atoi(value);
		}
		else if (strncmp(key, "bIVPHideAlarm2Email", 19) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bOutEMail = atoi(value);
		}
		else if (strncmp(key, "bIVPHideAlarm2VMS", 17) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bOutVMS = atoi(value);
		}
		else if (strncmp(key, "bIVPHideSound", 12) == 0)
		{
			mivp->st_hide_attr.stHideAlarmOut.bOutSound= atoi(value);
		}
		//遗留拿取报警
		else if (strncmp(key, "bIVPTLEnable", 12) == 0)
		{
			if (mivp->st_tl_attr.bTLEnable != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_tl_attr.bTLEnable = atoi(value);
		}
		else if (strncmp(key, "bIVPTLMode", 12) == 0)
		{
			if (mivp->st_tl_attr.nTLMode != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_tl_attr.nTLMode = atoi(value);
		}
		else if (strncmp(key, "IVPTLRgnCnt", 11) == 0)
		{
			if (mivp->st_tl_attr.nTLRgnCnt != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_tl_attr.nTLRgnCnt = atoi(value);
		}
		else if (strncmp(key, "IVPTLRegion", 11) == 0)
		{
			MIVPREGION_t stRegion[MAX_IVP_REGION_NUM];
			memcpy(&stRegion, &mivp->st_tl_attr.stTLRegion, sizeof(stRegion));
			int n = 0;
			char point[128];
			sscanf(key, "IVPTLRegion%d", &n);
			if (n >= MAX_IVP_REGION_NUM)
				break;
			sscanf(value, "%d:%d:%s",
					&mivp->st_tl_attr.stTLRegion[n].nIvpCheckMode,
					&mivp->st_tl_attr.stTLRegion[n].nCnt, point);
			for (i = 0; i < mivp->st_tl_attr.stTLRegion[n].nCnt; i++)
			{
				if (i == mivp->st_tl_attr.stTLRegion[n].nCnt - 1)
					sscanf(point, "%d,%d",&mivp->st_tl_attr.stTLRegion[n].stPoints[i].x,&mivp->st_tl_attr.stTLRegion[n].stPoints[i].y);
				else
					sscanf(point, "%d,%d-%s",
							&mivp->st_tl_attr.stTLRegion[n].stPoints[i].x,
							&mivp->st_tl_attr.stTLRegion[n].stPoints[i].y, point);
			}
			if (memcmp(&stRegion, &mivp->st_tl_attr.stTLRegion, sizeof(stRegion))
					!= 0)
				mivp->bNeedRestart = TRUE;
		}
		else if (strncmp(key, "IVPTLRecEnable", 14) == 0)
		{
			mivp->st_tl_attr.stTLAlarmOut.bEnableRecord = atoi(value);
		}
		else if (strncmp(key, "IVPTLOutAlarm1", 14) == 0)
		{
			mivp->st_tl_attr.stTLAlarmOut.bOutAlarm1 = atoi(value);
		}
		else if (strncmp(key, "IVPTLOutClient", 14) == 0)
		{
			mivp->st_tl_attr.stTLAlarmOut.bOutClient = atoi(value);
		}
		else if (strncmp(key, "IVPTLOutEMail", 13) == 0)
		{
			mivp->st_tl_attr.stTLAlarmOut.bOutEMail = atoi(value);
		}
		else if (strncmp(key, "IVPTLOutVMS", 11) == 0)
		{
			mivp->st_tl_attr.stTLAlarmOut.bOutVMS = atoi(value);
		}
		else if (strncmp(key, "IVPTLSen", 8) == 0)
		{
			mivp->st_tl_attr.nTLSen = atoi(value);
		}
		else if (strncmp(key, "IVPTLAlarmDuration", 18) == 0)
		{
			mivp->st_tl_attr.nTLAlarmDuration = atoi(value);
		}
		else if (strncmp(key, "IVPTLSuspectTime", 18) == 0)
		{
			mivp->st_tl_attr.nTLSuspectTime = atoi(value);
		}

		
	}
}
U32 build_ivp_param(int chn, char *pData)
{
	U32 i,j, nSize = 0;
	char acItem[256] ={ 0 };
	MIVP_t mivp;
	mivp_get_param(chn,&mivp);

	MIVP_t mivp_count;
	mivp_count_get(chn,&mivp_count);

	pData[0] = '\0';

	sprintf(acItem, "IVPEnable=%d;", mivp.st_rl_attr.bEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPRgnCnt=%d;", mivp.st_rl_attr.nRgnCnt);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	for(i = 0; i<mivp.st_rl_attr.nRgnCnt; i++)
	{
		sprintf(acItem, "IVPRegion%d=%d:%d:", i,mivp.st_rl_attr.stRegion[i].nIvpCheckMode,mivp.st_rl_attr.stRegion[i].nCnt);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		for(j = 0; j<mivp.st_rl_attr.stRegion[i].nCnt; j++)
		{
			sprintf(acItem, "%d,%d-",mivp.st_rl_attr.stRegion[i].stPoints[j].x,mivp.st_rl_attr.stRegion[i].stPoints[j].y);
			nSize += strlen(acItem);
			strcat(pData, acItem);
		}
		pData[nSize-1]=';';
	}
	sprintf(acItem, "nClimbPoints=%d:", mivp.st_rl_attr.stClimb.nPoints);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	for(j = 0; j<mivp.st_rl_attr.stClimb.nPoints; j++)
	{
		int x = mivp.st_rl_attr.stClimb.stPoints[j].x ;
		int y = mivp.st_rl_attr.stClimb.stPoints[j].y ;
		sprintf(acItem, "%d,%d-",x,y);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
//	if(mivp.st_rl_attr.stClimb.nPoints <= 0)
//		nSize++;
	pData[nSize-1]=';';
	
	sprintf(acItem, "bDrawFrame=%d;", mivp.st_rl_attr.bDrawFrame);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bFlushFrame=%d;", mivp.st_rl_attr.bFlushFrame);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bMarkAll=%d;", mivp.st_rl_attr.bMarkAll);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPAlpha=%d;", mivp.st_rl_attr.nAlpha);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPRecEnable=%d;", mivp.st_rl_attr.stAlarmOutRL.bEnableRecord);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPOutAlarm1=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutAlarm1);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPAlarmSound=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutSound);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPOutClient=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutClient);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPOutEMail=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutEMail);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPSen=%d;", mivp.st_rl_attr.nSen);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPThreshold=%d;", mivp.st_rl_attr.nThreshold);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPStayTime=%d;", mivp.st_rl_attr.nStayTime);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPOutVMS=%d;", mivp.st_rl_attr.stAlarmOutRL.bOutVMS);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	sprintf(acItem, "IVPMarkObject=%d;", mivp.st_rl_attr.bMarkObject);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPPlateSnap=%d;", mivp.bPlateSnap);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPSnapRes=%s;", mivp.sSnapRes);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	//人数统计部分
	sprintf(acItem, "bMarkObject=%d;", mivp_count.st_count_attr.bMarkObject);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bOpenCount=%d;", mivp_count.st_count_attr.bOpenCount);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bShowCount=%d;", mivp_count.st_count_attr.bShowCount);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bDrawCountLine=%d;", mivp_count.st_count_attr.bDrawFrame);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPCountOSDPos=%d;", mivp_count.st_count_attr.eCountOSDPos);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPCountOSDColor=%d;", mivp_count.st_count_attr.nCountOSDColor);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "nCountSaveDays=%d;", mivp_count.st_count_attr.nCountSaveDays);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "CheckCountMode=%d;", mivp_count.st_count_attr.nCheckMode);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "nCountPoints=%d:", mivp_count.st_count_attr.nPoints);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	for(j = 0; j<mivp_count.st_count_attr.nPoints; j++)
	{
		int x = mivp_count.st_count_attr.stPoints[j].x ;
		int y = mivp_count.st_count_attr.stPoints[j].y ;
		sprintf(acItem, "%d,%d-",x,y);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
//	if(mivp_count.st_count_attr.nPoints <= 0)
//		nSize++;
	pData[nSize-1]=';';
	//移动目标侦测
	sprintf(acItem, "bIVPDetectEn=%d;",mivp.st_detect_attr.bDetectEn);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//遮挡报警
	sprintf(acItem, "bIVPHideAlarmEn=%d;",mivp.st_hide_attr.bHideEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideThreshold=%d;",mivp.st_hide_attr.nThreshold);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideAlarmRecEn=%d;", mivp.st_hide_attr.stHideAlarmOut.bEnableRecord);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideAlarmOut1=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutAlarm1);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideAlarm2Client=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutClient);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideAlarm2Email=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutEMail);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideAlarm2VMS=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutVMS);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPHideSound=%d;", mivp.st_hide_attr.stHideAlarmOut.bOutSound);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	//遗留拿取报警
	sprintf(acItem, "bIVPTLEnable=%d;", mivp.st_tl_attr.bTLEnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "bIVPTLMode=%d;", mivp.st_tl_attr.nTLMode);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLRgnCnt=%d;", mivp.st_tl_attr.nTLRgnCnt);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	for (i = 0; i < mivp.st_tl_attr.nTLRgnCnt; i++)
	{
		sprintf(acItem, "IVPTLRegion%d=%d:%d:", i,
				mivp.st_tl_attr.stTLRegion[i].nIvpCheckMode,
				mivp.st_tl_attr.stTLRegion[i].nCnt);
		nSize += strlen(acItem);
		strcat(pData, acItem);
		for (j = 0; j < mivp.st_tl_attr.stTLRegion[i].nCnt; j++)
		{
			sprintf(acItem, "%d,%d-", mivp.st_tl_attr.stTLRegion[i].stPoints[j].x,
					mivp.st_tl_attr.stTLRegion[i].stPoints[j].y);
			nSize += strlen(acItem);
			strcat(pData, acItem);
		}
		pData[nSize - 1] = ';';
	}

	sprintf(acItem, "IVPTLRecEnable=%d;", mivp.st_tl_attr.stTLAlarmOut.bEnableRecord);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLOutAlarm1=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutAlarm1);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLOutClient=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutClient);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLOutEMail=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutEMail);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLOutVMS=%d;", mivp.st_tl_attr.stTLAlarmOut.bOutVMS);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLSen=%d;", mivp.st_tl_attr.nTLSen);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLAlarmDuration=%d;", mivp.st_tl_attr.nTLAlarmDuration);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "IVPTLSuspectTime=%d;", mivp.st_tl_attr.nTLSuspectTime);
	nSize += strlen(acItem);
	strcat(pData, acItem);

//	printf("build_ivp_param %s \n",pData);

	return nSize;
}

static void _ivp_vr_string2param(char *pData, MIVP_t *mivp)
{
	char key[30];
	char value[128];
	int i= 0;
	mivp->bNeedRestart = FALSE;
	while (*pData != '\0')
	{
		pData = sysfunc_get_key_value(pData, key, value, 128);
		if (strncmp(key, "bVREnable", 10) == 0)
		{
			if (mivp->st_vr_attr.bVREnable != atoi(value))
				mivp->bNeedRestart = TRUE;
			mivp->st_vr_attr.bVREnable = atoi(value);
		}
		else if (strncmp(key, "nVRThreshold", 12) == 0)
		{
			mivp->st_vr_attr.nVRThreshold = atoi(value);
		}
		else if (strncmp(key, "IVPSnapRes", 12) == 0)
		{
			strcpy(mivp->sSnapRes, value);
		}
		else if (strncmp(key, "nVRSen", 6) == 0)
		{
			mivp->st_vr_attr.nSen = atoi(value);
		}
	}
}
U32 build_ivp_vr_param(int chn, char *pData)
{
	U32 j, nSize = 0;
	char acItem[256] ={ 0 };
	MIVP_t mivp;
	mivp_get_param(chn,&mivp);

	sprintf(acItem, "bVREnable=%d;", mivp.st_vr_attr.bVREnable);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "nVRThreshold=%d;", mivp.st_vr_attr.nVRThreshold);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "nVariationRate=%d;", mivp.st_vr_attr.nVariationRate);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	sprintf(acItem, "nVRSen=%d;", mivp.st_vr_attr.nSen);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	
	sprintf(acItem, "IVPSnapRes=%s;", mivp.sSnapRes);
	nSize += strlen(acItem);
	strcat(pData, acItem);

	sprintf(acItem, "stIVPVRRegion=%d:", mivp.st_vr_attr.stVrRegion.cnt);
	nSize += strlen(acItem);
	strcat(pData, acItem);
	for (j = 0; j < mivp.st_vr_attr.stVrRegion.cnt; j++)
	{
		sprintf(acItem, "%d-%d-%d,", mivp.st_vr_attr.stVrRegion.stSegment[j].row,
				mivp.st_vr_attr.stVrRegion.stSegment[j].start,mivp.st_vr_attr.stVrRegion.stSegment[j].end);
		nSize += strlen(acItem);
		strcat(pData, acItem);
	}
	pData[nSize - 1] = ';';

	//printf("build_ivp_vr_param:%s\n",pData);
	return nSize;
}

VOID IVPProc(REMOTECFG *cfg)
{
	int i;
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_IVP;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;
	stSnapSize size;
	size.nWith = 640;
	size.nHeight = 480;
	MIVP_t mivp;

	switch (pstEx->nType)
	{
	case EX_IVP_REFRESH:
	{
		printf("EX_IVP_REFRESH\n");
		pstEx->nType = EX_IVP_REFRESH;
		pstEx->nParam1 = msnapshot_get_data(0, pstEx->acData,RC_DATA_SIZE-16,&size);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20 + pstEx->nParam1);
		break;
	}
	case EX_IVP_UPDATE:
	{
		printf("EX_IVP_UPDATE\n");
		pstEx->nType = EX_IVP_UPDATE;
		pstEx->nParam1 = msnapshot_get_data(0, pstEx->acData,RC_DATA_SIZE-16,&size);
		pstEx->nParam2 = build_ivp_param(0,
				(char *) pstEx->acData + pstEx->nParam1);
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket,
				20 + pstEx->nParam1 + pstEx->nParam2);

		break;
	}
	case EX_IVP_SUBMIT:
	{
		printf("EX_IVP_SUBMIT \n");
		mivp_get_param(0,&mivp);
		_ivp_string2param((char *)pstEx->acData,&mivp);
		mivp_set_param(0,&mivp);

		if(mivp.bNeedRestart)
			mivp_restart(0);
		else
		{
			mivp_SetSensitivity(0, mivp.st_rl_attr.nSen);
			mivp_SetThreshold(0, mivp.st_rl_attr.nThreshold);
			mivp_SetStaytime(0, mivp.st_rl_attr.nStayTime);
		}
		WriteConfigInfo();
		pstEx->nType = EX_IVP_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	}
	case EX_IVP_DETECT_SUBMIT:
	{
		printf("EX_IVP_DETECT_SUBMIT acData=%s\n", pstEx->acData);
		mivp_get_param(0,&mivp);
		_ivp_string2param((char *)pstEx->acData,&mivp);
		if(mivp.st_detect_attr.bDetectEn == TRUE)
		{
			MIVP_t mivp_count;
			mivp_count_get(0, &mivp_count);
			if(mivp_count.st_count_attr.bOpenCount == TRUE)
			{
				BOOL osd_flag = FALSE;
				mivp_count.st_count_attr.bOpenCount = FALSE;
				mivp_count.st_count_attr.bDrawFrame = FALSE;
				if(mivp_count.st_count_attr.bShowCount == TRUE)
				{
					osd_flag = TRUE;
					mivp_count.st_count_attr.bShowCount = FALSE;
				}
				mivp_count_set(0, &mivp_count);
				if(osd_flag == TRUE)
				{
					mchnosd_flush(0);
					mchnosd_flush(1);
					mchnosd_flush(2);
				}
			}
			if(mivp.st_hide_attr.bHideEnable == TRUE)
				mivp.st_hide_attr.bHideEnable = FALSE;
			if(mivp.st_rl_attr.bEnable == TRUE)
				mivp.st_rl_attr.bEnable = FALSE;
			if(mivp.st_vr_attr.bVREnable == TRUE)
				mivp.st_vr_attr.bVREnable = FALSE;
			if(mivp.st_tl_attr.bTLEnable == TRUE)
				mivp.st_tl_attr.bTLEnable = FALSE;
		}
		mivp_set_param(0,&mivp);
		if(mivp.bNeedRestart)
			mivp_restart(0);
		WriteConfigInfo();
		pstEx->nType = EX_IVP_DETECT_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	}
	case EX_IVP_COUNT:
	{
		printf("EX_IVP_COUNT acData=%s\n", pstEx->acData);
		MIVP_t mivp_count;
		mivp_count_get(0, &mivp_count);
		_ivp_string2param((char *)pstEx->acData,&mivp_count);
		mivp_count_set(0, &mivp_count);
		mchnosd_flush(0);
		mchnosd_flush(1);
		mchnosd_flush(2);
		mivp_restart(0);
		WriteConfigInfo();
		break;
	}
	case EX_IVP_COUNT_RESET:
	{
		mivp_count_reset(0);
		break;
	}
	case EX_IVP_VR_REFRESH:
	{
		break;
	}
	case EX_IVP_VR_UPDATE:
	{
		break;
	}
	case EX_IVP_VR_TRIGER:
	{
		break;
	}
	case EX_IVP_VR_SUBMIT:
	{
		break;
	}
	case EX_IVP_VR_REF:
	{
		break;
	}
	case EX_IVP_HIDE_SUBMIT:
	{
		printf("EX_IVP_HIDE_SUBMIT acData=%s\n", pstEx->acData);
		mivp_get_param(0, &mivp);
		_ivp_string2param((char *) pstEx->acData, &mivp);
		mivp_set_param(0, &mivp);
		if (mivp.bNeedRestart)
			mivp_restart(0);
		WriteConfigInfo();
		pstEx->nType = EX_IVP_HIDE_SUBMIT;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
				(U8*) &cfg->stPacket, 20);
		break;
	}
	case EX_IVP_TL_SUBMIT:
	{
		//printf("EX_IVP_TL_SUBMIT acData=%s\n", pstEx->acData);
		mivp_get_param(0, &mivp);
		_ivp_string2param((char *) pstEx->acData, &mivp);
		mivp_set_param(0, &mivp);
		if (mivp.bNeedRestart)
			mivp_restart(0);
		else
			mivp_tl_flush(0);
		WriteConfigInfo();
		break;
	}
	default:
		break;
	}
}

U32 build_chat_param(char *pData)
{
	U32 nSize = 0;
	char acItem[256] = {0};
	sprintf(acItem,"chatMode=%d;",hwinfo.remoteAudioMode);
	nSize += strlen(acItem);
	strcat(pData,acItem);

	return nSize;
}

VOID ChatProc(REMOTECFG* cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_CHAT;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;
	switch (pstEx->nType)
	{
		case EX_CHAT_REFRESH:
			Printf("EX_CHAT_REFRESH\n");
			pstEx->nParam1 = 0;
			memset(pstEx->acData, 0, RC_DATA_SIZE - 16);
			pstEx->nParam2 = build_chat_param((char *)pstEx->acData);
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
					20 + pstEx->nParam2);
			break;
			
		case EX_CHAT_SUBMIT:
			break;

		default:
			break;
	}

}

U32 build_cloud_param(char *pData)
{
	char acItem[256] = {0};
	U32 nSize  = 0;
	CLOUD cloud;
	mcloud_get_param(&cloud);
	sprintf(acItem, "bCloudEnable=%d;", cloud.bEnable);
	nSize += strlen(acItem);
	strcat(pData,acItem);
	
	sprintf(acItem, "CloudID=%s;", cloud.acID);
	nSize += strlen(acItem);
	strcat(pData,acItem);
	
	sprintf(acItem, "CloudPwd=%s;", cloud.acPwd);
	nSize += strlen(acItem);
	strcat(pData,acItem);

	sprintf(acItem, "CloudHost=%s;", cloud.host);
	nSize += strlen(acItem);
	strcat(pData,acItem);

	sprintf(acItem, "CloudBkt=%s;", cloud.bucket);
	nSize += strlen(acItem);
	strcat(pData,acItem);

	sprintf(acItem, "CloudType=%d;", cloud.type);
	nSize += strlen(acItem);
	strcat(pData,acItem);

	sprintf(acItem, "CloudExp=%s;", cloud.expireTime);
	nSize += strlen(acItem);
	strcat(pData,acItem);

	return nSize;
}

VOID CloudProc(REMOTECFG* cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_CLOUD;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;
	switch (pstEx->nType)
	{
		case EX_CLOUD_REFRESH:
			Printf("EX_CHAT_REFRESH\n");
			pstEx->nParam1 = 0;
			memset(pstEx->acData, 0, RC_DATA_SIZE - 16);
			pstEx->nParam2 = build_chat_param((char *)pstEx->acData);
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
					20 + pstEx->nParam2);
			break;
			
		case EX_CLOUD_SUBMIT:
			break;

		default:
			break;
	}	
}

VOID StreamProc(REMOTECFG *cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_STREAM;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;

	mstream_attr_t stAttr;
	mstream_get_param(0, &stAttr);
	char temp[32] = {0};
	BOOL bLDCEnable = FALSE;
	
	switch (pstEx->nType)
	{
		case EX_STREAM_REFRESH:
			Printf("EX_STREAM_REFRESH\n");
			pstEx->nParam1 = 0;
			memset(pstEx->acData, 0, RC_DATA_SIZE - 16);
			pstEx->nParam2 = build_stream_param((char *)pstEx->acData);
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
					20 + pstEx->nParam2);
			break;

		case EX_STREAM_SUBMIT:

			if(GetKeyValue((char *)pstEx->acData, "bLDCEnable", temp, sizeof(temp)))
				bLDCEnable = atoi(temp);

			if(bLDCEnable != stAttr.bLDCEnable)
			{
				stAttr.bLDCEnable = bLDCEnable;
				mstream_set_param(0,&stAttr);
				WriteConfigInfo();
			}

			break;
			
		default:
			break;
	}
}

typedef struct
{
	unsigned int starttime;		//录像开始时间
	unsigned int endingtime;		//录像结束时间
	char  videoType;				//录像类型
	char  chFileName[64];			//录像文件名称
}STR_REMOTE_VIDEO;

static BOOL s_bstart = FALSE;
static BOOL s_bAudioStart = FALSE;

static BOOL s_bhistoryStart = FALSE;
static BOOL s_bHistoryAudioStart = FALSE;

static BOOL s_bNextVideoFlag = FALSE;

static BOOL s_breplaying = FALSE;

static unsigned long long s_base_history_time = 0;



static void Remote_Player_Time_list(unsigned int last_time, int max_count, int *count, STR_REMOTE_VIDEO * range_list)
{
	Printf("last_time = %d,max_count = %d \n",last_time,max_count);
    char strT[7]={0};
	char strPath[128]={0};
	char strFolder[10]={0};
	U8	TypePlay;		//搜索到的录像文件类型
	U32	i, ChnPlay;	//搜索到的录像文件通道
	BOOL bFindLink=FALSE;

	char chPathTime[16] = {0};
	char chFilePath[64] = {0};
	char chStartTime[32] = {0};
	
	struct tm tm1;	
	time_t time1;  

	*count=0;
	
	localtime_r((time_t*)(&last_time),&tm1);
	sprintf(chPathTime,"%4.4d%2.2d%2.2d",tm1.tm_year + 1900, tm1.tm_mon + 1,tm1.tm_mday);
	Printf("get pathtime : %s \n",chPathTime);
	
	sprintf(strPath, "%s", MOUNT_PATH);
	if(access(strPath, F_OK))
	{
		Printf("Path:%s can't access\n", strPath);
		return;
	}
	memset(strFolder, 0, 10);
	strncat(strFolder, chPathTime, 8);
	strcat(strPath, strFolder);
	
	if(access(strPath, F_OK))
	{
		Printf("Path:%s can't access\n", strPath);
		return;
	}
	DIR	*pDir	= opendir(strPath);
	struct dirent *pDirent	= NULL;
	while((pDirent=readdir(pDir)) != NULL)
	{
		//在这里限制搜索类型和通道
		if(!strcmp(FILE_FLAG, pDirent->d_name+strlen(pDirent->d_name)-strlen(FILE_FLAG)))
		{
			sscanf(pDirent->d_name, "%c%2d%6s", &TypePlay, &ChnPlay, strT);
			sprintf(chStartTime,"%s%s",chPathTime,strT);
			sscanf(chStartTime, "%4d%2d%2d%2d%2d%2d",      
			      &tm1.tm_year,   
			      &tm1.tm_mon,   
			      &tm1.tm_mday,   
			      &tm1.tm_hour,   
			      &tm1.tm_min,  
			      &tm1.tm_sec);  

			tm1.tm_year -= 1900;  
			tm1.tm_mon --;  
			tm1.tm_isdst=-1;  
			time1 = mktime(&tm1);

			sprintf(chFilePath,"%s/%s",strPath,pDirent->d_name);
			struct stat statbuf;
			stat(chFilePath,&statbuf);

			if (max_count > *count)
			{
				Printf("index:%d,starttime:%d,endtime:%d,videoType:%d,filename:%s\n",
					*count,(int)time1,(int)statbuf.st_mtime,TypePlay,chFilePath);
				range_list[*count].starttime = time1;
				range_list[*count].endingtime = statbuf.st_mtime;
				range_list[*count].videoType  = TypePlay;
				strcpy(range_list[*count].chFileName, chFilePath);
				*count += 1;
			}
			else
			{
				break;
			}
			
		}
	}
	closedir(pDir);
	
	return;
}

VOID PlayRemoteProc(REMOTECFG* cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_PLAY_REMOTE;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;
	int num = 0;
	switch (pstEx->nType)
	{
		case EX_REPLAY_GETLIST:
#if 0
			/* 为手机兼容方便,使用回调方式,废弃该方式 */
			Remote_Player_Time_list(pstEx->nParam1,pstEx->nParam2,&num,(STR_REMOTE_VIDEO*)pstEx->acData);
			pstEx->nParam1 = num;
			pstEx->nParam2 = num * sizeof(STR_REMOTE_VIDEO);
			pstEx->nParam3 = 0;
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID,
					JVN_RSP_TEXTDATA, (U8*) &cfg->stPacket,
					20 + pstEx->nParam2);
#endif
			break;

		case EX_REPLAY_START:
			break;

		default:
			break;
	}

	
}

VOID RestrictionProc(REMOTECFG *cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_RESTRICTION;
	EXTEND *pstEx = (EXTEND*) cfg->stPacket.acData;
	ipcinfo_t ipcinfo;
	//if(ipcinfo_get_type() == IPCTYPE_SW)
	//	pstEx->nType = EX_RESTRICTION_REP;
	//else
	{
//printf("RestrictionProc %d\n",pstEx->nType);
		if (0 == access(RESTRICTION_FILE, F_OK))
		{
			if(pstEx->nType == EX_RESTRICTION_GET || pstEx->nType == EX_RESTRICTION_SET)
				pstEx->nType = EX_RESTRICTION_REP;
			else if(pstEx->nType == EX_RESTRICTION_REV)
			{
				char cmd[64];
				sprintf(cmd, "rm %s", RESTRICTION_FILE);
				utl_system(cmd);
				ipcinfo_get_param(&ipcinfo);
				ipcinfo.bRestriction = TRUE;
				ipcinfo_set_param(&ipcinfo);
				WriteConfigInfo();
				pstEx->nType = EX_RESTRICTION_REVOK;
			}
		}
		else
		{
			if(pstEx->nType == EX_RESTRICTION_GET)
			{
				pstEx->nType = EX_RESTRICTION_GETOK;
			}
			else if(pstEx->nType == EX_RESTRICTION_SET)
			{
				char cmd[64];
				sprintf(cmd, "touch %s", RESTRICTION_FILE);
				utl_system(cmd);
				ipcinfo_get_param(&ipcinfo);
				ipcinfo.bRestriction = FALSE;
				ipcinfo_set_param(&ipcinfo);
				WriteConfigInfo();
				pstEx->nType = EX_RESTRICTION_SETOK;
			}
			else
				pstEx->nType = EX_RESTRICTION_REVFAILED;
		}
	}
	MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
			(U8*) &cfg->stPacket, 20);
}

VOID ReqIDRProc(REMOTECFG *cfg)
{
	cfg->stPacket.nPacketType = RC_EXTEND;
	cfg->stPacket.nPacketCount = RC_EX_REQ_IDR;
	int channelid = cfg->stPacket.nPacketID;
	printf("ReqIDRProc channelid=%d\n", channelid);
	mstream_request_idr(channelid);
	MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
			(U8*) &cfg->stPacket, 20);
}

VOID DebugProc(REMOTECFG *cfg)
{
	EXTEND *pstDebugExt = (EXTEND*) (cfg->stPacket.acData);
	switch(pstDebugExt->nType)
	{
	case EX_DEBUG_TELNET:
		printf("====>>debug: start telnetd\n");
		mdebug_starttelnetd(7001);
		break;
	case EX_DEBUG_TELNET_OFF:
		printf("====>>debug: stop telnetd\n");
		mdebug_stoptelnetd();
		break;
	case EX_DEBUG_REDIRECTPRINT:
		printf("====>>redirect print to %s\n", mdebug_get_redirect_logfilename());
		mdebug_redirectprintf();
		break;
	case EX_DEBUG_RECOVERPRINT:
		printf("====>>recover print\n");
		mdebug_recoverprintf();
		break;
	case EX_DEBUG_GETLOG:
		pstDebugExt->nParam3 = mdebug_getlatestlog((char*)pstDebugExt->acData, 128 * 1024);
		if (pstDebugExt->nParam3 != -1)
		{
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,
					(U8*) &cfg->stPacket, 20 + pstDebugExt->nParam3);
		}
		break;
	default:
		break;
	}
}

