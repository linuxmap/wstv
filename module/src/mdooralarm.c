//RoyZhang
//2014.10.21
//门磁报警模块

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>

#include "jv_common.h"
#include "jv_gpio.h"
#include "utl_iconv.h"
#include "jv_dooralarm.h"
#include "jv_uartcomm.h"
#include "jv_gpiocomm.h"
#include "mdooralarm.h"
#include "SYSFuncs.h"
#include "maudio.h"
#include "malarmout.h"
#include "mlog.h"

#define DOOR_ALARM_GROUP_MAX	16
typedef struct mDoorAlarm_st
{
	unsigned char flag;			//是否有效
	unsigned int  guid;			//设备ID
	unsigned char name[64];		//名称
	unsigned char enable;		//是否使能
	DoorAlarmType_e type;		//类型: 1:门磁, 2:手环, 3:遥控器, 4:烟感探测器, 5:幕帘探测器, 6:红外探测器, 7:燃气探测器
	unsigned int triggerTime;   //上次触发的时间
	unsigned int triggerCount;  //设置时间段内触发的次数
}mDoorAlarmType;

static mDoorAlarmType mDoorAlarm[DOOR_ALARM_GROUP_MAX];

//传给网络报警
typedef struct DoorAlarmUp_ST
{
	unsigned char insert_index;
	DoorAlarmInsertSend insert_send;
	DoorAlarmOnSend	alarmon_send;
	DoorAlarmOnStop	alarmon_stop;
}DoorAlarmUpType;

static DoorAlarmUpType sDoorAlarmUp;

//学习
typedef struct DoorAlarmInsert_ST
{
	//门磁添加标志
	unsigned char insert_flag;
	//添加门磁超时等待10s钟
	unsigned int insert_count;
}DoorAlarmInsertType;

static DoorAlarmInsertType sDoorAlarmInsert;

static MDoorAlarm_t doorAlarmCfg;

static int (*_mdooralarm_getID)(unsigned int*);

static int _mdooralarm_save();
static int _mdooralarm_read();

#define ALARM_SEND_TIME_RANGE			10
#define ALARM_SEND_MIN_TRIGGER_COUNT	2

#define ALARM_TRIGGER_COUNT_TIME_LIMIT	2

/* 判断是单品还是多品 */
static BOOL __judgeMutliProduct()
{
	int i = 0;
	int doorCount = 0, pirCount = 0, curtainCount = 0;

	for (i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
	{
		if (mDoorAlarm[i].flag == 1 && mDoorAlarm[i].enable == 1)
		{
			if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_DOOR)
				doorCount++;
			if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_CURTAIN)
				curtainCount++;
			if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_PIR)
				pirCount++;
		}
	}
	if(doorCount > 0)
	{
		if(pirCount > 0 /*|| doorCount >= 2*/)
			return TRUE;
		/*else if(curtainCount > 0)*/
			/*return FALSE;*/
		else
			return FALSE;
	}
	else if(pirCount > 0)
	{
		if(pirCount >= 2)
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		return FALSE;
	}
}

