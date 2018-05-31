
#include "jv_common.h"
#include <jv_gpio.h>
#include "jv_alarmin.h"
#include <utl_cmd.h>
#include <utl_queue.h>
#include "mipcinfo.h"
#include "msnapshot.h"

#include "malarmout.h"
#include "mosd.h"
#include "mlog.h"
#include <smtp.h>
#include "maudio.h"
#include "mvoicedec.h"
#include "mcloud.h"
#include "msensor.h"
#include "utl_timer.h"
#include "utl_common.h"
#include "jv_ao.h"
#include "mrecord.h"
#include "mioctrl.h"

typedef struct {
	int channelid;
	char fname[256];
	char message[256];
}malarm_msg_t;
static ALARMSET alarmcfg;

static jv_thread_group_t mailtask;

static void _malarm_process(void);
extern BOOL SendMail(char *sServerHost,char *sFromName,char *sAuthUser,char *sAuthPass,char *sFromAddr,char *sToAddr,char *sSubject,char *sBody,char *sAttachment);

static BOOL _m_schedule(int tid,int second, void *param)
{
	ALARMSET alarm;
	int time_tmp;
	int i;

	malarm_sendmail(0, mlog_translate("Timing captured")); //借用mlog的多语言管理
	//mlog_write("MDoorAlarm: Mail Sended");

	char tmpPath[256] = {0};
	time_t nsecond;
	struct tm *tm_snap;
	if (0 != access("/progs/rec/00/", F_OK))  //查看TF卡是否挂载成功
	{
		Printf("cannot access sd card\n");
	}
	else
	{
		if(0 != access("/progs/rec/00/snapshot/timelapse/", F_OK))
		{
			Printf("cannot access the dest path :/progs/rec/00/snapshot/timelapse/\n");
		}
		else
		{
			nsecond = time(NULL);
			tm_snap = localtime(&nsecond);
			sprintf(tmpPath,"/progs/rec/00/snapshot/timelapse/%04d%02d%02d_%02d%02d%02d.jpg",
				tm_snap->tm_year+1900,
				tm_snap->tm_mon+1,
				tm_snap->tm_mday,
				tm_snap->tm_hour,
				tm_snap->tm_min,
				tm_snap->tm_sec);
			msnapshot_get_file(0, tmpPath);
		}
	}
	
	malarm_get_param(&alarm);
	for(i=0;i<5;i++)
	{
      if(schedule_id[i]==tid)
	  	break;
	}
	
	if(alarm.m_Schedule[i].num<=0)
		return FALSE;
	
	time_tmp=alarm.m_Schedule[i].Schedule_time_H*60+alarm.m_Schedule[i].Schedule_time_M+(alarm.m_Schedule[i].num-1)*alarm.m_Schedule[i].Interval;
	if(second>=time_tmp)
	{
		time_tmp=alarm.m_Schedule[i].Schedule_time_H*60+alarm.m_Schedule[i].Schedule_time_M;	
	}
	else
	{
		time_tmp=second+alarm.m_Schedule[i].Interval;//通过修改定时器定时时间的方式，实现间隔时间
	}
	
	utl_schedule_Enable(schedule_id[i],time_tmp);
	return TRUE;
}

int malarm_init(void)
{
	int i;
	
	mailtask.iMqHandle = utl_queue_create("mail_alarm_mq", sizeof(malarm_msg_t), 16); 
	if (mailtask.iMqHandle < 0)
	{
		return -1;
	}

	mailtask.running = TRUE;
	pthread_create(&mailtask.thread, NULL, (void *)_malarm_process, NULL);
	

	for(i=0;i<5;i++)
	{
	  schedule_id[i]=utl_schedule_create("m_schedule",_m_schedule,NULL);//创建定时任务
	}

	return 0;
}

int malarm_flush(void)
{   
	int i;
	ALARMSET alarm;
	int time_tmp,time_tmpMax;
	time_t tt;
	struct tm tm;
	int now;
	int cnt;
	
	malarm_get_param(&alarm);
	tt = time(NULL);
	localtime_r(&tt, &tm);
	now = tm.tm_hour*60 + tm.tm_min;
	for(i=0;i<5;i++)
	{
	  if(alarm.m_Schedule[i].bEnable==1)//开启定时任务
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
	}
	
	return 0;
}

