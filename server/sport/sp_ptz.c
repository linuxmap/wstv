#include "sp_define.h"
#include "sp_ptz.h"
#include "mptz.h"
#include <math.h>
#include <SYSFuncs.h>
#include "MRemoteCfg.h"
#include "muartcomm.h"
#include "mstream.h"
#include "msensor.h"

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
int sp_ptz_move_start(int channel, int left, int up, int zoomin)
{
	__FUNC_DBG__();
	BOOL bLeft, bUp;

	if (left == 0 && up == 0)
	{
		if (zoomin > 0 && zoomin <= 255)
		{
			PtzZoomInStart(channel);			
		}
		else if (zoomin >= -255 && zoomin < 0)
		{
			PtzZoomOutStart(channel);
		}
		else if (0 == zoomin)
		{
			PtzZoomOutStop(channel);
		}
		else
		{
			printf("param zoomin error!");
			return -1;
		}
		return 0;
	}


	if (left >= 0 && left <= 255)
	{
		bLeft = TRUE;
	}
	else if (left >= -255 && left < 0)
	{
		bLeft = FALSE;
	}
	else
	{
		printf("param left error!");
		return -1;
	}

	if (up >= 0 && up <= 255)
	{
		bUp = TRUE;
	}
	else if (up >= -255 && up < 0)
	{
		bUp = FALSE;
	}
	else
	{
		printf("param up error!");
		return -1;
	}
	PtzPanTiltStart(channel, bLeft, bUp, abs(left), abs(up));

	return 0;
}

/**
 *@brief 停止移动（各种移动）
 *
 *@param channel 通道号
 */
int sp_ptz_move_stop(int channel)
{

	__FUNC_DBG__();
	PtzPanTiltStop(channel);
	return 0;
}

/**
 *@brief 光圈控制，变焦控制
 *
 *@param channel 通道号
 *@param focusFar 往远聚集的速度，范围-255~255
 *@param irisOpen 增大光圈的速度，范围-255~255
 */
int sp_ptz_fi_start(int channel, int focusFar, int irisOpen)
{
	
	__FUNC_DBG__();
	if (focusFar > 0 && focusFar < 255)
	{
		PtzFocusFarStart(channel);
	}
	else if (focusFar < 0 && focusFar > -255)
	{
		PtzFocusNearStart(channel);
	}
	else if (irisOpen > 0 && irisOpen < 255)
	{
		PtzIrisOpenStart(channel);
	}
	else if (irisOpen < 0 && irisOpen > -255)
	{
		PtzIrisCloseStart(channel);
	}
	else
	{
		printf("param irisOpen error!");
		return -1;
	}
	return 0;
}

/**
 *@brief 光圈控制
 *
 *@param channel 通道号
 */
int sp_ptz_fi_stop(int channel)
{
	
	__FUNC_DBG__();
	PtzFocusNearStop(channel);

	PtzIrisOpenStop(channel);
	return 0;
}

/**
 *@brief 获取空闲的预置点点位
 */
int sp_ptz_get_free_presetno(int channel)
{
	PTZ_PRESET_INFO *pptz_preset_info;
	pptz_preset_info = PTZ_GetPreset();
	int i;
	int no;
	for (no=1;no<MAX_PRESET_CT;no++)
	{
		for (i=0;i<pptz_preset_info[channel].nPresetCt;i++)
		{
			if (no == pptz_preset_info[channel].pPreset[i])
			{
				break;
			}
		}
		if (i == pptz_preset_info[channel].nPresetCt)
		{
			return no;
		}
	}

	return -1;
}

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
int sp_ptz_preset_set(int channel, int presetno, char *name)
{
	
	__FUNC_DBG__();
	if (presetno == -1)
	{
		presetno = sp_ptz_get_free_presetno(channel);
	}
	int ret = PTZ_AddPreset(channel, presetno, name);
	if (ret < 0)
		return ret;
	WriteConfigInfo();
	return presetno;
}

/**
 *@brief 调用预置位
 *
 *@param channel 通道号
 *@param presetno 预置位号
 */
int sp_ptz_preset_locate(int channel, int presetno)
{
	
	__FUNC_DBG__();
	PtzLocatePreset(channel, presetno);
	return 0;
}

/**
 *@brief 删除预置位
 *
 *@param channel 通道号
 *@param presetno 预置位号
 *@return 0成功
 *			 -1要删除的预置号不存在
 */
