/*
 * sp_record.h
 *
 *  Created on: 2013-11-1
 *      Author: lfx
 */

#ifndef SP_ALARM_H_
#define SP_ALARM_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "malarmout.h"


typedef struct{
	BOOL bEnable;			//是否启动报警输入
	BOOL bNormallyClosed;	//是否常闭。其值为真时，断开报警。反之闭合报警

	unsigned char  u8AlarmNum;			//报警路数控制，每一位表示一路报警，1开0关，暂只有两路报警,只有后两位有效

	BOOL bEnableRecord;		//是否开启报警录像

	int nDelay;				//报警延时，小于此时间的多次报警只发送一次邮件，客户端不受此限制
	BOOL bStarting;			//是否正在报警,由于客户端报警会一直持续，用来检测是否在向客户端发送报警

	BOOL bBuzzing;			//是否蜂鸣器报警
	BOOL bSendtoClient;		//是否发送至分控
	BOOL bSendEmail;		//是否发送邮件
}SPAlarmIn_t;

/**
 *@brief 布防
 *
 *@param channelid 报警输入通道
 */
int sp_alarmin_start(int channelid);

/**
 *@brief 撤防
 *
 *@param channelid 报警输入通道
 */
int sp_alarmin_stop(int channelid);

/**qlc20141124
 * @brief 获取报警输入信息
 *
 *@param channelid 报警输入通道
 * @param alarm 报警输入参数
 *
 * @return 0 成功
 */
int sp_alarmin_get_param(int channelid, SPAlarmIn_t *param);

/**qlc20141124
 * @brief 获取报警输入信息
 *
 *@param channelid 报警输入通道
 * @param alarm 报警输入参数
 *
 * @return 0 成功
 */
int sp_alarmin_set_param(int channelid, SPAlarmIn_t *param);

/**
 *@brief 布防状态
 *
 *@param channelid 报警输入通道
 *
 *@return 布防时为真，否则为假
 */
int sp_alarmin_b_onduty(int channelid);

/**
 *@brief 报警状态
 *
 *@param channelid 报警输入通道
 *
 *@return 正在报警时为真，否则为假
 */
int sp_alarmin_b_alarming(int channelid);

typedef struct
{
	char tStart[16];		// 安全防护开始时间(格式: hh:mm:ss)
	char tEnd[16];			// 安全防护结束时间(格式: hh:mm:ss)
}SPAlarmTime;

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

	//add by xianlt at 20120628
	int port;				//服务器端口
	char crypto[12];		//加密传输方式
	//add by xianlt at 20120628

	BOOL bEnable;			//安全防护总开关
	SPAlarmTime alarmTime[3];	// 安全防护时间段设置
	
	char vmsServerIp[20];	//VMS服务器IP地址
	unsigned short vmsServerPort;		//VMS服务器端口
	
	BOOL bAlarmSoundEnable;	//报警声音开关
	BOOL bAlarmLightEnable;	//报警光开关
}SPAlarmSet_t;

/**lk20131120
 * @brief 获取报警信息
 *
 * @param alarm 报警信息输出
 *
 * @return 0 成功
 */
int sp_alarm_get_param(SPAlarmSet_t *alarm);

/**lk20131120
 * @brief 设置报警信息
 *
 * @param alarm 新的报警信息
 *
 * @return 0 成功
 */
int sp_alarm_set_param(SPAlarmSet_t *alarm);

int sp_alarm_sound_start();

int sp_alarm_sound_stop();

int sp_alarm_light_start();

int sp_alarm_light_stop();

int sp_alarm_buzzing_open();

int sp_alarm_buzzing_close();

/**lk20131120
 * @brief 发送测试邮件
 */
int  sp_mail_test(unsigned char *szIsSucceed);

/**
 * @brief 向手机端发送布防撤防消息
 *
 * @param 
 */
int sp_alarm_send_deployment();

#ifdef __cplusplus
}
#endif


#endif /* SP_RECORD_H_ */
