/*
 * msoftptz.h
 *
 *  Created on: 2014年8月20日
 *      Author: lfx
 */

#ifndef MSOFTPTZ_H_
#define MSOFTPTZ_H_

#include <jv_common.h>
#include <jv_ptz.h>

#define FIRSTPOS_FILE CONFIG_PATH"firstpos.dat"
#define PATROL_FLAG_FLAG CONFIG_PATH"patrol.dat"

typedef struct patrol_info
{
	int chan;
	int patrol_path;
}PATROL_INFO;

int msoftptz_init();

int msoftptz_deinit();

/**
 *@brief 本接口实现的云台控制，是否支持
 */
BOOL msoftptz_b_support(int channel);

/**
 *@brief 本接口实现的云台控制自动旋转，是否支持自动
 */

BOOL msoftptz_auto_support(int channel);

/**
 *@brief 是否需要自检
 */
BOOL msoftptz_b_need_selfcheck();

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
int msoftptz_move_start(int channel, int left, int up, int zoomin);

/**
 *@brief 自动巡航
 */
int msoftptz_move_auto(int channel, int speed);

/**
 *@brief 停止移动（各种移动）
 *
 *@param channel 通道号
 */
int msoftptz_move_stop(int channel);

/**
 *@brief 调用具体某个坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
int msoftptz_goto(int channel, JVPTZ_Pos_t *position);

int msoftptz_setalarmPreSpeed_flag();

/**
 *@brief 获取当前坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
int msoftptz_pos_get(int channel, JVPTZ_Pos_t *position);

/**
 *@brief 获取上次设置过的速度
 		手动调整和巡航都可能改变过速度，
 		本函数返回离本次调用最近一次设置过的速度
*@return 速度
 */
int msoftptz_speed_get(int channel);
//客户端圈定区域，进行3D定位，并放大缩小显示
//(x, y, w, h) 圈定区域中心坐标及宽高; (width, height)当前码流分辨率
//zoom  3D定位指令:	0xC0 放大，0xC1 缩小
int  msoftptz_ZoomZone(int x, int y, int w, int h, int width, int height,int zoom);



#endif /* MSOFTPTZ_H_ */
