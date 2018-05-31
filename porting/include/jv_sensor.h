
/**
 *@file jv_sensor.h file about sensor
 * define the interface of sensor.
 *@author vito
 */
#ifndef _JV_SENSOR_H_
#define _JV_SENSOR_H_
#include "jv_common.h"
#define WEIGHT_ZONE_ROW 15
#define WEIGHT_ZONE_COLUMN 17
typedef enum{
	SENCE_INDOOR,
	SENCE_OUTDOOR,
	SENCE_DEFAULT,
	SENCE_MODE1,
	SENCE_MAX
}Sence_e;

typedef enum{
	IMAGE_TYPE_STD,  //标准
	IMAGE_TYPE_HIGH_CONTRAST,//通透风格
	IMAGE_TYPE_LOW_CONTRAST, //低对比度，柔和风格
	IMAGE_TYPE_HIGH_SHARPNESS,//锐利风格
	IMAGE_TYPE_MAX,
}Image_Type;



typedef enum{
	DAYNIGHT_AUTO,  		//自动
	DAYNIGHT_ALWAYS_DAY,	//一直白天
	DAYNIGHT_ALWAYS_NIGHT,	//一直夜间
	DAYNIGHT_TIMER,		//定时白天
}DaynightMode_e;	//日夜模式


typedef struct {
	int light;
	int voltage;
}light_v;

typedef struct {
	int adcRValue;
	int adcLValue;
	int adcHValue;
}adc_value_v;



typedef struct {
	unsigned int  ExpThrd;
	unsigned int  LumThrd;
	BOOL  bLow;
	//unsigned int lowFramerate;  
}light_ae_node;

typedef struct {
	unsigned int  ExpThrd;
	unsigned int  LumThrd;
	BOOL  bLow_12;	//降到12帧使能
	BOOL  blow_5; //降到5帧使能
}light_ae_node_n;


typedef struct {
	BOOL   bLow[2];
	unsigned int LowRatio[2];
	unsigned int ExpLtoDThrd[2];//从亮到暗的升帧阀值
	unsigned int ExpDtoLThrd[2];//从暗到亮的降帧阀值
	unsigned int ExpLine[3]; //各个状态下的最大曝光行数
	
}frame_ctrl_node_n;

typedef struct {
	unsigned int  LumThrd;//只用lum来控制切换时间
	unsigned int  ISOThrd;
}light_ctrl_node_n;



typedef enum{
	D_LOW,
	D_NORMAL,
	D_HIGH,
	D_MAX,
}Image_D;

typedef enum
{
	FRM_RATIO_FULL,
	FRM_RATIO_HALF,
	FRM_RATIO_10,
	FRM_RATIO_8,
}FRM_RATIO;

typedef enum
{
    JV_EXP_AUTO_MODE,     //自动曝光
    JV_EXP_HIGH_LIGHT_MODE, //强光抑制
    JV_EXP_LOW_LIGHT_MODE, //背光补偿
    JV_EXP_MAX_MODE
}JV_EXP_MODE_e;

/*
 * 设置曝光模式
 * param mode 曝光模式
 * return 0 成功；<0失败
 */
int jv_sensor_set_exp_mode(int sensorid,JV_EXP_MODE_e mode);

typedef enum
{
    JV_ANTIFLICK_CLOSE,	//关闭
    JV_ANTIFLICK_50Hz,	//50Hz
    JV_ANTIFLICK_60Hz,	//60Hz
    JV_ANTIFLICK_MAX
}JV_ANTIFLICK_e;

//设置工频干扰全局变量
int jv_sensor_set_antiflick(int sensorid,JV_ANTIFLICK_e mode);

typedef struct _JV_LOCAL_EXPOSURE
{
	U8 u8Weight[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN];
}JV_SENSOR_LE_t;
typedef void (*restore_sensor)(int id);
/**
 *@brief open sensor device
 *@param no
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int sensor_ircut_init(void);

/**
 *@brief close sensor device
 *@param no
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int sensor_ircut_deinit(void);

/**
 *@brief 设置为夜间模式。可以做一些除了黑白，CUT之外的特色的工作
 */
