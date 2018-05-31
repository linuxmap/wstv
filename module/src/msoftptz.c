/*
 * msoftptz.c
 *
 *  Created on: 2014年8月20日
 *      Author: lfx
 */

#include <jv_ptz.h>
#include <msoftptz.h>
#include "utl_timer.h"
#include "SYSFuncs.h"
#include "mptz.h"
#include "mdetect.h"
#include "jv_ai.h"
#include <math.h>
#include <utl_cmd.h>
static int __msoftptz_pos_read(JVPTZ_Pos_t *lastpos);
static void __msoftptz_pos_save(JVPTZ_Pos_t *curpos);
static BOOL _timer_pos_save(int id, void* param);
static  int msoftptz_getSensorParam(double *SnW,double * SnH,double *SnF,double*StepAngleH,double * DriveRatioH, double*StepAngleV,double * DriveRatioV,int * maxLRStep,int * maxUDStep,int StreamWidth, int StreamHeight);

static BOOL g_bSoftptzRunning;
static int softptztimer=-1;
static JVPTZ_Pos_t stoppos;//保存手动移动后停止时的位置，过30秒后保存到flash，不在30s到了时获得位置是防止云台在30s内有巡航等移动
typedef enum{
	PTZ_CMD_IDLE,
	PTZ_CMD_STOP,
	PTZ_CMD_MOVE,// left, right, up, down
	PTZ_CMD_PRESET_LOCATE, //前往某个预置点
	PTZ_CMD_LOOP,//自动，一直转
}PTZCmd_t;


static struct{
	BOOL bValid; //是否支持
	BOOL bLeftUpTogether; //是否支持两个方向同时运转

	PTZCmd_t cmd; //当前执行的命令。主要用在本文中的线程中
	PTZCmd_t lastcmd;//记录上次执行过，move还是loop，用于确定最近哪个操作修改过速度
	int cmdIndex; //当前命令计数。用于标识是否命令发生改变
	struct{
		int left;
		int up;
		int zoomin;
	}moveSpeed; //#PTZ_CMD_MOVE 手动移动时的移动速度
	int speed; //#PTZ_CMD_PRESET_LOCATE, #PTZ_CMD_LOOP 的通用速度
	JVPTZ_Pos_t presetPos; //#PTZ_CMD_PRESET_LOCATE 预置点位置

	pthread_mutex_t mutex;
	pthread_t thread;
}sPTZInfo;

static int ptzSpeed_pre = 127;

static BOOL bAlarmPresetSpeed_Flag = FALSE;

static int __msoftptz_getpatrol(PATROL_INFO* patrol)
{
	FILE* fp = NULL;
	fp = fopen(PATROL_FLAG_FLAG,"rb");
	if(NULL == fp)
	{
		printf("%s,Failed open %s, because: %s\n", __func__,PATROL_FLAG_FLAG, strerror(errno));
		return -1;
	}
	
	int len;
	len = fread(patrol, 1, sizeof(PATROL_INFO), fp);
	printf("fread success\n");
	fclose(fp);
	if (len != sizeof(PATROL_INFO))
		return -1;
	return 0;
}

static int msoftptz_start_detect_timer = -1;
static BOOL __msoftptz_start_detect_timer_CB(int id, void* param)
{
	mdetect_enable();
	return 0;
}

void __msoftptz_start_detect()
{

	if (msoftptz_start_detect_timer == -1)
		msoftptz_start_detect_timer = utl_timer_create("__msoftptz_start_detect", 
		5*1000, __msoftptz_start_detect_timer_CB, NULL);
	
}

void __msoftptz_stop_detect()
{
	if (msoftptz_start_detect_timer != -1)
	{
		utl_timer_destroy(msoftptz_start_detect_timer);
		msoftptz_start_detect_timer = -1;
	}
	mdetect_disable();
}

