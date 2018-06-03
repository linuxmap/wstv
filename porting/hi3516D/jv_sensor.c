#include <jv_common.h>
#include <utl_timer.h>
#include "hicommon.h"
#include <jv_sensor.h>
#include"3518_isp.h"
#include <mpi_ae.h>
#include <jv_gpio.h>
#include <mpi_af.h>
#define DEV_SENSOR			"/dev/sensor"
#define ADC_READ	1

#define DEV_PWM			"/dev/pwm"
static int fdPwm = -1;

typedef struct hiPWM_DATA_S
{
	unsigned char pwm_num;  //0:PWM0,1:PWM1,2:PWM2,3:PWMII0,4:PWMII1,5:PWMII2
	unsigned int  duty;
	unsigned int  period;
	unsigned char enable;
} PWM_DATA_S;

#define PWM_CMD_WRITE      0x01
#define PWM_CMD_READ       0x03


#define FULL_FRAMERATE_RATIO  16
#define LIGHT_AE_NODE_MAX 11

static BOOL bAeNight = FALSE;
static int light_value = 0;

static HI_U8 CheckWaitTimes=0;

static BOOL StarLightEnable = FALSE;
static JV_SENSOR_STATE SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
jv_msensor_callback_t sensor_callback = NULL;
static BOOL bLowLightState =FALSE;

static JV_EXP_MODE_e jv_ae_strategy_mode[2] = {JV_EXP_AUTO_MODE,JV_EXP_AUTO_MODE};



HI_S32 HI_MPI_VPSS_SetNRV3Param(VPSS_GRP VpssGrp, VPSS_GRP_VPPNRBEX_S *pstVpssNrParam);
HI_S32 HI_MPI_VPSS_GetNRV3Param(VPSS_GRP VpssGrp, VPSS_GRP_VPPNRBEX_S *pstVpssNrParam);

int jv_sensor_switch_starlight(BOOL bOn);



static light_v light_v_list[] =
{
	{ 0,  240 },
	{ 1,  230 },
	{ 2,  220 },
	{ 3,  210 },
	{ 4,  200 },
	{ 5,  190 },
	{ 6,  185 },
	{ 7,  178 },
	{ 8,  167 },
	{ 9,  155 },
	{ 10, 150 },
	{ 11, 140 },//11 12 13 14没有实际意义
	{ 12, 130 },
	{ 13, 120 },
	{ 14, 110 }
};

static int fdSensor = -1;
static int speed = 60;


static BOOL bWDRStatus =FALSE;
static BOOL bWDRSwitchBusy=FALSE;
static BOOL bThreadFreeze= FALSE;


static int isp_helper_live=0;
static BOOL bNightMode = FALSE;
static pthread_mutex_t low_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t isp_wdr_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t isp_gamma_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int sensorType;




static BOOL  bAdcNight= FALSE;

const HI_U8 AE_WEIGHT_CENTER_NIGHT[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,4,4,4,4,4,4,4,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,4,6,6,6,6,6,6,6,4,4,1,1,1},
		{1,1,1,1,4,4,4,4,4,4,4,4,4,1,1,1,1},
		{1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

};
const HI_U8 AE_WEIGHT_CENTER_DAY[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
		{1,1,1,1,1,4,4,4,4,4,4,4,1,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,4,6,6,6,6,6,6,6,4,4,1,1,1},
		{1,1,1,1,4,4,4,4,4,4,4,4,4,1,1,1,1},
		{1,1,1,1,4,4,4,4,4,4,4,4,4,1,1,1,1},
		{1,1,1,1,3,3,3,3,3,3,3,3,3,1,1,1,1},


};
const HI_U8 AE_WEIGHT_CENTER_DEFAULT[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
		{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
		{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
		{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

};


static  light_ae_node light_ae_list_imx123[LIGHT_AE_NODE_MAX]=
{
	{ 3954*420*64, 8,TRUE},   //灵敏度为0,最晚切换到夜视

	{ 3954*360*64, 15,TRUE},  //灵敏度1

	{ 3954*320*64, 25,TRUE},  //灵敏度2

	{ 3954*320*64, 30,TRUE},   //灵敏度3

	{ 3954*320*64, 40,TRUE},  //灵敏度4

	{ 3954*300*64, 44,TRUE},  //灵敏度5

	{ 3954*300*64, 46,TRUE}, //灵敏度6

	{ 1977*300*64, 48,FALSE}, //灵敏度7

	{ 1977*300*64, 48,FALSE}, //灵敏度8
	{ 1977*280*64, 48,FALSE}, //灵敏度9
	{ 1977*220*64, 48,FALSE} //灵敏度10

};





BOOL __check_cp(void);
static void isp_helper_init(void);
void jv_common_wdr_switch(BOOL bEnable);



BOOL __check_wdr_state()//检测是否处于WDR模式下
{
	return bWDRStatus;
}


int _gpio_cutcheck_redlight_need_change()
{
	if(!strstr(hwinfo.devName, "N52-HS")&&(sensorType == SENSOR_IMX290||sensorType ==SENSOR_IMX123))
		return 1;
	return 0;
}
int _gpio_cutcheck_redlight_change()
{
	//38板+290 0_3用来red_light。其他的（目前是已经发布的4689仍然用来cut_check）；
	if(_gpio_cutcheck_redlight_need_change())
	{
		if (higpios.redlight.group == -1 && higpios.redlight.bit == -1)
		{
			higpios.redlight.group = higpios.cutcheck.group;
			higpios.redlight.bit = higpios.cutcheck.bit;

			higpios.cutcheck.group = -1;
			higpios.cutcheck.bit = -1;

			int dir = 0;
			dir = jv_gpio_dir_get(higpios.redlight.group);
			dir |= 1 << higpios.redlight.bit;
			jv_gpio_dir_set(higpios.redlight.group, dir);
		}
	}
	return 0;
}

//static int LowFrameRate =25;
static int ir_cut_staus=0;
static BOOL Red_Light_Status =FALSE;

#define PWM_TEST 0
#if PWM_TEST
int jv_pwm_set_redlight(int value);
static void pwm_test(void *param)
{
	sleep(10);
	printf("=================================================================start PWM test\n");
	int i = 0;
	static int f = 0;
	while (1)
	{
		usleep(100*1000);
		jv_pwm_set_redlight(i);
		printf("I am a pwm duty, my num is : %d\n", i);
		if(!f)
			i ++;
		if(f)
			i --;
		if(i==99)
			f = 1;
		if(i==1)
			f = 0;
	}
}
#endif

int jv_pwm_init()
{
	fdPwm = open(DEV_PWM, O_RDWR, 0);
	if (fdPwm <= 0)
	{
		printf("open PWM error...\n");
		return -1;
	}

	jv_gpio_muxctrl(0x200F00DC, 0x1);	//0_2 复用为PWM5
	/*
	 * PERI_CRG14 0x20030038: 0bit 0撤消复位 1复位；1bit 0关闭时钟 1打开时钟；[PERI_CRG14 2bit,PERI_CRG65 10bit] 00 3MHz, 01 50MHz, 1x 24MHz
	 * PERI_CRG65 0x20030104
	 * 如下配置为3MHz时钟频率
	 */
	jv_gpio_muxctrl(0x20030104, 0x0);
	jv_gpio_muxctrl(0x20030038, 0x2);

#if PWM_TEST
	pthread_t thread_tid;
	pthread_create(&thread_tid, NULL, (void *)pwm_test, NULL);
#endif

	return 0;
}

/*
 * value 占空比*100；[1,100]
 * 注意：由于电流变化曲线，30-40为最明显变化区间
 */
int jv_pwm_set_redlight(int value)
{
	PWM_DATA_S data;
	data.enable = 1;
	data.pwm_num = 5;
	data.period = 1000;	//根据INIT配置，3M/1000=3KHz	,这里配置为3KHz
	data.duty = data.period*value/100;
	ioctl(fdPwm,PWM_CMD_WRITE,(unsigned long)&data);
	return 0;
}

void jv_sensor_set_ircut(BOOL bNight)
{
#if GPIO
	if (bNight)
	{
		//开灯
		if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 1);

		jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
		jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 1);
		usleep(hwinfo.ir_power_holdtime);
		jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
		ir_cut_staus = 1;
		Red_Light_Status =TRUE;
	}
	else
	{
		//关灯
		if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 0);

		jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
		jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 1);
		usleep(hwinfo.ir_power_holdtime);
		jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
		ir_cut_staus = 0;
		Red_Light_Status =FALSE;
	}
#else
	if (bNight)
	{
		ir_cut_staus = 1;
		ioctl(fdSensor, 0x80040009, 0);
		Red_Light_Status =TRUE;
	}
	else
	{
		ir_cut_staus = 0;
		ioctl(fdSensor, 0x8004000A, 0);
		Red_Light_Status =FALSE;
	}
#endif
	//jv_cam_autofoucs_ircut(bNight);					//通知算法
}


int  jv_sensor_get_redlight_staus()
 {
	 return  Red_Light_Status;
 }


 int fd_adc;
 void jv_adc_init(void)
{
	int dir = 0;
  	fd_adc = open("/dev/hi_adc", O_RDWR);  
	if (fd_adc == -1)
   	{
	   printf("open adv mudule error\n");
   	}
}
 int jv_adc_read(void)
 {
	 int value,value_s;
	 static int mode_adc=0;
	 int i=0;
	 value_s=0;
	 if (fd_adc == -1)
	 {
		printf("open adv mudule error\n");
		return 0;
	 }
	 for(i=0;i<20;i++)
	 {
	 ioctl(fd_adc,ADC_READ,(unsigned long)&value);
	 value_s+=value;
	 usleep(1);
	 }
	 value=value_s/20;
	 //printf("value:%d\n",value); 
	 if(value<light_v_list[light_value+mode_adc].voltage)
	 {
		 mode_adc=0;
		 return 0;
	 }
	 else
	 {
		 mode_adc=4;
		 return 1;
	 }
 
 }

int  jv_sensor_get_ircut_staus()
{
	return ir_cut_staus;
}
int sensor_ircut_init(void)
{
	int ret;
#ifndef GPIO
	//打开sensor
	fdSensor = open(DEV_SENSOR, O_RDWR, 0);
	if(fdSensor<=0)
	{
		Printf("Sensor error...\n");
		return -1;
	}
#endif

//	jv_pwm_init();

	_gpio_cutcheck_redlight_change();

	ret=jv_autofocus_get_IRCfg();
	if(ret<0)
	{
		//设置sensor白天模式,
		jv_sensor_set_ircut(FALSE);
		jv_sensor_nocolour(0, FALSE);
	}
	else
	{	//通过机芯里的IR配置文件防止重启之后
		if(ret>0)
		{	//根据AF库里的配置文件初始化成Night模式
			jv_sensor_set_ircut(TRUE);
			jv_sensor_nocolour(0, TRUE);
		}
		else
		{	//根据AF库里的配置文件初始化成Day模式
			jv_sensor_set_ircut(FALSE);
			jv_sensor_nocolour(0, FALSE);
		}

	}
	if (strstr(hwinfo.devName, "N52-HS"))//通过adc读取光敏值
	{
		jv_adc_init();
	}

	isp_helper_init();
	return 0;
}

int sensor_ircut_deinit(void)
{
#ifndef GPIO
	if(fdSensor > 0)
	{
		close(fdSensor);
	}
#endif
	return 0;
}

int jv_sensor_daynight_ae_set(BOOL bNight)
{

	if(__check_wdr_state())
		return 0;
	if(1)
	{
	
		ISP_EXPOSURE_ATTR_S pstExpAttr;
	
		if(bNight)
		{
			if(jv_ae_strategy_mode[1] == JV_EXP_AUTO_MODE)//自动曝光
			{
				HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
				memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_NIGHT,255*1);
				pstExpAttr.stAuto.u16HistRatioSlope=128;
				pstExpAttr.stAuto.u8MaxHistOffset=8;
				pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
				HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
			}
			else if(jv_ae_strategy_mode[1] == JV_EXP_HIGH_LIGHT_MODE)
			{
				HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);//高光优先
				memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
				pstExpAttr.stAuto.u16HistRatioSlope=128;
				pstExpAttr.stAuto.u8MaxHistOffset=24;
				pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
				HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
			}
			else if(jv_ae_strategy_mode[1] == JV_EXP_LOW_LIGHT_MODE)//低光优先
			{
				HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
				memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
				pstExpAttr.stAuto.u16HistRatioSlope=128;
				pstExpAttr.stAuto.u8MaxHistOffset=16;
				pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
				HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
			}
		}
		else
		{
			if(jv_ae_strategy_mode[0] == JV_EXP_AUTO_MODE)
			{
				HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
				memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DAY,255*1);
				pstExpAttr.stAuto.u16HistRatioSlope=0;
				pstExpAttr.stAuto.u8MaxHistOffset=0;
				pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
				HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);		
			}
			else if(jv_ae_strategy_mode[0] == JV_EXP_HIGH_LIGHT_MODE)
			{
				HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
				memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
				pstExpAttr.stAuto.u16HistRatioSlope=128;
				pstExpAttr.stAuto.u8MaxHistOffset=24;
				pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
				HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
			}
			else if(jv_ae_strategy_mode[0] == JV_EXP_LOW_LIGHT_MODE)
			{
				HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
				memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
				pstExpAttr.stAuto.u16HistRatioSlope=128;
				pstExpAttr.stAuto.u8MaxHistOffset=16;
				pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
				HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
			}
		}
	}
	return 0;

}