int malarm_deinit(void)
{
	malarm_msg_t msg;
	msg.channelid = -1;
	mailtask.running = FALSE;
	utl_queue_send(mailtask.iMqHandle, &msg);
		
	pthread_join(mailtask.thread, NULL);
	utl_queue_destroy(mailtask.iMqHandle);
	return 0;
}

static void _malarm_do(int channelid, char *attachfname, char *message)
{
	//添加发送邮件函数
	char receiver[512]={0};
	mchnosd_attr attr;
	mchnosd_get_param(channelid,&attr);

	receiver[0] = '\0';
	if( strcmp(alarmcfg.receiver0,"(null)")!=0 && strcmp(alarmcfg.receiver0," ")!=0 && strcmp(alarmcfg.receiver0,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver0);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver0);
		}
	}
	if( strcmp(alarmcfg.receiver1,"(null)")!=0 && strcmp(alarmcfg.receiver1," ")!=0 && strcmp(alarmcfg.receiver1,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver1);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver1);
		}
	}
	if( strcmp(alarmcfg.receiver2,"(null)")!=0 && strcmp(alarmcfg.receiver2," ")!=0 && strcmp(alarmcfg.receiver2,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver2);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver2);
		}
	}
	if( strcmp(alarmcfg.receiver3,"(null)")!=0 && strcmp(alarmcfg.receiver3," ")!=0 && strcmp(alarmcfg.receiver3,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver3);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver3);
		}
	}

	Printf("server:%s\n",alarmcfg.server);
	Printf("devname:%s\n",attr.channelName);
	Printf("username:%s\n",alarmcfg.username);
	Printf("passwd:%s\n",alarmcfg.passwd);
	Printf("sender:%s\n",alarmcfg.sender);
	Printf("rc:%s\n",receiver);
	Printf("attachfname: %s, strlen: %d\n",attachfname,strlen(attachfname));

	//add by xianlt at 20120628
	//把正文写入文件
	char html[512]={0};
	char date[32]={0};
	char DevId[15]={0};
	jv_ystNum_parse(DevId, ipcinfo_get_param(NULL)->nDeviceInfo[6], ipcinfo_get_param(NULL)->ystID);

	if (ipcinfo_get_param(NULL)->nLanguage == LANGUAGE_CN)
	{
		mchnosd_time2str("YYYY年MM月DD日 hh:mm:ss", time(NULL), date);
	}
	else
	{
		mchnosd_time2str("MM/DD/YYYY hh:mm:ss", time(NULL), date);
	}
	sprintf(html, mlog_translate("<html><body>Dear Sir/Madam <br>	Thanks for using our company's IP Camera.<br>  This email that you received is recorded and sent out automatically by the IP Camera.<br>The reason is as follows: <br><b>%s At Time: %s Device number：%s Device name：%s</b><br>Please check the recorded file timely.Thanks for your cooperation!</body></html>"),
		message, date, DevId, ipcinfo_get_param(NULL)->acDevName);

	int ret = smtp(alarmcfg.server, alarmcfg.port, alarmcfg.crypto, alarmcfg.username, alarmcfg.passwd, alarmcfg.sender, mlog_translate("warning mail"), html, "GB2312", attachfname, receiver);
	//add by xianlt at 20120628

	unlink(attachfname);
}

U32 mail_test(char *szIsSucceed)
{
	//添加发送邮件函数
	szIsSucceed[0] = '\0';
	char receiver[512] ={0};
	//Printf("%s\n", alarmcfg.receiver0);
	if( strcmp(alarmcfg.receiver0,"(null)")!=0 && strcmp(alarmcfg.receiver0," ")!=0 && strcmp(alarmcfg.receiver0,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver0);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver0);
		}
	}
	if( strcmp(alarmcfg.receiver1,"(null)")!=0 && strcmp(alarmcfg.receiver1," ")!=0 && strcmp(alarmcfg.receiver1,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver1);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver1);
		}
	}
	if( strcmp(alarmcfg.receiver2,"(null)")!=0 && strcmp(alarmcfg.receiver2," ")!=0 && strcmp(alarmcfg.receiver2,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver2);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver2);
		}
	}
	if( strcmp(alarmcfg.receiver3,"(null)")!=0 && strcmp(alarmcfg.receiver3," ")!=0 && strcmp(alarmcfg.receiver3,"")!=0 )
	{
		if(strlen(receiver) != 0)
		{
			strcat(receiver,",");
			strcat(receiver,alarmcfg.receiver3);
		}
		else
		{
			strcpy(receiver,alarmcfg.receiver3);
		}
	}

	char html[512];
	sprintf(html, mlog_translate("<html><body>Dear Sir/Madam <br>	Congratulations.<br>  You successfully received the test email.</body></html>"));
	char subject[64] = {0x00};
	sprintf(subject, mlog_translate("warning mail"));

	int ret = smtp(alarmcfg.server, alarmcfg.port, alarmcfg.crypto, alarmcfg.username, alarmcfg.passwd, alarmcfg.sender, subject, html, "GB2312", NULL, receiver);
	if (ret == 0)
	{
		strcpy(szIsSucceed, "1");
	}
	else
	{
		strcpy(szIsSucceed, "0");
	}
	return 1;
}

