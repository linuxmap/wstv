/*
 * jv_ptz.h
 *
 *  Created on: 2014年8月12日
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */
#ifndef JV_PTZ_H_
#define JV_PTZ_H_

#include <jv_common.h>

#if 0
#define FUNC_DEBUG() do{printf("%s running\n", __func__);}while(0)
#else
#define FUNC_DEBUG() do{}while(0)
#endif

/**
 *@brief 位置信息
 */
typedef struct{
	unsigned int left;		// left right position. 取值为 0 ～ 0xFFFF
	unsigned int up;		// up down position. 取值为 0 ～ 0xFFFF
}JVPTZ_Pos_t;

typedef struct{
	BOOL bValid; //是否支持
	BOOL bLeftUpTogether; //是否支持两个方向同时运转
}JVPTZ_Capability_t;
typedef enum
{
	Horizontal=0,
	Vertical,
	ALL,
}ENUM_LEFT_OR_UP;

/**
 *@brief 初始化
 */
int jv_ptz_init();

/**
 *@brief 结束
 */
int jv_ptz_deinit();

typedef struct{
	int (*fptr_jv_ptz_get_capability)(int channel, JVPTZ_Capability_t *capability);
	int (*fptr_jv_ptz_self_check)(int channel);
	int (*fptr_jv_ptz_get_origin_pos)(int channel, JVPTZ_Pos_t* pos);
	int (*fptr_jv_ptz_move_start)(int channel, int left, int up, int zoomin);
	int (*fptr_jv_ptz_move_stop)(int channel);
	int (*fptr_jv_ptz_move_to)(int channel, JVPTZ_Pos_t *position,ENUM_LEFT_OR_UP direction,int speed);
	BOOL (*fptr_jv_ptz_move_done)(int channel);
	int (*fptr_jv_ptz_pos_get)(int channel, JVPTZ_Pos_t *position);
}jv_ptz_func_t;


extern jv_ptz_func_t g_ptzfunc;

/**
 *@brief 本接口实现的云台控制，是否支持
 */
static int jv_ptz_get_capability(int channel, JVPTZ_Capability_t *capability)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_get_capability)
		return g_ptzfunc.fptr_jv_ptz_get_capability(channel, capability);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 本接口实现的云台控制，是否支持
 */
static int jv_ptz_self_check(int channel)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_self_check)
		return g_ptzfunc.fptr_jv_ptz_self_check(channel);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 本接口实现获取云台的原点位置
 */
static int jv_ptz_get_origin_pos(int channel, JVPTZ_Pos_t* pos)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_get_origin_pos)
		return g_ptzfunc.fptr_jv_ptz_get_origin_pos(channel, pos);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 实现云台的移动和缩放
 *
 *@param channel 通道号
 *@param left 向左移动的速度，范围-255~255
 *@param up 向上移动的速度，范围-255~255
 *@param zoomin 放大的速度，范围-255~255
 *
 *@note 三个值 <0 表示相反方向。 都为0，则表示停止
 */
static int jv_ptz_move_start(int channel, int left, int up, int zoomin)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_move_start)
		return g_ptzfunc.fptr_jv_ptz_move_start(channel, left, up, zoomin);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 停止移动（各种移动）
 *
 *@param channel 通道号
 */
static int jv_ptz_move_stop(int channel)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_move_stop)
		return g_ptzfunc.fptr_jv_ptz_move_stop(channel);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 调用具体某个坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
static int jv_ptz_move_to(int channel, JVPTZ_Pos_t *position,ENUM_LEFT_OR_UP direction,int speed)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_move_to)
		return g_ptzfunc.fptr_jv_ptz_move_to(channel, position,direction,speed);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 检查jv_ptz_mvoe_to 或者jv_ptz_move_start是否完成
 */
static BOOL jv_ptz_move_done(int channel)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_move_done)
		return g_ptzfunc.fptr_jv_ptz_move_done(channel);
	return TRUE;
}

/**
 *@brief 获取当前坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
static int jv_ptz_pos_get(int channel, JVPTZ_Pos_t *position)
{
	FUNC_DEBUG();
	if (g_ptzfunc.fptr_jv_ptz_pos_get)
		return g_ptzfunc.fptr_jv_ptz_pos_get(channel, position);
	return JVERR_FUNC_NOT_SUPPORT;
}


#endif /* JV_PTZ_H_ */
