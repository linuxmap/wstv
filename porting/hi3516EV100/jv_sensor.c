#include <jv_common.h>
#include <utl_timer.h>
#include <utl_common.h>
#include "hicommon.h"
#include <jv_sensor.h>
#include"3518_isp.h"
#include <mpi_ae.h>
#include <mpi_awb.h>
#include <jv_gpio.h>
#include <mpi_af.h>
#include <hi_comm_isp.h>
#include <mpi_isp.h>
#include "jv_smart_sensor.h"

static BOOL bAeNight=FALSE;
static   BOOL bWhiteMode = FALSE;
static HI_U32 u32EnCoeff = 1;

struct offset_s BR_Gain_Offset_HC8A_OV2735[] = {

		{0,0,64*800},
		{2,2,64*1400},
		{4,3,64*2000},
		{5,5,64*5000*64},
}; 

Soft_Light_Sensitive_t  Soft_Light_Sensitive_V200_HC8A_OV2735 = {
	
	.Max_Row = 15,
	.Max_Col = 17,
	.Statistical_Accuracy = 16,
	
	.Ratio_Rg_Ir = 268,
	.Ratio_Bg_Ir = 261,
    
	.Ratio_Rg_Natural_Light_Max = 206,
	.Ratio_Rg_Natural_Light_Min = 170,
	.Ratio_Bg_Natural_Light_Max = 183,
	.Ratio_Bg_Natural_Light_Min = 120,
	
    
	.Ratio_Rg_Natural_Light_Typcial = 199,	
	.Ratio_Bg_Natural_Light_Typical = 167,
	
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

Soft_Light_Sensitive_t * pSoft_Light_Sensitive_2735 = &Soft_Light_Sensitive_V200_HC8A_OV2735;

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
	BOOL bNewDay = FALSE;
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
					En_Bayer_AE_R_lum = (HI_U64)(stStat.stWBStat.stBayerStatistics.au16ZoneAvgR[row][col])*4096/65535;
					En_Bayer_AE_G_lum = (HI_U64)(stStat.stWBStat.stBayerStatistics.au16ZoneAvgG[row][col])*4096/65535;
					En_Bayer_AE_B_lum = (HI_U64)(stStat.stWBStat.stBayerStatistics.au16ZoneAvgB[row][col])*4096/65535;
					
				if(En_Bayer_AE_R_lum<Soft_Light_S->R_Limit_Value_Max && En_Bayer_AE_G_lum<Soft_Light_S->G_Limit_Value_Max &&  En_Bayer_AE_B_lum < Soft_Light_S->B_Limit_Value_Max  && \
					En_Bayer_AE_R_lum>Soft_Light_S->R_Limit_Value_Min && En_Bayer_AE_G_lum>Soft_Light_S->G_Limit_Value_Min && En_Bayer_AE_B_lum > Soft_Light_S->B_Limit_Value_Min)
				{
					
					Lum_Effect_Block_Num += 10;

					//En_Bayer_AE_R_lum = stStat.stWBStat.stBayerStatistics.au16ZoneAvgR[row][col];
					//En_Bayer_AE_G_lum = stStat.stWBStat.stBayerStatistics.au16ZoneAvgG[row][col];
					//En_Bayer_AE_B_lum = stStat.stWBStat.stBayerStatistics.au16ZoneAvgB[row][col];
					

					
				
					//lum_tmp = ((HI_U64)En_Bayer_AE_R_lum*257L + (HI_U64)En_Bayer_AE_G_lum*504L + (HI_U64)En_Bayer_AE_B_lum*98L + 16L*256L*1000L )/1000L*(HI_U64)u32EnCoeff*64L;
					//lum_tmp = (HI_U64)En_Bayer_AE_G_lum*(HI_U64)u32EnCoeff*64L*(HI_U64)(Soft_Light_S->Ratio_G_To_L)/100L;
					
					lum_tmp = (HI_U64)lum*256L*(HI_U64)u32EnCoeff*64L;
					En_Lum = (HI_U32)(lum_tmp/(HI_U64)exposure);
					Ratio_RG_En =  En_Bayer_AE_R_lum * 256 / En_Bayer_AE_G_lum;
					Ratio_BG_En =  En_Bayer_AE_B_lum * 256 / En_Bayer_AE_G_lum;
					
					if(Ratio_RG_En >= Soft_Light_S->Ratio_Rg_Ir+6)
					{
						//Natural_Light_Lum_Typical	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_A_Light) * Soft_Light_S->Cut_Ratio_A_Light/100;
						//if(Soft_Light_S->Ratio_Bg_Ir-Soft_Light_S->Ratio_Bg_A_Light)
						if((( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) *100/(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_A_Light)) >90)
					//	if( Natural_Light_Lum_Typical > Night_To_Day_ThrLuma )
						{
							ALight_Actual_Effect_Block_Num += 10;
							printf("env natrual light is A  \n");
						}
					}
	
					if((Ratio_RG_En < Soft_Light_S->Ratio_Rg_Ir - RG_Offset) && (Ratio_BG_En < Soft_Light_S->Ratio_Bg_Ir - BG_Offset))
					{
						Natural_Light_Lum_Typical	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_Natural_Light_Typical) * Soft_Light_S->Cut_Ratio_Natural_Light_Typical/100;
						Natural_Light_Lum_Max	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_Natural_Light_Max) * Soft_Light_S->Cut_Ratio_Natural_Light_Typical/100;
						Natural_Light_Lum_Min	=	( Soft_Light_S->Ratio_Bg_Ir - Ratio_BG_En ) * En_Lum /(Soft_Light_S->Ratio_Bg_Ir - Soft_Light_S->Ratio_Bg_Natural_Light_Min) * Soft_Light_S->Cut_Ratio_Natural_Light_Typical/100;
						//printf("Natural_Light_Lum_Typical is %d\n",Natural_Light_Lum_Typical);
						
						if(Ratio_BG_En  < Soft_Light_S->Ratio_Bg_Natural_Light_Min )
						//if( Natural_Light_Lum_Min > Night_To_Day_ThrLuma )
						{
							Normal_Actual_Effect_Block_Num += 25;
							printf("11111111111 bg_min %d\n",Ratio_BG_En);
							//printf("1row is %d,col is %d ,Night_To_Day_ThrLuma is %u,Natural_Light_Lum_Min is %u,Ratio_RG_En is %u,Ratio_BG_En is %u,En_Bayer_AE_R_lum %u, En_Bayer_AE_B_lum %u,En_Bayer_AE_G_lum %u ,Natural_Light_Lum_Typical is %u,Natural_Light_Lum_Min is %u,Natural_Light_Lum_Max is %u,Night_To_Day_ThrLuma is %u \n",row,col,Night_To_Day_ThrLuma,Natural_Light_Lum_Min,Ratio_RG_En,Ratio_BG_En,En_Bayer_AE_R_lum,En_Bayer_AE_B_lum,En_Bayer_AE_G_lum,Natural_Light_Lum_Typical,Natural_Light_Lum_Min,Natural_Light_Lum_Max,Night_To_Day_ThrLuma);
						}
						//else if( Natural_Light_Lum_Typical > Night_To_Day_ThrLuma )
						if(Ratio_BG_En  < Soft_Light_S->Ratio_Bg_Natural_Light_Typical )	
						{
							Normal_Actual_Effect_Block_Num += 15;
							printf("22222222222 _Bg_Natural %d\n",Ratio_BG_En);
							//printf("2row is %d,col is %d ,,Night_To_Day_ThrLuma is %u,Natural_Light_Lum_Typical is %u,Ratio_RG_En is %u ,Ratio_BG_En is %u,En_Bayer_AE_R_lum %u, En_Bayer_AE_B_lum %u,En_Bayer_AE_G_lum %u ,Natural_Light_Lum_Typical is %u,Natural_Light_Lum_Min is %u,Natural_Light_Lum_Max is %u,Night_To_Day_ThrLuma is %u \n",row,col,Night_To_Day_ThrLuma,Natural_Light_Lum_Typical,Ratio_RG_En,Ratio_BG_En,En_Bayer_AE_R_lum,En_Bayer_AE_B_lum,En_Bayer_AE_G_lum,Natural_Light_Lum_Typical,Natural_Light_Lum_Min,Natural_Light_Lum_Max,Night_To_Day_ThrLuma);
						}
						if( Ratio_BG_En < Soft_Light_S->Ratio_Bg_Natural_Light_Max )
						{
							Normal_Actual_Effect_Block_Num += 10;
							printf("3333333333 _Bg_max %d\n",Ratio_BG_En);
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
		
		if(exposure < 1629*8*64)
		{
			if(Normal_Actual_Effect_Block_Num*5> Lum_Effect_Block_Num)
				B_Turn_Day_Normal_Num ++;
			else
				B_Turn_Day_Normal_Num = 0; 
			
			if(ALight_Actual_Effect_Block_Num*3 > Lum_Effect_Block_Num)
				B_Turn_Day_ALight_Num ++;
			else
				B_Turn_Day_ALight_Num = 0; 
		}
		else
		{
				B_Turn_Day_Normal_Num  = B_Turn_Day_ALight_Num = 0; 
		}
		
		if((B_Turn_Day_Normal_Num > 3) || (B_Turn_Day_ALight_Num > 3) )
		{
			printf("AE AAAAAAAAAAAAAAAAAAAAAAAAA  enter day mode %d -- %d \n",B_Turn_Day_Normal_Num,B_Turn_Day_ALight_Num);
			B_Turn_Day_Normal_Num = 0; 	
			B_Turn_Day_ALight_Num = 0; 
			B_Turn_Night_Num = 0;
			bTurnNight = 0;
			bNewDay = TRUE;
			//return bNewDay;
		}
		printf("%s....%d................. Lum_Effect_Block_Num %d , NOomal_Actual_Effect_Block_Num %d    Alight_Actual_Effect_Block_Num %d...\n",__func__,__LINE__,Lum_Effect_Block_Num,Normal_Actual_Effect_Block_Num,ALight_Actual_Effect_Block_Num);
	}
	/*
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
		
	}*/
	return bNewDay;
}


#define ADC_READ	1

#define MAX_ISO_TBL_INDEX 15
#define FULL_FRAMERATE_RATIO  16
#define LIGHT_AE_NODE_MAX 11

static WDR_MODE_E jv_wdr_state =0;
static int jv_wdr_switch_running=0;

static BOOL StarLightEnable = FALSE;

static HI_BOOL bUserSetGamma = HI_FALSE;

static BOOL bRedWhiteCtrl = FALSE;

static JV_SENSOR_STATE SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
static pthread_mutex_t isp_daynight_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t isp_gamma_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t thread;

static JV_EXP_MODE_e jv_ae_strategy_mode[2] = {JV_EXP_AUTO_MODE,JV_EXP_AUTO_MODE};

jv_msensor_callback_t sensor_callback = NULL;

static BOOL bLowLightState =FALSE;
BOOL bCheckNightDayUsingAE =FALSE; //软光敏开关


extern COMM_ISP_PARAMS *pCOMM_ISP_PARAMS;
extern COMM_ISP_PARAMS MN34227_COMM_ISP_PARAMS;

static HI_BOOL bCal3DNoise = FALSE;
static HI_BOOL bAE_Stable = HI_TRUE;

COMM_ISP_PARAMS_LEVEL CISP_lEVEL = {};
frame_ctrl_node_n * pframe_ctrl_node = NULL;
light_ctrl_node_n * plight_ctrl_node = NULL;
HI_U8 avm5fps_thrd =56;
static HI_BOOL bUseAdc =FALSE;

#define IMAGE_VERSION "0001.0002.0002" /*重大修改版本号(SDK).一般代码修改.一般库修改*/



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

//最大ISO的级数
HI_U8 MAX_ISO_INDEX = 10; 
NRS_PARAM_V2_S *pNrsParam_V300;

typedef struct {

	int Max_Ratio;
	int Mid_Ratio;
	int Min_Ratio;
}	Luma_Ratio;

typedef struct {
	Luma_Ratio Day_Ratio;
	Luma_Ratio Night_Ratio;

}	DayNight_Luma_Ratio;

DayNight_Luma_Ratio Gamma_Ratio = {
	{800,400,200},
	{800,300,50}
};


BOOL __check_kz() //判断是否为空竹版本?
{

	char *product;
	product = hwinfo.devName;
	if (product && (strstr(product, "kz")||strstr(product, "KZ")))
	{
		return TRUE;
	}
	else
		return FALSE;
}

BOOL __check_whitelight()
{
	return FALSE;

}



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
//小维光敏IC  下拉电阻3M
static light_v light_v_list_jyals[] =
{
	{ 0,  21 },	//0.2Lux
	{ 1,  23 },	//0.3Lux
	{ 2,  26 },	//0.4Lux
	{ 3,  32 },	//0.6Lux
	{ 4,  37 },	//0.8Lux  --def night
	{ 5,  43 },	//1Lux
	{ 6,  58 },	//1.5Lux
	{ 7,  74 },	//2Lux
	{ 8,  104 },//3Lux	--def day
	{ 9,  136 },//4Lux
	{ 10, 168 },//5Lux
	{ 11, 201 },//6Lux
	{ 12, 232 },//7Lux
	{ 13, 265 },//8Lux
	{ 14, 297 }	//9Lux
};