static void *__msoftptz_thread(void *arg)
{
	pthreadinfo_add((char *)__func__);

	int channel = 0;

	if (msoftptz_b_need_selfcheck()
		|| gp.bFactoryFlag)
	{
		__msoftptz_stop_detect();
		jv_ptz_self_check(channel);

		JVPTZ_Pos_t pos;
		if(strcmp(hwinfo.devName, "JD-H40810") == 0)
		{
		}
		else
		{
			// 产测或支持巡航时，才回到中心点或上次定位点
			if ((gp.bFactoryFlag && gp.TestCfg.bYTOriginReset)
				|| hwinfo.bSupportPatrol)
			{
				//go to last position after self_check
				if(__msoftptz_pos_read(&pos)==0)
				{
					printf("go to last position(%d,%d)after self_check.\n",pos.left,pos.up);
					msoftptz_goto(channel,&pos);
				}
				else
				{
					printf("no position file.\n");
					// pos.left=0x8000;//最大0x10000，走到中间位置
					// pos.up=0x8000;
					pos = stoppos;
					msoftptz_goto(channel,&pos);
				}
			}
		}
	}

	if(access(PATROL_FLAG_FLAG,F_OK) == 0)
	{
		PATROL_INFO patrolinfo;
		if(__msoftptz_getpatrol(&patrolinfo) == 0)	
			PTZ_StartPatrol(patrolinfo.chan,patrolinfo.patrol_path);
	}
	
	while(g_bSoftptzRunning)
	{
		usleep(100*1000);
		if (PTZ_CMD_IDLE == sPTZInfo.cmd)
		{
			continue;
		}
		pthread_mutex_lock(&sPTZInfo.mutex);

		int cmdIndex = sPTZInfo.cmdIndex;
		int cmd = sPTZInfo.cmd;
		
		printf("current cmd: %d\n", cmd);
		if (cmd != PTZ_CMD_STOP)
		{
			__msoftptz_stop_detect();
		}
		else 
		{
			__msoftptz_start_detect();
		}
		switch(cmd)
		{
		case PTZ_CMD_STOP:
			jv_ai_SetMicMute(0);
			jv_ptz_move_stop(channel);
			//save current position
			pthread_mutex_unlock(&sPTZInfo.mutex);
			msoftptz_pos_get(0,&stoppos);
			if(softptztimer==-1)
			{
				softptztimer=utl_timer_create("save position",1000*30,_timer_pos_save,&stoppos);
			}
			else
			{
				utl_timer_reset(softptztimer,1000*30,_timer_pos_save,&stoppos);
			}
			pthread_mutex_lock(&sPTZInfo.mutex);
			break;
		case PTZ_CMD_MOVE:
			jv_ai_SetMicMute(1);
			jv_ptz_move_start(channel, sPTZInfo.moveSpeed.left, sPTZInfo.moveSpeed.up, sPTZInfo.moveSpeed.zoomin);
			break;
		case PTZ_CMD_PRESET_LOCATE:
			jv_ai_SetMicMute(1);
			if (sPTZInfo.bLeftUpTogether)
			{
				jv_ptz_move_to(channel, &sPTZInfo.presetPos,ALL,sPTZInfo.speed);
				while(cmdIndex == sPTZInfo.cmdIndex)
				{
					if (jv_ptz_move_done(channel))
						break;
					pthread_mutex_unlock(&sPTZInfo.mutex);
					usleep(100*1000);
					pthread_mutex_lock(&sPTZInfo.mutex);
				}
			}
			else
			{
				JVPTZ_Pos_t pos;
				pos.left = sPTZInfo.presetPos.left;
				pos.up = 0;
				if (jv_ptz_move_to(channel, &pos,Horizontal,sPTZInfo.speed) == 0)
				{
					while(cmdIndex == sPTZInfo.cmdIndex)
					{
						if (jv_ptz_move_done(channel))
							break;
						pthread_mutex_unlock(&sPTZInfo.mutex);
						usleep(100*1000);
						pthread_mutex_lock(&sPTZInfo.mutex);
					}
				}
				
				if (cmdIndex != sPTZInfo.cmdIndex)
					break;
				
				pos.left = 0;
				pos.up = sPTZInfo.presetPos.up;
				if (jv_ptz_move_to(channel, &pos,Vertical,sPTZInfo.speed) == 0)
				{
					while(cmdIndex == sPTZInfo.cmdIndex)
					{
						if (jv_ptz_move_done(channel))
							break;
						pthread_mutex_unlock(&sPTZInfo.mutex);
						usleep(100*1000);
						pthread_mutex_lock(&sPTZInfo.mutex);
					}
				}
				jv_ptz_move_stop(channel);
			}
			__msoftptz_start_detect();
			jv_ai_SetMicMute(0);
			break;

		case PTZ_CMD_LOOP:
			while(cmdIndex == sPTZInfo.cmdIndex)
			{
				jv_ptz_move_start(channel, sPTZInfo.speed, 0, 0);
				while(cmdIndex == sPTZInfo.cmdIndex)
				{
					if (jv_ptz_move_done(channel))
						break;
					pthread_mutex_unlock(&sPTZInfo.mutex);
					usleep(100*1000);
					pthread_mutex_lock(&sPTZInfo.mutex);
				}
				if (cmdIndex != sPTZInfo.cmdIndex)
				{
					break;
				}
				sPTZInfo.speed = -sPTZInfo.speed;
			}
			jv_ptz_move_stop(channel);
			break;
		}
		if (cmdIndex == sPTZInfo.cmdIndex)
			sPTZInfo.cmd = PTZ_CMD_IDLE;
		pthread_mutex_unlock(&sPTZInfo.mutex);
	}
	printf("out of __msoftptz_thread\n");
	return NULL;
}