int jv_sensor_set_nightmode(BOOL bNight);

/**
 *@brief 暂时替代jv_sensor_b_nightmode。代表了灯板上的夜间模式
 */
BOOL jv_sensor_b_night(void);
/**
 *@brief AE数字感应代替光敏电阻器light sensitive resistor
 *@brief 和jv_sensor_b_night（）功能一样，判断是否夜模式
 *@retval 0： day 1:night
 */
BOOL jv_sensor_b_night_use_ae(void);


/**
 *@brief set brightness
 *@param brightness value 0~255
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_brightness(int sensorid, int nValue);

/**
 *@brief set antifog
 *@param antifog value 0~255
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_antifog(int sensorid, int nValue);

/**
 *@brief set saturation
 *@param saturation value 0~255
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
 int jv_sensor_light(int sensorid, int nValue);
/**
 *@brief set saturation
 *@param saturation value 0~255
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */

int jv_sensor_saturation(int sensorid, int nValue);

/**
 *@brief set contrast
 *@param contrast value 0~255
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_contrast(int sensorid, int nValue);

/**
 *@brief 设置指定sensor的锐度
 *
 *@param sensorid sensor号，目前摄像机为单sensor，传入0即可
 *@param sharpness 锐度值
 *
 */
int jv_sensor_sharpness(int sensorid, int sharpness);

int jv_sensor_hue(int sensorid, int value);

/**
 *@brief set chroma
 *@param chroma value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_chroma(int sensorid, int nValue);

/**
 *@brief set wdr
 *@param bEnable Enable or not wide dynamic range
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_wdr(int sensorid, int bEnable);

/**
 *@brief set mirror
 *@param sensorid
 *@param mirror value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_mirror_turn(int sensorid, BOOL bMirror, BOOL bTurn);

/**
 *@brief set awb
 *@param sensorid
 *@param awb value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_awb(int sensorid, int nValue);
/**
 *@brief set nocolour
 *@param sensorid
 *@param nocolour value
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_nocolour(int sensorid, BOOL bNoColor);

/**
 *@brief Get Reference Value for Focus
 *@param sensorid
 *@return Reference Value for Focus
 */
int jv_sensor_get_focus_value(int sensorid);
/**
 *@brief 设置是否降帧， 以提高亮度。请只在夜间使用
 *@param sensorid
 *@return Reference Value for Focus
 */
int jv_sensor_low_frame(int sensorid, int bEnable);

/**
 *@brief 获取降帧状态
 *
 *@param sensorid id
 *@param current OUT 降帧后的当前帧率
 *@param normal OUT 正常帧率
 *
 *@return 1 表示降帧  0 表示未降帧
 */
BOOL jv_sensor_get_b_low_frame(int sensorid, float *current, float *normal);

/**
 *@brief set sence
 *@param sensorid
 *@param sence sence for use
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_sensor_set_sence(int sensorid, Sence_e sence);


//是否开启自动降帧功能
int jv_sensor_auto_lowframe_enable(int sensorid, int bEnable);



/**
 *@brief set definition
 *@param sensorid
 *@param level for use
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */

int jv_sensor_set_definition(int sensorid, Image_D level);

struct offset_s {
		int RG_Offset;
		int BG_Offset;
		int Exp;
};

