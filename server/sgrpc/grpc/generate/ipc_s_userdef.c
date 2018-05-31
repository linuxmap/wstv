//Need user to modify this file.
#include "ipc.h"

#include "sp_define.h"
#include "sp_devinfo.h"
#include "sp_alarm.h"
#include "sp_audio.h"
#include "sp_connect.h"
#include "sp_ifconfig.h"
#include "sp_image.h"
#include "sp_mdetect.h"
#include "sp_osd.h"
#include "sp_privacy.h"
#include "sp_ptz.h"
#include "sp_stream.h"
#include "sp_storage.h"
#include "sp_snapshot.h"
#include "sp_user.h"
#include "sp_log.h"
#include "sp_ivp.h"
#include "sp_firmup.h"
#include "m_rtmp.h"
#include "mcloud.h"
#include "mlog.h"
// #include "mivp.h"
#include <utl_iconv.h>
#include <utl_filecfg.h>
#include <cJSON.h>
// #include <mprerec_upload.h>
#include "alarm_service.h"


int USERDEF_ipc_log_get(grpc_t *grpc, PARAM_REQ_ipc_log_get *req, PARAM_RESP_ipc_log_get *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	//int nYear, nMonth, nDay;
	char date[16];
	char time[64];
	char *pYear = NULL;
	char *pMonth = NULL;
	char *pDay = NULL;
	SPLOG log;
	int ret = 0;
	int i = 0;
	int endIndex = 0;
	struct tm tmDate;
	int log_items_cnt = 0;
	
	memset(&log, 0, sizeof(SPLOG));
	
	if(req->date == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid date!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->type == NULL)
	{
		req->type = "";
	}
	strncpy(date, req->date, 16);
	
	pYear = strtok(date, "-");
	if(pYear == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid date!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	//nYear = atoi(pYear);
	
	pMonth = strtok(NULL, "-");
	if(pMonth == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid date!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	//nMonth = atoi(pMonth);
	
	pDay = strtok(NULL, "-");
	if(pDay == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid date!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	//nDay = atoi(pDay);
		
	ret = sp_log_get(&log);
	/*	
	for(i=0; i<log.head.nRecordTotal; i++)
	{
		localtime_r((time_t *)&log.item[i].nTime, &tmDate);
		if(tmDate.tm_year+1900 == nYear && tmDate.tm_mon+1 == nMonth && tmDate.tm_mday == nDay)
		{
			log_items_cnt++;
			endIndex = i;
		}
	}
	*/
	log_items_cnt = log.head.nRecordTotal;
	endIndex = log.head.nRecordTotal-1;
	resp->log_pages_cnt = (log_items_cnt+99)/100;
	if(req->page > resp->log_pages_cnt || req->page <= 0)
		req->page = 1;
	int startIndex = endIndex-log_items_cnt+1;//
	int realEndIndex = startIndex+100;//
	int p = log.head.nCurRecord - (req->page-1)*100;
	if(p < 0)
		p += SPLOG_MAX_RECORD;
	startIndex = endIndex-log_items_cnt+1+(req->page-1)*100;
	if(resp->log_pages_cnt == req->page)
		realEndIndex = endIndex;
	else
		realEndIndex = startIndex+100;
	resp->log_items_cnt = realEndIndex-startIndex;
	printf("resp->log_items_cnt=%d\n", resp->log_items_cnt);
	resp->log_items = grpc_malloc(grpc, resp->log_items_cnt*sizeof(*resp->log_items));
	if(resp->log_items == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=startIndex; i<realEndIndex; i++,p--)
	{
		if(p < 0)
			p += SPLOG_MAX_RECORD;
		if(strcmp(log.item[p].strLog,"\0") == 0)
			continue;
		localtime_r((time_t *)&log.item[p].nTime, &tmDate);
		sprintf(time, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", tmDate.tm_year+1900,tmDate.tm_mon+1, tmDate.tm_mday, tmDate.tm_hour, tmDate.tm_min, tmDate.tm_sec);
		resp->log_items[i-startIndex].time = grpc_strdup(grpc, time);
		if(log.item[p].strLog != NULL)
		{
			char logUTF8[128];
			memset(logUTF8, 0, sizeof(logUTF8));
			utl_iconv_gb2312toutf8(log.item[p].strLog, logUTF8, sizeof(logUTF8)-1);
			resp->log_items[i-startIndex].strlog = grpc_strdup(grpc, logUTF8);
		}
		resp->log_items[i-startIndex].type = NULL;//grpc_strdup(grpc, req->type);
		resp->log_items[i-startIndex].username = NULL;//grpc_strdup(grpc, "system");
		resp->log_items[i-startIndex].nSub = 0;
		resp->log_items[i-startIndex].bNetuser = FALSE;
		resp->log_items[i-startIndex].bmain = FALSE;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_log_clear(grpc_t *grpc, PARAM_REQ_ipc_log_clear *req, PARAM_RESP_ipc_log_clear *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__NULL_FUNC_DBG__();
	int cnt = 1;
	int i;
	resp->users_cnt = cnt;
	resp->users = grpc_malloc(grpc, cnt * sizeof(*resp->users));
	for (i=0;i<cnt;i++)
	{
		resp->users[i].name = grpc_strdup(grpc, "username");
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_get_hwinfo(grpc_t *grpc, PARAM_REQ_ipc_dev_get_hwinfo *req, PARAM_RESP_ipc_dev_get_hwinfo *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	
	const SPHWInfo_t *ipcinfo = sp_dev_get_hwinfo();

	if(ipcinfo != NULL)
	{
		resp->type = grpc_strdup(grpc, ipcinfo->type);
		resp->hardware = grpc_strdup(grpc, ipcinfo->hardware);
		resp->firmware = grpc_strdup(grpc, ipcinfo->firmware);
		resp->sn= grpc_strdup(grpc, ipcinfo->sn);
		resp->ystID= grpc_strdup(grpc, ipcinfo->ystID);
		resp->manufacture = grpc_strdup(grpc, ipcinfo->manufacture);
		resp->model = grpc_strdup(grpc, ipcinfo->model);
		resp->bPtzSupport = ipcinfo->ptzBsupport;
		resp->channelCnt= 1;
		resp->streamCnt = ipcinfo->streamCnt;
	}
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get hwinfo failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_get_info(grpc_t *grpc, PARAM_REQ_ipc_dev_get_info *req, PARAM_RESP_ipc_dev_get_info *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	SPDevInfo_t info;
	int ret = 0;
	char tmpbuf[32];
	
	memset(&info, 0, sizeof(SPDevInfo_t));
	memset(&tmpbuf, 0, sizeof(tmpbuf));
	
	ret = sp_dev_get_info(&info);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get device info failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->bDiscoverable = info.bDiscoverable;
	resp->hostname = grpc_strdup(grpc, info.hostname);
	utl_iconv_utf8togb2312(info.name, tmpbuf, sizeof(tmpbuf));
	resp->name = grpc_strdup(grpc, tmpbuf);
	if(info.rebootDay == SP_REBOOT_TIMER_NEVER)
		resp->rebootDay = "never";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYDAY)
		resp->rebootDay = "everyday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYMOUNDAY)
		resp->rebootDay = "everymonday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYTUNESDAY)
		resp->rebootDay = "everytuesday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYWEDNESDAY)
		resp->rebootDay = "everywednesday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYTHURSDAY)
		resp->rebootDay = "everythursday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYFRIDAY)
		resp->rebootDay = "everyfriday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYSATURDAY)
		resp->rebootDay = "everysaturday";
	else if(info.rebootDay == SP_REBOOT_TIMER_EVERYSUNDAY)
		resp->rebootDay = "everysunday";
	else
		resp->rebootDay = NULL;
	resp->rebootHour= info.rebootHour;
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_set_info(grpc_t *grpc, PARAM_REQ_ipc_dev_set_info *req, PARAM_RESP_ipc_dev_set_info *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPDevInfo_t info;
	int ret = 0;
	
	memset(&info, 0, sizeof(SPDevInfo_t));
	
	ret = sp_dev_get_info(&info);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get device info failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	if(strchr(req->name, ';') != NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "';' is illegal!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(strlen(req->name) > 16)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Device name too long!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	info.bDiscoverable = req->bDiscoverable;
	if(req->hostname != NULL)
		strncpy(info.hostname, req->hostname, 32);
	if(req->hostname != NULL)
		strncpy(info.name, req->name, 32);
	if(req->rebootDay != NULL)
	{
		if(strcmp(req->rebootDay, "never") == 0)
			info.rebootDay = SP_REBOOT_TIMER_NEVER;
		else if(strcmp(req->rebootDay, "everyday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYDAY;
		else if(strcmp(req->rebootDay, "everymonday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYMOUNDAY;
		else if(strcmp(req->rebootDay, "everytuesday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYTUNESDAY;
		else if(strcmp(req->rebootDay, "everywednesday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYWEDNESDAY;
		else if(strcmp(req->rebootDay, "everythursday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYTHURSDAY;
		else if(strcmp(req->rebootDay, "everyfriday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYFRIDAY;
		else if(strcmp(req->rebootDay, "everysaturday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYSATURDAY;
		else if(strcmp(req->rebootDay, "everysunday") == 0)
			info.rebootDay = SP_REBOOT_TIMER_EVERYSUNDAY;
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid reboot timer param!");
			return GRPC_ERR_INVALID_PARAMS;
		}
			
	}	
	info.rebootHour= req->rebootHour;
	ret = sp_dev_set_info(&info);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set device info failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_stime(grpc_t *grpc, PARAM_REQ_ipc_dev_stime *req, PARAM_RESP_ipc_dev_stime *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;
	
	ret = sp_dev_stime((time_t *)(&req->tmsec));
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set device time failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	if(req->tz != NULL)
	{
		int timezone = atoi(req->tz);
		if(timezone < -12 || timezone > 13)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid timezone param!");
			return GRPC_ERR_INVALID_PARAMS;
		}
		ret = sp_dev_timezone_set(timezone);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set device timezone failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_gtime(grpc_t *grpc, PARAM_REQ_ipc_dev_gtime *req, PARAM_RESP_ipc_dev_gtime *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	char tmpBuf[32];
	//int ret = 0;
	memset(tmpBuf, 0, sizeof(tmpBuf));
	
	sprintf(tmpBuf, "%d", sp_dev_timezone_get());
	resp->tz = grpc_strdup(grpc, tmpBuf);
	resp->tmsec = sp_dev_gtime();

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_ntp_set(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_set *req, PARAM_RESP_ipc_dev_ntp_set *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPNtp_t ntp;
	int ret = 0;
	
	memset(&ntp, 0, sizeof(SPNtp_t));

	if(req->sntpInterval < 0 || req->sntpInterval > 200)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid NTP Interval!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ntp.bNtp = req->bEnableNtp;
	ntp.sntpInterval = req->sntpInterval;
	if(req->servers != NULL)
	{
		if(req->servers[0] != NULL)
			strncpy(ntp.ntpServer, req->servers[0], 256);
	}
	ret = sp_dev_ntp_set(&ntp);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set device ntp failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_ntp_get(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_get *req, PARAM_RESP_ipc_dev_ntp_get *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int i = 0;
	int ret = 0;
	SPNtp_t ntp;

	memset(&ntp, 0, sizeof(SPNtp_t));
	
	ret = sp_dev_ntp_get(&ntp);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get device ntp failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	resp->bEnableNtp= ntp.bNtp;
	resp->sntpInterval= ntp.sntpInterval;
	resp->servers_cnt = 1;
	resp->servers = grpc_malloc(grpc, resp->servers_cnt * sizeof(*resp->servers));
	if(resp->servers == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->servers_cnt; i++)
	{
		resp->servers[i] = grpc_strdup(grpc, ntp.ntpServer);
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_reboot(grpc_t *grpc, PARAM_REQ_ipc_dev_reboot *req, PARAM_RESP_ipc_dev_reboot *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->delaymSec < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid delay time!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ret = sp_dev_reboot(req->delaymSec);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Reboot device failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_factory_default(grpc_t *grpc, PARAM_REQ_ipc_dev_factory_default *req, PARAM_RESP_ipc_dev_factory_default *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;
	
	ret = sp_dev_factory_default(req->bHard);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Factory default device failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_dev_update_check(grpc_t *grpc, PARAM_REQ_ipc_dev_update_check *req, PARAM_RESP_ipc_dev_update_check *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	char version[64];
	char phase[16];
	int progress;
	memset(version, 0, sizeof(version));
	memset(phase, 0, sizeof(phase));
	int ret = sp_firmup_update_check(version);
	if(ret < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_TIMEOUT, "Update check failed!");
		return GRPC_ERR_TIMEOUT;		
	}
	resp->bNeedUpdate = ret;
	resp->version = grpc_strdup(grpc, version);
	sp_firmup_update_get_progress(phase, &progress);
	resp->progress = progress;
	resp->phase = grpc_strdup(grpc, phase);
#endif
	return 0;
}

int USERDEF_ipc_dev_update(grpc_t *grpc, PARAM_REQ_ipc_dev_update *req, PARAM_RESP_ipc_dev_update *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->method == NULL || req->url == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Factory default device failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	sp_firmup_update(req->method, req->url);
#endif
	return 0;
}

int USERDEF_ipc_alarmin_start(grpc_t *grpc, PARAM_REQ_ipc_alarmin_start *req, PARAM_RESP_ipc_alarmin_start *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	ret = sp_alarmin_start(req->channelid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Start alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarmin_stop(grpc_t *grpc, PARAM_REQ_ipc_alarmin_stop *req, PARAM_RESP_ipc_alarmin_stop *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	ret = sp_alarmin_stop(req->channelid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Stop alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarmin_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_get_param *req, PARAM_RESP_ipc_alarmin_get_param *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPAlarmIn_t alarm;
	int ret = 0;
	int i = 0;
	
	memset(&alarm, 0, sizeof(SPAlarmIn_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	ret = sp_alarmin_get_param(req->channelid, &alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarmin params failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->nDelay = alarm.nDelay;
	resp->u8AlarmNum = alarm.u8AlarmNum;
	resp->bBuzzing = alarm.bBuzzing;
	resp->bEnable = alarm.bEnable;
	resp->bEnableRecord = alarm.bEnableRecord;
	resp->bNormallyClosed = alarm.bNormallyClosed;
	resp->bSendEmail = alarm.bSendEmail;
	resp->bSendtoClient = alarm.bSendtoClient;
	resp->bStarting = alarm.bStarting;
	resp->nSOS = alarm.nSOS;
	resp->nGuardChn = alarm.nGuardChn;
	resp->type_cnt = sizeof(alarm.type)/sizeof(SPAlarmIn_Type_t);
	resp->type = grpc_malloc(grpc, resp->type_cnt * sizeof(*resp->type));
	if(resp->type == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->type_cnt; i++)
	{
		resp->type[i].type = alarm.type[i].type;
		resp->type[i].bAlarmOut = alarm.type[i].bAlarmOut;
		
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarmin_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_set_param *req, PARAM_RESP_ipc_alarmin_set_param *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPAlarmIn_t alarm;
	int ret = 0;
	int i = 0;
	
	memset(&alarm, 0, sizeof(SPAlarmIn_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	if(req->nDelay < 0 || req->nDelay > 100)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid delay param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ret = sp_alarmin_get_param(req->channelid, &alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarmin params failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	alarm.nDelay = req->nDelay;
	alarm.u8AlarmNum = (unsigned char)req->u8AlarmNum;
	alarm.bBuzzing = req->bBuzzing;
	alarm.bEnable = req->bEnable;
	alarm.bEnableRecord = req->bEnableRecord;
	alarm.bNormallyClosed = req->bNormallyClosed;
	alarm.bSendEmail = req->bSendEmail;
	alarm.bSendtoClient = req->bSendtoClient;
	alarm.bStarting = req->bStarting;
	alarm.nSOS = req->nSOS;
	alarm.nGuardChn = req->nGuardChn;
	int type_cnt = sizeof(alarm.type)/sizeof(SPAlarmIn_Type_t);
	for(i=0; i<(req->type_cnt>type_cnt ? type_cnt : req->type_cnt); i++)
	{
		alarm.type[i].type = req->type[i].type;
		alarm.type[i].bAlarmOut = req->type[i].bAlarmOut;
	}
	
	ret = sp_alarmin_set_param(req->channelid, &alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set alarmin params failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarmin_b_onduty(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_onduty *req, PARAM_RESP_ipc_alarmin_b_onduty *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	resp->bOnduty = sp_alarmin_b_onduty(req->channelid);
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarmin_b_alarming(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_alarming *req, PARAM_RESP_ipc_alarmin_b_alarming *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	resp->bAlarming = sp_alarmin_b_alarming(req->channelid);

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarm_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_param *req, PARAM_RESP_ipc_alarm_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPAlarmSet_t alarm;
	int ret = 0;
	
	memset(&alarm, 0, sizeof(SPAlarmSet_t));
	
	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->delay = alarm.delay;
	resp->port = alarm.port;
	resp->username = grpc_strdup(grpc, alarm.username);
	resp->passwd = grpc_strdup(grpc, alarm.passwd);
	resp->server = grpc_strdup(grpc, alarm.server);
	resp->sender = grpc_strdup(grpc, alarm.sender);
	if(alarm.receiver0[0] != 0)
	{
		resp->receiver_cnt++;
	}
	if(alarm.receiver1[0] != 0)
	{
		resp->receiver_cnt++;
	}
	if(alarm.receiver2[0] != 0)
	{
		resp->receiver_cnt++;
	}
	if(alarm.receiver3[0] != 0)
	{
		resp->receiver_cnt++;
	}
	resp->receiver = grpc_malloc(grpc, resp->receiver_cnt * sizeof(*resp->receiver));
	if(resp->receiver == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	if(alarm.receiver0[0] != 0)
	{
		resp->receiver[0] = grpc_strdup(grpc, alarm.receiver0);
	}
	if(alarm.receiver1[0] != 0)
	{
		resp->receiver[1] = grpc_strdup(grpc, alarm.receiver1);
	}
	if(alarm.receiver2[0] != 0)
	{
		resp->receiver[2] = grpc_strdup(grpc, alarm.receiver2);
	}
	if(alarm.receiver3[0] != 0)
	{
		resp->receiver[3] = grpc_strdup(grpc, alarm.receiver3);
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarm_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_set_param *req, PARAM_RESP_ipc_alarm_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPAlarmSet_t alarm;
	int ret = 0;
//	int i = 0;
	
	memset(&alarm, 0, sizeof(SPAlarmSet_t));

	if(req->username == NULL || req->passwd == NULL || req->sender == NULL || req->server == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid alarm params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	if(req->delay < 0 || req->delay > 100)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid delay param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	alarm.delay = req->delay;
	alarm.port = req->port;
	strncpy(alarm.username, req->username, 64);
	strncpy(alarm.passwd, req->passwd, 64);
	strncpy(alarm.sender, req->sender, 64);
	strncpy(alarm.server, req->server, 64);
	memset(alarm.receiver0, 0, sizeof(alarm.receiver0));
	memset(alarm.receiver1, 0, sizeof(alarm.receiver1));
	memset(alarm.receiver2, 0, sizeof(alarm.receiver2));
	memset(alarm.receiver3, 0, sizeof(alarm.receiver3));
	if(req->receiver != NULL)
	{
		if(req->receiver_cnt == 1)
		{	
			if(req->receiver[0] != NULL)
				strncpy(alarm.receiver0, req->receiver[0], 64);
		}
		else if(req->receiver_cnt == 2)
		{	
			if(req->receiver[0] != NULL)
				strncpy(alarm.receiver0, req->receiver[0], 64);
			if(req->receiver[1] != NULL)
				strncpy(alarm.receiver1, req->receiver[1], 64);
		}
		else if(req->receiver_cnt == 3)
		{	
			if(req->receiver[0] != NULL)
				strncpy(alarm.receiver0, req->receiver[0], 64);
			if(req->receiver[1] != NULL)
				strncpy(alarm.receiver1, req->receiver[1], 64);
			if(req->receiver[2] != NULL)
				strncpy(alarm.receiver2, req->receiver[2], 64);
		}
		else if(req->receiver_cnt == 4)
		{	
			if(req->receiver[0] != NULL)
				strncpy(alarm.receiver0, req->receiver[0], 64);
			if(req->receiver[1] != NULL)
				strncpy(alarm.receiver1, req->receiver[1], 64);
			if(req->receiver[2] != NULL)
				strncpy(alarm.receiver2, req->receiver[2], 64);
			if(req->receiver[3] != NULL)
				strncpy(alarm.receiver3, req->receiver[3], 64);
		}
	}
	ret = sp_alarm_set_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_alarm_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_get *req, PARAM_RESP_ipc_alarm_link_preset_get *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channelid!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	SPAlarmSet_t alarm;
	int ret = 0;
	
	memset(&alarm, 0, sizeof(SPAlarmSet_t));
	
	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	if(alarm.alarmLinkPreset == -1)
	{
		resp->presetno = -1;
	}
	else
	{
		//查找当前预置位，确认报警联动预置位是否存在
		int cnt = sp_ptz_preset_get_cnt(0);
		int i = 0;
		SPPreset_t preset;
		memset(&preset, 0, sizeof(SPPreset_t));
		for(i=0; i<cnt; i++)
		{
			sp_ptz_preset_get(0, i, &preset);
			if(alarm.alarmLinkPreset == preset.presetno)
				break;
		}
		if(i == cnt)
		{
			resp->presetno = -1;
		}
		else
		{
			resp->presetno = alarm.alarmLinkPreset;
		}
	}
#endif
	return 0;	
}

int USERDEF_ipc_alarm_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_set *req, PARAM_RESP_ipc_alarm_link_preset_set *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channelid!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	SPAlarmSet_t alarm;
	int ret = 0;
	
	memset(&alarm, 0, sizeof(SPAlarmSet_t));
	
	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	if(req->presetno == -1)
	{
		alarm.alarmLinkPreset = -1;
	}
	else
	{
		int cnt = sp_ptz_preset_get_cnt(0);
		int i = 0;
		SPPreset_t preset;
		memset(&preset, 0, sizeof(SPPreset_t));
		for(i=0; i<cnt; i++)
		{
			sp_ptz_preset_get(0, i, &preset);
			if(req->presetno == preset.presetno)
				break;
		}
		if(i == cnt)
		{
			printf("preset not exists\n");
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Invalid preset number!");
			return GRPC_ERR_OPERATION_REFUSED;				
		}
		else
		{
			alarm.alarmLinkPreset = preset.presetno;
		}
	}
	ret = sp_alarm_set_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
#endif
	return 0;	
}

int USERDEF_ipc_alarmin_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_get *req, PARAM_RESP_ipc_alarmin_link_preset_get *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channelid!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	SPAlarmSet_t alarm;
	int ret = 0;
	int i = 0;

	memset(&alarm, 0, sizeof(SPAlarmSet_t));

	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->link_cnt = sizeof(alarm.alarminLinkPreset)/(sizeof(alarm.alarminLinkPreset[0]));
	resp->link = grpc_malloc(grpc, resp->link_cnt*sizeof(*resp->link));
	if(resp->link == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->link_cnt; i++)
	{
		resp->link[i].bWireless = FALSE;
		resp->link[i].alarmin = i+1;
		if(alarm.alarminLinkPreset[i] == -1)
		{
			resp->link[i].presetno = -1;
		}
		else
		{
			//查找当前预置位，确认报警联动预置位是否存在
			int cnt = sp_ptz_preset_get_cnt(0);
			int ii = 0;
			SPPreset_t preset;
			memset(&preset, 0, sizeof(SPPreset_t));
			for(ii=0; ii<cnt; ii++)
			{
				sp_ptz_preset_get(0, ii, &preset);
				if(alarm.alarminLinkPreset[i] == preset.presetno)
					break;
			}
			if(ii == cnt)
			{
				resp->link[i].presetno = -1;
			}
			else
			{
				resp->link[i].presetno = alarm.alarminLinkPreset[i];
			}
		}
	}
#endif
	return 0;
	
}

int USERDEF_ipc_alarmin_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_set *req, PARAM_RESP_ipc_alarmin_link_preset_set *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->channelid != 0 || req->link == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channelid!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	SPAlarmSet_t alarm;
	int ret = 0;
	int i = 0;

	memset(&alarm, 0, sizeof(SPAlarmSet_t));

	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	for(i=0; i<req->link_cnt; i++)
	{
		if(!req->link[i].bWireless)
		{
			// 有效值范围1~8
			if ((req->link[i].alarmin < 1) || (req->link[i].alarmin > sizeof(alarm.alarminLinkPreset)/sizeof(alarm.alarminLinkPreset[0])))
			{
				printf("Invalid alarm in channel: %d\n", req->link[i].alarmin);
				grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid alarm in channel!");
				return GRPC_ERR_INVALID_PARAMS;
			}

			//有线报警输入
			if(req->link[i].presetno == -1)
			{
				alarm.alarminLinkPreset[req->link[i].alarmin - 1] = -1;
			}
			else
			{
				int cnt = sp_ptz_preset_get_cnt(0);
				int j = 0;
				SPPreset_t preset;
				memset(&preset, 0, sizeof(SPPreset_t));
				for(j=0; j<cnt; j++)
				{
					sp_ptz_preset_get(0, j, &preset);
					if(req->link[i].presetno == preset.presetno)
						break;
				}
				if(j == cnt)
				{
					printf("preset not exists\n");
					grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Invalid preset number!");
					return GRPC_ERR_OPERATION_REFUSED;				
				}
				else
				{
					alarm.alarminLinkPreset[req->link[i].alarmin - 1] = preset.presetno;
				}
			}
		}
		else
		{
			//无线报警输入
			
		}
	}
	ret = sp_alarm_set_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
#endif
	return 0;
}

int USERDEF_ipc_alarm_link_out_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_get *req, PARAM_RESP_ipc_alarm_link_out_get *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channelid!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	SPAlarmSet_t alarm;
	int ret = 0;
	int i = 0;

	memset(&alarm, 0, sizeof(SPAlarmSet_t));

	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->link_cnt = sizeof(alarm.alarmLinkOut)/(sizeof(alarm.alarmLinkOut[0]));
	resp->link = grpc_malloc(grpc, resp->link_cnt*sizeof(*resp->link));
	if(resp->link == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	memset(resp->link, 0, resp->link_cnt * sizeof(*resp->link));

	for(i=0; i<resp->link_cnt; i++)
	{
		resp->link[i].alarm_type = i;
		int cnt = 0;
		int ii = 0;
		unsigned int tmp = alarm.alarmLinkOut[i];
		for(ii=0; ii<32; ii++)
		{
			if(tmp & 0x00000001)
			{
				resp->link[i].alarm_out_cnt++;
			}
			tmp>>=1;
		}
		resp->link[i].alarm_out = grpc_malloc(grpc, resp->link[i].alarm_out_cnt*sizeof(*resp->link[i].alarm_out));
		if(resp->link[i].alarm_out == NULL)
		{
			grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
			return GRPC_ERR_NO_FREE_MEMORY; 
		}
		memset(resp->link[i].alarm_out, 0, resp->link[i].alarm_out_cnt * sizeof(*resp->link[i].alarm_out));

		int iii = 0;
		tmp = alarm.alarmLinkOut[i];
		for(ii=0; ii<32; ii++)
		{
			if(tmp & 0x00000001)
			{
				resp->link[i].alarm_out[iii++] = ii+1;
			}
			tmp>>=1;
		}
		
	}
#endif
	return 0;
}

int USERDEF_ipc_alarm_link_out_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_set *req, PARAM_RESP_ipc_alarm_link_out_set *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->channelid != 0 || req->link == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channelid!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	SPAlarmSet_t alarm;
	int ret = 0;
	int i = 0;

	memset(&alarm, 0, sizeof(SPAlarmSet_t));

	ret = sp_alarm_get_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	for(i=0; i<req->link_cnt; i++)
	{
		unsigned int tmp = 0;
		int ii = 0;
		for(ii=0; ii<req->link[i].alarm_out_cnt; ii++)
		{
			tmp |= (1<<(req->link[i].alarm_out[ii]-1));
		}
		alarm.alarmLinkOut[req->link[i].alarm_type] = tmp;
	}
	ret = sp_alarm_set_param(&alarm);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set alarm failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
#endif
	return 0;
}

int USERDEF_ipc_alarm_report(grpc_t *grpc, PARAM_REQ_ipc_alarm_report *req, PARAM_RESP_ipc_alarm_report *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
#endif
	return 0;
}

int USERDEF_ipc_login(grpc_t *grpc, PARAM_REQ_ipc_login *req, PARAM_RESP_ipc_login *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
#endif
	return 0;
}

int USERDEF_ipc_keep_online(grpc_t *grpc, PARAM_REQ_ipc_keep_online *req, PARAM_RESP_ipc_keep_online *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
#endif
	return 0;
}

int USERDEF_ipc_get_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_get_streamserver_addr *req, PARAM_RESP_ipc_get_streamserver_addr *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	printf("USERDEF_ipc_get_streamserver_addr\n");
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
#endif
	return 0;
}

int USERDEF_ipc_set_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_set_streamserver_addr *req, PARAM_RESP_ipc_set_streamserver_addr *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	printf("USERDEF_ipc_set_streamserver_addr, channelid=%d, streamid=%d\n", req->channelid, req->streamid);
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	if(req->streamid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid stream ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	int ret = 0;
	//int count = 0;
	//const char * rtmpURL = "rtmp://42.157.5.251:1935/live/B153080736_1?token=YJYCnRgmnidCLlMHhXATtoT5dkn5pIqDHILVtZ1xu3RcZ3";
#endif
	return ret;
}

int USERDEF_ipc_alarm_deployment(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment *req, PARAM_RESP_ipc_alarm_deployment *resp)
{
#ifndef ALARM_SERVICE_SUPPORT
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	if(req == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid alarm params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	SPAlarmSet_t alarm;
	int i = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	
	sp_alarm_get_param(&alarm);

	alarm.bManualDeploy = req->enable;
	alarm.ManualTime = time(NULL);

	//printf("req->timeRange_cnt=%d\n", req->timeRange_cnt);
	alarm.AutoPointCnt = req->timeRange_cnt;
	if (alarm.AutoPointCnt > ARRAY_SIZE(alarm.AutoPoint))
	{
		alarm.AutoPointCnt = ARRAY_SIZE(alarm.AutoPoint);
		printf("Too many alarm point, max: %d, now: %d\n", ARRAY_SIZE(alarm.AutoPoint), req->timeRange_cnt);
	}

	if(alarm.AutoPointCnt > 0)
	{
		for(i = 0; i < alarm.AutoPointCnt; ++i)
		{
			if(req->timeRange[i].dayOfWeek < 0 || req->timeRange[i].dayOfWeek > 6)
			{
				printf("Dayofweek is invalid of point %d: %d\n", i, req->timeRange[i].dayOfWeek);
				continue;
			}
			alarm.AutoPoint[i].dayOfWeek = req->timeRange[i].dayOfWeek;
			alarm.AutoPoint[i].bProtection = req->timeRange[i].bProtection;
			if(req->timeRange[i].time != NULL)
			{
				sscanf(req->timeRange[i].time, "%02d:%02d:%02d", &hour, &min, &sec);
				alarm.AutoPoint[i].time = hour * 3600 + min * 60 + sec;
			}
			//printf("alarm.alarmTime[i].dayOfWeek=%d, alarm.alarmTime[i].bProtection=%d, alarm.alarmTime[i].time=%d\n", 
			//	alarm.alarmTime[i].dayOfWeek, alarm.alarmTime[i].bProtection,alarm.alarmTime[i].time);
		}
	}
	sp_alarm_set_param(&alarm);

	#if 0
	CLOUD cloud;
	mcloud_get_param(&cloud);
	cloud.bEnable = req->bCloudRecord;
	mcloud_set_param(&cloud);
	#endif
	
	sp_alarm_send_deployment();
#endif
	return 0;
}

int USERDEF_ipc_alarm_deployment_query(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_query *req, PARAM_RESP_ipc_alarm_deployment_query *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	/*Alarm time range*/
	SPAlarmSet_t alarm;
	int i = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	char timeStr[16];
	
	sp_alarm_get_param(&alarm);
	resp->cid = req->cid;
	resp->enable = alarm.bEnable;
	
	resp->timeRange_cnt = alarm.alarmTimeCnt;
	resp->timeRange = grpc_malloc(grpc, resp->timeRange_cnt * sizeof(*resp->timeRange));
	if(resp->timeRange == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->timeRange_cnt; i++)
	{
		resp->timeRange[i].dayOfWeek = alarm.alarmTime[i].dayOfWeek-1;
		resp->timeRange[i].bProtection = alarm.alarmTime[i].bProtection;
		hour = alarm.alarmTime[i].time / 3600;
		min = (alarm.alarmTime[i].time % 3600) / 60;
		sec = alarm.alarmTime[i].time % 60;
		sprintf(timeStr, "%02d:%02d:%02d", hour, min, sec);
		resp->timeRange[i].time =  grpc_strdup(grpc, timeStr);
	}

#endif
	return 0;
}

int USERDEF_ipc_alarm_deployment_push(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_push *req, PARAM_RESP_ipc_alarm_deployment_push *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
#endif
	return 0;
}

int USERDEF_ipc_alarm_out(grpc_t *grpc, PARAM_REQ_ipc_alarm_out *req, PARAM_RESP_ipc_alarm_out *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->status)
	{
		SPAlarmIn_t alarm;
		sp_alarmin_get_param(0, &alarm);
		if(alarm.nGuardChn <= 0)
		{
			sp_alarm_buzzing_open();
		}
	}
	else
	{
		SPAlarmIn_t alarm;
		sp_alarmin_get_param(0, &alarm);
		if(alarm.nGuardChn <= 0)
		{
			sp_alarm_buzzing_close();
		}
	}
#endif
	return 0;
}

int USERDEF_ipc_alarm_get_status(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_status *req, PARAM_RESP_ipc_alarm_get_status *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	resp->port = req->port;
	resp->status = sp_alarm_get_status();
#endif
	return 0;	
}

int USERDEF_ipc_image_get_param(grpc_t *grpc, PARAM_REQ_ipc_image_get_param *req, PARAM_RESP_ipc_image_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPImage_t image;

	memset(&image, 0, sizeof(SPImage_t));
	
	ret = sp_image_get_param(&image);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get image param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	
	resp->contrast = image.contrast;
	resp->brightness = image.brightness;
	resp->saturation = image.saturation;
	resp->sharpen = image.sharpen;
	resp->exposureMax= image.exposureMax;
	resp->exposureMin= image.exposureMin;
	if(image.scene == SP_SCENE_INDOOR)
		resp->scene = "indoor";
	else if(image.scene == SP_SCENE_OUTDOOR)
		resp->scene = "outdoor";
	else if(image.scene == SP_SCENE_DEFAULT)
		resp->scene = "default";
	else if(image.scene == SP_SCENE_SOFT)
		resp->scene = "soft";
	else
		resp->scene = NULL;
	if(image.daynightMode == SP_SENSOR_DAYNIGHT_AUTO)
		resp->daynightMode = "auto";
	else if(image.daynightMode == SP_SENSOR_DAYNIGHT_ALWAYS_DAY)
		resp->daynightMode = "alwaysDay";
	else if(image.daynightMode == SP_SENSOR_DAYNIGHT_ALWAYS_NIGHT)
		resp->daynightMode = "alwaysNight";
	else if(image.daynightMode == SP_SENSOR_DAYNIGHT_TIMER)
		resp->daynightMode = "timer";
	else
		resp->daynightMode = NULL;
	resp->dayStart.hour = image.dayStart.hour;
	resp->dayStart.minute = image.dayStart.minute;
	resp->dayEnd.hour = image.dayEnd.hour;
	resp->dayEnd.minute = image.dayEnd.minute;
	resp->bEnableAWB = image.bEnableAWB;
	resp->bEnableMI = image.bEnableMI;
	resp->bEnableST = image.bEnableST;
	resp->bEnableNoC = image.bEnableNoC;
	resp->bEnableWDynamic = image.bEnableWDynamic;
	resp->bAutoLowFrameEn = image.bAutoLowFrameEn;
	resp->bNightOptimization= image.bNightOptimization;
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_image_set_param(grpc_t *grpc, PARAM_REQ_ipc_image_set_param *req, PARAM_RESP_ipc_image_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPImage_t image;

	memset(&image, 0, sizeof(SPImage_t));

	ret = sp_image_get_param(&image);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get image param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	if(req->contrast < 0 || req->contrast > 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid contrast param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	else
		image.contrast = req->contrast;
	if(req->sharpen < 0 || req->sharpen > 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid sharpen param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	else
		image.sharpen = req->sharpen;
	if(req->brightness < 0 || req->brightness > 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid brightness param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	else
		image.brightness = req->brightness;
	if(req->saturation < 0 || req->saturation > 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid saturation param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	else
		image.saturation = req->saturation;
	if(req->exposureMax > req->exposureMin)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid exposure time param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	switch(req->exposureMax)
	{
		case 3:
		case 6:
		case 12:
		case 25:
		case 50:
		case 100:
		case 250:
		case 500:
		case 750:
		case 1000:
		case 2000:
		case 4000:
		case 10000:
		case 100000:
			image.exposureMax = req->exposureMax;
			break;
		default:
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid max exposure time param!");
			return GRPC_ERR_INVALID_PARAMS;
	}
	switch(req->exposureMin)
	{
		case 3:
		case 6:
		case 12:
		case 25:
		case 50:
		case 100:
		case 250:
		case 500:
		case 750:
		case 1000:
		case 2000:
		case 4000:
		case 10000:
		case 100000:
			image.exposureMin = req->exposureMin;
			break;
		default:
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid min exposure time param!");
			return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->scene != NULL)
	{
		if(strcmp(req->scene, "indoor") == 0)
			image.scene = SP_SCENE_INDOOR;
		else if(strcmp(req->scene, "outdoor") == 0)
			image.scene = SP_SCENE_OUTDOOR;
		else if(strcmp(req->scene, "default") == 0)
			image.scene = SP_SCENE_DEFAULT;
		else if(strcmp(req->scene, "soft") == 0)
			image.scene = SP_SCENE_SOFT;
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid scene param!");
			return GRPC_ERR_INVALID_PARAMS;
		}
	}
	if(req->daynightMode != NULL)
	{
		if(strcmp(req->daynightMode, "auto") == 0)
			image.daynightMode = SP_SENSOR_DAYNIGHT_AUTO;
		else if(strcmp(req->daynightMode, "alwaysDay") == 0)
			image.daynightMode = SP_SENSOR_DAYNIGHT_ALWAYS_DAY;
		else if(strcmp(req->daynightMode, "alwaysNight") == 0)
			image.daynightMode = SP_SENSOR_DAYNIGHT_ALWAYS_NIGHT;
		else if(strcmp(req->daynightMode, "timer") == 0)
			image.daynightMode = SP_SENSOR_DAYNIGHT_TIMER;
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid daynightMode param!");
			return GRPC_ERR_INVALID_PARAMS;
		}
	}
	image.dayStart.hour = req->dayStart.hour;
	image.dayStart.minute = req->dayStart.minute;
	image.dayEnd.hour = req->dayEnd.hour;
	image.dayEnd.minute = req->dayEnd.minute;
	image.bEnableAWB = req->bEnableAWB;
	image.bEnableMI = req->bEnableMI;
	image.bEnableST = req->bEnableST;
	image.bEnableNoC = req->bEnableNoC;
	image.bEnableWDynamic = req->bEnableWDynamic;
	image.bAutoLowFrameEn = req->bAutoLowFrameEn;
	image.bNightOptimization= req->bNightOptimization;
	ret = sp_image_set_param(&image);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set image param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_chnosd_get_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_get_param *req, PARAM_RESP_ipc_chnosd_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPChnOsdAttr_t attr;
	int ret = 0;
	
	memset(&attr, 0, sizeof(SPChnOsdAttr_t));

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	ret = sp_chnosd_get_param(0, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get OSD param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->bShowOSD = attr.bShowOSD;
	resp->bLargeOSD = attr.bLargeOSD;
	resp->bOsdInvColEn= attr.osdbInvColEn;
	resp->timeFormat = grpc_strdup(grpc, attr.timeFormat);
	
	if(attr.position == SP_CHNOSD_POS_LEFT_TOP)
		resp->position = "left_top";
	else if(attr.position == SP_CHNOSD_POS_LEFT_BOTTOM)
		resp->position = "left_bottom";
	else if(attr.position == SP_CHNOSD_POS_RIGHT_TOP)
		resp->position = "right_top";
	else if(attr.position == SP_CHNOSD_POS_RIGHT_BOTTOM)
		resp->position = "right_bottom";
	else if(attr.position == SP_CHNOSD_POS_HIDE)
		resp->position = "hide";
	else
		resp->position = NULL;
	
	if(attr.timePos == SP_CHNOSD_POS_LEFT_TOP)
		resp->timePos = "left_top";
	else if(attr.timePos == SP_CHNOSD_POS_LEFT_BOTTOM)
		resp->timePos = "left_bottom";
	else if(attr.timePos == SP_CHNOSD_POS_RIGHT_TOP)
		resp->timePos = "right_top";
	else if(attr.timePos == SP_CHNOSD_POS_RIGHT_BOTTOM)
		resp->timePos = "right_bottom";
	else if(attr.timePos == SP_CHNOSD_POS_HIDE)
		resp->timePos = "hide";
	else
		resp->timePos = NULL;
	char channelNameUTF8[20];
	utl_iconv_gb2312toutf8(attr.channelName, channelNameUTF8, sizeof(channelNameUTF8)-1);
	resp->channelName= grpc_strdup(grpc, channelNameUTF8);
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_chnosd_set_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_set_param *req, PARAM_RESP_ipc_chnosd_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPChnOsdAttr_t attr;
	int ret = 0;
	int i = 0;
	const SPHWInfo_t *ipcinfo = sp_dev_get_hwinfo();
	
	SPDevInfo_t devinfo;
	
	memset(&devinfo, 0, sizeof(SPDevInfo_t));
	memset(&attr, 0, sizeof(SPChnOsdAttr_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	ret = sp_chnosd_get_param(0, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get OSD param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	attr.bShowOSD = req->attr.bShowOSD;
	attr.bLargeOSD= req->attr.bLargeOSD;
	attr.osdbInvColEn= req->attr.bOsdInvColEn;
	if(req->attr.timeFormat != NULL)
	{
		if(strcmp(req->attr.timeFormat, "YYYY-MM-DD hh:mm:ss") == 0)
			strcpy(attr.timeFormat, "YYYY-MM-DD hh:mm:ss");
		else if(strcmp(req->attr.timeFormat, "DD/MM/YYYY hh:mm:ss") == 0)
			strcpy(attr.timeFormat, "DD/MM/YYYY hh:mm:ss");
		else if(strcmp(req->attr.timeFormat, "MM/DD/YYYY hh:mm:ss") == 0)
			strcpy(attr.timeFormat, "MM/DD/YYYY hh:mm:ss");
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid time format!");
			return GRPC_ERR_INVALID_PARAMS;
		}
	}
	if(req->attr.channelName != NULL)
		strncpy(attr.channelName, req->attr.channelName, 20);

	if(req->attr.position != NULL)
	{
		if(strcmp(req->attr.position, "left_top") == 0)
			attr.position = SP_CHNOSD_POS_LEFT_TOP;
		else if(strcmp(req->attr.position, "left_bottom") == 0)
			attr.position = SP_CHNOSD_POS_LEFT_BOTTOM;
		else if(strcmp(req->attr.position, "right_top") == 0)
			attr.position = SP_CHNOSD_POS_RIGHT_TOP;
		else if(strcmp(req->attr.position, "right_bottom") == 0)
			attr.position = SP_CHNOSD_POS_RIGHT_BOTTOM;
		else if(strcmp(req->attr.position, "hide") == 0)
			attr.position = SP_CHNOSD_POS_HIDE;
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid OSD position!");
			return GRPC_ERR_INVALID_PARAMS;
		}
	}
	if(req->attr.timePos != NULL)
	{
		if(strcmp(req->attr.timePos, "left_top") == 0)
			attr.timePos = SP_CHNOSD_POS_LEFT_TOP;
		else if(strcmp(req->attr.timePos, "left_bottom") == 0)
			attr.timePos = SP_CHNOSD_POS_LEFT_BOTTOM;
		else if(strcmp(req->attr.timePos, "right_top") == 0)
			attr.timePos = SP_CHNOSD_POS_RIGHT_TOP;
		else if(strcmp(req->attr.timePos, "right_bottom") == 0)
			attr.timePos = SP_CHNOSD_POS_RIGHT_BOTTOM;
		else if(strcmp(req->attr.timePos, "hide") == 0)
			attr.timePos = SP_CHNOSD_POS_HIDE;
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid time position!");
			return GRPC_ERR_INVALID_PARAMS;
		}
	}
	if(req->attr.channelName != NULL)
		utl_iconv_utf8togb2312(req->attr.channelName, attr.channelName, sizeof(attr.channelName));
	for(i=0; i<ipcinfo->streamCnt; i++)
	{
		ret = sp_chnosd_set_param(i, &attr);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set OSD param failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
	}
	
	
	ret = sp_dev_get_info(&devinfo);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get device info failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	
	if(req->attr.channelName != NULL)
		strncpy(devinfo.name, req->attr.channelName, 20);
	
	ret = sp_dev_set_info(&devinfo);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set device info failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_storage_get_info(grpc_t *grpc, PARAM_REQ_ipc_storage_get_info *req, PARAM_RESP_ipc_storage_get_info *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPStorage_t storage;
	int ret = 0;

	memset(&storage, 0, sizeof(SPStorage_t));
	
	ret = sp_storage_get_info(&storage);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get storage info failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->bMounted = storage.mounted;
	// resp->mediaType = storage.mediaType;
	resp->mediaType = 0;
	resp->curPart = storage.nCurPart;
	resp->cylinder = storage.nCylinder;
	resp->entryCount = storage.nEntryCount;
	resp->freeSpace_cnt = PARTS_PER_DEV;
	resp->freeSpace = grpc_malloc(grpc, resp->freeSpace_cnt*sizeof(*resp->freeSpace));
	if(resp->freeSpace == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	resp->partSpace_cnt = PARTS_PER_DEV;
	resp->partSpace = grpc_malloc(grpc, resp->partSpace_cnt*sizeof(*resp->partSpace));
	if(resp->partSpace == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	resp->partition = storage.nPartition;
	resp->partSize = storage.nPartSize;
	resp->size = storage.nSize;
	resp->status = storage.nStatus;
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_storage_format(grpc_t *grpc, PARAM_REQ_ipc_storage_format *req, PARAM_RESP_ipc_storage_format *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->diskNum < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid disk number!");
		return GRPC_ERR_INVALID_PARAMS;		
	}

	ret = sp_storage_format(req->diskNum);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Storage format failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_storage_error_ignore(grpc_t *grpc, PARAM_REQ_ipc_storage_error_ignore *req, PARAM_RESP_ipc_storage_error_ignore *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
#ifdef ALARM_SERVICE_SUPPORT
	alarm_service_tfcard_notice_ignore();
#endif
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_set_oss_folder(grpc_t *grpc, PARAM_REQ_ipc_set_oss_folder *req, PARAM_RESP_ipc_set_oss_folder *resp)
{
#if (!PREREC_SUPPORT) // 云存储路径设置
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	if(req->fileTimeLen<30)
		return GRPC_ERR_INVALID_PARAMS;
	if(NULL != req->folderName)
		mprerec_set_FileFolderName(req->folderName, req->fileTimeLen);
#endif

	return 0;
}

int USERDEF_ipc_mdetect_set_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_set_param *req, PARAM_RESP_ipc_mdetect_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPMdetect_t md;
	int ret = 0;
	int i = 0;

	memset(&md, 0, sizeof(SPMdetect_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	if(req->md.delay < 0 || req->md.delay > 100)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid delay param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ret = sp_mdetect_get_param(req->channelid, &md);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get motion detect param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	md.bEnable = req->md.bEnable;
	md.bEnableRecord = req->md.bEnableRecord;
	md.nSensitivity= req->md.sensitivity;
	md.nRectNum= req->md.rects_cnt;
	md.nDelay= req->md.delay;
	md.bOutClient= req->md.bOutClient;
	md.bOutEMail= req->md.bOutEmail;
	memset(md.stRect, 0, sizeof(md.stRect));
	for(i=0; i<req->md.rects_cnt; i++)
	{
		if(i >= 4)
			break;
		if(req->md.rects != NULL)
		{
			md.stRect[i].x = req->md.rects[i].x;
			md.stRect[i].y = req->md.rects[i].y;
			md.stRect[i].w = req->md.rects[i].w;
			md.stRect[i].h = req->md.rects[i].h;
		}
	}
	ret = sp_mdetect_set_param(req->channelid, &md);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set motion detect param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_mdetect_get_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_get_param *req, PARAM_RESP_ipc_mdetect_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPMdetect_t md;
	int ret = 0;
	int i = 0;

	memset(&md, 0, sizeof(SPMdetect_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	ret = sp_mdetect_get_param(req->channelid, &md);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get motion detect param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	resp->bEnable = md.bEnable;
	resp->bEnableRecord= md.bEnableRecord;
	resp->bOutClient = md.bOutClient;
	resp->bOutEmail = md.bOutEMail;
	resp->delay = md.nDelay;
	resp->sensitivity = md.nSensitivity;
	resp->rects_cnt = md.nRectNum;
	resp->rects = grpc_malloc(grpc, resp->rects_cnt*sizeof(*resp->rects));
	if(resp->rects == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->rects_cnt; i++)
	{
		resp->rects[i].x = md.stRect[i].x;
		resp->rects[i].y = md.stRect[i].y;
		resp->rects[i].w = md.stRect[i].w;
		resp->rects[i].h = md.stRect[i].h;
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_mdetect_balarming(grpc_t *grpc, PARAM_REQ_ipc_mdetect_balarming *req, PARAM_RESP_ipc_mdetect_balarming *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	resp->bMdetectAlarming = sp_mdetect_balarming(req->channelid);
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_connection_get_list(grpc_t *grpc, PARAM_REQ_ipc_connection_get_list *req, PARAM_RESP_ipc_connection_get_list *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	
	int i = 0;
	SPConType_e conType;
	SPConnection_t *connection = NULL;

	if(req->conType == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid connection type!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	if(strcmp(req->conType, "all") == 0)
		conType = SP_CON_ALL;
	else if(strcmp(req->conType, "jovision") == 0)
		conType = SP_CON_JOVISION;
	else if(strcmp(req->conType, "rtsp") == 0)
		conType = SP_CON_RTSP;
	else if(strcmp(req->conType, "gb28181") == 0)
		conType = SP_CON_GB28181;
	else if(strcmp(req->conType, "psia") == 0)
		conType = SP_CON_PSIA;
	else if(strcmp(req->conType, "other") == 0)
		conType = SP_CON_OTHER;
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid connection type!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	resp->connectionList_cnt= sp_connect_get_cnt(conType);
	sp_connect_seek_set();
	resp->connectionList= grpc_malloc(grpc, resp->connectionList_cnt * sizeof(*resp->connectionList));
	if(resp->connectionList == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for (i=0; i<resp->connectionList_cnt; i++)
	{
		connection = sp_connect_get_next(conType);
		if(connection != NULL)
		{
			if(connection->conType == SP_CON_ALL)
				resp->connectionList[i].conType = "all";
			else if(connection->conType == SP_CON_JOVISION)
				resp->connectionList[i].conType = "jovision";
			else if(connection->conType == SP_CON_RTSP)
				resp->connectionList[i].conType = "rtsp";
			else if(connection->conType == SP_CON_GB28181)
				resp->connectionList[i].conType = "gb28181";
			else if(connection->conType == SP_CON_PSIA)
				resp->connectionList[i].conType = "psia";
			else if(connection->conType == SP_CON_OTHER)
				resp->connectionList[i].conType = "other";
			else
				resp->connectionList[i].conType = NULL;
			resp->connectionList[i].key = connection->key;
			resp->connectionList[i].addr = grpc_strdup(grpc, connection->addr);
			resp->connectionList[i].user = grpc_strdup(grpc, connection->user);
		}
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get connection failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_connection_breakoff(grpc_t *grpc, PARAM_REQ_ipc_connection_breakoff *req, PARAM_RESP_ipc_connection_breakoff *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	SPConnection_t connection;
	int ret = 0;
	int i = 0;

	memset(&connection, 0, sizeof(SPConnection_t));
	
	if(req->connectionList_cnt <= 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid connection list count!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	for(i=0; i<req->connectionList_cnt; i++)
	{
		if(req->connectionList != NULL)
		{
			if(req->connectionList[i].conType != NULL)
			{
				if(strcmp(req->connectionList[i].conType, "all") == 0)
					connection.conType = SP_CON_ALL;
				else if(strcmp(req->connectionList[i].conType, "jovision") == 0)
					connection.conType = SP_CON_JOVISION;
				else if(strcmp(req->connectionList[i].conType, "rtsp") == 0)
					connection.conType = SP_CON_RTSP;
				else if(strcmp(req->connectionList[i].conType, "gb28181") == 0)
					connection.conType = SP_CON_GB28181;
				else if(strcmp(req->connectionList[i].conType, "psia") == 0)
					connection.conType = SP_CON_PSIA;
				else if(strcmp(req->connectionList[i].conType, "other") == 0)
					connection.conType = SP_CON_OTHER;
				else
				{
					grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid connection type!");
					return GRPC_ERR_INVALID_PARAMS;
				}
			}
			connection.key = req->connectionList[i].key;
			ret = sp_connect_breakoff(&connection);
			if(ret != 0)
			{
				grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Break off connection failed!");
				return GRPC_ERR_INTERNAL_ERROR;
			}
		}
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_move_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_start *req, PARAM_RESP_ipc_ptz_move_start *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	if(req->panLeft <= -255 || req->panLeft >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid pan speed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->tiltUp <= -255 || req->tiltUp >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid tilt speed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->zoomIn <= -255 || req->zoomIn >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid zoom speed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
		
	ret = sp_ptz_move_start(req->channelid, req->panLeft, req->tiltUp, req->zoomIn);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Ptz move start failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_move_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_stop *req, PARAM_RESP_ipc_ptz_move_stop *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_move_stop(req->channelid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Ptz move stop failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_fi_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_start *req, PARAM_RESP_ipc_ptz_fi_start *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	if(req->focusFar <= -255 || req->focusFar >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid focus speed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->irisOpen <= -255 || req->irisOpen >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid iris speed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ret = sp_ptz_fi_start(req->channelid, req->focusFar, req->irisOpen);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Ptz focus or iris start failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_fi_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_stop *req, PARAM_RESP_ipc_ptz_fi_stop *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_fi_stop(req->channelid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Ptz focus or iris stop failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_preset_set(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_set *req, PARAM_RESP_ipc_ptz_preset_set *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->presetno < -1 ||  req->name == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid preset params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	char presetNameGB2312[32];
	memset(presetNameGB2312, 0, sizeof(presetNameGB2312));
	utl_iconv_utf8togb2312(req->name, presetNameGB2312, sizeof(presetNameGB2312)-1);
	
	ret = sp_ptz_preset_set(req->channelid, req->presetno, presetNameGB2312);
	if(ret < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Set preset failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_preset_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_locate *req, PARAM_RESP_ipc_ptz_preset_locate *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->presetno < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid preset params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_preset_locate(req->channelid, req->presetno);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Locate preset failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_preset_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_delete *req, PARAM_RESP_ipc_ptz_preset_delete *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->presetno < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid preset params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_preset_delete(req->channelid, req->presetno);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Preset does not exist!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_presets_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_presets_get *req, PARAM_RESP_ipc_ptz_presets_get *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	int i = 0;
	SPPreset_t preset;

	memset(&preset, 0, sizeof(SPPreset_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid preset params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	resp->presetsList_cnt = sp_ptz_preset_get_cnt(req->channelid);
	resp->presetsList = grpc_malloc(grpc, resp->presetsList_cnt*sizeof(*resp->presetsList));
	if(resp->presetsList == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->presetsList_cnt; i++)
	{
		ret = sp_ptz_preset_get(req->channelid, i, &preset);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Get preset failed!");
			return GRPC_ERR_INVALID_PARAMS;
		}
		resp->presetsList[i].presetno = preset.presetno;
		char presetNameUTF8[32];
		memset(presetNameUTF8, 0, sizeof(presetNameUTF8));
		utl_iconv_gb2312toutf8(preset.name, presetNameUTF8, sizeof(presetNameUTF8)-1);
		resp->presetsList[i].name = grpc_strdup(grpc, presetNameUTF8);
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_create(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_create *req, PARAM_RESP_ipc_ptz_patrol_create *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_patrol_create(req->channelid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Create patrol failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_delete *req, PARAM_RESP_ipc_ptz_patrol_delete *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->index < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_patrol_delete(req->channelid, req->index);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Delete patrol failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrols_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrols_get *req, PARAM_RESP_ipc_ptz_patrols_get *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	int i = 0;
	SPPatrol_t patrol;

	memset(&patrol, 0, sizeof(SPPatrol_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	resp->patrolsList_cnt = sp_ptz_patrol_get_cnt(req->channelid);
	resp->patrolsList = grpc_malloc(grpc, resp->patrolsList_cnt*sizeof(*resp->patrolsList));
	if(resp->patrolsList == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->patrolsList_cnt; i++)
	{
		ret = sp_ptz_patrol_get(req->channelid, i, &patrol);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Get patrol failed!");
			return GRPC_ERR_INVALID_PARAMS;
		}
		resp->patrolsList[i].patrolid= patrol.patrolid;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_get_nodes(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_get_nodes *req, PARAM_RESP_ipc_ptz_patrol_get_nodes *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	int i = 0;
	SPPatrol_t patrol;
	SPPatrolNode_t node;

	memset(&patrol, 0, sizeof(SPPatrol_t));
	memset(&node, 0, sizeof(SPPatrolNode_t));

	if(req->channelid != 0 || req->patrolid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	resp->patrolNodesList_cnt= sp_ptz_patrol_get_node_cnt(req->channelid, &patrol);
	resp->patrolNodesList = grpc_malloc(grpc, resp->patrolNodesList_cnt*sizeof(*resp->patrolNodesList));
	if(resp->patrolNodesList == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->patrolNodesList_cnt; i++)
	{
		ret = sp_ptz_patrol_get_node(req->channelid, &patrol, i, &node);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Get patrol node failed!");
			return GRPC_ERR_INVALID_PARAMS;
		}
		resp->patrolNodesList[i].staySeconds = node.staySeconds;
		resp->patrolNodesList[i].preset.presetno = node.preset.presetno;
		char presetNameUTF8[32];
		memset(presetNameUTF8, 0, sizeof(presetNameUTF8));
		utl_iconv_gb2312toutf8(node.preset.name, presetNameUTF8, sizeof(presetNameUTF8)-1);
		resp->patrolNodesList[i].preset.name = grpc_strdup(grpc, presetNameUTF8);
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_add_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_add_node *req, PARAM_RESP_ipc_ptz_patrol_add_node *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPPatrol_t patrol;

	if(req->channelid != 0 || req->patrolid < 0 || req->presetno < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	ret = sp_ptz_patrol_add_node(req->channelid, &patrol, req->presetno, req->staySeconds);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Add patrol node failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_del_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_del_node *req, PARAM_RESP_ipc_ptz_patrol_del_node *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPPatrol_t patrol;

	if(req->channelid != 0 || req->patrolid < 0 || req->presetindex < -1)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	ret = sp_ptz_patrol_del_node(req->channelid, &patrol, req->presetindex);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Delete patrol node failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_speed *req, PARAM_RESP_ipc_ptz_patrol_set_speed *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPPatrol_t patrol;

	if(req->channelid != 0 || req->patrolid < 0 
		|| req->speed < 0 || req->speed >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	
	ret = sp_ptz_patrol_set_speed(req->channelid, &patrol, req->speed);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Set patrol speed failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_set_stay_seconds(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_stay_seconds *req, PARAM_RESP_ipc_ptz_patrol_set_stay_seconds *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPPatrol_t patrol;

	if(req->channelid != 0 || req->patrolid < 0 || req->staySeconds < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	
	ret = sp_ptz_patrol_set_stay_seconds(req->channelid, &patrol, req->staySeconds);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Set patrol stay seconds failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_locate *req, PARAM_RESP_ipc_ptz_patrol_locate *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPPatrol_t patrol;

	if(req->channelid != 0 || req->patrolid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	
	ret = sp_ptz_patrol_locate(req->channelid, &patrol);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Locate patrol failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_patrol_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_stop *req, PARAM_RESP_ipc_ptz_patrol_stop *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPPatrol_t patrol;

	if(req->channelid != 0 || req->patrolid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid patrol params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	patrol.patrolid = req->patrolid;
	
	ret = sp_ptz_patrol_stop(req->channelid, &patrol);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Stop Patrol failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_scan_set_left(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_left *req, PARAM_RESP_ipc_ptz_scan_set_left *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->groupid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid scan params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_scan_set_left(req->channelid, req->groupid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Scan set left failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_scan_set_right(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_right *req, PARAM_RESP_ipc_ptz_scan_set_right *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->groupid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid scan params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_scan_set_right(req->channelid, req->groupid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Scan set right failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_scan_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_start *req, PARAM_RESP_ipc_ptz_scan_start *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->groupid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid scan params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_scan_start(req->channelid, req->groupid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Scan start failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_scan_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_stop *req, PARAM_RESP_ipc_ptz_scan_stop *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->groupid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid scan params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_scan_stop(req->channelid, req->groupid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Scan stop failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_scan_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_speed *req, PARAM_RESP_ipc_ptz_scan_set_speed *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	
	if(req->channelid != 0 || req->groupid < 0 
		|| req->speed < 0 || req->speed >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid scan params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	ret = sp_ptz_scan_set_speed(req->channelid, req->groupid, req->speed);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Scan set speed failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ptz_auto(grpc_t *grpc, PARAM_REQ_ipc_ptz_auto *req, PARAM_RESP_ipc_ptz_auto *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->channelid != 0 || req->speed < 0 || req->speed >= 255)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid auto params!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	ret = sp_ptz_auto(req->channelid, req->speed);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Auto set speed failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
#endif
	return 0;
}

int USERDEF_ipc_ptz_aux_on(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_on *req, PARAM_RESP_ipc_ptz_aux_on *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid aux params!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	ret = sp_ptz_aux_on(req->channelid, req->auxid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "aux set speed failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
#endif
	return 0;
}

int USERDEF_ipc_ptz_aux_off(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_off *req, PARAM_RESP_ipc_ptz_aux_off *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid aux params!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	ret = sp_ptz_aux_off(req->channelid, req->auxid);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "aux set speed failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
#endif
	return 0;
}

int USERDEF_ipc_ptz_zoom_zone(grpc_t *grpc, PARAM_REQ_ipc_ptz_zoom_zone *req, PARAM_RESP_ipc_ptz_zoom_zone *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int ret = 0;

	if(req->channelid!=0 || req->cmd>0xC1 || req->cmd<0xC0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid zone params!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	ret = sp_ptz_position(req->channelid, (SPPosition_t *)&req->zoneinfo, req->cmd);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "3d zone failed!");
		return GRPC_ERR_INVALID_PARAMS;
	}
#endif
	return 0;
}

int USERDEF_ipc_ai_get_param(grpc_t *grpc, PARAM_REQ_ipc_ai_get_param *req, PARAM_RESP_ipc_ai_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPAudioAttr_t attr;
	int ret = 0;
	
	memset(&attr, 0, sizeof(SPAudioAttr_t));

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
		
	ret = sp_audio_get_param(req->channelid, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get AI param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	switch(attr.encType)
	{
	default:
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Audio format not supported!");
		break;
	case SP_AUDIO_ENC_PCM:
		resp->encType = "pcm";
		break;
	case SP_AUDIO_ENC_G711_A:
		resp->encType = "g711a";
		break;
	case SP_AUDIO_ENC_G711_U:
		resp->encType = "g711u";
		break;
	case SP_AUDIO_ENC_G726_16K:
		resp->encType = "g726_16k";
		break;
	case SP_AUDIO_ENC_G726_24K:
		resp->encType = "g726_24k";
		break;
	case SP_AUDIO_ENC_G726_32K:
		resp->encType = "g726_32k";
		break;
	case SP_AUDIO_ENC_G726_40K:
		resp->encType = "g726_40k";
		break;
	case SP_AUDIO_ENC_ADPCM:
		resp->encType = "adpcm";
		break;
	case SP_AUDIO_ENC_AC3:
		resp->encType = "ac3";
		break;
	}
	resp->sampleRate = attr.sampleRate;
	if(attr.bitWidth == SP_AUDIO_BIT_WIDTH_8)
		resp->bitWidth = 8;
	else if(attr.bitWidth == SP_AUDIO_BIT_WIDTH_16)
		resp->bitWidth = 16;
	else if(attr.bitWidth == SP_AUDIO_BIT_WIDTH_32)
		resp->bitWidth = 32;

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ai_set_param(grpc_t *grpc, PARAM_REQ_ipc_ai_set_param *req, PARAM_RESP_ipc_ai_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPAudioAttr_t attr;
	int ret = 0;
	
	memset(&attr, 0, sizeof(SPAudioAttr_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
		
	ret = sp_audio_get_param(req->channelid, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get AI param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	if(req->audioAttr.bitWidth == 8)
		attr.bitWidth = SP_AUDIO_BIT_WIDTH_8;
	else if(req->audioAttr.bitWidth == 16)
		attr.bitWidth = SP_AUDIO_BIT_WIDTH_16;
	else if(req->audioAttr.bitWidth == 32)
		attr.bitWidth = SP_AUDIO_BIT_WIDTH_32;
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid AI bit width!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->audioAttr.sampleRate == 8000)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_8000;
	else if(req->audioAttr.sampleRate == 11025)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_11025;
	else if(req->audioAttr.sampleRate == 16000)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_16000;
	else if(req->audioAttr.sampleRate == 22050)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_22050;
	else if(req->audioAttr.sampleRate == 24000)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_24000;
	else if(req->audioAttr.sampleRate == 32000)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_32000;
	else if(req->audioAttr.sampleRate == 44100)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_44100;
	else if(req->audioAttr.sampleRate == 48000)
		attr.sampleRate = SP_AUDIO_SAMPLE_RATE_48000;
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid AI sample rate!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->audioAttr.encType != NULL)
	{
		if(strcmp(req->audioAttr.encType, "pcm") == 0)
			attr.encType = SP_AUDIO_ENC_PCM;
		else if(strcmp(req->audioAttr.encType, "g711a") == 0)
			attr.encType = SP_AUDIO_ENC_G711_A;
		else if(strcmp(req->audioAttr.encType, "g711u") == 0)
			attr.encType = SP_AUDIO_ENC_G711_U;
		else if(strcmp(req->audioAttr.encType, "g726_16k") == 0)
			attr.encType = SP_AUDIO_ENC_G726_16K;
		else if(strcmp(req->audioAttr.encType, "g726_24k") == 0)
			attr.encType = SP_AUDIO_ENC_G726_24K;
		else if(strcmp(req->audioAttr.encType, "g726_32k") == 0)
			attr.encType = SP_AUDIO_ENC_G726_32K;
		else if(strcmp(req->audioAttr.encType, "g726_40k") == 0)
			attr.encType = SP_AUDIO_ENC_G726_40K;
		else if(strcmp(req->audioAttr.encType, "adpcm") == 0)
			attr.encType = SP_AUDIO_ENC_ADPCM;
		else if(strcmp(req->audioAttr.encType, "aac") == 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "AAC not supported!");
			return GRPC_ERR_INVALID_PARAMS;
		}
		else if(strcmp(req->audioAttr.encType, "ac3") == 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "AC3 not supported!");
			return GRPC_ERR_INVALID_PARAMS;
		}
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid AI encode type!");
			return GRPC_ERR_INVALID_PARAMS;
		}
	}
	ret = sp_audio_set_param(req->channelid, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set AI param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ao_get_param(grpc_t *grpc, PARAM_REQ_ipc_ao_get_param *req, PARAM_RESP_ipc_ao_get_param *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	jv_audio_attr_t ja;
	jv_ao_get_attr(req->channelid, &ja);
	switch(ja.encType)
	{
	default:
		grpc_s_set_error(grpc, GRPC_ERR_SERVER_ERROR_START, "Audio format not supported!");
		break;
	case JV_AUDIO_ENC_PCM:
		resp->encType = grpc_strdup(grpc, "pcm");
		break;
	case JV_AUDIO_ENC_G711_A:
		resp->encType = grpc_strdup(grpc, "g711a");
		break;
	case JV_AUDIO_ENC_G711_U:
		resp->encType = grpc_strdup(grpc, "g711u");
		break;
	case JV_AUDIO_ENC_G726_16K:
		resp->encType = grpc_strdup(grpc, "g726_16k");
		break;
	case JV_AUDIO_ENC_G726_24K:
		resp->encType = grpc_strdup(grpc, "g726_24k");
		break;
	case JV_AUDIO_ENC_G726_32K:
		resp->encType = grpc_strdup(grpc, "g726_32k");
		break;
	case JV_AUDIO_ENC_G726_40K:
		resp->encType = grpc_strdup(grpc, "g726_40k");
		break;
	case JV_AUDIO_ENC_ADPCM:
		resp->encType = grpc_strdup(grpc, "adpcm");
		break;
	}
	resp->sampleRate = ja.sampleRate;
	resp->bitWidth = ja.bitWidth;

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ao_set_param(grpc_t *grpc, PARAM_REQ_ipc_ao_set_param *req, PARAM_RESP_ipc_ao_set_param *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_stream_get_param(grpc_t *grpc, PARAM_REQ_ipc_stream_get_param *req, PARAM_RESP_ipc_stream_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPStreamAttr_t attr;

	if(req->channelid != 0 || req->streamid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID or stream ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	const SPHWInfo_t *hwinfo = sp_dev_get_hwinfo();
	if(hwinfo == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get hwinfo failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
		
	if(req->streamid >= hwinfo->streamCnt)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID or stream ID!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	memset(&attr, 0, sizeof(SPStreamAttr_t));
	ret = sp_stream_get_param(req->streamid, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get stream param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->bitRate = attr.bitrate;
	resp->frameRate = attr.framerate;
	resp->height = attr.height;
	resp->width = attr.width;
	resp->ngop_s = attr.ngop_s;
	
	if(attr.maxQP >= 50)
		resp->quality = 20;
	else if(attr.maxQP >= 48)
		resp->quality = 40;
	else if(attr.maxQP >= 45)
		resp->quality = 60;
	else if(attr.maxQP >= 42)
		resp->quality = 80;
	else
		resp->quality = 100;
	
	if(attr.rcMode == SP_VENC_RC_MODE_CBR)
		resp->rcMode = "cbr";
	else if(attr.rcMode == SP_VENC_RC_MODE_VBR)
		resp->rcMode = "vbr";
	else if(attr.rcMode == SP_VENC_RC_MODE_FIXQP)
		resp->rcMode = "fixQP";
	else
		resp->rcMode = NULL;
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_stream_get_params(grpc_t *grpc, PARAM_REQ_ipc_stream_get_params *req, PARAM_RESP_ipc_stream_get_params *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	int i = 0;
	SPStreamAttr_t attr;

	memset(&attr, 0, sizeof(SPStreamAttr_t));
	const SPHWInfo_t *hwinfo = sp_dev_get_hwinfo();
	if(hwinfo == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get hwinfo failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->streams_cnt = hwinfo->streamCnt;
	resp->streams = grpc_malloc(grpc, resp->streams_cnt*sizeof(*resp->streams));
	if(resp->streams == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->streams_cnt; i++)
	{
		ret = sp_stream_get_param(i, &attr);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get stream param failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
		resp->streams[i].channelid = 0;
		resp->streams[i].streamid = i;
		resp->streams[i].bitRate = attr.bitrate;
		resp->streams[i].frameRate = attr.framerate;
		resp->streams[i].height = attr.height;
		resp->streams[i].width = attr.width;
		resp->streams[i].ngop_s = attr.ngop_s;
		
		if(attr.maxQP >= 50)
			resp->streams[i].quality = 20;
		else if(attr.maxQP >= 48)
			resp->streams[i].quality = 40;
		else if(attr.maxQP >= 45)
			resp->streams[i].quality = 60;
		else if(attr.maxQP >= 42)
			resp->streams[i].quality = 80;
		else
			resp->streams[i].quality = 100;
		
		if(attr.rcMode == SP_VENC_RC_MODE_CBR)
			resp->streams[i].rcMode = "cbr";
		else if(attr.rcMode == SP_VENC_RC_MODE_VBR)
			resp->streams[i].rcMode = "vbr";
		else if(attr.rcMode == SP_VENC_RC_MODE_FIXQP)
			resp->streams[i].rcMode = "fixQP";
		else
			resp->streams[i].rcMode = NULL;
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_stream_set_param(grpc_t *grpc, PARAM_REQ_ipc_stream_set_param *req, PARAM_RESP_ipc_stream_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	SPStreamAttr_t attr;
	
	memset(&attr, 0, sizeof(SPStreamAttr_t));

	if(req->channelid != 0 || req->streamid < 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID or stream ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	ret = sp_stream_get_param(req->streamid, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get stream param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	attr.bitrate = req->bitRate;
	attr.framerate = req->frameRate;
	attr.height = req->height;
	attr.width = req->width;
	attr.ngop_s = req->ngop_s;
	attr.quality = req->quality;
	if(req->quality > 80 && req->quality <= 100)
	{
		attr.maxQP = 40;
		attr.minQP = 20;
	}
	else if(req->quality > 60)
	{
		attr.maxQP = 42;
		attr.minQP = 22;
	}
	else if(req->quality > 40)
	{
		attr.maxQP = 45;
		attr.minQP = 24;
	}
	else if(req->quality > 20)
	{
		attr.maxQP = 48;
		attr.minQP = 26;
	}
	else if(req->quality > 0)
	{
		attr.maxQP = 50;
		attr.minQP = 30;
	}
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid quality param!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	if(strcmp(req->rcMode, "cbr") == 0)
		attr.rcMode = SP_VENC_RC_MODE_CBR;
	else if(strcmp(req->rcMode, "vbr") == 0)
		attr.rcMode = SP_VENC_RC_MODE_VBR;
	else if(strcmp(req->rcMode, "fixQP") == 0)
		attr.rcMode = SP_VENC_RC_MODE_FIXQP;
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid bitrate control mode!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	ret = sp_stream_set_param(req->streamid, &attr);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Set stream param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_stream_get_ability(grpc_t *grpc, PARAM_REQ_ipc_stream_get_ability *req, PARAM_RESP_ipc_stream_get_ability *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int ret = 0;
	int i = 0;
	SPStreamAbility_t ability;
	const SPHWInfo_t *ipcinfo = sp_dev_get_hwinfo();

	memset(&ability, 0, sizeof(SPStreamAbility_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	if(req->streamid < 0 || req->streamid >= ipcinfo->streamCnt)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid stream ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}

	ret = sp_stream_get_ability(req->streamid, &ability);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get stream ability failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->inputRes.width = ability.inputRes.w;
	resp->inputRes.height = ability.inputRes.h;
	resp->maxFramerate = ability.maxFramerate;
	resp->maxKBitrate = ability.maxKBitrate;
	resp->maxNGOP = ability.maxNGOP;
	resp->minFramerate = ability.minFramerate;
	resp->minKBitrate = ability.minKBitrate;
	resp->minNGOP = ability.minNGOP;
	resp->resolutions_cnt = ability.resListCnt;
	resp->resolutions = grpc_malloc(grpc, resp->resolutions_cnt*sizeof(*resp->resolutions));
	if(resp->resolutions == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	for(i=0; i<resp->resolutions_cnt; i++)
	{
		resp->resolutions[i].width = ability.resList[i].w;
		resp->resolutions[i].height = ability.resList[i].h;
	}
	
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_stream_snapshot(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot *req, PARAM_RESP_ipc_stream_snapshot *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	const char *fname = sp_snapshot_get_uri(req->channelid);
	resp->snapshot = grpc_strdup(grpc, fname);
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

static const char *base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


//base64编码，下面的编码函数占cpu高达90%，所以重写。N.B:dst长度>=len/3*4,函数内部没有检板//bytes_to_endode:编码数据指针
//in_len:数据长度
//dst:编码后的数据存放在此处，认为有足够空间保存，不做检查，调用时请注意
static void __base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, char *dst)
{
	unsigned char char_array_3[3], char_array_4[4];
	unsigned int j;
	char *p = dst + strlen(dst);

	while (in_len >= 3)
	{
		memcpy(char_array_3, bytes_to_encode, 3);
		bytes_to_encode += 3;
		in_len -= 3;
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		*p++ = base64_chars[char_array_4[0]];
		*p++ = base64_chars[char_array_4[1]];
		*p++ = base64_chars[char_array_4[2]];
		*p++ = base64_chars[char_array_4[3]];
	}

	if (in_len)
	{
		memcpy(char_array_3, bytes_to_encode, in_len);
		memset(char_array_3+in_len, '\0', 3-in_len);

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; j <= in_len; j++)
			*p++ = base64_chars[char_array_4[j]];

		while((in_len++ < 3))
			*p++ = '=';

	}
	*p = '\0';
}

int USERDEF_ipc_stream_snapshot_base64(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot_base64 *req, PARAM_RESP_ipc_stream_snapshot_base64 *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	stSPSnapSize size;
	size.nWith = req->width;
	size.nHeight = req->height;
	int maxlen = req->width*req->height*2/5; //jpeg compression ratio usually between 10:1 to 40:1.
	void *data = grpc_malloc(grpc, maxlen);
	if(data == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY;	
	}
	int retlen;


	if (data)
	{
		retlen = sp_snapshort_get_data(req->channelid, data, maxlen, &size);
		if (retlen <= 0)
		{
			maxlen *= 2;
			data = grpc_malloc(grpc, maxlen);
			if(data == NULL)
			{
				grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
				return GRPC_ERR_NO_FREE_MEMORY; 
			}
			retlen = sp_snapshort_get_data(req->channelid, data, maxlen, &size);
		}
		if (retlen > 0)
		{
			void *base64 = grpc_malloc(grpc, retlen*4/3+1);
			if(base64 == NULL)
			{
				grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
				return GRPC_ERR_NO_FREE_MEMORY; 
			}
			__base64_encode(data, retlen, base64);
			resp->format = "jpg";
			resp->snapshot = base64;
			return 0;
		}
	}

	grpc->error.errcode = GRPC_ERR_NO_FREE_MEMORY;
	return GRPC_ERR_NO_FREE_MEMORY;
#endif
}

int USERDEF_ipc_stream_request_idr(grpc_t *grpc, PARAM_REQ_ipc_stream_request_idr *req, PARAM_RESP_ipc_stream_request_idr *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	sp_stream_request_idr(req->streamid);
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_account_login(grpc_t *grpc, PARAM_REQ_ipc_account_login *req, PARAM_RESP_ipc_account_login *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_account_login_force(grpc_t *grpc, PARAM_REQ_ipc_account_login_force *req, PARAM_RESP_ipc_account_login_force *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_account_get_users(grpc_t *grpc, PARAM_REQ_ipc_account_get_users *req, PARAM_RESP_ipc_account_get_users *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	SPUser_t muser;
	int ret = 0;
	int i = 0;
	
	memset(&muser, 0, sizeof(SPUser_t));
	
	resp->users_cnt = sp_user_get_cnt();
	resp->users = grpc_malloc(grpc, resp->users_cnt * sizeof(*resp->users));
	if(resp->users == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY; 
	}
	
	for (i=0; i<resp->users_cnt; i++)
	{
		ret = sp_user_get(i, &muser);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Get accounts failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
		resp->users[i].name = grpc_strdup(grpc, muser.name);
		if(muser.level == PS_USER_LEVEL_Administrator)
			resp->users[i].level = "admin";
		else if(muser.level == PS_USER_LEVEL_Operator)
			resp->users[i].level = "operator";
		else if(muser.level == PS_USER_LEVEL_User)
			resp->users[i].level = "user";
		else if(muser.level == PS_USER_LEVEL_Anonymous)
			resp->users[i].level = "anonymous";
		else if(muser.level == PS_USER_LEVEL_Extended)
			resp->users[i].level = "extended";
		else
			resp->users[i].level = NULL;
		resp->users[i].description = grpc_strdup(grpc, muser.descript);
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_account_add_user(grpc_t *grpc, PARAM_REQ_ipc_account_add_user *req, PARAM_RESP_ipc_account_add_user *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPUser_t muser;
	int ret = 0;
	
	memset(&muser, 0, sizeof(SPUser_t));
	
	if(req->name == NULL || req->passwd == NULL || req->level == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(strlen(req->name) == 0 || strlen(req->passwd) == 0 
		|| strlen(req->name) > 12 || strlen(req->passwd) > 12
		|| strlen(req->description) > 32 )
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	strncpy(muser.name, req->name, 32);
	strncpy(muser.passwd, req->passwd, 32);
	if(strcmp(req->level, "admin") == 0)
		muser.level = PS_USER_LEVEL_Administrator;
	else if(strcmp(req->level, "operator") == 0)
		muser.level = PS_USER_LEVEL_Operator;
	else if(strcmp(req->level, "user") == 0)
		muser.level = PS_USER_LEVEL_User;
	else if(strcmp(req->level, "anonymous") == 0)
		muser.level = PS_USER_LEVEL_Anonymous;
	else if(strcmp(req->level, "extended") == 0)
		muser.level = PS_USER_LEVEL_Extended;
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account level!");
		return GRPC_ERR_INVALID_PARAMS;
	}
		
	if(req->description != NULL)
		strncpy(muser.descript, req->description, 32);
	
	ret = sp_user_add(&muser);
	if(ret != 0)
	{
		if(ret == ERR_USER_EXISTED)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Account already exists!");
		else if(ret == ERR_USER_LIMITED)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Accounts exceed limit!");
		else if(ret == ERR_USER_PERMISION_DENIED)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Permission denied!");
			
		return GRPC_ERR_OPERATION_REFUSED;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_account_del_user(grpc_t *grpc, PARAM_REQ_ipc_account_del_user *req, PARAM_RESP_ipc_account_del_user *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPUser_t muser;
	int ret = 0;
	
	memset(&muser, 0, sizeof(SPUser_t));
	if(req->name == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account name!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	if(strlen(req->name) == 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account params!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	strncpy(muser.name, req->name, 32);
	ret = sp_user_del(&muser);
	if(ret != 0)
	{
		if(ret == ERR_USER_NOTEXIST)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Account does not exist!");
		else if(ret == ERR_USER_PERMISION_DENIED)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Permission denied!");
		return GRPC_ERR_OPERATION_REFUSED;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_account_modify_user(grpc_t *grpc, PARAM_REQ_ipc_account_modify_user *req, PARAM_RESP_ipc_account_modify_user *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPUser_t muser;
	int ret = 0;
	
	memset(&muser, 0, sizeof(SPUser_t));

	if(req->name == NULL || req->passwd == NULL || req->level == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(strlen(req->name) == 0 || strlen(req->passwd) == 0 
		|| strlen(req->name) > 12 || strlen(req->passwd) > 12
		|| strlen(req->description) > 32 )
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account params!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	
	strncpy(muser.name, req->name, 32);
	strncpy(muser.passwd, req->passwd, 32);
	if(strcmp(req->level, "admin") == 0)
		muser.level = PS_USER_LEVEL_Administrator;
	else if(strcmp(req->level, "operator") == 0)
		muser.level = PS_USER_LEVEL_Operator;
	else if(strcmp(req->level, "user") == 0)
		muser.level = PS_USER_LEVEL_User;
	else if(strcmp(req->level, "anonymous") == 0)
		muser.level = PS_USER_LEVEL_Anonymous;
	else if(strcmp(req->level, "extended") == 0)
		muser.level = PS_USER_LEVEL_Extended;
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid account level!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if(req->description != NULL)
		strncpy(muser.descript, req->description, 32);
	
	ret = sp_user_set(&muser);
	if(ret != 0)
	{
		if(ret == ERR_USER_NOTEXIST)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Account does not exist!");
		else if(ret == ERR_USER_PERMISION_DENIED)
			grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Permission denied!");
		return GRPC_ERR_OPERATION_REFUSED;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_get_inet(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_get_inet *req, PARAM_RESP_ipc_ifconfig_get_inet *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	char iface[16];
	
	memset(iface, 0,sizeof(iface));
	
	sp_ifconfig_get_iface(iface);
	if(strcmp(iface, "eth0") == 0)
	{
		resp->iface = grpc_strdup(grpc, iface);
		SPEth_t eth;
		int ret = 0;
		
		ret = sp_ifconfig_eth_get(&eth);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR,"Getting ifconfig eth failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
		resp->eth.name = grpc_strdup(grpc, eth.name);
		resp->eth.addr = grpc_strdup(grpc, eth.addr);
		resp->eth.mask = grpc_strdup(grpc, eth.mask);
		resp->eth.gateway = grpc_strdup(grpc, eth.gateway);
		resp->eth.dns = grpc_strdup(grpc, eth.dns);
		resp->eth.bDHCP = eth.bDHCP;
	}
	else if(strcmp(iface, "ppp") == 0)
	{
		SPPppoe_t ppp;
		int ret = 0;

		resp->iface = grpc_strdup(grpc, iface);
		ret = sp_ifconfig_ppp_get(&ppp);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR,"Getting ifconfig pppoe failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
		resp->ppp.name = grpc_strdup(grpc, ppp.name);
		resp->ppp.username = grpc_strdup(grpc, ppp.username);
		resp->ppp.passwd = grpc_strdup(grpc, ppp.passwd);
	}
	else if(strcmp(iface, "wlan0") == 0)
	{
		resp->iface = grpc_strdup(grpc, iface);
		
		SPWifiAp_t wifiap;
		SPEth_t eth;
		int ret = 0;
		
		ret = sp_ifconfig_wifi_get_current(&wifiap);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR,"Getting ifconfig WIFI failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
		resp->wifi.name = grpc_strdup(grpc, wifiap.name);
		resp->wifi.iestat = grpc_strdup(grpc, wifiap.iestat);
		resp->wifi.quality = wifiap.quality;
		resp->wifi.keystat = wifiap.keystat;
		
		ret = sp_ifconfig_wifi_get(iface, &eth);
		if(ret != 0)
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR,"Getting ifconfig WIFI failed!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
		resp->wifi.bDHCP = eth.bDHCP;
		resp->wifi.addr = grpc_strdup(grpc, eth.addr);
		resp->wifi.mask = grpc_strdup(grpc, eth.mask);
		resp->wifi.gateway = grpc_strdup(grpc, eth.gateway);
		resp->wifi.mac = grpc_strdup(grpc, eth.mac);
		resp->wifi.dns = grpc_strdup(grpc, eth.dns);
		
	}
	else
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Getting network configuration failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
		
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_eth_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_eth_set *req, PARAM_RESP_ipc_ifconfig_eth_set *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPEth_t eth;
	int ret = 0;

	memset(&eth, 0, sizeof(SPEth_t));

	if(req->name == NULL || req->addr == NULL || req->mask == NULL || req->gateway == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid eth params!");
		return GRPC_ERR_OPERATION_REFUSED;
	}
	strncpy(eth.name, req->name, 12);
	strncpy(eth.addr, req->addr, 16);
	strncpy(eth.mask, req->mask, 16);
	strncpy(eth.gateway, req->gateway, 16);
	if(req->dns != NULL)
		strncpy(eth.dns, req->dns, 16);
	eth.bDHCP = req->bDHCP;
	ret = sp_ifconfig_eth_set(&eth);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Setting eth ifconfig failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_ppp_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_ppp_set *req, PARAM_RESP_ipc_ifconfig_ppp_set *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPPppoe_t ppp;
	int ret = 0;
	
	memset(&ppp, 0, sizeof(SPPppoe_t));

	if(req->name == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid pppoe name!");
		return GRPC_ERR_OPERATION_REFUSED;
	}
	if(req->username== NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid pppoe username!");
		return GRPC_ERR_OPERATION_REFUSED;
	}
	if(req->passwd == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid pppoe password!");
		return GRPC_ERR_OPERATION_REFUSED;
	}
	strncpy(ppp.name, req->name, 12);
	strncpy(ppp.username, req->username, 32);
	strncpy(ppp.passwd, req->passwd, 32);
	ret = sp_ifconfig_ppp_set(&ppp);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Setting pppoe ifconfig failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_wifi_connect(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_connect *req, PARAM_RESP_ipc_ifconfig_wifi_connect *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPWifiAp_t ap;
	int ret = 0;
	int i = 0;
	
	memset(&ap, 0, sizeof(SPWifiAp_t));

	if(req->name == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid wifi ssid!");
		return GRPC_ERR_OPERATION_REFUSED;
	}
	if(req->passwd == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid wifi password!");
		return GRPC_ERR_OPERATION_REFUSED;
	}
		
	strncpy(ap.name, req->name, 32);
	strncpy(ap.passwd, req->passwd, 16);
	SPWifiAp_t *list = sp_ifconfig_wifi_list_ap();
	if(list == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Getting wifi list failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	int apList_cnt = sp_ifconfig_wifi_list_cnt(list);
	for(i=0; i<apList_cnt; i++)
	{
		if(strcmp(ap.name, list[i].name) == 0)
		{
			strncpy(ap.iestat, list[i].iestat, 8);
			ap.quality = list[i].quality;
			ap.keystat = list[i].keystat;
			break;
		}
	}
	if(i == apList_cnt)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Setting wifi ifconfig failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	
	ret = sp_ifconfig_wifi_connect(&ap);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Setting wifi ifconfig failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_wifi_list_ap(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_list_ap *req, PARAM_RESP_ipc_ifconfig_wifi_list_ap *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	//int ret = 0;
	int i = 0;
	//int ap_count = 0;

	SPWifiAp_t *list = sp_ifconfig_wifi_list_ap();
	if(list == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Getting wifi list failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->apList_cnt = sp_ifconfig_wifi_list_cnt(list);
	resp->apList = grpc_malloc(grpc, resp->apList_cnt*sizeof(*resp->apList));
	if(resp->apList == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY; 
	}
	for(i=0; i<resp->apList_cnt; i++)
	{
		resp->apList[i].name = grpc_strdup(grpc, list[i].name);
		resp->apList[i].passwd= grpc_strdup(grpc, list[i].passwd);
		resp->apList[i].iestat = grpc_strdup(grpc, list[i].iestat);
		resp->apList[i].quality = list[i].quality;
		resp->apList[i].keystat = list[i].keystat;
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_server_get(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_get *req, PARAM_RESP_ipc_ifconfig_server_get *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPServer_t serverInfo;
	
	memset(&serverInfo, 0, sizeof(SPServer_t));
	sp_ifconfig_server_get(&serverInfo);
	resp->vmsServer.ipaddr = grpc_strdup(grpc, serverInfo.vmsServerIp);
	resp->vmsServer.port = serverInfo.vmsServerPort;
	resp->rtmpServer.bEnable = serverInfo.bRTMPEnable;
	resp->rtmpServer.channel = serverInfo.rtmpChannel;
	resp->rtmpServer.serverURL = grpc_strdup(grpc, serverInfo.rtmpURL);
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ifconfig_server_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_set *req, PARAM_RESP_ipc_ifconfig_server_set *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPServer_t serverInfo;
	
	memset(&serverInfo, 0, sizeof(SPServer_t));
	sp_ifconfig_server_get(&serverInfo);
	if(req->vmsServer.ipaddr != NULL)
		strcpy(serverInfo.vmsServerIp, req->vmsServer.ipaddr);
	serverInfo.vmsServerPort = req->vmsServer.port;
	serverInfo.bRTMPEnable = req->rtmpServer.bEnable;
	serverInfo.rtmpChannel = req->rtmpServer.channel;
	if(req->rtmpServer.serverURL != NULL)
		strcpy(serverInfo.rtmpURL, req->rtmpServer.serverURL);
	
	sp_ifconfig_server_set(&serverInfo);
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_record_get(grpc_t *grpc, PARAM_REQ_ipc_record_get *req, PARAM_RESP_ipc_record_get *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int cnt = 1;
	int i;
	resp->users_cnt = cnt;
	resp->users = grpc_malloc(grpc, cnt * sizeof(*resp->users));
	for (i=0;i<cnt;i++)
	{
		resp->users[i].name = grpc_strdup(grpc, "username");
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_record_set(grpc_t *grpc, PARAM_REQ_ipc_record_set *req, PARAM_RESP_ipc_record_set *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int cnt = 1;
	int i;
	resp->users_cnt = cnt;
	resp->users = grpc_malloc(grpc, cnt * sizeof(*resp->users));
	for (i=0;i<cnt;i++)
	{
		resp->users[i].name = grpc_strdup(grpc, "username");
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_privacy_get_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_get_param *req, PARAM_RESP_ipc_privacy_get_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPRegion_t region;
	int ret = 0;
	int i = 0;
	int j = 0;
	
	memset(&region, 0, sizeof(SPRegion_t));
	
	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	ret = sp_privacy_get_param(req->channelid, &region);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Getting privacy param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	resp->bEnable = region.bEnable;
	for(i=0; i<MAX_PYRGN_NUM; i++)
	{
		if(region.stRect[i].w != 0)
			resp->rects_cnt++;
	}
	resp->rects = grpc_malloc(grpc, resp->rects_cnt*sizeof(*resp->rects));
	if(resp->rects == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY; 
	}
	for(i=0; i<MAX_PYRGN_NUM; i++)
	{
		if(region.stRect[i].w != 0)
		{
			resp->rects[j].x = region.stRect[i].x;
			resp->rects[j].y = region.stRect[i].y;
			resp->rects[j].w = region.stRect[i].w;
			resp->rects[j].h = region.stRect[i].h;
			j++;
		}
	}
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_privacy_set_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_set_param *req, PARAM_RESP_ipc_privacy_set_param *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	SPRegion_t region;
	int ret = 0;
	int i = 0;
	
	memset(&region, 0, sizeof(SPRegion_t));

	if(req->channelid != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid channel ID!");
		return GRPC_ERR_INVALID_PARAMS;		
	}
	
	ret = sp_privacy_get_param(req->channelid, &region);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Getting privacy param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}
	region.bEnable = req->region.bEnable;
	memset(region.stRect, 0, sizeof(region.stRect));
	for(i=0; i<req->region.rects_cnt; i++)
	{
		if(i >= MAX_PYRGN_NUM)
			break;
		if(req->region.rects != NULL)
		{
			region.stRect[i].x = req->region.rects[i].x;
			region.stRect[i].y = req->region.rects[i].y;
			region.stRect[i].w = req->region.rects[i].w;
			region.stRect[i].h = req->region.rects[i].h;
		}
	}
	ret = sp_privacy_set_param(req->channelid, &region);
	if(ret != 0)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Setting privacy param failed!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}


int USERDEF_ipc_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_get_record_list *req, PARAM_RESP_ipc_get_record_list *resp)
{
#if (!ALARM_SERVICE_SUPPORT || !SD_RECORD_SUPPORT)
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	S32 i = 0;
	S8 SearchResult[MAX_RESULT_CNT * LEN_PER_RESULT + 1] = {0};
	S8 Partition = 0, VideoType = 0;
	S32 nCh = 0, Date = 0, Time = 0;

	if ((NULL == req) || (NULL == req->starttime) || (NULL == req->endtime))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or start time or endtime!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	resp->recordlist_cnt = NAPlay_SearchVideo(req->channelid, req->starttime, req->endtime, SearchResult, sizeof(SearchResult));

	if (-1 == resp->recordlist_cnt)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Start time or end time format error!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	else if (-2 == resp->recordlist_cnt)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Too many results!");
		return GRPC_ERR_INTERNAL_ERROR;
	}

	resp->recordlist = grpc_malloc(grpc, sizeof(*(resp->recordlist)) * resp->recordlist_cnt);
	if(resp->recordlist == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY; 
	}

	for (i = 0; i < resp->recordlist_cnt; ++i)
	{
		// 格式: 通道(2)|录像日期(8)|录像时间(6)|录像类型(1)|分区序号(1)
		sscanf(SearchResult + i * LEN_PER_RESULT, "%02d%8d%6d%c%c", &nCh, &Date, &Time, &VideoType, &Partition);

		resp->recordlist[i].date = (char*)grpc_malloc(grpc, 9); 		// 录像日期: 8Byte
		if(resp->recordlist[i].date == NULL)
		{
			grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
			return GRPC_ERR_NO_FREE_MEMORY; 
		}
		resp->recordlist[i].filename = (char*)grpc_malloc(grpc, 12);	// 分区序号: 1Byte, "/": 1Byte, 录像类型1Byte, 通道号: 2Byte, 录像时间6Byte
		if(resp->recordlist[i].filename == NULL)
		{
			grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
			return GRPC_ERR_NO_FREE_MEMORY; 
		}

		resp->recordlist[i].channelid = nCh - 1;						// 录像通道，从0开始
		sprintf(resp->recordlist[i].date, "%8d", Date);
		sprintf(resp->recordlist[i].filename, "%02d/%c%02d%06d", Partition - 'C', VideoType, nCh, Time);
	}

#endif

	return 0;
}

int USERDEF_ipc_get_audio_status(grpc_t *grpc, PARAM_REQ_ipc_get_audio_status *req, PARAM_RESP_ipc_get_audio_status *resp)
{
#if (!ALARM_SERVICE_SUPPORT || !SD_RECORD_SUPPORT)
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	CPlayerStatus_t PlayerState;

	if ((NULL == req) || (NULL == req->session) || (0 == strlen(req->session)))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or session!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if (NULL == resp)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid resp!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	if (0 != NAPlay_GetPlayStatus(req->session, &PlayerState))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid session!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	resp->playing = !PlayerState.bMute;
#endif

	return 0;
}

int USERDEF_ipc_set_audio_status(grpc_t *grpc, PARAM_REQ_ipc_set_audio_status *req, PARAM_RESP_ipc_set_audio_status *resp)
{
#if (!ALARM_SERVICE_SUPPORT || !SD_RECORD_SUPPORT)
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	CPLAYER_CMD_e PlayCmd = CPLAYER_CMD_MAX;

	if ((NULL == req) || (NULL == req->session) || (0 == strlen(req->session)))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or session!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	PlayCmd = (req->play) ? CPLAYER_CMD_UNMUTE : CPLAYER_CMD_MUTE;

	if (0 != NAPlay_ChgPlayState(req->session, PlayCmd, (void*)0))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid session!");
		return GRPC_ERR_INVALID_PARAMS;
	}
#endif

	return 0;
}

int USERDEF_ipc_play_record(grpc_t *grpc, PARAM_REQ_ipc_play_record *req, PARAM_RESP_ipc_play_record *resp)
{
#if (!ALARM_SERVICE_SUPPORT || !SD_RECORD_SUPPORT)
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	CPLAYER_CMD_e PlayCmd = CPLAYER_CMD_MAX;
	S32 Param[] = {-1, 0};		// Frame、Speed

	if ((NULL == req) || (NULL == req->session) || (0 == strlen(req->session)))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or session!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	switch (req->status)
	{
	case 0:			// 连续播放模式
		if (0 == req->speed)
		{
			PlayCmd = CPLAYER_CMD_PLAY;
		}
		else
		{
			PlayCmd = CPLAYER_CMD_SPEED;
			Param[1] = req->speed;
		}

		if (req->frame >= 0)
		{
			Param[0] = req->frame;
		}
		break;
	case 1:			// 暂停
		PlayCmd = CPLAYER_CMD_PAUSE;
		break;
	case 2:			// 继续
		PlayCmd = CPLAYER_CMD_RESUME;
		break;
	case 3:			// 定位到frame帧
		PlayCmd = CPLAYER_CMD_SEEK;
		Param[0] = req->frame;
		break;
	case 4:			// 停止播放
		if (0 == NAPlay_StopPlay(req->session))
		{
			return 0;
		}
		else
		{
			grpc_s_set_error(grpc, GRPC_ERR_INTERNAL_ERROR, "Invalid session!");
			return GRPC_ERR_INTERNAL_ERROR;
		}
	default:
		break;
	}

	switch (NAPlay_ChgPlayState(req->session, PlayCmd, (void*)Param))
	{
	case RET_BADPARAM:
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid session!");
		return GRPC_ERR_INVALID_PARAMS;
	case RET_OPERATION_REFUSED:
		grpc_s_set_error(grpc, GRPC_ERR_OPERATION_REFUSED, "Cannot find I frame");
		return GRPC_ERR_OPERATION_REFUSED;
	default:
		break;
	}

#endif

	return 0;
}

int USERDEF_ipc_play_record_over(grpc_t *grpc, PARAM_REQ_ipc_play_record_over *req, PARAM_RESP_ipc_play_record_over *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int cnt = 1;
	int i;
	resp->users_cnt = cnt;
	VOID_PTR_DECLARE(resp->users) = grpc_malloc(grpc, cnt * sizeof(*resp->users));
	for (i=0;i<cnt;i++)
	{
		resp->users[i].name = grpc_strdup(grpc, "username");
	}

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_get_record_info(grpc_t *grpc, PARAM_REQ_ipc_get_record_info *req, PARAM_RESP_ipc_get_record_info *resp)
{
#if (!ALARM_SERVICE_SUPPORT || !SD_RECORD_SUPPORT)
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	CPlayerStatus_t PlayerState;

	if ((NULL == req) || (NULL == req->session) || (0 == strlen(req->session)))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or session!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	if (NULL == resp)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid resp!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	if (0 != NAPlay_GetPlayStatus(req->session, &PlayerState))
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid session!");
		return GRPC_ERR_INVALID_PARAMS;
	}

	resp->totalframe = PlayerState.nTotalFrame;
	resp->currframe = PlayerState.nCurFrame;

#endif

	return 0;
}

int USERDEF_ipc_ivp_start(grpc_t *grpc, PARAM_REQ_ipc_ivp_start *req, PARAM_RESP_ipc_ivp_start *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	sp_ivp_start(req->channelid);
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ivp_stop(grpc_t *grpc, PARAM_REQ_ipc_ivp_stop *req, PARAM_RESP_ipc_ivp_stop *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	sp_ivp_stop(req->channelid);

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}


int USERDEF_ipc_ivp_get_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_get_param *req, PARAM_RESP_ipc_ivp_get_param *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
	int i = 0;
	int j = 0;
	SPIVP_t param;
	
	memset(&param, 0, sizeof(SPIVP_t));
	sp_ivp_get_param(req->channelid, &param);
	
	resp->bEnable = param.bEnable;
	resp->nDelay = param.nDelay;
	resp->bStarting = param.bStarting;
	resp->nRgnCnt = param.nRgnCnt;
	resp->stRegion_cnt = 4;
	resp->stRegion = grpc_malloc(grpc, resp->stRegion_cnt * sizeof(*resp->stRegion));
	if(resp->stRegion == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
		return GRPC_ERR_NO_FREE_MEMORY; 
	}
	for(i=0; i<resp->stRegion_cnt; i++)
	{
		resp->stRegion[i].nCnt = param.stRegion[i].nCnt;
		resp->stRegion[i].stPoints_cnt = 20;
		resp->stRegion[i].stPoints = grpc_malloc(grpc, resp->stRegion[i].stPoints_cnt * sizeof(*resp->stRegion[i].stPoints));
		if(resp->stRegion[i].stPoints == NULL)
		{
			grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
			return GRPC_ERR_NO_FREE_MEMORY; 
		}
		for(j=0; j<resp->stRegion[i].stPoints_cnt; j++)
		{
			resp->stRegion[i].stPoints[j].x = param.stRegion[i].stPoints[j].x;
			resp->stRegion[i].stPoints[j].y = param.stRegion[i].stPoints[j].y;
		}
		resp->stRegion[i].nIvpCheckMode = param.stRegion[i].nIvpCheckMode;
	}
	resp->bDrawFrame = param.bDrawFrame;
	resp->bFlushFrame = param.bFlushFrame;
	resp->bMarkObject = param.bMarkObject;
	resp->bMarkAll = param.bMarkAll;
	resp->bOpenCount = param.bOpenCount;
	resp->bShowCount = param.bShowCount;
	resp->bPlateSnap = param.bPlateSnap;
	resp->nAlpha = param.nAlpha;
	resp->nSen = param.nSen;
	resp->nThreshold = param.nThreshold;
	resp->nStayTime = param.nStayTime;
	resp->bEnableRecord = param.bEnableRecord;
	resp->bOutAlarm1 = param.bOutAlarm1;
	resp->bOutClient = param.bOutClient;
	resp->bOutEMail = param.bOutEMail;
	resp->bOutVMS = param.bOutVMS;
	resp->bNeedRestart = param.bNeedRestart;
	resp->eCountOSDPos = param.eCountOSDPos;
	resp->nCountOSDColor = param.nCountOSDColor;
	resp->nCountSaveDays = param.nCountSaveDays;
	resp->nTimeIntervalReport = param.nTimeIntervalReport;
	resp->sSnapRes = grpc_strdup(grpc, param.sSnapRes);
	resp->bLPREn = param.bLPREn;
	resp->ivpLprDir = param.ivpLprDir;
	resp->bIvpLprDisplay = param.bIvpLprDisplay;
	resp->ivpLprPos = param.ivpLprPos;
	resp->ivpLprROI.x = param.ivpLprROI.x;
	resp->ivpLprROI.y = param.ivpLprROI.y;
	resp->ivpLprROI.width = param.ivpLprROI.width;
	resp->ivpLprROI.height = param.ivpLprROI.height;
	resp->ivpLprHttpServer.ivpLprHttpIP = grpc_strdup(grpc, param.ivpLprHttpServer.ivpLprHttpIP);
	resp->ivpLprHttpServer.ivpLprHttpPort = param.ivpLprHttpServer.ivpLprHttpPort;
	resp->ivpLprHttpServer.ivpLprHttpAddr = grpc_strdup(grpc, param.ivpLprHttpServer.ivpLprHttpAddr);
	resp->ivpLprFtpServer.ivpLprFtpIP = grpc_strdup(grpc, param.ivpLprFtpServer.ivpLprFtpIP);
	resp->ivpLprFtpServer.ivpLprFtpPort = param.ivpLprFtpServer.ivpLprFtpPort;
	resp->ivpLprFtpServer.ivpLprFtpAccount = grpc_strdup(grpc, param.ivpLprFtpServer.ivpLprFtpAccount);
	resp->ivpLprFtpServer.ivpLprFtpPasswd = grpc_strdup(grpc, param.ivpLprFtpServer.ivpLprFtpPasswd);
	resp->ivpLprFtpServer.ivpLprFtpDir = grpc_strdup(grpc, param.ivpLprFtpServer.ivpLprFtpDir);
	resp->ivpLprReUploadInt = param.ivpLprReUploadInt;
	resp->bIvpLprImgFull = param.bIvpLprImgFull;
	resp->bIvpLprImgLP = param.bIvpLprImgLP;
	resp->uploadTimeout = param.uploadTimeout;
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ivp_set_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_set_param *req, PARAM_RESP_ipc_ivp_set_param *resp)
{
#if 1
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

	int i = 0;
	int j = 0;
	SPIVP_t param;
	
	memset(&param, 0, sizeof(SPIVP_t));
	sp_ivp_get_param(req->channelid, &param);
	
	param.bEnable = req->bEnable;
	param.nDelay = req->nDelay;
	param.bStarting = req->bStarting;
	param.nRgnCnt = req->nRgnCnt;
	if(req->stRegion != NULL)
	{
		for(i=0; i<req->stRegion_cnt && i<4; i++)
		{
			param.stRegion[i].nCnt = req->stRegion[i].nCnt;
			param.stRegion[i].nIvpCheckMode = req->stRegion[i].nIvpCheckMode;
			if(param.stRegion[i].stPoints != NULL)
			{
				for(j=0; j<req->stRegion[i].stPoints_cnt && j<20; j++)
				{
					param.stRegion[i].stPoints[j].x = req->stRegion[i].stPoints[j].x;
					param.stRegion[i].stPoints[j].y = req->stRegion[i].stPoints[j].y;
				}
			}
		}
	}
	param.bDrawFrame = req->bDrawFrame;
	param.bFlushFrame = req->bFlushFrame;
	param.bMarkObject = req->bMarkObject;
	param.bMarkAll = req->bMarkAll;
	param.bOpenCount = req->bOpenCount;
	param.bShowCount = req->bShowCount;
	param.bPlateSnap = req->bPlateSnap;
	param.nAlpha = req->nAlpha;
	param.nSen = req->nSen;
	param.nThreshold = req->nThreshold;
	param.nStayTime = req->nStayTime;
	param.bEnableRecord = req->bEnableRecord;
	param.bOutAlarm1 = req->bOutAlarm1;
	param.bOutClient = req->bOutClient;
	param.bOutEMail = req->bOutEMail;
	param.bOutVMS = req->bOutVMS;
	param.bNeedRestart = req->bNeedRestart;
	param.eCountOSDPos = req->eCountOSDPos;
	param.nCountOSDColor = req->nCountOSDColor;
	param.nCountSaveDays = req->nCountSaveDays;
	param.nTimeIntervalReport = req->nTimeIntervalReport;
	if(req->sSnapRes != NULL)
		strncpy(param.sSnapRes, req->sSnapRes, sizeof(param.sSnapRes)-1);
	param.bLPREn = req->bLPREn;
	param.ivpLprDir = req->ivpLprDir;
	param.bIvpLprDisplay = req->bIvpLprDisplay;
	param.ivpLprPos = req->ivpLprPos;
	param.ivpLprROI.x = req->ivpLprROI.x;
	param.ivpLprROI.y = req->ivpLprROI.y;
	param.ivpLprROI.width = req->ivpLprROI.width;
	param.ivpLprROI.height = req->ivpLprROI.height;
	param.ivpLprReUploadInt = req->ivpLprReUploadInt;
	param.bIvpLprImgFull = req->bIvpLprImgFull;
	param.bIvpLprImgLP = req->bIvpLprImgLP;
	param.uploadTimeout = req->uploadTimeout;

	if(req->ivpLprHttpServer.ivpLprHttpIP != NULL)
		strncpy(param.ivpLprHttpServer.ivpLprHttpIP, req->ivpLprHttpServer.ivpLprHttpIP, sizeof(param.ivpLprHttpServer.ivpLprHttpIP)-1);
	param.ivpLprHttpServer.ivpLprHttpPort = req->ivpLprHttpServer.ivpLprHttpPort;
	if(req->ivpLprHttpServer.ivpLprHttpAddr != NULL)
		strncpy(param.ivpLprHttpServer.ivpLprHttpAddr, req->ivpLprHttpServer.ivpLprHttpAddr, sizeof(param.ivpLprHttpServer.ivpLprHttpAddr)-1);

	if(req->ivpLprFtpServer.ivpLprFtpIP != NULL)
		strncpy(param.ivpLprFtpServer.ivpLprFtpIP, req->ivpLprFtpServer.ivpLprFtpIP, sizeof(param.ivpLprFtpServer.ivpLprFtpIP)-1);
	param.ivpLprFtpServer.ivpLprFtpPort = req->ivpLprFtpServer.ivpLprFtpPort;
	if(req->ivpLprFtpServer.ivpLprFtpAccount != NULL)
		strncpy(param.ivpLprFtpServer.ivpLprFtpAccount, req->ivpLprFtpServer.ivpLprFtpAccount, sizeof(param.ivpLprFtpServer.ivpLprFtpAccount)-1);
	if(req->ivpLprFtpServer.ivpLprFtpPasswd != NULL)
		strncpy(param.ivpLprFtpServer.ivpLprFtpPasswd, req->ivpLprFtpServer.ivpLprFtpPasswd, sizeof(param.ivpLprFtpServer.ivpLprFtpPasswd)-1);
	if(req->ivpLprFtpServer.ivpLprFtpDir != NULL)
		strncpy(param.ivpLprFtpServer.ivpLprFtpDir, req->ivpLprFtpServer.ivpLprFtpDir, sizeof(param.ivpLprFtpServer.ivpLprFtpDir)-1);
	sp_ivp_set_param(req->channelid, &param);

	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ivp_lpr_trigger(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_trigger *req, PARAM_RESP_ipc_ivp_lpr_trigger *resp)
{
#if 0
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
	
		//grpc_set_error(grpc, 0, );
#endif
		return 0;
}

int USERDEF_ipc_ivp_lpr_import_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_import_wblist *req, PARAM_RESP_ipc_ivp_lpr_import_wblist *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();
#ifdef IWISDOM_LPR
	if(req == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or session!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	FILE *fp = NULL;
	int i = 0;
	char buf[32] = {0};
	
	//printf("total:%d\n", req->whiteList_cnt);
	if(req->whiteList != NULL)
	{
		fp = fopen(IVP_LPR_WHITELIST, "w+");
		if(fp != NULL)
		{
			sprintf(buf, "total:%d\n", req->whiteList_cnt);
			fwrite(buf, 1, strlen(buf), fp);
			for(i=0; i<req->whiteList_cnt; i++)
			{
				
				snprintf(buf, 31, "%-8.8s,%-8.8s\n", req->whiteList[i].lpstr, req->whiteList[i].expDate);
				fwrite(buf, 1, strlen(buf), fp);
			}
			fclose(fp);
		}
	}
	//printf("total:%d\n", req->blackList_cnt);
	if(req->blackList != NULL)
	{
		fp = fopen(IVP_LPR_BLACKLIST, "w+");
		if(fp != NULL)
		{
			sprintf(buf, "total:%d\n", req->blackList_cnt);
			fwrite(buf, 1, strlen(buf), fp);
			for(i=0; i<req->blackList_cnt; i++)
			{
				snprintf(buf, 31, "%-8.8s,%-8.8s\n", req->blackList[i].lpstr, req->blackList[i].expDate);
				fwrite(buf, 1, strlen(buf), fp);
			}
			fclose(fp);
		}
	}
#endif
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ivp_lpr_export_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_export_wblist *req, PARAM_RESP_ipc_ivp_lpr_export_wblist *resp)
{
#if 0
	__NULL_FUNC_DBG__();
	grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
	return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
	__GENERATE_FUNC_DEBUG__();

#ifdef IWISDOM_LPR
	if(req == NULL)
	{
		grpc_s_set_error(grpc, GRPC_ERR_INVALID_PARAMS, "Invalid req or session!");
		return GRPC_ERR_INVALID_PARAMS;
	}
	FILE *fp = NULL;
	char buf[32] = {0};
	int i = 0;
	
	if(access(IVP_LPR_WHITELIST, F_OK) == 0)
	{
		struct stat statbuf;
		if(stat(IVP_LPR_WHITELIST, &statbuf) == 0)
		{
			fp = fopen(IVP_LPR_WHITELIST, "r");
			if(fp != NULL)
			{
				fgets(buf, sizeof(buf), fp);
				//printf("w buf:\n%s\n", buf);
				char *num = strchr(buf, ':');
				if(num != NULL)
				{
					resp->whiteList_cnt = atoi(num+1);
					//printf("whiteList_cnt=%d\n", resp->whiteList_cnt);
					resp->whiteList = grpc_malloc(grpc, resp->whiteList_cnt*sizeof(*(resp->whiteList)));
					if(resp->whiteList == NULL)
					{
						grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
						fclose(fp);
						return GRPC_ERR_NO_FREE_MEMORY; 
					}
					while (!feof(fp))
					{
						if(i >= resp->whiteList_cnt)
							break;
						fgets(buf, sizeof(buf), fp);
						//printf("buf:\n%s\n", buf);
						char *expDate = strchr(buf, ',');
						if(expDate != NULL)
						{
							*expDate = '\0';
							expDate++;
							char *end = strchr(expDate, '\n');
							if(end != NULL)
							{
								*end = '\0';
								//printf("buf:\n%s\n", buf);
								//printf("expDate:\n%s\n", expDate);
								resp->whiteList[i].lpstr = grpc_strdup(grpc, buf);
								resp->whiteList[i].expDate = grpc_strdup(grpc, expDate);
								i++;
							}
						}
					}
					for(;i<resp->whiteList_cnt; i++)
					{
						resp->whiteList[i].lpstr = grpc_strdup(grpc, "");
						resp->whiteList[i].expDate = grpc_strdup(grpc, "");						
					}
				}
				fclose(fp);
			}
		}
	}
	if(access(IVP_LPR_BLACKLIST, F_OK) == 0)
	{
		struct stat statbuf;
		if(stat(IVP_LPR_BLACKLIST, &statbuf) == 0)
		{
			fp = fopen(IVP_LPR_BLACKLIST, "r");
			if(fp != NULL)
			{
				memset(buf, 0, sizeof(buf));
				fgets(buf, sizeof(buf), fp);
				//printf("b buf:\n%s\n", buf);
				char *num = strchr(buf, ':');
				if(num != NULL)
				{
					resp->blackList_cnt= atoi(num+1);
					//printf("blackList_cnt=%d\n", resp->blackList_cnt);
					i = 0;
					resp->blackList = grpc_malloc(grpc, resp->blackList_cnt*sizeof(*(resp->blackList)));
					if(resp->blackList == NULL)
					{
						grpc_s_set_error(grpc, GRPC_ERR_NO_FREE_MEMORY, "No free memory!");
						fclose(fp);
						return GRPC_ERR_NO_FREE_MEMORY; 
					}
					while (!feof(fp))
					{
						if(i >= resp->blackList_cnt)
							break;
						fgets(buf, sizeof(buf), fp);
						//printf("buf:\n%s\n", buf);
						char *expDate = strchr(buf, ',');
						if(expDate != NULL)
						{
							*expDate = '\0';
							expDate++;
							char *end = strchr(expDate, '\n');
							if(end != NULL)
							{
								*end = '\0';
								//printf("buf:\n%s\n", buf);
								//printf("expDate:\n%s\n", expDate);
								resp->blackList[i].lpstr = grpc_strdup(grpc, buf);
								resp->blackList[i].expDate = grpc_strdup(grpc, expDate);
								i++;
							}
						}
					}
					for(;i<resp->blackList_cnt; i++)
					{
						resp->blackList[i].lpstr = grpc_strdup(grpc, "");
						resp->blackList[i].expDate = grpc_strdup(grpc, "");						
					}
				}
				fclose(fp);
			}
		}
	}
#endif
	//grpc_set_error(grpc, 0, );
#endif

	return 0;
}

int USERDEF_ipc_ivp_lpr_manual_open_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_open_gate *req, PARAM_RESP_ipc_ivp_lpr_manual_open_gate *resp)
{
#if 1
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
		SPAlarmIn_t alarm;
		sp_alarmin_get_param(0, &alarm);
		if(alarm.nGuardChn == 0)
		{
			sp_alarm_buzzing_open();
		}
	
		//grpc_set_error(grpc, 0, );
#endif
	
		return 0;	
}

int USERDEF_ipc_ivp_lpr_manual_close_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_close_gate *req, PARAM_RESP_ipc_ivp_lpr_manual_close_gate *resp)
{
#if 1
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
		SPAlarmIn_t alarm;
		sp_alarmin_get_param(0, &alarm);
		if(alarm.nGuardChn == 0)
		{
			sp_alarm_buzzing_close();
		}
	
		//grpc_set_error(grpc, 0, );
#endif
	
		return 0;	
}

int USERDEF_ipc_ivp_lpr_get_last_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_last_record *req, PARAM_RESP_ipc_ivp_lpr_get_last_record *resp)
{
#if 0
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
#if (defined IWISDOM_LPR)
		MIVP_LPR_RESULT_t result;
		memset(&result, 0, sizeof(MIVP_LPR_RESULT_t));
		mivp_lpr_get_last_result(&result);
		char imgPath[32];//http://192.168.5.146/cgi-bin/snapshot/lprimg.jpeg
		char *picName = strrchr(result.imgPath, '/');
		if(picName != NULL)
		{
			sprintf(imgPath, "/cgi-bin/snapshot/%s", picName+1);
		}
		else
		{
			sprintf(imgPath, "/cgi-bin/snapshot/%s", result.imgPath);
		}

		resp->recordID = 0;
		resp->bright = 0;
		resp->carBright = 0;
		resp->carColor = 0;
		resp->colorType = 0;
		resp->colorValue = 0;
		resp->confidence = 100;
		resp->direction = 0;
		resp->imagePath = grpc_strdup(grpc, imgPath);
		resp->license = grpc_strdup(grpc, result.lp);
		resp->location.RECT.left = result.lp_location.x;
		resp->location.RECT.top = result.lp_location.y;
		resp->location.RECT.right = result.lp_location.width;
		resp->location.RECT.bottom = result.lp_location.height;
		resp->lpImagePath = grpc_strdup(grpc, "");
		resp->timeStamp.Timeval.sec = 0;
		resp->timeStamp.Timeval.usec = 0;
		resp->timeUsed = 0;
		resp->triggerType = 1;
		resp->type = 0;
#endif
		//grpc_set_error(grpc, 0, );
#endif
	
		return 0;	
}

int USERDEF_ipc_ivp_lpr_get_max_record_id(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_max_record_id *req, PARAM_RESP_ipc_ivp_lpr_get_max_record_id *resp)
{
#if 0
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
	
		//grpc_set_error(grpc, 0, );
#endif
	
		return 0;	
}

int USERDEF_ipc_ivp_lpr_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record_list *req, PARAM_RESP_ipc_ivp_lpr_get_record_list *resp)
{
#if 0
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
	
		//grpc_set_error(grpc, 0, );
#endif
	
		return 0;	
}

int USERDEF_ipc_ivp_lpr_get_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record *req, PARAM_RESP_ipc_ivp_lpr_get_record *resp)
{
#if 0
		__NULL_FUNC_DBG__();
		grpc_s_set_error(grpc, GRPC_ERR_METHOD_NOT_IMPLEMENTED, "Method not implemented");
		return GRPC_ERR_METHOD_NOT_IMPLEMENTED;
#else
		__GENERATE_FUNC_DEBUG__();
	
		//grpc_set_error(grpc, 0, );
#endif
	
		return 0;	
}
