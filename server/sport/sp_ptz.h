/*
 * sp_ptz.h
 *
 *  Created on: 2013-11-1
 *      Author: lfx
 */

#ifndef SP_PTZ_H_
#define SP_PTZ_H_

#ifdef __cplusplus
extern "C" {
#endif

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
int sp_ptz_move_start(int channel, int left, int up, int zoomin);

/**
 *@brief 停止移动（各种移动）
 *
 *@param channel 通道号
 */
int sp_ptz_move_stop(int channel);

/**
 *@brief 光圈控制，变焦控制
 *
 *@param channel 通道号
 *@param focusFar 往远聚集的速度，范围-255~255
 *@param irisOpen 增大光圈的速度，范围-255~255
 */
int sp_ptz_fi_start(int channel, int focusFar, int irisOpen);

/**
 *@brief 光圈控制
 *
 *@param channel 通道号
 */
int sp_ptz_fi_stop(int channel);

/**
 *@brief 获取空闲的预置点点位
 */
int sp_ptz_get_free_presetno(int channel);

/**
 *@brief 设置预置位
 *
 *@param channel 通道号
 *@param presetno 预置位号，如果为-1，则分配一个可用的
 *@param name 预置位名字
 *@return: >= 0 成功，其值意义为预置点号
 * 			-1 预置点非法
 * 			-2 预置点重复
 * 			-3 预置点数量到达上限
 */
int sp_ptz_preset_set(int channel, int presetno, char *name);

/**
 *@brief 调用预置位
 *
 *@param channel 通道号
 *@param presetno 预置位号
 */
int sp_ptz_preset_locate(int channel, int presetno);

/**
 *@brief 删除预置位
 *
 *@param channel 通道号
 *@param presetno 预置位号
 *@return 0成功
 *			 -1要删除的预置号不存在
 */
int sp_ptz_preset_delete(int channel, int presetno);

/**
 *@brief 获取设置过的预置点的个数
 *
 *@param channel 通道号
 */
int sp_ptz_preset_get_cnt(int channel);

typedef struct {
	int presetno;
	char name[32];
} SPPreset_t;
/**
 *@brief 获取预置点信息
 *
 *@param channel 通道号
 *@param index 序号
 *@param preset 输出 预置点信息
 */
int sp_ptz_preset_get(int channel, int index, SPPreset_t *preset);

typedef struct{
	int patrolid;
}SPPatrol_t;

/**
 *@brief 创建一个巡航
 *
 *@param channel
 *
 *@return index of patrol
 */
int sp_ptz_patrol_create(int channel);

/**
 *@brief 销毁一个巡航
 */
int sp_ptz_patrol_delete(int channel, int index);

/**
 *@brief 获取巡航线个数
 */
int sp_ptz_patrol_get_cnt(int channel);

/**
 *@brief 获取巡航线
 *
 *@param channel 通道号
 *@param index
 *@param patrol
 *
 *@return 0 if success
 */
int sp_ptz_patrol_get(int channel, int index, SPPatrol_t *patrol);

/**
 *@brief 获取巡航线
 *
 *@param channel 通道号
 *@param patrol
 *
 *@return index of patrol
 */
int sp_ptz_patrol_get_index(int channel, SPPatrol_t *patrol);

/**
 *@brief 获取巡航线的预置点数量
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 */
int sp_ptz_patrol_get_node_cnt(int channel, SPPatrol_t *patrol);

typedef struct{
	SPPreset_t preset;
	int staySeconds;
}SPPatrolNode_t;

/**
 *@brief 获取巡航线的预置点
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param index 序号
 */
int sp_ptz_patrol_get_node(int channel, SPPatrol_t *patrol, int index, SPPatrolNode_t *node);

/**
 *@brief 增加一个巡航点
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param presetno 预置位号
 *@param staySeconds 停留的秒数
 */
int sp_ptz_patrol_add_node(int channel, SPPatrol_t *patrol, int presetno, int staySeconds);

/**
 *@brief 删除一个巡航点
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param presetindex 预置位索引号。 presetindex为-1时，删除其上的所有预置点
 */
int sp_ptz_patrol_del_node(int channel, SPPatrol_t *patrol, int presetindex);

/**
 *@brief 设置巡航速度
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param speed 速度，取值 0~255
 */
int sp_ptz_patrol_set_speed(int channel, SPPatrol_t *patrol, int speed);

/**
 *@brief 设置巡航停留时间。将会改变所有预置点的停留时间
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param staySeconds 停留的秒数
 */
int sp_ptz_patrol_set_stay_seconds(int channel, SPPatrol_t *patrol, int staySeconds);

/**
 *@brief 开始巡航
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 */
int sp_ptz_patrol_locate(int channel, SPPatrol_t *patrol);

/**
 *@brief 停止巡航
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 */
int sp_ptz_patrol_stop(int channel, SPPatrol_t *patrol);

/**
 *@brief 开始录制轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_rec_start(int channel, int nTrail);

/**
 *@brief 停止录制轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_rec_stop(int channel, int nTrail);

/**
 *@brief 开始轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_start(int channel, int nTrail);

/**
 *@brief 停止轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_stop(int channel, int nTrail);

/**
 *@brief 设置扫描左边界
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_set_left(int channel, int groupid);

/**
 *@brief 设置扫描右边界
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_set_right(int channel, int groupid);

/**
 *@brief 开始扫描
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_start(int channel, int groupid);

/**
 *@brief 结束扫描
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_stop(int channel, int groupid);

/**
 *@brief 设置扫描速度
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 *@param speed 扫描速度，0~255
 */
int sp_ptz_scan_set_speed(int channel, int groupid, int speed);

/**
 *@brief 设置云台自动
 *
 *@param channel 通道号
 *@param speed 速度，0~255
 */
int sp_ptz_auto(int channel, unsigned int speed);

/**
 *@brief 开启辅助
 *
 *@param channel 通道号
 *@param n 第n项辅助功能
 */
int sp_ptz_aux_on(int channel, int n);

/**
 *@brief 设置扫描速度
 *
 *@param channel 通道号
 *@param n 第n项辅助功能
 */
int sp_ptz_aux_off(int channel, int n);

/**
 *@brief 云台菜单操作
 *
 *@param channel 通道号
 *@param n:0-on,1-off,2-ok,3-return;4-up,5-down,6-left,7-right
 */
int sp_ptz_menu_op(int channel, int n);


//3D定位结构体
typedef struct _POSITION
{
	int x;		//客户端圈定区域中心 x坐标
	int y;		//客户端圈定区域中心 y坐标
	int w;		//客户端圈定区域宽
	int h;		//客户端圈定区域高
} SPPosition_t;
/**
 *@brief 3D定位功能
 *
 *@param :channel 通道号
 *@param :pos 圈定区域坐标及宽高信息，cmd 放大/缩小命令
 */
int sp_ptz_position(int channel, SPPosition_t *pos, int cmd);

#ifdef __cplusplus
}
#endif



#endif /* SP_PTZ_H_ */