typedef struct  {
	
	///////////////////////////姝や袱鑰呭钩鍙扮浉鍏筹紝涓虹櫧骞宠　鍒嗗尯鐨勮鍒楁暟
	unsigned char Max_Row;
	unsigned char Max_Col;
	///////////////////////////骞冲彴鐩稿叧,缁熻淇℃伅鐩稿浜?56鐨勭簿搴︼紝
	unsigned char Statistical_Accuracy;
	
	///////////////////////////绾㈠涓嬬殑R/G B/G
	int	Ratio_Rg_Ir;
	int	Ratio_Bg_Ir;
	
	/////////////////////////////涓�鑸嚜鐒跺厜涓婻/G,B/G鐨勫彇鍊艰寖鍥?
	unsigned short Ratio_Rg_Natural_Light_Max;
	unsigned short Ratio_Rg_Natural_Light_Min;
	unsigned short Ratio_Bg_Natural_Light_Max;
	unsigned short Ratio_Bg_Natural_Light_Min;
	
	//////////////////////////////涓�鑸嚜鐒跺厜涓婻/G,B/G鐨勭粡鍏稿�?
	unsigned short Ratio_Rg_Natural_Light_Typcial;	
	unsigned short Ratio_Bg_Natural_Light_Typical;
	
	//////////////////////////////A鍏夋簮涓婻/G,B/G鐨勫彇鍊?
	unsigned short Ratio_Rg_A_Light;	
	unsigned short Ratio_Bg_A_Light;
	
	///////////////////////////////A鍏夋簮涓嬬孩鐧絚ut鐗囩殑閫忚繃鐜囩殑姣斿�?
	unsigned char Cut_Ratio_A_Light;
	//////////////////////////////缁忓吀鐨勮嚜鐒跺厜涓嬬孩鐧界墖閫忚繃鐜囩殑姣斿�?
	unsigned char Cut_Ratio_Natural_Light_Typical;
	//////////////////////////////缁忓吀姣斾緥涓嬬豢鑹插悜浜害鐨勮浆鍖栨瘮渚?
	unsigned char Ratio_G_To_L;
	
	///////////////////////////鍚堢悊璁＄畻鐐圭殑鍙栧�肩┖闂?
	unsigned int R_Limit_Value_Max;
	unsigned int G_Limit_Value_Max;
	unsigned int B_Limit_Value_Max; 
	unsigned int R_Limit_Value_Min; 
	unsigned int G_Limit_Value_Min; 
	unsigned int B_Limit_Value_Min; 
	
	///////////////////////////澶у皬鍦烘櫙涓婻/G,B/G鐨勫亸绉?
	struct offset_s * BR_Gain_Offset; 
	int BR_Gain_Offset_Num;
} Soft_Light_Sensitive_t;


/**
 *@brief reload isp_helper thread
 */
void isp_helper_reload(void);

typedef struct{
	//ISP_AE_MODE_E  enAEMode;
	BOOL bAEME;//是否自动曝光
	BOOL bByPassAE;
	BOOL bLowFrameMode; //降帧方式，暂时无用
	unsigned int exposureMax; //最大曝光时间。曝光时间为 ： 1/exposureMax 秒，取值 3 - 100000
	unsigned int exposureMin;
	unsigned short u16DGainMax;
	unsigned short u16DGainMin;
	unsigned short u16AGainMax;
	unsigned short u16AGainMin;
    unsigned int u32ISPDGainMax;      /*RW,  the ISPDgain's  max value, Range : [0x400, 0xFFFFFFFF]*/
    unsigned int u32SystemGainMax;    /*RW, the maximum gain of the system, Range: [0x400, 0xFFFFFFFF],it's related to specific sensor*/
//	unsigned char u8ExpStep;
//	unsigned short s16ExpTolerance;
//	HI_U8  u8ExpCompensation;
	//ISP_AE_FRAME_END_UPDATE_E enFrameEndUpdateMode；
//	HI_U8 u8Weight[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN];
	/*手动曝光参数*/
	int s32AGain;			/*RW,  sensor analog gain (unit: times), Range: [0x0, 0xFF],it's related to the specific sensor */
	int s32DGain;			/*RW,  sensor digital gain (unit: times), Range: [0x0, 0xFF],it's related to the specific sensor */
	unsigned int u32ISPDGain; 		/*RW,  sensor digital gain (unit: times), Range: [0x0, 0xFF],it's related to the specific isp	*/
	unsigned int u32ExpLine;			/*RW,  sensor exposure time (unit: line ), Range: [0x0, 0xFFFF],it's related to the specific sensor */

	BOOL bManualExpLineEnable;
	BOOL bManualAGainEnable;
	BOOL bManualDGainEnable;
	BOOL bManualISPGainEnable;
}AutoExposure_t;