int sp_ptz_preset_delete(int channel, int presetno)
{
	
	__FUNC_DBG__();
	int ret = PTZ_DelPreset(channel, presetno);
	WriteConfigInfo();

	return ret;
}

/**
 *@brief 获取设置过的预置点的个数
 *
 *@param channel 通道号
 */
int sp_ptz_preset_get_cnt(int channel)
{
	__FUNC_DBG__();
	PTZ_PRESET_INFO *pptz_preset_info;
	pptz_preset_info = PTZ_GetPreset();
	return pptz_preset_info[channel].nPresetCt;
}

/**
 *@brief 获取预置点信息
 *
 *@param channel 通道号
 *@param index 序号
 *@param preset 输出 预置点信息
 */
int sp_ptz_preset_get(int channel, int index, SPPreset_t *preset)
{
	__FUNC_DBG__();
	PTZ_PRESET_INFO * pptz_preset_info;
	pptz_preset_info = PTZ_GetPreset();

	preset->presetno = pptz_preset_info[channel].pPreset[index];
	strcpy(preset->name, pptz_preset_info[channel].name[index]);

	return 0;
}

/**
 *@brief 创建一个巡航
 *
 *@param channel
 *
 *@return index of patrol
 */
int sp_ptz_patrol_create(int channel)
{
	return 0;
}

/**
 *@brief 销毁一个巡航
 */
int sp_ptz_patrol_delete(int channel, int index)
{
	if(index < 0 || index > 1)
	{
		printf("only support 2 patrolpath	:%d\n",index);
		return -1;
	}
	SPPatrol_t patrol;
	patrol.patrolid = index;
	int ret = sp_ptz_patrol_del_node(channel, &patrol, -1);
	if(ret <0 )
		printf("sp_ptz_patrol_del_node error 	:%d\n", patrol.patrolid);
	return 0;
}

/**
 *@brief 获取巡航线个数，当前只有一个
 */
int sp_ptz_patrol_get_cnt(int channel)
{
	return 1;
}

/**
 *@brief 获取巡航线
 *
 *@param index
 *@param patrol
 *
 *@return 0 if success
 */
int sp_ptz_patrol_get(int channel, int index, SPPatrol_t *patrol)
{
	patrol->patrolid = 0;
	return 0;
}

/**
 *@brief 获取巡航线的预置点数量
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 */
int sp_ptz_patrol_get_node_cnt(int channel, SPPatrol_t *patrol) //patrolid,没用到
{
	__FUNC_DBG__();
	if(patrol->patrolid < 0 || patrol->patrolid > 1)
	{
		printf("only support 2 patrolpath	:%d\n",patrol->patrolid);
		return -1;
	}
	//printf("sp_ptz_patrol_get_node_cnt: 	patid:%d\n", patrol->patrolid);
	PTZ_PATROL_INFO *pptz_patrol_info;
	pptz_patrol_info = PTZ_GetPatrol();
	return pptz_patrol_info[patrol->patrolid].nPatrolSize;
}

/**
 *@brief 获取巡航线的预置点
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param index 序号
 */
int sp_ptz_patrol_get_node(int channel, SPPatrol_t *patrol, int index,
		SPPatrolNode_t *node) //patrolid,没用到
{
	if(patrol->patrolid < 0 || patrol->patrolid > 1)
	{
		printf("only support 2 patrolpath	:%d\n",patrol->patrolid);
		return -1;
	}
	int nPreset;
	int i = 0;
	PTZ_PATROL_INFO *pptz_patrol_info;
	PTZ_PRESET_INFO * pptz_preset_info;
	__FUNC_DBG__();
	//printf("sp_ptz_patrol_get_node: 	patid:%d\n", patrol->patrolid);
	pptz_patrol_info = PTZ_GetPatrol();
	if (index > pptz_patrol_info[patrol->patrolid].nPatrolSize)
	{
		printf("overtop PatroSize!");
		return -1;
	}
	nPreset = pptz_patrol_info[patrol->patrolid].aPatrol[index].nPreset;

	pptz_preset_info = PTZ_GetPreset();

	node->staySeconds = pptz_patrol_info[patrol->patrolid].aPatrol[index].nStayTime;
	node->preset.presetno = pptz_patrol_info[patrol->patrolid].aPatrol[index].nPreset;
	for(i=0; i<pptz_preset_info[channel].nPresetCt; i++)
	{
		if(pptz_preset_info[channel].pPreset[i] == node->preset.presetno)
			strcpy(node->preset.name, pptz_preset_info[channel].name[i]);
	}

	return 0;
}

