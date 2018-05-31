#ifndef M_DOORALARM_H
#define M_DOORALARM_H

#define DoorAlarm_FILE			CONFIG_PATH"dooralarm.dat"			//门磁报警信息

typedef enum
{
	DOOR_INSERT_OK = 0,
	DOOR_INSERT_TIMEDOUT = -1,
	DOOR_INSERT_FULL = -2,
	DOOR_REINSERT = -3,
}DOOR_INSERT_TYPE;

typedef struct
{
	BOOL bEnable;			//是否开启报警
	BOOL bSendtoClient;		//是否发送至分控
	BOOL bSendEmail;		//是否发送邮件
	BOOL bSendtoVMS;		//是否发送至VMS服务器
	BOOL bBuzzing;			//是否蜂鸣器报警
	BOOL bEnableRecord;		//是否开启报警录像
}MDoorAlarm_t;
typedef enum{
	DOOR_ALARM_TYPE_DOOR = 1,	//门磁
	DOOR_ALARM_TYPE_SPIRE,		//手环
	DOOR_ALARM_TYPE_REMOTE_CTRL,//遥控
	DOOR_ALARM_TYPE_SMOKE,		//烟感探测器
	DOOR_ALARM_TYPE_CURTAIN,	//幕帘探测器
	DOOR_ALARM_TYPE_PIR,		//红外探测器
	DOOR_ALARM_TYPE_GAS,		//燃气探测器
	DOOR_ALARM_TYPE_BUT
}DoorAlarmType_e;

typedef void (*DoorAlarmInsertSend)(signed char result, signed char index);
typedef void (*DoorAlarmOnSend)(unsigned char* name, int arg);
typedef void (*DoorAlarmOnStop)();

int mdooralarm_init(DoorAlarmInsertSend insert_send, DoorAlarmOnSend alarmon_send, DoorAlarmOnStop alarmon_stop);
int mdooralarm_insert(int dooralarm_type);
int mdooralarm_set(unsigned char index, unsigned char* name, unsigned char enable);
int mdooralarm_del(unsigned char index);
int mdooralarm_del_all();
int mdooralarm_select(char* buf);
BOOL mdooralarm_bSupport();	//判断设备是否支持门磁报警

//#define DEMO_NOUSE_MIDIFIED_FUNC
#ifdef DEMO_NOUSE_MIDIFIED_FUNC

#include "jv_common.h"

typedef struct
{
	unsigned int id; //无线报警设备的ID号
	unsigned int cmd; //报警设备的命令号
	char name[64];				//名称
	BOOL bEnable;		//是否使能
	DoorAlarmType_e type;			//类型	门磁:0	手环:1

}DoorAlarm_t;

/**
 *@brief 报警回调函数
 *@param bSearch 是否是搜索的结果（不是搜索才表示有报警到达）
 *@param 报警信息。或者设备配对时的信息
 */
typedef void (*DoorAlarmCallback_t)(BOOL bSearch, DoorAlarm_t *alarm);

/**
 *@brief 初始化
 *@param callback 设置回调函数，负责产生报警时回调，以及设备配对时回调
 */
int mdooralarm_init(DoorAlarmCallback_t callback);

/**
 *@brief 设备搜索，只要搜索到一个，立即回调，且后续不再产生
 */
int mdooralarm_scan(void);

/**
 *@brief 添加报警模块
 */
int mdooralarm_add(DoorAlarm_t *alarm);

/**
 *@brief 获取报警模块的数量
 */
int mdooralarm_get_cnt(void);

/**
 *@brief 获取报警模块信息
 */
int mdooralarm_get(int index, DoorAlarm_t *alarm);

#endif

/**
 * @brief 获取报警参数
 *
 * @param param 报警参数输出
 *
 * @return 属性指针
 */
MDoorAlarm_t *mdooralarm_get_param(MDoorAlarm_t *param);

/**
 *@brief 设置参数
 *@param param 参数指针
 *
 *@return 0成功  -1失败
 */
int mdooralarm_set_param(MDoorAlarm_t *param);

//设备重置时，门磁报警信息要清除
void mdooralarm_reset_info();


#endif