static void _checkAlarmStatus(mDoorAlarmType *doorAlarm)
{
	int i = 0;
	time_t tNow;
	BOOL bSendFlag = FALSE;

	if(sDoorAlarmUp.alarmon_send == NULL)
	{
		printf("alarmon_send is null!\n");
		return;
	}

	tNow = time(NULL);
	printf("type:%d triggerTime:%d tNow:%d triggerCount:%d\n",
			doorAlarm->type, doorAlarm->triggerTime, (int)tNow, doorAlarm->triggerCount);

	printf("bEnable type triggerTime triggerCount    guid\n");
	for (i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
	{
		if(mDoorAlarm[i].flag)
			printf("   %d     %d   %10d       %d       0x%08X\n",
					mDoorAlarm[i].enable, mDoorAlarm[i].type,
					mDoorAlarm[i].triggerTime,
					mDoorAlarm[i].triggerCount,
					mDoorAlarm[i].guid);
	}

	/* 手环 烟感 燃气触发即报警,其它遵循报警逻辑 */
	if(doorAlarm->type == DOOR_ALARM_TYPE_SPIRE ||
		doorAlarm->type == DOOR_ALARM_TYPE_SMOKE ||
		doorAlarm->type == DOOR_ALARM_TYPE_GAS)
	{
		bSendFlag = TRUE;
	}
	else if(doorAlarm->type == DOOR_ALARM_TYPE_DOOR ||
			doorAlarm->type == DOOR_ALARM_TYPE_CURTAIN ||
			doorAlarm->type == DOOR_ALARM_TYPE_PIR)
	{
		/* 判断是属于单品还是多品 */
		if(__judgeMutliProduct())
		{
			if(tNow - doorAlarm->triggerTime >= ALARM_SEND_TIME_RANGE)
			{
				doorAlarm->triggerTime = tNow;
				doorAlarm->triggerCount = 1;
				printf("doorAlarm->triggerCount:%d\n", doorAlarm->triggerCount);
			}
			else
			{
				if(tNow - doorAlarm->triggerTime > ALARM_TRIGGER_COUNT_TIME_LIMIT)
					doorAlarm->triggerCount++;

				/* 如果连续触发，则认为设备异常情况，不报警 */
				doorAlarm->triggerTime = tNow;
				/* 在限定的时间内触发了多次 */
				printf("doorAlarm->triggerCount:%d\n", doorAlarm->triggerCount);
				if(doorAlarm->triggerCount >= ALARM_SEND_MIN_TRIGGER_COUNT)
				{
					bSendFlag = TRUE;
				}
			}

			for (i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
			{
				if (mDoorAlarm[i].enable == 1 && mDoorAlarm[i].triggerCount >= 1 &&
						mDoorAlarm[i].guid !=  doorAlarm->guid)
				{
					if(doorAlarm->type == DOOR_ALARM_TYPE_DOOR)
					{
						if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_DOOR)
						{
							bSendFlag = TRUE;
							mDoorAlarm[i].triggerCount = 0;
						}
						else if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_PIR)
						{
							bSendFlag = TRUE;
							mDoorAlarm[i].triggerCount = 0;
						}
					}
					else if(doorAlarm->type == DOOR_ALARM_TYPE_PIR)
					{
						if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_DOOR)
						{
							bSendFlag = TRUE;
							mDoorAlarm[i].triggerCount = 0;
						}
						else if(mDoorAlarm[i].type == DOOR_ALARM_TYPE_PIR)
						{
							bSendFlag = TRUE;
							mDoorAlarm[i].triggerCount = 0;
						}
					}
				}
			}
		}
		else
		{
			if(tNow - doorAlarm->triggerTime >= ALARM_TRIGGER_COUNT_TIME_LIMIT)
			{
				doorAlarm->triggerTime = tNow;
				bSendFlag = TRUE;
			}
		}
	}

	// 手环SOS报警，无论任何时候都响应
	if((doorAlarm->enable == 1 && bSendFlag == TRUE && doorAlarmCfg.bEnable) ||
		doorAlarm->type == DOOR_ALARM_TYPE_SPIRE)
	{
		printf("start send alarm.\n");
		doorAlarm->triggerCount = 0;
		sDoorAlarmUp.alarmon_send(doorAlarm->name, doorAlarm->type);
	}
}

