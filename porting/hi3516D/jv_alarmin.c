/*
 * jv_alarmin.c
 *
 *  Created on: 2013-11-30
 *      Author: LK
 */
#include "hicommon.h"
#include "jv_common.h"
#include "jv_alarmin.h"
#include <jv_gpio.h>

typedef struct
{
	jv_alarmin_attr_t jv_alarmin_attr;	//报警输入属性

	jv_alarmin_callback_t callback_ptr;	//回调函数
	void *callback_param;				//回调函数参数
} jv_alarmin_t;

static jv_alarmin_t jv_alarmin;

static jv_thread_group_t group;

/**
 * @brief 报警输入信号侦测线程
 */
static void _jv_alarmin_process(void *param)
{
	S32 nRet, i, j, nResult;

	//报警输入检测状态,开为1,闭为0
	int bAlarmInStatus1,bAlarmInStatus2;
	int aiGroup1, aiBit1,aiGroup2, aiBit2;
	aiGroup1 = higpios.alarmin1.group;
	aiGroup2 = higpios.alarmin2.group;
	aiBit1 = higpios.alarmin1.bit;
	aiBit2 = higpios.alarmin2.bit;
	while (group.running)
	{
		pthread_mutex_lock(&group.mutex);
		bAlarmInStatus1 = jv_gpio_read(aiGroup1, aiBit1);
		bAlarmInStatus2 = jv_gpio_read(aiGroup2, aiBit2);
		pthread_mutex_unlock(&group.mutex);

		//printf("bAlarmInStatus:%d\n", bAlarmInStatus);
		//注：通过callback_ptr可判断是否启动了报警输入
		//常闭模式则断开时报警
		if(jv_alarmin.jv_alarmin_attr.bNormallyClosed)
		{
			bAlarmInStatus1 = ((jv_alarmin.jv_alarmin_attr.u8AlarmNum>>0)&0x1)?bAlarmInStatus1:0;
			bAlarmInStatus2 = ((jv_alarmin.jv_alarmin_attr.u8AlarmNum>>1)&0x1)?bAlarmInStatus2:0;
			if((bAlarmInStatus1 > 0 || bAlarmInStatus2 > 0)&& jv_alarmin.callback_ptr)
			{
					//jv_alarmin.callback_ptr(0, jv_alarmin.callback_param);
				if(bAlarmInStatus1 > 0)
					jv_alarmin.callback_ptr(0, (void*)1);
				else if(bAlarmInStatus2 > 0)
					jv_alarmin.callback_ptr(0, (void*)2);
			}
		}
		else//常开模式则连接时报警
		{
			bAlarmInStatus1 = ((jv_alarmin.jv_alarmin_attr.u8AlarmNum>>0)&0x1)?bAlarmInStatus1:1;
			bAlarmInStatus2 = ((jv_alarmin.jv_alarmin_attr.u8AlarmNum>>1)&0x1)?bAlarmInStatus2:1;
			if((bAlarmInStatus1 == 0 || bAlarmInStatus2 ==0)&& jv_alarmin.callback_ptr)
			{
				//jv_alarmin.callback_ptr(0, jv_alarmin.callback_param);
				if(bAlarmInStatus1 == 0)
					jv_alarmin.callback_ptr(0, (void*)1);
				else if(bAlarmInStatus2 == 0)
					jv_alarmin.callback_ptr(0, (void*)2);
			}
		}
		usleep(200*1000);
	}
}

/**
 *@brief  初始化
 *@return 0 成功
 */
int jv_alarmin_init(void)
{
	group.running = TRUE;
	pthread_mutex_init(&group.mutex, NULL);
	pthread_create(&group.thread, NULL, (void *) _jv_alarmin_process, NULL);
	return 0;
}

/**
 *@brief 结束
 *@return 0 成功
 */
int jv_alarmin_deinit(void)
{
	group.running = FALSE;
	pthread_join(group.thread, NULL);
	pthread_mutex_destroy(&group.mutex);
	return 0;
}

/**
 * @brief 获取报警输入属性信息
 *
 * @param channelid 通道号
 * @param attr 属性信息输出
 *
 * @return 0 成功
 */
int jv_alarmin_get_param(int channelid, jv_alarmin_attr_t *attr)
{
	memcpy(attr, &jv_alarmin.jv_alarmin_attr, sizeof(jv_alarmin_attr_t));
	return 0;
}

/**
 * @brief 设置报警输入属性信息
 *
 * @param channelid 通道号
 * @param attr 属性信息
 *
 * @return 0 成功
 */
int jv_alarmin_set_param(int channelid, jv_alarmin_attr_t *attr)
{
	memcpy(&jv_alarmin.jv_alarmin_attr, attr, sizeof(jv_alarmin_attr_t));
	return 0;
}

/**
 *@brief 开始报警输入检测
 *@param 通道号
 *@param 回调函数，检测到报警输入信号时被调用
 *@param 回调函数参数
 *@return 0 成功
 */
int jv_alarmin_start(int channelid, jv_alarmin_callback_t callback, void *param)
{
	pthread_mutex_lock(&group.mutex);
	jv_alarmin.callback_ptr = callback;
	jv_alarmin.callback_param = param;
	pthread_mutex_unlock(&group.mutex);
	return 0;
}

/**
 *@brief 停止报警输入侦测
 *@param 通道号
 *@return 0 成功
 */
int jv_alarmin_stop(int channelid)
{
	pthread_mutex_lock(&group.mutex);
	jv_alarmin.callback_ptr = NULL;
	jv_alarmin.callback_param = NULL;
	pthread_mutex_unlock(&group.mutex);
	return 0;
}
int jv_alarm_buzzing_on(int bON)
{
	jv_gpio_write(higpios.alarmout1.group,higpios.alarmout1.bit,bON);
	return 0;
}