int jv_sensor_set_exp_mode(int sensorid,JV_EXP_MODE_e mode)
{
	if(bNightMode)
		jv_ae_strategy_mode[1]=mode;
	else
		jv_ae_strategy_mode[0]=mode;

	jv_sensor_daynight_ae_set(bNightMode);

	return 0;
}


static BOOL bCheckNight =FALSE;

int jv_sensor_set_nightmode(BOOL bNight)
{

	pthread_mutex_lock(&isp_gamma_mutex);
	if(bNight)
	{
		Printf("Trun to NightMode...\n");
		bNightMode =TRUE;
        isp_ioctl(0,ADJ_SCENE,CHECK_NIGHT);
		
	}
	else
	{
		Printf("Trun to DayMode...\n");
		bNightMode =FALSE;
        isp_ioctl(0,ADJ_SCENE,CHECK_DAY);
		
	}
	pthread_mutex_unlock(&isp_gamma_mutex);
	
	jv_sensor_daynight_ae_set(bNightMode);

	if(sensorType == SENSOR_IMX123||sensorType==SENSOR_IMX290)
	{
		ISP_EXPOSURE_ATTR_S pstExpAttr;
		HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
		pstExpAttr.stAuto.u8Speed = 86;
		HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
	}
	CheckWaitTimes =0;
	bCheckNight = TRUE;
	return 0;
}

static BOOL bFirstCheck =TRUE;

int jv_sensor_Adc_CheckNight_v2(void)//采用该接口，不接灯板默认为彩色
{

	BOOL bNight=FALSE;
	int mode;

	mode = jv_adc_read();
	
	if(!mode) 
		bFirstCheck =FALSE;  //一开始是白天，肯定接灯板了
	
	if(bFirstCheck)//adc黑夜，或未接灯板，或真是黑夜
	{
		if(bAdcNight) //ae判断为黑夜则进入黑夜模式，并清first flag
		{
			bNight =TRUE;
			bFirstCheck =FALSE;
		}
		else 
			bNight= FALSE;
 
	}
	else
	{
		bNight= (mode>0) ? TRUE:FALSE;
	}
	return bNight;




}


/**
 *@brief 暂时替代jv_sensor_b_nightmode。代表了灯板上的夜间模式
 */
BOOL jv_sensor_b_night(void)
{
   int mode=1;


   if(StarLightEnable&&(sensorType == SENSOR_IMX290||sensorType == SENSOR_IMX123))
   		return bAeNight;
   if((!strstr(hwinfo.devName, "N52-HS"))&&(sensorType == SENSOR_IMX290||sensorType == SENSOR_IMX123))
   		return bAeNight;
   
   if(StarLightEnable && sensorType==SENSOR_OV2710)
	   return bAeNight;
   
   if (strstr(hwinfo.devName, "N52-HS"))//通过adc读取光敏值
	{
		if(sensorType == SENSOR_OV4689||sensorType == SENSOR_AR0230||sensorType == SENSOR_AR0237 \
			||sensorType == SENSOR_OV2710||sensorType == SENSOR_IMX290||sensorType == SENSOR_IMX123||sensorType == SENSOR_AR0237DC)
		{
			return jv_sensor_Adc_CheckNight_v2();
		}
		else
		{
		  mode = jv_adc_read();
		  return mode;
		}
		  
	}

	switch(hwinfo.ir_sw_mode)
	{
		case IRCUT_SW_BY_GPIO:
		{
			#if GPIO
				if(higpios.cutcheck.group != -1&&higpios.cutcheck.bit != -1)
					mode = jv_gpio_read(higpios.cutcheck.group, higpios.cutcheck.bit);
				else
					printf("cut_check gpio error; maybe 0_3 used to red_light!\n");
			#else
				ioctl(fdSensor, 0x8004000B, &mode);
			#endif
				if (mode == 0)
					return TRUE;
				return FALSE;
		}
		case IRCUT_SW_BY_AE:
		{
			return jv_sensor_b_night_use_ae();
			break;
		}
		case IRCUT_SW_BY_UART:
		{
			return ir_cut_staus;
			break;
		}
		default:
		{
			return ir_cut_staus;
			break;
		}
	}
	
}
/**
 *@brief AE数字感应代替光敏电阻器light sensitive resistor
 *@brief 和jv_sensor_b_night（）功能一样，判断是否夜模式
 *@retval 0： day 1:night
 *FUNC 需要自己实现
 */
BOOL jv_sensor_b_night_use_ae(void)
{
#if 0
	ISP_DEV IspDev = 0; 
	ISP_INNER_STATE_INFO_S pstInnerStateInfo;
	HI_MPI_ISP_QueryInnerStateInfo(IspDev, &pstInnerStateInfo);
//	return 0;	//需要调试完毕再定下面的参数
//	printf("AE statics\n");
//	printf("u32ExposureTime=%8x\n",pstInnerStateInfo.u32ExposureTime);
//	printf("u32AnalogGain=%8x\n",pstInnerStateInfo.u32AnalogGain);
//	printf("u32DigitalGain=%8x\n",pstInnerStateInfo.u32DigitalGain);
//	printf("u8AveLum=%8x\n",pstInnerStateInfo.u8AveLum);
//	printf("u32Exposure=%8x\n",pstInnerStateInfo.u32Exposure);

    //printf("%s  ExposTime:%d, DGain:%d, ir_cut:%d    798 ",__FUNCTION__, pstInnerStateInfo.u32ExposureTime,
    //    pstInnerStateInfo.u32DigitalGain, ir_cut_staus);
	if((pstInnerStateInfo.u32ExposureTime>0x31E) && (pstInnerStateInfo.u32DigitalGain > 3)&&(!ir_cut_staus))
	{
		return 1;
	}
	else if((pstInnerStateInfo.u32ExposureTime < 0x31E) && (pstInnerStateInfo.u32AnalogGain < 7)&& ir_cut_staus)
	{
		return 0;
	}
	else
	{
		return ir_cut_staus;
	}
#endif

	return 0;
}

/**
 *@brief set the iris/aperture  模拟调节光圈大小
 *@param sensorid
 *@param size up or down
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_iris_adjust(int sensorid, BOOL SIZEUP)
{
    static int nValue = 0;
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
    if(SIZEUP)
    {
		nValue = stExpAttr.stAuto.u8Compensation + 8;
        if(nValue > 255)
            stExpAttr.stAuto.u8Compensation = 255;
        else
            stExpAttr.stAuto.u8Compensation = nValue;
    }
    else
    {
		nValue = stExpAttr.stAuto.u8Compensation - 8;
        if(nValue < 0)
            stExpAttr.stAuto.u8Compensation = 0;
        else
            stExpAttr.stAuto.u8Compensation = nValue;
    }

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	return 0;
}


/**
 *@brief set brightness
 *@param sensorid
 *@param brightness value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_brightness(int sensorid, int nValue)
{
	isp_ioctl(0,ADJ_BRIGHTNESS,nValue);
	return 0;
}
/**
 *@brief set antifog
 *@param sensorid
 *@param antifog value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_antifog(int sensorid, int nValue)
{
	ISP_DEFOG_ATTR_S  pstDefogAttr;
	HI_MPI_ISP_GetDeFogAttr(0,&pstDefogAttr);
	if(nValue==1)
		pstDefogAttr.bEnable=HI_TRUE;
	else if(nValue==0)
		pstDefogAttr.bEnable=HI_FALSE;
	HI_MPI_ISP_SetDeFogAttr(0,&pstDefogAttr);

	return 0;
}

/**
 *@brief set saturation
 *@param sensorid
 *@param saturation value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
static  unsigned int LightAeIndex  = 4; 
int jv_sensor_light(int sensorid, int nValue)
{
	if(nValue >= LIGHT_AE_NODE_MAX || nValue < 0)
		return 0;

	if(hwinfo.sensor == SENSOR_IMX123)
	{
		pthread_mutex_lock(&low_frame_mutex);
		light_value  = 4;

		if( nValue < LightAeIndex )
		{
			LightAeIndex = nValue;
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			bAeNight =FALSE;
			//bFirstCheck =FALSE;//采用星光级，则不接灯板也是彩色
			//StarLightEnable =TRUE;
			JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
		}
		LightAeIndex = nValue;
		pthread_mutex_unlock(&low_frame_mutex); 
		
	}
	else
	{
		light_value =nValue;
	}
	return 0;
}

/**
 *@brief set saturation
 *@param sensorid
 *@param saturation value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_saturation(int sensorid, int nValue)
{
    isp_ioctl(0,ADJ_SATURATION,nValue);
	return 0;
}


/**
 *@brief set the sensor the daynight mode
 *@param sensorid
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_switch_starlight(BOOL bOn)
{
	if(StarLightEnable==bOn)
		return 0;
	//if(hwinfo.sensor==SENSOR_OV2710)
	 if((sensorType == SENSOR_IMX290||sensorType == SENSOR_IMX123||sensorType == SENSOR_OV2710)&&strstr(hwinfo.devName, "N52-HS"))
	{
		pthread_mutex_lock(&low_frame_mutex);
   		if(bOn==FALSE)
   		{
   			printf("star light function  off\n");
			StarLightEnable =FALSE;
			if(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)
			{
				JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
				SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			}			
   		}
		else
		{
			printf("star light function  on\n");  //打开星光级，且重置状态
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			bAeNight =FALSE;
			bFirstCheck =FALSE;//采用星光级，则不接灯板也是彩色
			StarLightEnable =TRUE;
			JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);		
		}
		pthread_mutex_unlock(&low_frame_mutex); 
	}
	
	return 0;
		
}

static unsigned int DayNightMode = 0xff;
int jv_sensor_set_daynight_mode(int sensorid, int mode)
{
	//if(!StarLightSupported)
		//return 0;
	if(__check_cp())
		return 0;
	DayNightMode =mode;
	if(sensorType == SENSOR_IMX290||sensorType == SENSOR_IMX123) //imx290 禁灰，一直开启星光级 
	{
		if(mode==DAYNIGHT_AUTO||mode==DAYNIGHT_ALWAYS_DAY)
			jv_sensor_switch_starlight(TRUE);
		else
			jv_sensor_switch_starlight(FALSE);
		return 0;
	}
	
	if(sensor_callback)
	{
		if(sensor_callback(NULL) == FALSE)
	 		jv_sensor_switch_starlight(FALSE);
		else
		{
			if(mode==DAYNIGHT_AUTO||mode==DAYNIGHT_ALWAYS_DAY)
				jv_sensor_switch_starlight(TRUE);
			else
				jv_sensor_switch_starlight(FALSE);
		}
	}
	else
	{

		//if(DayNightMode == mode)
			//return 0; 
   		//DayNightMode =mode;
	
   		if(mode==DAYNIGHT_AUTO||mode==DAYNIGHT_ALWAYS_DAY)
			jv_sensor_switch_starlight(TRUE);
		else
			jv_sensor_switch_starlight(FALSE);
	}
	return 0;
}


/**
 *@brief set contrast
 *@param sensorid
 *@param contrast value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_contrast(int sensorid, int nValue)
{
	if(bLowLightState==FALSE)
	{
		isp_ioctl(0,ADJ_CONTRAST,nValue);
	}
	
	return 0;
}

/**
 *@brief 设置指定sensor的锐度
 *
 *@param sensorid sensor号，目前摄像机为单sensor，传入0即可
 *@param sharpness 锐度值
 *
 */
int jv_sensor_sharpness(int sensorid, int sharpness)
{
	Printf("Sharpness: %d\n", sharpness);
    isp_ioctl(0,ADJ_SHARPNESS,sharpness);

	return 0;
}