/**
 *@brief 设置自动曝光的参数
 */
int jv_sensor_ae_set(AutoExposure_t *ae);

/**
 *@brief 获取自动曝光的参数
 */
int jv_sensor_ae_get(AutoExposure_t *ae);

typedef struct _DRC_t
{
    BOOL bDRCEnable;
    //BOOL bDRCManualEnable;        
    //unsigned int  u32StrengthTarget;  /*RW,  Range: [0, 0xFF]. It is not the final strength used by ISP. 
    //                                 * It will be clipped to reasonable value when image is noisy. */
    //unsigned int  u32SlopeMax;        /*RW,  Range: [0, 0xFF]. Not recommended to change. */
    //unsigned int  u32SlopeMin;        /*RW,  Range: [0, 0xFF]. Not recommended to change. */
    //unsigned int  u32WhiteLevel;      /*RW,  Range: [0, 0xFFF]. Not recommended to change. */
    //unsigned int  u32BlackLevel;      /*RW,  Range: [0, 0xFFF]. Not recommended to change. */
    //unsigned int  u32VarianceSpace;     /*RW,  Range: [0, 0xF]. Not recommended to change*/
    //unsigned int  u32VarianceIntensity; /*RW,  Range: [0, 0xF].Not recommended to change*/
} DRC_t;

typedef struct _JV_EXP_CHECK_THRESH_S
{
	unsigned int ToDayExpThresh; //软光敏切换到白天临界值
	unsigned int ToDayAEDiffTresh;//切换到白天AE变化差值
	
	unsigned int  ToNightExpThresh; //软光敏切换晚上临界?
	unsigned char ToNightLumThresh;
	

	unsigned int DayFullFrameExpThresh; //全帧模式进入的阀值
	unsigned int DayHalfFrameExpThresh; //半全帧模式进入的阀值
	unsigned int DayLowFrameExpThresh_0; //全彩模式下降帧的exp以及frm,第一级，比如降到半全帧
	unsigned int DayLowFrameExpThresh_1; //全彩模式下降帧的exp以及frm,第二级，比如降到1/4全帧
	unsigned char DayLowFrameRate_0;
	unsigned int OutDoorExpLineThresh;
	unsigned int InDoorExpLineThresh;
	unsigned int DayLowFrameRatio;       //采用比率形式进行降帧，修复由于降帧导致VI帧频不正确
	unsigned int DayLowFrameRatio_1;       //采用比率形式进行降帧，修复由于降帧导致VI帧频不正确

}JV_EXP_CHECK_THRESH_S;




typedef enum{
	SENSOR_STATE_UNKNOWN,
	SENSOR_STATE_DAY_FULL_FRAMRATE,  		//25帧频彩色
	SENSOR_STATE_DAY_HALF_FRAMRATE,  		//12帧频彩色
	SENSOR_STATE_DAY_QUARTER_FRAMRATE,  	//6帧频彩色
	SENSOR_STATE_NIGHT,		                //25帧黑白图像
	//SENSOR_STATE_NIGHT_FULL_FRAMRATE,        //12帧黑白图像
}JV_SENSOR_STATE;	



typedef enum{
	AE_UNKNOWN_MODE,
	AE_25_FULLCOLOR,  		//25帧频彩色
	AE_12_FULLCOLOR,  		//12帧频彩色
	AE_12_NO_COLOR,        //12帧黑白图像
}AE_COLOR_FRM_MODE;	//日夜模式

typedef int (*jv_msensor_callback_t)(int *param);

