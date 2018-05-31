#include "mcloud.h"
#include "jv_common.h"
#include "JvServer.h"
#include "sctrl.h"
#include "mipcinfo.h"
#include "jhlsupload.h"
#include "cJSON.h"
#include "utl_ifconfig.h"
#include "mtransmit.h"
#include "malarmout.h"
#include "httpclient.h"

#define OBSS_GET_URL  "http://xwcs.cloudsee.net:9888/publicService/csManage/csTokenInfo.do?deviceGuid="
#define OBSS_INFO_FILE "/tmp/obss_info"

#define IsSystemFail(ret) (-1 == (ret) || 0 == WIFEXITED((ret)) || 0 != WEXITSTATUS((ret)))

//上传路径/days/ystNO/time/filename
OBSS_INFO obss_info;
static CLOUD cloudCfg;

static char s_mcloud_get_token = 1;


#ifdef OBSS_CLOUDSTORAGE

static int _mcloud_parse_obssinfofile(char* filePath,OBSS_INFO* obss_info)
{
	if(NULL == filePath)
		return -1;

	char buf1[1024] = {0};
	char buf[1024] = {0};

	char* tmp_s = NULL;
	char* tmp_e = NULL;

	int fd = -1;
	int readlen = 0;

	if(0 != access(filePath,F_OK))
	{
		printf("mcloud info file %s access failed!\n",filePath);
		return -1;
	}

	FILE *f=fopen(filePath,"rb");
	fseek(f,0,SEEK_END);
	long len=ftell(f);
	fseek(f,0,SEEK_SET);
	char *data=malloc(len+1);
	fread(data,1,len,f);
	fclose(f);

	cJSON *json;
	
	json=cJSON_Parse(data);
	if (!json) {printf("Error before: [%s]\n",cJSON_GetErrorPtr());}
	else
	{
		cJSON *root = cJSON_GetObjectItem(json, "root");
		const char *result = cJSON_GetObjectValueString(root, "result");
		if (NULL != result && strcmp(result, "true") == 0)
		{
			cJSON *param = cJSON_GetObjectItem(root, "data");
			if(NULL != param)
			{
				const char *bucketName = cJSON_GetObjectValueString(param, "bucketName");
				if (bucketName != NULL)
					strncpy(obss_info->bucketName, bucketName, 31);
				else
					return -1;
				
				const char *endpoint = cJSON_GetObjectValueString(param, "endpoint");
				if (endpoint != NULL)
					strncpy(obss_info->endpoint, endpoint, 63);
				else
					return -1;
				
				const char *deviceGuid = cJSON_GetObjectValueString(param, "deviceGuid");
				if (deviceGuid != NULL)
					strncpy(obss_info->deviceGuid, deviceGuid, 15);
				else
					return -1;
				
				const char *keyId = cJSON_GetObjectValueString(param, "keyId");
				if (keyId != NULL)
					strncpy(obss_info->keyId, keyId, 63);
				else
					return -1;
				
				const char *expiration = cJSON_GetObjectValueString(param, "expiration");
				if (expiration != NULL)
					strncpy(obss_info->expiration, expiration,63);
				else
					return -1;
				
				const char *keySecret = cJSON_GetObjectValueString(param, "keySecret");
				if (keySecret != NULL)
					strncpy(obss_info->keySecret, keySecret, 127);
				else
					return -1;
				
				const char *token = cJSON_GetObjectValueString(param, "token");
				if (token != NULL)
					strncpy(obss_info->token, token, 1023);
				else
					return -1;
				
				const char *days = cJSON_GetObjectValueString(param, "days");
				if (days != NULL)
					strncpy(obss_info->days, days, 16);
				else
					return -1;
				
				const char *type = cJSON_GetObjectValueString(param, "type");
				if (type != NULL)
					strncpy(obss_info->type, type, 8);
				else
					return -1;
			}
			cloudCfg.bEnable = TRUE;
		}
		else
		{
			cloudCfg.bEnable = FALSE;
			return -1;
		}
		
	}


	free(data);

	return 0;
}

static int _mcloud_get_obssinfo(OBSS_INFO* obss_info)
{
	int ret;
	char url[128] = {0};
	ipcinfo_t ipcinfo;
	char ystNo[64] = {0};
	ipcinfo_get_param(&ipcinfo);
	jv_ystNum_parse(ystNo, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);

	sprintf(url,OBSS_GET_URL"%s",ystNo);
	char path_file[64] = {0};
	strcpy(path_file, OBSS_INFO_FILE);
	char *pfile = NULL;
	pfile = strrchr(path_file, '/');
	*pfile = 0;
	ret = http_download(url, path_file, pfile+1, NULL, NULL);
	if(ret == -1)
	{
		printf("download file(http) failed!\n");
		return -2;
	}

	return _mcloud_parse_obssinfofile(OBSS_INFO_FILE,obss_info);
	
}

int mcloud_parse_obssstate(char *data)
{
	cJSON *json = cJSON_Parse(data);

	if (!json)
	{
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
	}
	else
	{
		const char *state = cJSON_GetObjectValueString(json, "status");
		if (state != NULL)
		{
			if (atoi(state))
			{
				s_mcloud_get_token = 1;
			}
			else
			{
				cloudCfg.bEnable = FALSE;
			}
		}
		else
		{
			printf("cJSON_GetObjectValueString failed!\n");
		}

		cJSON_Delete(json);
	}

	return 0;
}