/**
 *@brief set mirror
 *@param sensorid
 *@param mirror value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_mirror_turn(int sensorid, BOOL bMirror, BOOL bTurn)
{
    //sleep(15);
    int nValue = 0;
	if (bMirror)
		nValue |= 1;
	if (bTurn)
		nValue |= (1<<1);
    isp_ioctl(0,MIRROR_TURN,nValue);
//    printf("jv_sensor_mirror:%d\n",nValue);
	return 0;
}

/**
 *@brief set awb
 *@param sensorid
 *@param awb value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_awb(int sensorid, int nValue)
{
//    ioctl(fdSensor, 0x80040020 , nValue);
//    printf("jv_sensor_awb\n");
	return 0;
}
/**
 *@brief set nocolour
 *@param sensorid
 *@param nocolour value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_nocolour(int sensorid, BOOL bNoColor)
{
    isp_ioctl(0,ADJ_COLOURMODE,bNoColor ? 2 : 1);
	return 0;
}
/**
 *@brief set chroma
 *@param sensorid
 *@param chroma value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_chroma(int sensorid, int nValue)
{
	return 0;
}

/**
 *@brief set wdr
 *@param bEnable Enable or not wide dynamic range
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_wdr(int sensorid, int bEnable)
{
	//Printf("sensor wdr: %d\n", bEnable);
    isp_ioctl(0,WDR_SW,bEnable);
	return 0;
}

int jv_sensor_wdr_mode_get();



/**
 *@brief设置sensor基准帧频
 *@param 基准帧频
 *@retval 0 if success
 *@retval <0 if failed.
 */
int jv_sensor_set_fps(int frameRate)
{
	return JV_ISP_COMM_Set_StdFps(frameRate);
}



/**
 *@brief 降帧接口
 *@param bEnable 是否降帧
 *@retval 0 if success
 *@retval <0 if failed.
 */
int jv_sensor_low_frame(int sensorid, int bEnable)
{

	pthread_mutex_lock(&low_frame_mutex);
	int tp;
	if (bEnable)
	{
		if(sensorType == SENSOR_IMX290)
			tp = 27; //14.8 fps
		else
			tp = 32; //默认降1/2
	}
	else
		tp = 16;
	
	JV_ISP_COMM_Set_LowFps(tp);  //fps=ViFps*16/tp;
	
	if(bNightMode  ==FALSE)
	{
		SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
		bAeNight =FALSE;
	}
	pthread_mutex_unlock(&low_frame_mutex); 


	return 0;

}



int jv_sensor_auto_lowframe_enable(int sensorid, int bEnable)
{

	return 0;
}


/**
 *@brief 获取降帧状态
 *
 *@param sensorid id
 *@param current OUT 降帧后的当前帧率
 *@param normal OUT 正常帧率
 *
 *@return 1 表示降帧  0 表示未降帧
 */
 BOOL jv_sensor_get_b_low_frame(int sensorid, float *current, float *normal)
{
	float fps;
	BOOL blow = FALSE;
	if (normal)
		*normal = 25.0;
	
	if (current)
	{
		fps = JV_ISP_COMM_Get_Fps();
		*current =(float)( (int)(fps+0.5));
	}
	
	blow = JV_ISP_COMM_Query_LowFps();
	return blow;

}


/**
 *@brief Get Reference Value for Focus
 *@param sensorid
 *@return Reference Value for Focus
 */
int jv_sensor_get_focus_value(int sensorid)
{
	return 0;
}

int jv_sensor_set_definition(int sensorid, Image_D level)
{

	return 0;

}


int jv_sensor_set_sence(int sensorid, Sence_e sence)
{
	int a;
	Printf("Sence: %d, fdSensor: %d\n", sence, sensorid);
	switch(sence)
	{
	default:
	case SENCE_INDOOR:
		a = 0x07;
		break;
	case SENCE_OUTDOOR:
		a = 0x08;
		break;
    case SENCE_DEFAULT:
        a=0x07;
        break;
    case SENCE_MODE1:
        a=0x08;
        break;        
	}
	isp_ioctl(0,0x80040026,a);// a=1 indoor ; a=2 outdoor;
//	jv_sensor_nocolour(sensorid, sBNoColor);

	return 0;
}
unsigned int __exposure_time2line(int fps, unsigned int linesCnt, unsigned int time)
{
	unsigned int line =1000000/25;
	if(time>0)
		 line = 1000000/time; //16a的曝光时间单位为微妙，故修改该接口
	if (line < 2)
		line = 2;
	return line;
}

unsigned int __exposure_line2time(int fps, unsigned int linesCnt, unsigned int line)
{
	unsigned int time =25;
	if(line>0)
		time = 1000000 / line;  //16a的曝光时间单位为微秒
	return time;
}

/**
 *@brief 设置自动曝光的参数
 */
int jv_sensor_ae_set(AutoExposure_t *ae)
{
	ISP_DEV IspDev = 0;
	ISP_PUB_ATTR_S stPubAttr;
	if(__check_cp()||__check_wdr_state())
		return 0;
	HI_MPI_ISP_GetPubAttr(IspDev, &stPubAttr);
	int rf = stPubAttr.f32FrameRate;
	if (ae->exposureMin < rf)
		ae->exposureMin = rf;

	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	if(ae->bAEME)
	{
		stExpAttr.stAuto.stExpTimeRange.u32Max = __exposure_time2line(rf, VI_HEIGHT, ae->exposureMax);
		stExpAttr.stAuto.stExpTimeRange.u32Min = __exposure_time2line(rf, VI_HEIGHT, ae->exposureMin);
		//printf("ae attr: [%d, %d]\n", attr.u16ExpTimeMin, attr.u16ExpTimeMax);
		CHECK_RET(HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr));
	}
	else
	{
	}
	return 0;
}

int jv_sensor_set_exp_params(int sensorid,int value)
{
	
	return 0;
}


/**
 *@brief 获取自动曝光的参数
 */
int jv_sensor_ae_get(AutoExposure_t *ae)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	if(stExpAttr.enOpType == OP_TYPE_AUTO)
		ae->bAEME = TRUE;
	else if(stExpAttr.enOpType == OP_TYPE_MANUAL)
		ae->bAEME = FALSE;
	ae->bLowFrameMode = FALSE;
	if(ae->bAEME)
	{
		ae->exposureMax = __exposure_line2time(25, VI_HEIGHT, stExpAttr.stAuto.stExpTimeRange.u32Max);
		ae->exposureMin = __exposure_line2time(25, VI_HEIGHT, stExpAttr.stAuto.stExpTimeRange.u32Min);
		ae->u16DGainMax = stExpAttr.stAuto.stDGainRange.u32Max;
		ae->u16DGainMin = stExpAttr.stAuto.stDGainRange.u32Min;
		ae->u16AGainMax = stExpAttr.stAuto.stAGainRange.u32Max;
		ae->u16AGainMin = stExpAttr.stAuto.stAGainRange.u32Max;
		ae->u32ISPDGainMax = stExpAttr.stAuto.stISPDGainRange.u32Max;
		ae->u32SystemGainMax = stExpAttr.stAuto.stSysGainRange.u32Max;
		ae->bByPassAE = stExpAttr.bByPass;
	}
	else
	{
		ae->bManualExpLineEnable = stExpAttr.stManual.enExpTimeOpType;
		ae->bManualDGainEnable = stExpAttr.stManual.enDGainOpType;
		ae->bManualExpLineEnable = stExpAttr.stManual.enAGainOpType;
		ae->bManualISPGainEnable = stExpAttr.stManual.enISPDGainOpType;
		ae->s32AGain = stExpAttr.stManual.u32AGain;
		ae->s32DGain = stExpAttr.stManual.u32DGain;
		ae->u32ISPDGain = stExpAttr.stManual.u32ISPDGain;
		ae->u32ExpLine = stExpAttr.stManual.u32ExpTime;
	}
	return 0;
}

/**
 *@brief 设置DRC的参数
 */
int jv_sensor_drc_set(DRC_t *drc)
{

	return 0;
}

/**
 *@brief 获取DRC的参数
 */
int jv_sensor_drc_get(DRC_t *drc)
{
	ISP_DEV IspDev = 0;
	ISP_DRC_ATTR_S pstDRCAttr;
	HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
	drc->bDRCEnable = pstDRCAttr.bEnable;
	return 0;
}



