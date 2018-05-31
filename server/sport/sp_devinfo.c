#include <time.h>
#include "sp_define.h"
#include "sp_devinfo.h"

#include <jv_common.h>
#include "mipcinfo.h"
#include "SYSFuncs.h"
#include <utl_iconv.h>
#include "sctrl.h"
#include "mioctrl.h"

static SPHWInfo_t sHWInfo = {
		"IPC",		//char type[32]; //类型 DEVICE,IPC,NVR,DVR
		"1.0",		//char hardware[32];	//硬件名称
		"112233",		//char sn[32];		//SN
		"1.0",		//char firmware[32];	//软件版本
		"JOVISION",		//char manufacture[32]; //生产厂商
		"H200",		//char model[32];

		1,		//int ptzBsupport;//是否支持PTZ
		2,		//int streamCnt;	//编码个数。原来都是3路。现在有些方案，只有2路
};
const SPHWInfo_t *sp_dev_get_hwinfo()
{
	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	SYSFuncs_GetValue(CONFIG_FILE, "Version", sHWInfo.firmware, sizeof(sHWInfo.firmware));
	strcpy(sHWInfo.manufacture, "JOVISION");
	strcpy(sHWInfo.hardware, ipcinfo.product);
	char *p = strstr(sHWInfo.hardware, "JVS-");
	if (p)
	{
		p[0] = 'N';
		p[1] = 'V';
		p[2] = 'T';
	}

	if (gp.bNeedYST == FALSE)
	{
		//没有云视通的IPC，NVR通讯使用onvif
		strcpy(sHWInfo.model, "SOOVVI-IPC");
	}
	else
	{
		strcpy(sHWInfo.model, "JOVISION-IPC");
	}
	strcpy(sHWInfo.type, hwinfo.type);
	sHWInfo.ptzBsupport = hwinfo.ptzBsupport;
	sHWInfo.streamCnt = hwinfo.streamCnt;
	sprintf(sHWInfo.sn,"%d", ipcinfo.sn);
	jv_ystNum_parse(sHWInfo.ystID,  ipcinfo.nDeviceInfo[6],ipcinfo.ystID);
	return &sHWInfo;
}

int sp_dev_get_info(SPDevInfo_t *info)
{
	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	info->rebootDay = ipcinfo.rebootDay;
	info->rebootHour= ipcinfo.rebootHour;

	utl_iconv_gb2312toutf8(ipcinfo.acDevName, info->name, sizeof(ipcinfo.acDevName));
//	strcpy(info->name, ipcinfo.acDevName);

	return 0;
}

int sp_dev_set_info(SPDevInfo_t *info)
{
	ipcinfo_t ipcinfo;
//	int i;
	ipcinfo_get_param(&ipcinfo);
	utl_iconv_utf8togb2312(info->name,ipcinfo.acDevName,sizeof(info->name));
//	strcpy(ipcinfo.acDevName, info->name);
	ipcinfo.acDevName[31] = '\0';
	ipcinfo.rebootDay = info->rebootDay;
	ipcinfo.rebootHour= info->rebootHour;
	ipcinfo_set_param(&ipcinfo);
	WriteConfigInfo();
	return 0;
}

/**
 *@brief 设置当前时间
 *
 *@param tmsec 当前时间，距离1970年以来的秒数
 */
int sp_dev_stime(time_t *tmsec)
{
	ipcinfo_set_time_ex(tmsec);
	return 0;
}

/**
 *@brief 获取当前时间
 *
 *@return 当前时间，距离1970年以来的秒数
 */
time_t sp_dev_gtime()
{
	return time(NULL);
}


int sp_dev_ntp_set(SPNtp_t *ntp)
{
	ipcinfo_t info;
	ipcinfo_get_param(&info);
	info.bSntp = ntp->bNtp;
	info.sntpInterval= ntp->sntpInterval;
	strncpy(info.ntpServer, ntp->ntpServer, sizeof(info.ntpServer));
	ipcinfo_set_param(&info);
	WriteConfigInfo();
	return 0;
}

int sp_dev_ntp_get(SPNtp_t *ntp)
{
	ipcinfo_t info;
	ipcinfo_get_param(&info);
	ntp->bNtp = info.bSntp ;
	ntp->sntpInterval = info.sntpInterval;
	strncpy(ntp->ntpServer, info.ntpServer, sizeof(ntp->ntpServer));
	return 0;
}

int sp_dev_timezone_set(int tz)
{
	ipcinfo_t info;
	ipcinfo_get_param(&info);
	info.tz = tz;
	ipcinfo_set_param(&info);
	WriteConfigInfo();
	return 0;
}

int sp_dev_timezone_get(void)
{
	ipcinfo_t info;
	ipcinfo_get_param(&info);
	return info.tz;
}

/**
 *@brief 重启
 *
 */
int sp_dev_reboot(int delaymSec)
{
	__FUNC_DBG__();
	SYSFuncs_reboot();
	return 0;
}

int sp_dev_factory_default(BOOL bHard)
{
	SYSFuncs_factory_default(bHard);
	return 0;
}

int sp_dev_discovermode_get(void)
{
	return 1;
}
int sp_dev_discovermode_set(int bEnable)
{
	return 0;
}

int sp_dev_hostname_set(const char* hostname,int len)
{
	if(hostname)
	{
		sethostname(hostname,len);

		//strncpy(g_devInfo.hostname,hostname,32);
	}
	return 0;
}

int sp_dev_LetCtrl(int mode,int link)
{
	return 0;
}

