#ifndef JV_UARTCOMM_H
#define JV_UARTCOMM_H

#include "jv_common.h"

enum
{
	STC_NONE,
	STC_MOTOR_UD_MAX_STEP,
	STC_MOTOR_LR_MAX_STEP,
	STC_MOTOR_UP,
	STC_MOTOR_DOWN,
	STC_MOTOR_LEFT,
	STC_MOTOR_RIGHT,
	STC_MOTOR_STOP,
	STC_MOTOR_SET_SPEED,
	STC_MOTOR_GET_UD_STEP,
	STC_MOTOR_GET_LR_STEP,
	STC_MOTOR_UD_WORK_STEP,
	STC_MOTOR_LR_WORK_STEP,
	STC_MOTOR_IS_DONE
};



typedef struct{
	
	int (*fptr_uartcomm_doorID_get)(unsigned int* data);		//从jv层获得数据
	int (*fptr_uartcomm_sendB)(unsigned char* data, unsigned int len);
	int (*fptr_uartcomm_send)(unsigned char cmd, unsigned int data);	//发送命令到单片机
	int (*fptr_uartcomm_motor_get_done)(unsigned char* done);	//获取motor是否初始化完成
	int (*fptr_uartcomm_motor_get_step)(char index, unsigned int* step);
}jv_uartcomm_func_t;

extern jv_uartcomm_func_t g_uartcomm_func;

static int jv_uartcomm_doorID_get(unsigned int* data)
{
	if (g_uartcomm_func.fptr_uartcomm_doorID_get)
		return g_uartcomm_func.fptr_uartcomm_doorID_get(data);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_uartcomm_sendB(unsigned char* data, unsigned int len)
{
	if (g_uartcomm_func.fptr_uartcomm_sendB)
		return g_uartcomm_func.fptr_uartcomm_sendB(data, len);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_uartcomm_send(unsigned char cmd, unsigned int data)
{
	if (g_uartcomm_func.fptr_uartcomm_send)
		return g_uartcomm_func.fptr_uartcomm_send(cmd, data);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_uartcomm_motor_get_done(unsigned char* done)
{
	if (g_uartcomm_func.fptr_uartcomm_motor_get_done)
		return g_uartcomm_func.fptr_uartcomm_motor_get_done(done);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_uartcomm_motor_get_step(char index, unsigned int* step)
{
	if (g_uartcomm_func.fptr_uartcomm_motor_get_step)
		return g_uartcomm_func.fptr_uartcomm_motor_get_step(index, step);
	return JVERR_FUNC_NOT_SUPPORT;
}


void jv_uartcomm_init(void);



#endif
