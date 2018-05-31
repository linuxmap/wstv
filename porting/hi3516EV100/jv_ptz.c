
#include <jv_common.h>
#include "jv_motor.h"
#include "jv_ptz.h"
#include <math.h>

static struct{
	int fd; //电机控制句柄

	BOOL bAfterSelfCheck;

	//最大步数
	int maxLRStep;
	int maxUDStep;

}sPtzParam;


//设置速度
static void __set_speed(char left_or_up, int speed)
{
	unsigned int delay_us = 0;

	if (HWTYPE_MATCH(HW_TYPE_V6))
	{
		// 3: 8ms, 255: 3ms
		delay_us = 3000 + 5000 * (255 - speed) / 252;
	}
	else if (HWTYPE_MATCH(HW_TYPE_V3)
				|| HWTYPE_MATCH(HW_TYPE_C9))
	{
		// 3: 3.5ms, 255: 1.5ms
		delay_us = 1500 + 2000 * (255 - speed) / 252;
	}
	else
	{
		// 3: 10ms, 255: 4ms
		delay_us = 4000 + 6000 * (255 - speed) / 252;
	}

	int ret = ioctl(sPtzParam.fd, MOTOR_SET_DELAY, &delay_us);
	if (ret != 0)
		printf("Failed set speed: %s\n", strerror(errno));
}

static BOOL __check_done_base()
{
	int done;
	ioctl(sPtzParam.fd, MOTOR_GET_DONE, (unsigned long)&done);
	return done;
}

//检查是否已完成
static BOOL __check_done()
{
	int i = 0;
	char done = 0;
	/*最慢2分钟*/
	for (i = 0; i < 600; i++)
	{
		if (__check_done_base())
			return 1;

		usleep(200000);
	}
	printf("app motor getdone error\n");
	return 0;
}

//开机自检
static int src_self_check(int channel)
{
	if(!hwinfo.bSupportPatrol && !gp.bFactoryFlag)
	{
		return 0;
	}

	int i = 0;
	int nCheckTimes = 1;
	int direct;
	int step;

	if (gp.bFactoryFlag)
	{
		nCheckTimes = gp.TestCfg.nYTCheckTimes;
	}

	__set_speed(0, gp.bFactoryFlag ? gp.TestCfg.nYTSpeed : 255);
	__set_speed(1, gp.bFactoryFlag ? gp.TestCfg.nYTSpeed : 255);

	// 偶数向左下方转，奇数向右上方转
	for (i = 0; i < nCheckTimes; ++i)
	{
		if (i % 2 == 0)
		{
			/*向左下转*/
			step = sPtzParam.maxLRStep - 1;
			direct = MOTOR_DIRECT_LEFT | MOTOR_DIRECT_DOWN;
			ioctl(sPtzParam.fd, MOTOR_SET_LR_STEP, &step);
			ioctl(sPtzParam.fd, MOTOR_SET_UD_STEP, 0);
			ioctl(sPtzParam.fd, MOTOR_SET_DIRECT, &direct);
			if (ioctl(sPtzParam.fd, MOTOR_SET_START, 0) == 0)
			{
				__check_done();
			}
			else
				printf("ERROR: Failed start move down...\n");

			if (gp.TestCfg.nInterval)
			{
				sleep(gp.TestCfg.nInterval);
			}
		}
		else
		{
			/*向右上转*/
			direct = MOTOR_DIRECT_RIGHT | MOTOR_DIRECT_UP;
			ioctl(sPtzParam.fd, MOTOR_SET_DIRECT, &direct);
			if (ioctl(sPtzParam.fd, MOTOR_SET_START, 0) == 0)
			{
				__check_done();
			}
			else
				printf("ERROR: Failed start move up...\n");

			if (gp.TestCfg.nInterval)
			{
				sleep(gp.TestCfg.nInterval);
			}
		}
	}

	sPtzParam.bAfterSelfCheck = TRUE;

	return 0;
}

/**
 *@brief 本接口实现的云台控制，是否支持
 */