static void _mdooralarm_thread(void* param)
{
	int i = 0;
	int cnt = 0;
	int find = 0;
	unsigned int guid = 0;

	pthreadinfo_add(__func__);

	if (_mdooralarm_getID == NULL)
		return;

#if 1	//此处仅仅是为了工厂检测门磁功能所加，之后不再重新做门磁检测程序
	if (gp.bFactoryFlag)
	{
		char count = 4;
		/* 工厂的三个433设备 
		0x004F67A5 1 0 1 0 0 1 0 1 1 1 1 0 0 1 1 0 1 1 1 1 0 0 1 0 0 
		0x00AEAEB0 0 0 0 0 1 1 0 1 0 1 1 1 0 1 0 1 0 1 1 1 0 1 0 1 0 
		0x00AAE2AB 1 1 0 1 0 1 0 1 0 1 0 0 0 1 1 1 0 1 0 1 0 1 0 1 0
		0x00A704E2 */
		unsigned int check_GUID1 = 0x00A5E6F2; 
		unsigned int check_GUID2 = 0x000D7575;
		unsigned int check_GUID3 = 0x00D54755;
		unsigned int check_GUID4 = 0x00A704E2;

		while (1)
		{
			cnt = _mdooralarm_getID(&guid);

			if (cnt > 0)
			{
				//工厂检测，上电检测3遍
				if (count > 0)
				{
					if (check_GUID1 == guid || check_GUID2 == guid ||
						check_GUID3 == guid || check_GUID4 == guid)
					{
						maudio_speaker(VOICE_ALARMING, FALSE, TRUE, TRUE);
						malarm_buzzing_open();
						sleep(2);
						malarm_buzzing_close();
						printf("dooralarm check success\n");
						count--;
					}
				}
				else
					break;
			}
			usleep(100 * 1000);
		}
	}
#endif
	while (1)
	{
		cnt = _mdooralarm_getID(&guid);

		if (cnt > 0)
		{
			//获得guid
			if (sDoorAlarmInsert.insert_flag == 1)
			{
				/* 使用新版单片机433解码流程，识别时间大幅缩短，但可能存在小概率误码
				   因此绑定时需要多次取样，确保数值正确
				   学习取到guid，先查询是否已被绑定 */
				for (i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
					if (mDoorAlarm[i].flag == 1 && mDoorAlarm[i].guid == guid)
						break;

				if(i < DOOR_ALARM_GROUP_MAX)
				{
					sDoorAlarmUp.insert_send(DOOR_REINSERT, i); // 重复绑定
				}
				else
				{
					mDoorAlarm[sDoorAlarmUp.insert_index].guid = guid;
					mDoorAlarm[sDoorAlarmUp.insert_index].flag = 1;
					if (sDoorAlarmUp.insert_send != NULL)
						sDoorAlarmUp.insert_send(DOOR_INSERT_OK, sDoorAlarmUp.insert_index);
					_mdooralarm_save();
					sDoorAlarmInsert.insert_flag = 0;
				}
			}
			else
			{
				//判断 报警
				for (i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
				{
					if ((mDoorAlarm[i].flag == 1) && (mDoorAlarm[i].enable == 1))
					{
						//遥控器
						if (mDoorAlarm[i].type == DOOR_ALARM_TYPE_REMOTE_CTRL)
						{
							if ((mDoorAlarm[i].guid >> 8) == (guid >> 8))
								find = 1;
						}
						else
						{
							if (mDoorAlarm[i].guid == guid)
								find = 1;
						}
					}

					if(find == 1)
						break;
				}

				if (find == 1)
				{
					find = 0;
					printf("type:%d enable:%d guid:%08x\n",
							mDoorAlarm[i].type, mDoorAlarm[i].enable, mDoorAlarm[i].guid);
					if (mDoorAlarm[i].type == DOOR_ALARM_TYPE_REMOTE_CTRL)
					{
						//遥控器
						if((guid & 0x0C) == 0x0C)
						{
							printf("cmd protect1\n");
							doorAlarmCfg.bEnable = TRUE;
							doorAlarmCfg.bEnableRecord = TRUE;
							ALARMSET alarm;
							malarm_get_param(&alarm);
							alarm.bAlarmSoundEnable = TRUE;
							alarm.bAlarmLightEnable = TRUE;
							alarm.bEnable = TRUE;
							char *msg = "遥控器;临时布防";
							mlog_write(msg);
							char description[64] = {0};
							utl_iconv_gb2312toutf8(msg, description, sizeof(description));
							malarm_set_param(&alarm);
							WriteConfigInfo();
							maudio_speaker(VOICE_STARTPROTECT, TRUE,TRUE, TRUE);
						}
						else if((guid & 0x30) == 0x30)
						{
							printf("cmd 2\n");
						}
						else if((guid & 0x03) == 0x03)
						{
							printf("cmd not protect1\n");
							doorAlarmCfg.bEnable = FALSE;
							doorAlarmCfg.bEnableRecord = FALSE;
							ALARMSET alarm;
							malarm_get_param(&alarm);
							alarm.bEnable = FALSE;
							alarm.bAlarmSoundEnable = FALSE;
							alarm.bAlarmLightEnable = FALSE;
							char *msg = "遥控器;临时撤防";
							mlog_write(msg);
							char description[64] = {0};
							utl_iconv_gb2312toutf8(msg, description, sizeof(description));
							malarm_set_param(&alarm);
							WriteConfigInfo();
							sDoorAlarmUp.alarmon_stop();
							maudio_speaker(VOICE_CLOSEPROTECT, TRUE,TRUE, TRUE);
						}
						else if((guid & 0xC0) == 0xC0)
						{
							printf("cmd 4\n");
						}
					}
					else
					{
						_checkAlarmStatus(&mDoorAlarm[i]);
					}
				}
			}
		}
		else
		{
			if (sDoorAlarmInsert.insert_flag == 1)
			{
				//学习
				if (sDoorAlarmInsert.insert_count <= 0)
				{
					//超时
					if (sDoorAlarmUp.insert_send != NULL)
						sDoorAlarmUp.insert_send(DOOR_INSERT_TIMEDOUT, 0);
					sDoorAlarmInsert.insert_flag = 0;
				}
				sDoorAlarmInsert.insert_count--;
			}
		}
		
		usleep(100 * 1000);
	}
}

int mdooralarm_insert(int dooralarm_type)
{
	unsigned char i = 0;
	for (i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
	{
		if (mDoorAlarm[i].flag == 0)
		{
			mDoorAlarm[i].type = dooralarm_type;
			sDoorAlarmUp.insert_index = i;
			break;
		}
	}
	if (i == DOOR_ALARM_GROUP_MAX)
	{
		printf("dooralarm insert full\n");
		sDoorAlarmUp.insert_send(DOOR_INSERT_FULL, 0);
		return -1;
	}
	//学习
	sDoorAlarmInsert.insert_count = 100;
	sDoorAlarmInsert.insert_flag = 1;
	
	return 0;
}

int mdooralarm_set(unsigned char index, unsigned char* name, unsigned char enable)
{
	//char gbkString[64] = {0};

	if (index >= DOOR_ALARM_GROUP_MAX)
	{
		return -1;
	}
	if (mDoorAlarm[index].flag == 0)
	{
		return -1;
	}
	mDoorAlarm[index].enable = enable;
	memcpy(mDoorAlarm[index].name, name, sizeof(mDoorAlarm[index].name) - 1);
	_mdooralarm_save();
	printf("mdooralarm_set end\n");
	return 0;
}

int mdooralarm_del(unsigned char index)
{
	if (index >= DOOR_ALARM_GROUP_MAX)
	{
		return -1;
	}
	memset(&mDoorAlarm[index], 0, sizeof(mDoorAlarmType));
	_mdooralarm_save();
	printf("mdooralarm_del end\n");
	return 0;
}

int mdooralarm_del_all()
{
	int i = 0;
	for(i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
	{
		if(mDoorAlarm[i].flag == 0)
			break;
		mdooralarm_del(i);
	}
	
	return 0;
}


//判断是否支持门磁
BOOL mdooralarm_bSupport()
{
	return hwinfo.bSupportMCU433;
}

int mdooralarm_select(char* buf)
{
	char *p = buf;
	int i = 0;
	int j = 0;


	if(!mdooralarm_bSupport())
		return -1;

	if(!buf)
	{
		return -1;
	}

	for(i = 0; i < DOOR_ALARM_GROUP_MAX; i++)
	{
		if(mDoorAlarm[i].flag == 1)
		{
			p += sprintf(p,  "type=%d;", mDoorAlarm[i].type);
			p += sprintf(p,  "guid=%d;", i);
			p += sprintf(p,  "enable=%d;", mDoorAlarm[i].enable);
			p += sprintf(p,  "name=%s;$", mDoorAlarm[i].name);
		}
	}

	printf("mdooralarm_select end\n");
	return p-buf;
}

int mdooralarm_init(DoorAlarmInsertSend insert_send, DoorAlarmOnSend alarmon_send, DoorAlarmOnStop alarmon_stop)
{
	int ret = 0;
	int ret2 = 0;
	int ret3 = -1;
	int group = 0, bit = 0;
	pthread_t pid_dooralarm;

	if (!hwinfo.bSupportMCU433)
	{
		printf("dooralarm not support\n");
		return -1;
	}

	jv_uartcomm_init();

	_mdooralarm_getID = jv_uartcomm_doorID_get;
	printf("mcu dooralarm init!!\n");

	//memset(&doorAlarmCfg, 0, sizeof(MDoorAlarm_t));
	memset(mDoorAlarm, 0, sizeof(mDoorAlarm));
	_mdooralarm_read();
	
	sDoorAlarmInsert.insert_count = 100;
	sDoorAlarmInsert.insert_flag = 0;

	sDoorAlarmUp.insert_send = insert_send;
	sDoorAlarmUp.alarmon_send = alarmon_send;
	sDoorAlarmUp.alarmon_stop = alarmon_stop;
	sDoorAlarmUp.insert_index = 0;

	doorAlarmCfg.bEnable = TRUE;
	doorAlarmCfg.bSendtoClient = FALSE;
	doorAlarmCfg.bSendEmail = FALSE;
	doorAlarmCfg.bSendtoVMS = FALSE;
	doorAlarmCfg.bBuzzing = FALSE;
	doorAlarmCfg.bEnableRecord = TRUE;
	
	//开启一个门磁线程
	pthread_create(&pid_dooralarm, NULL, (void*)_mdooralarm_thread, NULL);

	return 0;
}

static int _mdooralarm_save()
{
	FILE *fOut = NULL;
	
	fOut=fopen(DoorAlarm_FILE, "wb");
	if(!fOut)
	{
		Printf("save dooralarm dat: %s, err: %s\n", DoorAlarm_FILE, strerror(errno));
		return -1;
	}
	
	fwrite(mDoorAlarm, 1, sizeof(mDoorAlarm), fOut);
	
	fclose(fOut);
	return 0;
}

static int _mdooralarm_read()
{
	FILE *fOut = NULL;
	
	if (access(DoorAlarm_FILE, F_OK) != 0)
	{
		return -1;
	}
	fOut=fopen(DoorAlarm_FILE, "rb");
	if(!fOut)
	{
		Printf("get dooralarm dat: %s, err: %s\n", DoorAlarm_FILE, strerror(errno));
		return -1;
	}
	
	fread(mDoorAlarm, 1, sizeof(mDoorAlarm), fOut);

	fclose(fOut);
	return 0;
}

MDoorAlarm_t *mdooralarm_get_param(MDoorAlarm_t *param)
{
	if(param)
	{
		memcpy(param, &doorAlarmCfg, sizeof(MDoorAlarm_t));
		return param;
	}

	return &doorAlarmCfg;
}

int mdooralarm_set_param(MDoorAlarm_t *param)
{
	if(param)
	{
		memcpy(&doorAlarmCfg, param, sizeof(MDoorAlarm_t));
		return 0;
	}

	return -1;
}

void mdooralarm_reset_info()
{
	mdooralarm_del_all();
	_mdooralarm_read();
}


