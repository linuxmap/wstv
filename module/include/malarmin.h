/*
 * malarmin.h
 *
 *  Created on: 2013-11-25
 *      Author: lfx
 */

#ifndef MALARMIN_H_
#define MALARMIN_H_
#include <jv_common.h>

typedef struct{
	BOOL bEnable;			//是否启动报警输入
	BOOL bNormallyClosed;	//是否常闭。其值为真时，断开报警。反之闭合报警

	U8  u8AlarmNum;			//报警路数控制，每一位表示一路报警，1开0关，暂只有两路报警,只有后两位有效

	BOOL bEnableRecord;		//是否开启报警录像

	int nDelay;				//报警延时，小于此时间的多次报警只发送一次邮件，客户端不受此限制
	BOOL bStarting;			//是否正在报警,由于客户端报警会一直持续，用来检测是否在向客户端发送报警

	BOOL bBuzzing;			//是否蜂鸣器报警
	BOOL bSendtoClient;		//是否发送至分控
	BOOL bSendEmail;		//是否发送邮件
	BOOL bSendtoVMS;		//是否发送至VMS服务器
}MAlarmIn_t;

extern int ma_timer;


/**
 *@brief 需要报警时的回调函数
 * 该函数主要用于通知分控，是否有警报发生
 *@param channelid 发生报警的通道号
 *@param bAlarmOn 报警开启或者关闭
 *
 */
typedef void (*alarmin_alarming_callback_t)(int channelid, BOOL bAlarmOn);

/**
 * @brief 注册报警输入，向客户端发送警报的回调函数
 */
int malarmin_set_callback(alarmin_alarming_callback_t callback);

/**
 * @brief 初始化
 */
int malarmin_init(void);

/**
 * @brief 结束
 */
int malarmin_deinit(void);

/**
 * @brief 设置报警参数
 * @param channel 通道号
 * @param param 报警参数
 *
 * @return 0成功
 */
int malarmin_set_param(int channel, MAlarmIn_t *param);

/**
 * @brief 获取报警参数
 *
 * @param channel 通道号
 * @param param 报警信息输出
 *
 * @return 0 成功
 */
int malarmin_get_param(int channel, MAlarmIn_t *param);

/**
 * @brief 开始检测报警输入信号
 *
 * @param channel 通道号
 *
 * @return 0 成功
 */
int malarmin_start(int channel);

/**
 * @brief 停止检测报警输入信号
 *
 * @param channel 通道号
 *
 * @return 0 成功
 */
int malarmin_stop(int channel);

/**
 *@brief 使设置生效
 *	在#malarmin_set_param之后，如果改变了使能状态，调用本函数
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int malarmin_flush(int channelid);


int _malarmin_process(int tid, void *param);



#endif /* MALARMIN_H_ */