static void _malarm_process(void)
{
	int len;
	malarm_msg_t msg;
	pthreadinfo_add((char *)__func__);
	while(mailtask.running)
	{
		len = utl_queue_recv(mailtask.iMqHandle, &msg, -1);
		if (len != 0)
		{
			Printf("timeout happened when mq_receive data\n");
			usleep(1);
			continue;
		}
		if (msg.channelid == -1)
			continue;
		_malarm_do(msg.channelid, msg.fname,msg.message);


		
	}
}

int malarm_sendmail(int channelid, char *message)
{
	//添加发送邮件函数
	malarm_msg_t msg = {0};

	//生成抓图文件并将抓图文件邮件
	//抓图文件放在/tmp目录中
	static int cnt = 0;
	char fname[256] = {0};
	sprintf(fname, "/tmp/warning%d.jpg", cnt);
	if(++cnt > 10)
	{
		cnt = 0;
	}
	msnapshot_get_file(channelid, fname);
	msg.channelid = channelid;
	strncpy(msg.fname, fname, 256);
	strncpy(msg.message, message, 256);
	utl_queue_send(mailtask.iMqHandle, &msg);

	return 0;
}

void malarm_set_param(ALARMSET *alarm)
{
	AlarmTime alarmTime[MAX_ALATM_TIME_NUM];

	memset(alarmTime, 0, sizeof(alarmTime));

	if ((alarm->bEnable != alarmcfg.bEnable)
		|| (memcmp(&alarmcfg.alarmTime[0], &alarm->alarmTime[0], sizeof(alarmcfg.alarmTime[0])) != 0))
	{
		if (alarm->bEnable)
		{
			if (memcmp(alarmTime, alarm->alarmTime, sizeof(alarmTime)) == 0)
			{
				mlog_write("Enable protection: %s ~ %s", "00:00:00", "23:59:59");
			}
			else
			{
				mlog_write("Enable protection: %s ~ %s", alarm->alarmTime[0].tStart, alarm->alarmTime[0].tEnd);
			}
		}
		else
		{
			if (alarmcfg.bEnable)
			{
				mlog_write("Disable protection");
			}
		}
	}

	memcpy(&alarmcfg, alarm, sizeof(ALARMSET));
}

void malarm_get_param(ALARMSET *alarm)
{
	memcpy(alarm, &alarmcfg, sizeof(ALARMSET));
}

BOOL malarm_check_enable()
{
	return alarmcfg.bEnable;
}

BOOL malarm_check_validtime()
{
	int i;
	int hour, min, sec;
	int startAlarmTime, endAlarmTime, nowTime;
	AlarmTime alarmTime[MAX_ALATM_TIME_NUM];

	memset(alarmTime, 0, sizeof(alarmTime));
	if(memcmp(alarmTime, alarmcfg.alarmTime, sizeof(alarmTime)) == 0)
	{
		// 未设置，默认全天
		//Printf("No alarm time set, default whole day\n");
		return TRUE;
	}

	for(i=0; i<MAX_ALATM_TIME_NUM; i++)
	{
		if(sscanf(alarmcfg.alarmTime[i].tStart, "%02d:%02d:%02d", &hour, &min, &sec) != 3)
			continue;
		startAlarmTime = hour*3600 + min*60 + sec;
		if(sscanf(alarmcfg.alarmTime[i].tEnd, "%02d:%02d:%02d", &hour, &min, &sec) != 3)
			continue;
		endAlarmTime = hour*3600 + min*60 + sec;

		time_t tNow = time(NULL);
		struct tm tmNow;
		localtime_r(&tNow, &tmNow);
		nowTime = tmNow.tm_hour*3600 + tmNow.tm_min*60 + tmNow.tm_sec;
		//Printf("alarm time[%s-%s], cur time is %02d:%02d:%02d\n", alarmcfg.alarmTime[i].tStart, alarmcfg.alarmTime[i].tEnd, 
		//	tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);

		if(isInTimeRange(nowTime, startAlarmTime, endAlarmTime))
		{
			return TRUE;
		}
	}

	return FALSE;
}