static BOOL src_get_capability(int channel, JVPTZ_Capability_t *capability)
{
	if (!capability)
		return JVERR_BADPARAM;
	if (sPtzParam.fd > 0)
	{
		capability->bValid = TRUE;
		capability->bLeftUpTogether = TRUE;
		return 0;
	}
	return FALSE;
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
static int src_move_start(int channel, int left, int up, int zoomin)
{
	int direct = MOTOR_DIRECT_NONE;
	int speed = 0;
	char left_or_up = 0;

	if (!sPtzParam.bAfterSelfCheck)
	{
		printf("device self checking...\n");
		return JVERR_DEVICE_BUSY;
	}

	//因为不能同时做左右和上下两方向移动，这里只好优先处理左右
	if (left != 0)
	{
		direct |= ((left > 0) ? MOTOR_DIRECT_LEFT : MOTOR_DIRECT_RIGHT);
		speed = ((left < 0) ? -left : left);
	}
	if (up != 0)
	{
		direct |= ((up > 0) ? MOTOR_DIRECT_UP : MOTOR_DIRECT_DOWN);
		speed = ((up < 0) ? -up : up);
	}

	if (0 == direct)
	{
		printf("not moved\n");
		return JVERR_FUNC_NOT_SUPPORT;
	}

	__set_speed(left_or_up, speed);
	int ret = ioctl(sPtzParam.fd, MOTOR_SET_DIRECT, &direct);
	ret |= ioctl(sPtzParam.fd, MOTOR_SET_START, &left_or_up);
	if (ret != 0)
		printf("ERROR: %s failed\n", __func__);

	return 0;
}

/**
 *@brief 停止移动（各种移动）
 *
 *@param channel 通道号
 */
static int src_move_stop(int channel)
{
	if (!sPtzParam.bAfterSelfCheck)
	{
		printf("device self checking...\n");
		return JVERR_DEVICE_BUSY;
	}
	ioctl(sPtzParam.fd, MOTOR_SET_STOP, 0);

	return 0;
}

/**
 *@brief 获取当前坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
static int src_pos_get(int channel, JVPTZ_Pos_t *position)
{
	if (!sPtzParam.bAfterSelfCheck)
	{
		printf("device self checking...\n");
		return JVERR_DEVICE_BUSY;
	}
	memset(position, 0, sizeof(JVPTZ_Pos_t));
	unsigned long temp;
	if (sPtzParam.maxUDStep)
		if (ioctl(sPtzParam.fd, MOTOR_GET_UD_STEP, (unsigned long)&temp) == 0)
		{
			position->up = temp * 0x10000 / sPtzParam.maxUDStep;
		}

	if (sPtzParam.maxLRStep)
		if (ioctl(sPtzParam.fd, MOTOR_GET_LR_STEP, (unsigned long)&temp) == 0)
		{
			position->left = temp * 0x10000 / sPtzParam.maxLRStep;
		}

//	printf("up: %d, left: %d\n", position->up, position->left);
	return 0;
}

/**
 *@brief 调用具体某个坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
static int src_move_to(int channel, JVPTZ_Pos_t *position,ENUM_LEFT_OR_UP direction,int speed)
{
	int hstep,vstep;
	JVPTZ_Pos_t curPos;
	int direct = MOTOR_DIRECT_NONE;

	__set_speed(0, speed);
	src_pos_get(channel, &curPos);
	//这里有点问题，上下和左右不能同时动作。所以，只好分先后
	//水平方向优先
	if (direction==Horizontal)
	{
		hstep = position->left - curPos.left;
		if(hstep==0)
			return 0;
		Printf("cur: %d, goto: %d\n", curPos.left, position->left);
		if (hstep > 0)
		{
			direct = MOTOR_DIRECT_RIGHT;
		}
		else if (hstep < 0)
		{
			direct = MOTOR_DIRECT_LEFT;
			hstep = -hstep;
		}
		//比例换算
		hstep = hstep * sPtzParam.maxLRStep / 0x10000;
		Printf("goto left: %d,sPtzParam.maxLRStep=%d\n", hstep,sPtzParam.maxLRStep);
		if (hstep)
		{
			ioctl(sPtzParam.fd, MOTOR_SET_DIRECT, &direct);
			ioctl(sPtzParam.fd, MOTOR_SET_LR_WORK, &hstep);
			ioctl(sPtzParam.fd, MOTOR_SET_START, 1);

		}
	}
	else if (direction==Vertical)
	{
		__set_speed(1, speed);
		//再上下方向
		vstep = position->up - curPos.up;
		if(vstep==0)
			return 0;
		Printf("up down.. cur: %d, goto: %d\n", curPos.up, position->up);
		if (vstep > 0)
		{
			direct = MOTOR_DIRECT_DOWN;
		}
		else if (vstep < 0)
		{
			direct = MOTOR_DIRECT_UP;
			vstep = -vstep;
		}
		//比例换算
		vstep = vstep * sPtzParam.maxUDStep / 0x10000;
		Printf("goto up: %d,sPtzParam.maxUDStep=%d\n", vstep, sPtzParam.maxUDStep);
		if (vstep)
		{
			ioctl(sPtzParam.fd, MOTOR_SET_DIRECT, &direct);
			ioctl(sPtzParam.fd, MOTOR_SET_UD_WORK, &vstep);
			ioctl(sPtzParam.fd, MOTOR_SET_START, 0);
		}
	}
	else if (direction==ALL)
	{
		hstep = position->left - curPos.left;
		vstep = position->up - curPos.up;

		if (hstep != 0)
		{
			direct |= ((hstep < 0) ? MOTOR_DIRECT_LEFT : MOTOR_DIRECT_RIGHT);
		}
		if (vstep != 0)
		{
			direct |= ((vstep < 0) ? MOTOR_DIRECT_UP : MOTOR_DIRECT_DOWN);
		}

		if (MOTOR_DIRECT_NONE == direct)
		{
			return 0;
		}

		//比例换算
		hstep = abs(hstep) * sPtzParam.maxLRStep / 0x10000;
		vstep = abs(vstep) * sPtzParam.maxUDStep / 0x10000;
		Printf("goto left: %d,sPtzParam.maxLRStep=%d ", hstep,sPtzParam.maxLRStep);
		Printf("up: %d,sPtzParam.maxUDStep=%d\n", vstep, sPtzParam.maxUDStep);
		printf("\n#######direct=%d,hstep=%d,vstep=%d\n",direct,hstep,vstep);
		ioctl(sPtzParam.fd, MOTOR_SET_DIRECT, &direct);
		ioctl(sPtzParam.fd, MOTOR_SET_LR_WORK, &hstep);
		ioctl(sPtzParam.fd, MOTOR_SET_UD_WORK, &vstep);
		ioctl(sPtzParam.fd, MOTOR_SET_START, 1);
	}

	return 0;
}


/**
 *@brief 调用具体某个坐标
 *@param position 期望调用的位置
 *
 *@return 0 if success
 */
static BOOL src_move_done(int channel)
{
	return __check_done_base();
}

static void __set_ptz_config(motor_conf_t* config)
{
	if (gp.bFactoryFlag 
		&& gp.TestCfg.nYTLRSteps != 0 
		&& gp.TestCfg.nYTUDSteps != 0)			// 工厂检测使用配置文件参数
	{
		sPtzParam.maxLRStep = gp.TestCfg.nYTLRSteps; 
		sPtzParam.maxUDStep = gp.TestCfg.nYTUDSteps;
	}
	else if (hwinfo.bSupportPatrol)  			// 支持巡航时需要匹配步数
	{
		if (HWTYPE_MATCH(HW_TYPE_V6))
		{
			sPtzParam.maxLRStep = 1860; 
			sPtzParam.maxUDStep = 1200;
		}
		else if (HWTYPE_MATCH(HW_TYPE_V3))
		{
			sPtzParam.maxLRStep = 4200;
			sPtzParam.maxUDStep = 1200;
		}
		else
		{
			sPtzParam.maxLRStep = 3800;
			sPtzParam.maxUDStep = 960;
		}
	}
    else
	{
		sPtzParam.maxLRStep = 0xffff;
		sPtzParam.maxUDStep = 0xffff;
	}

	if (HWTYPE_MATCH(HW_TYPE_V6))
	{
		motor_conf_t config_v6[MOTOR_ID_CNT] = {
			[MOTOR_ID_LR] = {
				.drv_pins = {
					[MOTOR_DRV_A] = {0x200F00E8, 1, 7, 2, 1},
					[MOTOR_DRV_B] = {0x200F00F4, 1, 7, 5, 1},
					[MOTOR_DRV_C] = {0x200F00EC, 1, 7, 3, 1},
					[MOTOR_DRV_D] = {0x200F00F0, 1, 7, 4, 1},
				},
				.maxstep = sPtzParam.maxLRStep,
				.metercnt = 8,
				.reverse = 1,
			},
			[MOTOR_ID_UD] = {
				.drv_pins = {
					[MOTOR_DRV_A] = {0x200F00CC, 0, 6, 3, 1},
					[MOTOR_DRV_B] = {0x200F00D0, 0, 6, 4, 1},
					[MOTOR_DRV_C] = {0x200F00D4, 0, 6, 5, 1},
					[MOTOR_DRV_D] = {0x200F00D8, 0, 6, 6, 1},
				},
				.maxstep = sPtzParam.maxUDStep,
				.metercnt = 8,
				.reverse = 1,
			},
		};
		memcpy(config, config_v6, sizeof(config_v6));
	}
	else if (HWTYPE_MATCH(HW_TYPE_V3))
	{
		motor_conf_t config_v3[MOTOR_ID_CNT] = {
			[MOTOR_ID_LR] = {
				.drv_pins = {
					[MOTOR_DRV_CLK] = {0x200F00CC, 0, 6, 3, 1},
					[MOTOR_DRV_DR] = {0x200F00D0, 0, 6, 4, 1},
					[MOTOR_DRV_EN] = {0x200F00E8, 1, 7, 2, 1},
				},
				.maxstep = sPtzParam.maxLRStep,
				.metercnt = 2,
				.reverse = 1,
			},
			[MOTOR_ID_UD] = {
				.drv_pins = {
					[MOTOR_DRV_CLK] = {0x200F00C8, 0, 6, 2, 1},
					[MOTOR_DRV_DR] = {0x200F00C0, 0, 6, 0, 1},
					[MOTOR_DRV_EN] = {0x200F00E8, 1, 7, 2, 1},
				},
				.maxstep = sPtzParam.maxUDStep,
				.metercnt = 2,
			},
		};
		memcpy(config, config_v3, sizeof(config_v3));
	}
	else if(HWTYPE_MATCH(HW_TYPE_C3W))
	{
		motor_conf_t config_com[MOTOR_ID_CNT] = {
			[MOTOR_ID_LR] = {
				.drv_pins = {
					[MOTOR_DRV_A] = {0x120400F4, 0, 0, 0, 1},
					[MOTOR_DRV_B] = {0x120400F8, 0, 0, 1, 1},
					[MOTOR_DRV_C] = {0x0, 0, 0, 2, 1},
					[MOTOR_DRV_D] = {0x1204001C, 0, 6, 0, 1},
				},
				.maxstep = sPtzParam.maxLRStep,
				.metercnt = 8,
			},
			[MOTOR_ID_UD] = {
				.drv_pins = {
					[MOTOR_DRV_A] = {0x12040094, 0, 3, 0, 1},
					[MOTOR_DRV_B] = {0x1204008C, 0, 3, 2, 1},
					[MOTOR_DRV_C] = {0x12040008, 0, 6, 6, 1},
					[MOTOR_DRV_D] = {0x1204000C, 0, 6, 7, 1},
				},
				.maxstep = sPtzParam.maxUDStep,
				.metercnt = 8,
				.reverse = 1,
			},
		};
		memcpy(config, config_com, sizeof(config_com));
	}
	else
	{
		motor_conf_t config_com[MOTOR_ID_CNT] = {
			[MOTOR_ID_LR] = {
				.drv_pins = {
					[MOTOR_DRV_A] = {0x200F00E8, 1, 7, 2, 1},
					[MOTOR_DRV_B] = {0x200F00EC, 1, 7, 3, 1},
					[MOTOR_DRV_C] = {0x200F00F0, 1, 7, 4, 1},
					[MOTOR_DRV_D] = {0x200F00F4, 1, 7, 5, 1},
				},
				.maxstep = sPtzParam.maxLRStep,
				.metercnt = 8,
			},
			[MOTOR_ID_UD] = {
				.drv_pins = {
					[MOTOR_DRV_A] = {0x200F00C0, 0, 6, 0, 1},
					[MOTOR_DRV_B] = {0x200F00C8, 0, 6, 2, 1},
					[MOTOR_DRV_C] = {0x200F00D0, 0, 6, 4, 1},
					[MOTOR_DRV_D] = {0x200F00CC, 0, 6, 3, 1},
				},
				.maxstep = sPtzParam.maxUDStep,
				.metercnt = 8,
				.reverse = 1,
			},
		};
		memcpy(config, config_com, sizeof(config_com));
	}
}

static int src_get_origin_pos(int channel, JVPTZ_Pos_t* pos)
{
	if (!pos)
		return -1;

	// 产测时，读取产测配置参数
	if (gp.bFactoryFlag
		&& gp.TestCfg.nYTLREndStep != 0
		&& gp.TestCfg.nYTUDEndStep != 0)
	{
		pos->left = gp.TestCfg.nYTLREndStep * 0x10000 / sPtzParam.maxLRStep;
		pos->up = gp.TestCfg.nYTUDEndStep * 0x10000 / sPtzParam.maxUDStep;
	}
	else if (hwinfo.bSupportPatrol)
	{
		// 支持巡航时，按照机型适配，默认在上下左右的中间位置
		pos->left = 0x8000;
		pos->up = 0x8000;
	}
	else
	{
		pos->left = 0x8000;
		pos->up = 0x8000;
	}

	return 0;
}

jv_ptz_func_t g_ptzfunc ;

/**
 *@brief 初始化
 */
int jv_ptz_init()
{
	int step = 0x8000;
	motor_conf_t config[MOTOR_ID_CNT];

	memset(&g_ptzfunc, 0, sizeof(g_ptzfunc));
	memset(&sPtzParam, 0, sizeof(sPtzParam));

	if (hwinfo.bSoftPTZ == 0)
	{
		return -1;
	}
	
	utl_system("insmod /home/ipc_drv/extdrv/hi_motor.ko");
	sleep(1);

	sPtzParam.fd = open("/dev/motor", O_RDWR);
	if (sPtzParam.fd < 0)
	{
		printf("ERROR: Failed open motor driver: %s\n", strerror(errno));
		return -1;
	}

	__set_ptz_config(config);

	ioctl(sPtzParam.fd, MOTOR_SET_CONF, config);

	// 将电机定位到中间位置，确保能转到极限位置
	if(!gp.bFactoryFlag)
	{
		ioctl(sPtzParam.fd, MOTOR_SET_LR_STEP, &step);
		ioctl(sPtzParam.fd, MOTOR_SET_UD_STEP, &step);
		sPtzParam.bAfterSelfCheck = TRUE;
	}

	g_ptzfunc.fptr_jv_ptz_get_capability = src_get_capability;
	g_ptzfunc.fptr_jv_ptz_self_check = src_self_check;
	g_ptzfunc.fptr_jv_ptz_get_origin_pos = src_get_origin_pos;
	g_ptzfunc.fptr_jv_ptz_move_start = src_move_start;
	g_ptzfunc.fptr_jv_ptz_move_stop = src_move_stop;
	g_ptzfunc.fptr_jv_ptz_move_to = src_move_to;
	g_ptzfunc.fptr_jv_ptz_move_done = src_move_done;
	g_ptzfunc.fptr_jv_ptz_pos_get = src_pos_get;

	return 0;
}

/**
 *@brief 结束
 */
int jv_ptz_deinit()
{
	if (hwinfo.bSoftPTZ == 0)
	{
		return -1;
	}
	if (sPtzParam.fd > 0)
	{
		close(sPtzParam.fd);
	}
	return 0;
}

