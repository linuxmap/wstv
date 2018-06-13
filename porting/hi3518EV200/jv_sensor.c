#include <jv_common.h>
#include <utl_timer.h>
#include "hicommon.h"
#include <jv_sensor.h>
#include"3518_isp.h"
#include <mpi_ae.h>
#include <mpi_awb.h>
#include <jv_gpio.h>
#include <jv_mdetect.h>
#include <mpi_af.h>
#include "utl_common.h"


#define ADC_READ	1

#define MAX_ISO_TBL_INDEX 15
#define LIGHT_AE_NODE_MAX 11


static WDR_MODE_E jv_wdr_state =0;
static int jv_wdr_switch_running=0;


BOOL bCheckNightDayUsingAE =FALSE; //软光敏开关


static BOOL StarLightEnable = FALSE;
static BOOL bAeNight=FALSE;
static JV_SENSOR_STATE SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
static pthread_mutex_t isp_daynight_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t thread;

static int FULL_FRAMERATE_RATIO = 16 ;//16  //全帧模式下修改为20帧 


static JV_EXP_MODE_e jv_ae_strategy_mode[2] = {JV_EXP_AUTO_MODE,JV_EXP_AUTO_MODE};
//HI_U32 ov9732_gamma = 1;//1,正常照度，2，低照度
//extern HI_U16 u16Gamma_ov9732_day[257];
//extern HI_U16  V200_LINE_DAY_LV1_GAMMA[257];

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
//不一定是晶域光敏，暂时这样定义
static light_v light_v_jyals_list[] =
{
	{ 0,  18 },//2lux
	{ 1,  21 },//2.5lux
	{ 2,  30 },//3lux
	{ 3,  39 },//4lux
	{ 4,  48 },	//5lux
	{ 5,  57 },//6lux
	{ 6,  75 },//8lux
	{ 7,  84 },//9lux
	{ 8,  93 },//10lux
	{ 9,  110 },//12lux
	{ 10, 123 },//14lux
	{ 11, 132 },//161 12 13 14没有实际意义
	{ 12, 140 },//18lux
	{ 13, 146 },//20lux
	{ 14, 148 }	//20lux
};


static HI_U32 u32EnCoeff = 1;


struct offset_s BR_Gain_Offset_HC8A_OV2735[] = {

		{0,0,64*800},
		{2,2,64*1400},
		{4,3,64*2000},
		{5,5,64*5000*64},
}; 

int jv_sensor_set_max_vi_20fps(BOOL Enable)
{
	if(Enable)
		FULL_FRAMERATE_RATIO = 20;////设置该值时，vi最大帧频为20帧
	else 
		FULL_FRAMERATE_RATIO =16;

	return 0;
}

int jv_sensor_cut_trigger(BOOL bStatus)
{
	jv_mdetect_silence_callback(bStatus);
	return 0;
};

typedef enum{
	CUT_TRIGGER_TYPE_TO_DAY_PRE,
	CUT_TRIGGER_TYPE_TO_DAY,
	CUT_TRIGGER_TYPE_TO_NIGHT,
	CUT_TRIGGER_TYPE_FPS_CHANGE,
	CUT_TRIGGER_TYPE_SYS_INIT,
	CUT_TRIGGER_TYPE_TO_AE_CHANGE,
	CUT_TRIGGER_TYPE_MAX
}CUT_TRIGGER_TYPE;

static CUT_TRIGGER_TYPE current_trigger_type = CUT_TRIGGER_TYPE_MAX;
static BOOL last_cut_status = 0xff;
static int cut_trigger_delay = 0;
int isp_cut_trigger_inner(BOOL bStatus,CUT_TRIGGER_TYPE type)
{
	current_trigger_type  = type;
	cut_trigger_delay  =0;

	if(last_cut_status != bStatus)
	{
		last_cut_status = bStatus ;
		jv_sensor_cut_trigger(bStatus);
	}
	
	
	return 0;
}




Soft_Light_Sensitive_t  Soft_Light_Sensitive_V200_HC8A_OV2735 = {
	
	.Max_Row = 15,
	.Max_Col = 17,
	.Statistical_Accuracy = 16,
	
	.Ratio_Rg_Ir = 268,
	.Ratio_Bg_Ir = 264,
    
	.Ratio_Rg_Natural_Light_Max = 206,
	.Ratio_Rg_Natural_Light_Min = 170,
	.Ratio_Bg_Natural_Light_Max = 183,
	.Ratio_Bg_Natural_Light_Min = 116,
	
    
	.Ratio_Rg_Natural_Light_Typcial = 199,	
	.Ratio_Bg_Natural_Light_Typical = 153,
	
	.Ratio_Rg_A_Light = 301,	
	.Ratio_Bg_A_Light = 201,
	
	.Cut_Ratio_A_Light = 11,
	.Cut_Ratio_Natural_Light_Typical = 82,
	.Ratio_G_To_L = 81,
	
	.R_Limit_Value_Max = 3000,
	.G_Limit_Value_Max = 3000,
	.B_Limit_Value_Max = 3000,
	.R_Limit_Value_Min = 128,
	.G_Limit_Value_Min = 128, 
	.B_Limit_Value_Min = 128, 
	
	.BR_Gain_Offset = BR_Gain_Offset_HC8A_OV2735,
	.BR_Gain_Offset_Num = 4
};


Soft_Light_Sensitive_t * pSoft_Light_Sensitive = &Soft_Light_Sensitive_V200_HC8A_OV2735;


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
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,4,6,6,6,6,6,6,6,4,4,1,1,1},
		{1,1,1,1,4,4,4,4,4,4,4,4,4,1,1,1,1},
		{1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},


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
const HI_U8 AE_WEIGHT_CENTER_DAY_OV2735[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,4,4,4,4,4,4,4,1,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,6,6,6,6,6,6,6,6,6,4,1,1,1},
		{1,1,1,4,4,6,6,6,6,6,6,6,4,4,1,1,1},
		{1,1,1,1,4,4,4,4,4,4,4,4,4,1,1,1,1},
		{1,1,1,1,4,4,4,4,4,4,4,4,4,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},


};

const HI_U8 AE_WEIGHT_DAY_V1[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,1,5,5,5,5,5,5,5,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

};

const HI_U8 AE_WEIGHT_NIGHT_V1[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,1,6,6,6,6,6,6,6,6,6,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,1,6,6,6,6,6,6,6,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

};


static  light_ae_node light_ae_list_ov2710[]=
{
	{ 2654*36*64, 30,TRUE},   //灵敏度为0,最晚切换到夜视

	{ 2654*35*64, 35,TRUE},  //灵敏度1

	{ 2654*35*64, 40,TRUE},  //灵敏度2

	{ 2654*32*64, 40,TRUE},   //灵敏度3

	{ 2654*32*64, 45,TRUE},  //灵敏度4

	{ 2654*32*64, 46,TRUE},  //灵敏度5

	{ 2654*30*64, 48,TRUE}, //灵敏度6

	{ 2654*30*64, 48,TRUE}, //灵敏度7

	{ 2654*25*64, 50,TRUE}, //灵敏度8
	{ 2654*25*64, 52,TRUE}, //灵敏度9
	{ 2654*25*64, 56,TRUE} //灵敏度10

};

static  light_ae_node light_ae_list_sc2235[]=
{
	{ 2812*100*64, 20,TRUE},   //灵敏度为0,最晚切换到夜视

	{ 2812*100*64, 30,TRUE},  //灵敏度1

	{ 2812*80*64, 45,TRUE},  //灵敏度2

	{ 2812*60*64, 45,TRUE},   //灵敏度3

	{ 2812*43*64, 50,TRUE},  //灵敏度4  //DEFFF  降帧后12fps是2812.5行

	{ 2812*35*64, 52,TRUE},  //灵敏度5

	{ 2812*30*64, 52,TRUE}, //灵敏度6

	{ 2812*22*64, 54,TRUE}, //灵敏度7

	{ 2812*18*64, 56,TRUE}, //灵敏度8

	{ 1687*20*64, 50,FALSE}, //灵敏度9

	{ 1687*20*64, 55,FALSE} //灵敏度10

};

static  light_ae_node light_ae_list_ov2735[]=
{
	{ 3262*50*64, 20,TRUE},   //灵敏度为0,最晚切换到夜视

	{ 3262*50*64, 30,TRUE},  //灵敏度1

	{ 3262*40*64, 45,TRUE},  //灵敏度2

	{ 3262*30*64, 45,TRUE},   //灵敏度3

	{ 3262*30*64, 50,TRUE},  //灵敏度4  //DEFFF

	{ 3262*25*64, 52,TRUE},  //灵敏度5

	{ 3262*25*64, 52,TRUE}, //灵敏度6

	{ 3262*25*64, 54,TRUE}, //灵敏度7

	{ 3262*20*64, 56,TRUE}, //灵敏度8
	
	{ 3262*20*64, 56,TRUE}, //灵敏度9
	{ 3262*20*64, 60,TRUE} //灵敏度10

};

static  light_ae_node light_ae_list_mn34227[]=
{
	{ 3377*250*64, 10,TRUE},   //灵敏度为0,最晚切换到夜视

	{ 3377*250*64, 20,TRUE},  //灵敏度1

	{ 3377*250*64, 25,TRUE},  //灵敏度2

	{ 3377*250*64, 30,TRUE},   //灵敏度3

	{ 3377*250*64, 35,TRUE},  //灵敏度4  //DEFFF

	{ 3377*220*64, 40,TRUE},  //灵敏度5

	{ 3377*220*64, 45,TRUE}, //灵敏度6

	{ 3377*220*64, 50,TRUE}, //灵敏度7

	{ 3377*220*64, 50,TRUE}, //灵敏度8
	
	{ 3377*220*64, 50,TRUE}, //灵敏度9
	{ 3377*220*64, 50,TRUE} //灵敏度10

};




static  light_ae_node light_ae_list_ov9750[]=
{
	{ 4748*50*64, 20,TRUE},   //灵敏度为0,最晚切换到夜视

	{ 4748*50*64, 30,TRUE},  //灵敏度1

	{ 4748*45*64, 45,TRUE},  //灵敏度2

	{ 4748*42*64, 45,TRUE},   //灵敏度3

	{ 4748*42*64, 50,TRUE},  //灵敏度4  //DEFFF

	{ 4748*36*64, 52,TRUE},  //灵敏度5

	{ 4748*36*64, 52,TRUE}, //灵敏度6

	{ 4748*32*64, 54,TRUE}, //灵敏度7

	{ 4748*32*64, 56,TRUE}, //灵敏度8
	
	{ 4748*32*64, 56,TRUE}, //灵敏度9
	{ 4748*30*64, 60,TRUE} //灵敏度10

};





static  light_ae_node light_ae_list_ov9732[]=
{
	{ 970*31*64, 30,FALSE},   //灵敏度为0,最晚切换到夜视

	{ 970*31*64, 35,FALSE},  //灵敏度1

	{ 970*31*64, 38,FALSE},  //灵敏度2

	{ 970*31*64, 40,FALSE},   //灵敏度3

	{ 970*31*64, 45,FALSE},  //灵敏度4

	{ 970*30*64, 46,FALSE},  //灵敏度5

	{ 970*30*64, 48,FALSE}, //灵敏度6

	{ 970*30*64, 48,FALSE}, //灵敏度7

	{ 970*30*64, 50,FALSE}, //灵敏度8
	{ 970*28*64, 52,FALSE}, //灵敏度9
	{ 970*28*64, 56,FALSE} //灵敏度10

};

static void isp_helper_init(void);
void WDR_Switch(int mode);

static int isp_helper_live=0;
static BOOL bLowNow = FALSE;
static BOOL bNightMode = FALSE;
static pthread_mutex_t low_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t isp_wdr_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int sensorType;

//static int LowFrameRate =25;
static int ir_cut_staus=0;
static int light_value = 0;
static  unsigned int LightAeIndex  = 4; 
int _gpio_cutcheck_redlight_need_change()
{
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

void jv_sensor_switch_ircut_inner(BOOL bNight)
{
	if (bNight)
	{
		//开灯
		//if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			//jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 1);
		
		if(strcmp(hwinfo.devName, "E2-5013W") == 0)
		{
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
		}
		else
		{
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
		}
		//ir_cut_staus = 1;
	}
	else
	{
		//关灯
		//if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			//jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 0);
			
		if(strcmp(hwinfo.devName, "E2-5013W") == 0)
		{
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
		}
		else
		{
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
		}
		//ir_cut_staus = 0;
	}
}


void jv_sensor_set_ircut(BOOL bNight)
{
	if (bNight)
	{
		//开灯
		if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 1);

		if (higpios.cutnight.group != -1 && higpios.cutnight.bit != -1 &&
			higpios.cutday.group != -1 && higpios.cutday.bit != -1)
		{
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
			ir_cut_staus = 1;
		}
	}
	else
	{
		//关灯
		if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 0);

		if (higpios.cutnight.group != -1 && higpios.cutnight.bit != -1 &&
			higpios.cutday.group != -1 && higpios.cutday.bit != -1)
		{
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
			ir_cut_staus = 0;
		}
	}
}

int jv_msensor_set_callback(jv_msensor_callback_t callback)
{
	return 0;
}

int fd_adc = -1;
 void jv_adc_init(void)
{
	int dir = 0;
  	fd_adc = open("/dev/hi_adc", O_RDWR);
	if (fd_adc == -1)
   	{
	   printf("open adc mudule error\n");
   	}
}

int jv_sensor_get_adc_high_val()
{
	return light_v_jyals_list[LightAeIndex+4].voltage;
}

int jv_sensor_get_adc_low_val()
{
	return light_v_jyals_list[LightAeIndex].voltage;
}

int jv_sensor_get_adc_val()

{
	int value = 0, value_s = 0;
	int i = 0;

	if (fd_adc == -1)
	{
		printf("open adc mudule error\n");
		return -1;
	}

	for(i = 0; i < 20; i++)
	{
		ioctl(fd_adc, ADC_READ, (unsigned long)&value);
		//printf("value:%d\n",value);
		value_s += value;
		usleep(1);
	}
	value = value_s / 20;

	//printf("get_adc:%d\n",value);
	return value;
}

int jv_adc_read(void)
{
	int value = 0;
	static char last_type = IRCUT_DAY;
	int high_value = jv_sensor_get_adc_high_val();
	int low_value = jv_sensor_get_adc_low_val();

	value = jv_sensor_get_adc_val();
	//printf("value:%d\n",value);
	if (value < 0)
	{
		return last_type;
	}

	if (value > high_value)
	{
		last_type = IRCUT_DAY;
	}
	else if (value <= low_value)
	{
		last_type = IRCUT_NIGHT;
	}
	//printf("last:%d\n",last_type);
	return last_type;
}

int  jv_sensor_get_ircut_staus()
{
	return ir_cut_staus;
}
int sensor_ircut_init(void)
{
	int ret;
	_gpio_cutcheck_redlight_change();

	//设置sensor白天模式,
	jv_sensor_set_ircut(FALSE);
	jv_sensor_nocolour(0, FALSE);
	
	if(hwinfo.sensor==SENSOR_OV9732 || (hwinfo.sensor == SENSOR_OV2710 && hwinfo.bHomeIPC))	
		bCheckNightDayUsingAE =TRUE; //当为9732采用软光敏
	if(hwinfo.sensor==SENSOR_OV2735&& hwinfo.bHomeIPC)
		bCheckNightDayUsingAE =TRUE;
	if(hwinfo.sensor==SENSOR_OV9750&& hwinfo.bHomeIPC)
		bCheckNightDayUsingAE =TRUE;
	if(hwinfo.sensor==SENSOR_MN34227)
		bCheckNightDayUsingAE =TRUE;
	if(hwinfo.sensor==SENSOR_SC2235&& hwinfo.bHomeIPC)
		bCheckNightDayUsingAE =TRUE;

	jv_adc_init();
	isp_helper_init();
	return 0;
}
static void isp_helper_deinit(void);

int sensor_ircut_deinit(void)
{
	isp_helper_deinit();
	return 0;
}
int jv_sensor_daynight_ae_set(BOOL bNight)
{
#if 0
	ISP_EXPOSURE_ATTR_S pstExpAttr;
	if(bNight)
	{
		if(jv_ae_strategy_mode[1] == JV_EXP_AUTO_MODE)//自动曝光
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_NIGHT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=10;
            pstExpAttr.stAuto.u8Speed = 120;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[1] == JV_EXP_HIGH_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);//高光优先
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=40;
            pstExpAttr.stAuto.u8Speed = 120;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[1] == JV_EXP_LOW_LIGHT_MODE)//低光优先
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=16;
            pstExpAttr.stAuto.u8Speed = 120;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
	}
	else
	{
		if(jv_ae_strategy_mode[0] == JV_EXP_AUTO_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=0;
			pstExpAttr.stAuto.u8MaxHistOffset=0;
            pstExpAttr.stAuto.u8Speed = 60;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);		
		}
		else if(jv_ae_strategy_mode[0] == JV_EXP_HIGH_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=20;
            pstExpAttr.stAuto.u8Speed = 60;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[0] == JV_EXP_LOW_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=16;
            pstExpAttr.stAuto.u8Speed = 60;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
	}
