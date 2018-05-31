
/*	mstream.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织视频相关代码
	更改历史详见svn版本库日志
*/

#ifndef __MSENSOR_H__
#define __MSENSOR_H__
#include "jv_sensor.h"

//sensor的数量
#define MAX_SENSOR	1
#define EFFECT_AWB				0
#define EFFECT_MIRROR			0x01
#define EFFECT_TURN				0x02
#define EFFECT_NOCOLOUR			0x03
#define EFFECT_LOW_FRAME        0x04
#define EFFECT_AGC              0x05    //自动增益
#define EFFECT_ABLC             0x06    //自动背光补偿

typedef enum{
	MSENSOR_DAYNIGHT_AUTO,  		//自动
	MSENSOR_DAYNIGHT_ALWAYS_DAY,	//一直白天
	MSENSOR_DAYNIGHT_ALWAYS_NIGHT,	//一直夜间
	MSENSOR_DAYNIGHT_TIMER,		//定时白天
}MSensorDaynightMode_e;	//日夜模式
typedef enum{
	MSENSOR_DAYNIGHT_UART_NULL=0,  	//NULL模式，不支持UART
	MSENSOR_DAYNIGHT_UART_AUTO,  	//自动
	MSENSOR_DAYNIGHT_UART_TODAY,	//TO白天
	MSENSOR_DAYNIGHTUART_TONIGHT,	//TO夜间
}MUARTDaynightMode_e;	//日夜模式

typedef struct
{
	BOOL 			bOpenCutTest;	//开启测试
	unsigned int	nInterval;		//每次间隔时间单位s，MIN:0
	unsigned int	nCountTimes;			//切换次数
}cut_test_t;

typedef struct
{
	unsigned int	brightness;
	unsigned int	saturation;
    unsigned int    contrast;
	unsigned int    sharpness;
	unsigned int 	exposure;
	unsigned int	antifog;    //去雾强度
    unsigned int    sw_cut;
    unsigned int    cut_rate;
	unsigned int    light;     //光照强度
	AutoExposure_t  ae;
	DRC_t drc;
    BOOL			bSupportWdr;		//是否支持宽动态
    BOOL			bEnableWdr;		//是否开启宽动态
//    BOOL			bDigitalLDR;	 //是否AE数字感应代替，光敏电阻器light sensitive resistor
    BOOL			bDISOpen;			//是否开启数字防抖

    MSensorDaynightMode_e 	daynightMode;		//日夜模式
    MUARTDaynightMode_e	  	uartdaynightMode;	//UART的日夜驱动模式
    JV_EXP_MODE_e 			exp_mode;				//曝光模式

    struct{
    	char hour;
    	char minute;
    }dayStart, dayEnd;

    unsigned int    effect_flag;    //sensor效果控制位，bit0:awb	bit1:mirror		bit2:turn		bit3:nocloour   bit4:auto low frame 
                                    //bit5:auto gain control bit6:auto blacklight compensation
    unsigned int    ir_cut;          //ir_cut    的开关状态，当是夜间模式时，禁止转换为彩色模式，运行中参数无需写入配置文件
    Sence_e sence;				//应用场景：室内、室外等

	unsigned int	cutDelay;//CUT切换时间延迟，单位为秒。0则不延迟
	JVRotate_e rotate; //旋转，用于实现走廊模式
	Image_D    imageD; //图像清晰度级别，级别越大，越清楚，但噪点越多
	BOOL       AutoLowFrameEn;  //是否夜视自动降帧
	BOOL bSupportSl;  //是否支持星光级
	BOOL bEnableSl;  //是否开启星光级

	cut_test_t stCutTest;
}msensor_attr_t;

/**
 *@brief 获取当前的Rotate状态。
 *@note 这个状态，是刚开机时的状态。中途即使通过#msensor_setparam 修改，本函数获取的仍然是原来的值，只有重启才生效
 */
JVRotate_e msensor_get_rotate();

/*******************局部曝光****************************/
#define RECT_MAX_CNT	4		//允许画的矩形最多个数
typedef struct _LOCAL_EXPOSURE
{
	BOOL	bLE_Enable;			//是否开启局部曝光
	BOOL	bLE_Reverse;		//区域反选
	U32		nLE_Weight;			//曝光权值
	U32		nLE_RectNum;			//曝光区域个数，最大为10
	RECT	stLE_Rect[RECT_MAX_CNT];
}MLE_t;
/*******************局部曝光****************************/

/**
 *@brief 通过#rotate 计算旋转后的位置
 */
int msensor_rotate_calc(JVRotate_e rotate, int viw, int vih, RECT *rect);

/**
 *@brief 设置sensor模块的参数
 *
 *@param pstAttr 传入的参数
 *
 */
void msensor_setparam(msensor_attr_t *pstAttr);

/**
 *@brief 获取sensor模块的参数
 *
 *@param pstAttr 传入的参数
 *
 */
void msensor_getparam(msensor_attr_t *pstAttr);
/**
 *@brief 获取sensor模块的参数
 *
 *@param pstAttr 传入的参数
 *
 */
void msensor_init(int sensorid);

/**
 *@brief 刷新sensor使用已保存的参数
 *
 *@param sensorid sensor号，目前摄像机为单sensor，传入0即可
 *
 */
void msensor_flush(int sensorid);


/**
 * 获取对焦的焦点值
 */
int msensor_get_focus_value(int sensorid);

/**
 * @brief 设置局部曝光参数
 *
 * @param
 */
int msensor_set_local_exposure(int sensorid,MLE_t *mle);
/*
 * @brief 获取局部曝光参数
 */
int msensor_get_local_exposure(int sensorid,MLE_t *mle);
/**
 * 设置自动曝光默认值
 */
int msensor_exposure_set_default(int sensorid);

/**
 * 获取数字防抖状态
 * 返回值：返回数字防抖开启关闭状态，1为开启0为关闭
 */
int msensor_dis_get(int sensorid);
/*
 *设置数字防抖状态
 *bDISOpen 数字防抖开启状态0为关闭，1为开启
 *返回值：0成功-1失败
 */
int msensor_dis_set(int snesorid,BOOL bDISOpen);

/*获取sensor夜晚状态*/
BOOL msensor_mode_get();

int msensor_callback(int *param);

/* 设置白光灯是否使能 */
void  msensor_set_whitelight_function(BOOL bEnable);
BOOL  msensor_get_whitelight_function();

/* 白光灯报警闪烁时告知 */
void msensor_set_alarm_light(BOOL bEnable);

/* 获取当前全彩模式下的白光灯状态 */
BOOL msensor_get_whitelight_status();
#endif