#if 0
static int sptz_debug_main(int argc, char *argv[])
{
	int disx,disy;
	double anglex,angley;
	double SnW, SnH,SnF,StepAngleH,DriveRatioH,StepAngleV,DriveRatioV;
	JVPTZ_Pos_t  curpos,dstpos;
	int maxLRStep,maxUDStep;
	if(msoftptz_getSensorParam(&SnW,&SnH,&SnF,&StepAngleH,&DriveRatioH,&StepAngleV,&DriveRatioV, &maxLRStep,&maxUDStep,1280, 960)<0)
		return -1;
	 if(strcmp(argv[1], "right")==0)				
	{
		msoftptz_pos_get(0, &curpos);
		dstpos.left =curpos.left+761*0x10000 /maxLRStep;
		dstpos.up =curpos.up;
		msoftptz_goto(0, &dstpos);


	}
	else  if(strcmp(argv[1], "left")==0)				
	{
		msoftptz_pos_get(0, &curpos);
		dstpos.left =curpos.left-761*0x10000 /maxLRStep;
		dstpos.up =curpos.up;
		msoftptz_goto(0, &dstpos);

	}
	return 0;
}
#endif
int msoftptz_init()
{
	JVPTZ_Capability_t cap = {FALSE, FALSE};
	memset(&sPTZInfo, 0, sizeof(sPTZInfo));
	
	jv_ptz_init();
	jv_ptz_get_capability(0, &cap);
	sPTZInfo.bValid = cap.bValid;
	if (sPTZInfo.bValid == FALSE)
	{
		return -1;
	}

	sPTZInfo.bLeftUpTogether = cap.bLeftUpTogether;

	hwinfo.ptzBsupport = TRUE;

	// 获取原点位置
	if (jv_ptz_get_origin_pos(0, &stoppos) != 0)
	{
		stoppos.left = 0x8000;
		stoppos.up = 0x8000;
	}

	pthread_mutex_init(&sPTZInfo.mutex, 0);
	g_bSoftptzRunning = TRUE;
	pthread_create(&sPTZInfo.thread, NULL, __msoftptz_thread, NULL);
	//utl_cmd_insert("sptz", "sptz debug", "\nexp: sptz param1 \n",sptz_debug_main);
	printf("msoftptz init success\n");
	return 0;
}

int msoftptz_deinit()
{
	// jv_ptz_deinit();
	if(g_bSoftptzRunning)
	{
		g_bSoftptzRunning = FALSE;

		pthread_mutex_lock(&sPTZInfo.mutex);
		sPTZInfo.cmdIndex++;
		sPTZInfo.cmd = PTZ_CMD_IDLE;
		pthread_mutex_unlock(&sPTZInfo.mutex);

		pthread_join(sPTZInfo.thread, NULL);
		pthread_mutex_destroy(&sPTZInfo.mutex);
	}
	jv_ptz_deinit();
	return 0;
}

/**
 *@brief 本接口实现的云台控制，是否支持
 */
BOOL msoftptz_b_support(int channel)
{
	return sPTZInfo.bValid;
}

/**
 *@brief 本接口实现的云台控制自动旋转，是否支持自动
 */

BOOL msoftptz_auto_support(int channel)
{
	return FALSE;
}

/**
 *@brief 是否需要自检
 */