const HI_U8 AE_WEIGHT_CENTER_NIGHT[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
{
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
		{1,1,1,1,1,4,5,5,5,5,5,4,1,1,1,1,1},
		{1,1,1,1,4,5,6,6,6,6,6,5,4,1,1,1,1},
		{1,1,1,2,4,5,6,7,7,7,6,5,4,2,1,1,1},
		{1,1,1,2,4,5,6,7,7,7,6,5,4,2,1,1,1},
		{1,1,1,2,4,5,6,6,6,6,6,5,4,2,1,1,1},
		{1,1,1,2,4,5,5,5,5,5,5,5,4,2,1,1,1},
		{1,1,1,2,4,4,4,4,4,4,4,4,4,2,1,1,1},
		{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
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


static light_ctrl_node_n light_ctrl_ae_list_mn34227[] =
{
		{25,500},//index -0 0.1lux 
		
		{30,500},//index -1	0.15lux
		
		{35,393},//index -2	0.2lux
		
		{40,290},//index -3	0.3lux
		
		{40,215},//index -4  0.4lux --def
		
		{45,152},//index -5	0.6lux
		
		{48,112},//index -6 0.8lux
		
		{48,95},//index -7 1lux
		{57,80},//index -8 1.5lux
		{57,49},//index -9 2lux
		{57,39},//index -10 3lux

};

static light_ctrl_node_n light_ctrl_ae_list_ar0237[] =
{
		{11,250},//index -0 
		
		{16,250},//index -1
		
		{22,250},//index -2
		
		{28,250},//index -3
		
		{38,250},//index -4  -default
		
		{47,250},//index -5
		
		{50,190},//index -6
		
		{52,100},//index -7
		{52,70},//index -8
		{54,50},//index -9
		{56,40},//index -10

};
static light_ctrl_node_n light_ctrl_ae_list_ov2735[] =
{
		{16,45},//index -0	-0.8Lux
		
		{30,45},//index -1	-1.5Lux	
		
		{40,45},//index -2	-2Lux
		
		{42,45},//index -3	-3Lux
		
		{45,35},//index -4	-4Lux -default
		
		{46,30},//index -5	-5Lux
		
		{46,30},//index -6	-6Lux
		
		{48,35},//index -7	-7Lux
		{48,32},//index -8	-8Lux
		{50,28},//index -9	-9Lux
		{50,26},//index -10	-10Lux

};



static frame_ctrl_node_n frame_ctrl_ae_list_mn34227[] =
{
	//index -0
	{
		{TRUE,TRUE},
		{27,  80},////20->12->4
		{1688*100*64,2814*500*64},//ExpLtoDThrd  //从亮到暗的降帧阀值
		{1688*65*64, 2814*420*64},//ExpDtoLThrd //从暗到亮的升帧阀值
		{1688,  2814, 8440}
	},
	//index -1
	{
		{TRUE,TRUE},
		{27,  80},////20->12->4
		{1688*100*64,2814*500*64},//ExpLtoDThrd  //从亮到暗的降帧阀值
		{1688*65*64, 2814*420*64},//ExpDtoLThrd //从暗到亮的升帧阀值
		{1688,  2814, 8440}
	},
	//index -2
	{
		{TRUE,TRUE},
		{27,  54},////20->12->6
		{1688*100*64,2814*500*64},//ExpLtoDThrd  //从亮到暗的降帧阀值
		{1688*65*64, 2814*420*64},//ExpDtoLThrd //从暗到亮的升帧阀值
		{1688,  2814, 5626}
	},
	//index -3 -default
	{
		{TRUE,TRUE},  //20->12->8
		{27,  40},
		{1688*100*64,2814*360*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1688*65*64, 2814*280*64},//ExpDtoLThrd
		{1688,  2814, 4220}
	},
	{ //index -4 -default
		{TRUE,TRUE},  //20->12->8
		{27,  40},
		{1688*100*64,2814*500*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1688*65*64, 2814*420*64},//ExpDtoLThrd
		{1688,  2814, 4220}
	},

	{//index -5-default
		{TRUE,FALSE},  //20->12
		{27,  40},
		{1688*100*64,2814*500*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1688*65*64, 2814*420*64},//ExpDtoLThrd
		{1688,  2814, 4220}
	},

	{//index -6 -default
		{FALSE,FALSE},  //20
		{27,  40},
		{1688*100*64,2814*500*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1688*65*64, 2814*420*64},//ExpDtoLThrd
		{1688,  2814, 4220}
	},

};
static light_ctrl_node_n light_ctrl_ae_list_imx291[] =
{
		{8,600},//index -0 
		
		{15,600},//index -1
		
		{20,600},//index -2
		
		{32,600},//index -3
		
		{40,520},//index -4  -default
		
		{45,500},//index -5
		
		{45,450},//index -6
		
		{45,400},//index -7
		{48,350},//index -8
		{48,300},//index -9
		{50,200},//index -10

};


static frame_ctrl_node_n frame_ctrl_ae_list_ar0237[] =
{
	//index -0
	{
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},
	//index -1
	{
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},
	//index -2
	{
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},
	
	//index -3 -default
	{
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},

	{ //index -4 -default
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},

	{//index -5-default
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},

	{//index -6 -default
		{TRUE,TRUE},  //25->12->6
		{32,  73},
		{1328*55*64,2659*250*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1328*38*64, 2659*110*64},//ExpDtoLThrd
		{1328,  2659, 6047}
	},

};

static frame_ctrl_node_n frame_ctrl_ae_list_imx291[] =
{
	//index -0
	{
		{TRUE,TRUE},  //25->12->6
		{34,  67},
		{1359*200*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2887, 5650}
	},

	//index -1
	{
		{TRUE,TRUE},  //25->12->6
		{34,  67},
		{1359*200*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2887, 5650}
	},

	//index -2
	{
		{TRUE,TRUE},  //25->12->8
		{34,  50},
		{1359*200*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2887, 4246}
	},



		//index -3 -default /////////ok
	{
		{TRUE,FALSE},  //25->12
		{34,  34},
		{1359*220*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2887, 2887}
	},

	{
		{TRUE,FALSE},  //25->15
		{27,  27},
		{1359*200*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2290, 2290}
	},


	{
		{FALSE,FALSE},  //25
		{27,  27},
		{1359*200*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2290, 2290}
	},



	{
		{FALSE,FALSE},  //25
		{27,  27},
		{1359*200*64,2887*400*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{1359*120*64, 2887*270*64},//ExpDtoLThrd
		{1359,  2290, 2290}
	},



};

static frame_ctrl_node_n frame_ctrl_ae_list_ov2735[] =
{
	//index -0
	{
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},
	//index -1
	{
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},
	//index -2
	{
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},
	
	//index -3 -default
	{
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},

	{ //index -4 -default
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},
	
	{//index -5-default
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},

	{//index -6 -default
		{TRUE,FALSE},  //20->12
		{27,  73},
		{2038*15*64,3262*50*64},//ExpLtoDThrd  //从亮到暗的升帧阀值
		{2038*9*64, 3262*30*64},//ExpDtoLThrd
		{2038,  3262, 6047}
	},

};



const HI_U8 OV2735_DEFOG_LUT[256] =
{
	92,93,94,95,96,97,98,99,101,102,103,104,105,106,107,108,109,110,111,113,114,115,116,117,118,119,120,121,122,124,125,126,127,128,129,130,131,132,133,134,135,136,138,139,140,141,142,143,144,145,146,147,148,\
	149,150,151,152,153,154,154,155,156,157,158,159,160,161,162,162,163,164,165,166,167,168,168,169,170,171,172,172,173,174,175,175,176,177,178,178,179,180,181,181,182,183,184,184,185,186,186,187,188,188,189,\
	190,190,191,192,192,193,194,194,195,196,196,197,198,198,199,199,200,201,201,202,203,203,204,204,205,206,206,207,207,208,209,209,210,210,211,211,212,213,213,214,214,215,215,216,216,217,217,218,218,219,219,\
	220,220,221,221,222,222,223,223,224,224,225,225,226,226,227,227,228,228,228,229,229,230,230,231,231,231,232,232,233,233,234,234,234,235,235,236,236,236,237,237,238,238,238,239,239,239,240,240,240,241,241,\
	241,242,242,242,243,243,243,243,244,244,244,245,245,245,245,246,246,246,247,247,247,247,248,248,248,248,249,249,249,249,250,250,250,251,251,251,251,252,252,252,252,253,253,253,254,254,254,254,255,255
};

BOOL __check_cp();

static void isp_helper_init(void);
static void isp_helper_deinit(void);
void WDR_Switch(int mode);

static int isp_helper_live=0;
static BOOL bLowNow = FALSE;
static BOOL bNightMode = FALSE;
static pthread_mutex_t low_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t isp_wdr_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int sensorType;
extern int imx290_wdr_mipi_flag;

//static int LowFrameRate =25;
static int ir_cut_staus=0;
static int light_value = 0;
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

	jv_gpio_muxctrl(0x200F00EC, 0x1);	//7_3 复用为PWM1
	/*
	 * PERI_CRG14 0x20030038: 0bit 0撤消复位 1复位；1bit 0关闭时钟 1打开时钟；[PERI_CRG14 2bit,3bit] 00 3MHz, 01 50MHz, 1x 24MHz
	 * 如下配置为3MHz时钟频率
	 */
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
	data.pwm_num = 1;
	data.period = 1000;	//根据INIT配置，3M/1000=3KHz	,这里配置为3KHz
	data.duty = data.period*value/100;
	ioctl(fdPwm,PWM_CMD_WRITE,(unsigned long)&data);
	return 0;
}

/*设置降噪*/
int jv_sensor_set_denoise(int sensorid,int value)
{
	
	if(value >ISP_SET_LEVEL_MAX)
		value =ISP_SET_LEVEL_MAX;
	if(value<0)
		value =ISP_SET_LEVEL_DEFAULT;
	if(pCOMM_ISP_PARAMS == NULL || pNrsParam_V300 == NULL)
		return -1;
	int i = 0;
	if(CISP_lEVEL.u8DeNoise != value)
	{
		CISP_lEVEL.u8DeNoise = value;
		isp_comm_denoise_level_set(CISP_lEVEL.u8DeNoise);
		/*空域降噪处理*/
		for(;i<16;i++)
		{
			pNrsParam_V300[i].SBS3 = pCOMM_ISP_PARAMS->SBS3[value].data[i];
			pNrsParam_V300[i].SDS3 = pCOMM_ISP_PARAMS->SDS3[value].data[i];
			bCal3DNoise = HI_TRUE;
		}
	}
	
	return 0;
};

/*设置清晰度*/
int jv_sensor_set_defini(int sensorid,int value)
{
	
	if(pCOMM_ISP_PARAMS == NULL)
		return 0;
	
	if(value >ISP_SET_LEVEL_MAX)
		value =ISP_SET_LEVEL_MAX;
	if(value<0)
		value =ISP_SET_LEVEL_DEFAULT;
	if(CISP_lEVEL.u8Definition != value)
	{
		CISP_lEVEL.u8Definition =value;
		isp_comm_sharpen_level_set(CISP_lEVEL.u8Definition);
		bCal3DNoise = HI_TRUE;
	}

	return 0;
};
/*设置拖影抑制*/
int jv_sensor_set_desmear(int sensorid,int value)
{
	
	if(pCOMM_ISP_PARAMS == NULL || pNrsParam_V300==NULL)
		return 0;
	
	if(value >ISP_SET_LEVEL_MAX)
		value =ISP_SET_LEVEL_MAX;
	if(value<0)
		value =ISP_SET_LEVEL_DEFAULT;
	int i = 0;
	if(CISP_lEVEL.u8DeSmear != value )
	{
		CISP_lEVEL.u8DeSmear =value;
		/*时域降噪处理*/
		for(;i<16;i++)
		{
			pNrsParam_V300[i].MATH1 = pNrsParam_V300[i].MATH2 = pCOMM_ISP_PARAMS->MATH[value].data[i];
		}
	}
	return 0;
};
/*设置流畅度*/
int jv_sensor_set_fluency(int sensorid,int value)
{
	//return;
	if(pCOMM_ISP_PARAMS == NULL)
		return 0;
	
	if(value >ISP_SET_LEVEL_MAX)
		value =ISP_SET_LEVEL_MAX;
	if(value<0)
		value =ISP_SET_LEVEL_DEFAULT;
	pthread_mutex_lock(&low_frame_mutex);
		
	if(CISP_lEVEL.u8Fluency != value)
	{
		SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
		bAeNight = FALSE;
		JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
		CISP_lEVEL.u8Fluency = value;
	}

	pthread_mutex_unlock(&low_frame_mutex); 

	return 0;
};

int jv_sensor_night_ae_speed_set(BOOL bSet)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	if(bSet)
	{
		bAE_Stable = HI_FALSE;
		HI_MPI_ISP_GetExposureAttr(0, &stExpAttr);
		stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 2;
		stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
		stExpAttr.stAuto.u8Speed =96;
		HI_MPI_ISP_SetExposureAttr(0, &stExpAttr);
	}
	else
	{
		HI_MPI_ISP_GetExposureAttr(0, &stExpAttr);
		stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 5;
		stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
		stExpAttr.stAuto.u8Speed = 72;
		HI_MPI_ISP_SetExposureAttr(0, &stExpAttr);		
	}
	return 0;

}

static BOOL bWhiteLightFunc = FALSE;

void  jv_sensor_set_whitelight_function(BOOL bEnable)
{
	bWhiteLightFunc = bEnable;
	return ;
}

BOOL jv_sensor_get_whitelight_function()
{
	return bWhiteLightFunc;
}

static BOOL bAlarmLight =FALSE;
BOOL jv_sensor_b_alarm_light()
{
	return bAlarmLight;
}

void jv_sensor_set_alarm_light(BOOL bEnable)
{
	bAlarmLight = bEnable;
}

static BOOL bWhiteLightStatus = FALSE;
void jv_sensor_set_whitelight(BOOL bEnable)
{
	bWhiteLightStatus = bEnable;
	jv_sensor_night_ae_speed_set(HI_TRUE);
	if(bEnable)
	{
		printf("lllllllllllllllllllllllllllllllllllllll open \n");
		if (higpios.whitelight.group != -1 && higpios.whitelight.bit != -1)
			jv_gpio_write(higpios.whitelight.group, higpios.whitelight.bit, 1);
	}
	else
	{
		printf("lllllllllllllllllllllllllllllllllllllll close \n");
		if (higpios.whitelight.group != -1 && higpios.whitelight.bit != -1)
			jv_gpio_write(higpios.whitelight.group, higpios.whitelight.bit, 0);
	}
	
	return;
}

BOOL jv_sensor_get_whitelight_status()
{
	return bWhiteLightStatus;
}

void jv_sensor_set_ircut(BOOL bNight)
{
	if (bNight)
	{
		//开灯
		//printf("lllllllllllllllllllllllllllllllllllllll open zzzzzzzzzzzz \n");
		if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 1);

		//if (higpios.whitelight.group != -1 && higpios.whitelight.bit != -1)
			//jv_gpio_write(higpios.whitelight.group, higpios.whitelight.bit, 1);

		if (higpios.cutnight.group != -1 && higpios.cutnight.bit != -1 &&
			higpios.cutday.group != -1 && higpios.cutday.bit != -1)
		{
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
			ir_cut_staus = 1;
		}
		
		jv_sensor_night_ae_speed_set(HI_TRUE);
		
		{
			ISP_WB_ATTR_S pstWBAttr;
			HI_MPI_ISP_GetWBAttr(0,&pstWBAttr);
			pstWBAttr.bByPass = HI_TRUE;
			HI_MPI_ISP_SetWBAttr(0,&pstWBAttr);
		}
		
	}
	else
	{
		//关灯
		if (higpios.redlight.group != -1 && higpios.redlight.bit != -1)
			jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 0);

		//if (higpios.whitelight.group != -1 && higpios.whitelight.bit != -1)
			//jv_gpio_write(higpios.whitelight.group, higpios.whitelight.bit, 0);

		if (higpios.cutnight.group != -1 && higpios.cutnight.bit != -1 &&
			higpios.cutday.group != -1 && higpios.cutday.bit != -1)
		{
			jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 1);
			usleep(hwinfo.ir_power_holdtime);
			jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);
			ir_cut_staus = 0;
		}
		
		{
			ISP_WB_ATTR_S pstWBAttr;
			HI_MPI_ISP_GetWBAttr(0,&pstWBAttr);
			pstWBAttr.bByPass = HI_FALSE;
			pstWBAttr.u8AWBRunInterval = 2;
			HI_MPI_ISP_SetWBAttr(0,&pstWBAttr);
		}
		
	}
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

int fd_adc = -1;
 void jv_adc_init(void)
{
	int dir = 0;
  	fd_adc = open("/dev/hi_adc", O_RDWR);
	if (fd_adc == -1)
   	{
	   printf("open adv mudule error\n");
   	}
}
static  unsigned int LightAeIndex  = 4;
int jv_sensor_get_adc_high_val()
{
	return light_v_list_jyals[LightAeIndex].voltage;
}

int jv_sensor_get_adc_low_val()
{
	return light_v_list_jyals[LightAeIndex+4].voltage;;
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
		value_s += value;
		usleep(1);
	}
	value = value_s / 20;
	//printf("value:%d\n",value);
	return value;
}

int jv_adc_read(void)
{
	int value=0,value_s=0;
	static int dn_status=0;
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
		//value = value>>2;//V300精度10bit，16D精度8bit
		value_s+=value;
		usleep(1);
	}
	value=value_s/20;
	//printf("value:%d\n",value);

	if(value>light_v_list_jyals[LightAeIndex+4].voltage)
	{
		dn_status = 0;
		return 0;
	}
	else if(value<=light_v_list_jyals[LightAeIndex].voltage)
	{
		dn_status = 1;
		return 1;
	}
	else
	{
		return dn_status;
	}
}


int  jv_sensor_get_ircut_staus()
{
	return ir_cut_staus;
}
int sensor_ircut_init(void)
{
	int ret;
//	_gpio_cutcheck_redlight_change();

//	jv_pwm_init();
#if 0
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
#endif
	if(hwinfo.sensor==SENSOR_OV2735 && PRODUCT_MATCH(PRODUCT_C3W))
		bCheckNightDayUsingAE =TRUE;
	
	jv_adc_init();
	isp_helper_init();

	if(hwinfo.sensor == SENSOR_OV2735 && HWTYPE_MATCH(HW_TYPE_C8H))
	{
		bRedWhiteCtrl = TRUE;
		jv_sensor_set_whitelight(FALSE);
	}
	return 0;
}

int sensor_ircut_deinit(void)
{
	isp_helper_deinit();
	return 0;
}

//设置工频干扰全局变量
int jv_sensor_set_antiflick(int sensorid,JV_ANTIFLICK_e mode)
{
	//printf("[%s] line:%d	anti:%d=>", __FUNCTION__, __LINE__, jv_antiflick_flag);
	//jv_antiflick_flag = mode;
	//printf("%d\n",jv_antiflick_flag);
	return 0;
}

