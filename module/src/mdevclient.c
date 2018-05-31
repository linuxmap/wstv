#include "mdevclient.h"
#include "JvServer.h"
#include "mcloud.h"
#include "utl_iconv.h"
#include "mtransmit.h"
#include "SYSFuncs.h"

#define MAX_BUF_SIZE				1024
static pthread_mutex_t msgIDLock;

static void _generateMsgID(int *mid)
{
	static int msgID = 1;

	if(mid)
	{
		pthread_mutex_lock(&msgIDLock);
		if(msgID > 999)
		{
			msgID = 1;
		}
		*mid = msgID++;
		pthread_mutex_unlock(&msgIDLock);
	}
}

static int _sendAlarmMsg(JV_ALARM *alarm)
{
	if(alarm == NULL)
	{
		return -1;
	}
	int mid;
	char buf[MAX_BUF_SIZE] = {0};
	int len = sizeof(buf);
	int pos = 0;
	char utf8Name[64] = {0};

	_generateMsgID(&mid);

	ALARM_TEXT_PARA alarm_para;
	memset(&alarm_para,0,sizeof(ALARM_TEXT_PARA));

	if(alarm->cmd[1] == ALARM_TEXT)
	{
		alarm_para.usChannel = alarm->nChannel;
		alarm_para.uchAlarmSolution = alarm->pushType;
		alarm_para.uchMsgType = alarm->cmd[1];
		alarm_para.uchAlarmType = alarm->alarmType;
		struct tm tmAlarm;
		char alarmTime[16];
		localtime_r(&alarm->time, &tmAlarm);
		snprintf(alarmTime, sizeof(alarmTime), "%04d%02d%02d%02d%02d%02d",
			tmAlarm.tm_year+1900, tmAlarm.tm_mon+1, tmAlarm.tm_mday, tmAlarm.tm_hour, tmAlarm.tm_min, tmAlarm.tm_sec);
		strcpy(alarm_para.chAlarmTime,alarmTime);
		utl_iconv_gb2312toutf8(alarm->devName, utf8Name, sizeof(utf8Name));
		strcpy(alarm_para.chDevName,utf8Name);

		printf("alarm video name : %s \n",alarm->cloudVideoName);

		if(alarm->pushType == ALARM_PUSH_YST)
		{
			strcpy(alarm_para.chPicPath,alarm->PicName);
			strcpy(alarm_para.chVideoPath,alarm->VideoName);
		}
		else if(alarm->pushType == ALARM_PUSH_CLOUD)
		{
			char *ptr = NULL;
			ptr = strrchr(alarm->cloudPicName, '/');
			if(ptr)
				snprintf(alarm_para.chCloudPicPath, 128, "http://%s.%s/%s/%d/%d/%d/%s",alarm->cloudBucket, alarm->cloudHost,
					alarm->ystNo, tmAlarm.tm_year+1900, tmAlarm.tm_mon+1, tmAlarm.tm_mday, ptr+1);
			if('\0' != alarm->cloudVideoName[0])
				strcpy(alarm_para.chCloudVideoName, alarm->cloudVideoName);
		}
		else if(alarm->pushType == ALARM_PUSH_YST_CLOUD)
		{
			strcpy(alarm_para.chPicPath,alarm->PicName);
			strcpy(alarm_para.chVideoPath,alarm->VideoName);

			char *ptr = NULL;
			ptr = strrchr(alarm->cloudPicName, '/');
			if(ptr)
				snprintf(alarm_para.chCloudPicPath, 128, "http://%s.%s/%s/%d/%d/%d/%s",alarm->cloudBucket, alarm->cloudHost,
					alarm->ystNo, tmAlarm.tm_year+1900, tmAlarm.tm_mon+1, tmAlarm.tm_mday, ptr+1);
			if('\0' != alarm->cloudVideoName[0])
				strcpy(alarm_para.chCloudVideoName, alarm->cloudVideoName);
		}

		strncpy(alarm_para.chOtherMsg,alarm->alarmDevName,strlen(alarm->alarmDevName));	//增加报警设备名称

	}

	printf("cloudsee alarm_param :  \n");
	printf("alarm_para.chanel :  %d \n",alarm_para.usChannel);
	printf("alarm_para.uchAlarmSolution :  %d \n",alarm_para.uchAlarmSolution);
	printf("alarm_para.uchMsgType :  %d \n",alarm_para.uchMsgType);
	printf("alarm_para.uchAlarmType :  %d \n",alarm_para.uchAlarmType);
	printf("alarm_para.chAlarmTime :  %s \n",alarm_para.chAlarmTime);
	printf("alarm_para.chDevName :  %s \n",alarm_para.chDevName);
	printf("alarm_para.chCloudPicPath :  %s \n",alarm_para.chCloudPicPath);
	printf("alarm_para.chCloudVideoName :  %s \n",alarm_para.chCloudVideoName);
	printf("alarm_para.chPicPath :  %s \n",alarm_para.chPicPath);
	printf("alarm_para.chVideoPath :  %s \n",alarm_para.chVideoPath);
	printf("alarm_para.chOtherMsg :  %s \n",alarm_para.chOtherMsg);
#ifdef YST_SVR_SUPPORT
	return JVN_PushAlarmMsg(&alarm_para);
#else
	return 0;
#endif
}

void mdevSendAlarm2Server(JV_ALARM *alarm)
{
#ifdef YST_SVR_SUPPORT
	jv_assert(alarm != NULL, return);
	int ret;
	struct timespec timestruct = {0, 0};
	YST stYST;
	GetYSTParam(&stYST);

	if(stYST.nStatus == JVN_SONLINETYPE_ONLINE)
	{
		ret = _sendAlarmMsg(alarm);
	}
	else if(stYST.nStatus == JVN_SONLINETYPE_OFFLINE)
	{
		ret = -1;
		printf("push message failed====> YST Is OffLine\n");
	}
	if(ret)
	{
		printf("send alarm failed\n");
		return;
	}
	printf("send alarm ret %d (%ld): type=%d, uid=%s\n", ret, time(0), alarm->cmd[1], alarm->uid);
#endif
}