BOOL msoftptz_b_need_selfcheck()
{
	return hwinfo.bSupportPatrol;
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
int msoftptz_move_start(int channel, int left, int up, int zoomin)
{
	printf("msoftptz_move_start\n");
	ptzSpeed_pre = abs(msoftptz_speed_get(channel));
	
	pthread_mutex_lock(&sPTZInfo.mutex);
	sPTZInfo.cmdIndex++;
	sPTZInfo.cmd = PTZ_CMD_MOVE;
	sPTZInfo.moveSpeed.left = left;
	sPTZInfo.moveSpeed.up = up;
	sPTZInfo.moveSpeed.zoomin = zoomin;
	sPTZInfo.lastcmd=PTZ_CMD_MOVE;
	pthread_mutex_unlock(&sPTZInfo.mutex);
	return 0;
}

/**
 *@brief 自动巡航
 */
int msoftptz_move_auto(int channel, int speed)
{
	pthread_mutex_lock(&sPTZInfo.mutex);
	sPTZInfo.cmdIndex++;
	sPTZInfo.cmd = PTZ_CMD_LOOP;
	sPTZInfo.speed = speed;
	sPTZInfo.lastcmd=PTZ_CMD_LOOP;
	pthread_mutex_unlock(&sPTZInfo.mutex);

	return 0;
}

/**
 *@brief 停止移动（各种移动）
 *
 *@param channel 通道号
 */
int msoftptz_move_stop(int channel)
{
	pthread_mutex_lock(&sPTZInfo.mutex);
	sPTZInfo.cmdIndex++;
	sPTZInfo.cmd = PTZ_CMD_STOP;
	pthread_mutex_unlock(&sPTZInfo.mutex);

	if(ptzSpeed_pre != abs(msoftptz_speed_get(channel)))
	{
		WriteConfigInfo();
	}
	
	return 0;
}

/**
 *@brief 调用具体某个坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
int msoftptz_goto(int channel, JVPTZ_Pos_t *position)
{
	int speed = abs(msoftptz_speed_get(0));
	printf("goto(%d,%d), speed=%d\n",position->left,position->up,speed);
	pthread_mutex_lock(&sPTZInfo.mutex);
	sPTZInfo.cmdIndex++;
	sPTZInfo.cmd = PTZ_CMD_PRESET_LOCATE;
	sPTZInfo.presetPos = *position;
	sPTZInfo.speed=bAlarmPresetSpeed_Flag ? 255 : speed;
	sPTZInfo.lastcmd=PTZ_CMD_PRESET_LOCATE;
	pthread_mutex_unlock(&sPTZInfo.mutex);
	bAlarmPresetSpeed_Flag = FALSE;
	return 0;
}

int msoftptz_setalarmPreSpeed_flag()
{
	bAlarmPresetSpeed_Flag = TRUE;
	return 0;
}

/**
 *@brief 获取当前坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
int msoftptz_pos_get(int channel, JVPTZ_Pos_t *position)
{
	pthread_mutex_lock(&sPTZInfo.mutex);
	jv_ptz_pos_get(channel, position);
	pthread_mutex_unlock(&sPTZInfo.mutex);
	printf("position: %d, %d\n", position->left, position->up);

	return 0;
}

/**
 *@brief 获取上次设置过的速度
 		手动调整和巡航都可能改变过速度，
 		本函数返回离本次调用最近一次设置过的速度
*@return 速度
 */
int msoftptz_speed_get(int channel)
{
	int speed=0;
	pthread_mutex_lock(&sPTZInfo.mutex);
	if(PTZ_CMD_MOVE==sPTZInfo.lastcmd)
	{
		if(0==sPTZInfo.moveSpeed.left)
		{
			speed=sPTZInfo.moveSpeed.up;
		}
		else
		{	
			speed=sPTZInfo.moveSpeed.left;
		}
	}
	else if(PTZ_CMD_LOOP==sPTZInfo.lastcmd)
	{
		speed=sPTZInfo.speed;
	}
	else if(PTZ_CMD_PRESET_LOCATE==sPTZInfo.lastcmd)
	{
		speed=sPTZInfo.speed;
	}
	else
	{
		// 工厂测试时，直接使用工厂指定的速度
		if (gp.bFactoryFlag)
		{
			speed = gp.TestCfg.nYTSpeed;
		}
		else
		{
			//没有设置过巡航速度或者手动移动的速度
			if (!strcmp(hwinfo.devName, "H411AJL") ||
				!strcmp(hwinfo.devName, "H51X") ||
				!strcmp(hwinfo.devName,"H510-EN") ||
				!strcmp(hwinfo.devName, "A14-PC7000-MT1") ||
				!strcmp(hwinfo.devName,"HA520D-H1") ||
				!strcmp(hwinfo.devName, "HC520D-H1") ||
				!strcmp(hwinfo.devName,"H411-H1") ||
				!strcmp(hwinfo.devName, "HC421S-H1") || 
				!strcmp(hwinfo.devName, "JD-H40810") ||
				!strcmp(hwinfo.devName, "HC420-H2") ||
				!strcmp(hwinfo.devName, "HC300"))
				speed = 255;
			else if (!strcmp(hwinfo.devName, "H511"))
				speed = 100;
			else
				speed = 3;
		}
	}
	pthread_mutex_unlock(&sPTZInfo.mutex);
	return speed;
}

static void __msoftptz_pos_save(JVPTZ_Pos_t *curpos)
{
	FILE *fp;
	fp = fopen(FIRSTPOS_FILE, "wb");
	if (!fp)
	{
		printf("In softptz position save function,Failed open %s, because: %s\n", FIRSTPOS_FILE, strerror(errno));
		return ;
	}

	if (curpos)
	{
		fwrite(curpos, 1, sizeof(JVPTZ_Pos_t), fp);
	}

	fclose(fp);
}

static int __msoftptz_pos_read(JVPTZ_Pos_t *lastpos)
{
	FILE *fp;
	fp = fopen(FIRSTPOS_FILE, "rb");
	if (!fp)
	{
		printf("%s,Failed open %s, because: %s\n", __func__,FIRSTPOS_FILE, strerror(errno));
		return -1;
	}
	int len;
	len = fread(lastpos, 1, sizeof(JVPTZ_Pos_t), fp);
	printf("fread success\n");
	fclose(fp);
	if (len != sizeof(JVPTZ_Pos_t))
		return -1;
	return 0;
}


static BOOL _timer_pos_save(int id, void* param)
{
	JVPTZ_Pos_t *temppos=(JVPTZ_Pos_t *)param;
	__msoftptz_pos_save(temppos);
	printf("have pass 30s,saved the position(%d,%d)\n",temppos->left,temppos->up);
	return 0;
}

//获取sensor参数
// sensor  宽 ,高,焦距 ,水平垂直步距角,减速比,当前码流的分辨率
static int msoftptz_getSensorParam(double *SnW,double * SnH,double *SnF,double*StepAngleH,double * DriveRatioH, double*StepAngleV,double * DriveRatioV,int * maxLRStep,int * maxUDStep,int StreamWidth, int StreamHeight)
{
	if (!hwinfo.bSupport3DLocate)
	{
		return -1;
	}

	printf("%s, devname=%s, sensor: %d, encryptCode: %d\n",__func__, hwinfo.devName, hwinfo.sensor, hwinfo.encryptCode);

	switch (hwinfo.sensor)
	{
	case SENSOR_OV9732:
		if (ENCRYPT_100W == hwinfo.encryptCode)
		{
			* SnW			= 1280 * 3.0 / 1000;
			* SnH			= StreamHeight * 1280 * 3.0 / (1000 * StreamWidth);
			* SnF			= 3.6;
			* StepAngleH	= 4.425;
			* DriveRatioH	= 64;
			* StepAngleV	= 3.925;
			* DriveRatioV	= 64;
			* maxLRStep		= 0x0ffff;
			* maxUDStep		= 0x0ffff;
			return 0;
		}
		if (ENCRYPT_130W == hwinfo.encryptCode)			// 假130W
		{
			* SnW			= 1216 * 3.75 / 1000;
			* SnH			= StreamHeight * 1216 * 3.75 / (1000 * StreamWidth);
			* SnF			= 3.63;
			* StepAngleH	= 7.325;
			* DriveRatioH	= 64;
			* StepAngleV	= 8.625;
			* DriveRatioV	= 64;
			* maxLRStep		= 0x0ffff;
			* maxUDStep		= 0x0ffff;
			return 0;
		}
		break;
	case SENSOR_OV2710:
	case SENSOR_OV2735:
		if (HWTYPE_MATCH(HW_TYPE_C3)
			|| HWTYPE_MATCH(HW_TYPE_C3W)
			|| HWTYPE_MATCH(HW_TYPE_C5)
			|| HWTYPE_MATCH(HW_TYPE_V3))
		{
			* SnW			= 1920 * 3.0 / 1000;
			* SnH			= StreamHeight * 1920 * 3.0 / (1000 * StreamWidth);
			* SnF			= 3.63;
			* StepAngleH	= 5.625;
			* DriveRatioH	= 64;
			* StepAngleV	= 5.625;
			* DriveRatioV	= 64;
			* maxLRStep		= 0x0ffff;
			* maxUDStep		= 0x0ffff;
			return 0;
		}
		if (HWTYPE_MATCH(HW_TYPE_V6))
		{
			* SnW			= 1920 * 3.0 / 1000;
			* SnH			= StreamHeight * 1920 * 3.0 / (1000 * StreamWidth);
			* SnF			= 3.63;
			* StepAngleH	= 9;
			* DriveRatioH	= 50;
			* StepAngleV	= 5.625;
			* DriveRatioV	= 64;
			* maxLRStep		= 0x0ffff;
			* maxUDStep		= 0x0ffff;
			return 0;
		}
		break;
	default:
		break;
	}

	return -1;

}

//客户端圈定区域，进行3D定位，并放大缩小显示
//(x, y, w, h) 圈定区域中心坐标及宽高; (width, height)当前码流分辨率
//zoom  3D定位指令:	0xC0 放大，0xC1 缩小
int  msoftptz_ZoomZone(int x, int y, int w, int h, int width, int height,int zoom)
{
	printf( "%s: [3D];x=%d;y=%d;w=%d;h=%d;swidth=%d;sheight=%d;zoom=%d\n",__func__, x, y, w,h,width,height, zoom);
	int disx,disy;
	double anglex,angley;
	double SnW, SnH,SnF,StepAngleH,DriveRatioH,StepAngleV,DriveRatioV;
	JVPTZ_Pos_t  curpos,dstpos;
	int maxLRStep,maxUDStep;
	if(msoftptz_getSensorParam(&SnW,&SnH,&SnF,&StepAngleH,&DriveRatioH,&StepAngleV,&DriveRatioV, &maxLRStep,&maxUDStep,width, height)<0)
		return -1;
	printf(" SnW=%f,SnH=%f\n,",SnW,SnH);

	msoftptz_pos_get(0, &curpos);

	if(x>0)		//move right
	{
		anglex=atan((double)(x-0)*1000*SnW/(SnF*4094*1000));
		disx=anglex*180*1000*DriveRatioH/(1000*StepAngleH*M_PI);
	}
	else if(x<0)	//move left
	{
		anglex=atan((double)(0-x)*1000*SnW/(SnF*4094*1000));
		disx=-anglex*180*1000*DriveRatioH/(1000*StepAngleH*M_PI);
	}
	else
	{
		anglex=0;
		disx=0;
	}
	printf("anglex=%f,disx=%d,\n,",anglex,disx);
	disx=disx *0x10000 /maxLRStep;
	printf("adp disx=%d,\n",disx);

	dstpos.left=curpos.left+disx;

	if(y>0)		//move up
	{
		angley=atan((double)(y-0)*1000*SnH/(SnF*4094*1000));
		disy=-angley*180*1000*DriveRatioV/(1000*StepAngleV*M_PI)*cos(anglex);
	}
	else if(y<0)	//move down
	{
		angley=atan((double)(0-y)*1000*SnH/(SnF*4094*1000));
		disy=angley*180*1000*DriveRatioV/(1000*StepAngleV*M_PI)*cos(anglex);
	}
	else
	{
		angley=0;
		disy=0;
	}
	printf("angley=%f,disy=%d,\n,",angley,disy);
	disy=disy*0x10000/maxUDStep;
	printf("disy=%d,maxUDStep=%d\n,",disy,maxUDStep);
	
	dstpos.up=curpos.up+disy;
	printf("curpos.left=%d,curpos.up=%d,dstpos.left=%d,dstpos.up=%d\n",curpos.left,curpos.up,dstpos.left,dstpos.up);
	msoftptz_setalarmPreSpeed_flag();
	msoftptz_goto(0, &dstpos);

	return 0;
}