#else
	ISP_EXPOSURE_ATTR_S pstExpAttr;
	isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_AE_CHANGE);
	if(bNight)
	{
		if(jv_ae_strategy_mode[1] == JV_EXP_AUTO_MODE)//自动曝光
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_NIGHT_V1,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=12;
           // pstExpAttr.stAuto.u8Speed = 100;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[1] == JV_EXP_HIGH_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);//高光优先
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_NIGHT_V1,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=40;
            //pstExpAttr.stAuto.u8Speed = 100;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[1] == JV_EXP_LOW_LIGHT_MODE)//低光优先
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_NIGHT_V1,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=16;
           // pstExpAttr.stAuto.u8Speed = 100;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
	}
	else
	{
		if(jv_ae_strategy_mode[0] == JV_EXP_AUTO_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_DAY_V1,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=10;
			//if(hwinfo.sensor == SENSOR_OV2735)
				//memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DAY_OV2735,255*1);	
            //pstExpAttr.stAuto.u8Speed = 64;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);		
		}
		else if(jv_ae_strategy_mode[0] == JV_EXP_HIGH_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=40;
           // pstExpAttr.stAuto.u8Speed = 64;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[0] == JV_EXP_LOW_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=16;
           // pstExpAttr.stAuto.u8Speed = 64;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
	}

#endif
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

int jv_sensor_set_exp_params(int sensorid,int value)
{
	
	return 0;
}


static BOOL bCheckNight =FALSE;
static HI_U8 CheckWaitTimes=0;

int jv_sensor_set_nightmode(BOOL bNight)
{


	ISP_EXPOSURE_ATTR_S pstExpAttr;
	HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
    //pstExpAttr.stAuto.u8Speed = 200;//200;
    if(hwinfo.sensor ==SENSOR_OV2735 || hwinfo.sensor == SENSOR_SC2235)
		pstExpAttr.stAuto.u8Speed  =120;
	else if(hwinfo.sensor ==SENSOR_OV9750)
		pstExpAttr.stAuto.u8Speed  =120;
	else
		pstExpAttr.stAuto.u8Speed  =100;
	pstExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 2;
	pstExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
	HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
	
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
	jv_sensor_daynight_ae_set(bNightMode);

	CheckWaitTimes =0;
	bCheckNight = TRUE;
	
	return 0;
}



static BOOL  bAdcNight= FALSE;
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
	static BOOL bLastSatus =0xff;
   int mode=1;

   if(bCheckNightDayUsingAE)
   		return bAeNight;
   

	switch(hwinfo.ir_sw_mode)
	{
		case IRCUT_SW_BY_GPIO:
		{
			mode = jv_gpio_read(higpios.cutcheck.group, higpios.cutcheck.bit);
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
		case IRCUT_SW_BY_ADC0:
		case IRCUT_SW_BY_ADC1:
		case IRCUT_SW_BY_ADC2:
		{
			if(hwinfo.sensor==SENSOR_OV2710&&StarLightEnable) 
			{
				return bAeNight;
			}
			
			mode = jv_adc_read();
			if(mode !=  bLastSatus)
			{
				isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_NIGHT);
				bLastSatus = mode;
			}
			//printf("****jv_sensor mode is %d\n",mode);
			return mode;
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
    Printf(">>> %s  sensorid:%d  ,antifog:%d", __FUNCTION__, sensorid, nValue);
	
	ISP_DEFOG_ATTR_S pstDefogAttr;
	HI_MPI_ISP_GetDeFogAttr(0, &pstDefogAttr);
	pstDefogAttr.bEnable=nValue;
	HI_MPI_ISP_SetDeFogAttr(0, &pstDefogAttr);   
	return 0;
}

/**
 *@brief set saturation
 *@param sensorid
 *@param saturation value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */

//int jv_sensor_light(int sensorid, int nValue)
//{
	//light_value=nValue;
//	return 0;
//}
int jv_sensor_light(int sensorid, int nValue)
{
	//light_value=nValue;
	if(nValue >= LIGHT_AE_NODE_MAX || nValue < 0)
		return 0;

	if(bCheckNightDayUsingAE)
	{
		pthread_mutex_lock(&low_frame_mutex);
		light_value  = 4;

		if( nValue < LightAeIndex )
		{
			LightAeIndex = nValue;
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			bAeNight =FALSE;
			isp_set_low_fps(16);
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
 *@brief set contrast
 *@param sensorid
 *@param contrast value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_contrast(int sensorid, int nValue)
{
	
    isp_ioctl(0,ADJ_CONTRAST,nValue);
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


int jv_sensor_set_fps(int frameRate)//针对16D需要增加该接口，否则会导致降帧问题
{
	int tp;
	tp = frameRate;
	if(sensorType == SENSOR_SC2235)
		tp = (frameRate>20)?20:frameRate;

	return isp_set_std_fps(frameRate);
	
}

BOOL jv_sensor_b_daynight_usingAE()
{
	return bCheckNightDayUsingAE;
}

int jv_sensor_low_frame_inner(int sensorid, BOOL bEnable)
{

	//pthread_mutex_lock(&low_frame_mutex);
	int tp;
	if (bEnable)
	{
		if(hwinfo.sensor==SENSOR_MN34227)
			tp = 34;
		else if(sensorType == SENSOR_SC2235)
			tp = 26;
		else	
			tp = 32;
	}
	else
		tp = FULL_FRAMERATE_RATIO;
	isp_set_low_fps(tp);//fps=ViFps*16/tp;
	
	//pthread_mutex_unlock(&low_frame_mutex);
	return 0;
}

BOOL bNightLowframe =FALSE;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
int jv_sensor_low_frame(int sensorid, int bEnable)
{
   if(bNightMode)
		bNightLowframe =bEnable;//夜视模式下才允许改变
	return 0;

}
/*
int jv_sensor_low_frame(int sensorid, int bEnable)
{

	pthread_mutex_lock(&low_frame_mutex);
	int tp;
	if (bEnable)
		tp = 32;	
	else
		tp = 16;
	isp_set_low_fps(tp);//fps=ViFps*16/tp;
	
	if(bNightMode  ==FALSE)
	{
		SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
		bAeNight =FALSE;
	}
	bNightLowframe =bEnable;
	pthread_mutex_unlock(&low_frame_mutex);

	return 0;

}*/




int jv_sensor_auto_lowframe_enable(int sensorid, int bEnable)
{
	return 0;
}
/**
 *@brief set the sensor the daynight mode
 *@param sensorid
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */

static unsigned int DayNightMode = 0xff;
static BOOL bStarSwitched =FALSE;
int jv_sensor_set_daynight_mode(int sensorid, int mode)
{
	if(DayNightMode == mode)
		return 0; 

	if(bCheckNightDayUsingAE)
	{
		if(mode==DAYNIGHT_AUTO)
		{
			pthread_mutex_lock(&low_frame_mutex);
			isp_set_low_fps(16);
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			bAeNight =FALSE;
			StarLightEnable =TRUE;
			bStarSwitched =TRUE;
			pthread_mutex_unlock(&low_frame_mutex); 
		}
		else if(mode==DAYNIGHT_ALWAYS_DAY)
		{
			pthread_mutex_lock(&low_frame_mutex);
			isp_set_low_fps(16);
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			bAeNight =FALSE;
			StarLightEnable =TRUE;
			bStarSwitched =TRUE;
			pthread_mutex_unlock(&low_frame_mutex); 
		}
		else
		{
			pthread_mutex_lock(&low_frame_mutex);
			isp_set_low_fps(16);// 屏蔽掉是因为自动模式下黑白然后一直黑白变成不降帧了
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
			bAeNight =FALSE;
			StarLightEnable =FALSE;
			bStarSwitched =TRUE;
			pthread_mutex_unlock(&low_frame_mutex); 

		}
	}
   	DayNightMode =mode;
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
		fps = isp_get_fps();
		*current =(float)( (int)(fps+0.5));
	}
	
	blow = isp_check_low_fps();
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
#if 1
	ISP_DEV IspDev = 0;
	ISP_PUB_ATTR_S stPubAttr;
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
		printf("ae attr: [%d, %d]\n", stExpAttr.stAuto.stExpTimeRange.u32Max, stExpAttr.stAuto.stExpTimeRange.u32Min);
		CHECK_RET(HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr));
	}
	else
	{
	}
#endif
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


int jv_sensor_wdr_mode_set(WDR_MODE_E mode)
{

	//return isp_wdr_mode_set(mode);int jv_wdr_state =0;
	pthread_mutex_lock(&isp_wdr_mutex); 
	if(jv_wdr_switch_running)
	{
		pthread_mutex_unlock(&isp_wdr_mutex);
		return 0;
	}
	usleep(200000);
	if(mode)
		jv_wdr_state =1;
	else
		jv_wdr_state =0;
	isp_wdr_init(jv_wdr_state);
	pthread_mutex_unlock(&isp_wdr_mutex);

	return 0;
}
int jv_sensor_wdr_mode_get()
{
	int mode;
	pthread_mutex_lock(&isp_wdr_mutex); 
	if(jv_wdr_switch_running)
	{
		pthread_mutex_unlock(&isp_wdr_mutex);
		return -1;

	}
	else
	{
		mode=jv_wdr_state;
		pthread_mutex_unlock(&isp_wdr_mutex);


	}
	return mode;
}

static int ae_route_cnt=0;
int jv_sensor_wdr_switch(int mode)
{
	
	pthread_mutex_lock(&isp_wdr_mutex);
	if(mode==jv_wdr_state)
	{
		
		printf("wdr mode has been set,not need to switch!!!!!!!\n");
		pthread_mutex_unlock(&isp_wdr_mutex);
		return 0;
	}
	else 
	{
		if(mode&&bNightMode)
		{
			printf("night mode !!! <<wdr mode aborted>>\n");
			pthread_mutex_unlock(&isp_wdr_mutex);
			return 0;

		}
		
		int tick=0;
		int allow=0;
		
		
			
			if(jv_wdr_switch_running)
			{
				pthread_mutex_unlock(&isp_wdr_mutex);
				
				while(1)
				{
					usleep(300000);
					pthread_mutex_lock(&isp_wdr_mutex);
					if(!jv_wdr_switch_running)
					{
						
						jv_wdr_switch_running=1;
						allow=1;
						if(jv_wdr_state==mode)
						{
							allow=0;
							jv_wdr_switch_running=0;
						}
						pthread_mutex_unlock(&isp_wdr_mutex);
						break;
						

					}
					pthread_mutex_unlock(&isp_wdr_mutex);
						
					tick++;
					if(tick>=4)
					{
						allow=0;
						printf("timeout for wdr switch\n");
						break;
					}

				}
				
			}
			else
			{
				allow=1;
				jv_wdr_switch_running =1;
				if(jv_wdr_state==mode)
				{
					allow=0;
					jv_wdr_switch_running=0;
				}
				pthread_mutex_unlock(&isp_wdr_mutex);
			}
		

		if(!allow)
			return -1;
		
		WDR_Switch(mode);
		usleep(800000);	
		pthread_mutex_lock(&isp_wdr_mutex);
		isp_wdr_init(mode);
		jv_wdr_switch_running =0;
		jv_wdr_state =mode;
		ae_route_cnt =0;
		pthread_mutex_unlock(&isp_wdr_mutex);

		
		printf("wdr mode %d switch success!!!!!!\n",jv_wdr_state);
		
		return  0;


	}


}

int jv_sensor_check_live()
{
	return 0;
}

int jv_sensor_rotate_get(JVRotate_e *jvRotate)
{
	HI_S32 s32Ret;
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = 1;
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
	ROTATE_E newRotate = (ROTATE_E)jvRotate;
	int i;
	VPSS_CHN VpssChn[3] = {0,1,2};
	if(hwinfo.encryptCode == ENCRYPT_200W)
	{
		VpssChn[0] = 1;
		VpssChn[1] = 2;
		VpssChn[2] = 3;
	}
	for(i=0; i<HWINFO_STREAM_CNT; i++)
	{
		s32Ret = HI_MPI_VPSS_SetRotate(VpssGrp, VpssChn[i], newRotate);
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

HI_U32 jv_IQ_AgcTableCalculate(const HI_S32 *s32Array, HI_U8 u8Index, HI_U32 u32ISO)
{    
	HI_S32 u32Data1, u32Data2;    
	HI_S32 u32Range, u32ISO1;    
	u32Data1 = s32Array[u8Index];    
	u32Data2 = (MAX_ISO_TBL_INDEX == u8Index)? s32Array[u8Index]: s32Array[u8Index + 1];       
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


HI_U32 NoiseControlGainTbl[SHADING_MESH_NUM] =
{

	700,700,700,700,700,800,900,960,960,960,900,800,700,700,700,700,700,
	700,700,700,700,800,900,960,960,960,960,960,900,800,700,700,700,700,
	700,700,700,800,900,960,960,960,960,960,960,960,900,800,700,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,800,900,960,960,960,960,960,960,960,960,960,900,800,700,700,
	700,700,700,800,900,960,960,960,960,960,960,960,900,800,700,700,700,
	700,700,700,700,800,900,960,960,960,960,960,900,800,700,700,700,700,
	700,700,700,700,700,800,900,960,960,960,900,800,700,700,700,700,700


};

int jv_sensor_CheckDay_By_Sensor(HI_U8 lum,HI_U32 exposure,HI_U32 Night_To_Day_ThrLuma,HI_U32 Day_To_Night_ThrLuma,Soft_Light_Sensitive_t * Soft_Light_S)
{
	//return 0;
	if(NULL == Soft_Light_S)
	{
		printf("ERR:Soft Light Sensitive Parameter is NULL !\n");
		return -1;
	}
	static HI_U32 En_Bayer_R = 0,En_Bayer_G = 0;
	static HI_U32 En_Bayer_AE_R_lum = 0,En_Bayer_AE_G_lum = 0,En_Bayer_AE_B_lum = 0,After_To_Night_Bayer_AE_G_lum = 0,After_To_Night_Bayer_AE_R_lum = 0;
	static HI_U32 B_Turn_Day_Normal_Num = 0;
	static HI_U32 B_Turn_Day_ALight_Num = 0;
	static HI_U32 B_Turn_Night_Num = 0;
	int Lum_Effect_Block_Num = 0;//Soft_Light_S->Max_Row*Soft_Light_S->Max_Col*10;//=0;
	int Normal_Actual_Effect_Block_Num = 0;
	int ALight_Actual_Effect_Block_Num = 0;

	int row = 0;
	int col = 0;

	int Ratio_RG_En = 0;
	int Ratio_BG_En = 0;
	int Natural_Light_Lum_Typical	=	0;
	int Natural_Light_Lum_Max	=	0;
	int Natural_Light_Lum_Min	=	0;
	HI_U64 lum_tmp = 0;
	HI_U32 En_Lum = 0;
	
	int RG_Offset = 2;
	int BG_Offset = 2;
	static int bTurnNight = 0;
	
	if(bAeNight)
	{
		ISP_STATISTICS_S stStat;
		HI_MPI_ISP_GetStatistics(0, &stStat);

		int i = 0;
		for(i=0;i < Soft_Light_S->BR_Gain_Offset_Num;i++)
		{
			if(exposure < Soft_Light_S->BR_Gain_Offset[i].Exp)
				break;
		}
		RG_Offset = Soft_Light_S->BR_Gain_Offset[i].RG_Offset;
		BG_Offset = Soft_Light_S->BR_Gain_Offset[i].BG_Offset;
		
		for (row=0;row<Soft_Light_S->Max_Row;row++)
		{
			for(col=0;col<Soft_Light_S->Max_Col;col++)
			{	

				if(stStat.stWBStat.stBayerStatistics.au16ZoneAvgR[row][col]<Soft_Light_S->R_Limit_Value_Max && stStat.stWBStat.stBayerStatistics.au16ZoneAvgG[row][col]<Soft_Light_S->G_Limit_Value_Max &&  stStat.stWBStat.stBayerStatistics.au16ZoneAvgB[row][col] < Soft_Light_S->B_Limit_Value_Max  && \
					stStat.stWBStat.stBayerStatistics.au16ZoneAvgR[row][col]>Soft_Light_S->R_Limit_Value_Min && stStat.stWBStat.stBayerStatistics.au16ZoneAvgG[row][col]>Soft_Light_S->G_Limit_Value_Min && stStat.stWBStat.stBayerStatistics.au16ZoneAvgB[row][col] > Soft_Light_S->B_Limit_Value_Min)
				{
					
					Lum_Effect_Block_Num += 10;

					En_Bayer_AE_R_lum = stStat.stWBStat.stBayerStatistics.au16ZoneAvgR[row][col];
					En_Bayer_AE_G_lum = stStat.stWBStat.stBayerStatistics.au16ZoneAvgG[row][col];
					En_Bayer_AE_B_lum = stStat.stWBStat.stBayerStatistics.au16ZoneAvgB[row][col];

					
				
					//lum_tmp = ((HI_U64)En_Bayer_AE_R_lum*257L + (HI_U64)En_Bayer_AE_G_lum*504L + (HI_U64)En_Bayer_AE_B_lum*98L + 16L*256L*1000L )/1000L*(HI_U64)u32EnCoeff*64L;
					lum_tmp = (HI_U64)En_Bayer_AE_G_lum*(HI_U64)u32EnCoeff*64L*(HI_U64)(Soft_Light_S->Ratio_G_To_L)/100L;
					//lum_tmp = (HI_U64)lum*256L*(HI_U64)u32EnCoeff*64L;
					En_Lum = (HI_U32)(lum_tmp/(HI_U64)exposure);
					Ratio_RG_En =  En_Bayer_AE_R_lum * 256 / En_Bayer_AE_G_lum;
					Ratio_BG_En =  En_Bayer_AE_B_lum * 256 / En_Bayer_AE_G_lum;
					
//////////////////////////////////////////////////////////////////////////

					if(Ratio_RG_En >= Soft_Light_S->Ratio_Rg_Ir+6)
					{
						Natural_Light_Lum_Typical	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_A_Light) * Soft_Light_S->Cut_Ratio_A_Light/100;
						if( Natural_Light_Lum_Typical > Night_To_Day_ThrLuma )
						{
							ALight_Actual_Effect_Block_Num += 10;
							//printf("env natrual light is A  \n");
						}
					}
					///
					if((Ratio_RG_En < Soft_Light_S->Ratio_Rg_Ir - RG_Offset) && (Ratio_BG_En < Soft_Light_S->Ratio_Bg_Ir - BG_Offset))
					{
						Natural_Light_Lum_Typical	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_Natural_Light_Typical) * Soft_Light_S->Cut_Ratio_Natural_Light_Typical/100;
						Natural_Light_Lum_Max	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_Natural_Light_Max) * Soft_Light_S->Cut_Ratio_Natural_Light_Typical/100;
						Natural_Light_Lum_Min	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_Natural_Light_Min) * Soft_Light_S->Cut_Ratio_Natural_Light_Typical/100;
						
						if( Natural_Light_Lum_Min > Night_To_Day_ThrLuma )
						{
							Normal_Actual_Effect_Block_Num += 10;
							//printf("1row is %d,col is %d ,Night_To_Day_ThrLuma is %u,Natural_Light_Lum_Min is %u,Ratio_RG_En is %u,Ratio_BG_En is %u,En_Bayer_AE_R_lum %u, En_Bayer_AE_B_lum %u,En_Bayer_AE_G_lum %u ,Natural_Light_Lum_Typical is %u,Natural_Light_Lum_Min is %u,Natural_Light_Lum_Max is %u,Night_To_Day_ThrLuma is %u \n",row,col,Night_To_Day_ThrLuma,Natural_Light_Lum_Min,Ratio_RG_En,Ratio_BG_En,En_Bayer_AE_R_lum,En_Bayer_AE_B_lum,En_Bayer_AE_G_lum,Natural_Light_Lum_Typical,Natural_Light_Lum_Min,Natural_Light_Lum_Max,Night_To_Day_ThrLuma);
						}
						else if( Natural_Light_Lum_Typical > Night_To_Day_ThrLuma )
						{
							Normal_Actual_Effect_Block_Num += 6;
							//printf("2row is %d,col is %d ,,Night_To_Day_ThrLuma is %u,Natural_Light_Lum_Typical is %u,Ratio_RG_En is %u ,Ratio_BG_En is %u,En_Bayer_AE_R_lum %u, En_Bayer_AE_B_lum %u,En_Bayer_AE_G_lum %u ,Natural_Light_Lum_Typical is %u,Natural_Light_Lum_Min is %u,Natural_Light_Lum_Max is %u,Night_To_Day_ThrLuma is %u \n",row,col,Night_To_Day_ThrLuma,Natural_Light_Lum_Typical,Ratio_RG_En,Ratio_BG_En,En_Bayer_AE_R_lum,En_Bayer_AE_B_lum,En_Bayer_AE_G_lum,Natural_Light_Lum_Typical,Natural_Light_Lum_Min,Natural_Light_Lum_Max,Night_To_Day_ThrLuma);
						}
						else if( Natural_Light_Lum_Max > Night_To_Day_ThrLuma )
						{
							Normal_Actual_Effect_Block_Num += 1;
							//printf("3row is %d,col is %d ,Night_To_Day_ThrLuma is %u,Natural_Light_Lum_Max is %u,Ratio_RG_En is %u ,Ratio_BG_En is %u,En_Bayer_AE_R_lum %u, En_Bayer_AE_B_lum %u,En_Bayer_AE_G_lum %u ,Natural_Light_Lum_Typical is %u,Natural_Light_Lum_Min is %u,Natural_Light_Lum_Max is %u,Night_To_Day_ThrLuma is %u \n",row,col,Night_To_Day_ThrLuma,Natural_Light_Lum_Max,Ratio_RG_En,Ratio_BG_En,En_Bayer_AE_R_lum,En_Bayer_AE_B_lum,En_Bayer_AE_G_lum,Natural_Light_Lum_Typical,Natural_Light_Lum_Min,Natural_Light_Lum_Max,Night_To_Day_ThrLuma);
						}
						else
						{
							;
							//printf("5row is %d,col is %d ,Night_To_Day_ThrLuma is %u,Natural_Light_Lum_Max is %u,Ratio_RG_En is %u ,Ratio_BG_En is %u,En_Bayer_AE_R_lum %u, En_Bayer_AE_B_lum %u,En_Bayer_AE_G_lum %u ,Natural_Light_Lum_Typical is %u,Natural_Light_Lum_Min is %u,Natural_Light_Lum_Max is %u,Night_To_Day_ThrLuma is %u ,En_Lum is %u \n",row,col,Night_To_Day_ThrLuma,Natural_Light_Lum_Max,Ratio_RG_En,Ratio_BG_En,En_Bayer_AE_R_lum,En_Bayer_AE_B_lum,En_Bayer_AE_G_lum,Natural_Light_Lum_Typical,Natural_Light_Lum_Min,Natural_Light_Lum_Max,Night_To_Day_ThrLuma,En_Lum);
						}
					}
////////////////////////////////////////////////////////////////////////////////////////
				}
			}
			
		}
		if(Normal_Actual_Effect_Block_Num*3 > Lum_Effect_Block_Num)
			B_Turn_Day_Normal_Num ++;
		else
			B_Turn_Day_Normal_Num = 0; 
		
		if(ALight_Actual_Effect_Block_Num*3 > Lum_Effect_Block_Num)
			B_Turn_Day_ALight_Num ++;
		else
			B_Turn_Day_ALight_Num = 0; 
		
		if((B_Turn_Day_Normal_Num > 3) || (B_Turn_Day_ALight_Num > 3) )
		{
			printf("AE  enter day mode \n");
			B_Turn_Day_Normal_Num = 0; 	
			B_Turn_Day_ALight_Num = 0; 
			B_Turn_Night_Num = 0;
			bTurnNight = 0;
		}
		//printf("%s....%d................. Lum_Effect_Block_Num %d , NOomal_Actual_Effect_Block_Num %d    Alight_Actual_Effect_Block_Num %d...\n",__func__,__LINE__,Lum_Effect_Block_Num,Normal_Actual_Effect_Block_Num,ALight_Actual_Effect_Block_Num);
	}
	else
	{
		lum_tmp = (HI_U64)lum*pSoft_Light_Sensitive->Statistical_Accuracy*(HI_U64)u32EnCoeff*64L;
		En_Lum= (HI_U32)(lum_tmp/(HI_U64)exposure);
		if(En_Lum < Day_To_Night_ThrLuma )
		{
			B_Turn_Night_Num  ++;
		}
		else
			B_Turn_Night_Num = 0;
		
		if( B_Turn_Night_Num > 3 )
		{
			printf("AE  enter night mode \n");
			bTurnNight = 1;
			B_Turn_Night_Num = 0;
			B_Turn_Day_Normal_Num = 0; 	
			B_Turn_Day_ALight_Num = 0; 
		}
		
	}
	return bTurnNight;
}



static void *thread_isp_helper(void *param)
{  
    Printf("isp adjust thread\n");
    ISP_DEV IspDev = 0;
	HI_U8 stAvm = 0;
    HI_U32 u32ISO, u32ISOTmp;
	HI_U32 u32ISOLast=0;
    unsigned long sensor;
	HI_U8 u8Index;
	isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_SYS_INIT);
	pthreadinfo_add((char *)__func__);

	sleep(6); //等待ISP稳定
	ISP_EXP_INFO_S stExpInfo;
    isp_ioctl(0, GET_ID,(unsigned long)&sensor);
    printf("get sensor ID :%ld\n",sensor);
	
	VI_DCI_PARAM_S pstDciParam;
	HI_MPI_VI_GetDCIParam(0, &pstDciParam);
	pstDciParam.bEnable=HI_FALSE;
	HI_MPI_VI_SetDCIParam(0, &pstDciParam);

	ISP_ANTI_FALSECOLOR_S  pstAntiFC;
	HI_MPI_ISP_GetAntiFalseColorAttr(0,&pstAntiFC);
	pstAntiFC.bEnable =HI_FALSE;
	HI_MPI_ISP_SetAntiFalseColorAttr(0,&pstAntiFC); //暂时关闭去伪彩模块，该模块开启会导致颜色丢失

	VPSS_NR_PARAM_V1_S stNrParamV1;
	VPSS_NR_PARAM_U punNrParam;
	
	HI_S32  YSF_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  YTF_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  CSF_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  YPK_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  TFMAX_TBL[MAX_ISO_TBL_INDEX+1];

	HI_S32  CTF_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  YSMTH_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  YSFDLT_TBL[MAX_ISO_TBL_INDEX+1];
	HI_S32  YSFDL_TBL[MAX_ISO_TBL_INDEX+1];

    HI_U32 ysf;
    HI_U32 ytf;
    HI_U32 tfStrMax;

	HI_U32 Night_To_Day_ThrLuma = 1;
	HI_U32 Day_To_Night_ThrLuma = 1;
        
	if (sensor == SENSOR_OV9750||sensor == SENSOR_OV9750m)
	{
		//设置总gain
		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 62*1024;
		//stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 15;
		//stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 6;
		stExpAttr.u8AERunInterval =2;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

		
		//动态坏点校正
		//ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		//HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		//pstDPDynamicAttr.bEnable=1;
		//pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		//memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		//memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		//HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

		//大面积偏色--室内外判断阈值
		ISP_AWB_ATTR_EX_S pstAWBAttrEx;
		HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
		pstAWBAttrEx.stInOrOut.u32OutThresh=11760;// 700*16.8
		HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);
		int i;
		HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={84,90,95,100,120,123,124,160,160,160,160,160,160,160,160,160};
		HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={48,54,68,75,85,90,92,110,120,120,120,120,120,120,120,120};
		
		HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{

			YSF_TBL[i]=ysf[i];
			YTF_TBL[i]=ytf[i];
			CSF_TBL[i]=csf[i];
		}
		
		//ISP_DRC_ATTR_S pstDRC;
       // HI_MPI_ISP_GetDRCAttr(0,&pstDRC);
       // pstDRC.u8Asymmetry=7;
       // pstDRC.u8SecondPole=180;
       // pstDRC.u8Stretch=55;
        //HI_MPI_ISP_SetDRCAttr(0,&pstDRC);
	}
	else if(sensor == SENSOR_OV9732)
	{
		//设置总gain
		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 33*1024;
		//stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 15;
		//stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 6;
		stExpAttr.u8AERunInterval =2;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

		
		//动态坏点校正
		//ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		//HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		//pstDPDynamicAttr.bEnable=1;
		//pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		//memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		//memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		//HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

		//大面积偏色--室内外判断阈值
		ISP_AWB_ATTR_EX_S pstAWBAttrEx;
		HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
		pstAWBAttrEx.stInOrOut.u32OutThresh=28629;// 700/968*40000
		HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);

		
        HI_U8 u8Strength[16] = {110,120,140,140,150,170,170,170,160,110,110,255,255,255,255,255};
        HI_U16 u16Threshold[16] = {1500,1500,1500,1500,1500,1500,1500,1600,1500,1250,1250,1250,1250,1250,1250,1250};
        ISP_NR_ATTR_S pstNRAttr;
        HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
        memcpy(&pstNRAttr.stAuto.au8VarStrength,&u8Strength,16);
        memcpy(&pstNRAttr.stAuto.au16Threshold,&u16Threshold,16*2);
        HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);

		
		int i;
		HI_S32 ov9732_ysf[MAX_ISO_TBL_INDEX+1]={70,80,90,100,105,115,130,160,160,160,160,160,160,160,160,160};
		HI_S32 ov9732_ytf[MAX_ISO_TBL_INDEX+1]={55,65,70,78,85,90,95,110,120,120,120,120,120,120,120,120};
		HI_S32 ov9732_csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
		HI_S32 ov9732_ypk[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{

			YSF_TBL[i]=ov9732_ysf[i];
			YTF_TBL[i]=ov9732_ytf[i];
			CSF_TBL[i]=ov9732_csf[i];
			YPK_TBL[i]=ov9732_ypk[i];
		}
		
		//ISP_DRC_ATTR_S pstDRC;
       // HI_MPI_ISP_GetDRCAttr(0,&pstDRC);
       // pstDRC.u8Asymmetry=7;
       // pstDRC.u8SecondPole=180;
       // pstDRC.u8Stretch=55;
        //HI_MPI_ISP_SetDRCAttr(0,&pstDRC);
	}
	int swmode =0;
	if (sensor == SENSOR_OV2710&&swmode)
	{
		//设置总gain
		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 44*1024;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

		
		//动态坏点校正
		ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		pstDPDynamicAttr.bEnable=0;
		pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		//memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		//memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

		//dci关闭
		VI_DCI_PARAM_S pstDciParam;
		HI_MPI_VI_GetDCIParam(0, &pstDciParam);
		pstDciParam.bEnable=HI_FALSE;
		HI_MPI_VI_SetDCIParam(0, &pstDciParam);

        //drc参数设置
        ISP_DRC_ATTR_S pstDRC;
        HI_MPI_ISP_GetDRCAttr(0,&pstDRC);
        pstDRC.bEnable = HI_FALSE;
        pstDRC.u8Asymmetry=7;
        pstDRC.u8SecondPole=180;
        pstDRC.u8Stretch=55;
        HI_MPI_ISP_SetDRCAttr(0,&pstDRC);

        HI_U8 u8Strength[16] = {70,90,100,110,120,130,140,150,160,110,110,255,255,255,255,255};
        HI_U16 u16Threshold[16] = {800,1100,1300,1350,1400,1500,1600,1700,1500,1250,1250,1250,1250,1250,1250,1250};
        ISP_NR_ATTR_S pstNRAttr;
        HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
        memcpy(&pstNRAttr.stAuto.au8VarStrength,&u8Strength,16);
        memcpy(&pstNRAttr.stAuto.au16Threshold,&u16Threshold,16*2);
        HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);

        ISP_ACM_LUT_S pstACMLut;
        const short LUT_Luma[ACM_Y_NUM][ACM_S_NUM][ACM_H_NUM]=
        {
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   4,  20,  37,   7,   5,   5,   5,   5,   6,   6,   7,   7,   7,   8,   7,   6,   5,   2,  -1,  -6,  10,  12,  10,   6,   4,   0, -10,  -8,   4,},
                {  -3,  41,  80,  47,  31,  29,  29,  30,  35,  44,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -39,  -3,},
                {  14, 102, 177, 126,  69,  60,  55,  59,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -51,  14,},
                {   0,   0, 254, 254,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  30,  43,  56,  -8,  -9,  -9, -10, -10, -11, -11, -12, -13, -14, -16, -17, -19, -22, -24, -27, -30,  10,  11,  10,   8,   6,  27,  21,  22,  30,},
                {   8,  41,  74,  15,  11,  11,  11,  11,  12,  13,  14,  15,  16,  16,  16,  14,  11,   5,  -1, -11,  20,  26,  21,  12,   8,   0, -19, -16,   8,},
                {   0,   0, 108,   0,   0,   0,   0,   0,   0,   0,  53,  63,  81,   0,   0,   0,   0,   0,   0,   0,   0,   0,  41,   0,   0,   0, -60,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  57,  67,  79, -22, -23, -23, -24, -25, -26, -27, -28, -29, -31, -32, -34, -37, -39, -42, -44, -47,  11,  12,  12,  11,   9,  52,  48,  50,  57,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -10, -13, -17, -21, -27, -33, -40,  19,  23,  20,  15,  10,  28,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  17,   9,  -2, -16,  31,  39,  32,  19,  12,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  49,  23,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
        };
        const short LUT_Hue[ACM_Y_NUM][ACM_S_NUM][ACM_H_NUM]=
        {
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  -3,   2,  17,  30,  23,  20,  16,  11,   8,   3,   0,  -4,  -7,  -8, -10, -11, -11, -12,  -9, -10,  -8,  -9, -13, -15, -10,   3,   9,   3,  -3,},
                { -12, -14,   8,  28,  22,  17,  12,   5,   0,  -5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9, -12,},
                { -17, -26,   3,  35,  26,  19,  10,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  22, -17,},
                {   0,   0,  -1,  63,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  10,  22,  35,  41,  32,  29,  24,  20,  16,  12,   7,   3,  -1,  -5,  -9, -12, -14, -18, -18, -21, -22, -27, -30, -28, -21, -10,   1,   5,  10,},
                {  -3,   2,  18,  29,  23,  20,  17,  12,   8,   3,   0,  -4,  -7,  -9, -10, -10, -11, -11, -10,  -9,  -9,  -9, -13, -16, -10,   3,   9,   3,  -3,},
                {   0,   0,  11,   0,   0,   0,   0,   0,   0,   0,  -5,  -8,  -8,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -8,   0,   0,   0,  20,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  23,  39,  52,  53,  41,  38,  32,  28,  23,  18,  12,   7,   2,  -3,  -8, -13, -16, -21, -24, -29, -30, -41, -46, -44, -35, -21,  -4,   9,  23,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -7,  -9, -11, -14, -15, -16, -16, -17, -19, -21, -22, -15,  -3,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -11, -11, -10,  -9,  -9,  -9, -13, -16, -10,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -8, -16,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
        };
        const short LUT_Sat[ACM_Y_NUM][ACM_S_NUM][ACM_H_NUM]=
        {
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  -3, -11, -11, -14,  -3,  -1,  -1,  -1,  -2,  -4,  -6,  -8, -13, -17, -21, -25, -29, -33, -36, -38, -33, -25, -16, -11, -11,  -9,  -3,   2,  -3,},
                {   1, -31, -42, -45, -16, -13, -12, -16, -22, -32,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  12,   1,},
                {  -6, -74,-111,-103, -36, -28, -26, -34,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  -6,},
                {   0,   0,-255,-255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                { -14, -15,  -8,  -8,   6,   8,  10,  11,  12,  12,  11,  11,   9,   7,   5,   3,   0,  -3,  -6,  -7,   0,  -5, -10, -15, -19, -17, -15, -14, -14,},
                {  -5, -20, -21, -27,  -6,  -3,   0,  -1,  -3,  -6, -11, -17, -25, -31, -40, -49, -56, -64, -70, -75, -65, -50, -32, -22, -22, -16,  -4,   5,  -5,},
                {   0,   0, -44,   0,   0,   0,   0,   0,   0,   0, -44, -61, -83,   0,   0,   0,   0,   0,   0,   0,   0,   0, -71,   0,   0,   0,  -9,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                { -27, -21,  -7,  -3,  15,  18,  21,  23,  25,  26,  25,  25,  25,  23,  21,  20,  17,  14,  12,  10,  16,   6,  -6, -17, -27, -28, -30, -29, -27,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -5, -10, -16, -22, -28, -32, -37, -23, -24, -23, -25, -29, -21,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -84, -95,-104,-113, -97, -75, -47, -32, -33,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -81, -39,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
        };
        memcpy(&pstACMLut.as16Y,&LUT_Luma,5*7*29 * 2);
        memcpy(&pstACMLut.as16H,&LUT_Hue,5*7*29 * 2);
        memcpy(&pstACMLut.as16S,&LUT_Sat,5*7*29 * 2);
        HI_MPI_ISP_SetAcmCoeff(0,&pstACMLut,ISP_ACM_MODE_GREEN);
        
        ISP_ACM_ATTR_S pstACMAttr;
        HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
        pstACMAttr.bEnable = HI_TRUE;       
        pstACMAttr.bDemoEnable = HI_FALSE;
        HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
		
	}	
	else if(sensor== SENSOR_OV2710)
	{
		int i;
		//HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={100,100,100,110,120,120,140,160,160,160,160,160,160,160,160,160};
		//HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={64,64,68,78,80,100,110,110,120,120,120,120,120,120,120,120};
		//HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
		
		HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={90,100,100,100,105,111,135,150,160,160,160,160,160,160,160,160};
		HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={64,64,70,75,83,95,110,110,120,120,120,120,120,120,120,120};
		HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
		
		HI_S32 ypk[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{

			YSF_TBL[i] = ysf[i];
			YTF_TBL[i] = ytf[i];
			CSF_TBL[i] = csf[i];
			YPK_TBL[i] = ypk[i];
		}
		
		ISP_AWB_ATTR_EX_S pstAWBAttrEx;
		HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
		pstAWBAttrEx.stInOrOut.u32OutThresh=15048;
		HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);

		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 42*1024;
		//stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 15;
		//stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 2;
		stExpAttr.u8AERunInterval =2;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

		ISP_SHADING_ATTR_S  pstShadingAttr;		
		HI_MPI_ISP_GetMeshShadingAttr(0,&pstShadingAttr);
		pstShadingAttr.bEnable =HI_TRUE;
		for(i=0;i<SHADING_MESH_NUM;i++)
		{
			pstShadingAttr.au32NoiseControlGain[i] =NoiseControlGainTbl[i];



		}
		if (!strcmp(hwinfo.devName,"HV310"))
		{
			pstShadingAttr.bEnable =HI_FALSE;

		}
		HI_MPI_ISP_SetMeshShadingAttr(0,&pstShadingAttr);

		StarLightEnable =TRUE; //针对zw开启星光级
		
		

	}
	else if(sensor== SENSOR_SC2135)
	{
		int i;
		//HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={100,100,100,110,120,120,140,160,160,160,160,160,160,160,160,160};
		//HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={64,64,68,78,80,100,110,110,120,120,120,120,120,120,120,120};
		//HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
		
		HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={100,100,100,100,105,111,135,150,160,160,160,160,160,160,160,160};
		HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={64,64,70,75,83,95,110,110,120,120,120,120,120,120,120,120};
		HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
		
		HI_S32 ypk[MAX_ISO_TBL_INDEX+1]={12,12,10,7,0,0,0,0,0,0,0,0,0,0,0,0};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{

			YSF_TBL[i] = ysf[i];
			YTF_TBL[i] = ytf[i];
			CSF_TBL[i] = csf[i];
			YPK_TBL[i] = ypk[i];
		}
		
		ISP_AWB_ATTR_EX_S pstAWBAttrEx;
		HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
		pstAWBAttrEx.stInOrOut.u32OutThresh=15048;
		HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);

		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 62*1024;
		//stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 15;
		//stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 2;
		stExpAttr.u8AERunInterval =2;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
		

	}
	
	else if(sensor == SENSOR_SC2045)
	{
		int i;
		
		HI_S32 sc2045_ysf[MAX_ISO_TBL_INDEX+1]={80,90,100,110,115,115,120,150,160,160,160,160,160,160,160,160};
		HI_S32 sc2045_ytf[MAX_ISO_TBL_INDEX+1]={64,72,75,80,85,90,95,110,120,120,120,120,120,120,120,120};
		
		HI_S32 sc2045_ypk[MAX_ISO_TBL_INDEX+1]={10,10,10,8,0,0,0,0,0,0,0,0,0,0,0,0};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{

			YSF_TBL[i] = sc2045_ysf[i];
			YTF_TBL[i] = sc2045_ytf[i];
			YPK_TBL[i] = sc2045_ypk[i];
		}

		//动态坏点校正
		HI_U16 dpc_slope[16] = {200,210,210,220,220,220,220,255,255,255,255,255,255,255,255,255};
        HI_U16 dpc_ratio[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		pstDPDynamicAttr.bEnable=1;
		pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

		HI_U8 u8Strength[16] = {125,130,140,160,160,150,140,130,130,100,100,100,100,100,100,100};
        HI_U16 u16Threshold[16] = {1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1200,1200,1200,1200,1200,1200};
        ISP_NR_ATTR_S pstNRAttr;
        HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
        memcpy(&pstNRAttr.stAuto.au8VarStrength,&u8Strength,16);
        memcpy(&pstNRAttr.stAuto.au16Threshold,&u16Threshold,16*2);
        HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);
		
		ISP_AWB_ATTR_EX_S pstAWBAttrEx;
		HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
		pstAWBAttrEx.stInOrOut.u32OutThresh=15048;
		HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);

		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 62*1024;
		stExpAttr.u8AERunInterval =2;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	}
  
    if (sensor == SENSOR_AR0130)
	{
		//设置总gain
		ISP_EXPOSURE_ATTR_S stExpAttr;
		HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
		stExpAttr.stAuto.stSysGainRange.u32Max = 60*1024;
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

		
		//动态坏点校正
		HI_U16 dpc_slope[16] = {100,140,180,210,230,240,250,255,255,255,255,255,255,255,255,255};
        HI_U16 dpc_ratio[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		pstDPDynamicAttr.bEnable=1;
		pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

		//dci关闭
		VI_DCI_PARAM_S pstDciParam;
		HI_MPI_VI_GetDCIParam(0, &pstDciParam);
		pstDciParam.bEnable=HI_FALSE;
		HI_MPI_VI_SetDCIParam(0, &pstDciParam);

        //drc参数设置
        ISP_DRC_ATTR_S pstDRC;
        HI_MPI_ISP_GetDRCAttr(0,&pstDRC);
        pstDRC.bEnable = HI_FALSE;
        pstDRC.u8Asymmetry=7;
        pstDRC.u8SecondPole=180;
        pstDRC.u8Stretch=55;
        HI_MPI_ISP_SetDRCAttr(0,&pstDRC);

        HI_U8 u8Strength[16] = {24,24,32,32,44,44,48,48,56,56,56,255,255,255,255,255};
        HI_U16 u16Threshold[16] = {900,1100,1300,1400,1500,1600,1700,1700,1700,1500,1500,1250,1250,1250,1250,1250};
        ISP_NR_ATTR_S pstNRAttr;
        HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
        memcpy(&pstNRAttr.stAuto.au8VarStrength,&u8Strength,16);
        memcpy(&pstNRAttr.stAuto.au16Threshold,&u16Threshold,16*2);
        HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);

        ISP_ACM_LUT_S pstACMLut;
        const short LUT_Luma[ACM_Y_NUM][ACM_S_NUM][ACM_H_NUM]=
        {
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  61,  92, 120,  16,  17,  16,  15,  14,  12,  10,   7,   5,   1,  -2,  -6, -11, -16, -21, -27, -31,  17,  13,   3,  -5,  -7,  23,  19,  34,  61,},
                {  57, 124, 194,  38,  45,  45,  45,  44,  43,  40,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,  57,},
                {  47, 147, 254,  49,  63,  67,  70,  72,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -7,  47,},
                {   0,   0, 225,  15,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                { 110, 137, 158,   8,   6,   5,   4,   2,   1,  -1,  -4,  -6,  -9, -12, -15, -18, -21, -24, -27, -29,  12,  11,   6,   1,  -3,  38,  58,  82, 110,},
                { 122, 185, 242,  33,  34,  33,  31,  28,  25,  21,  16,  10,   3,  -4, -12, -22, -32, -42, -52, -62,  36,  28,   7, -10, -14,  48,  40,  68, 122,},
                {   0,   0, 254,   0,   0,   0,   0,   0,   0,   0,  42,  32,  18,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -4,   0,   0,   0,  41,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                { 150, 175, 192,   1,  -1,  -2,  -3,  -5,  -7,  -9, -12, -14, -16, -19, -21, -24, -27, -29, -31, -33,  10,  10,   7,   2,  -2,  68,  94, 122, 150,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -14, -21, -28, -35, -42, -49, -54,  29,  25,  12,  -1,  -9,  53,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -48, -63, -77, -91,  54,  41,  10, -15, -21,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1, -34,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
        };
        const short LUT_Hue[ACM_Y_NUM][ACM_S_NUM][ACM_H_NUM]=
        {
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  -3,  19,  41,  46,  34,  30,  25,  21,  16,  11,   6,   1,  -3,  -7, -11, -13, -16, -18, -18, -18, -16,   0,  10,   2, -10, -11, -10, -10,  -3,},
                {  -3,  15,  39,  45,  33,  29,  24,  19,  14,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -3,},
                {  -3,  14,  41,  48,  35,  30,  25,  20,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  19,  -3,},
                {   0,   0,  25,  54,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {  -2,  29,  52,  53,  40,  36,  31,  26,  21,  16,  11,   7,   2,  -2,  -5,  -8, -10, -12, -13, -14, -12,  -4,   3,   2,  -5, -14, -19, -18,  -2,},
                {  -3,  19,  41,  46,  34,  30,  25,  20,  16,  11,   6,   2,  -3,  -7, -10, -13, -16, -18, -18, -18, -16,   0,  10,   2, -10, -11, -10, -11,  -3,},
                {   0,   0,  16,   0,   0,   0,   0,   0,   0,   0,   5,   0,  -4,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,  -2,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   1,  40,  63,  62,  48,  43,  37,  32,  27,  22,  17,  11,   7,   3,  -1,  -5,  -7, -10, -11, -13, -13, -10,  -5,  -6, -13, -22, -28, -24,   1,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -4,  -8, -10, -13, -14, -15, -14, -13,  -2,   7,   4,  -4, -11,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -16, -18, -18, -18, -16,   0,  10,   2, -10,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10, -19,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
        };
        const short LUT_Sat[ACM_Y_NUM][ACM_S_NUM][ACM_H_NUM]=
        {
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                { -57, -70, -33, -51,  -8,  -2,   3,   6,   5,   4,  -1,  -5, -13, -21, -30, -40, -53, -64, -76, -85, -59, -66, -26,  15,  12,  19,   2, -22, -57,},
                { -52, -87,   2,-103, -28, -17, -11, -11, -12, -17,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -7, -52,},
                { -44, -92,  98,-145, -34, -24, -18, -18,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -49, -44,},
                {   0,   0,  24,-146,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {-101,-105, -58, -42,   7,  15,  22,  27,  28,  28,  27,  24,  18,  12,   3,  -5, -15, -26, -34, -42, -33, -47, -32,  -7,   4,   6, -19, -61,-101,},
                {-110,-138, -63, -98, -15,  -2,   7,  11,  11,   8,   1,  -9, -23, -40, -60, -81,-102,-126,-147,-169,-117,-131, -50,  31,  27,  42,   6, -41,-110,},
                {   0,   0,-212,   0,   0,   0,   0,   0,   0,   0, -28, -43, -58,   0,   0,   0,   0,   0,   0,   0,   0,   0, -20,   0,   0,   0, -38,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {-136,-130, -68, -36,  23,  32,  41,  45,  49,  51,  51,  49,  45,  41,  33,  27,  17,   7,  -2,  -9,  -6, -26, -23, -10,  -6,  -7, -44, -95,-136,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  -6, -24, -42, -62, -84,-102,-121, -96,-113, -62,   1,  23,  33,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,-154,-188,-222,-255,-174,-196, -72,  48,  40,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, -52, 128,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
            {
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
                {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
            },
        };
        memcpy(&pstACMLut.as16Y,&LUT_Luma,5*7*29 * 2);
        memcpy(&pstACMLut.as16H,&LUT_Hue,5*7*29 * 2);
        memcpy(&pstACMLut.as16S,&LUT_Sat,5*7*29 * 2);
        HI_MPI_ISP_SetAcmCoeff(0,&pstACMLut,ISP_ACM_MODE_GREEN);
        
        ISP_ACM_ATTR_S pstACMAttr;
        HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
        pstACMAttr.u32GainLuma = 129;
        pstACMAttr.u32GainHue = 288;
        pstACMAttr.u32GainSat = 79;
        pstACMAttr.bEnable = HI_TRUE;       
        pstACMAttr.bDemoEnable = HI_FALSE;
        HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
		
	}	  
	else if(sensor== SENSOR_OV2735)
	{
		int i;

		//HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={75,88,102,110,118,120,120,125,126,128,130,160,160,160,160,160};
		//HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={85,90,102,110,118,120,120,125,126,128,130,160,160,160,160,160};
		HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={90,95,110,115,120,120,124,124,126,128,130,160,160,160,160,160};
		HI_S32 ysf_910[MAX_ISO_TBL_INDEX+1]={90,95,110,118,123,125,126,126,126,128,130,160,160,160,160,160};
		//HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={64,72,76,80,90,95,95,110,115,115,120,120,120,120,120,120};
		HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={64,72,80,83,88,96,100,112,115,115,120,120,120,120,120,120};
		
		HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,48,64,72,72,75,75,75,75,75,32,32,32,32};
		HI_S32 ctf[MAX_ISO_TBL_INDEX+1]={0,0,2,6,12,20,20,22,22,22,22,22,22,24,32,32};
		
		//HI_S32 ypk[MAX_ISO_TBL_INDEX+1]={10,8,6,0,0,0,0,0,0,0,0,0,0,0,0,0};
		HI_S32 ypk[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


		//HI_S32 ysmthStr[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,48,60,90,90,90,90,90,0,0,0,0};
		HI_S32 ysmthStr[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,0,0,0,90,90,90,90,0,0,0,0};

		//HI_S32 ysfStrDlt[MAX_ISO_TBL_INDEX+1]= {0,0,0,0,0,10,20,25,26,32,54,64,36,0,0,0};
		HI_S32 ysfStrDlt[MAX_ISO_TBL_INDEX+1]= {0,0,0,0,0,10,26,26,26,32,54,64,36,0,0,0};
		
		HI_S32 ysfStrDl[MAX_ISO_TBL_INDEX+1]= {0,0,0,10,36,60,90,90,90,90,125,125,0,0,0,0};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{
			if(HWTYPE_MATCH(HW_TYPE_C9))
				YSF_TBL[i] = ysf_910[i];
			else
				YSF_TBL[i] = ysf[i];
			YTF_TBL[i] = ytf[i];
			CSF_TBL[i] = csf[i];
			YPK_TBL[i] = ypk[i];


			CTF_TBL [i] =ctf[i];
			YSMTH_TBL[i] = ysmthStr[i];
			
			YSFDLT_TBL[i] = ysfStrDlt[i];
			YSFDL_TBL[i] = ysfStrDl[i];

			//YTFDLT_TBL[i] = ytfStrDlt[i];
			//YTFDL_TBL[i] = ytfStrDl[i];
		}
		if(HWTYPE_MATCH(HW_TYPE_C9))
		{
			HI_U16 dpc_slope[16] = {80,110,160,220,238,248,252,255,255,255,255,255,255,255,255,255};

			ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
			HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
			pstDPDynamicAttr.bEnable=1;
			pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
			memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
			HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

			HI_U8 u8FixStrength[16] = {0,0,0,10,36,85,110,120,120,120,255,255,255,255,255,255};
			ISP_NR_ATTR_S pstNRAttr;
			HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
			memcpy(&pstNRAttr.stAuto.au8FixStrength,&u8FixStrength,16);
			HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);
		}
		
		if (HWTYPE_MATCH(HW_TYPE_C8)
			|| HWTYPE_MATCH(HW_TYPE_V3)
			|| HWTYPE_MATCH(HW_TYPE_V6)
			|| HWTYPE_MATCH(HW_TYPE_C8A)
			|| HWTYPE_MATCH(HW_TYPE_C5))
		{
			pSoft_Light_Sensitive = &Soft_Light_Sensitive_V200_HC8A_OV2735;
			if(HWTYPE_MATCH(HW_TYPE_V6))
				pSoft_Light_Sensitive->Ratio_Rg_Ir=267;
		}
	}
	else if(sensor == SENSOR_SC2235)
	{

		HI_U8 u8Strength[16] = {100,110,120,140,160,180,180,180,255,255,255,255,255,255,255,255};
		HI_U8 u8FixStr[16] = {0,0,0,0,3,8,10,15,20,20,20,20,20,20,20,20};
		HI_U8 u8LowFreq[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2};
        HI_U16 u16Threshold[16] = {1500,1500,1500,1600,1700,1700,1750,1750,1800,1800,1800,1800,1800,1800,1800,1800};
        ISP_NR_ATTR_S pstNRAttr;
        HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
        memcpy(&pstNRAttr.stAuto.au8VarStrength,&u8Strength,16);
		memcpy(&pstNRAttr.stAuto.au8FixStrength,&u8FixStr,16);
		memcpy(&pstNRAttr.stAuto.au8LowFreqSlope,&u8LowFreq,16);
        memcpy(&pstNRAttr.stAuto.au16Threshold,&u16Threshold,16*2);
        HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);
	
		int i;
		
		HI_S32 ysf[MAX_ISO_TBL_INDEX+1]={90,96,100,105,110,118,124,126,130,130,130,130,130,130,130,130};
		HI_S32 ytf[MAX_ISO_TBL_INDEX+1]={75,78,80,83,90,95,105,115,120,120,120,120,120,120,120,120};
		HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,40,50,64,64,72,32,32,32,32,32,32,32,32};
		HI_S32 ctf[MAX_ISO_TBL_INDEX+1]={0,0,0,0,10,15,20,20,22,22,22,22,22,22,22,22};
		//HI_S32 ypk[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		
		HI_S32 ysfStrDlt[MAX_ISO_TBL_INDEX+1]= {0,0,0,0,3,10,23,28,32,32,32,32,32,32,32,32};
		HI_S32 ysfStrDl[MAX_ISO_TBL_INDEX+1]= {20,20,20,20,43,60,70,80,90,90,90,90,90,90,90,90};

		for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		{

			YSF_TBL[i] = ysf[i];
			YTF_TBL[i] = ytf[i];
			CSF_TBL[i] = csf[i];
			CTF_TBL [i] =ctf[i];
			//YPK_TBL[i] = ypk[i];
			
			YSFDLT_TBL[i] = ysfStrDlt[i];
			YSFDL_TBL[i] = ysfStrDl[i];
		}
		
		ISP_AWB_ATTR_EX_S pstAWBAttrEx;
		HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
		pstAWBAttrEx.stInOrOut.u32OutThresh=15048;
		HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);

		HI_U16 dpc_slope[16] = {100,110,125,140,180,200,210,220,255,255,255,255,255,255,255,255};
        HI_U16 dpc_ratio[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		pstDPDynamicAttr.bEnable=1;
		pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

	}
	else if (sensor == SENSOR_MN34227)
	{
		
		int i;
		
		HI_S32 ysf[MAX_ISO_TBL_INDEX+1]= { 42,50,63,80,100,110,120,132,165,188,  175,178,182,182,182,182};
		HI_S32 ysf_wl[MAX_ISO_TBL_INDEX+1]={35,42,57,80,105,110,120,140,165,188,  175,178,182,182,182,182};//sf of white light
		HI_S32 ytf[MAX_ISO_TBL_INDEX+1]= {  73,77,85,92,96,98,103,112,112,112, 125,127,127,127,127,127};
		HI_S32 ytf_wl[MAX_ISO_TBL_INDEX+1]={73,77,85,92,100,100,103,112,112,111, 125,127,127,127,127,127};//tf of white light
		HI_S32 csf[MAX_ISO_TBL_INDEX+1]={32,32,32,32,45,60,64,70,70,75, 76,76,77,77,77,77};		
		HI_S32 ypk[MAX_ISO_TBL_INDEX+1]={0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0};
        HI_S32 tfmax[MAX_ISO_TBL_INDEX+1]={14,14,14,14,14,14,14,14,14,14, 15,15,15,15,15,15};
        
        //if(bWhiteLight)
		//{


           // for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		   // {

			   // YSF_TBL[i] = ysf_wl[i];
			  //  YTF_TBL[i] = ytf_wl[i];
			  //  CSF_TBL[i] = csf[i];
			    //YPK_TBL[i] = ypk[i];
              //  TFMAX_TBL[i]=tfmax[i];
		    //}
        //}
        //else
        {
            for(i=0;i<=MAX_ISO_TBL_INDEX;i++)
		    {

			    YSF_TBL[i] = ysf[i];
			    YTF_TBL[i] = ytf[i];
			    CSF_TBL[i] = csf[i];
			    YPK_TBL[i] = ypk[i];
                TFMAX_TBL[i]=tfmax[i];
		    }
        }
        
        HI_U16 dpc_slope[16] = {70,100,170,220,240,252,248,235,230,220,220,210,200,200,200,200};
        HI_U16 dpc_ratio[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		ISP_DP_DYNAMIC_ATTR_S  pstDPDynamicAttr;
		HI_MPI_ISP_GetDPDynamicAttr(0, &pstDPDynamicAttr);
		pstDPDynamicAttr.bEnable=1;
		pstDPDynamicAttr.enOpType=OP_TYPE_AUTO;
		memcpy(pstDPDynamicAttr.stAuto.au16Slope,dpc_slope,16*2);
		memcpy(pstDPDynamicAttr.stAuto.au16BlendRatio,dpc_ratio,16*2);
		HI_MPI_ISP_SetDPDynamicAttr(0,&pstDPDynamicAttr);

        ISP_ACM_LUT_S pstACMLut;

const short LUT_Luma[5][7][29]=
{
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   3,   6,   8,   4,   2,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,  13,  27,  32,  16,   8,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,  14,  29,  32,  16,   8,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   3,   7,   8,   4,   2,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   3,   6,   8,   4,   2,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,  13,  27,  32,  16,   8,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,  14,  29,  32,  16,   8,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   3,   7,   8,   4,   2,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
};
const short LUT_Hue[5][7][29]=
{
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  13,  15,   5,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  18,  53,  62,  21,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  19,  56,  63,  21,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  14,  16,   5,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  13,  15,   5,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  18,  53,  62,  21,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  19,  56,  63,  21,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  14,  16,   5,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
};
const short LUT_Sat[5][7][29]=
{
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   5,  15,  17,   5,   2,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  20,  60,  71,  23,  11,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  21,  64,  72,  24,  12,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   5,  16,  18,   6,   3,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   5,  15,  17,   5,   2,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  20,  61,  71,  23,  11,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  21,  64,  72,  24,  12,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   5,  16,  18,   6,   3,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
    {
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
        {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,},
    },
};

        memcpy(&pstACMLut.as16Y,&LUT_Luma,5*7*29 * 2);
        memcpy(&pstACMLut.as16H,&LUT_Hue,5*7*29 * 2);
        memcpy(&pstACMLut.as16S,&LUT_Sat,5*7*29 * 2);
        HI_MPI_ISP_SetAcmCoeff(0,&pstACMLut,ISP_ACM_MODE_GREEN);

        ISP_ACM_ATTR_S pstACMAttr;
        HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
        pstACMAttr.bEnable = HI_TRUE;       
        pstACMAttr.bDemoEnable = HI_FALSE;
		pstACMAttr.u32GainLuma = 64;
        pstACMAttr.u32GainHue = 200;
        pstACMAttr.u32GainSat = 64;
        HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
            
    }
	JV_EXP_CHECK_THRESH_S jv_exp_thresh;
	unsigned int jv_exp;
	unsigned int jv_exp_last=0;
	int low_frame_cnt=0; //全彩低帧统计次数
	int full_frame_cnt=0;//全彩全帧统计次数
	int ToNightCnt=0;//进入黑白夜视统计次数
	unsigned char  AdcNightCnt =0;
	unsigned char  AdcDayCnt =0;
	HI_U32  expTime=0;
	if(sensor==SENSOR_OV2710)
	{
		jv_exp_thresh.DayFullFrameExpThresh=1325*10*64; //1359*50*64...60gain;to彩色全帧的曝光阈值
		jv_exp_thresh.DayLowFrameExpThresh_0=1325*16*64; //1359*70*64...60gain; to彩色降帧的曝光阈值
		// jv_exp_thresh.DayLowFrameRate_0=13;//彩色降帧帧率
		jv_exp_thresh.DayLowFrameRatio = 32;
	 
		jv_exp_thresh.ToNightExpThresh=2654*32*64;//2552*30*64;//软光敏--彩色切黑白的阈值
		jv_exp_thresh.ToNightLumThresh=45;//软光敏彩色切黑白的亮度均
	}
	else if(sensor==SENSOR_MN34227 )
    {
        jv_exp_thresh.DayFullFrameExpThresh=1351*75*64; //2778*70*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=1351*128*64; //1351*170*64...170gain; to彩色降帧的曝光阈值

		jv_exp_thresh.DayHalfFrameExpThresh=2876*420*64; //彩色(1/4)全帧to彩色(1/2)全帧的曝光阈?

		jv_exp_thresh.DayLowFrameExpThresh_1=2876*500*64; //彩色(1或1/2)全帧to彩色(1/4)全帧的曝光阈值
		 
        jv_exp_thresh.DayLowFrameRatio = 40; //10
	    jv_exp_thresh.DayLowFrameRatio_1 = 72; //5.5

        jv_exp_thresh.ToNightExpThresh=2876*500*64;//2778*450*64;//软光敏--彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=35;//软光敏彩色切黑白的亮度均
    }

	else if(sensor==SENSOR_OV2735)
    {
        jv_exp_thresh.DayFullFrameExpThresh=1629*8*64;//1460*22*64; //1359*50*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=1629*14*64;//1460*32*64; //1359*70*64...60gain; to彩色降帧的曝光阈值
       // jv_exp_thresh.DayLowFrameRate_0=13;//彩色降帧帧率
        jv_exp_thresh.DayLowFrameRatio = 32;

        jv_exp_thresh.ToNightExpThresh=3262*30*64;//2552*30*64;//软光敏--彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=50;//软光敏彩色切黑白的亮度均
       // StarLightEnable =TRUE;

	   	u32EnCoeff = 1629*2*64*50/256;
	   	Night_To_Day_ThrLuma = 60*pSoft_Light_Sensitive->Statistical_Accuracy*u32EnCoeff / (1629*24*64/64);
		Day_To_Night_ThrLuma = jv_exp_thresh.ToNightLumThresh*pSoft_Light_Sensitive->Statistical_Accuracy*u32EnCoeff / ( jv_exp_thresh.ToNightExpThresh/64);
    }
	else if(sensor==SENSOR_SC2235)
    {
        jv_exp_thresh.DayFullFrameExpThresh=1687*15*64; //1359*50*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=1687*25*64; //1359*70*64...60gain; to彩色降帧的曝光阈值
       // jv_exp_thresh.DayLowFrameRate_0=13;//彩色降帧帧率
        jv_exp_thresh.DayLowFrameRatio = 26;

        jv_exp_thresh.ToNightExpThresh=2812*43*64;//2552*30*64;//软光敏--彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=50;//软光敏彩色切黑白的亮度均
       // StarLightEnable =TRUE;
    }
	else if(sensor==SENSOR_OV9750)
    {
        jv_exp_thresh.DayFullFrameExpThresh=2372*12*64;//1460*22*64; //1359*50*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=2372*22*64;//1460*32*64; //1359*70*64...60gain; to彩色降帧的曝光阈值
       // jv_exp_thresh.DayLowFrameRate_0=13;//彩色降帧帧率
        jv_exp_thresh.DayLowFrameRatio = 32;

        jv_exp_thresh.ToNightExpThresh=4748*40*64;//2552*30*64;//软光敏--彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=50;//软光敏彩色切黑白的亮度均
       // StarLightEnable =TRUE;
    }


	#define MAX_GR_GB_COUNT 2
	HI_U8   AEToDayCnt = 0;
	HI_U8   AEToNightCnt = 0;
	
	BOOL bNightStatisticDone =FALSE;


	HI_U32 ExposureTbl[MAX_GR_GB_COUNT]; 
	HI_U16 LineTbl[MAX_GR_GB_COUNT];
	HI_U32 IsoTbl[MAX_GR_GB_COUNT]; 
	
	HI_U32 Exp_Night;
	HI_U32 ISO_Night;
	HI_U32 LINE_Night;
	HI_U8  u8NightIndex=0;
	HI_U32 u32ExpDayDelta =0;
	BOOL   bSmallScene =FALSE; //是否为小场景

	BOOL   bTryCutOpen = FALSE;
	HI_U8   TryCutOpenCnt =0;
	HI_U8   TryCutOpenDelay =0;
	BOOL  bIspNight =FALSE;


	if(sensor == SENSOR_OV9732)
	{
        jv_exp_thresh.DayFullFrameExpThresh=970*10*64; //1359*50*64...60gain;to彩色全帧的曝光阈值
        jv_exp_thresh.DayLowFrameExpThresh_0=970*20*64; //1359*70*64...60gain; to彩色降帧的曝光阈值
        jv_exp_thresh.DayLowFrameRatio = 16;

        jv_exp_thresh.ToNightExpThresh=970*31*64;//2552*30*64;//软光敏--彩色切黑白的阈值
        jv_exp_thresh.ToNightLumThresh=45;//软光敏彩色切黑白的亮度均
	}
	BOOL bCheckNightStatus =FALSE;
	BOOL bCheckDayStatus =FALSE;
	BOOL bCheckNightLowFrameStatus =FALSE;
/*
	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	stExpAttr.stAuto.stAntiflicker.enMode = ISP_ANTIFLICKER_NORMAL_MODE;
	stExpAttr.stAuto.stAntiflicker.bEnable = TRUE;
	stExpAttr.stAuto.stAntiflicker.u8Frequency = 60;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr); 
*/
    while (isp_helper_live)
    {
    	// sleep(1);
		utl_WaitTimeout(!isp_helper_live, 1000);

		if(bCheckNight)
		{
			CheckWaitTimes++;
			if(CheckWaitTimes>25)
			{
				
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.u8Speed = 72;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);	
				bCheckNight =FALSE;
				CheckWaitTimes =0;
			}
		}
		if(last_cut_status)
		{
			if(current_trigger_type == CUT_TRIGGER_TYPE_SYS_INIT)
				isp_cut_trigger_inner(FALSE,CUT_TRIGGER_TYPE_MAX);
			else if(current_trigger_type == CUT_TRIGGER_TYPE_FPS_CHANGE ||current_trigger_type ==CUT_TRIGGER_TYPE_TO_AE_CHANGE)
			{
				cut_trigger_delay++;
				if(cut_trigger_delay > 5)
				{
					isp_cut_trigger_inner(FALSE,CUT_TRIGGER_TYPE_MAX);
					cut_trigger_delay =0;
				}
			}
			else if(current_trigger_type == CUT_TRIGGER_TYPE_TO_DAY ||current_trigger_type == CUT_TRIGGER_TYPE_TO_NIGHT ||current_trigger_type == CUT_TRIGGER_TYPE_TO_DAY_PRE)
			{
				cut_trigger_delay++;
				if(cut_trigger_delay > 22)
				{
					isp_cut_trigger_inner(FALSE,CUT_TRIGGER_TYPE_MAX);
					cut_trigger_delay =0;
				}
			}	
		}
		
		HI_MPI_ISP_QueryExposureInfo(IspDev, &stExpInfo);
		

		u32ISO  = ((HI_U64)stExpInfo.u32AGain * stExpInfo.u32DGain* stExpInfo.u32ISPDGain * 100) >> (10 * 3);
		jv_exp = stExpInfo.u32Exposure;
		stAvm=stExpInfo.u8AveLum;
		expTime = stExpInfo.u32ExpTime;
		//exp_line= jv_exp*100/64;
		//exp_line= exp_line/u32ISO;
		// printf("jv_exp jv_exp %d\n",jv_exp);

		
		pthread_mutex_lock(&isp_daynight_mutex);
		if(sensor == SENSOR_OV2735 || sensor == SENSOR_SC2235)
		{
			if(bNightMode != bIspNight) //检测到切换夜视后，改变下ISO,复位下AE evBias
			{

				bIspNight = bNightMode;
				ISP_EXPOSURE_ATTR_S tmpExpAttr;
				HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
				tmpExpAttr.stAuto.u16EVBias = 1024;
				HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);

				u32ISOLast =0;	
				
			}
		}
		pthread_mutex_unlock(&isp_daynight_mutex);

		
		if(sensor == SENSOR_OV2710)
		{
			if(u32ISO > 1600)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			
			if(AdcNightCnt>=4)
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		else if(sensor == SENSOR_MN34227)
		{
			if(u32ISO > 3000)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			if(AdcNightCnt>=4)	
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}
		if(sensor == SENSOR_SC2235)
		{
			if(u32ISO > 800)
				AdcNightCnt++;
			else
				AdcNightCnt=0;
			
			if(AdcNightCnt>=4)
			{
				bAdcNight =TRUE;
				AdcNightCnt =0;
			}
		}


		if(sensor == SENSOR_OV9732&&bCheckNightDayUsingAE)
		{
            int bNight;
			unsigned int expthrd ;
			unsigned int  lumthrd;
			light_ae_node aeNode ;
            pthread_mutex_lock(&low_frame_mutex);
			
			aeNode = light_ae_list_ov9732[LightAeIndex];
			expthrd = aeNode.ExpThrd;
			lumthrd = aeNode.LumThrd;

			jv_exp_thresh.ToNightExpThresh = expthrd;
			jv_exp_thresh.ToNightLumThresh = lumthrd;
			
			if(isp_get_std_fps()>25) //30 fps情况下
				jv_exp_thresh.ToNightExpThresh = jv_exp_thresh.ToNightExpThresh*25/30;
			if(StarLightEnable)
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}

 				 if(bNightMode == FALSE)//白天模式
 				 {

					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("star light day init\n");
					}
	                if((bAeNight==FALSE)&&(jv_exp >= jv_exp_thresh.ToNightExpThresh)&&(stAvm<=jv_exp_thresh.ToNightLumThresh))
					{
					 	ToNightCnt++;
						if(ToNightCnt>=2)
						{

							if(DayNightMode!=DAYNIGHT_ALWAYS_DAY)
							{
								 bAeNight =TRUE;
								 //SensorState = SENSOR_STATE_NIGHT;
								 bNightStatisticDone=FALSE;
								 u8NightIndex =0;
								 AEToDayCnt =0;
								 AEToNightCnt =0;
								 TryCutOpenDelay =0;
								 bTryCutOpen =FALSE;
								 isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_NIGHT);
								 
								 printf("wwww9732 enter night state sensor,bAe %d\n",bAeNight);
							}

							 ToNightCnt=0;
						}
					
					 }
					 else
						 ToNightCnt =0;
 				 }
                 else if(bAeNight)
                 {
                 	 if(bCheckNightStatus==FALSE)
					 {
						 bCheckNightStatus= TRUE;
						 jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);
						 printf("star light night init ok\n");
						
					 }
					 if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
					 {
						 jv_sensor_low_frame_inner(0,bNightLowframe);
						 bCheckNightLowFrameStatus=bNightLowframe;
						 printf("star light night low frame change..\n");
					 }
					 bCheckDayStatus =FALSE;
                 	 
					 if(bNightStatisticDone==FALSE&&jv_sensor_get_ircut_staus())
					 {
						 u8NightIndex++;
						 if(u8NightIndex>=6)
						 {
						 
							 ExposureTbl[u8NightIndex-6] = jv_exp;
							 //LineTbl[u8NightIndex-4] = expTime;
							 IsoTbl[u8NightIndex-6] = u32ISO;
							 if(u8NightIndex>=6)
							 {
								 bNightStatisticDone =TRUE; 							 
								 Exp_Night = ExposureTbl[0];//+ExposureTbl[1])/2;
								// LINE_Night = LineTbl[0];//+LineTbl[1])/2;
								 ISO_Night =   IsoTbl[0];//+ IsoTbl[1])/2;
								 printf("ov9732 night Statistic data :Exp_Night %d,isoNIGHT %d\n",\
									 Exp_Night,ISO_Night);
								 u8NightIndex =0;

								 if(Exp_Night<=450*1*64 )
								 	bSmallScene =TRUE;
								 else							 	
							 		bSmallScene =FALSE;
								 
								u32ExpDayDelta = Exp_Night/3;

								if(Exp_Night <= 100*1*64 )
								{
									  u32ExpDayDelta = Exp_Night/10;
									  printf("11111 %d\n",u32ExpDayDelta/64);
								}
								else if(Exp_Night <= 250*1*64 )
								{
									  u32ExpDayDelta = Exp_Night/8;
									    printf("22222 %d\n",u32ExpDayDelta/64);
								}
								else if(Exp_Night <= 600*1*64 )
								{
									  u32ExpDayDelta = Exp_Night/7;
									  printf("33333 %d\n",u32ExpDayDelta/64);
								}
								else if(Exp_Night <= 970*1*64 )
								{
									  u32ExpDayDelta = Exp_Night/6;
									  printf("4444 %d\n",u32ExpDayDelta/64);
								}

								else if(Exp_Night <= 1500*1*64 )
								{
									  u32ExpDayDelta = Exp_Night/5;
									  printf("5555 %d\n",u32ExpDayDelta/64);
								}
								else if(Exp_Night <= 1944*1*64 )
								{
									  u32ExpDayDelta = Exp_Night/4;
									  printf("6666 %d\n",u32ExpDayDelta/64);
								}
								else if(Exp_Night <= 1944*2*64 )
								{
									  u32ExpDayDelta = Exp_Night/3;
									  printf("6666 %d\n",u32ExpDayDelta/64);
								}
								
								
								
								if(Exp_Night >= 1944*16*64 )
								{
									 u32ExpDayDelta = Exp_Night*5/10;
									 printf("8888 %d\n",u32ExpDayDelta/64);
								}
								
								else if(Exp_Night >= 1944*8*64 )
								{
									 u32ExpDayDelta = Exp_Night*5/10;
									 printf("999 %d\n",u32ExpDayDelta/64);
								}
								
								else if(Exp_Night >= 1944*4*64 )
								{
									 u32ExpDayDelta = Exp_Night*4/10;
									 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
								}
								if(u32ExpDayDelta < 5*64)
									u32ExpDayDelta =5*64;
									
							 }
					 
						   }
					 
					 }

					 if(bTryCutOpen)
					 {
					 	TryCutOpenDelay++;
						printf("9732try cut open state TryCutOpenDelay %d exp: %d \n",TryCutOpenDelay,jv_exp/64);

						if(TryCutOpenDelay>=5)
						{
							if(u32ISO <= 1200)
							{
								AEToDayCnt ++;
								printf("9732 todayday try cut open state AEToDayCnt  %d\n",AEToDayCnt);
							}
							else
								AEToDayCnt =0;


							if(u32ISO >= 1800)
							{
								AEToNightCnt++;
								printf("9732 tonight ngiht try cut open state AEToNightCnt  %d\n",AEToNightCnt);
							}
							else
								AEToNightCnt =0;
							
								
							if(AEToDayCnt>=1)
							{
								//success 切换到白天
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY);
								AEToDayCnt =0;
								AEToNightCnt =0;
								TryCutOpenDelay =0;
								bTryCutOpen =FALSE;	
								printf("aaaaaaaaa enter day okkkkkk\n");
								bNightStatisticDone =FALSE;
								bAeNight =FALSE;
							}
							else if(AEToNightCnt>=2)
							{
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY_PRE);
								jv_sensor_switch_ircut_inner(TRUE);// failed ... 进入黑夜cut状态
								printf("bbbbbbb enter day okkkkkk\n");
								AEToDayCnt =0;
								AEToNightCnt =0;
								
								TryCutOpenDelay =0;
								bTryCutOpen =FALSE;	
								bNightStatisticDone =FALSE;

							}
							else if(TryCutOpenDelay>=9) //等待
							{	isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY_PRE);
								jv_sensor_switch_ircut_inner(TRUE);//failed... 进入黑夜cut状态
								printf("cccccccccc enter day okkkkkk\n");
								
								AEToDayCnt =0;
								TryCutOpenDelay =0;
								bTryCutOpen =FALSE;	
								AEToNightCnt =0;
								bNightStatisticDone =FALSE;

							}

							


						}

					 }
			 
					 
					 
					 if(bNightStatisticDone&&jv_exp<=970*10*64&&bTryCutOpen ==FALSE)
					 {
						//printf("@@@@@@@@u32ExpDayDelta@@@@ is %d small scene %d\n",u32ExpDayDelta/64,bSmallScene);
						if(bSmallScene)
						{
							if(((jv_exp + u32ExpDayDelta)<= Exp_Night)||( jv_exp >= (Exp_Night+u32ExpDayDelta)))
								AEToDayCnt ++;
							else 								
								AEToDayCnt =0;

						}
						else
						{
							if((jv_exp + u32ExpDayDelta)<= Exp_Night)
								AEToDayCnt ++;
							else 								
								AEToDayCnt =0;
						}

						if(AEToDayCnt>=12) //尝试关闭cut
						{
							printf("try to close cut to verify light...\n");
							isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY_PRE);
							ISP_EXPOSURE_ATTR_S stExpAttr;
							HI_MPI_ISP_GetExposureAttr(0, &stExpAttr);
							stExpAttr.stAuto.u8Speed =96;
							stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 2;
							stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
							HI_MPI_ISP_SetExposureAttr(0, &stExpAttr);
							
							bCheckNight =TRUE;
							
							jv_sensor_switch_ircut_inner(FALSE);//只切换cut
							
							bTryCutOpen = TRUE;
							AEToDayCnt =0;
									 
					 	}

				 	 }               
                    
               }
			}
			else
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode)
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);	
						printf("no star night init ok\n");
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("no star night low frame change \n");
					}
					bCheckDayStatus =FALSE;
				}
				else
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("no star day init ok\n");
					}
					
				}



			}
			pthread_mutex_unlock(&low_frame_mutex); 
                
		}
		else if(sensor == SENSOR_OV2735
				&& (bCheckNightDayUsingAE ==TRUE)
				&& (HWTYPE_MATCH(HW_TYPE_C8) || HWTYPE_MATCH(HW_TYPE_V3) || HWTYPE_MATCH(HW_TYPE_V6) 
						||HWTYPE_MATCH(HW_TYPE_C8A) || HWTYPE_MATCH(HW_TYPE_C3) || HWTYPE_MATCH(HW_TYPE_C5)))
		{
			int bNight;
			unsigned int expthrd ;
			unsigned int  lumthrd;
			light_ae_node aeNode ;
			int ret = 0;
			pthread_mutex_lock(&low_frame_mutex);

			if(sensor == SENSOR_OV2735)
				aeNode = light_ae_list_ov2735[LightAeIndex];
			expthrd = aeNode.ExpThrd;
			lumthrd = aeNode.LumThrd;
			//blow = aeNode.bLow;

			jv_exp_thresh.ToNightExpThresh = expthrd;			
			jv_exp_thresh.ToNightLumThresh = lumthrd;
			
			if(isp_get_std_fps()>25) //30 fps锟斤拷锟斤拷锟?
				jv_exp_thresh.ToNightExpThresh = jv_exp_thresh.ToNightExpThresh*25/30;

			
			
			if(StarLightEnable)
			{
				 ret = jv_sensor_CheckDay_By_Sensor(stAvm,jv_exp,Night_To_Day_ThrLuma,Day_To_Night_ThrLuma,pSoft_Light_Sensitive);
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode == FALSE)//锟斤拷锟斤拷
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//锟斤拷锟斤拷锟斤拷锟斤拷锟叫伙拷锟斤拷锟斤拷锟斤拷牡锟揭伙拷锟街∑碉拷锟绞硷拷锟斤拷透锟斤拷锟斤拷锟斤拷锟斤拷始锟斤拷
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("star light day init\n");
					}
					if(SensorState==SENSOR_STATE_DAY_FULL_FRAMRATE&&jv_exp>=jv_exp_thresh.DayLowFrameExpThresh_0)
					{
						low_frame_cnt++;
						if(low_frame_cnt>=1)
						{
							isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_FPS_CHANGE);
							isp_set_low_fps(jv_exp_thresh.DayLowFrameRatio);
							SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
							low_frame_cnt=0;
							printf("yyyyyy enter half framerate sensor state %d\n",SensorState);
						}
					}
					else 
						low_frame_cnt=0;
					if((SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)&&(jv_exp <= jv_exp_thresh.DayFullFrameExpThresh))
					{
						full_frame_cnt++;
						
						if(full_frame_cnt>=4)
						{
							isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_FPS_CHANGE);
							isp_set_low_fps(FULL_FRAMERATE_RATIO);
							SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
							printf("yyyyyyyy enter full framerate sensor state %d\n",SensorState);
							full_frame_cnt=0;
						 }
					
					 }
					 else
						 full_frame_cnt =0;


					//if(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE&&sensor == SENSOR_OV2710)
					//{
						//jv_exp_thresh.ToNightExpThresh = 2654*32*64;
						//if(jv_sensor_get_vi_framerate()>25)
						//if(isp_get_std_fps()>25) 
							//jv_exp_thresh.ToNightExpThresh = 2654*25*64;
					//}
					
					 if((bAeNight ==FALSE)&& ret > 0)
					 {
						ToNightCnt++;
						if(ToNightCnt>=1)
						{

							if(DayNightMode!=DAYNIGHT_ALWAYS_DAY)
							{
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_NIGHT);
								bAeNight =TRUE;
								SensorState = SENSOR_STATE_NIGHT;
								printf("zzzyyyyyy enter night state sensor state  %d,bAe %d\n",SensorState,bAeNight);
								  
								//bNightStatisticDone=FALSE;
								u8NightIndex =0;
								AEToDayCnt =0;
								AEToNightCnt =0;
							}
							 ToNightCnt=0;
						}
					
					 }
					 else
						 ToNightCnt =0;
				}
				else if(bAeNight) //night =true
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);
						printf("star light night init ok\n");
						
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实锟斤拷锟斤拷夜锟皆讹拷锟斤拷帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("star light night low frame change..\n");
					}
					bCheckDayStatus =FALSE;
					if(ret == FALSE)
						isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY);
					
					bAeNight = ret;//jv_sensor_CheckDay_By_Sensor(stAvm,jv_exp,Night_To_Day_ThrLuma,Day_To_Night_ThrLuma,pSoft_Light_Sensitive);
					 
			   }
			}
			else //锟斤拷锟角癸拷
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode)
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);	
						printf("no star night init ok\n");
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实锟斤拷锟斤拷夜锟皆讹拷锟斤拷帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("no star night low frame change \n");
					}
					bCheckDayStatus =FALSE;
				}
				else
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//锟斤拷锟斤拷锟斤拷锟斤拷锟叫伙拷锟斤拷锟斤拷锟斤拷牡锟揭伙拷锟街∑碉拷锟绞硷拷锟斤拷透锟斤拷锟斤拷锟斤拷锟斤拷始锟斤拷
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("no star day init ok\n");
					}
					
				}
				
			

			}
			pthread_mutex_unlock(&low_frame_mutex); 
				
		}
		else if(sensor==SENSOR_SC2235&& bCheckNightDayUsingAE==TRUE)
		{
            int bNight;
			BOOL blow = TRUE;
			unsigned int expthrd;
			unsigned int  lumthrd;
			light_ae_node aeNode;
            pthread_mutex_lock(&low_frame_mutex);

			//if(sensor==SENSOR_SC2235)
            {
			    aeNode = light_ae_list_sc2235[LightAeIndex];
			
				expthrd = aeNode.ExpThrd;
				lumthrd = aeNode.LumThrd;
				blow = aeNode.bLow;

				jv_exp_thresh.ToNightExpThresh = expthrd;

						
				jv_exp_thresh.ToNightLumThresh = lumthrd;
            }
			//if(JV_ISP_COMM_Get_StdFps()>25) //30 fps情况下
			if( isp_get_std_fps()>25)
				jv_exp_thresh.ToNightExpThresh = jv_exp_thresh.ToNightExpThresh*25/30;

			if(StarLightEnable) 
			{
				//printf("SC2235\n");
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode)
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);
						printf("star light night init ok\n");
						
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("star light night low frame change..\n");
					}
					bCheckDayStatus =FALSE;
					
				    if(bAeNight)
					{
						bNight =jv_adc_read();
						if(bNight)
							AdcDayCnt=0;
						else
						{
							AdcDayCnt++;
							if(AdcDayCnt>=4)
							{
								bAeNight =FALSE;
								AdcDayCnt =0;
							}				
						}
				 
					}	
					if(bAeNight ==FALSE&&SensorState!=SENSOR_STATE_DAY_FULL_FRAMRATE) //为了保证回到初始状态，其实可以不设定sensor_state_night状态，但是好为了跟踪
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						printf("star light night to full framerate\n");
						
					}

				}
				else //白天模式
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("star light day init\n");
					}
					else
					{
						if(SensorState==SENSOR_STATE_DAY_FULL_FRAMRATE&&jv_exp>=jv_exp_thresh.DayLowFrameExpThresh_0)
						{
							low_frame_cnt++;
		                    if(low_frame_cnt>=1&& blow)
		                    {
		                    	//JV_ISP_COMM_Set_LowFps(jv_exp_thresh.DayLowFrameRatio);
		                    	isp_set_low_fps(jv_exp_thresh.DayLowFrameRatio);
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
								//JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
								isp_set_low_fps(FULL_FRAMERATE_RATIO);
								SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
								printf("enter full framerate sensor state %d\n",SensorState);
								full_frame_cnt=0;
							}
						
						}
						else
							full_frame_cnt =0;
						if((SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)&&(isp_check_low_fps()==FALSE))//不应该发生，但还是确保下，有可能会受上层干扰
						{
							isp_set_low_fps(jv_exp_thresh.DayLowFrameRatio);
						}
						if((bAeNight==FALSE)&&(jv_exp >= jv_exp_thresh.ToNightExpThresh)&&stAvm<=jv_exp_thresh.ToNightLumThresh)
						{
				 				//printf("to night tresh is %d lumthrd is %d\n",jv_exp_thresh.ToNightExpThresh,jv_exp_thresh.ToNightLumThresh);
				 			ToNightCnt++;
							if(ToNightCnt>=2)
							{

								bNight =jv_adc_read();
								//printf("bNight %d\n",bNight);
								if(bNight&&(DayNightMode ==DAYNIGHT_AUTO))
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

					}
			
				}
				
			} 
			else //非星光
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode)
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);	
						printf("no star night init ok\n");
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("no star night low frame change \n");
					}
					bCheckDayStatus =FALSE;
				}
				else
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("no star day init ok\n");
					}
					
				}
				
				
			}
			pthread_mutex_unlock(&low_frame_mutex); 
                
		}
		else if((sensor == SENSOR_MN34227||sensor == SENSOR_OV2710||((sensor == SENSOR_OV2735)&&(HWTYPE_NOT_MATCH(HW_TYPE_C8)))||sensor == SENSOR_OV9750)&&(bCheckNightDayUsingAE ==TRUE))
		{
            int bNight;
			unsigned int expthrd ;
			unsigned int  lumthrd;
			light_ae_node aeNode ;
            pthread_mutex_lock(&low_frame_mutex);

			if(sensor == SENSOR_OV2710)
				aeNode = light_ae_list_ov2710[LightAeIndex];
			else if(sensor == SENSOR_OV2735)
				aeNode = light_ae_list_ov2735[LightAeIndex];
			else if(sensor == SENSOR_OV9750)
				aeNode = light_ae_list_ov9750[LightAeIndex];
			else if(sensor == SENSOR_MN34227)
				aeNode = light_ae_list_mn34227[LightAeIndex];
			else if(sensor == SENSOR_SC2235)
				aeNode = light_ae_list_sc2235[LightAeIndex];
			expthrd = aeNode.ExpThrd;
			lumthrd = aeNode.LumThrd;
			//blow = aeNode.bLow;

			jv_exp_thresh.ToNightExpThresh = expthrd;			
			jv_exp_thresh.ToNightLumThresh = lumthrd;
			
			if(isp_get_std_fps()>25) //30 fps情况下
				jv_exp_thresh.ToNightExpThresh = jv_exp_thresh.ToNightExpThresh*25/30;
			
			
			if(StarLightEnable)
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
			 	if(bNightMode == FALSE)//白天
			 	{
			 		bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("star light day init\n");
					}
					if(SensorState==SENSOR_STATE_DAY_FULL_FRAMRATE&&jv_exp>=jv_exp_thresh.DayLowFrameExpThresh_0)
					{
						low_frame_cnt++;
	                    if(low_frame_cnt>=1)
	                    {
							isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_FPS_CHANGE);
	                    	isp_set_low_fps(jv_exp_thresh.DayLowFrameRatio);
	                        SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
	                        low_frame_cnt=0;
	                        printf("yyyyyy enter half framerate sensor state %d\n",SensorState);
	                    }
	                }
	                else 
	                    low_frame_cnt=0;
	                if((SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE)&&(jv_exp <= jv_exp_thresh.DayFullFrameExpThresh))
					{
						full_frame_cnt++;
						
						if(full_frame_cnt>=4)
						{
							isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_FPS_CHANGE);
							isp_set_low_fps(FULL_FRAMERATE_RATIO);
							SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
							printf("yyyyyyyy enter full framerate sensor state %d\n",SensorState);
							full_frame_cnt=0;
						 }
					
					 }
					 else
						 full_frame_cnt =0;


					//if(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE&&sensor == SENSOR_OV2710)
					//{
						//jv_exp_thresh.ToNightExpThresh = 2654*32*64;
						//if(jv_sensor_get_vi_framerate()>25)
						//if(isp_get_std_fps()>25) 
							//jv_exp_thresh.ToNightExpThresh = 2654*25*64;
					//}
					 
	                 if((bAeNight ==FALSE)&&(jv_exp >= jv_exp_thresh.ToNightExpThresh)&&stAvm<=jv_exp_thresh.ToNightLumThresh)
					 {
					 	ToNightCnt++;
						if(ToNightCnt>=2)
						{

							if(DayNightMode!=DAYNIGHT_ALWAYS_DAY)
							{
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_NIGHT);
								 bAeNight =TRUE;
								 SensorState = SENSOR_STATE_NIGHT;
								  printf("zzzyyyyyy enter night state sensor state  %d,bAe %d\n",SensorState,bAeNight);
								  
								  bNightStatisticDone=FALSE;
								  u8NightIndex =0;
								  AEToDayCnt =0;
								  AEToNightCnt =0;
								  TryCutOpenDelay =0;
								  bTryCutOpen =FALSE;
							}

							 ToNightCnt=0;
						}
					
					 }
					 else
						 ToNightCnt =0;
			 	}
				else if(bAeNight) //night =true
                {
                    if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);
						printf("star light night init ok\n");
						
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("star light night low frame change..\n");
					}
					bCheckDayStatus =FALSE;
					
                	 int DelayMaxtick =4;
					 if(sensor ==SENSOR_OV2735||sensor ==SENSOR_OV9750)
					 	DelayMaxtick = 8;
					 if(bNightStatisticDone==FALSE&&jv_sensor_get_ircut_staus())
					 {
						 u8NightIndex++;
						 printf("u8NightIndex is %d iso %d jv_exp %d exptime %d\n",u8NightIndex,u32ISO,jv_exp,expTime);
						 if(u8NightIndex>=DelayMaxtick)
						 {
						 
							 ExposureTbl[0] = jv_exp;
							 LineTbl[0] = expTime;
							 IsoTbl[0] = u32ISO;
							 if(u8NightIndex>=DelayMaxtick)
							 {
								 bNightStatisticDone =TRUE; 							 
								 Exp_Night = ExposureTbl[0];//+ExposureTbl[1])/2;
								 LINE_Night = LineTbl[0];//+LineTbl[1])/2;
								 ISO_Night =   IsoTbl[0];//+ IsoTbl[1])/2;
								 printf("ii night Statistic data :Exp_Night %d,Line_Night %d,isoNIGHT %d\n",\
									 Exp_Night,LINE_Night,ISO_Night);
								 u8NightIndex =0;
								 if(sensor == SENSOR_OV2710)
								 {
									
									 if(Exp_Night<=1300*1*64 )
									 	bSmallScene =TRUE;
									 else							 	
								 		bSmallScene =FALSE;
								 }
								 else if(sensor == SENSOR_OV2735)
								 {	

									if(Exp_Night<=1000*1*64 )
									 	bSmallScene =TRUE;
									 else							 	
								 		bSmallScene =FALSE;
								 }
								 else if(sensor == SENSOR_OV9750)
								 {	

									if(Exp_Night<=2372*1*64 )
									 	bSmallScene =TRUE;
									 else							 	
								 		bSmallScene =FALSE;
								 }
								 
								u32ExpDayDelta = Exp_Night/2;
								
								if(sensor == SENSOR_OV2710)
								{
									if(Exp_Night <= 100*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/12;
										  printf("11111 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 250*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/10;
										    printf("22222 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 600*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/7;
										  printf("33333 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 1325*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/5;
										  printf("4444 %d\n",u32ExpDayDelta/64);
									}

									else if(Exp_Night <= 2000*1*64 )
									{
										  u32ExpDayDelta = Exp_Night*3/10;
										  printf("5555 %d\n",u32ExpDayDelta/64);
									}
									
									else if(Exp_Night <= 2654*1*64 )
									{
										  u32ExpDayDelta = Exp_Night*4/10;
										  printf(" 6666 %d\n",u32ExpDayDelta/64);
									}

									else if(Exp_Night <= 2654*2*64 )
									{
										  u32ExpDayDelta = Exp_Night/2;
										  printf("7777 %d\n",u32ExpDayDelta/64);
									}

									
									
									if(ISO_Night>= 1600 )
									{
										 u32ExpDayDelta = Exp_Night*7/10;
										 printf("8888 %d\n",u32ExpDayDelta/64);
									}
									
									else if(ISO_Night >= 800)
									{
										 u32ExpDayDelta = Exp_Night*6/10;
										 printf("999 %d\n",u32ExpDayDelta/64);
									}
									
									else if(ISO_Night >= 400)
									{
										 u32ExpDayDelta = Exp_Night*6/10;
										 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
									}
									else if(ISO_Night >= 200)
									{
										 u32ExpDayDelta = Exp_Night*5/10;
										 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
									}
								}
								else if(sensor == SENSOR_OV2735)
								{
									if(Exp_Night <= 150*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/12;
										  printf("11111 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 450*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/10;
										    printf("22222 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 800*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/8;
										  printf("33333 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 1629*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/7;
										  printf("4444 %d\n",u32ExpDayDelta/64);
									}

									else if(Exp_Night <= 2300*1*64 )
									{
										  u32ExpDayDelta = Exp_Night*2/10;
										  printf("5555 %d\n",u32ExpDayDelta/64);
									}
									
									else if(Exp_Night <= 3262*1*64 )
									{
										  u32ExpDayDelta = Exp_Night*3/10;
										  printf(" 6666 %d\n",u32ExpDayDelta/64);
									}

									else if(Exp_Night <= 3262*2*64 )
									{
										  u32ExpDayDelta = Exp_Night/2;
										  printf("7777 %d\n",u32ExpDayDelta/64);
									}

									
									
									if(ISO_Night>= 1600 )
									{
										 u32ExpDayDelta = Exp_Night*5/10;
										 printf("8888 %d\n",u32ExpDayDelta/64);
									}
									
									else if(ISO_Night >= 800)
									{
										 u32ExpDayDelta = Exp_Night*5/10;
										 printf("999 %d\n",u32ExpDayDelta/64);
									}
									
									else if(ISO_Night >= 400)
									{
										 u32ExpDayDelta = Exp_Night*5/10;
										 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
									}
									else if(ISO_Night >= 200)
									{
										 u32ExpDayDelta = Exp_Night*4/10;
										 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
									}
								}
								else if(sensor == SENSOR_OV9750)
								{
									if(Exp_Night <= 218*1*64 )    //1629 --3262      9750:2372  --4748
									{
										  u32ExpDayDelta = Exp_Night/10;
										  printf("11111 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 655*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/9;
										    printf("22222 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 1165*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/7;
										  printf("33333 %d\n",u32ExpDayDelta/64);
									}
									else if(Exp_Night <= 2372*1*64 )
									{
										  u32ExpDayDelta = Exp_Night/5;
										  printf("4444 %d\n",u32ExpDayDelta/64);
									}

									else if(Exp_Night <= 3500*1*64 )
									{
										  u32ExpDayDelta = Exp_Night*3/10;
										  printf("5555 %d\n",u32ExpDayDelta/64);
									}
									
									else if(Exp_Night <= 4748*1*64 )
									{
										  u32ExpDayDelta = Exp_Night*4/10;
										  printf(" 6666 %d\n",u32ExpDayDelta/64);
									}

									else if(Exp_Night <= 4748*2*64 )
									{
										  u32ExpDayDelta = Exp_Night/2;
										  printf("7777 %d\n",u32ExpDayDelta/64);
									}

									
									
									if(ISO_Night>= 1600 )
									{
										 u32ExpDayDelta = Exp_Night*7/10;
										 printf("8888 %d\n",u32ExpDayDelta/64);
									}
									
									else if(ISO_Night >= 800)
									{
										 u32ExpDayDelta = Exp_Night*6/10;
										 printf("999 %d\n",u32ExpDayDelta/64);
									}
									
									else if(ISO_Night >= 400)
									{
										 u32ExpDayDelta = Exp_Night*6/10;
										 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
									}
									else if(ISO_Night >= 200)
									{
										 u32ExpDayDelta = Exp_Night*5/10;
										 printf("AAAAAAA %d\n",u32ExpDayDelta/64);
									}
								}
								if(u32ExpDayDelta < 4*64)
									u32ExpDayDelta =4*64;
									
							 }
					 
						   }
					 
					 }

					 if(bTryCutOpen)
					 {
					 	TryCutOpenDelay++;
						printf("zzztry cut open state TryCutOpenDelay %d exp: %d \n",TryCutOpenDelay,jv_exp/64);
						//printf("ggggggg iso iso %d\n",u32ISO);

						if(TryCutOpenDelay>=5)
						{
							if(u32ISO <= 1200)
							{
								AEToDayCnt ++;
								printf("vvvvvvvvv todayday try cut open state AEToDayCnt  %d\n",AEToDayCnt);
							}
							else
								AEToDayCnt =0;


							if(u32ISO  >= 2000)
							{
								AEToNightCnt++;
								printf("vvvvvvvvv tonight ngiht try cut open state AEToNightCnt  %d\n",AEToNightCnt);
							}
							else
								AEToNightCnt =0;
							
								
							if(AEToDayCnt>=1)
							{
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY);
								//success 切换到白天
								AEToDayCnt =0;
								AEToNightCnt =0;
								TryCutOpenDelay =0;
								bTryCutOpen =FALSE;	
								printf("aaaaaaaaa enter day okkkkkk\n");
								bNightStatisticDone =FALSE;
								bAeNight =FALSE;
							}
							else if(AEToNightCnt>=2)
							{
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY_PRE);
								jv_sensor_switch_ircut_inner(TRUE);// failed ... 进入黑夜cut状态
								printf("bbbbbbb enter night again okkkkkk\n");
								AEToDayCnt =0;
								AEToNightCnt =0;
								
								TryCutOpenDelay =0;
								bTryCutOpen =FALSE;	
								bNightStatisticDone =FALSE;

								ISP_EXPOSURE_ATTR_S stExpAttr;
								HI_MPI_ISP_GetExposureAttr(0, &stExpAttr);
								//stExpAttr.stAuto.u8Speed =200;
								if(sensor ==SENSOR_OV2735)
									stExpAttr.stAuto.u8Speed =120;
								else if(sensor ==SENSOR_OV2710)
									stExpAttr.stAuto.u8Speed =100;
								else if(sensor ==SENSOR_OV9750)
									stExpAttr.stAuto.u8Speed =120;
								stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 2;
								stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
								HI_MPI_ISP_SetExposureAttr(0, &stExpAttr);
								
								bCheckNight =TRUE;
								usleep(1000*1000);

							}
							else if(TryCutOpenDelay>=9) //等待
							{
								isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY_PRE);
								jv_sensor_switch_ircut_inner(TRUE);//failed... 进入黑夜cut状态
								printf("cccccccccc enter day okkkkkk\n");
								
								AEToDayCnt =0;
								TryCutOpenDelay =0;
								bTryCutOpen =FALSE;	
								AEToNightCnt =0;
								bNightStatisticDone =FALSE;

								ISP_EXPOSURE_ATTR_S stExpAttr;
								HI_MPI_ISP_GetExposureAttr(0, &stExpAttr);
								//stExpAttr.stAuto.u8Speed =200;
								if(sensor ==SENSOR_OV2735)
									stExpAttr.stAuto.u8Speed =120;
								else if(sensor ==SENSOR_OV2710)
									stExpAttr.stAuto.u8Speed =100;
								else if(sensor ==SENSOR_OV9750)
									stExpAttr.stAuto.u8Speed =120;
								stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 2;
								stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
								HI_MPI_ISP_SetExposureAttr(0, &stExpAttr);
								
								bCheckNight =TRUE;
								usleep(1000*1000);

							}

							


						}

					 }
			 
					 
					 int ToDayExpLimit =1325*7*64;
					 if(sensor== SENSOR_OV2710)
					 	ToDayExpLimit =1325*12*64;
					 else if(sensor ==SENSOR_OV2735)
					 	ToDayExpLimit =1629*16*64;
					 else if(sensor ==SENSOR_OV9750)
					 	ToDayExpLimit =2372*8*64;
					 if(bNightStatisticDone&&jv_exp<=ToDayExpLimit&&bTryCutOpen ==FALSE)
					 {
						//printf("@@@@@@@@u32ExpDayDelta@@@@ is %d small scene %d\n",u32ExpDayDelta/64,bSmallScene);
						if(bSmallScene)
						{
							if(((jv_exp + u32ExpDayDelta)<= Exp_Night)||( jv_exp >= (Exp_Night+u32ExpDayDelta)))
								AEToDayCnt ++;
							else 								
								AEToDayCnt =0;

						}
						else
						{
							if((jv_exp + u32ExpDayDelta)<= Exp_Night)
								AEToDayCnt ++;
							else 								
								AEToDayCnt =0;
						}

						if(AEToDayCnt>=12) //尝试关闭cut
						{
							isp_cut_trigger_inner(TRUE,CUT_TRIGGER_TYPE_TO_DAY_PRE);
							printf("try to close cut to verify light...\n");

							ISP_EXPOSURE_ATTR_S stExpAttr;
							HI_MPI_ISP_GetExposureAttr(0, &stExpAttr);
							
							if(sensor ==SENSOR_OV2735)
								stExpAttr.stAuto.u8Speed =120;
							else if(sensor ==SENSOR_OV2710)
								stExpAttr.stAuto.u8Speed =100;
							else if(sensor ==SENSOR_OV9750)
									stExpAttr.stAuto.u8Speed =120;
							
							stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 2;
							stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
							HI_MPI_ISP_SetExposureAttr(0, &stExpAttr);
							
							bCheckNight =TRUE;
							
							jv_sensor_switch_ircut_inner(FALSE);//只切换cut
							
							bTryCutOpen = TRUE;
							AEToDayCnt =0;
									 
					 	}

				 	 }               
                    
               }
			}
			else //非星光
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode)
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);	
						printf("no star night init ok\n");
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("no star night low frame change \n");
					}
					bCheckDayStatus =FALSE;
				}
				else
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						isp_set_low_fps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("no star day init ok\n");
					}
					
				}
				
			

			}
			pthread_mutex_unlock(&low_frame_mutex); 
                
		}
		else
		{
			if(bNightMode)
			{
				if(bCheckNightStatus==FALSE)
				{
					bCheckNightStatus= TRUE;
					jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);	
					printf("no star night init ok\n");
				}
				if(bCheckNightLowFrameStatus!=bNightLowframe) //实现日夜自动降帧
				{
					jv_sensor_low_frame_inner(0,bNightLowframe);
					bCheckNightLowFrameStatus=bNightLowframe;
					printf("no star night low frame change \n");
				}
				bCheckDayStatus =FALSE;
			}
			else
			{
				bCheckNightStatus =FALSE;
				if(bCheckDayStatus==FALSE)//用来进行切换到白天的第一次帧频初始化和各类参数初始化
				{
					SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
					isp_set_low_fps(FULL_FRAMERATE_RATIO);
					bCheckDayStatus =TRUE;
					printf("no star day init ok\n");
				}
					
			}
		}





		
		if( jv_exp == jv_exp_last )
			continue;
		
		u32ISOTmp = u32ISO / 100;
		for(u8Index = 0; u8Index < MAX_ISO_TBL_INDEX;u8Index++)        
		{            
			if(1 == u32ISOTmp)            
			{                
				break;            
			}            
			u32ISOTmp >>= 1;     
		}
		if(sensor== SENSOR_OV9750|| sensor == SENSOR_OV9750m)
		{
			
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);		
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);               	 
			punNrParam.stNRParam_V1.s32CSFStr = 32;
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			punNrParam.stNRParam_V1.s32TFStrMax = 14;
			if(u32ISO>800)
				punNrParam.stNRParam_V1.s32CTFstr =16;
			else if(u32ISO<650)
				punNrParam.stNRParam_V1.s32CTFstr =0;
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);
		}
		else if(sensor== SENSOR_OV9732)
		{
		/*
			if(ov9732_gamma==2&&u32ISO>=3200)
			{	
				ISP_GAMMA_ATTR_S pstGammaAttr;
				pstGammaAttr.bEnable = HI_TRUE;
				pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
				memcpy(&pstGammaAttr.u16Table,&V200_LINE_DAY_LV1_GAMMA,257 * 2);
				ov9732_gamma =5;
				HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);
			}

			if(ov9732_gamma==5&&u32ISO<=2800)
			{	
				ISP_GAMMA_ATTR_S pstGammaAttr;
				pstGammaAttr.bEnable = HI_TRUE;
				pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
				memcpy(&pstGammaAttr.u16Table,&u16Gamma_ov9732_day,257 * 2);
				ov9732_gamma =2;
				HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);
			}*/
			
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);		
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);    
			punNrParam.stNRParam_V1.s32YPKStr = jv_IQ_AgcTableCalculate( YPK_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32CSFStr = 32;
			punNrParam.stNRParam_V1.s32TFStrMax = 14;
			if(u32ISO>1200)
				punNrParam.stNRParam_V1.s32CTFstr =16;
			else if(u32ISO<800)
				punNrParam.stNRParam_V1.s32CTFstr =0;
			if(u32ISO>2800)
				punNrParam.stNRParam_V1.s32YSFBriRat=24;
			else if(u32ISO<2400)
				punNrParam.stNRParam_V1.s32YSFBriRat=64;;
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);
		}

		if(sensor== SENSOR_OV2710)
		{
			
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);	
			punNrParam.stNRParam_V1.s32YPKStr = jv_IQ_AgcTableCalculate( YPK_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);               	 
			punNrParam.stNRParam_V1.s32CSFStr = 32;
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			punNrParam.stNRParam_V1.s32TFStrMax = 14;
			if(u32ISO>800)
				punNrParam.stNRParam_V1.s32CTFstr =16;
			else if(u32ISO<650)
				punNrParam.stNRParam_V1.s32CTFstr =0;
			
			punNrParam.stNRParam_V1.s32YPKStr =0;
			if(jv_exp <= 12800)
				punNrParam.stNRParam_V1.s32YPKStr =10;
			
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);
		}
		if(sensor== SENSOR_SC2135)
		{
			
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);	
			punNrParam.stNRParam_V1.s32YPKStr = jv_IQ_AgcTableCalculate( YPK_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);               	 
			punNrParam.stNRParam_V1.s32CSFStr = 32;
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			punNrParam.stNRParam_V1.s32TFStrMax = 14;
			if(u32ISO>800)
				punNrParam.stNRParam_V1.s32CTFstr =16;
			else if(u32ISO<650)
				punNrParam.stNRParam_V1.s32CTFstr =0;
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);
		}
        if (sensor == SENSOR_AR0130)
        {
            
            if(u32ISO<=200)
            {
                ysf=80;
                ytf=64;                 
            }
            else if(u32ISO>200&&u32ISO<=400)
            {
                ysf=88;
                ytf=70;                 
            }
            else if(u32ISO>400&&u32ISO<=1200)
            {
                ysf=96;
                ytf=76;                 
            }
            else if(u32ISO>1200&&u32ISO<=2400)
            {
                ysf=102;
                ytf=84;                 
            }
            else if(u32ISO>2400&&u32ISO<=3600)
            {
                ysf=110;
                ytf=92;                 
            }
            else if(u32ISO>3600&&u32ISO<=6400)
            {
                ysf=124;
                ytf=100;                 
            }
            else if(u32ISO>6400)
            {
                ysf=136;
                ytf=108;                    
            }

            if(u32ISO<=36)
                tfStrMax=12;
            else
                tfStrMax=14;
            HI_MPI_VPSS_GetNRParam(0,&punNrParam);
            punNrParam.stNRParam_V1.s32YSFStr=ysf;
            punNrParam.stNRParam_V1.s32YTFStr=ytf;
            punNrParam.stNRParam_V1.s32TFStrMax=tfStrMax;
            HI_MPI_VPSS_SetNRParam(0, &punNrParam);
            
        }  
		if(sensor== SENSOR_SC2045)
		{
			
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);	
			punNrParam.stNRParam_V1.s32YPKStr = jv_IQ_AgcTableCalculate( YPK_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO); 				 
			punNrParam.stNRParam_V1.s32CSFStr = 32;
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			punNrParam.stNRParam_V1.s32TFStrMax = 13;
			if(u32ISO>1400)
				punNrParam.stNRParam_V1.s32CTFstr =12;
			else if(u32ISO<800)
				punNrParam.stNRParam_V1.s32CTFstr =0;
			
			if(u32ISO < 500)
				punNrParam.stNRParam_V1.s32TFStrMax = 12;
			else if(u32ISO < 1800)
				punNrParam.stNRParam_V1.s32TFStrMax = 13;
			else 
				punNrParam.stNRParam_V1.s32TFStrMax = 14;
			
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);
		}
		if(sensor== SENSOR_OV2735)
		{
			
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);	
			punNrParam.stNRParam_V1.s32YPKStr = jv_IQ_AgcTableCalculate( YPK_TBL, u8Index, u32ISO);
			
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStrDlt = jv_IQ_AgcTableCalculate( YSFDLT_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStrDl= jv_IQ_AgcTableCalculate( YSFDL_TBL, u8Index, u32ISO);

			punNrParam.stNRParam_V1.s32YSmthStr= jv_IQ_AgcTableCalculate( YSMTH_TBL, u8Index, u32ISO);
			
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);  

			punNrParam.stNRParam_V1.s32CSFStr = jv_IQ_AgcTableCalculate( CSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32CTFstr = jv_IQ_AgcTableCalculate( CTF_TBL, u8Index, u32ISO); 
			
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			punNrParam.stNRParam_V1.s32TFStrMax = 14;

			if(u32ISO>10800)
				punNrParam.stNRParam_V1.s32TFStrMax = 14;
			if(u32ISO>=1500)
				punNrParam.stNRParam_V1.s32YSFBriRat=48;

			//if(punNrParam.stNRParam_V1.s32YSmthStr > 32)
				//punNrParam.stNRParam_V1.s32YSmthRat = 0;
			//else
			{
				punNrParam.stNRParam_V1.s32YSmthRat = 16;
				punNrParam.stNRParam_V1.s32YSmthStr =0;
			}

			punNrParam.stNRParam_V1.s32YPKStr =0;
			if(jv_exp <= 12800)
				punNrParam.stNRParam_V1.s32YPKStr =10;
 
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);


			ISP_EXPOSURE_ATTR_S tmpExpAttr;
			int evBias =1024;
			HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);

			evBias= tmpExpAttr.stAuto.u16EVBias ;

			if(bIspNight==FALSE)//白天
			{
				if(evBias ==1024)  //60-54(922)-48(820)
				{
				 	if(u32ISO>= 1800)
						evBias =922;
					if(u32ISO>= 5600)
						evBias =820;
				}
				else if(evBias ==922)
				{
				 	if(u32ISO<= 1000)
						evBias =1024;
					
					if(u32ISO>= 5600)
						evBias =820;
				}
				else if(evBias ==820)
				{
					if(u32ISO<= 3900)
						evBias =922;
					if(u32ISO<= 1000)
						evBias =1024;
				}	
			}
			else             //晚上
			{
				if(evBias ==1024)  //50-45(922)
				{
					if(u32ISO>= 2500)
						evBias =922;
				}
				else if(evBias ==922)
				{
					if(u32ISO<= 1500)
						evBias =1024;
				}	

			}
			tmpExpAttr.stAuto.u16EVBias = evBias;
			HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);

		}
		if(sensor== SENSOR_SC2235)
		{
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);	
			
			
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStrDlt = jv_IQ_AgcTableCalculate( YSFDLT_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStrDl= jv_IQ_AgcTableCalculate( YSFDL_TBL, u8Index, u32ISO);

			//punNrParam.stNRParam_V1.s32YSmthStr= jv_IQ_AgcTableCalculate( YSMTH_TBL, u8Index, u32ISO);
			
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);  

			punNrParam.stNRParam_V1.s32CSFStr = jv_IQ_AgcTableCalculate( CSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32CTFstr = jv_IQ_AgcTableCalculate( CTF_TBL, u8Index, u32ISO); 
			
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			punNrParam.stNRParam_V1.s32TFStrMax = 14;		
			punNrParam.stNRParam_V1.s32YSmthRat = 16;
			punNrParam.stNRParam_V1.s32YSmthStr =0;
			punNrParam.stNRParam_V1.s32YPKStr = 0;
			if(jv_exp<=12800)
				punNrParam.stNRParam_V1.s32YPKStr=7;
			
 
			HI_MPI_VPSS_SetNRParam(0,&punNrParam);



			ISP_EXPOSURE_ATTR_S tmpExpAttr;
			int evBias =1024;
			HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);

			evBias= tmpExpAttr.stAuto.u16EVBias ;
			
			if(bIspNight ==FALSE)//白天
			{
				if(evBias ==1024)  //60-54(922)-48(820)
				{
					if(u32ISO>= 1000)
						evBias =900;
					if(u32ISO>= 5600)
						evBias =780;
				}
				else if(evBias ==900)
				{
					if(u32ISO<= 400)
						evBias =1024;
					
					if(u32ISO>= 5600)
						evBias =780;
				}
				else if(evBias ==780)
				{
					if(u32ISO<= 3900)
						evBias =900;
					if(u32ISO<= 400)
						evBias =1024;
				}	
			}
			else			 //晚上
			{
				if(evBias ==1024)  //50-45(922)
				{
					if(u32ISO>= 2500)
						evBias =900;
				}
				else if(evBias ==900)
				{
					if(u32ISO<= 1500)
						evBias =1024;
				}	

			}
			tmpExpAttr.stAuto.u16EVBias = evBias;
			HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);

		}



			
		if (sensor == SENSOR_MN34227)
		{
			ISP_EXPOSURE_ATTR_S stExpAttr;
			VPSS_NR_PARAM_U punNrParam;
			HI_MPI_VPSS_GetNRParam(0,&punNrParam);	
			punNrParam.stNRParam_V1.s32YPKStr = jv_IQ_AgcTableCalculate( YPK_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YSFStr = jv_IQ_AgcTableCalculate( YSF_TBL, u8Index, u32ISO);
			punNrParam.stNRParam_V1.s32YTFStr = jv_IQ_AgcTableCalculate( YTF_TBL, u8Index, u32ISO);               	 
			punNrParam.stNRParam_V1.s32CSFStr = jv_IQ_AgcTableCalculate( CSF_TBL, u8Index, u32ISO); 
            punNrParam.stNRParam_V1.s32TFStrMax= jv_IQ_AgcTableCalculate( TFMAX_TBL, u8Index, u32ISO); 
            
			punNrParam.stNRParam_V1.s32YSFBriRat=64;
			//punNrParam.stNRParam_V1.s32TFStrMax = 13;
			//if(u32ISO > 3200)
				//punNrParam.stNRParam_V1.s32TFStrMax = 14;
			
			if(u32ISO>5000)
				punNrParam.stNRParam_V1.s32CTFstr = 12;
			else if(u32ISO<3500)
				punNrParam.stNRParam_V1.s32CTFstr = 0;	
			if(u32ISO>20000)
				punNrParam.stNRParam_V1.s32CTFstr = 16;

            //if(u32ISO>120000)
               // punNrParam.stNRParam_V1.s32YTFStrDlt=32;
           // else if(u32ISO<80000)
                //punNrParam.stNRParam_V1.s32YTFStrDlt=0;
            //if(u32ISO>204800)
               // punNrParam.stNRParam_V1.s32YTFStrDlt=64;

			punNrParam.stNRParam_V1.s32YPKStr =0;
			if(jv_exp <= 12800)
				punNrParam.stNRParam_V1.s32YPKStr =10;

			HI_MPI_VPSS_SetNRParam(0,&punNrParam);


            int evBias =0;
            if(u32ISO<=6000)
            {
                evBias=1024;
                                    
            }
            else if(u32ISO>=10000)
            {
                evBias=800;
            }
            
            
            ISP_EXPOSURE_ATTR_S tmpExpAttr;
            HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
            if(evBias>0)
                tmpExpAttr.stAuto.u16EVBias = evBias;
            HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);


			/*if(bWhiteLight)
			{
				
				ISP_DRC_ATTR_S pstDRCAttr;
				HI_MPI_ISP_GetDRCAttr(0, &pstDRCAttr);
					
				pstDRCAttr.bEnable = HI_TRUE;
				pstDRCAttr.enOpType = OP_TYPE_MANUAL;
				pstDRCAttr.stManual.u8Strength =75;
				pstDRCAttr.u8Asymmetry=2;
				pstDRCAttr.u8SecondPole=150;
				pstDRCAttr.u8Stretch=43;
				pstDRCAttr.stManual.u8LocalMixingBrigtht = jv_IQ_AgcTableCalculate( DRC_Bright, u8Index, u32ISO); 
				pstDRCAttr.stManual.u8LocalMixingDark =jv_IQ_AgcTableCalculate( DRC_Dark, u8Index, u32ISO); 
				HI_MPI_ISP_SetDRCAttr(0, &pstDRCAttr);

			}*/

            
         }
		//u32ISOLast=u32ISO;
		jv_exp_last =jv_exp;
			
	}	
	return NULL;

}
static void isp_helper_deinit(void)
{
	isp_helper_live=0;
	// sleep(2);
	pthread_join(thread,NULL);
	printf("%s..%d..xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx \n",__func__,__LINE__);
	return;
}

static void isp_helper_init(void)
{
	isp_helper_live=1;
	pthread_create(&thread, NULL, thread_isp_helper, NULL);
	// pthread_detach(thread);
}
void isp_helper_reload(void)
{
    isp_helper_live=1;
    sleep(1);
    isp_helper_init();
}