int malarm_buzzing_open()
{
	return 0;
}

int malarm_buzzing_close()
{
	return 0;
}

void malarm_build_info(void *alarmInfo, ALARM_TYPEs type)
{
	jv_assert(alarmInfo != NULL, return);

	JV_ALARM *jvAlarm = (JV_ALARM *)alarmInfo;
	memset(jvAlarm, 0, sizeof(JV_ALARM));
	jvAlarm->cmd[0] = 0x11;
	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	jv_ystNum_parse(jvAlarm->ystNo, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
	jvAlarm->nChannel = 1;
	
	if (mcloud_check_enable() == TRUE
		&& type != ALARM_ONE_MIN
		&& type != ALARM_CHAT_START
		&& type != ALARM_CHAT_STOP
		&& type != ALARM_SHOT_PUSH
		&& type != ALARM_REC_PUSH
		)
	{
		char tmpbuf[] = "100";
		if (type == ALARM_MOTIONDETECT)
			tmpbuf[1] = '0';
		else if (type == ALARM_DOOR)
			tmpbuf[1] = '2';
		else if (type == ALARM_GPIN)
			tmpbuf[1] = '3';
		else if (type == ALARM_IVP)
			tmpbuf[1] = '8';		
		else if (type == ALARM_CAT_HIDEDETECT)	//遮挡报警
			tmpbuf[1] = '9';		
		else
			tmpbuf[1] = '2';
		tmpbuf[2] = obss_info.type[0];
		jvAlarm->alarmType = atoi(tmpbuf);
		jvAlarm->pushType = ALARM_PUSH_YST_CLOUD;
	}
	else
	{
		jvAlarm->alarmType = type;
		jvAlarm->pushType = ALARM_PUSH_YST;
	}
	printf("alarm type:%d\n",jvAlarm->alarmType);
	
	jvAlarm->time = time(NULL);
	snprintf(jvAlarm->devName, sizeof(jvAlarm->devName), "%s", ipcinfo.acDevName);
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	sprintf(jvAlarm->uid, "%s%02d%x%x", jvAlarm->ystNo, jvAlarm->alarmType, (U32)tv.tv_sec, (U32)tv.tv_nsec);

	CLOUD cloudCfg;
	mcloud_get_param(&cloudCfg);
	snprintf(jvAlarm->cloudBucket, sizeof(jvAlarm->cloudBucket), "%s", cloudCfg.bucket);
	snprintf(jvAlarm->cloudHost, sizeof(jvAlarm->cloudHost), "%s", cloudCfg.host);
}


pthread_t pid;


static void* alarm_proc(void* param)
{
	int i = 0;
	pthreadinfo_add((char *)__func__);
	
	for(i=0;i<20 && (speakerowerStatus == JV_SPEAKER_OWER_ALARMING); i++)
	{
		maudio_speaker(VOICE_ALARMING, FALSE,TRUE, TRUE);
	}
	
	if(speakerowerStatus <= JV_SPEAKER_OWER_ALARMING)
		speakerowerStatus = JV_SPEAKER_OWER_NONE;
	
	return NULL;
}


void malarm_sound_start()
{
	if(speakerowerStatus < JV_SPEAKER_OWER_ALARMING && maudio_speaker_GetEndingFlag())
	{
		speakerowerStatus = JV_SPEAKER_OWER_ALARMING;
		pthread_create_normal(&pid,NULL,alarm_proc,NULL);	
		pthread_detach(pid);
	}		
}

int malarm_sound_stop()
{
	if(speakerowerStatus <= JV_SPEAKER_OWER_ALARMING)
		speakerowerStatus = JV_SPEAKER_OWER_NONE;

	return maudio_speaker_stop();
}

BOOL malarm_get_speakerFlag()
{
	return maudio_speaker_GetEndingFlag();
}

void malarm_light_start()
{
	msensor_set_alarm_light(TRUE);
	mio_set_light_alarm_st(DEV_ST_LIGHT_ALARM_ON);
}

void malarm_light_stop()
{
	msensor_set_alarm_light(FALSE);
	mio_set_light_alarm_st(DEV_ST_LIGHT_ALARM_OFF);
}

