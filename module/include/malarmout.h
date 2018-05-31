

/*	malarmout.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织报警输出相关代码
	更改历史详见svn版本库日志
*/

#ifndef __MALARMOUT_H__
#define __MALARMOUT_H__

#include "jv_common.h"

#define MAX_ALATM_TIME_NUM	3
#define MAX_DEPLOY_POINT_NUM		(100)

typedef struct
{
	char tStart[16];		// 安全防护开始时间(格式: hh:mm:ss)
	char tEnd[16];			// 安全防护结束时间(格式: hh:mm:ss)
}AlarmTime;

typedef struct
{
	int dayOfWeek;			//一周中第几天, 0:无效，1:周日, 2:周一, 3:周二...7:周六
	BOOL bProtection;		// 是否开启安全防护
	int time;				// 开启安全防护时间点，当天的秒数
}DeployPoint;

typedef struct 
{
	int  bEnable;			//使能
   	int  Schedule_time_H;	//设置时间点
   	int  Schedule_time_M;
    int  num;				//数量
	int  Interval;			//时间间隔
	
}MSchedule;

typedef struct 
{
	int delay;				//间隔时间
	char sender[64];		//发件人
	char server[64];		//服务器
	char username[64];		//用户名
	char passwd[64];		//密码
	char receiver0[64];	    //收件人1
	char receiver1[64];
	char receiver2[64];
	char receiver3[64];

	MSchedule  m_Schedule[5]; //定时抓拍设置
	//add by xianlt at 20120628
	int port;				//服务器端口
	char crypto[12];		//加密传输方式
	//add by xianlt at 20120628

	BOOL bEnable;			//安全防护总开关
	AlarmTime alarmTime[MAX_ALATM_TIME_NUM];	// 安全防护时间段设置
	
	char vmsServerIp[20];	//VMS服务器IP地址
	unsigned short vmsServerPort;		//VMS服务器端口
	
	BOOL bAlarmSoundEnable;	//报警声音开关
	BOOL bAlarmLightEnable;	//报警灯光开关
}ALARMSET;

//报警输出类型枚举
typedef enum
{
	ALARM_OUT_CLIENT = 0x01,	//发送至分控
	ALARM_OUT_BUZZ = 0x02,		//蜂鸣器
	ALARM_OUT_EMAIL = 0x04,		//发送邮件
	ALARM_OUT_SOUND = 0x08,		//报警声音
	ALARM_OUT_LIGHT = 0x10,		//报警光
	ALARM_OUT_BUTT
}AlarmOutType_e;

//报警类型枚举
typedef enum
{
	ALARM_VMS_MDETECT 		= 0x01,		//移动侦测报警
	ALARM_VMS_VIDEOCOVER 	= 0x02,		//视频遮挡报警
	ALARM_VMS_VIDEOLOST 	= 0x03,		//视频丢失报警
	ALARM_VMS_DISKLOST 		= 0x04,		//硬盘丢失报警
	ALARM_VMS_ALARMIN 		= 0x05,		//报警输入
	ALARM_VMS_IVP 			= 0x06,		//智能分析报警
	ALARM_VMS_DOOR 			= 0x07,		//门控报警
	ALARM_VMS_BUTT
}AlarmType_e;

//报警传输通道类型枚举
typedef enum
{
	ALARM_NONE 	= 0x00,
	ALARM_UDP 	= 0x01,
	ALARM_TCP 	= 0x02,
	ALARM_BUTT
}AlarmChannelType_e;

#define ALARM_TEXT			0x00
#define ALARM_PIC			0x01
#define ALARM_VIDEO 		0x02

typedef struct
{
	char cmd[4];			//第一位:0x11手机推送服务器 
							//第二位:0x01报警基本信息 0x02报警图片信息 0x03报警视频信息
							//后两位预留
	char ystNo[32]; 		//云视通号
	U16 nChannel;			//从1开始
	U16 alarmType;			//ALARM_TYPEs
	long time;				//时间
	char PicName[64];		//报警关联的本地图片名称
	char VideoName[64];		//报警关联的本地视频名称
	char cloudPicName[64];  //报警关联的云存储图片名称
	char cloudVideoName[64];//报警关联的云存储视频名称
	char uid[36];			//唯一标识符
	char devName[32];		//设备昵称
	char alarmDevName[32];	//针对外设报警，外部报警设备的名称
	U32 pushType;			//推送方案，见AlarmPushType
	S32 errorCode;			//上传错误码，见AlarmErrorCode
	char cloudHost[64];		//云存储服务器
	char cloudBucket[32];	//云存储空间
	int  cloudSt;			//云存储状态
}JV_ALARM;

// 推送方案
enum AlarmPushType
{
	ALARM_PUSH_YST,
	ALARM_PUSH_CLOUD,
	ALARM_PUSH_YST_CLOUD,
	ALARM_PUSH_BUTT,
};


int malarm_init(void);

int malarm_flush(void);

int malarm_deinit(void);

int malarm_sendmail(int channelid, char * message);

void malarm_set_param(ALARMSET *alarm);

void malarm_get_param(ALARMSET *alarm);

U32 mail_test(char *szIsSucceed);

BOOL malarm_check_enable();
BOOL malarm_check_validtime();

/*
 * 蜂鸣器报警开关，打开蜂鸣器报警，响两秒，自动关闭
 */
int malarm_buzzing_open();
int malarm_buzzing_close();

void malarm_build_info(void *alarmInfo, ALARM_TYPEs type);

void malarm_sound_start();
int malarm_sound_stop();

void malarm_light_start();
void malarm_light_stop();

BOOL malarm_get_speakerFlag();



#endif