int jv_sensor_daynight_ae_set(BOOL bNight)
{
	ISP_EXPOSURE_ATTR_S pstExpAttr;
	if(bNight)
	{
		if(jv_ae_strategy_mode[1] == JV_EXP_AUTO_MODE)//自动曝光
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_NIGHT_V1,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=128;
			pstExpAttr.stAuto.u8MaxHistOffset=7;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[1] == JV_EXP_HIGH_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);//高光优先
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=32;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[1] == JV_EXP_LOW_LIGHT_MODE)//低光优先
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=25;
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
			pstExpAttr.stAuto.u8MaxHistOffset=9;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);		
		}
		else if(jv_ae_strategy_mode[0] == JV_EXP_HIGH_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=32;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_HIGHLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
		}
		else if(jv_ae_strategy_mode[0] == JV_EXP_LOW_LIGHT_MODE)
		{
			HI_MPI_ISP_GetExposureAttr(0, &pstExpAttr);
			memcpy(&pstExpAttr.stAuto.au8Weight,&AE_WEIGHT_CENTER_DEFAULT,255*1);
			pstExpAttr.stAuto.u16HistRatioSlope=256;
			pstExpAttr.stAuto.u8MaxHistOffset=25;
			pstExpAttr.stAuto.enAEStrategyMode=AE_EXP_LOWLIGHT_PRIOR;
			HI_MPI_ISP_SetExposureAttr(0, &pstExpAttr);
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

int jv_sensor_set_exp_params(int sensorid,int value)
{
	
	return 0;
}


static BOOL bCheckNight =FALSE;

int jv_sensor_set_nightmode(BOOL bNight)
{
	pthread_mutex_lock(&isp_daynight_mutex);
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

	pthread_mutex_unlock(&isp_daynight_mutex);

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
   int mode=1;
   
	if(bCheckNightDayUsingAE)
	{
		//printf("[%s] %d	bAeNight=%d\n",__FUNCTION__, __LINE__,bAeNight);
		return bAeNight;
	}
   
   if(StarLightEnable)
		return bAeNight;


	switch(hwinfo.ir_sw_mode)
	{
		case IRCUT_SW_BY_GPIO:
		{
			return FALSE;
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
			mode = jv_adc_read();
			return mode;
			break;
		}
		case IRCUT_SW_BY_ISP:
		{
			if(hwinfo.sensor==SENSOR_MN34227||hwinfo.sensor==SENSOR_IMX291||hwinfo.sensor==SENSOR_AR0237DC||hwinfo.sensor==SENSOR_OV2735)
				return jv_adc_read();
			else
				return jv_adc_read();
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
static BOOL bUserDefogEnable =FALSE;
int jv_sensor_antifog(int sensorid, int nValue)
{
    Printf(">>> %s  sensorid:%d  ,antifog:%d", __FUNCTION__, sensorid, nValue);
	
	ISP_DEFOG_ATTR_S pstDefogAttr;
	HI_MPI_ISP_GetDeFogAttr(0, &pstDefogAttr);
	pstDefogAttr.bEnable=nValue;
	bUserDefogEnable  =pstDefogAttr.bEnable;
	if(hwinfo.sensor ==SENSOR_OV2735)
	{
		//HI_MPI_ISP_SetDeFogAttr(0, &pstDefogAttr);
		return 0;
	}
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
 
 int jv_sensor_light(int sensorid, int nValue)
{
	//light_value=nValue;

	if(nValue >= LIGHT_AE_NODE_MAX || nValue < 0)
		return 0;

	if(hwinfo.sensor == SENSOR_OV2710||hwinfo.sensor == SENSOR_MN34227||hwinfo.sensor == SENSOR_IMX291||hwinfo.sensor == SENSOR_OV2735)
	{
		pthread_mutex_lock(&low_frame_mutex);
		light_value  = 4;

		if( nValue != LightAeIndex )
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
 *@brief set contrast
 *@param sensorid
 *@param contrast value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_contrast(int sensorid, int nValue)
{
	pthread_mutex_lock(&isp_gamma_mutex);
	if(bLowLightState==FALSE)
	{
		isp_ioctl(0,ADJ_CONTRAST,nValue);
	}
	pthread_mutex_unlock(&isp_gamma_mutex);
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
	if(__check_cp())
		bNoColor =FALSE;
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
	return JV_ISP_COMM_Set_StdFps(frameRate);
}

BOOL jv_sensor_b_daynight_usingAE()
{
	if(bRedWhiteCtrl)
	{
		return TRUE;
	}
	else
		return bCheckNightDayUsingAE;
}

int jv_sensor_low_frame_inner(int sensorid, BOOL  bEnable)
{
	int tp;
	if (bEnable)
	{
		if(hwinfo.sensor==SENSOR_IMX291)
			tp = 34;
		else if(hwinfo.sensor==SENSOR_OV2735)//20->12
			tp = 27;
		else
			tp = 32;
	}
	else
		tp = 16;
	
	JV_ISP_COMM_Set_LowFps(tp);//fps=ViFps*16/tp;
	
	return 0;

}

static BOOL bNightLowframe =FALSE;
int jv_sensor_low_frame(int sensorid, int bEnable)
{
	if(bNightMode)
		bNightLowframe =bEnable;//夜视模式下才允许改变

	return 0;

}

int jv_sensor_auto_lowframe_enable(int sensorid, int bEnable)
{
	return 0;
}

static BOOL bStarSwitched =FALSE;
int jv_sensor_switch_starlight(BOOL bOn)
{
	//if(StarLightEnable==bOn)
		//return 0;
	if(hwinfo.sensor==SENSOR_IMX291||hwinfo.sensor==SENSOR_AR0237DC||hwinfo.sensor==SENSOR_MN34227||hwinfo.sensor == SENSOR_OV2735)
	{
		pthread_mutex_lock(&low_frame_mutex);
   		if(bOn==FALSE)
   		{
   			printf("star light function  off\n");
			//jv_sensor_set_whitelight(FALSE);//关白光灯
			StarLightEnable =FALSE;		
   		}
		else
		{
			printf("star light function  on\n");  //打开星光级，且重置状态
			//jv_sensor_set_whitelight(FALSE);//关白光灯
			StarLightEnable =TRUE;		
		}
		bStarSwitched =TRUE;
		JV_ISP_COMM_Set_LowFps(16);
		SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
		bAeNight =FALSE;
		pthread_mutex_unlock(&low_frame_mutex); 
	}
	
	return 0;
		
}

static unsigned int DayNightMode = 0xff;
int jv_sensor_set_daynight_mode(int sensorid, int mode)
{
	//if(!StarLightSupported)
		//return 0
	if(DayNightMode == mode)
		return 0;
	
	DayNightMode =mode;
	
	if(hwinfo.sensor==SENSOR_MN34227||hwinfo.sensor==SENSOR_IMX291||hwinfo.sensor==SENSOR_AR0237DC||hwinfo.sensor==SENSOR_OV2710||hwinfo.sensor == SENSOR_OV2735)
	{
		if(mode==DAYNIGHT_AUTO||mode==DAYNIGHT_ALWAYS_DAY ||(bRedWhiteCtrl && mode==DAYNIGHT_TIMER))
			jv_sensor_switch_starlight(TRUE);
		else
			jv_sensor_switch_starlight(FALSE);

		return 0;

	}
	else
	{
	
		if(mode==DAYNIGHT_AUTO||mode==DAYNIGHT_ALWAYS_DAY)
			jv_sensor_switch_starlight(TRUE);
		else
			jv_sensor_switch_starlight(FALSE);

		return 0;
	}
	
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
	
	blow = JV_ISP_COMM_Query_LowFps();
	if (current)
	{
		fps = JV_ISP_COMM_Get_Fps();
		if(hwinfo.sensor==SENSOR_IMX291)
			if(blow)
				fps = fps*34/32;
		*current =(float)( (int)(fps+0.5));
	}
	
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
		//printf("ae attr: [%d, %d]\n", attr.u16ExpTimeMin, attr.u16ExpTimeMax);
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
	printf("%s\n", __FUNCTION__);
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
HI_BOOL jv_sensor_CheckDay_By_Sensor(ISP_DEV IspDev,HI_U32 ISO,HI_U32 expTime,HI_U8 lum)
{

		static HI_U8 bDay_Times = 0;
		HI_U32 ScLight = 0;
		HI_U32  Recip_Of_Lum = 1;
		ISP_STATISTICS_S stStat;
		HI_MPI_ISP_GetStatistics(IspDev, &stStat);
		if(hwinfo.sensor == SENSOR_MN34227)
		{
			if(((ISO*expTime*256)/(lum*100)) >20000)
				return HI_FALSE;
			ScLight = (550*stStat.stWBStat.stBayerStatistics.u16GlobalG - 522*stStat.stWBStat.stBayerStatistics.u16GlobalB)/251;
			Recip_Of_Lum = (256*ISO*expTime)/(ScLight*100);
			printf("Recip_Of_Lum is %u \n",Recip_Of_Lum);
			if(Recip_Of_Lum < 20000)
				bDay_Times ++;
			else
				bDay_Times = 0;
		}
		
		if(bDay_Times > 10)
		{
			bDay_Times = 0;
			return HI_TRUE;
		}
		else
			return HI_FALSE;
		
	
}*/

int jv_sensor_set_high_ca(HI_BOOL set)
{
	static HI_BOOL bset = HI_FALSE;
	if(bset == set)
		return 0;
	
	HI_U16 lum_ca_normal[] = {  36,   81,  111,  136,  158,  182,  207,  228, 259,  290,  317,  345,  369,  396,  420,  444,
	 	468,  492,  515,  534,  556,  574,  597,  614, 632,  648,  666,  681,  697,  709,  723,  734,748,  758,  771,  780,  788,  800,  808,  815,
	 	822,  829,  837,  841,  848,  854,  858,  864, 868,  871,  878,  881,  885,  890,  893,  897, 900,  903,  906,  909,  912,  915,  918,  921,
	 	924,  926,  929,  931,  934,  936,  938,  941, 943,  945,  947,  949,  951,  952,  954,  956, 958,  961,  962,  964,  966,  968,  969,  970,
	 	971,  973,  974,  976,  977,  979,  980,  981, 983,  984,  985,  986,  988,  989,  990,  991, 992,  993,  995,  996,  997,  998,  999, 1000,
	 	1001, 1004, 1005, 1006, 1007, 1009, 1010, 1011,1012, 1014, 1016, 1017, 1019, 1020, 1022, 1024,};

	HI_U16 lum_ca_high[] = {1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,
		1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,
		1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,
		1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,
		1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,
		1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800,};
	
	ISP_CA_ATTR_S stCAAttr;
	int ret = 0;	
	HI_MPI_ISP_GetCAAttr(0, &stCAAttr);
	if(set)
	{
		memcpy(stCAAttr.au16YRatioLut,lum_ca_high,128*2);
	}
	else
	{
		memcpy(stCAAttr.au16YRatioLut,lum_ca_normal,128*2);
	}
	
	ret = HI_MPI_ISP_SetCAAttr(0,&stCAAttr);

	bset = set;
	
	return ret;
	
}


#if 0
void jv_sensor_Fps_Self_Adapt(HI_U32 jv_exp,HI_U32 ISO,HI_U32 exptime,HI_U8 jvAvm)
{

		static HI_U8 timcnt[4] = {0};
		static HI_U8  AdcDayCnt = 0;
		static HI_U8 AE_Stable_cnt = 0;
		
		HI_BOOL bFirstLow = HI_FALSE; 
		HI_BOOL bSecondLow = HI_FALSE; 
		HI_U8 FirstRatio = 0; 
	    HI_U8 SecondRatio = 0;
		HI_U32 u32FirstExpLtoDThrd = 0;
		HI_U32 u32SecondExpLtoDThrd = 0;
		HI_U16 ToNightISOThrd = 0;
		HI_U8 ToNightLumThrd = 0;
		HI_U32 ToNightExpThrd = 0;
		HI_U32 u32FirstExpDtoLThrd = 0;
		HI_U32 u32SecondExpDtoLThrd = 0;
		
		pthread_mutex_lock(&low_frame_mutex);
		
		{
			bFirstLow  = pframe_ctrl_node[CISP_lEVEL.u8framelevel].bLow[0];
			bSecondLow = pframe_ctrl_node[CISP_lEVEL.u8framelevel].bLow[1];
			FirstRatio  = pframe_ctrl_node[CISP_lEVEL.u8framelevel].LowRatio[0];
			SecondRatio = pframe_ctrl_node[CISP_lEVEL.u8framelevel].LowRatio[1];

			u32FirstExpLtoDThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[0];
			u32SecondExpLtoDThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[1];

			u32FirstExpDtoLThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[0];
			u32SecondExpDtoLThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[1];
			ToNightISOThrd = plight_ctrl_node[LightAeIndex].ISOThrd;
			ToNightLumThrd = plight_ctrl_node[LightAeIndex].LumThrd;

			ToNightExpThrd = ToNightISOThrd * pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLine[0]*64;
			if(bFirstLow)
				ToNightExpThrd = ToNightISOThrd * pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLine[1]*64;
			if(bSecondLow)
				ToNightExpThrd = ToNightISOThrd * pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLine[2]*64;
			

			if(JV_ISP_COMM_Get_StdFps()>25)
			{
				u32FirstExpLtoDThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[0]*25/30;
				u32SecondExpLtoDThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[1]*25/30;

				u32FirstExpDtoLThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[0]*25/30;
				u32SecondExpDtoLThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[1]*25/30;
				ToNightExpThrd = ToNightExpThrd*25/30;
			}
		}
		if(StarLightEnable)
		{
			
			if((bAeNight==FALSE) && jv_exp>= ToNightExpThrd && jvAvm<= ToNightLumThrd)//enter night mode
			{
				timcnt[3]++;
				if(timcnt[3] >= 1)
				{
					if(jv_adc_read(HI_TRUE)&&DayNightMode ==DAYNIGHT_AUTO)
					{
						bAeNight = TRUE;
						SensorState = SENSOR_STATE_NIGHT;
						printf("enter sensor night state xxxxxxxxxx%hhu %hhu %d\n",ToNightLumThrd,jvAvm,SensorState);
					}
					timcnt[3] = 0;
				}
			}
			else
				timcnt[3] = 0;	
			
			 if((bAeNight==FALSE) && (SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE || SensorState==SENSOR_STATE_DAY_QUARTER_FRAMRATE) && jv_exp<=u32FirstExpDtoLThrd)//from 6 or 12 to 25 fps
			{
				timcnt[0]++;
				if(timcnt[0] >= 3)
				{
					JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
					printf("enter sensor full 25fps state %d\n",SensorState);
					timcnt[0] = 0;
				}
			}
			else
				timcnt[0] = 0;
			 if(bAeNight==FALSE && jv_exp > u32FirstExpDtoLThrd )//to 6 fps or 12 fps
			{
				if(SensorState !=SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp >= u32SecondExpLtoDThrd && bSecondLow&&jvAvm<=avm5fps_thrd)// enter 6 fps
				{ 
					timcnt[1] = 0;
					
					timcnt[2]++;
					if(timcnt[2] >= 1 )
					{
						JV_ISP_COMM_Set_LowFps(SecondRatio);
						SensorState = SENSOR_STATE_DAY_QUARTER_FRAMRATE;
						printf("enter sensor 6fps state %d\n",SensorState);
						timcnt[2] = 0;
					}
				}
				else if((SensorState == SENSOR_STATE_DAY_FULL_FRAMRATE && jv_exp>=u32FirstExpLtoDThrd && bFirstLow)
					||(SensorState == SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp <= u32SecondExpDtoLThrd && bFirstLow))//enter 12 fps
				{
					timcnt[2] = 0;
					
					timcnt[1]++;
					if(timcnt[1] >= 2)
					{
						JV_ISP_COMM_Set_LowFps(FirstRatio);
						SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
						printf("enter sensor 12.5fps state %d\n",SensorState);
						timcnt[1] = 0;
					}						 
				}
				else
				{
					timcnt[1] = 0;
					timcnt[2] = 0;
				}
			}
			
			if(SensorState == SENSOR_STATE_NIGHT)//night to day of 25
			{  
				if(bUseAdc)
				{
					if(jv_adc_read(HI_FALSE))
					{
						AdcDayCnt=0;
					}
					else
					{
						AdcDayCnt++;
						if(AdcDayCnt>=3)
						{
							bAeNight =FALSE;
							printf("enter day\n");
							AdcDayCnt=0;
						}
					}
				}
				else
				{
					if(jv_sensor_CheckDay_By_Sensor(0,ISO, exptime,jvAvm))
						bAeNight = FALSE;
				}
			} 

		}
		
		if(!bAE_Stable)
			AE_Stable_cnt ++;
		if(AE_Stable_cnt > 20)
		{
			bAE_Stable = HI_TRUE;
			AE_Stable_cnt = 0;
			jv_sensor_night_ae_speed_set(FALSE);
		}
		pthread_mutex_unlock(&low_frame_mutex);
}
#else

static HI_U32 whiteLightExp =0;
//static int whiteDelay = 0;
static unsigned char  whiteTodayTick0 = 0;
static unsigned char  whiteTodayTick1 = 0;
static unsigned char  whiteTodayTick2 = 0;
void jv_sensor_Fps_Self_Adapt(HI_U32 jv_exp,HI_U32 ISO,HI_U32 exptime,HI_U8 jvAvm)
{

	static HI_U8 timcnt[6] = {0};
	static HI_U8  AdcDayCnt = 0;
	static HI_U8 AE_Stable_cnt = 0;
	
	static BOOL bCheckNightStatus =FALSE;
	static BOOL bCheckDayStatus =FALSE;
	static BOOL bCheckNightLowFrameStatus =FALSE;

	
	HI_BOOL bFirstLow = HI_FALSE; 
	HI_BOOL bSecondLow = HI_FALSE; 
	HI_U8 FirstRatio = 0; 
    HI_U8 SecondRatio = 0;
	HI_U32 u32FirstExpLtoDThrd = 0;
	HI_U32 u32SecondExpLtoDThrd = 0;
	HI_U16 ToNightISOThrd = 0;
	HI_U8 ToNightLumThrd = 0;
	HI_U32 ToNightExpThrd = 0;
	HI_U32 u32FirstExpDtoLThrd = 0;
	HI_U32 u32SecondExpDtoLThrd = 0;
	static BOOL bAlarmed = 0xff;
	//static BOOL bAlarmCut = FALSE;
	static int alarmTick = 0;
	static BOOL bWLFunc = FALSE;
	
	if(pframe_ctrl_node == NULL)
		return;
	
	pthread_mutex_lock(&low_frame_mutex);

	{
		bFirstLow  = pframe_ctrl_node[CISP_lEVEL.u8framelevel].bLow[0];
		bSecondLow = pframe_ctrl_node[CISP_lEVEL.u8framelevel].bLow[1];
		FirstRatio  = pframe_ctrl_node[CISP_lEVEL.u8framelevel].LowRatio[0];
		SecondRatio = pframe_ctrl_node[CISP_lEVEL.u8framelevel].LowRatio[1];

		u32FirstExpLtoDThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[0];
		u32SecondExpLtoDThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[1];

		u32FirstExpDtoLThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[0];
		u32SecondExpDtoLThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[1];
		ToNightISOThrd = plight_ctrl_node[LightAeIndex].ISOThrd;
		ToNightLumThrd = plight_ctrl_node[LightAeIndex].LumThrd;

		ToNightExpThrd = ToNightISOThrd * pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLine[0]*64;
		if(bFirstLow)
			ToNightExpThrd = ToNightISOThrd * pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLine[1]*64;
		if(bSecondLow)
			ToNightExpThrd = ToNightISOThrd * pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLine[2]*64;
		

		if(JV_ISP_COMM_Get_StdFps()>25)
		{
			u32FirstExpLtoDThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[0]*25/30;
			u32SecondExpLtoDThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpLtoDThrd[1]*25/30;

			u32FirstExpDtoLThrd  =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[0]*25/30;
			u32SecondExpDtoLThrd =  pframe_ctrl_node[CISP_lEVEL.u8framelevel].ExpDtoLThrd[1]*25/30;
			ToNightExpThrd = ToNightExpThrd*25/30;
		}
	}
	if(bRedWhiteCtrl ==  FALSE)
	{
		if(StarLightEnable)
		{
			if(bCheckNightDayUsingAE == FALSE)
			{
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus  =FALSE;
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
					if(bCheckNightLowFrameStatus!=bNightLowframe) //
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("star light night low frame change..\n");
					}
					bCheckDayStatus =FALSE;
				
					if(bAeNight)
					{
						if(jv_adc_read())
							AdcDayCnt=0;
						else
						{
							AdcDayCnt++;
							if(AdcDayCnt>=3)
							{
								bAeNight =FALSE;
								printf("enter day\n");
								AdcDayCnt=0;
							}

						}

					}
				
					if(bAeNight ==FALSE&&SensorState!=SENSOR_STATE_DAY_FULL_FRAMRATE) 
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
						printf("star light night to full framerate\n");
					
					}
				
				}
				else
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("star light day init\n");
					}
					else
					{
					
					
						if((bAeNight==FALSE) && (SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE || SensorState==SENSOR_STATE_DAY_QUARTER_FRAMRATE) && jv_exp<=u32FirstExpDtoLThrd)//from 6 or 12 to 25 fps
						{
							timcnt[0]++;
							if(timcnt[0] >= 3)
							{
								JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
								SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
								printf("enter sensor full 25fps state %d\n",SensorState);
								timcnt[0] = 0;
							}
						}
						else
							timcnt[0] = 0;
						 if(bAeNight==FALSE && jv_exp > u32FirstExpDtoLThrd )//to 6 fps or 12 fps
						{
							if(SensorState !=SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp >= u32SecondExpLtoDThrd && bSecondLow&&jvAvm<=avm5fps_thrd)// enter 6 fps
							{ 
								timcnt[1] = 0;
							
								timcnt[2]++;
								if(timcnt[2] >= 1 )
								{
									JV_ISP_COMM_Set_LowFps(SecondRatio);
									SensorState = SENSOR_STATE_DAY_QUARTER_FRAMRATE;
									printf("enter sensor 6fps state %d\n",SensorState);
									timcnt[2] = 0;
								}
							}
							else if((SensorState == SENSOR_STATE_DAY_FULL_FRAMRATE && jv_exp>=u32FirstExpLtoDThrd && bFirstLow)
								||(SensorState == SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp <= u32SecondExpDtoLThrd && bFirstLow))//enter 12 fps
							{
								timcnt[2] = 0;
							
								timcnt[1]++;
								if(timcnt[1] >= 2)
								{
									JV_ISP_COMM_Set_LowFps(FirstRatio);
									SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
									printf("enter sensor 12.5fps state %d\n",SensorState);
									timcnt[1] = 0;
								}						 
							}
							else
							{
								timcnt[1] = 0;
								timcnt[2] = 0;
							}
						}
					
						if((bAeNight==FALSE) && jv_exp>= ToNightExpThrd && jvAvm<= ToNightLumThrd)//enter night mode
						{
							timcnt[3]++;
							if(timcnt[3] >= 1)
							{
								if(jv_adc_read()&&DayNightMode ==DAYNIGHT_AUTO)
								{
									bAeNight = TRUE;
									SensorState = SENSOR_STATE_NIGHT;
									printf("enter sensor night state ToNightLumThrd %hhu jvAvm %hhu SensorState %d\n",ToNightLumThrd,jvAvm,SensorState);
								}
								timcnt[3] = 0;
							}
						}
						else
							timcnt[3] = 0;	
					}
				}
			}
			else
			{
				int retttt = jv_sensor_smart_judge(bNightMode,LightAeIndex,100);//无adc，传100吧
				if(bStarSwitched)
				{
					bCheckNightStatus =FALSE;
					bCheckDayStatus  =FALSE;
					bCheckNightLowFrameStatus =FALSE;
					bStarSwitched =FALSE;
				}
				if(bNightMode)
				{
					if(bCheckNightStatus==FALSE)
					{
						bCheckNightStatus= TRUE;
						jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);
						printf("ae2.0 star light night init ok\n");
					
					}
					if(bCheckNightLowFrameStatus!=bNightLowframe) //
					{
						jv_sensor_low_frame_inner(0,bNightLowframe);
						bCheckNightLowFrameStatus=bNightLowframe;
						printf("ae2.0 star light night low frame change..\n");
					}
					bCheckDayStatus =FALSE;

					 bAeNight = retttt;

				
					if(bAeNight ==FALSE&&SensorState!=SENSOR_STATE_DAY_FULL_FRAMRATE) 
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
						printf("ae2.0 star light night to full framerate\n");
					
					}
				
				}
				else
				{
					bCheckNightStatus =FALSE;
					if(bCheckDayStatus==FALSE)//
					{
						SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
						JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
						bCheckDayStatus =TRUE;
						printf("ae2.0 star light day init\n");
					}
					else
					{
					
					
						if((bAeNight==FALSE) && (SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE || SensorState==SENSOR_STATE_DAY_QUARTER_FRAMRATE) && jv_exp<=u32FirstExpDtoLThrd)//from 6 or 12 to 25 fps
						{
							timcnt[0]++;
							if(timcnt[0] >= 3)
							{
								JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
								SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
								printf("ae2.0 enter sensor full 25fps state %d\n",SensorState);
								timcnt[0] = 0;
							}
						}
						else
							timcnt[0] = 0;
						 if(bAeNight==FALSE && jv_exp > u32FirstExpDtoLThrd )//to 6 fps or 12 fps
						{
							if(SensorState !=SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp >= u32SecondExpLtoDThrd && bSecondLow&&jvAvm<=avm5fps_thrd)// enter 6 fps
							{ 
								timcnt[1] = 0;
							
								timcnt[2]++;
								if(timcnt[2] >= 1 )
								{
									JV_ISP_COMM_Set_LowFps(SecondRatio);
									SensorState = SENSOR_STATE_DAY_QUARTER_FRAMRATE;
									printf("ae2.0 enter sensor 6fps state %d\n",SensorState);
									timcnt[2] = 0;
								}
							}
							else if((SensorState == SENSOR_STATE_DAY_FULL_FRAMRATE && jv_exp>=u32FirstExpLtoDThrd && bFirstLow)
								||(SensorState == SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp <= u32SecondExpDtoLThrd && bFirstLow))//enter 12 fps
							{
								timcnt[2] = 0;
							
								timcnt[1]++;
								if(timcnt[1] >= 2)
								{
									JV_ISP_COMM_Set_LowFps(FirstRatio);
									SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
									printf("ae2.0 enter sensor 12.5fps state %d\n",SensorState);
									timcnt[1] = 0;
								}						 
							}
							else
							{
								timcnt[1] = 0;
								timcnt[2] = 0;
							}
						}
					
						if((bAeNight==FALSE) && retttt>0)//enter night mode
						{
							timcnt[3]++;
							if(timcnt[3] >= 1)
							{
								if(DayNightMode!=DAYNIGHT_ALWAYS_DAY)
								{
									bAeNight = TRUE;
									SensorState = SENSOR_STATE_NIGHT;
									printf("ae2.0 enter sensor night state ToNightLumThrd %hhu jvAvm %hhu SensorState %d\n",ToNightLumThrd,jvAvm,SensorState);
								}
								timcnt[3] = 0;
							}
						}
						else
							timcnt[3] = 0;	
					}
				}			
			}
		}
		else //
		{
				

			if(bStarSwitched)
			{
				bCheckNightStatus =FALSE;
				bCheckDayStatus  =FALSE;
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
				if(bCheckNightLowFrameStatus!=bNightLowframe) //
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
				if(bCheckDayStatus==FALSE)//
				{
					SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
					JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					bCheckDayStatus =TRUE;
					printf("no star day init ok\n");
				}
				
			}
			
			
		}
	}
	else  if(bRedWhiteCtrl == TRUE) //针对红+白光灯双摄一体机进行实现 
	{

		//printf("sssssssssssss\n");
		//u32EnCoeff = 1629*2*64*50/256;
		//HI_U32 Night_To_Day_ThrLuma = 60*256*u32EnCoeff*64 / 1629*16*64;
		//HI_U32 Day_To_Night_ThrLuma = 50*256*u32EnCoeff *64 / 3262*16*64;
		//BOOL bDay = FALSE;
#define ALARM_DELAY_TICK 7
		if(bAlarmed !=jv_sensor_b_alarm_light())
		{
			//bAlarmCut = TRUE;
			alarmTick =0;
			if(bAlarmed == 0xff)
				alarmTick = ALARM_DELAY_TICK;
			bAlarmed =jv_sensor_b_alarm_light();
			
			bWhiteMode = FALSE;
			
			whiteLightExp = 0;
			whiteTodayTick1 = whiteTodayTick0 = whiteTodayTick2 =0;		
			
			timcnt[4] = 0;
			timcnt[5] = 0;
		}
		//else
			//bAlarmCut = FALSE;

		alarmTick++;

		if(alarmTick >= ALARM_DELAY_TICK )
			alarmTick = ALARM_DELAY_TICK ;
		
		if(bWLFunc != bWhiteLightFunc)
		{
			bWLFunc = bWhiteLightFunc;
			bStarSwitched =TRUE;
			bAeNight =FALSE;
			JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
			SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
		}
		
	
		//ret = jv_sensor_CheckDay_By_Sensor(jvAvm,jv_exp,Night_To_Day_ThrLuma,Day_To_Night_ThrLuma,pSoft_Light_Sensitive);
		if(bAlarmed)
		{
		
		}	
		else if(StarLightEnable)
		{

			if(bStarSwitched)
			{
				bCheckNightStatus =FALSE;
				bCheckDayStatus  =FALSE;
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
				if(bCheckNightLowFrameStatus!=bNightLowframe) //
				{
					jv_sensor_low_frame_inner(0,bNightLowframe);
					bCheckNightLowFrameStatus=bNightLowframe;
					printf("star light night low frame change..\n");
				}
				bCheckDayStatus =FALSE;
				
				if(bAeNight)
				{

					if(alarmTick >=ALARM_DELAY_TICK )
					{
						
						int retttt = jv_sensor_smart_judge(bNightMode,LightAeIndex,100);//无adc，传100吧
						bAeNight =retttt;
					}
						
					//printf("meeeee use soft bDay is %d\n",bDay);
				}
				
				if(bAeNight ==FALSE&&SensorState!=SENSOR_STATE_DAY_FULL_FRAMRATE) 
				{
					SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
					JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					printf("star light night to full framerate\n");
					
				}
				
			}
			else
			{
				int retttt = jv_sensor_smart_judge(bNightMode,LightAeIndex,100);//无adc，传100吧
				bCheckNightStatus =FALSE;
				if(bCheckDayStatus==FALSE)//
				{
					SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
					JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					bCheckDayStatus =TRUE;
					printf("star light day init\n");

					bWhiteMode = FALSE;
					whiteLightExp = 0;
					jv_sensor_set_whitelight(FALSE);//关白光灯
						
					whiteTodayTick1 = whiteTodayTick0 = whiteTodayTick2 =0;		
					timcnt[4] = 0;
					timcnt[5] = 0;
				}
				else
				{
					
					
					if((bAeNight==FALSE) && (SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE || SensorState==SENSOR_STATE_DAY_QUARTER_FRAMRATE) && jv_exp<=u32FirstExpDtoLThrd)//from 6 or 12 to 25 fps
					{
						timcnt[0]++;
						if(timcnt[0] >= 3)
						{
							JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
							SensorState = SENSOR_STATE_DAY_FULL_FRAMRATE;
							printf("enter sensor full 25fps state %d\n",SensorState);
							timcnt[0] = 0;
						}
					}
					else
						timcnt[0] = 0;
					 if(bAeNight==FALSE && jv_exp > u32FirstExpDtoLThrd )//to 6 fps or 12 fps
					{
						if(SensorState !=SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp >= u32SecondExpLtoDThrd && bSecondLow&&jvAvm<=avm5fps_thrd)// enter 6 fps
						{ 
							timcnt[1] = 0;
							
							timcnt[2]++;
							if(timcnt[2] >= 1 )
							{
								JV_ISP_COMM_Set_LowFps(SecondRatio);
								SensorState = SENSOR_STATE_DAY_QUARTER_FRAMRATE;
								printf("enter sensor 6fps state %d\n",SensorState);
								timcnt[2] = 0;
							}
						}
						else if((SensorState == SENSOR_STATE_DAY_FULL_FRAMRATE && jv_exp>=u32FirstExpLtoDThrd && bFirstLow)
							||(SensorState == SENSOR_STATE_DAY_QUARTER_FRAMRATE && jv_exp <= u32SecondExpDtoLThrd && bFirstLow))//enter 12 fps
						{
							timcnt[2] = 0;
							
							timcnt[1]++;
							if(timcnt[1] >= 1)
							{
								JV_ISP_COMM_Set_LowFps(FirstRatio);
								SensorState = SENSOR_STATE_DAY_HALF_FRAMRATE;
								printf("enter sensor 12.5fps state %d\n",SensorState);
								timcnt[1] = 0;
							}						 
						}
						else
						{
							timcnt[1] = 0;
							timcnt[2] = 0;
						}
					}
					
					if(bWLFunc)
					{
						if(alarmTick >=ALARM_DELAY_TICK&&bWhiteMode == FALSE &&jv_exp>= 3262*30*64 && jvAvm<= 47 &&bAeNight ==FALSE)
						{
							timcnt[4]++;
							if(timcnt[4] >= 2)
							{
								jv_sensor_set_whitelight(TRUE);//开白光灯
								printf("open whilelihgt 8888888888888888888888888888888888\n");
								bWhiteMode = TRUE;
								sleep(1); //延时2s
								whiteLightExp = 0;
								whiteTodayTick1 = whiteTodayTick0 = whiteTodayTick2 =0;		
								
								timcnt[4] = 0;
								timcnt[5] = 0;
							}
							
						}
						else
							timcnt[4] = 0;
						
						if(bWhiteMode&& timcnt[5] <60*1)
						{
							timcnt[5]++;
							if(timcnt[5] == 10)
							{
								whiteLightExp =jv_exp;
								//timcnt[5] =0;
							}			
						}
						
						if(whiteLightExp && timcnt[5] >=60*1 )
						{
							if(jv_exp<=1629*4*64) 
							{
								if(whiteLightExp <1200*64)//小场景
								{
									unsigned int diffExp = whiteLightExp/5;
									if(diffExp<4*64)
										diffExp =4*64;
									if((jv_exp>(whiteLightExp + diffExp)) || (whiteLightExp >(jv_exp+diffExp)))
									{
										whiteTodayTick0++;
										printf("whiteTodayTick0 is %d\n",whiteTodayTick0);
									}
									else
										whiteTodayTick0 =0;
									
									
									whiteTodayTick1 =0;	
									//whiteTodayTick2 = 0;
										
								}
								else
								{
									unsigned int diffExp = whiteLightExp/3;
									if(diffExp<4*64)
										diffExp =4*64;
									if( whiteLightExp >(jv_exp+diffExp))
									{
										whiteTodayTick1++;
										printf("whiteTodayTick1 is %d\n",whiteTodayTick1);
									}
									else
										whiteTodayTick1 =0;	
									
									whiteTodayTick0 =0;	
									//whiteTodayTick2 = 0;									
								}
								
								if(jv_exp <= 160*64)
								{
									whiteTodayTick2++;	
									printf("whiteTodayTick2 is %d\n",whiteTodayTick2);									
								}
								else
									whiteTodayTick2 = 0;
							}
							else
							{
									whiteTodayTick1 = whiteTodayTick0 = whiteTodayTick2 =0;	
							}
							
							if(whiteTodayTick1 >= 10 ||whiteTodayTick0 >= 10 || whiteTodayTick2 >= 20)  //关白光灯
							{
								jv_sensor_set_whitelight(FALSE);//关白光灯
								printf("close whilelight 8888888888888888888888888888888888\n");
								bWhiteMode = FALSE;
								whiteTodayTick1 = whiteTodayTick0 = whiteTodayTick2 =0;		
							}
						}
						//printf("timcnt[5] is %d\n",timcnt[5]);
						if(bWhiteMode&&timcnt[5]>5 &&bAeNight==FALSE&&jv_exp>= ToNightExpThrd && jvAvm<= 40&&retttt>0)  //进入到红外黑白模式
						{
							timcnt[3]++;
							//printf("time[3] is adc read %d--timecnt %d\n",timcnt[3]);
							if(timcnt[3] >= 1)
							{
								if(DayNightMode ==DAYNIGHT_AUTO)
								{
									bAeNight = TRUE;
									SensorState = SENSOR_STATE_NIGHT;
									jv_sensor_set_whitelight(FALSE);
									printf("close whilelight 8888882222222222222222\n");
									bWhiteMode =FALSE;
									printf("enter sensor night state ToNightLumThrd %hhu jvAvm %hhu SensorState %d\n",ToNightLumThrd,jvAvm,SensorState);
								}
								timcnt[3] = 0;
							}
						}
						else
							timcnt[3] = 0;	
					
					}
					else if((bAeNight==FALSE)&&jv_exp>= ToNightExpThrd && jvAvm<= 46&&retttt>0)//enter night mode
					{
						timcnt[3]++;
						if(timcnt[3] >= 1)
						{
							if(DayNightMode ==DAYNIGHT_AUTO)
							{
								bAeNight = TRUE;
								SensorState = SENSOR_STATE_NIGHT;
								printf("enter sensor night state ToNightLumThrd %hhu jvAvm %hhu SensorState %d\n",ToNightLumThrd,jvAvm,SensorState);
							}
							timcnt[3] = 0;
						}
					}
					else
						timcnt[3] = 0;	
				}

			}
		}
		else 
		{
			if(bStarSwitched)
			{
				bCheckNightStatus =FALSE;
				bCheckDayStatus  =FALSE;
				bCheckNightLowFrameStatus =FALSE;
				bStarSwitched =FALSE;
				bWhiteMode = FALSE;
				jv_sensor_set_whitelight(FALSE);//关白光灯
			}
			if(bNightMode)
			{
				if(bCheckNightStatus==FALSE)
				{
					bCheckNightStatus= TRUE;
					jv_sensor_low_frame_inner(0,bCheckNightLowFrameStatus);	
					printf("no star night init ok\n");
				}
				if(bCheckNightLowFrameStatus!=bNightLowframe) //
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
				if(bCheckDayStatus==FALSE)//
				{
					SensorState=SENSOR_STATE_DAY_FULL_FRAMRATE;
					JV_ISP_COMM_Set_LowFps(FULL_FRAMERATE_RATIO);
					bCheckDayStatus =TRUE;
					printf("no star day init ok\n");
				}
				
			}
			
			
		}
		
		
		
	}
		

	if(!bAE_Stable)
		AE_Stable_cnt ++;
	if(AE_Stable_cnt > 20)
	{
		bAE_Stable = HI_TRUE;
		AE_Stable_cnt = 0;
		jv_sensor_night_ae_speed_set(FALSE);
	}
	pthread_mutex_unlock(&low_frame_mutex);
}


#endif
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

	
NRS_PARAM_V2_S MN34227_NRS[] = {
	//小光圈参数
	{//ISO=100
		.SBS0=140,.SBS1=100,.SBS2=100,.SBS3=5,.SDS0=140,.SDS1=100,.SDS2=100,.SDS3=5,
		.STH0=100,.STH1=100,.STH2=100,.STH3=100,.MATH1=100,.MATH2=100,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=20,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=200
		.SBS0=160,.SBS1=120,.SBS2=120,.SBS3=15,.SDS0=160,.SDS1=120,.SDS2=120,.SDS3=15,
		.STH0=106,.STH1=106,.STH2=106,.STH3=106,.MATH1=105,.MATH2=105,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=42,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=400
		.SBS0=120,.SBS1=120,.SBS2=120,.SBS3=25,.SDS0=120,.SDS1=120,.SDS2=120,.SDS3=25,
		.STH0=112,.STH1=112,.STH2=112,.STH3=112,.MATH1=110,.MATH2=110,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=64,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=800
		.SBS0=120,.SBS1=120,.SBS2=120,.SBS3=40,.SDS0=120,.SDS1=120,.SDS2=100,.SDS3=40,
		.STH0=118,.STH1=118,.STH2=118,.STH3=118,.MATH1=115,.MATH2=115,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=96,.TFC=6,.TPC=6,.TRC=16
	},
	{//ISO=1600
		.SBS0=130,.SBS1=135,.SBS2=135,.SBS3=50,.SDS0=130,.SDS1=135,.SDS2=135,.SDS3=50,
		.STH0=125,.STH1=125,.STH2=125,.STH3=125,.MATH1=125,.MATH2=125,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=128,.TFC=10,.TPC=10,.TRC=30
	},
	{//ISO=3200
		.SBS0=130,.SBS1=140,.SBS2=140,.SBS3=50,.SDS0=140,.SDS1=140,.SDS2=140,.SDS3=50,
		.STH0=130,.STH1=130,.STH2=130,.STH3=130,.MATH1=130,.MATH2=130,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=10,.TPC=10,.TRC=40
	},
	{//ISO=6400
		.SBS0=170,.SBS1=180,.SBS2=180,.SBS3=60,.SDS0=170,.SDS1=180,.SDS2=180,.SDS3=60,
		.STH0=130,.STH1=130,.STH2=130,.STH3=130,.MATH1=136,.MATH2=136,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=14,.TPC=14,.TRC=60
	},
	{//ISO=12800
		.SBS0=185,.SBS1=190,.SBS2=190,.SBS3=60,.SDS0=185,.SDS1=190,.SDS2=190,.SDS3=60,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=139,.MATH2=139,.TFS1=11,.TFS2=11,
		.MDDZ1=48,.MDDZ2=48,.MDP=2,.SFC=190,.TFC=16,.TPC=16,.TRC=80
	},
	{//ISO=25600
		.SBS0=200,.SBS1=200,.SBS2=200,.SBS3=60,.SDS0=200,.SDS1=200,.SDS2=200,.SDS3=60,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=142,.MATH2=142,.TFS1=11,.TFS2=11,
		.MDDZ1=80,.MDDZ2=80,.MDP=2,.SFC=200,.TFC=16,.TPC=16,.TRC=100
	},
	{//ISO=51200
		.SBS0=255,.SBS1=255,.SBS2=255,.SBS3=60,.SDS0=255,.SDS1=255,.SDS2=255,.SDS3=60,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=150,.MATH2=150,.TFS1=11,.TFS2=11,
		.MDDZ1=128,.MDDZ2=128,.MDP=2,.SFC=200,.TFC=20,.TPC=20,.TRC=120
	},
};
//原中维大光圈参数
/*	{//ISO=100
		.SBS0=140,.SBS1=100,.SBS2=100,.SBS3=0,.SDS0=140,.SDS1=100,.SDS2=100,.SDS3=0,
		.STH0=100,.STH1=100,.STH2=100,.STH3=100,.MATH1=98,.MATH2=98,.TFS1=7,.TFS2=7,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=20,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=200
		.SBS0=160,.SBS1=120,.SBS2=120,.SBS3=0,.SDS0=160,.SDS1=120,.SDS2=120,.SDS3=0,
		.STH0=104,.STH1=104,.STH2=104,.STH3=104,.MATH1=102,.MATH2=102,.TFS1=7,.TFS2=7,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=42,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=400
		.SBS0=120,.SBS1=120,.SBS2=120,.SBS3=0,.SDS0=120,.SDS1=120,.SDS2=120,.SDS3=0,
		.STH0=110,.STH1=110,.STH2=110,.STH3=110,.MATH1=106,.MATH2=106,.TFS1=7,.TFS2=7,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=64,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=800
		.SBS0=120,.SBS1=120,.SBS2=120,.SBS3=10,.SDS0=120,.SDS1=120,.SDS2=100,.SDS3=10,
		.STH0=116,.STH1=116,.STH2=116,.STH3=116,.MATH1=114,.MATH2=114,.TFS1=8,.TFS2=8,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=96,.TFC=6,.TPC=6,.TRC=16
	},
	{//ISO=1600
		.SBS0=130,.SBS1=135,.SBS2=135,.SBS3=20,.SDS0=130,.SDS1=135,.SDS2=135,.SDS3=20,
		.STH0=120,.STH1=120,.STH2=120,.STH3=120,.MATH1=122,.MATH2=124,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=128,.TFC=10,.TPC=10,.TRC=30
	},
	{//ISO=3200
		.SBS0=130,.SBS1=140,.SBS2=140,.SBS3=30,.SDS0=140,.SDS1=140,.SDS2=140,.SDS3=30,
		.STH0=128,.STH1=128,.STH2=128,.STH3=128,.MATH1=128,.MATH2=128,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=10,.TPC=10,.TRC=40
	},
	{//ISO=6400
		.SBS0=170,.SBS1=180,.SBS2=180,.SBS3=30,.SDS0=170,.SDS1=180,.SDS2=180,.SDS3=30,
		.STH0=130,.STH1=130,.STH2=130,.STH3=130,.MATH1=136,.MATH2=136,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=14,.TPC=14,.TRC=60
	},
	{//ISO=12800
		.SBS0=180,.SBS1=190,.SBS2=190,.SBS3=30,.SDS0=190,.SDS1=190,.SDS2=160,.SDS3=30,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=139,.MATH2=139,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=16,.TPC=16,.TRC=80
	},
	{//ISO=25600
		.SBS0=180,.SBS1=200,.SBS2=200,.SBS3=30,.SDS0=180,.SDS1=200,.SDS2=200,.SDS3=30,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=142,.MATH2=142,.TFS1=11,.TFS2=11,
		.MDDZ1=64,.MDDZ2=64,.MDP=2,.SFC=200,.TFC=16,.TPC=16,.TRC=100
	},
	{//ISO=51200
		.SBS0=220,.SBS1=240,.SBS2=240,.SBS3=25,.SDS0=220,.SDS1=240,.SDS2=240,.SDS3=50,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=143,.MATH2=143,.TFS1=11,.TFS2=11,
		.MDDZ1=128,.MDDZ2=128,.MDP=2,.SFC=200,.TFC=20,.TPC=20,.TRC=120
	},
};*/

NRS_PARAM_V2_S AR0237_NRS[] = {
	{//ISO=100
		.SBS0=120,.SBS1=0,.SBS2=70,.SBS3=0,.SDS0=120,.SDS1=70,.SDS2=0,.SDS3=0,
		.STH0=104,.STH1=104,.STH2=104,.STH3=104,.MATH1=98,.MATH2=98,.TFS1=7,.TFS2=7,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=20,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=200
		.SBS0=140,.SBS1=0,.SBS2=100,.SBS3=0,.SDS0=140,.SDS1=100,.SDS2=0,.SDS3=0,
		.STH0=108,.STH1=108,.STH2=108,.STH3=108,.MATH1=102,.MATH2=102,.TFS1=7,.TFS2=7,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=42,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=400
		.SBS0=120,.SBS1=56,.SBS2=120,.SBS3=0,.SDS0=120,.SDS1=120,.SDS2=56,.SDS3=0,
		.STH0=112,.STH1=112,.STH2=112,.STH3=112,.MATH1=106,.MATH2=106,.TFS1=7,.TFS2=7,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=64,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=800
		.SBS0=120,.SBS1=80,.SBS2=120,.SBS3=0,.SDS0=120,.SDS1=120,.SDS2=80,.SDS3=0,
		.STH0=116,.STH1=116,.STH2=116,.STH3=116,.MATH1=114,.MATH2=114,.TFS1=8,.TFS2=8,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=96,.TFC=6,.TPC=6,.TRC=16
	},
	{//ISO=1600
		.SBS0=120,.SBS1=80,.SBS2=138,.SBS3=6,.SDS0=120,.SDS1=138,.SDS2=80,.SDS3=6,
		.STH0=120,.STH1=120,.STH2=120,.STH3=120,.MATH1=118,.MATH2=118,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=128,.TFC=10,.TPC=10,.TRC=30
	},
	{//ISO=3200
		.SBS0=130,.SBS1=86,.SBS2=156,.SBS3=10,.SDS0=130,.SDS1=156,.SDS2=86,.SDS3=10,
		.STH0=128,.STH1=128,.STH2=128,.STH3=128,.MATH1=124,.MATH2=124,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=10,.TPC=10,.TRC=40
	},
	{//ISO=6400
		.SBS0=140,.SBS1=112,.SBS2=180,.SBS3=10,.SDS0=140,.SDS1=180,.SDS2=112,.SDS3=10,
		.STH0=130,.STH1=130,.STH2=130,.STH3=130,.MATH1=128,.MATH2=128,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=14,.TPC=14,.TRC=60
	},
	{//ISO=12800
		.SBS0=160,.SBS1=150,.SBS2=200,.SBS3=24,.SDS0=160,.SDS1=200,.SDS2=150,.SDS3=24,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=132,.MATH2=132,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=16,.TPC=16,.TRC=80
	},
	{//ISO=25600
		.SBS0=180,.SBS1=160,.SBS2=220,.SBS3=20,.SDS0=180,.SDS1=220,.SDS2=160,.SDS3=40,
		.STH0=135,.STH1=135,.STH2=135,.STH3=135,.MATH1=136,.MATH2=136,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=200,.TFC=16,.TPC=16,.TRC=100
	}
};

NRS_PARAM_V2_S IMX291_NRS[] = {
	{//ISO=100
		.SBS0=100,.SBS1=0,.SBS2=60,.SBS3=0,.SDS0=100,.SDS1=60,.SDS2=0,.SDS3=0,
		.STH0=100,.STH1=100,.STH2=100,.STH3=100,.MATH1=112,.MATH2=112,.TFS1=7,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=10,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=200
		.SBS0=140,.SBS1=40,.SBS2=100,.SBS3=0,.SDS0=140,.SDS1=100,.SDS2=40,.SDS3=0,
		.STH0=104,.STH1=104,.STH2=104,.STH3=104,.MATH1=112,.MATH2=112,.TFS1=7,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=16,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=400
		.SBS0=160,.SBS1=40,.SBS2=140,.SBS3=0,.SDS0=160,.SDS1=140,.SDS2=40,.SDS3=0,
		.STH0=110,.STH1=110,.STH2=110,.STH3=110,.MATH1=112,.MATH2=112,.TFS1=7,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=40,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=800
		.SBS0=180,.SBS1=40,.SBS2=160,.SBS3=0,.SDS0=180,.SDS1=160,.SDS2=40,.SDS3=0,
		.STH0=116,.STH1=116,.STH2=116,.STH3=116,.MATH1=112,.MATH2=112,.TFS1=8,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=56,.TFC=6,.TPC=6,.TRC=16
	},
	{//ISO=1600
		.SBS0=180,.SBS1=60,.SBS2=160,.SBS3=0,.SDS0=180,.SDS1=160,.SDS2=60,.SDS3=0,
		.STH0=120,.STH1=120,.STH2=120,.STH3=120,.MATH1=114,.MATH2=114,.TFS1=8,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=68,.TFC=10,.TPC=10,.TRC=30
	},
	{//ISO=3200
		.SBS0=180,.SBS1=100,.SBS2=160,.SBS3=0,.SDS0=180,.SDS1=160,.SDS2=100,.SDS3=0,
		.STH0=128,.STH1=128,.STH2=128,.STH3=128,.MATH1=118,.MATH2=118,.TFS1=9,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=80,.TFC=10,.TPC=10,.TRC=40
	},
	{//ISO=6400
		.SBS0=190,.SBS1=100,.SBS2=160,.SBS3=20,.SDS0=190,.SDS1=160,.SDS2=100,.SDS3=20,
		.STH0=130,.STH1=130,.STH2=130,.STH3=130,.MATH1=122,.MATH2=122,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=90,.TFC=10,.TPC=10,.TRC=70
	},
	{//ISO=12800
		.SBS0=180,.SBS1=180,.SBS2=200,.SBS3=30,.SDS0=180,.SDS1=200,.SDS2=180,.SDS3=30,
		.STH0=138,.STH1=138,.STH2=138,.STH3=138,.MATH1=126,.MATH2=126,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=120,.TFC=12,.TPC=12,.TRC=85
	},
	{//ISO=25600
		.SBS0=180,.SBS1=255,.SBS2=255,.SBS3=30,.SDS0=180,.SDS1=255,.SDS2=255,.SDS3=30,
		.STH0=146,.STH1=146,.STH2=146,.STH3=146,.MATH1=130,.MATH2=130,.TFS1=11,.TFS2=11,
		.MDDZ1=64,.MDDZ2=64,.MDP=2,.SFC=150,.TFC=12,.TPC=12,.TRC=100
	},
	{//ISO=51200
		.SBS0=220,.SBS1=255,.SBS2=255,.SBS3=30,.SDS0=220,.SDS1=255,.SDS2=255,.SDS3=30,
		.STH0=152,.STH1=152,.STH2=152,.STH3=152,.MATH1=133,.MATH2=133,.TFS1=11,.TFS2=11,
		.MDDZ1=128,.MDDZ2=128,.MDP=2,.SFC=180,.TFC=12,.TPC=12,.TRC=120
	},
	{//ISO=102400
		.SBS0=220,.SBS1=255,.SBS2=255,.SBS3=60,.SDS0=220,.SDS1=255,.SDS2=255,.SDS3=60,
		.STH0=160,.STH1=160,.STH2=160,.STH3=160,.MATH1=140,.MATH2=140,.TFS1=11,.TFS2=11,
		.MDDZ1=128,.MDDZ2=128,.MDP=2,.SFC=200,.TFC=20,.TPC=20,.TRC=120
	},
};

NRS_PARAM_V2_S OV2735_NRS[] = {
	{//ISO=100
		.SBS0=120,.SBS1=150,.SBS2=0,.SBS3=0,.SDS0=120,.SDS1=150,.SDS2=0,.SDS3=0,
		.STH0=110,.STH1=110,.STH2=110,.STH3=110,.MATH1=100,.MATH2=100,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=20,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=200
		.SBS0=140,.SBS1=150,.SBS2=0,.SBS3=0,.SDS0=140,.SDS1=150,.SDS2=0,.SDS3=0,
		.STH0=115,.STH1=115,.STH2=115,.STH3=115,.MATH1=104,.MATH2=104,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=42,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=400
		.SBS0=160,.SBS1=160,.SBS2=0,.SBS3=0,.SDS0=160,.SDS1=160,.SDS2=0,.SDS3=0,
		.STH0=112,.STH1=112,.STH2=112,.STH3=112,.MATH1=108,.MATH2=108,.TFS1=9,.TFS2=9,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=64,.TFC=0,.TPC=0,.TRC=10
	},
	{//ISO=800
		.SBS0=180,.SBS1=170,.SBS2=0,.SBS3=0,.SDS0=180,.SDS1=170,.SDS2=0,.SDS3=0,
		.STH0=116,.STH1=116,.STH2=116,.STH3=116,.MATH1=114,.MATH2=114,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=96,.TFC=6,.TPC=6,.TRC=16
	},
	{//ISO=1600
		.SBS0=190,.SBS1=180,.SBS2=0,.SBS3=0,.SDS0=190,.SDS1=180,.SDS2=0,.SDS3=0,
		.STH0=134,.STH1=134,.STH2=134,.STH3=134,.MATH1=123,.MATH2=123,.TFS1=10,.TFS2=10,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=128,.TFC=10,.TPC=10,.TRC=30
	},
	{//ISO=3200
		.SBS0=200,.SBS1=196,.SBS2=0,.SBS3=0,.SDS0=200,.SDS1=196,.SDS2=0,.SDS3=0,
		.STH0=134,.STH1=134,.STH2=134,.STH3=134,.MATH1=126,.MATH2=126,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=10,.TPC=10,.TRC=40
	},
	{//ISO=6400
		.SBS0=220,.SBS1=218,.SBS2=0,.SBS3=25,.SDS0=220,.SDS1=218,.SDS2=0,.SDS3=25,
		.STH0=145,.STH1=145,.STH2=145,.STH3=145,.MATH1=130,.MATH2=130,.TFS1=11,.TFS2=11,
		.MDDZ1=32,.MDDZ2=32,.MDP=2,.SFC=190,.TFC=14,.TPC=14,.TRC=60
	}
};



VPSS_GRP_NRS_PARAM_S * jv_sensor_3DnrParam_Cal(VPSS_GRP_NRS_PARAM_S *pNrsParam, HI_U32 u32ISO)
{
	
	HI_U8 u8Index = 0;
	HI_U32 u32ISO1 = 0;
	NRS_PARAM_V2_S * pNrs1 = NULL;
	NRS_PARAM_V2_S * pNrs2 = NULL;
	HI_U32 u32ISOTmp = u32ISO / 100;
	if(NULL == pNrsParam)
	{
		printf("Error:%s....%d....param pNrsParam is NULL\n",__func__,__LINE__);
		return NULL;
	}
	
	for(; u8Index < MAX_ISO_TBL_INDEX;u8Index++)        
	{            
		if(1 == u32ISOTmp)            
		{                
			break;            
		}            
		u32ISOTmp >>= 1;     
	}

	if(MAX_ISO_INDEX < u8Index)
	{
		return NULL;
	}
	else if(MAX_ISO_INDEX == u8Index)
	{
		pNrs1 = &(pNrsParam_V300[u8Index]);
		pNrs2 = &(pNrsParam_V300[u8Index]);
	}
	else
	{
		pNrs1 = &(pNrsParam_V300[u8Index]);
		pNrs2 = &(pNrsParam_V300[u8Index + 1]);
	}
	u32ISO1 = 100<<u8Index;

	pNrsParam->stNRSParam_V2.SBS0 = ((u32ISO1*2 - u32ISO)*pNrs1->SBS0 + (u32ISO - u32ISO1)*pNrs2->SBS0 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SBS1 = ((u32ISO1*2 - u32ISO)*pNrs1->SBS1 + (u32ISO - u32ISO1)*pNrs2->SBS1 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SBS2 = ((u32ISO1*2 - u32ISO)*pNrs1->SBS2 + (u32ISO - u32ISO1)*pNrs2->SBS2 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SBS3 = ((u32ISO1*2 - u32ISO)*pNrs1->SBS3 + (u32ISO - u32ISO1)*pNrs2->SBS3 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SDS0 = ((u32ISO1*2 - u32ISO)*pNrs1->SDS0 + (u32ISO - u32ISO1)*pNrs2->SDS0 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SDS1 = ((u32ISO1*2 - u32ISO)*pNrs1->SDS1 + (u32ISO - u32ISO1)*pNrs2->SDS1 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SDS2 = ((u32ISO1*2 - u32ISO)*pNrs1->SDS2 + (u32ISO - u32ISO1)*pNrs2->SDS2 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SDS3 = ((u32ISO1*2 - u32ISO)*pNrs1->SDS3 + (u32ISO - u32ISO1)*pNrs2->SDS3 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.STH0 = ((u32ISO1*2 - u32ISO)*pNrs1->STH0 + (u32ISO - u32ISO1)*pNrs2->STH0 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.STH1 = ((u32ISO1*2 - u32ISO)*pNrs1->STH1 + (u32ISO - u32ISO1)*pNrs2->STH1 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.STH2 = ((u32ISO1*2 - u32ISO)*pNrs1->STH2 + (u32ISO - u32ISO1)*pNrs2->STH2 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.STH3 = ((u32ISO1*2 - u32ISO)*pNrs1->STH3 + (u32ISO - u32ISO1)*pNrs2->STH3 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.MATH1 = ((u32ISO1*2 - u32ISO)*pNrs1->MATH1 + (u32ISO - u32ISO1)*pNrs2->MATH1 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.MATH2 = ((u32ISO1*2 - u32ISO)*pNrs1->MATH2 + (u32ISO - u32ISO1)*pNrs2->MATH2 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.MDDZ1 = ((u32ISO1*2 - u32ISO)*pNrs1->MDDZ1 + (u32ISO - u32ISO1)*pNrs2->MDDZ1 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.MDDZ2 = ((u32ISO1*2 - u32ISO)*pNrs1->MDDZ2 + (u32ISO - u32ISO1)*pNrs2->MDDZ2 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.TFS1 = ((u32ISO1*2 - u32ISO)*pNrs1->TFS1 + (u32ISO - u32ISO1)*pNrs2->TFS1 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.TFS2 = ((u32ISO1*2 - u32ISO)*pNrs1->TFS2 + (u32ISO - u32ISO1)*pNrs2->TFS2 + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.MDP = ((u32ISO1*2 - u32ISO)*pNrs1->MDP + (u32ISO - u32ISO1)*pNrs2->MDP + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.SFC = ((u32ISO1*2 - u32ISO)*pNrs1->SFC + (u32ISO - u32ISO1)*pNrs2->SFC + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.TFC = ((u32ISO1*2 - u32ISO)*pNrs1->TFC + (u32ISO - u32ISO1)*pNrs2->TFC + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.TPC = ((u32ISO1*2 - u32ISO)*pNrs1->TPC + (u32ISO - u32ISO1)*pNrs2->TPC + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.TRC = ((u32ISO1*2 - u32ISO)*pNrs1->TRC + (u32ISO - u32ISO1)*pNrs2->TRC + u32ISO1/2)/u32ISO1;
	pNrsParam->stNRSParam_V2.IES0 = 0;
	pNrsParam->stNRSParam_V2.Pro3 = 0;
	pNrsParam->stNRSParam_V2.LNTH = 0;

	return pNrsParam;

}
void jv_sensor_constract_as_luma(SensorType_e sensor,HI_U32 u32ISO)
{
	static HI_U8 HighLightCnt = 0;
	static HI_U8 LowLightCnt = 0;
	static HI_U32 HToLThr = 5000;
	static HI_U32 LToHThr = 3600;

	if(sensor == SENSOR_MN34227)
	{
		HToLThr = 5000;
		LToHThr = 3600;
	}
	else if(sensor == SENSOR_AR0237DC)
	{
		HToLThr = 1800;
		LToHThr = 1200;
	}
	else if(sensor == SENSOR_IMX291)
	{
		HToLThr = 5600;
		LToHThr = 3600;
	}
	else if(sensor == SENSOR_OV2735)
	{
		HToLThr = 1800;
		LToHThr = 1200;
	}

	
	pthread_mutex_lock(&isp_gamma_mutex);
	if(bNightMode == FALSE)
	{
		if(u32ISO >=HToLThr)//&&stAvm<32)
		{
			LowLightCnt++;
			if(LowLightCnt>=3)
			{
				if(bLowLightState ==FALSE)
				{
					bLowLightState =TRUE;
					isp_ioctl(0,ADJ_CONTRAST,255);
				}
				LowLightCnt =0;
				
			}
		}
		else 
			LowLightCnt =0;
		
		if(u32ISO <= LToHThr)
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
int jv_sensor_wdr_adapt(HI_U32 iso);
static HI_U32 u32ISO_Bias = 0;
static BOOL bDefog =FALSE;
static BOOL bNightInit =FALSE;
static BOOL bDayInit =FALSE;
static HI_S32 OV2735_DEFOG_STR[MAX_ISO_TBL_INDEX+1]={80,85,92,86,80,72,64,60,  50,50,50,50,50,50,50,50};
static HI_U32 tmp_iso_defog = 0;

void jv_sensor_pararm_adapt_as_luma(SensorType_e sensor,HI_U32 u32ISO)
{
	int evBias = 1024;
	HI_U8 u8Index =0;
	if(sensor == SENSOR_MN34227)
	{
		VPSS_GRP_ATTR_S stVpssGrpAttr;
		HI_MPI_VPSS_GetGrpAttr (0,&stVpssGrpAttr);
		if(u32ISO<=1000)
			stVpssGrpAttr.bSharpenEn = HI_FALSE;
		if(u32ISO>=1500)
			stVpssGrpAttr.bSharpenEn = HI_TRUE;
		HI_MPI_VPSS_SetGrpAttr (0,&stVpssGrpAttr);
		
		if(bNightMode == FALSE)
		{
			ISP_EXPOSURE_ATTR_S tmpExpAttr;
	        HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
	        evBias= tmpExpAttr.stAuto.u16EVBias ;
			if(tmpExpAttr.stAuto.u16EVBias ==1024)
			{
			 	if(u32ISO>= 10000)
					evBias =768;
			}
			else if(tmpExpAttr.stAuto.u16EVBias  ==768)
			{
				if(u32ISO<= 6000)
					evBias =1024;
			}
			else
			{
				evBias =1024;
			}
	    	/*if(tmpExpAttr.stAuto.u16EVBias ==1024)
			{
			 	if(u32ISO>= 6400)
					evBias =854;
				if(u32ISO>= 28000)
					evBias =768;
			}
			else if(tmpExpAttr.stAuto.u16EVBias ==854)
			{
			 	if(u32ISO<= 4000)
					evBias =1024;	
				if(u32ISO>= 24000)
					evBias =768;
			}
			else if(tmpExpAttr.stAuto.u16EVBias  ==768)
			{
				if(u32ISO<= 16000)
					evBias =854;
				if(u32ISO<= 3600)
					evBias =1024;
			}*/
			if(tmpExpAttr.stAuto.u16EVBias != evBias)
			{
				tmpExpAttr.stAuto.u16EVBias = evBias;
	        	HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);
			}

			ISP_COLOR_TONE_ATTR_S stCTAttr;
			HI_MPI_ISP_GetColorToneAttr(0,&stCTAttr);
			if(u32ISO<=6400)
			{
				stCTAttr.u16GreenCastGain = 260;
				stCTAttr.u16BlueCastGain = 258;
			}
	        if(u32ISO >= 8800)
	        {
	        	stCTAttr.u16GreenCastGain = 270;
				stCTAttr.u16BlueCastGain = 258;
	        }
			if(u32ISO >= 12800)
	        {
	        	stCTAttr.u16GreenCastGain = 280;
				stCTAttr.u16BlueCastGain = 258;
	        }
			HI_MPI_ISP_SetColorToneAttr(0,&stCTAttr);

			ISP_ACM_ATTR_S pstACMAttr;
			HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
			if(bLowLightState == TRUE)
			{
				if(pstACMAttr.bEnable == HI_TRUE)
				{
			        pstACMAttr.bEnable = HI_FALSE;       
			        HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
				}
			}
			else
			{
				if(pstACMAttr.bEnable == HI_FALSE)
				{
			        pstACMAttr.bEnable = HI_TRUE;       
			        HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
				}
			}
			ISP_CA_ATTR_S stCAAttr;
			HI_MPI_ISP_GetCAAttr(0, &stCAAttr);
			if(u32ISO <= 4800)
			{
				stCAAttr.bEnable=HI_FALSE;
				HI_MPI_ISP_SetCAAttr(0,&stCAAttr);
			}
			else if(u32ISO >= 6400)
			{
				stCAAttr.bEnable=HI_TRUE;
				HI_MPI_ISP_SetCAAttr(0,&stCAAttr);
			}

		}
		else
		{
			ISP_EXPOSURE_ATTR_S tmpExpAttr;
	        HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
	        evBias= tmpExpAttr.stAuto.u16EVBias ;
			
	    	if(tmpExpAttr.stAuto.u16EVBias ==1024)
			{
				if(u32ISO>= 10000)
					evBias =800;
			}
			else if(tmpExpAttr.stAuto.u16EVBias ==800)
			{
			 	if(u32ISO<= 6000)
					evBias =1024;
			}
			else
			{
				evBias =1024;
			}
			if(tmpExpAttr.stAuto.u16EVBias != evBias)
			{
				tmpExpAttr.stAuto.u16EVBias = evBias;
	        	HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);
			}
			
			ISP_COLOR_TONE_ATTR_S stCTAttr;
			HI_MPI_ISP_GetColorToneAttr(0,&stCTAttr);
			stCTAttr.u16GreenCastGain = 256;
			stCTAttr.u16BlueCastGain = 256;
			HI_MPI_ISP_SetColorToneAttr(0,&stCTAttr);
			
			ISP_CA_ATTR_S stCAAttr;
			HI_MPI_ISP_GetCAAttr(0, &stCAAttr);
			stCAAttr.bEnable=HI_FALSE;
			HI_MPI_ISP_SetCAAttr(0,&stCAAttr);

			ISP_ACM_ATTR_S pstACMAttr;
			HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
			if(pstACMAttr.bEnable == HI_TRUE)
			{
		        pstACMAttr.bEnable = HI_FALSE;       
		        HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
			}
		}

	}
	if(sensor == SENSOR_AR0237DC)
	{

        if(u32ISO<=5000)
        {
            evBias=1024;
                                
        }
        else if(u32ISO>=7000)
        {
            evBias=800;
        }
        ISP_EXPOSURE_ATTR_S tmpExpAttr;
        HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
        if(evBias>0)
            tmpExpAttr.stAuto.u16EVBias = evBias;
        HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);
		
	}
	if(sensor == SENSOR_IMX291)
	{
		
		ISP_EXPOSURE_ATTR_S tmpExpAttr;
        HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
        evBias= tmpExpAttr.stAuto.u16EVBias ;
		
    	if(tmpExpAttr.stAuto.u16EVBias ==1024)
		 {
		 	if(u32ISO>= 11000)
				evBias =921;
			if(u32ISO>= 46000)
				evBias =785;
		}
		else if(tmpExpAttr.stAuto.u16EVBias ==921)
		{
		 	if(u32ISO<= 6400)
				evBias =1024;
			
			if(u32ISO>= 41300)
				evBias =785;
		}
		else if(tmpExpAttr.stAuto.u16EVBias  ==785)
		{
			if(u32ISO<= 30000)
				evBias =921;
			if(u32ISO<= 6400)
				evBias =1024;
		}	
		if(tmpExpAttr.stAuto.u16EVBias != evBias)
		{
			tmpExpAttr.stAuto.u16EVBias = evBias;
        	HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);
		}

		//处理低照偏色问题
		HI_U16 BlackLevel = 240;
		ISP_BLACK_LEVEL_S stBlackLevel;
		if(u32ISO > 26000)
		{
			BlackLevel = 240 + (u32ISO - 26000)*24/(72000-26000);

		}
		else
		{
			BlackLevel = 240;
			BlackLevel = 240;
		}
		HI_MPI_ISP_GetBlackLevelAttr(0, &stBlackLevel);
		if((stBlackLevel.au16BlackLevel[0]+2) < BlackLevel || (stBlackLevel.au16BlackLevel[0] - 2) > BlackLevel)
		{
			stBlackLevel.au16BlackLevel[0]=BlackLevel;
			stBlackLevel.au16BlackLevel[1]=BlackLevel;
			stBlackLevel.au16BlackLevel[2]=BlackLevel;
			stBlackLevel.au16BlackLevel[3]=BlackLevel;
			HI_MPI_ISP_SetBlackLevelAttr(0, &stBlackLevel);
		}
		
		VPSS_GRP_ATTR_S stVpssGrpAttr;
		HI_MPI_VPSS_GetGrpAttr (0,&stVpssGrpAttr);
		if(u32ISO<=1000)
			stVpssGrpAttr.bSharpenEn = HI_FALSE;
		if(u32ISO>=1500)
			stVpssGrpAttr.bSharpenEn = HI_TRUE;
		HI_MPI_VPSS_SetGrpAttr (0,&stVpssGrpAttr);

		//处理低照度饱和度偏低问题
		if(SensorState == SENSOR_STATE_DAY_HALF_FRAMRATE || SensorState == SENSOR_STATE_DAY_QUARTER_FRAMRATE)
			jv_sensor_set_high_ca(HI_TRUE);
		else
			jv_sensor_set_high_ca(HI_FALSE);
		
	}
	if(sensor == SENSOR_OV2735)
	{
		jv_sensor_wdr_adapt(u32ISO);

		//defog setting of ON & OFF
		if(bNightMode == FALSE)
		{
			bNightInit =FALSE;
			
			if((bDefog !=bUserDefogEnable)||(bDayInit == FALSE))
			{
				bDayInit = TRUE;
				u32ISO_Bias =0;
				
				ISP_DEFOG_ATTR_S  pstDefogAttr;
				if(bUserDefogEnable)
				{
					
					HI_MPI_ISP_GetDeFogAttr(0,&pstDefogAttr);
					pstDefogAttr.bEnable=HI_TRUE;
					pstDefogAttr.enOpType =OP_TYPE_AUTO;
					pstDefogAttr.stAuto.u8strength=64;
					pstDefogAttr.bUserLutEnable=HI_FALSE;
					HI_MPI_ISP_SetDeFogAttr(0,&pstDefogAttr);
				}
				else
				{
					HI_MPI_ISP_GetDeFogAttr(0,&pstDefogAttr);
					pstDefogAttr.bEnable=HI_FALSE;
					pstDefogAttr.enOpType =OP_TYPE_MANUAL;
					pstDefogAttr.stManual.u8strength=80;
					pstDefogAttr.bUserLutEnable=HI_FALSE;
					HI_MPI_ISP_SetDeFogAttr(0,&pstDefogAttr);
				}
				
				VI_DCI_PARAM_S stDCIParam;
				HI_MPI_VI_GetDCIParam(0,  &stDCIParam);
				stDCIParam.bEnable =HI_TRUE;
				stDCIParam.u32BlackGain = 8;
				stDCIParam.u32ContrastGain = 16;
				stDCIParam.u32LightGain = 8;
				HI_MPI_VI_SetDCIParam(0,  &stDCIParam);

				
				ISP_COLOR_TONE_ATTR_S stCTAttr;
				HI_MPI_ISP_GetColorToneAttr(0,&stCTAttr);
				stCTAttr.u16GreenCastGain = 259;
				stCTAttr.u16BlueCastGain = 258;
				HI_MPI_ISP_SetColorToneAttr(0,&stCTAttr);
			}

			
		}
		else
		{
			bDayInit =FALSE;

			if((bDefog !=bUserDefogEnable)||(bNightInit == FALSE))
			{
				bNightInit =TRUE;
				u32ISO_Bias =0;
				
				ISP_DEFOG_ATTR_S  pstDefogAttr;
				if(bUserDefogEnable)
				{
					
					HI_MPI_ISP_GetDeFogAttr(0,&pstDefogAttr);
					pstDefogAttr.bEnable=HI_TRUE;
					pstDefogAttr.enOpType =OP_TYPE_AUTO;
					pstDefogAttr.stAuto.u8strength=64;
					pstDefogAttr.bUserLutEnable=HI_FALSE;
					HI_MPI_ISP_SetDeFogAttr(0,&pstDefogAttr);
				}
				else
				{
					HI_MPI_ISP_GetDeFogAttr(0,&pstDefogAttr);
					pstDefogAttr.bEnable=HI_TRUE;
					pstDefogAttr.enOpType =OP_TYPE_MANUAL;
					pstDefogAttr.stManual.u8strength=80;
					pstDefogAttr.bUserLutEnable=HI_TRUE;
					memcpy(&pstDefogAttr.au8DefogLut,&OV2735_DEFOG_LUT,256 * 1);
					HI_MPI_ISP_SetDeFogAttr(0,&pstDefogAttr);
				}

				VI_DCI_PARAM_S stDCIParam;
				HI_MPI_VI_GetDCIParam(0,  &stDCIParam);
				stDCIParam.bEnable =HI_TRUE;
				stDCIParam.u32BlackGain = 15;
				stDCIParam.u32ContrastGain = 30;
				stDCIParam.u32LightGain = 15;
				HI_MPI_VI_SetDCIParam(0,  &stDCIParam);

				ISP_COLOR_TONE_ATTR_S stCTAttr;
				HI_MPI_ISP_GetColorToneAttr(0,&stCTAttr);
				stCTAttr.u16GreenCastGain = 256;
				stCTAttr.u16BlueCastGain = 256;
				HI_MPI_ISP_SetColorToneAttr(0,&stCTAttr);
			}
			
		}



		if(u32ISO_Bias != u32ISO)
		{
			//BIAS setting
			ISP_EXPOSURE_ATTR_S tmpExpAttr;
			HI_MPI_ISP_GetExposureAttr(0, &tmpExpAttr);
			evBias= tmpExpAttr.stAuto.u16EVBias ;
			if(bNightMode ==FALSE)//白天 55--49--43
			{
				if(evBias ==1024)  //55-49(912)-43(800)
				{
				 	if(u32ISO>= 1200)
						evBias =912;
					if(u32ISO>= 4800)
						evBias =800;
				}
				else if(evBias ==912)
				{
				 	if(u32ISO<= 600)
						evBias =1024;
					
					if(u32ISO>= 4800)
						evBias =800;
				}
				else if(evBias ==800)
				{
					if(u32ISO<= 3200)
						evBias =912;
					if(u32ISO<= 600)
						evBias =1024;
				}	
			}
			else             //晚上
			{
				if(evBias == 1024)  //50-45(922)
				{
					if(u32ISO>= 2500)
						evBias =922;
				}
				else if(evBias ==922)
				{
					if(u32ISO<= 1500)
						evBias =1024;
				}
				else
				{
					evBias = 1024;
				}

			}
			if(evBias>0)
				tmpExpAttr.stAuto.u16EVBias = evBias;
		    HI_MPI_ISP_SetExposureAttr(0, &tmpExpAttr);
			
			//COLOR TONE  & acm  & defog str settings  by iso
			
			if(bNightMode == FALSE)
			{
				if(u32ISO<=600)
				{
					ISP_COLOR_TONE_ATTR_S stCTAttr;
					HI_MPI_ISP_GetColorToneAttr(0,&stCTAttr);
					stCTAttr.u16GreenCastGain = 259;
					stCTAttr.u16BlueCastGain = 258;
					HI_MPI_ISP_SetColorToneAttr(0,&stCTAttr);

					ISP_ACM_ATTR_S pstACMAttr;
					HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
					pstACMAttr.bEnable = HI_TRUE;		 
					HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
				}
		        if(u32ISO >= 2000)
		        {
		        	ISP_COLOR_TONE_ATTR_S stCTAttr;
					HI_MPI_ISP_GetColorToneAttr(0,&stCTAttr);
					stCTAttr.u16GreenCastGain = 259;
					stCTAttr.u16BlueCastGain = 256;
					HI_MPI_ISP_SetColorToneAttr(0,&stCTAttr);

					ISP_ACM_ATTR_S pstACMAttr;
					HI_MPI_ISP_GetAcmAttr(0,&pstACMAttr);
					pstACMAttr.bEnable = HI_FALSE;		 
					HI_MPI_ISP_SetAcmAttr(0,&pstACMAttr);
		        }
				//低照度偏紫问题优化
				ISP_BLACK_LEVEL_S stBlackLevel;
				HI_U16 BlackLevel = 250;
				if(u32ISO > 1000)
					BlackLevel = 252;
				if(u32ISO > 2800)
					BlackLevel = 254;

				HI_MPI_ISP_GetBlackLevelAttr(0, &stBlackLevel);
				stBlackLevel.au16BlackLevel[0]=BlackLevel;
				stBlackLevel.au16BlackLevel[3]=BlackLevel;
				HI_MPI_ISP_SetBlackLevelAttr(0, &stBlackLevel);
				
			}
			else
			{
				//defog STR settings by ISO
				tmp_iso_defog= u32ISO / 100;
				for(u8Index = 0; u8Index < MAX_ISO_TBL_INDEX;u8Index++) 	   
				{			 
					if(1 == tmp_iso_defog)			  
					{				 
						break;			  
					}			 
					tmp_iso_defog >>= 1;	 
				}
			
				ISP_DEFOG_ATTR_S  pstDefogAttr;
				HI_MPI_ISP_GetDeFogAttr(0,&pstDefogAttr);
				pstDefogAttr.stManual.u8strength=jv_IQ_AgcTableCalculate(OV2735_DEFOG_STR, u8Index, u32ISO); 
				HI_MPI_ISP_SetDeFogAttr(0,&pstDefogAttr);
			}
			
			
		}
		
		u32ISO_Bias = u32ISO;
		bDefog = bUserDefogEnable;
		
	}


	//if(!bUserSetGamma)
	if(sensor == SENSOR_MN34227)
		jv_sensor_constract_as_luma(sensor,u32ISO);
}

void jv_sensor_sensorbase_set(SensorType_e sensor)
{

	switch(sensor)
	{
		case SENSOR_AR0237DC:
	    {
	    
			bUseAdc = HI_TRUE;
			//设置降帧结构	
			pframe_ctrl_node = frame_ctrl_ae_list_ar0237;
			//设置切夜视结构
			plight_ctrl_node = light_ctrl_ae_list_ar0237;

			avm5fps_thrd =56;
			
			//设置ISP默认等级
			
			pCOMM_ISP_PARAMS = NULL;
			
			CISP_lEVEL.u8Definition = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeNoise = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeSmear = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8Fluency = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8framelevel = 3;

	    	MAX_ISO_INDEX = sizeof(AR0237_NRS)/sizeof(AR0237_NRS[0]) - 1; 
			pNrsParam_V300 = AR0237_NRS;

			VI_DCI_PARAM_S stDciParam;
			HI_MPI_VI_GetDCIParam(0,&stDciParam);
			stDciParam.bEnable = HI_FALSE;
			HI_MPI_VI_SetDCIParam(0,&stDciParam);
			
			ISP_AWB_ATTR_EX_S pstAWBAttrEx;
			HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
			pstAWBAttrEx.stInOrOut.u32OutThresh=10000;
			HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);
			
	    }
	   break;
	    case SENSOR_MN34227:
	    {

			bUseAdc = HI_TRUE;
			//设置降帧结构	
			pframe_ctrl_node = frame_ctrl_ae_list_mn34227;
			//设置切夜视结构
			plight_ctrl_node = light_ctrl_ae_list_mn34227;

			avm5fps_thrd =40;
			
			//设置ISP默认等级
			
			pCOMM_ISP_PARAMS = &MN34227_COMM_ISP_PARAMS;
			
			CISP_lEVEL.u8Definition = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeNoise = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeSmear = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8Fluency = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8framelevel = 3;
			//最大ISO的级数
			MAX_ISO_INDEX = sizeof(MN34227_NRS)/sizeof(MN34227_NRS[0]) - 1; 
			pNrsParam_V300 = MN34227_NRS;

			VI_DCI_PARAM_S stDciParam;
			HI_MPI_VI_GetDCIParam(0,&stDciParam);
			stDciParam.bEnable = HI_FALSE;
			HI_MPI_VI_SetDCIParam(0,&stDciParam);

			ISP_COLORMATRIX_ATTR_S stCCMAttr;
			HI_MPI_ISP_GetCCMAttr(0, &stCCMAttr);
			stCCMAttr.stAuto.bISOActEn = HI_TRUE;
			HI_MPI_ISP_SetCCMAttr(0, &stCCMAttr);
			//小光圈参数
			ISP_SHARPEN_ATTR_S stSharpenAttr;
			HI_MPI_ISP_GetSharpenAttr(0,&stSharpenAttr);
			memcpy(stSharpenAttr.stAuto.au8SharpenD,MN34227_COMM_ISP_PARAMS.SharpenAltD[ISP_SET_LEVEL_DEFAULT].data,16);
			memcpy(stSharpenAttr.stAuto.au8TextureThr,MN34227_COMM_ISP_PARAMS.TextureThr[ISP_SET_LEVEL_DEFAULT].data,16);
			memcpy(stSharpenAttr.stAuto.au8DetailCtrl,MN34227_COMM_ISP_PARAMS.DetailCtrl[ISP_SET_LEVEL_DEFAULT].data,16);
			HI_MPI_ISP_SetSharpenAttr(0,&stSharpenAttr);
			
			VPSS_GRP_SHARPEN_ATTR_S stGrpSharpenAttr;
			HI_MPI_VPSS_GetGrpSharpen(0,&stGrpSharpenAttr);
			stGrpSharpenAttr.enOpType = SHARPEN_OP_TYPE_AUTO;
			HI_U8 au8SharD[16]={30,30,30,30,30,40,50,60,50,50,50,50,200,200,200,200};
			HI_U8 au8SharUd[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			HI_U8 au8UnderShoot[16]={100 ,100 ,100,100,100,100,100,100,80,60,60,60,60,200,200,200};
			HI_U8 au8OverShoot[16]={50 ,50 ,50,50,50,50,50,50,30,20,10,10,10,200,200,200};
			HI_U8 au8ShootSupStr[16]={30 ,30 ,30,30,30,30,30,30,30,30,30,30,30,200,200,200};
			HI_U8 au8Detail[16]={128,128,128,128,128,128,110,110,100,100,100,100,100,100,200,200};
			HI_U8 au8SharpenEdge[16]={0};
			HI_U8 au8EdgeThd[16]={0};
			HI_U8 au8TextTr[16]={0 ,0,0,0,0,0,8,14,28,45,50,60,0,0,0,0};
			HI_U8 au8LumaWgt[32]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};

			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8SharpenD,&au8SharD,16);
            memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8SharpenUd,&au8SharUd,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8OverShoot,&au8OverShoot,16);
            memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8UnderShoot,&au8UnderShoot,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8ShootSupStr,&au8ShootSupStr,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8SharpenEdge,&au8SharpenEdge,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8EdgeThd,&au8EdgeThd,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8TextureThr,&au8TextTr,16);
            memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8DetailCtrl,&au8Detail,16);
			memcpy(&stGrpSharpenAttr.au8LumaWgt,&au8LumaWgt,32);
			HI_MPI_VPSS_SetGrpSharpen(0,&stGrpSharpenAttr);
			
			jv_sensor_night_ae_speed_set(FALSE);
	    }
		break;
		
		case SENSOR_IMX291:
	    {

			bUseAdc = HI_TRUE;
			//设置降帧结构	
			pframe_ctrl_node = frame_ctrl_ae_list_imx291;
			//设置切夜视结构
			plight_ctrl_node = light_ctrl_ae_list_imx291;

			avm5fps_thrd =40;
			
			//设置ISP默认等级
			
			pCOMM_ISP_PARAMS = NULL;
			
			CISP_lEVEL.u8Definition = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeNoise = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeSmear = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8Fluency = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8framelevel = 3;
			//最大ISO的级数
			MAX_ISO_INDEX = sizeof(IMX291_NRS)/sizeof(IMX291_NRS[0]) - 1; 
			pNrsParam_V300 = IMX291_NRS;

		
			VI_DCI_PARAM_S stDciParam;
			HI_MPI_VI_GetDCIParam(0,&stDciParam);
			stDciParam.bEnable = HI_FALSE;
			HI_MPI_VI_SetDCIParam(0,&stDciParam);

			ISP_COLORMATRIX_ATTR_S stCCMAttr;
			HI_MPI_ISP_GetCCMAttr(0, &stCCMAttr);
			stCCMAttr.stAuto.bISOActEn = HI_TRUE;
			HI_MPI_ISP_SetCCMAttr(0, &stCCMAttr);

			VPSS_GRP_SHARPEN_ATTR_S stGrpSharpenAttr;
			HI_MPI_VPSS_GetGrpSharpen(0,&stGrpSharpenAttr);
			stGrpSharpenAttr.enOpType = SHARPEN_OP_TYPE_AUTO;
			HI_U8 au8SharD[16]={30,30,30,30,30,40,50,60,50,50,50,50,200,200,200,200};
			HI_U8 au8SharUd[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			HI_U8 au8UnderShoot[16]={100 ,100 ,100,100,100,100,100,100,80,60,60,60,60,200,200,200};
			HI_U8 au8OverShoot[16]={50 ,50 ,50,50,50,50,50,50,30,20,10,10,10,200,200,200};
			HI_U8 au8ShootSupStr[16]={30 ,30 ,30,30,30,30,30,30,30,30,30,30,30,200,200,200};
			HI_U8 au8Detail[16]={128,128,128,128,128,128,110,110,100,100,100,100,100,100,200,200};
			HI_U8 au8SharpenEdge[16]={0};
			HI_U8 au8EdgeThd[16]={0};
			HI_U8 au8TextTr[16]={0 ,0,0,0,0,0,8,14,28,45,50,60,0,0,0,0};
			//HI_U8 au8OverShoot[16]={50 ,50 ,50,50,50,50,50,50,30,20,10,10,10,200,200,200};
			HI_U8 au8LumaWgt[32]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};

			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8SharpenD,&au8SharD,16);
            memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8SharpenUd,&au8SharUd,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8OverShoot,&au8OverShoot,16);
            memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8UnderShoot,&au8UnderShoot,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8ShootSupStr,&au8ShootSupStr,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8SharpenEdge,&au8SharpenEdge,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8EdgeThd,&au8EdgeThd,16);
			memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8TextureThr,&au8TextTr,16);
            memcpy(&stGrpSharpenAttr.stSharpenAutoAttr.au8DetailCtrl,au8Detail,16);
			memcpy(&stGrpSharpenAttr.au8LumaWgt,au8LumaWgt,32);
			HI_MPI_VPSS_SetGrpSharpen(0,&stGrpSharpenAttr);

			/*
			修改白平衡范围，解决低照偏色问题
			*/
			HI_U16 au16CrMax[16] = {304,304,304,304,304,304,304,350,400,650,900,1024,1024,1024,1024,1024};
			HI_U16 au16CbMax[16] = {288,288,288,292,296,300,304,318,400,600,800,800,800,800,800,800};
			
			ISP_WB_ATTR_S stWBAttr;
			HI_MPI_ISP_GetWBAttr(0,&stWBAttr);

			memcpy(stWBAttr.stAuto.stCbCrTrack.au16CrMax,au16CrMax,16*2);
			memcpy(stWBAttr.stAuto.stCbCrTrack.au16CbMax,au16CbMax,16*2);
			
			HI_MPI_ISP_SetWBAttr(0,&stWBAttr);

			ISP_DP_DYNAMIC_ATTR_S stDPDynamicAttr;
			HI_MPI_ISP_GetDPDynamicAttr(0,&stDPDynamicAttr);
			stDPDynamicAttr.bEnable = HI_TRUE;
			//stDPDynamicAttr.bSupTwinkleEn = HI_TRUE;
			//stDPDynamicAttr.s8SoftThr = 22;
			//stDPDynamicAttr.u8SoftSlope = 24;
			HI_MPI_ISP_SetDPDynamicAttr(0,&stDPDynamicAttr);

			HI_U32 as32ISORatio[16] = { 1300, 1300, 1250, 1200, 1150, 1100, 1050, 1000, 1800,  1800,  1400,  800,  800,  800,  800,  800};
			ISP_CA_ATTR_S stCAAttr;
			HI_MPI_ISP_GetCAAttr(0, &stCAAttr);
			memcpy(stCAAttr.as32ISORatio,as32ISORatio,16*4);
			HI_MPI_ISP_SetCAAttr(0,&stCAAttr);

			HI_U32 u32Value = 0;
			HI_MPI_ISP_GetRegister(0,0x300e0, &u32Value);
			u32Value = u32Value & 0xfffeffff;
			HI_MPI_ISP_SetRegister(0,0x300e0, u32Value);

			//HI_MPI_ISP_GetRegister(0,0x300e0, &u32Value);

			//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<    0x300e0 is  0x%x \n",u32Value);
	
	    }
		break;
		
		case SENSOR_OV2735:
		{
	    
			bUseAdc = HI_TRUE;
			//设置降帧结构	
			pframe_ctrl_node = frame_ctrl_ae_list_ov2735;
			//设置切夜视结构
			plight_ctrl_node = light_ctrl_ae_list_ov2735;
			//OV2735只设一级降帧，。。。
			avm5fps_thrd =56;
			
			//设置ISP默认等级
			
			pCOMM_ISP_PARAMS = NULL;
			
			CISP_lEVEL.u8Definition = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeNoise = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8DeSmear = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8Fluency = ISP_SET_LEVEL_DEFAULT;
			CISP_lEVEL.u8framelevel = 3;

	    	MAX_ISO_INDEX = sizeof(OV2735_NRS)/sizeof(OV2735_NRS[0]) - 1; 
			pNrsParam_V300 = OV2735_NRS;

			/*Gamma_Ratio.Night_Ratio.Max_Ratio = 800;
			Gamma_Ratio.Night_Ratio.Mid_Ratio = 300;
			Gamma_Ratio.Night_Ratio.Min_Ratio = 50;
			
			Gamma_Ratio.Day_Ratio.Max_Ratio = 800;
			Gamma_Ratio.Day_Ratio.Mid_Ratio = 350;
			Gamma_Ratio.Day_Ratio.Min_Ratio = 50;*/
	
			VI_DCI_PARAM_S stDciParam;
			HI_MPI_VI_GetDCIParam(0,&stDciParam);
			stDciParam.bEnable = HI_FALSE;
			HI_MPI_VI_SetDCIParam(0,&stDciParam);
			
			ISP_AWB_ATTR_EX_S pstAWBAttrEx;
			HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
			pstAWBAttrEx.stInOrOut.u32OutThresh=10000;
			HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);
			
			//修改白平衡范围，解决低照偏色问题
			HI_U16 au16CrMax[16] = {304,304,304,340,405,500,620,620,  800,1000,1000,1024,1024,1024,1024,1024};
			HI_U16 au16CbMax[16] = {288,288,288,308,370,480,600,600,  700,800,800,800,800,800,800,800};
			
			ISP_WB_ATTR_S stWBAttr;
			HI_MPI_ISP_GetWBAttr(0,&stWBAttr);
			memcpy(stWBAttr.stAuto.stCbCrTrack.au16CrMax,au16CrMax,16*2);
			memcpy(stWBAttr.stAuto.stCbCrTrack.au16CbMax,au16CbMax,16*2);
			HI_MPI_ISP_SetWBAttr(0,&stWBAttr);
			if(bCheckNightDayUsingAE || bRedWhiteCtrl)
				jv_sensor_smart_init(SENSOR_OV2735);
	    }
	   	break;


		default:
		printf("%s..%d..has not match valid sensor \n",__func__,__LINE__);	
		break;
	}
}

static HI_U32 tmp_iso = 0;
static BOOL bDayWdrEnable =FALSE;
static HI_S32 OV2735_DRC_STR[MAX_ISO_TBL_INDEX+1]={60,50,45,40,30,20,20,10,  10,10,6,6,6,6,6,6};

int jv_sensor_wdr_adapt(HI_U32 iso)
{
	int IspDev = 0;
	static BOOL bEnWdr = FALSE;
	HI_U32 u32ISOTmp;
	static BOOL bNightInit=FALSE;
	static BOOL bDayInit=FALSE;
	ISP_DRC_ATTR_S pstDRCAttr;
	HI_U8 u8Index =0;


	if(hwinfo.sensor==SENSOR_OV2735)
	{
		if(bNightMode)
		{
			if(bNightInit == FALSE)
			{
				bNightInit =TRUE;
				tmp_iso =0;
			}
			bDayInit=FALSE;
			
			if(iso != tmp_iso)	
			{
				u32ISOTmp = iso / 100;
				for(u8Index = 0; u8Index < MAX_ISO_TBL_INDEX;u8Index++)        
				{            
					if(1 == u32ISOTmp)            
					{                
						break;            
					}            
					u32ISOTmp >>= 1;     
				}
				
				ISP_DRC_ATTR_S pstDRCAttr;
				HI_MPI_ISP_GetDRCAttr(0, &pstDRCAttr);
				pstDRCAttr.bEnable = HI_TRUE;
				pstDRCAttr.enOpType = OP_TYPE_MANUAL;
				pstDRCAttr.stManual.u8Strength =jv_IQ_AgcTableCalculate(OV2735_DRC_STR, u8Index, iso); 
				pstDRCAttr.u8SpatialVar=6;
				pstDRCAttr.u8RangeVar=8;
				pstDRCAttr.u8Asymmetry=2;
				pstDRCAttr.u8SecondPole=150;
				pstDRCAttr.u8Stretch=54;
				pstDRCAttr.u8Compress=180;
				pstDRCAttr.u8LocalMixingBrigtht = 35;
				pstDRCAttr.u8LocalMixingDark =70;
				HI_MPI_ISP_SetDRCAttr(0, &pstDRCAttr);
			}
		}
		else //白天
		{
			if(bDayInit ==FALSE)
			{
				bDayInit =TRUE;
				tmp_iso =0;
			}
			bNightInit=FALSE;
			if((JV_ISP_COMM_Query_UserWDR()!= bDayWdrEnable)||(iso != tmp_iso) )
			{
				bDayWdrEnable = JV_ISP_COMM_Query_UserWDR();
				
				ISP_DRC_ATTR_S pstDRCAttr;
				HI_MPI_ISP_GetDRCAttr(0, &pstDRCAttr);
				if(bDayWdrEnable)
				{
					pstDRCAttr.bEnable = HI_TRUE;
					pstDRCAttr.stManual.u8Strength =68;
					if(iso>350)
					{
						pstDRCAttr.bEnable = HI_FALSE;
					}
					
				}
				else
				{
					pstDRCAttr.bEnable = HI_TRUE;
					pstDRCAttr.stManual.u8Strength =16;
					if(iso>300)
					{
						pstDRCAttr.bEnable = HI_FALSE;
					}	

				}
				
				pstDRCAttr.enOpType = OP_TYPE_MANUAL;
				pstDRCAttr.u8SpatialVar=6;
				pstDRCAttr.u8RangeVar=8;
				pstDRCAttr.u8Asymmetry=2;
				pstDRCAttr.u8SecondPole=150;
				pstDRCAttr.u8Stretch=54;
				pstDRCAttr.u8Compress=180;
				pstDRCAttr.u8LocalMixingBrigtht = 35;
				pstDRCAttr.u8LocalMixingDark =70;
				HI_MPI_ISP_SetDRCAttr(0, &pstDRCAttr);

			}	
		}
		
		tmp_iso = iso;	
		return 0;
	}
	
	return 0;
}

static HI_U8 DAY_skinGain[16]={64,64,80,127,127,135,145,155,155,155,155,155,155,155,155,155};
static HI_U8 NIGHT_skinGain[16]={127,127,127,127,127,135,145,155,155,155,155,155,155,155,155,155};
static HI_U8 DAY_skinGain_OV2735[16]={80,100,150,180,190,200,200,220, 255,255,255,255,255,255,255,255};
static HI_U8 NIGHT_skinGain_OV2735[16]={255,255,255,255,255,255,255,255, 255,255,255,255,255,255,255,255};


static void *thread_isp_helper(void *param)
{  
    Printf("isp adjust thread\n");
    ISP_DEV IspDev = 0;
    HI_U32 u32ISO = 100;
	HI_U32 u32ISOLast=0;
    unsigned long sensor;
	sleep(2); //等待ISP稳定
	ISP_EXP_INFO_S stExpInfo = {};
    isp_ioctl(0, GET_ID,(unsigned long)&sensor);
    printf("get sensor ID :%ld\n",sensor);
	printf("The Image Version of this IPcamera is %s\n",IMAGE_VERSION);
  	jv_sensor_sensorbase_set(sensor);
	unsigned int jv_exp = 0;
	unsigned char jvAvm = 0;
	HI_U32 expTime = 0;
	BOOL bNightInit =FALSE;
	BOOL bDayInit =FALSE;
	unsigned int jv_exp_old = 0;

    while (isp_helper_live)
    {
		utl_WaitTimeout(!isp_helper_live, 1000);
		HI_MPI_ISP_QueryExposureInfo(IspDev, &stExpInfo);

		

		u32ISO  = ((HI_U64)stExpInfo.u32AGain * stExpInfo.u32DGain* stExpInfo.u32ISPDGain * 100) >> (10 * 3);
		jv_exp = stExpInfo.u32Exposure;
		jvAvm=stExpInfo.u8AveLum;
		expTime = stExpInfo.u32ExpTime;

		if(hwinfo.sensor == SENSOR_MN34227)
		{

			if(bNightMode)
			{
				if(bNightInit ==FALSE)
				{
					//printf("nightset skin now.	NIGHT_skinGain\n");
					bNightInit =TRUE;
					ISP_SHARPEN_ATTR_S pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(0,&pstSharpenAttr);
					memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&NIGHT_skinGain,16);
					HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
					
				}
				bDayInit =FALSE;
			}
			else
			{
				if(bDayInit==FALSE)
				{
					bDayInit=TRUE;
					jv_exp_old = 0;
					ISP_SHARPEN_ATTR_S pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(0,&pstSharpenAttr);
					memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&DAY_skinGain,16);
					HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
				}
				bNightInit =FALSE;

				if(jv_exp != jv_exp_old)
				{
					
					ISP_SHARPEN_ATTR_S pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(0,&pstSharpenAttr);
					if(jv_exp <=120*64 )
					{
						//printf("dayset skin now.	NIGHT_skinGain\n");
						memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&NIGHT_skinGain,16);
						HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
					}
					else if(jv_exp >270*64 )
					{
						//printf("dayset skin now.	DAY_skinGain\n");
						memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&DAY_skinGain,16);
						HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
					}
					
	
				}
			}

		}
		
		if(hwinfo.sensor == SENSOR_OV2735)
		{

			if(bNightMode)
			{
				if(bNightInit ==FALSE)
				{
					//printf("nightset skin now.	NIGHT_skinGain\n");
					bNightInit =TRUE;
					ISP_SHARPEN_ATTR_S pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(0,&pstSharpenAttr);
					memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&NIGHT_skinGain_OV2735,16);
					HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
					
				}
				bDayInit =FALSE;
			}
			else
			{
				if(bDayInit==FALSE)
				{
					bDayInit=TRUE;
					jv_exp_old = 0;
					ISP_SHARPEN_ATTR_S pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(0,&pstSharpenAttr);
					memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&DAY_skinGain_OV2735,16);
					HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
				}
				bNightInit =FALSE;

				if(jv_exp != jv_exp_old)
				{
					
					ISP_SHARPEN_ATTR_S pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(0,&pstSharpenAttr);
					if(jv_exp <=150*64 )
					{
						//printf("dayset skin now.	NIGHT_skinGain\n");
						memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&NIGHT_skinGain_OV2735,16);
						HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
					}
					else if(jv_exp >300*64 )
					{
						//printf("dayset skin now.	DAY_skinGain\n");
						memcpy(&pstSharpenAttr.stAuto.au8SkinGain,&DAY_skinGain_OV2735,16);
						HI_MPI_ISP_SetSharpenAttr(0,&pstSharpenAttr);
					}
					
	
				}
			}

		}

		jv_sensor_Fps_Self_Adapt(jv_exp,u32ISO,expTime,jvAvm);

		jv_sensor_pararm_adapt_as_luma(sensor,u32ISO);
		
		if( jv_exp_old == jv_exp  && !bCal3DNoise)
			continue;

		if(bCal3DNoise)
			bCal3DNoise = HI_FALSE;
		
		VPSS_GRP_NRS_PARAM_S	stNRSParam;
		stNRSParam.enNRVer = VPSS_NR_V2;
		HI_MPI_VPSS_GetGrpNRSParam(0, &stNRSParam);
		jv_sensor_3DnrParam_Cal(&stNRSParam,u32ISO);
		if(hwinfo.sensor == SENSOR_OV2735 && jv_exp <=150*64)
		{
			stNRSParam.stNRSParam_V2.SBS1 = 80;
			stNRSParam.stNRSParam_V2.SDS1 = 128;
		}
		HI_MPI_VPSS_SetGrpNRSParam(0, &stNRSParam);
		
		//u32ISOLast=u32ISO;
		jv_exp_old=jv_exp;
    }
    return NULL;

}

static void isp_helper_deinit(void)
{
	isp_helper_live=0;
	pthread_join(thread,NULL);
	return;
}

static void isp_helper_init(void)
{
    isp_helper_live=1;
	pthread_create(&thread, NULL, thread_isp_helper, NULL);
}
void isp_helper_reload(void)
{
    isp_helper_live=1;
    sleep(1);
    isp_helper_init();
}

int jv_msensor_set_getadc_callback(jv_msensor_callback_t callback)
{
	return 0;
}