int jv_msensor_set_callback(jv_msensor_callback_t callback);
int jv_msensor_set_getadc_callback(jv_msensor_callback_t callback);
/**
 *@brief 设置DRC的参数
 */
int jv_sensor_drc_set(DRC_t *drc);

/**
 *@brief 获取DRC的参数
 */
int jv_sensor_drc_get(DRC_t *drc);

/**
 *@brief change ir cut to night or day
 */
void jv_sensor_set_ircut(BOOL bNight);

/*
 * brief 设置局部曝光
 * sensorid 0
 * jv_sensor_le 局部曝光权值表
 */
int jv_sensor_set_local_exposure(int sensorid,JV_SENSOR_LE_t *jv_sensor_le);
/*
 * brief 获取局部曝光
 * sensorid 0
 * jv_sensor_le 局部曝光权值表
 */
int jv_sensor_get_local_exposure(int sensorid,JV_SENSOR_LE_t *jv_sensor_le);

/*
 * 获取数字防抖开启状态
 * 返回0没开启，1开启
 */
int jv_sensor_get_dis(int sensorid);
/*
 * 设置数字防抖
 * sensorid 0
 * disopen 1:打开，0关闭
 */
int jv_sensor_set_dis(int sensorid,BOOL disopen);


//针对海康延时的问题将sensor帧频将低一点点

int jv_sensor_frame_trip(int framerate);

/**
 *@brief set the iris/aperture  调节光圈大小
 *@param sensorid
 *@param size up or down
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_iris_adjust(int sensorid, BOOL SIZEUP);

/*
 * 获取旋转属性
 * jvRotate 旋转属性指针
 * 返回值  0成功，-1失败
 */
int jv_sensor_rotate_get(JVRotate_e *jvRotate);

/*
 * 设置旋转属性
 * jvRotate  旋转属性
 * 返回值  0成功，-1失败
 */
int jv_sensor_rotate_set(JVRotate_e jvRotate);

/*
 * 告诉底层用户设置是否为一直彩色模式
 * 返回值  0成功，-1失败
 */

int jv_sensor_set_daynight_mode(int sensorid, int mode);


int jv_sensor_base_framerate_init(int rf);

int jv_sensor_set_framerate(int frameRate);

int jv_sensor_set_framerate_v2(int frameRate,BOOL bLow);


int jv_sensor_set_vi_framerate(int frm);

/*
 *@brief  设置VI最新接口，后续会陆续更换使用该接口
 *@parms frameRate -设置的VI帧频
 *@retval返回值  0成功，-1失败
 */
int jv_sensor_set_fps(int frameRate);

/*
 *@brief  设置曝光策略参数
 *
 */
int jv_sensor_set_exp_params(int sensorid,int value);

int jv_sensor_set_denoise(int sensorid,int value);
int jv_sensor_set_defini(int sensorid,int value);
int jv_sensor_set_desmear(int sensorid,int value);
int jv_sensor_set_fluency(int sensorid,int value);
/*
*@brief		获取adc实时值，切cut值，生产用
*
*/
int jv_adc_get_value(adc_value_v* value);

BOOL jv_sensor_get_outindoor();

typedef enum
{
    IRCUT_DAY,
    IRCUT_NIGHT,
}IRCUT_STATUS;

int jv_sensor_get_adc_high_val();
int jv_sensor_get_adc_low_val();
int jv_sensor_get_adc_val();

BOOL jv_sensor_b_daynight_usingAE();

int jv_sensor_set_max_vi_20fps(BOOL Enable);

int jv_get_vi_maxframrate();

/* 设置白光灯是否使能 */
void  jv_sensor_set_whitelight_function(BOOL bEnable);
BOOL  jv_sensor_get_whitelight_function();

/* 白光灯报警闪烁时告知 */
void jv_sensor_set_alarm_light(BOOL bEnable);

/* 获取当前全彩模式下的白光灯状态 */
BOOL jv_sensor_get_whitelight_status();
#endif

