/*
 * jv_alarmin.h
 *
 *  Created on: 2013-11-30
 *      Author: LK
 */

#ifndef JV_ALARMIN_H_
#define JV_ALARMIN_H_

#include "jv_common.h"

typedef struct{
	BOOL bNormallyClosed;	//是否常闭。其值为真时，断开报警。反之闭合报警
	U8  u8AlarmNum;			//报警路数控制，每一位表示一路报警，1开0关，暂只有两路报警,只有后两位有效
}jv_alarmin_attr_t;

typedef void (*jv_alarmin_callback_t)(int channelid, void *param);

/**
 *@brief  初始化
 *@return 0 成功
 */
int jv_alarmin_init(void);

/**
 *@brief 结束
 *@return 0 成功
 */
int jv_alarmin_deinit(void);

/**
 * @brief 获取报警输入属性信息
 *
 * @param channelid 通道号
 * @param attr 属性信息输出
 *
 * @return 0 成功
 */
int jv_alarmin_get_param(int channelid, jv_alarmin_attr_t *attr);

/**
 * @brief 设置报警输入属性信息
 *
 * @param channelid 通道号
 * @param attr 属性信息
 *
 * @return 0 成功
 */
int jv_alarmin_set_param(int channelid, jv_alarmin_attr_t *attr);

/**
 *@brief 开始报警输入检测
 *@param 通道号
 *@param 回调函数，检测到报警输入信号时被调用
 *@param 回调函数参数
 *@return 0 成功
 */
int jv_alarmin_start(int channelid, jv_alarmin_callback_t callback, void *param);

/**
 *@brief 停止报警输入侦测
 *@param 通道号
 *@return 0 成功
 */
int jv_alarmin_stop(int channelid);

/*
 * 报警输出，先放在这里吧，只有一个接口
 */
int jv_alarm_buzzing_on(int bON);


#endif /* JV_ALARMIN_H_ */