static int _mcloud_parse_obssstatefile(char* filePath)
{
	if(NULL == filePath)
		return -1;

	char buf1[1024] = {0};
	char buf[1024] = {0};

	char* tmp_s = NULL;
	char* tmp_e = NULL;

	int fd = -1;
	int readlen = 0;

	if(0 != access(filePath,F_OK))
	{
		printf("mcloud file %s access failed!\n",filePath);
		return -1;
	}

	FILE *f=fopen(filePath,"rb");
	fseek(f,0,SEEK_END);
	long len=ftell(f);
	fseek(f,0,SEEK_SET);
	char *data=malloc(len+1);
	fread(data,1,len,f);
	fclose(f);

	mcloud_parse_obssstate(data);
	
	free(data);

	return 0;
}

static void mcloud_update_thread(void* p)
{
	BOOL bObss_init = FALSE;
	int count = 0;
	int ret = 0;

	pthreadinfo_add((char *)__func__);
	
	while (!utl_ifconfig_net_prepared())
	{
		sleep(1);
	}

	while (1)
	{
		count = 0;
		if (s_mcloud_get_token == 1 || cloudCfg.bEnable == TRUE)
		{
			s_mcloud_get_token = 0;
			while (1)
			{
				ret = _mcloud_get_obssinfo(&obss_info);
				if(ret == 0)
				{
					Printf("backname : %s \n endpoint : %s \n keyId : %s \n keySecret : %s \n token : %s \n deviceGuid : %s \n days : %s \n type : %s\n",
						obss_info.bucketName,obss_info.endpoint,obss_info.keyId,obss_info.keySecret,obss_info.token,obss_info.deviceGuid,obss_info.days,obss_info.type);
					if(!bObss_init)
					{
						if(0 == jhlsup_init(obss_info.endpoint,80,obss_info.keyId,obss_info.keySecret,obss_info.token,obss_info.deviceGuid,obss_info.bucketName,3))
							bObss_init = TRUE;
					}
					else
					{
						jhlsup_reset(obss_info.keyId,obss_info.keySecret,obss_info.token);
					}
					break;
				}
				else if (ret == -1)
				{
					break;
				}
				else if (ret == -2)
				{
					cloudCfg.bEnable = FALSE;
					sleep(5*60);
				}
			}
		}

		while (s_mcloud_get_token == 0 && count < (50*60))
		{
			count++;
			sleep(1);
		}
	}
}

#endif

int mcloud_init()
{
#ifdef OBSS_CLOUDSTORAGE

	if (hwinfo.bSupportXWCloud == TRUE)
	{
		pthread_create_detached(NULL, NULL, (void*)mcloud_update_thread, NULL);
	}
	
#endif
	return 0;
}

int mcloud_deinit()
{
#ifdef OBSS_CLOUDSTORAGE
	
	jhlsup_deinit();

#endif

	return 0;
}

int mcloud_get_param(CLOUD *cloud)
{
	jv_assert(cloud != NULL, return -1);
	memcpy(cloud, &cloudCfg, sizeof(CLOUD));
	return 0;
}

int mcloud_set_param(CLOUD *cloud)
{
	return 0;
}

BOOL mcloud_check_enable()
{
	return cloudCfg.bEnable;
}

static int mcloud_upload_file_ex(int year, int month, int day, char *remoteFileName, char *localFileName,int flag, int nType)
{
#ifdef OBSS_CLOUDSTORAGE

	char cloudFileName[64] = {0};
	ipcinfo_t ipcinfo;
	char ystNo[64] = {0};
	ipcinfo_get_param(&ipcinfo);
	jv_ystNum_parse(ystNo, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);

	sprintf(cloudFileName,"%s/%s/%.4d%.2d%.2d/%s",obss_info.days,ystNo,year,month,day,remoteFileName);

	printf("obss upload pic cloudname : %s \n",cloudFileName);

	if(access(localFileName,F_OK) == 0)
		jhlsup_loadFile(cloudFileName,localFileName,NULL);

#endif
	return 0;
}

void mcloud_upload_alarm_file(JV_ALARM *jvAlarm)
{
	jv_assert(jvAlarm != NULL, return);

	if(!mcloud_check_enable())
	{
		if(jvAlarm->cmd[1]==ALARM_VIDEO || jvAlarm->cloudVideoName[0]==0)
			jvAlarm->cloudSt = CLOUD_OFF;
	}

	YST stYST;
	char remoteName[64] = {0};
	char *alarmFile = NULL;
	char *ptr = NULL;
	struct tm tmAlarm;
	int ret = -1;

	if(jvAlarm->cmd[1] == ALARM_PIC)
		alarmFile = jvAlarm->cloudPicName;
	else
		alarmFile = jvAlarm->cloudVideoName;
	ptr = strrchr(alarmFile, '/');
	if(ptr)
	{
		snprintf(remoteName, sizeof(remoteName), "%s", ptr+1);
		localtime_r(&jvAlarm->time, &tmAlarm);
		if (utl_ifconfig_net_prepared > 0)
			ret = mcloud_upload_file_ex(tmAlarm.tm_year+1900, tmAlarm.tm_mon+1, tmAlarm.tm_mday, remoteName, alarmFile, jvAlarm->cloudSt,
		jvAlarm->alarmType==ALARM_DOOR?1:0);
		printf("upload alarm file [%d]: file=%s, uid=%s\n", ret, alarmFile, jvAlarm->uid);
	}
	//在内存中删掉，在TF卡中不删
	if (strstr(alarmFile, "/tmp/") != NULL)
	{
		unlink(alarmFile);
	}
}