/**
 *@brief 增加一个巡航点
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param presetno 预置位号
 *@param staySeconds 停留的秒数
 */
int sp_ptz_patrol_add_node(int channel, SPPatrol_t *patrol, int presetno,
		int staySeconds) //patrolid,没用到
{
	if(patrol->patrolid < 0 || patrol->patrolid > 1)
	{
		printf("only support 2 patrolpath	:%d\n",patrol->patrolid);
		return -1;
	}
    //printf("+++1    patid:%d  presetno:%d  staytime:%d\n", patrol->patrolid, presetno, staySeconds);
	__FUNC_DBG__();
	PTZ_PRESET_INFO * pptz_preset_info;
	PTZ_PATROL_INFO *pptz_patrol_info;
	int i, ret;
	pptz_preset_info = PTZ_GetPreset();
	for (i = 0; i < pptz_preset_info[channel].nPresetCt; i++)
	{
		if (presetno == pptz_preset_info[channel].pPreset[i])
		{
			break;
		}
	}
	if (i == pptz_preset_info[channel].nPresetCt)
	{
		printf("can not find!");
		return -1;
	}

	pptz_patrol_info = PTZ_GetPatrol();
    //printf("+++2    patid:%d  [i]:%d  staytime:%d\n", patrol->patrolid, presetno, staySeconds);
	ret = AddPatrolNod(&pptz_patrol_info[patrol->patrolid], presetno, staySeconds);

	WriteConfigInfo();
	if (ret == -1)
	{
		printf("overtop MAX_PATROL_NOD");
		return -1;
	}

	return 0;
}

/**
 *@brief 删除一个巡航点
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param presetindex 预置位在巡航线中的索引号。 presetindex为-1时，删除其上的所有预置点
 */
int sp_ptz_patrol_del_node(int channel, SPPatrol_t *patrol, int presetindex) //patrolid,没用到
{
	if(patrol->patrolid < 0 || patrol->patrolid > 1)
	{
		printf("only support 2 patrolpath	:%d\n",patrol->patrolid);
		return -1;
	}
    //printf("---1   patid:%d  presetno:%d\n", patrol->patrolid, presetindex);
	__FUNC_DBG__();
	PTZ_PATROL_INFO *pptz_patrol_info;
	int i, ret;
	PTZ_PRESET_INFO * pptz_preset_info;
	pptz_preset_info = PTZ_GetPreset();
	pptz_patrol_info = PTZ_GetPatrol();

	if (presetindex == -1)
	{
		for (i = pptz_patrol_info[patrol->patrolid].nPatrolSize - 1; i >= 0; i--)
		{
			DelPatrolNod(&pptz_patrol_info[patrol->patrolid], i);
		}
		WriteConfigInfo();
		return 0;
	}

	/*
	for (i = 0; i < pptz_preset_info[channel].nPresetCt; i++)
	{
		if (presetindex == pptz_preset_info[channel].pPreset[i])
		{
			break;
		}
	}
	if (i == pptz_preset_info[channel].nPresetCt)
	{
		printf("can not find in preset!");
		return -1;
	}
	
	for (i=0; i < pptz_patrol_info[patrol->patrolid].nPatrolSize; i++)
	{
		if (presetindex == pptz_patrol_info[patrol->patrolid].aPatrol[i].nPreset)
		{
			break;
		}
	}
	if (i == pptz_patrol_info[patrol->patrolid].nPatrolSize)
	{
		printf("can not find in patrol!");
		return -1;
	}
	*/
	ret = DelPatrolNod(&pptz_patrol_info[patrol->patrolid], presetindex);
	if (ret == -1)
	{
		printf("delete failed!");
		return -1;
	}
	WriteConfigInfo();

	return 0;
}

/**
 *@brief 设置巡航速度
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param speed 速度，取值 0~255
 */
int sp_ptz_patrol_set_speed(int channel, SPPatrol_t *patrol, int speed)
{
	__FUNC_DBG__();
	return 0;
}

/**
 *@brief 设置巡航停留时间。将会改变所有预置点的停留时间
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 *@param staySeconds 停留的秒数
 */