int jv_sensor_set_local_exposure(int sensorid,JV_SENSOR_LE_t *jv_sensor_le)
{
	EXPOSURE_DATA data;
	isp_exposure_get(&data);
	memcpy(&(data.u8Weight),&(jv_sensor_le->u8Weight),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
	isp_exposure_set(&data);
	return 0;
}
int jv_sensor_get_local_exposure(int sensorid,JV_SENSOR_LE_t *jv_sensor_le)
{
	EXPOSURE_DATA data;
	isp_exposure_get(&data);
	memcpy(&(jv_sensor_le->u8Weight),&(data.u8Weight),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
	return 0;
}

int jv_sensor_get_dis(int sensorid)
{
	return 0;
}
int jv_sensor_set_dis(int sensorid,BOOL bDisopen)
{
	return 0;
}


int jv_sensor_set_wdr_mode(BOOL bWDRMode)
{

	pthread_mutex_lock(&isp_wdr_mutex); 
	if(bWDRSwitchBusy)
	{
		pthread_mutex_unlock(&isp_wdr_mutex);
		return 0;
	}
	
	bWDRStatus = bWDRMode;
	isp_wdr_init(bWDRStatus);
	pthread_mutex_unlock(&isp_wdr_mutex);

	return 0;
}
int jv_sensor_get_wdr_mode()
{
	int mode;
	pthread_mutex_lock(&isp_wdr_mutex); 
	if(bWDRSwitchBusy)
	{
		pthread_mutex_unlock(&isp_wdr_mutex);
		return -1;

	}
	else
	{
		if(bWDRStatus)
			mode = 1;
		else
			mode = 0;
		pthread_mutex_unlock(&isp_wdr_mutex);		
		return mode;
	}
}

int jv_sensor_wdr_switch(BOOL bWDREn)
{
	
	pthread_mutex_lock(&isp_wdr_mutex);
	if(bWDREn==bWDRStatus)
	{
		
		printf("wdr mode has been set,not need to switch!!!!!!!\n");
		pthread_mutex_unlock(&isp_wdr_mutex);	
		return 0;
	}
	else 
	{
		if(bWDREn&&bNightMode)
		{
			printf("night mode !!! <<wdr mode aborted>>\n");
			pthread_mutex_unlock(&isp_wdr_mutex);			
			return 0;
		}
		
		bWDRSwitchBusy = TRUE;

		while(!bThreadFreeze)   //等待线程freeze
			usleep(200000);

		bLowLightState =FALSE;
		
		jv_common_wdr_switch(bWDREn);
	
		usleep(800000);	

		isp_wdr_init(bWDREn);
		bWDRStatus =bWDREn;
		bThreadFreeze = FALSE;
		bWDRSwitchBusy =FALSE;
		pthread_mutex_unlock(&isp_wdr_mutex);
		printf("wdr mode %d switch success!!!!!!\n",bWDRStatus);

		return  0;
	}
}

int jv_sensor_rotate_get(JVRotate_e *jvRotate)
{
	HI_S32 s32Ret;
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = 0;
	ROTATE_E curRotate;

	if(!jvRotate)
		return -1;

	s32Ret = HI_MPI_VPSS_GetRotate(VpssGrp, VpssChn, &curRotate);
	if(s32Ret != HI_SUCCESS)
	{
		Printf("HI_MPI_VPSS_GetRotate failed with %#x!\n", s32Ret);
		return -1;
	}

	*jvRotate = (JVRotate_e)curRotate;
	return 0;
}

int jv_sensor_rotate_set(JVRotate_e jvRotate)
{
	HI_S32 s32Ret;
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = 0;
	ROTATE_E newRotate = (ROTATE_E)jvRotate;
	int i;

	for(i=0; i<HWINFO_STREAM_CNT; i++)
	{
		VpssChn = i;
		s32Ret = HI_MPI_VPSS_SetRotate(VpssGrp, VpssChn, newRotate);
		if(s32Ret != HI_SUCCESS)
		{
			Printf("HI_MPI_VPSS_SetRotate failed with %#x!\n", s32Ret);
			return -1;
		}
	}

	return 0;
}

/*
static HI_U32 au32RouteNode[7][3]=		
{
	{56,  1024,   1},		 
	{900, 1024,   1},		
	{900, 3328,   1},		 
	{4500, 3328,   1},		  
	{4500, 5280, 1},  
	{24290, 5280,	1},
	{24290, 20480,	 1}
};*/

HI_U32 jv_IQ_AgcTableCalculate(const HI_U8 *au8Array, HI_U8 u8Index, HI_U32 u32ISO)
{    
	HI_U32 u32Data1, u32Data2;    
	HI_U32 u32Range, u32ISO1;    
	u32Data1 = au8Array[u8Index];    
	u32Data2 = (7 == u8Index)? au8Array[u8Index]: au8Array[u8Index + 1];       
	u32ISO1 = 100 << u8Index;    u32Range = u32ISO1;    
	if(u32Data1 > u32Data2)    
	{        
		return u32Data1 - ((u32Data1 - u32Data2) * (u32ISO - u32ISO1) + (u32Range >> 1)) / u32Range;    
	}  
	else    
	{   
		return u32Data1 + ((u32Data2 - u32Data1) * (u32ISO - u32ISO1) + (u32Range >> 1)) / u32Range;   
	}
}

HI_U32 jv_IQ_AgcTableCalculate_v2_u32(const HI_U32 *au32Array, HI_U8 u8Index, HI_U32 u32ISO)
{    
	HI_U32 u32Data1, u32Data2;    
	HI_U32 u32Range, u32ISO1;    
	u32Data1 = au32Array[u8Index];    
	u32Data2 = (15 == u8Index)? au32Array[u8Index]: au32Array[u8Index + 1];       
	u32ISO1 = 100 << u8Index;    u32Range = u32ISO1;    
	if(u32Data1 > u32Data2)    
	{        
		return u32Data1 - ((u32Data1 - u32Data2) * (u32ISO - u32ISO1) + (u32Range >> 1)) / u32Range;    
	}  
	else    
	{   
		return u32Data1 + ((u32Data2 - u32Data1) * (u32ISO - u32ISO1) + (u32Range >> 1)) / u32Range;   
	}
}

HI_U32 jv_IQ_AgcTableCalculate_v2_u8(const HI_U8 *au8Array, HI_U8 u8Index, HI_U32 u32ISO)
{    
	HI_U32 u32Data1, u32Data2;    
	HI_U32 u32Range, u32ISO1;    
	u32Data1 = au8Array[u8Index];    
	u32Data2 = (15 == u8Index)? au8Array[u8Index]: au8Array[u8Index + 1];       
	u32ISO1 = 100 << u8Index;    u32Range = u32ISO1;    
	if(u32Data1 > u32Data2)    
	{        
		return u32Data1 - ((u32Data1 - u32Data2) * (u32ISO - u32ISO1) + (u32Range >> 1)) / u32Range;    
	}  
	else    
	{   
		return u32Data1 + ((u32Data2 - u32Data1) * (u32ISO - u32ISO1) + (u32Range >> 1)) / u32Range;   
	}
}


static void *thread_isp_helper(void *param)
{  


    Printf("isp adjust thread\n");
	ISP_DEV IspDev = 0;
    HI_U32 u32ISO, u32ISOTmp;
	HI_U32 u32ISOLast=0;
    unsigned long sensor;
	HI_U8 u8Index;
	sleep(4); //等待ISP稳定
	ISP_EXP_INFO_S stExpInfo;
	HI_U8  OV2710_Luma_SF_STILL_Tbl[8]={32,32,32,32,48,62,64,64};//
	HI_U8  OV2710_Luma_SF_MOVE_Tbl[8]= {3,5,6,9,13,26,32,50};
	//HI_U8  OV2710_Luma_TF_Tbl[8]={80,90,100,135,160,200,220,220};
	HI_U8  OV2710_Luma_TF_Tbl[8]= {80,86,100,110,120,166,190,200};//OK
	//HI_U8  OV2710_Chr_SF_Tbl[8]={8,8,10,12,20,24,26,28};
	HI_U8  OV2710_Chr_SF_Tbl[8]={8,8,10,20,40,65,70,80};

	HI_U32  IMX123_LINE_3D_NR_Tbl[16]={128,140,203,310,360,410,500,650,900,900,1000,650,650,650,650,650};
	
	HI_U32  IMX123_WDR_3D_NR_Tbl[16] ={180,250,300,400,420,500,600,650,650,650,650,650,650,650,650,650};



	HI_U8  OV4689_Luma_SF_STILL_Tbl[16]={10,10,10,25,32,42,42,56,56,56,56,56,56,56,56,56};
	HI_U8  OV4689_Luma_SF_MOVE_Tbl[16]= {3,4,5,11,24,24,24,50,50,50,50,50,50,50,50,50};
	HI_U32  OV4689_Luma_TF_Tbl[16]= {90,100,104,120,140,195,260,300,300,300,300,300,300,300,300,300};
	HI_U8  OV4689_Chr_SF_Tbl[16]={8,10,12,64,80,120,180,180,180,180,180,180,180,180,180,180};


	
	HI_U32  OV4689_LINE_3D_NR_Tbl[16]={180,220,280,440,550,600,600,650,650,650,650,650,650,650,650,650};
	
	HI_U32  OV4689_WDR_3D_NR_Tbl[16] ={190,250,350,500,600,600,600,650,650,650,650,650,650,650,650,650};

	HI_U32  AR0230_LINE_3D_NR_Tbl[16]={128,170,300,350,400,500,600,650,650,650,650,650,650,650,650,650};
	
	HI_U8  AR0237_Luma_SF_STILL_Tbl[8]={10,12,30,45,48,55,63,63};
	HI_U8  AR0237_Luma_SF_MOVE_Tbl[8]= {5,6,8,12,20,32,45,50};
	HI_U8  AR0237_Luma_TF_Tbl[8]= {64,64,72,85,106,130,135,136};
	HI_U8  AR0237_Chr_SF_Tbl[8]={8,8,10,20,30,40,60,70};

	HI_U8  IMX185_Luma_SF_Tbl[8]={8,8,14,16,24,40,45,45};
	HI_U8  IMX185_Luma_TF_Tbl[8]={32,42,44,50,60,64,90,120};
	HI_U8  IMX185_Chr_SF_Tbl[8]={8,8,8,8,12,16,20,24};
	HI_U32 AeBias =1024; 


		
    HI_U32  Imx291_Luma_TF_Tbl[16]= {56,73,80,92,110,145,240,300,350,400,450,450,500,500,500,500};
		
	HI_U8  Imx291_Luma_SF_MOVE_Tbl[16] ={1,1,1,2,4,10,12,15,24,36,36,36,36,42,42,42};
	HI_U8  Imx291_Luma_SF_STILL_Tbl[16]={8,8,8,16,24,24,42,48,48,48,48,48,48,48,48,48};
		
	HI_U8  Imx291_Chr_SF_Tbl[16]={12,12,20,36,64,64,160,180,180,180,180,180,180,180,180,180}; 
	HI_U8  Imx291_Chr_TF_Tbl[16]={0,0,1,6,8,10,12,12,12,12,15,20,32,32,32,32};	
	
	
    isp_ioctl(0, GET_ID,(unsigned long)&sensor);
    printf("get sensor ID :%ld\n",sensor);

	if (sensor == SENSOR_IMX178)
	{
		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 90000; 
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	}
	if (sensor == SENSOR_IMX123)
	{
		//ISP_EXPOSURE_ATTR_S stExpAttr;
		//HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		//stExpAttr.stAuto.stSysGainRange.u32Max = 160*1024; 
		//HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	}
	else if (sensor == SENSOR_OV4689)
	{

	}
	else if (sensor == SENSOR_AR0330)
	{
		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 50*1024;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	}
	else if (sensor == SENSOR_OV2710)
	{
		//ISP_EXPOSURE_ATTR_S stExpAttr;
		//HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		//stExpAttr.stAuto.stSysGainRange.u32Max = 30*1024;
		//HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	}
	else if (sensor == SENSOR_AR0230)
	{
		//ISP_EXPOSURE_ATTR_S stExpAttr;
		//HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		//stExpAttr.stAuto.stSysGainRange.u32Max = 64*1024;
		//HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
		//ISP_CR_ATTR_S  pstCRAttr;
		//HI_MPI_ISP_GetCrosstalkAttr(IspDev,&pstCRAttr);
		//pstCRAttr.u8Sensitivity =0;
		//pstCRAttr.u16Slope =0;
		//HI_MPI_ISP_SetCrosstalkAttr(IspDev,&pstCRAttr);

	}
	//else if (sensor == SENSOR_IMX290)
	//{
		//ISP_EXPOSURE_ATTR_S stExpAttr;
		//HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		//stExpAttr.stAuto.stSysGainRange.u32Max = 160*1024;
		///HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	//}
	
	VPSS_GRP_PARAM_S pstVpssParam;

	HI_MPI_VPSS_GetGrpParam(0, &pstVpssParam);
	pstVpssParam.s32GlobalStrength = 129;
	HI_MPI_VPSS_SetGrpParam(0, &pstVpssParam);//暂不适用第二代3D参数

	ISP_DP_ATTR_S  pstDPAttr;
	HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
	pstDPAttr.stDynamicAttr.u16Slope=128;
	HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);
	
	
	unsigned int jv_exp;
	unsigned char NightCnt=0;//软光敏白切黑的次数统计
	unsigned char DayCnt=0;//软光敏黑切白次数统计
	unsigned char stAvm;
	//BOOL  Jv_Night_St= FALSE;
	BOOL  Night_Start =FALSE;
	unsigned int Exp_Tbl[5];
	int Night_EXP_CAL =0;
	unsigned char Night_Wait_Times=0;
	unsigned char AeCalTimes = 0;
	int DrcStrength=0;

    int low_frame_cnt=0; //全彩低帧统计次数
	int full_frame_cnt=0;//全彩全帧统计次数
	int ToNightCnt=0;//进入黑白夜视统计次数

	unsigned char  AdcNightCnt =0;
	unsigned char  AdcDayCnt =0;
	
    JV_EXP_CHECK_THRESH_S jv_exp_thresh;
	HI_U32 expTime;
	static HI_U8 LowLightCnt =0;
	static HI_U8 HighLightCnt =0;
    if(sensor==SENSOR_IMX123)
    {
        jv_exp_thresh.DayFullFrameExpThresh=1977*50*64;//to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=1977*100*64;//to彩色降帧的曝光阈值
        //jv_exp_thresh.DayLowFrameRate_0=12;//彩色降帧帧?
        jv_exp_thresh.DayLowFrameRatio = 32;

        jv_exp_thresh.ToNightExpThresh=3954*320*64;//软光敏-彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=40;//软光敏彩色切黑白的亮度均值

        jv_exp_thresh.ToDayExpThresh=1977*10*64;//软光敏-黑白切彩色阈值
        jv_exp_thresh.ToDayAEDiffTresh=30*64;//软光敏-黑白切彩色的AE差值
    }
    if(sensor==SENSOR_IMX290)
    {
        jv_exp_thresh.DayFullFrameExpThresh=4348800; //1359*50*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=6088320; //1359*70*64...60gain; to彩色降帧的曝光阈值
        //jv_exp_thresh.DayLowFrameRate_0=15;//彩色降帧帧率
         jv_exp_thresh.DayLowFrameRatio = 27; //25fps low to 14.8fps  30fps low to 17. 8

        jv_exp_thresh.ToNightExpThresh=2294*220*64;//软光敏--彩色切黑白的阈?2294 --14.8; 1907--17.8;
        jv_exp_thresh.ToNightLumThresh=32;//软光敏彩色切黑白的亮度均值

        jv_exp_thresh.ToDayExpThresh=1359*6*64;//软光敏-黑白切彩色阈值
        jv_exp_thresh.ToDayAEDiffTresh=45*64;//软光敏-黑白切彩色的AE差值
    }
	ISP_SHADING_ATTR_S  stShadingAttr_OV2710;
	ISP_SHADING_ATTR_S  stShadingAttr_OV2710_half;
	if(sensor==SENSOR_OV2710)
    {
        jv_exp_thresh.DayFullFrameExpThresh=1325*12*64; //1359*50*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=1325*19*64; //1359*70*64...60gain; to彩色降帧的曝光阈值
       // jv_exp_thresh.DayLowFrameRate_0=13;//彩色降帧帧率
        jv_exp_thresh.DayLowFrameRatio = 32;

        jv_exp_thresh.ToNightExpThresh=2654*25*64;//软光敏--彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=64;//软光敏彩色切黑白的亮度均

		HI_MPI_ISP_GetShadingAttr(0,&stShadingAttr_OV2710);

		HI_MPI_ISP_GetShadingAttr(0,&stShadingAttr_OV2710_half);


		
		int i;
		for(i=0;i<129;i++)
		{
			stShadingAttr_OV2710_half.astRadialShading[0].u32Table[i]= stShadingAttr_OV2710.astRadialShading[0].u32Table[i]*91/100;
		}
		for(i=0;i<129;i++)
		{
			stShadingAttr_OV2710_half.astRadialShading[1].u32Table[i]= stShadingAttr_OV2710.astRadialShading[1].u32Table[i]*91/100;
		}
		for(i=0;i<129;i++)
		{
			stShadingAttr_OV2710_half.astRadialShading[2].u32Table[i]= stShadingAttr_OV2710.astRadialShading[2].u32Table[i]*91/100;
		}
    }
    
    
    while (1)
    {
    
		sleep(1);

		if(bWDRSwitchBusy)
		{
			if(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)
			{
				if(sensor == SENSOR_IMX123)
				{
					JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
				}
			}
			 bThreadFreeze =TRUE;
			 u32ISOLast =0;
			 continue;
		}

		if((sensor== SENSOR_IMX123||sensor== SENSOR_IMX290)&&bCheckNight)
		{
			CheckWaitTimes++;
			if(CheckWaitTimes>10)
			{
				
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				//stExpAttr.stAuto.u8Tolerance = 2;
				//stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 10;
				//stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 2;
				stExpAttr.stAuto.u8Speed = 64;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);	
				bCheckNight =FALSE;
				CheckWaitTimes =0;
			}
		}

		HI_MPI_ISP_QueryExposureInfo(IspDev, &stExpInfo);
		

		u32ISO  = ((HI_U64)stExpInfo.u32AGain * stExpInfo.u32DGain* stExpInfo.u32ISPDGain * 100) >> (10 * 3);
		jv_exp = stExpInfo.u32Exposure;
		stAvm=stExpInfo.u8AveLum;
		expTime = stExpInfo.u32ExpTime;

		if(sensor == SENSOR_OV4689)
		{
			int IsoThresh =1500;
			if(__check_wdr_state())
				IsoThresh = 1300;
			if(u32ISO > IsoThresh)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			
			if(AdcNightCnt>=4)
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		else if(sensor == SENSOR_AR0230)
		{
			if(u32ISO > 4000)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			if(AdcNightCnt>=4)	
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		else if(sensor == SENSOR_AR0237||sensorType == SENSOR_AR0237DC)
		{
			if(u32ISO > 2000)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			if(AdcNightCnt>=4)	
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		else if(sensor == SENSOR_IMX290)
		{
			if(u32ISO > 4000)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			if(AdcNightCnt>=3)	
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		else if(sensor == SENSOR_IMX123)
		{
			if(u32ISO > 3500)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			if(AdcNightCnt>=3)	
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		else if(sensor == SENSOR_OV2710)
		{
			if(u32ISO > 1400)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			if(AdcNightCnt>=3)	
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
	
		if(sensor == SENSOR_IMX123)
		{
			pthread_mutex_lock(&isp_gamma_mutex);


			if(bNightMode ==FALSE)
			{
				if(u32ISO >=16000)//&&stAvm<32)
				{
					LowLightCnt++;
					if(LowLightCnt>=3)
					{
						if(bLowLightState ==FALSE)
						{
							bLowLightState =TRUE;
							isp_ioctl(0,ADJ_CONTRAST,40);
						}
						LowLightCnt =0;
						
					}
				}
				else 
					LowLightCnt =0;
				
				if(u32ISO <=11000)
				{
					HighLightCnt++;
					if(HighLightCnt>=3)
					{
						if(bLowLightState==TRUE)
						{
							isp_ioctl(0,ADJ_CONTRAST,128);
							bLowLightState =FALSE;
						}
						HighLightCnt =0;
					}
				}
				else
					HighLightCnt =0;
			}
			pthread_mutex_unlock(&isp_gamma_mutex);
		}


		if(sensor == SENSOR_IMX290)
		{
			pthread_mutex_lock(&isp_gamma_mutex);


			if(bNightMode ==FALSE)
			{
				if(u32ISO >=30000)//&&stAvm<32)
				{
					LowLightCnt++;
					if(LowLightCnt>=3)
					{
						if(bLowLightState ==FALSE)
						{
							bLowLightState =TRUE;
							isp_ioctl(0,ADJ_CONTRAST,40);
						}
						LowLightCnt =0;
						
					}
				}
				else 
					LowLightCnt =0;
				
				if(u32ISO <=25000)
				{
					HighLightCnt++;
					if(HighLightCnt>=3)
					{
						if(bLowLightState==TRUE)
						{
							isp_ioctl(0,ADJ_CONTRAST,128);
							bLowLightState =FALSE;
						}
						HighLightCnt =0;
					}
				}
				else
					HighLightCnt =0;
			}
			pthread_mutex_unlock(&isp_gamma_mutex);
		}

		if ((sensor == SENSOR_IMX123 || sensor== SENSOR_IMX290)&&(__check_cp()==FALSE))
		{
            int bNight;
			
			BOOL blow = TRUE;
			unsigned int expthrd ;
			unsigned int  lumthrd;
			light_ae_node aeNode ;
			
			pthread_mutex_lock(&low_frame_mutex);
			
			if(sensor== SENSOR_IMX123)
			{

			    aeNode = light_ae_list_imx123[LightAeIndex];
				expthrd = aeNode.ExpThrd;
				lumthrd = aeNode.LumThrd;
				blow = aeNode.bLow;

				jv_exp_thresh.ToNightExpThresh = expthrd;

						
				jv_exp_thresh.ToNightLumThresh = lumthrd;


				if(JV_ISP_COMM_Get_StdFps()>25) //30 fps情况下
					jv_exp_thresh.ToNightExpThresh = jv_exp_thresh.ToNightExpThresh*25/30;
			}
				
			
            if(strstr(hwinfo.devName, "N52-HS"))
			{
             
                if(StarLightEnable)
                {

                    if((__check_wdr_state()==FALSE)&&SensorState==SENSOR_STATE_DAY_FULL_FRAMRATE&&jv_exp>=jv_exp_thresh.DayLowFrameExpThresh_0)
                    {
                        low_frame_cnt++;
                        if(low_frame_cnt>=2&&blow)
                        {
                            JV_ISP_COMM_Set_LowFps(jv_exp_thresh.DayLowFrameRatio);
                            SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
                            low_frame_cnt=0;
                            printf("enter sensor state %d\n",SensorState);
                        }
                    }
                    else 
                        low_frame_cnt=0;
                    if((SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)&&(jv_exp <= jv_exp_thresh.DayFullFrameExpThresh))
				    {
					    full_frame_cnt++;
					
					    if(full_frame_cnt>=4)
					    {
					
						    JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
						    SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
						    printf("enter sensor state %d\n",SensorState);
						    full_frame_cnt=0;
					    }
				
				    }
				    else
					    full_frame_cnt =0;
					
					if( (sensor== SENSOR_IMX290)&&(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE))
					{
						

						jv_exp_thresh.ToNightExpThresh = 2294*220*64;

						if(JV_ISP_COMM_Get_StdFps()>25) //30 fps情况下
							jv_exp_thresh.ToNightExpThresh = 1907*220*64;
					}


					//if( (sensor== SENSOR_IMX123)&&(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE))
					//{
						

						//jv_exp_thresh.ToNightExpThresh = 2294*220*64;

						//if(JV_ISP_COMM_Get_StdFps()>25) //30 fps情况下
							//jv_exp_thresh.ToNightExpThresh = 1907*220*64;
						
				//	}


					if((__check_wdr_state())&&(sensor== SENSOR_IMX123))
					{
						//if(u32ISO>=4800)
							//ToNightToken++;
							bNight =jv_adc_read();
							if(bNight)
						    {
							    bAeNight =TRUE;
							    SensorState = SENSOR_STATE_NIGHT;
							    //printf("enter sensor state  %d,bAe %d\n",SensorState,bAeNight);
						    }
	
					}
					 	
                    if((bAeNight==FALSE)&&(__check_wdr_state()==FALSE)&&(jv_exp >= jv_exp_thresh.ToNightExpThresh)&&stAvm<=jv_exp_thresh.ToNightLumThresh)
				    {
				        printf("666666666666 bllloowwww %d goto night thread is exp %d, exp thd %d,avm %d ,avmthrd %d\n",JV_ISP_COMM_Query_LowFps(),jv_exp,jv_exp_thresh.ToNightExpThresh,stAvm,jv_exp_thresh.ToNightLumThresh);
				    	
					    ToNightCnt++;
					    if(ToNightCnt>=2)
					    {

						    bNight =jv_adc_read();
                            //printf("guangmin is %d\n",bNight);
						    if(bNight&&(DayNightMode!=DAYNIGHT_ALWAYS_DAY))
						    {
							    bAeNight =TRUE;
							    SensorState = SENSOR_STATE_NIGHT;
							    //printf("enter sensor state  %d,bAe %d\n",SensorState,bAeNight);
						    }

						    ToNightCnt=0;
					    }
				
				    }
				    else
					    ToNightCnt =0;
                    if(SensorState == SENSOR_STATE_NIGHT)
                    {
                        bNight =jv_adc_read();
                        if(bNight)
                        {
                        }
                        else
                        {
                              bAeNight =FALSE;
                        }
                       // printf("enter sensor state  %d,bAe %d\n",SensorState,bAeNight);
                                    
                    }                
                    
                }

                
                
			}
               
			else if(!strstr(hwinfo.devName, "N52-HS"))
			{
              
                if(Night_Start&&jv_sensor_get_redlight_staus())
			    {
				
				    if(Night_Wait_Times<=5)
				    {
					    Night_Wait_Times++;
				    }
				    else
				    {
					    Exp_Tbl[AeCalTimes]=jv_exp;
					    AeCalTimes++;
					    if(AeCalTimes>=1)
					    {
						    Night_EXP_CAL= Exp_Tbl[0]; // +Exp_Tbl[1])/2;
						    printf("got ae caltime done is %d\n",Night_EXP_CAL);
						    Night_Start=FALSE;
						    AeCalTimes=0;
						    Night_Wait_Times=0;
					    }
				    }
				
			    }
				if( (sensor== SENSOR_IMX290)&& (bAeNight==FALSE))
				{
					jv_exp_thresh.ToNightExpThresh = 2294*220*64;

					if(JV_ISP_COMM_Get_StdFps()>25) //30 fps情况下
						jv_exp_thresh.ToNightExpThresh = 1907*220*64;
				}

				//if(sensor== SENSOR_IMX123)
				//{

					//light_ae_node aeNode = light_ae_list_imx123[LightAeIndex];
					//expthrd = aeNode.ExpThrd;
					//lumthrd = aeNode.LumThrd;
					//blow = aeNode.bLow;

					//jv_exp_thresh.ToNightExpThresh = expthrd;
					//jv_exp_thresh.ToNightLumThresh = lumthrd;
				//}

		        //白天切换到夜视
			    if((bAeNight==FALSE)&&jv_exp >=jv_exp_thresh.ToNightExpThresh&&stAvm<=jv_exp_thresh.ToNightLumThresh)
			    {
			    
					printf("3888 666666666666 bllloowwww %d goto night thread is exp %d, exp thd %d,avm %d ,avmthrd %d\n",JV_ISP_COMM_Query_LowFps(),jv_exp,jv_exp_thresh.ToNightExpThresh,stAvm,jv_exp_thresh.ToNightLumThresh);
				
				    NightCnt++;
				    if(NightCnt>=2)
				    {
					    bAeNight =TRUE;
						
					    NightCnt=0;
						AeCalTimes=0;
					 	Night_Wait_Times=0;
						Night_Start=TRUE;
				    }
			    }	
			    else
				    NightCnt=0;
			    //夜视切换到白天
			    if((bAeNight)&&(jv_exp <=jv_exp_thresh.ToDayExpThresh)&&(Night_EXP_CAL>0))
			    {

					if( sensor== SENSOR_IMX290)
					{
						jv_exp_thresh.ToDayAEDiffTresh =  45*64;

						if(Night_EXP_CAL<= 1359*64)
							jv_exp_thresh.ToDayAEDiffTresh =  100*64;
						else if(Night_EXP_CAL<= 600*64)
							jv_exp_thresh.ToDayAEDiffTresh =  80*64;
						else if(Night_EXP_CAL<= 300*64)
							jv_exp_thresh.ToDayAEDiffTresh =  60*64;
						else if(Night_EXP_CAL<= 150*64)
							jv_exp_thresh.ToDayAEDiffTresh =  40*64;
						else if(Night_EXP_CAL<= 70*64)
							jv_exp_thresh.ToDayAEDiffTresh =  20*64;
						else if(Night_EXP_CAL<= 10*64)
							jv_exp_thresh.ToDayAEDiffTresh =  5*64;

						if(Night_EXP_CAL>= 1359*50*64)
							jv_exp_thresh.ToDayAEDiffTresh =  45*64;
						else if(Night_EXP_CAL>= 1359*80*64)
							jv_exp_thresh.ToDayAEDiffTresh =  60*64;
						else if(Night_EXP_CAL>= 1359*150*64)
							jv_exp_thresh.ToDayAEDiffTresh =  80*64;
						else if(Night_EXP_CAL>= 1359*190*64)
							jv_exp_thresh.ToDayAEDiffTresh =  110*64;


						jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/4;


						if(Night_EXP_CAL>= 1359*10*64)
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/2;
						else if(Night_EXP_CAL<= 1359*64)
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/4;
						else
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/3;
						
						if(Night_EXP_CAL<= 600*64)
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/5;
							

					}
					else if(sensor== SENSOR_IMX123)
					{
						jv_exp_thresh.ToDayAEDiffTresh =  30*64;
						
						if(Night_EXP_CAL<= 1977*64)
							jv_exp_thresh.ToDayAEDiffTresh = 150*64;
						else if(Night_EXP_CAL<=900*64)
							jv_exp_thresh.ToDayAEDiffTresh =  100*64;
						else if(Night_EXP_CAL<= 450*64)
							jv_exp_thresh.ToDayAEDiffTresh =  80*64;
						else if(Night_EXP_CAL<= 220*64)
							jv_exp_thresh.ToDayAEDiffTresh =  50*64;
						else if(Night_EXP_CAL<= 100*64)
							jv_exp_thresh.ToDayAEDiffTresh =  20*64;
						else if(Night_EXP_CAL<= 50*64)
							jv_exp_thresh.ToDayAEDiffTresh =  20*64;
						else if(Night_EXP_CAL<= 20*64)
							jv_exp_thresh.ToDayAEDiffTresh =  10*64;
						else if(Night_EXP_CAL<= 10*64)
							jv_exp_thresh.ToDayAEDiffTresh =  5*64;

						if(Night_EXP_CAL>= 1977*50*64)
							jv_exp_thresh.ToDayAEDiffTresh =  30*64;
						else if(Night_EXP_CAL>= 1977*80*64)
							jv_exp_thresh.ToDayAEDiffTresh =  60*64;
						else if(Night_EXP_CAL>= 1977*100*64)
							jv_exp_thresh.ToDayAEDiffTresh =  80*64;
						else if(Night_EXP_CAL>= 1977*120*64)
							jv_exp_thresh.ToDayAEDiffTresh =  100*64;




						if(Night_EXP_CAL>= 1977*10*64)
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/2;
						else if(Night_EXP_CAL<= 1977*64)
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/4;
						else
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/3;


						if(Night_EXP_CAL<= 900*64)
							jv_exp_thresh.ToDayAEDiffTresh = Night_EXP_CAL/5;

							

					}
			    	
				    if((jv_exp+jv_exp_thresh.ToDayAEDiffTresh)<=Night_EXP_CAL)
				    {
					    DayCnt++;
					    if(DayCnt>=4)
					    {
						    DayCnt=0;
						    Night_EXP_CAL =0;    
							bAeNight =FALSE;
					    }                        
				    }
				    else
					    DayCnt=0;
			    }	
			    else
				    DayCnt=0;

                
			    //if(Jv_Night_St!=bAeNight)
			  //  {
				   // printf(" bAeNight entering %d mode\n",Jv_Night_St);
				  //  if(Jv_Night_St)
				  ///  {
					  //  Night_Start=TRUE;
				   // }
				   // bAeNight= Jv_Night_St;

			   // }
                
                //白天全彩模式下降帧
                if(jv_exp> jv_exp_thresh.DayLowFrameExpThresh_0&&bNightMode==FALSE&&(DayNightMode!=DAYNIGHT_TIMER))
			    {
				    low_frame_cnt++;
				    if(low_frame_cnt>=2&&blow)
				    {
					     //jv_sensor_set_framerate_v2(jv_exp_thresh.DayLowFrameRate_0,TRUE);
					     printf("day lowframe done gggggggggggggggggggggggggg\n");
					     JV_ISP_COMM_Set_LowFps(jv_exp_thresh.DayLowFrameRatio);
					     low_frame_cnt=0;
                         
				    }
			    }
			    else 
				     low_frame_cnt=0;
			
			    if(jv_exp<jv_exp_thresh.DayFullFrameExpThresh&&bNightMode==FALSE&&(DayNightMode!=DAYNIGHT_TIMER))
			    {
				     full_frame_cnt++;
				     if(full_frame_cnt>=4)
				     {
					       JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					       full_frame_cnt=0;
                           
				     }
			    }
			    else
				     full_frame_cnt=0;
			}
			pthread_mutex_unlock(&low_frame_mutex);
            
            
		}
		if(sensor == SENSOR_OV2710)
		{
            int bNight;
            pthread_mutex_lock(&low_frame_mutex);
			if(StarLightEnable)
			{
				if(SensorState==SENSOR_STATE_DAY_FULL_FRAMRATE&&jv_exp>=jv_exp_thresh.DayLowFrameExpThresh_0)
				{
					low_frame_cnt++;
                    if(low_frame_cnt>=3)
                    {
                    	JV_ISP_COMM_Set_LowFps(jv_exp_thresh.DayLowFrameRatio);
                        SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
                        low_frame_cnt=0;
                        printf("enter half framerate sensor state %d\n",SensorState);
                    }
                }
                else 
                    low_frame_cnt=0;
                if((SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)&&(jv_exp <= jv_exp_thresh.DayFullFrameExpThresh))
				{
					full_frame_cnt++;
					
					if(full_frame_cnt>=4)
					{
						JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
						SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
						printf("enter full framerate sensor state %d\n",SensorState);
						full_frame_cnt=0;
					 }
				
				 }
				 else
					 full_frame_cnt =0;
				 
				if(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)
				{
					jv_exp_thresh.ToNightExpThresh = 2654*26*64;
					//if(jv_sensor_get_vi_framerate()>25)
					if(JV_ISP_COMM_Get_StdFps()>25) 
						jv_exp_thresh.ToNightExpThresh = 2654*21*64;
				}
				 
                 if((SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)&&(jv_exp >= jv_exp_thresh.ToNightExpThresh))//&&stAvm<=jv_exp_thresh.ToNightLumThresh)
				 {
				 	ToNightCnt++;
					if(ToNightCnt>=2)
					{

						bNight =jv_adc_read();
						if(bNight&&(DayNightMode!=DAYNIGHT_ALWAYS_DAY))
						{
							 bAeNight =TRUE;
							 SensorState = SENSOR_STATE_NIGHT;
							 AdcDayCnt =0;
							   printf("enter night state sensor state  %d,bAe %d\n",SensorState,bAeNight);
						 }

						 ToNightCnt=0;
					}
				
				 }
				 else
					 ToNightCnt =0;
                 if(SensorState == SENSOR_STATE_NIGHT)
                 {
                 	bNight =jv_adc_read();
                    if(bNight)
                    {
                     	AdcDayCnt=0;
                    }
                    else
                    {
                    	AdcDayCnt++;
						if(AdcDayCnt>=3)
						{
                        	bAeNight =FALSE;
							AdcDayCnt =0;
						}
                    }
				 }                
                    
               }
			
			pthread_mutex_unlock(&low_frame_mutex); 
                
		}
               

		if( u32ISO == u32ISOLast)
			continue;

		if(sensor == SENSOR_OV2710)
		{
			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 7;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;    
				
			}
			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 14;
			pstVpssParamV2.Chroma_SF_Strength =jv_IQ_AgcTableCalculate(OV2710_Chr_SF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_MotionThresh =jv_IQ_AgcTableCalculate(OV2710_Luma_TF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_StillArea =jv_IQ_AgcTableCalculate(OV2710_Luma_SF_STILL_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_MoveArea =jv_IQ_AgcTableCalculate(OV2710_Luma_SF_MOVE_Tbl, u8Index, u32ISO);

			if(u32ISO>=2400)
				pstVpssParamV2.Chroma_TF_Strength = 16;
			else if(u32ISO<=1600)
				pstVpssParamV2.Chroma_TF_Strength = 8;
			
			//if(pstVpssParamV2.Luma_SF_MoveArea>=32)
			//	pstVpssParamV2.Luma_SF_MoveArea=32;
			//if(u32ISO>=3200)
				//pstVpssParamV2.Luma_SF_MoveArea=40;
			//if(u32ISO>=4200)
				//pstVpssParamV2.Luma_SF_MoveArea=50;
			if(bNightMode ==FALSE)
			{
				if(pstVpssParamV2.Luma_MotionThresh >=160)
					pstVpssParamV2.Luma_MotionThresh = 172;

			}
				
			
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);

			ISP_DP_ATTR_S pstDPAttr;
			HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
			pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;

			pstDPAttr.stDynamicAttr.u16Slope=512;
			pstDPAttr.stDynamicAttr.u16Thresh=64;
			
			if(u32ISO<=800)
			{
				pstDPAttr.stDynamicAttr.u16Slope=256;
				pstDPAttr.stDynamicAttr.u16Thresh=188;
			}
			else if(u32ISO<=1600)
			{
				pstDPAttr.stDynamicAttr.u16Slope=512;
				pstDPAttr.stDynamicAttr.u16Thresh=188;
			}
			else if(u32ISO<=2700)
			{
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh=128;
			}
			else
			{
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh=64;
			}	
			HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);


			int evBias =0;
			if(u32ISO >=2600)
			{
				evBias=860;
									
			}
			//else if(u32ISO>=1200&&u32ISO<1700)
		//	{
				//evBias=768; //45					
			//}
			else if(u32ISO<=1600)
			{
				evBias=1024;//32	
			}

			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			if(evBias>0)
				stExpAttr.stAuto.u16EVBias = evBias;
			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			
			if(u32ISO >=2400)
			{
				HI_MPI_ISP_SetShadingAttr(0,&stShadingAttr_OV2710_half);

			}
			else if(u32ISO <=1600)
			{
				HI_MPI_ISP_SetShadingAttr(0,&stShadingAttr_OV2710);
			}
			
		}
		if(sensor == SENSOR_AR0237||sensorType == SENSOR_AR0237DC)
		{
			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 7;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;    
				
			}
			
			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 14;
			pstVpssParamV2.Chroma_SF_Strength =jv_IQ_AgcTableCalculate(AR0237_Chr_SF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_MotionThresh =jv_IQ_AgcTableCalculate(AR0237_Luma_TF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_StillArea =jv_IQ_AgcTableCalculate(AR0237_Luma_SF_STILL_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_MoveArea =jv_IQ_AgcTableCalculate(AR0237_Luma_SF_MOVE_Tbl, u8Index, u32ISO);

			if(u32ISO>=2400)
				pstVpssParamV2.Chroma_TF_Strength = 20;
			else if(u32ISO<=1600)
				pstVpssParamV2.Chroma_TF_Strength = 8;
			if(__check_wdr_state())
			{
				//if(u32ISO>=800)
					//pstVpssParamV2.Luma_SF_MoveArea =22;
				if(u32ISO>=1500)
				{
					pstVpssParamV2.Luma_SF_MoveArea =32;
					pstVpssParamV2.Luma_SF_StillArea = 60;
				}
				if(pstVpssParamV2.Luma_SF_MoveArea<=32)
					pstVpssParamV2.Luma_SF_MoveArea = 32;
				
				if(pstVpssParamV2.Luma_MotionThresh<=64)
					pstVpssParamV2.Luma_MotionThresh = 64;
			}
			
			
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
           	if(__check_wdr_state())
           	{
           		int drcStrength = 0;
				if(u32ISO<=1000)			
					drcStrength = 128;
				else if(1400<u32ISO&&u32ISO<=2400)
					drcStrength = 108;
				else if(u32ISO>=2600)
					drcStrength = 88;

				if(drcStrength > 0)
				{
           			ISP_DRC_ATTR_S pstDRCAttr;
					HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
					pstDRCAttr.bEnable = HI_TRUE;
					pstDRCAttr.stAuto.u32Strength =drcStrength;
					HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);
				}
           	}

			ISP_DP_ATTR_S pstDPAttr;
			HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
			pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;
			pstDPAttr.stDynamicAttr.u16Slope=512;
			pstDPAttr.stDynamicAttr.u16Thresh=188;

			if(__check_wdr_state()) //wdr 模式下
			{
				if(u32ISO>=3000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=1000;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else if(u32ISO>=2000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=800;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else if(u32ISO>=1400)
				{
					pstDPAttr.stDynamicAttr.u16Slope=800;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else if(u32ISO>=800)
				{
					pstDPAttr.stDynamicAttr.u16Slope=720;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else
				{
					pstDPAttr.stDynamicAttr.u16Slope= 600;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
			}
			else  //线性模式
			{
			
				if(u32ISO>=3000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=1000;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else if(u32ISO>=2000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=800;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else if(u32ISO>=1400)
				{
					pstDPAttr.stDynamicAttr.u16Slope=600;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
				else if(u32ISO>=800)
				{
					pstDPAttr.stDynamicAttr.u16Slope=256;
					pstDPAttr.stDynamicAttr.u16Thresh=188;
				}
				else
				{
					pstDPAttr.stDynamicAttr.u16Slope=128;
					pstDPAttr.stDynamicAttr.u16Thresh=188;
				}		
				//HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);
			}
			HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);		
		}
		if (sensor == SENSOR_AR0230)
		{
			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 15;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;    
				
			}

			
			int globa_value=128;
			int evBias =0;
			ISP_EXPOSURE_ATTR_S stExpAttr;

			if(__check_wdr_state()==FALSE)
			{
				globa_value = jv_IQ_AgcTableCalculate_v2_u32(AR0230_LINE_3D_NR_Tbl,u8Index,u32ISO);

				if(u32ISO<=800)
					evBias =1024;
				else if(2000<u32ISO&&u32ISO<=3200)
					evBias =896;
				else if( u32ISO>= 4800)
					evBias =768;
			}
			else
			{
				if(u32ISO<=120)
					globa_value =128;
				else if(120<u32ISO&&u32ISO<=160)
					globa_value =140;
				else if(160<u32ISO&&u32ISO<=250)
					globa_value =160;
				else if(250<u32ISO&&u32ISO<=800)
					globa_value =200;
				else if(800<u32ISO&&u32ISO<=4500)
					globa_value =270;
				else if(u32ISO>4500)
					globa_value =350;
			}

			if(__check_wdr_state())
			{
				
				DrcStrength =0;
				if(u32ISO<=2400)
					DrcStrength = 220;
				else if(u32ISO>2800&&u32ISO<=3600)
					DrcStrength = 200;
				else if(u32ISO>4000&&u32ISO<=4800)
					DrcStrength = 180;
				else if(u32ISO>5200&&u32ISO<=6000)
					DrcStrength = 150;
				else if(u32ISO>6400&&u32ISO<=7200)
					DrcStrength = 130;
				else if(u32ISO>8000)
					DrcStrength = 100;			
			}
			if(1)
			{
				HI_MPI_VPSS_GetGrpParam(0, &pstVpssParam);
				pstVpssParam.s32GlobalStrength = globa_value;
				HI_MPI_VPSS_SetGrpParam(0, &pstVpssParam);
				if(__check_wdr_state()==FALSE)
				{
					if(evBias>0)
					{
						HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
						stExpAttr.stAuto.u16EVBias = evBias;
						HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
					}

				}
				else
				{
					if(DrcStrength>0)
					{
						ISP_DRC_ATTR_S pstDRCAttr;
           				HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);

						pstDRCAttr.enOpType= OP_TYPE_MANUAL;
					
						pstDRCAttr.stManual.u32Strength=DrcStrength;
           				HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);  
					}
				}
			}

			if(u32ISO>=6400)
			{

				ISP_DP_ATTR_S pstDPAttr;
				HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
				pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh=128;
				HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);

			}
			else if(u32ISO>=1800)
			{

				ISP_DP_ATTR_S pstDPAttr;
				HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
				pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;
				pstDPAttr.stDynamicAttr.u16Slope=512;
				pstDPAttr.stDynamicAttr.u16Thresh=128;
				HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);

			}
			else
			{
				ISP_DP_ATTR_S pstDPAttr;
				HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
				pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;
				pstDPAttr.stDynamicAttr.u16Slope=256;
				pstDPAttr.stDynamicAttr.u16Thresh=188;
				HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);


			}


		}
		
		if (sensor == SENSOR_AR0330)
		{
			int globa_value=128;
			int evBias =0;
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			HI_MPI_VPSS_GetGrpParam(0, &pstVpssParam);
			
			if(u32ISO<=120)
				globa_value =128;
			else if(120<u32ISO&&u32ISO<=150)
				globa_value =160;
			else if(150<u32ISO&&u32ISO<=280)
				globa_value =210;
			else if(280<u32ISO&&u32ISO<=550)
				globa_value =265;
			else if(550<u32ISO&&u32ISO<=1000)
				globa_value =310;
			else if(1000<u32ISO&&u32ISO<=2000)
				globa_value =380;
			else if(2000<u32ISO&&u32ISO<=3000)
				globa_value =480;
			else if(3000<u32ISO&&u32ISO<=5000)
				globa_value =580;
			else if(5000<u32ISO&&u32ISO<=8000)
				globa_value =620;
			else if(8000<u32ISO)
				globa_value =680;

			pstVpssParam.s32GlobalStrength = globa_value;
			HI_MPI_VPSS_SetGrpParam(0, &pstVpssParam);

			if(u32ISO<=1000)
				evBias =1024;
			else if(1000<u32ISO&&u32ISO<=3200)
				evBias =896;
			else if(3200<u32ISO&&u32ISO<=5800)
				evBias =820;
			else if(5800<u32ISO&&u32ISO<=8000)
				evBias =768;
			else if( u32ISO> 800)
				evBias =700;
			if(evBias>0)
				stExpAttr.stAuto.u16EVBias = evBias;

			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			ISP_DP_ATTR_S pstDPAttr;
			HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
			pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;
			if(u32ISO<=150)
			{
				pstDPAttr.stDynamicAttr.u16Slope=512;
				pstDPAttr.stDynamicAttr.u16Thresh=90;
			}
			else
			{
				pstDPAttr.stDynamicAttr.u16Slope=1000;
				pstDPAttr.stDynamicAttr.u16Thresh=64;
			}
				
			HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);
		}
		
		if (sensor == SENSOR_IMX290)
		{

			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 15;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;    
				
			}


			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Chroma_SF_Strength =jv_IQ_AgcTableCalculate_v2_u8(Imx291_Chr_SF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_MotionThresh = jv_IQ_AgcTableCalculate_v2_u32(Imx291_Luma_TF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_StillArea = jv_IQ_AgcTableCalculate_v2_u8(Imx291_Luma_SF_STILL_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_MoveArea = jv_IQ_AgcTableCalculate_v2_u8(Imx291_Luma_SF_MOVE_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_TF_Strength =jv_IQ_AgcTableCalculate_v2_u8(Imx291_Chr_TF_Tbl, u8Index, u32ISO);
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);

			
			/*
			int globa_value=128;

			HI_MPI_VPSS_GetGrpParam(0, &pstVpssParam);

			if(u32ISO<=400)
				globa_value =128;
			else if(400<u32ISO&&u32ISO<=800)
				globa_value =250;
			else if(800<u32ISO&&u32ISO<=1200)
				globa_value =300;
			else if(1200<u32ISO&&u32ISO<=2800)
				globa_value =350;
			else if(2800<u32ISO&&u32ISO<=6300)
				globa_value =380;
			else if(6300<u32ISO&&u32ISO<=8000)
				globa_value =450;
			else if(8000<u32ISO&&u32ISO<=10000)
				globa_value =500;
			else if(u32ISO>10000&&u32ISO<=20000)
				globa_value =600;
			else if(u32ISO>20000)
				globa_value =900;
			pstVpssParam.s32GlobalStrength = globa_value;
			HI_MPI_VPSS_SetGrpParam(0, &pstVpssParam);

			if(u32ISO>4500)
			{

			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_SF_Strength =100;
			pstVpssParamV2.Luma_MotionThresh =250;
			pstVpssParamV2.Luma_SF_StillArea =48;
			pstVpssParamV2.Luma_SF_MoveArea =12;
			
			pstVpssParamV2.Chroma_TF_Strength = 12;
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}
			if(u32ISO>10000)//22000)
			{

			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_SF_Strength =100;
			pstVpssParamV2.Luma_MotionThresh =300;
			pstVpssParamV2.Luma_SF_StillArea =48;
			pstVpssParamV2.Luma_SF_MoveArea =12;
			
			pstVpssParamV2.Chroma_TF_Strength = 12;
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);	
			}
			
			if(u32ISO>25000)
			{

			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_SF_Strength =100;
			pstVpssParamV2.Luma_MotionThresh =350;
			pstVpssParamV2.Luma_SF_StillArea =48;
			pstVpssParamV2.Luma_SF_MoveArea =12;
			
			pstVpssParamV2.Chroma_TF_Strength = 12;
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}

			if(u32ISO>30000
			{

			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_SF_Strength =100;
			pstVpssParamV2.Luma_MotionThresh =380;
			pstVpssParamV2.Luma_SF_StillArea =48;
			pstVpssParamV2.Luma_SF_MoveArea =12;
			
			pstVpssParamV2.Chroma_TF_Strength = 12;
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}




			if(u32ISO>36000)
			{

			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_SF_Strength =160;
			pstVpssParamV2.Luma_MotionThresh =450;
			pstVpssParamV2.Luma_SF_StillArea =64;
			pstVpssParamV2.Luma_SF_MoveArea =12;
			
			pstVpssParamV2.Chroma_TF_Strength = 12;
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}



			

			
			
			{
				VPSS_GRP_VPPNRBEX_S pstVpssNrParam;
				HI_MPI_VPSS_GetNRV3Param(0, &pstVpssNrParam);
				pstVpssNrParam.iNRb.SBSi= -1;
				pstVpssNrParam.iNRb.SBSj= 32;
				pstVpssNrParam.iNRb.SBSk= 16;


				pstVpssNrParam.iNRb.SBTi= -1;
				pstVpssNrParam.iNRb.SBTj= 8;
				pstVpssNrParam.iNRb.SBTk= 8;

				pstVpssNrParam.iNRb.SDSi= -1;
				pstVpssNrParam.iNRb.SDSj= 64;
				pstVpssNrParam.iNRb.SDSk= 32;


				pstVpssNrParam.iNRb.SDTi= -1;
				pstVpssNrParam.iNRb.SDTj= 8;
				pstVpssNrParam.iNRb.SDTk= 8;



				
				pstVpssNrParam.iNRb.SBFi= -1;
				pstVpssNrParam.iNRb.SBFj= 1;
				pstVpssNrParam.iNRb.SBFk= 0;


				pstVpssNrParam.iNRb.SHPi= 85;
				pstVpssNrParam.iNRb.SHPj= 64;
				pstVpssNrParam.iNRb.SHPk= 32;



				pstVpssNrParam.iNRb.TFSi= -1;
				pstVpssNrParam.iNRb.TFSj= 12;
				pstVpssNrParam.iNRb.TFSk= 13;


				pstVpssNrParam.iNRb.TFRi= -1;
				pstVpssNrParam.iNRb.TFRj= 12;
				pstVpssNrParam.iNRb.TFRk= 31;



				pstVpssNrParam.iNRb.MDZi= -1;
				pstVpssNrParam.iNRb.MDZj= 32;
				pstVpssNrParam.iNRb.MATH = 450;

				pstVpssNrParam.iNRb.TFC = 12;
				pstVpssNrParam.iNRb.SFC= 160;
				
				HI_MPI_VPSS_SetNRV3Param(0, &pstVpssNrParam); */
				
			/*
			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Luma_TF_Strength = 13;
			pstVpssParamV2.Chroma_SF_Strength =160;
			pstVpssParamV2.Luma_MotionThresh =450;
			pstVpssParamV2.Luma_SF_StillArea =64;
			pstVpssParamV2.Luma_SF_MoveArea =12;
			
			pstVpssParamV2.Chroma_TF_Strength = 12;
			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);*/
				
			//}

			HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
			pstDPAttr.stDynamicAttr.u16Slope=128;
			pstDPAttr.stDynamicAttr.u16Thresh=188;
			HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);



			if(u32ISO>=3200)
			{
				HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
				pstDPAttr.stDynamicAttr.u16Slope=512;
				pstDPAttr.stDynamicAttr.u16Thresh=188;
				HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);

			}
			if(u32ISO>=12800)
			{
				HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh=188;
				HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);

			}

			if(u32ISO>=40000)
			{
				HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh=128;
				HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);

			}
			
			//if(u32ISO<=1200)
				//evBias =1024;
			//else if(1700<u32ISO&&u32ISO<=3200)
				//evBias =896;
			//else if( u32ISO>= 8000)
				//evBias =800;
			//if(evBias>0)
				//stExpAttr.stAuto.u16EVBias = evBias;

			

			//HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

			int evBias =0;
			if(u32ISO>15000)
			{
				evBias =760;
				
			}
			else if(u32ISO<= 7500)
			{
				evBias =1024;
				
			}
			
			if(evBias>0)
			{
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.u16EVBias = evBias;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			}
			
	




		}

			
		if (sensor == SENSOR_IMX123)
		{
			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 15;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;    
				
			}
			int globa_value=128;
			if(__check_wdr_state())		
				globa_value = jv_IQ_AgcTableCalculate_v2_u32(IMX123_WDR_3D_NR_Tbl,u8Index,u32ISO);
			else
				globa_value = jv_IQ_AgcTableCalculate_v2_u32(IMX123_LINE_3D_NR_Tbl,u8Index,u32ISO);
			

			HI_MPI_VPSS_GetGrpParam(0, &pstVpssParam);
			pstVpssParam.s32GlobalStrength = globa_value;
			HI_MPI_VPSS_SetGrpParam(0, &pstVpssParam);
			
			if(u32ISO>28000)
			{
			/*
				VPSS_GRP_VPPNRBEX_S pstVpssNrParam;
				HI_MPI_VPSS_GetNRV3Param(0, &pstVpssNrParam);
				pstVpssNrParam.iNRb.SBSi= -1;
				pstVpssNrParam.iNRb.SBSj= 32;
				pstVpssNrParam.iNRb.SBSk= 32;


				pstVpssNrParam.iNRb.SBTi= -1;
				pstVpssNrParam.iNRb.SBTj= 8;
				pstVpssNrParam.iNRb.SBTk= 8;

				pstVpssNrParam.iNRb.SDSi= -1;
				pstVpssNrParam.iNRb.SDSj= 64;
				pstVpssNrParam.iNRb.SDSk= 32;


				pstVpssNrParam.iNRb.SDTi= -1;
				pstVpssNrParam.iNRb.SDTj= 12;
				pstVpssNrParam.iNRb.SDTk= 12;



				
				pstVpssNrParam.iNRb.SBFi= -1;
				pstVpssNrParam.iNRb.SBFj= 1;
				pstVpssNrParam.iNRb.SBFk= 0;


				pstVpssNrParam.iNRb.SHPi= 85;
				pstVpssNrParam.iNRb.SHPj= 64;
				pstVpssNrParam.iNRb.SHPk= 32;



				pstVpssNrParam.iNRb.TFSi= -1;
				pstVpssNrParam.iNRb.TFSj= 12;
				pstVpssNrParam.iNRb.TFSk= 13;


				pstVpssNrParam.iNRb.TFRi= -1;
				pstVpssNrParam.iNRb.TFRj= 20;
				pstVpssNrParam.iNRb.TFRk= 31;



				pstVpssNrParam.iNRb.MDZi= -1;
				pstVpssNrParam.iNRb.MDZj= 64;
				pstVpssNrParam.iNRb.MATH = 450;

				pstVpssNrParam.iNRb.TFC = 12;
				pstVpssNrParam.iNRb.SFC= 160;
				
				HI_MPI_VPSS_SetNRV3Param(0, &pstVpssNrParam);*/
				VPSS_GRP_PARAM_V2_S pstVpssParamV2;
				HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
				pstVpssParamV2.Luma_TF_Strength = 13;
				pstVpssParamV2.Chroma_SF_Strength =160;
				pstVpssParamV2.Luma_MotionThresh =400;
				pstVpssParamV2.Luma_SF_StillArea =64;
				pstVpssParamV2.Luma_SF_MoveArea =16;
				
				pstVpssParamV2.Chroma_TF_Strength = 12;
				HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}


           	if(__check_wdr_state())
           	{
           			
           		int drcStrength = 0;
									
				if(u32ISO<=3800)			
					drcStrength = 128;
				else if(4200<u32ISO&&u32ISO<=5900)
					drcStrength = 108;
				else if(u32ISO>=6300)
					drcStrength = 78;

				if(drcStrength > 0)
				{
           				ISP_DRC_ATTR_S pstDRCAttr;
						HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
						pstDRCAttr.bEnable = HI_TRUE;
						pstDRCAttr.stAuto.u32Strength =drcStrength;
						HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);
				}
           	}
				
			ISP_DP_ATTR_S pstDPAttr;
			HI_MPI_ISP_GetDPAttr(IspDev, &pstDPAttr);
			pstDPAttr.stDynamicAttr.bEnable=HI_TRUE;
			if(u32ISO<=250)
			{
				pstDPAttr.stDynamicAttr.u16Slope=250;
				pstDPAttr.stDynamicAttr.u16Thresh=150;
			}
			else 
			{
				pstDPAttr.stDynamicAttr.u16Slope=500;
				pstDPAttr.stDynamicAttr.u16Thresh=100;
				if(u32ISO>=6000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=700;
					pstDPAttr.stDynamicAttr.u16Thresh=100;
				}
				if(u32ISO>=12000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=1024;
					pstDPAttr.stDynamicAttr.u16Thresh=100;
				}
				if(u32ISO>=20000)
				{
					pstDPAttr.stDynamicAttr.u16Slope=1024;
					pstDPAttr.stDynamicAttr.u16Thresh=64;
				}
			}
				
			HI_MPI_ISP_SetDPAttr(IspDev, &pstDPAttr);

			int evBias =1024;
			if(u32ISO>=22000&&AeBias==1024)//相对于1024
			{
				evBias =690;//ISO拉回16171
				
			}
			else if(u32ISO>=19000&&AeBias==1024)//相对于1024
			{
				evBias =862;//ISO拉回16171	
				
			}
			else if(u32ISO<=13000)
			{
				evBias =1024;
			}
			
			if(evBias != AeBias)
			{
				AeBias = evBias;
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.u16EVBias = evBias;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			}
			
		}
		if(sensor == SENSOR_IMX185)
		{
			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 7;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;     
			}

			
			
			//VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			//HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			//pstVpssParamV2.Chroma_SF_Strength =8;
			//pstVpssParamV2.Luma_MotionThresh =64;
			//pstVpssParamV2.Luma_SF_MoveArea = 32;
			//pstVpssParamV2.Luma_SF_StillArea =32;
			//pstVpssParamV2.Luma_TF_Strength =12;

			VPSS_GRP_PARAM_V2_S pstVpssParamV2;
			HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
			pstVpssParamV2.Chroma_SF_Strength =jv_IQ_AgcTableCalculate(IMX185_Chr_SF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_MotionThresh =jv_IQ_AgcTableCalculate(IMX185_Luma_TF_Tbl, u8Index, u32ISO);
			//pstVpssParamV2.Luma_SF_MoveArea  = 32;
			pstVpssParamV2.Luma_SF_StillArea =jv_IQ_AgcTableCalculate(IMX185_Luma_SF_Tbl, u8Index, u32ISO);
			pstVpssParamV2.Luma_SF_MoveArea =pstVpssParamV2.Luma_SF_StillArea;
			if(pstVpssParamV2.Luma_SF_MoveArea>=32)
				pstVpssParamV2.Luma_SF_MoveArea=32;

			HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);

		}

		if (sensor == SENSOR_OV4689)
		{
			int evBias =0;
			if(u32ISO<=1100)
			{
				evBias=1024; //60
									
			}
			//else if(u32ISO>=1200&&u32ISO<1700)
		//	{
				//evBias=768; //45					
			//}
			else if(u32ISO>=2400)
			{
				evBias=850;//32	
			}

			u32ISOTmp = u32ISO / 100;
			for(u8Index = 0; u8Index < 15;u8Index++)        
			{            
				if(1 == u32ISOTmp)            
				{                
					break;            
				}            
				u32ISOTmp >>= 1;    
				
			}


			int globa_value=128;
			if(__check_wdr_state())	
			{
				globa_value = jv_IQ_AgcTableCalculate_v2_u32(OV4689_WDR_3D_NR_Tbl,u8Index,u32ISO);			
				HI_MPI_VPSS_GetGrpParam(0, &pstVpssParam);
				pstVpssParam.s32GlobalStrength = globa_value;
				HI_MPI_VPSS_SetGrpParam(0, &pstVpssParam);

				
			}
			else
			{
				//globa_value = jv_IQ_AgcTableCalculate_v2_u32(OV4689_LINE_3D_NR_Tbl,u8Index,u32ISO);
				VPSS_GRP_PARAM_V2_S pstVpssParamV2;
				HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
				pstVpssParamV2.Luma_TF_Strength = 14;
				pstVpssParamV2.Chroma_SF_Strength =jv_IQ_AgcTableCalculate_v2_u8(OV4689_Chr_SF_Tbl, u8Index, u32ISO);
				pstVpssParamV2.Luma_MotionThresh =jv_IQ_AgcTableCalculate_v2_u32(OV4689_Luma_TF_Tbl, u8Index, u32ISO);
				pstVpssParamV2.Luma_SF_StillArea =jv_IQ_AgcTableCalculate_v2_u8(OV4689_Luma_SF_STILL_Tbl, u8Index, u32ISO);
				pstVpssParamV2.Luma_SF_MoveArea =jv_IQ_AgcTableCalculate_v2_u8(OV4689_Luma_SF_MOVE_Tbl, u8Index, u32ISO);

				pstVpssParamV2.Chroma_TF_Strength = 0;
				if(u32ISO>=800)
					pstVpssParamV2.Chroma_TF_Strength = 8;
				if(u32ISO>=1600)
					pstVpssParamV2.Chroma_TF_Strength = 10;
				
				if(u32ISO>=2400)
					pstVpssParamV2.Chroma_TF_Strength = 12;

			
			
				HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
			}

			/*
			if(u32ISO>4000)//22000)
			{

				VPSS_GRP_PARAM_V2_S pstVpssParamV2;
				HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
				pstVpssParamV2.Luma_TF_Strength = 13;
				pstVpssParamV2.Chroma_SF_Strength =160;
				pstVpssParamV2.Luma_MotionThresh =300;
				pstVpssParamV2.Luma_SF_StillArea =64;
				pstVpssParamV2.Luma_SF_MoveArea =12;
				
				pstVpssParamV2.Chroma_TF_Strength = 12;
				HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}
			if(u32ISO>5000)//22000)
			{

				VPSS_GRP_PARAM_V2_S pstVpssParamV2;
				HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
				pstVpssParamV2.Luma_TF_Strength = 13;
				pstVpssParamV2.Chroma_SF_Strength =160;
				pstVpssParamV2.Luma_MotionThresh =350;
				pstVpssParamV2.Luma_SF_StillArea =64;
				pstVpssParamV2.Luma_SF_MoveArea =12;
			
				pstVpssParamV2.Chroma_TF_Strength = 12;
				HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}
			if(u32ISO>6000)//22000)
			{

				VPSS_GRP_PARAM_V2_S pstVpssParamV2;
				HI_MPI_VPSS_GetGrpParamV2(0,&pstVpssParamV2);
				pstVpssParamV2.Luma_TF_Strength = 13;
				pstVpssParamV2.Chroma_SF_Strength =160;
				pstVpssParamV2.Luma_MotionThresh =400;
				pstVpssParamV2.Luma_SF_StillArea =64;
				pstVpssParamV2.Luma_SF_MoveArea =12;
			
				pstVpssParamV2.Chroma_TF_Strength = 12;
				HI_MPI_VPSS_SetGrpParamV2(0,&pstVpssParamV2);
				
			}*/


			
			if(__check_wdr_state())
           	{
           		int drcStrength = 0;
				if(u32ISO<=800)			
					drcStrength = 128;
				else if(u32ISO>=1100&&u32ISO<=1300)
					drcStrength = 108;
				else if(u32ISO >=1450)
					drcStrength = 78;

				if(drcStrength > 0)
				{
           			ISP_DRC_ATTR_S pstDRCAttr;
					HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
					pstDRCAttr.bEnable = HI_TRUE;
					pstDRCAttr.stAuto.u32Strength =drcStrength;
					HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);
				}
           	}
			else
			{
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				if(evBias>0)
					stExpAttr.stAuto.u16EVBias = evBias;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			}

			if(u32ISO<=800)
			{
				ISP_DP_ATTR_S  pstDPAttr;
				HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
				pstDPAttr.stDynamicAttr.bEnable =HI_TRUE;
				pstDPAttr.stDynamicAttr.u16Slope=500;
				pstDPAttr.stDynamicAttr.u16Thresh= 128;
				HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);
			}
			else if(u32ISO<=1800)
			{
				ISP_DP_ATTR_S  pstDPAttr;
				HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
				pstDPAttr.stDynamicAttr.bEnable =HI_TRUE;
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh= 128;				
				HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);
			}
			else
			{
				ISP_DP_ATTR_S  pstDPAttr;
				HI_MPI_ISP_GetDPAttr(IspDev,&pstDPAttr);
				pstDPAttr.stDynamicAttr.bEnable =HI_TRUE;
				pstDPAttr.stDynamicAttr.u16Slope=1024;
				pstDPAttr.stDynamicAttr.u16Thresh= 128;							
				HI_MPI_ISP_SetDPAttr(IspDev,&pstDPAttr);

			}

		}

		u32ISOLast=u32ISO;
 
    }
    return NULL;

}

static void isp_helper_init(void)
{
	static pthread_t thread;
	pthread_create(&thread, NULL, thread_isp_helper, NULL);
}
void isp_helper_reload(void)
{
    isp_helper_live=1;
    sleep(1);
    isp_helper_init();
}

int jv_msensor_set_callback(jv_msensor_callback_t callback)
{
	if(sensor_callback == NULL)
	{
		sensor_callback = callback;
		return 0;
	}
	else if(sensor_callback == callback)
	{
		return 0;
	}
	return -1;
}