int sp_ptz_patrol_set_stay_seconds(int channel, SPPatrol_t *patrol, int staySeconds)
{
	if(patrol->patrolid < 0 || patrol->patrolid > 1)
	{
		printf("only support 2 patrolpath	:%d\n",patrol->patrolid);
		return -1;
	}
	__FUNC_DBG__();
	PTZ_PATROL_INFO *pptz_patrol_info;
	int i, presetindex, ret;
	//printf("sp_ptz_patrol_set_stay_seconds:  patid:%d\n", patrol->patrolid);
	pptz_patrol_info = PTZ_GetPatrol();
	for (i = 0; i < pptz_patrol_info[patrol->patrolid].nPatrolSize; i++)
	{
		pptz_patrol_info[patrol->patrolid].aPatrol[i].nStayTime = staySeconds;
	}
	WriteConfigInfo();
	return 0;
}

/**
 *@brief 开始巡航
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 */
int sp_ptz_patrol_locate(int channel, SPPatrol_t *patrol)
{
	if(patrol->patrolid < 0 || patrol->patrolid > 1)
	{
		printf("only support 2 patrolpath	:%d\n",patrol->patrolid);
		return -1;
	}
	
	__FUNC_DBG__();
	PTZ_StartPatrol(channel, patrol->patrolid);
	return 0;
}

/**
 *@brief 停止巡航
 *
 *@param channel 通道号
 *@param patrolid 巡航线号
 */
int sp_ptz_patrol_stop(int channel, SPPatrol_t *patrol)
{
	__FUNC_DBG__();
	PTZ_StopPatrol(channel);
	return 0;
}

/**
 *@brief 开始录制轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_rec_start(int channel, int nTrail)
{
	
	__FUNC_DBG__();
	PtzTrailStartRec(channel,nTrail);
	return 0;
}

/**
 *@brief 停止录制轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_rec_stop(int channel, int nTrail)
{
	
	__FUNC_DBG__();
	PtzTrailStopRec(channel,nTrail);
	return 0;
}

/**
 *@brief 开始轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_start(int channel, int nTrail)
{
	
	__FUNC_DBG__();
	PtzTrailStart(channel,nTrail);
	return 0;
}

/**
 *@brief 停止轨迹
 *
 *@param channel 通道号
 *@param nTrail 
 */
int sp_ptz_trail_stop(int channel, int nTrail)
{
	
	__FUNC_DBG__();
	PtzTrailStop(channel,nTrail);
	return 0;
}

/**
 *@brief 设置扫描左边界
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_set_left(int channel, int groupid)
{
	
	__FUNC_DBG__();
	PtzLimitScanLeft(channel);
	return 0;
}

/**
 *@brief 设置扫描右边界
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_set_right(int channel, int groupid)
{
	
	__FUNC_DBG__();
	PtzLimitScanRight(channel);
	return 0;
}

/**
 *@brief 开始扫描
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_start(int channel, int groupid)
{
	
	__FUNC_DBG__();
	PTZ *ptz = PTZ_GetInfo();
	PtzLimitScanSpeed(channel, 0, ptz[0].scanSpeed);
	PtzLimitScanStart(channel, groupid);
	return 0;
}
/**
 *@brief 结束扫描
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 */
int sp_ptz_scan_stop(int channel, int groupid)
{
	
	__FUNC_DBG__();
	PtzLimitScanStop(channel, groupid);
	return 0;
}

/**
 *@brief 设置扫描速度
 *
 *@param channel 通道号
 *@param groupid 扫描组号
 *@param speed 扫描速度，0~255
 */
int sp_ptz_scan_set_speed(int channel, int groupid, int speed)
{
	
	__FUNC_DBG__();
	PtzLimitScanSpeed(channel, groupid, speed);
	return 0;
}

int sp_ptz_auto(int channel, unsigned int speed)
{
	
	PTZ_Auto(channel, speed);
	__FUNC_DBG__();
	return 0;
}

int sp_ptz_aux_on(int channel, int n)
{
	
	PtzAuxAutoOn(channel, n);
	__FUNC_DBG__();
	return 0;
}

int sp_ptz_aux_off(int channel, int n)
{
	
	PtzAuxAutoOff(channel, n);
	__FUNC_DBG__();
	return 0;
}

/**
 *@brief 云台菜单操作
 *
 *@param channel 通道号
 *@param n:0-on,1-off,2-ok,3-return;4-up,5-down,6-left,7-right
 */
int sp_ptz_menu_op(int channel, int n)
{
	
	__FUNC_DBG__();
	return 0;
}
/**
 *@brief 3D定位功能
 *
 *@param :channel 通道号
 *@param :pos 圈定区域坐标及宽高信息，cmd 放大/缩小命令
 */
int sp_ptz_position(int channel, SPPosition_t *pos, int cmd)
{
	
	__FUNC_DBG__();
	return 0;	
}

