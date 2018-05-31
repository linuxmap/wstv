/*
 * mivp.h
 *
 *  Created on: 2014-11-7
 *      Author: Administrator
 */

#ifndef MIVP_H_
#define MIVP_H_

#include "ivp.h"
#include "types_c.h"
#ifdef IVP_SUPPORT					//智能分析,开关量在set.sh中默认关闭

#ifdef PLATFORM_hi3516D
#define IVP_RL_SUPPORT 		1		//区域入侵和绊线检测 RL=Region Line
#define IVP_HIDE_SUPPORT 	1		//遮挡报警
//#define IVP_TL_SUPPORT 1		//拿取遗留报警：TL=Take Left
//#define IVP_CDE_SUPPORT					1	//人群密度
//#define IVP_OCL_SUPPORT                   1   //超员检测
//#define IVP_HOVER_SUPPORT				1	//徘徊检测
//#define IVP_FM_SUPPORT			1		//快速移动
//#define IVP_FIRE_SUPPORT			1	//烟火检测
//#define IVP_HM_SUPPORT			1		//热度图
//#define IVP_LPR_SUPPORT			1		//车牌识别
//#define IVP_COUNT_SUPPORT 	1		//客流量统计
#define IVP_SC_SUPPORT		1			//场景变更
#define IVP_VF_SUPPORT			1		//虚焦检测
#endif

#if (defined PLATFORM_hi3518EV200) ||  (defined PLATFORM_hi3516EV100)
//#define IVP_HIDE_SUPPORT 	1		//遮挡报警
//#define IVP_RL_SUPPORT 		1		//区域入侵和绊线检测 RL=Region Line
//#define IVP_HIDE_SUPPORT 	0		//遮挡报警
//#define IVP_VF_SUPPORT			1		//虚焦检测
#endif

//#define IVP_RL_SUPPORT 		1		//区域入侵和绊线检测
//#define IVP_COUNT_SUPPORT 	1		//客流量统计，普通软件这里注释掉，支持的软件要打开
//#define	IVP_HIDE_SUPPORT		1		//遮挡报警
#endif
extern int bIVP_COUNT_SUPPORT;		//客流量统计，开关量在hwconfig.cfg中控制，默认关闭 注：次变量已经不再使用，默认为1，使用下面的宏定义进行控制


#define MAX_POINT_NUM 10
typedef struct __MIVPREGION
{
	int						nCnt;						//点的个数>=2,=2时是绊线
	IVP_POINT				stPoints[MAX_POINT_NUM];	//图形的点的坐标
	U32			 			nIvpCheckMode;				//绊线的方向检测
	U32 						eAlarmType;				//规则是否被触发
	int 						staytimeout;					//报警超时时间
	int 						flickertimeout;				//OSD闪烁超时时间
	BOOL					bAlarmed;
}MIVPREGION_t;

typedef enum{
	IVP_RL = 1,
	IVP_CDE,
	IVP_FM,
	IVP_HOVER,
}IVP_type_e;



typedef enum{
	MCOUNTOSD_POS_LEFT_TOP=0,
	MCOUNTOSD_POS_LEFT_BOTTOM,
	MCOUNTOSD_POS_RIGHT_TOP,
	MCOUNTOSD_POS_RIGHT_BOTTOM,
	MCOUNTOSD_POS_HIDE,
}mivpcountosd_pos_e;

#define MAX_IVP_REGION_NUM 4

#define	MAX_SEGMENT_CNT	80*10		//80行*10段
typedef struct _VR_REGION
{
	int 					cnt;
	ivpINTERVAL				stSegment[MAX_SEGMENT_CNT];
}IVPVRREGION_t;

typedef struct	__ALARM_OUT
{
	BOOL 			bStarting;					//是否正在报警,由于客户端报警会一直持续，用来检测是否在向客户端发送报警

	U32 			nDelay;						//报警延时，小于此时间的多次报警只发送一次邮件，客户端不受此限制

	BOOL 		bEnableRecord;				//是否开启报警录像
	BOOL 		bOutAlarm1;					//是否输出到 1路报警输出
	U32 			bOutClient;					//是否输出到客户端报警
	U32 			bOutEMail;					//是否发送邮件报警
	BOOL 		bOutVMS;					//是否发送至VMS服务器
	BOOL		bOutSound;					//家用机声音输出
}Alarm_Out_t;
typedef struct 
{
	//攀爬

	int					nPoints;							//点的个数==2,是爬高
	IVP_POINT			stPoints[2];						//图形的点的坐标
	U32 					eAlarmType;				//规则是否被触发
	int 					staytimeout;					//报警超时时间
	int 					flickertimeout;				//OSD闪烁超时时间
	BOOL				bAlarmed;
}MIVP_CL_t, *PMIVP_CL_t;


typedef struct _RL
{
	//绊线检测和区域入侵
	BOOL 			bEnable;							//是否开启智能视频分析
	
	BOOL			bHKEnable;							//海康nvr对接专用
	BOOL			bCLEnable;							//海康nvr对接专用，越界侦测 cross line	使用reg0
	BOOL			bRInvEnable;							//海康nvr对接专用，区域入侵侦测 region invasion 使用reg1
	BOOL			bRInEnable;							//海康nvr对接专用，进入区域侦测	region in 使用reg2
	BOOL			bROutEnable;							//海康nvr对接专用，离开区域侦测 region out 使用reg3
	
	U32 			nRgnCnt;							//报警区域个数
	MIVPREGION_t 	stRegion[MAX_IVP_REGION_NUM];		//报警区域

	MIVP_CL_t		stClimb;
	//绊线和区域入侵的高级设置
	BOOL 			bDrawFrame;							//画出拌线或防区
	BOOL 			bFlushFrame;						//报警产生时拌线或防区边线闪烁
	BOOL 			bMarkObject;						//标记报警物体--v2
	BOOL 			bMarkAll;							//标记全部运动物体

	U32 			nAlpha;								//防区透明度  库里取消不用了，采用像素格式了 20151105
	U32 			nSen;								//灵敏度
	U32 			nThreshold;							//阀值
	U32 			nStayTime;							//停留时间

	Alarm_Out_t		stAlarmOutRL;						//报警输出
}MIVP_RL_t, *PMIVP_RL_t;


typedef struct _COUNT
{
	//人数统计
	BOOL				bOpenCount;						//开启人数统计功能--v2
	BOOL				bShowCount;						//显示人数统计--v2
	mivpcountosd_pos_e	eCountOSDPos;					//位置
	U32					nCountOSDColor;					//字体颜色
	U32					nCountSaveDays;					//保存天数
	U32					nTimeIntervalReport;				//上报迅卫士平台间隔时间
	int					nPoints;							//点的个数>=2,=2时是绊线
	IVP_POINT			stPoints[2];						//图形的点的坐标
	U32 					nCheckMode;					//检测模式A2B B2A
	//图线输出参数
	BOOL 			bDrawFrame;							//画出拌线或防区
	BOOL 			bFlushFrame;						//报警产生时拌线或防区边线闪烁
	BOOL 			bMarkObject;						//标记报警物体--v2
	BOOL 			bMarkAll;							//标记全部运动物体

	BOOL			nLinePosY;
}MIVP_Count_t, *PMIVP_Count_t;

typedef struct _VR
{
	BOOL 			bVREnable;						//是否开启占有率报警
	int				nVRThreshold;					//占有率报警阈值
	int 			nSen;							//灵敏度%
	int 			nCircleTime;					//检测周期，单位s，默认3600s//暂未提供给用户
	int 			nVariationRate;					//占有率
	BOOL			bFinished;
	IVPVRREGION_t	stVrRegion;						//占有率检测范围框
}MIVP_VR_t, *PMIVP_VR_t;

typedef struct _DETECT
{
	BOOL			bDetectEn;					//是否开启移动目标侦测
}MIVP_DETECT_t,*PMIVP_DETECT_t;

typedef struct _HIDE
{
	BOOL			bHideEnable;					//是否开启遮挡报警
	int 			nThreshold;
	Alarm_Out_t		stHideAlarmOut;					//报警输出控制
}MIVP_HIDE_t,*PMIVP_HIDE_t;

typedef struct _TakeLeft
{
	BOOL			bTLEnable;						//是否开启物品遗留拿取报警

	BOOL			bHKEnable;						//是否是海康nvr设置
	BOOL			bLEnable;						//haik 是否开启物品遗留报警		left
	BOOL			bTEnable;						//haik 是否开启物品拿取报警		take

	int				nTLMode;						//检测模式	0Left 1Take
	int				nTLAlarmDuration;				//报警持续时间
	int				nTLSuspectTime;					//可疑判定时间
	int				nTLSen;							//灵敏度

	int				nTLRgnCnt;
	MIVPREGION_t 	stTLRegion[MAX_IVP_REGION_NUM];		//报警区域

	Alarm_Out_t		stTLAlarmOut;					//报警输出控制
}MIVP_TL_t,*PMIVP_TL_t;

typedef struct __MIVPRECT
{
	int						nCnt;						//标定框点数
	IVP_POINT				stPoints[4];	//标定框左上右下两点坐标
}MIVPRECT_t;


typedef struct _CROWD_DENSITY
{
	BOOL			bEnable;				//是否开启人员密度检测
	int				nCDERate;				//人员密度值
	U32 			nRgnCnt;						//报警区域个数
	BOOL 			bDrawFrame;						//画出防区
	MIVPRECT_t 		stRegion[MAX_IVP_REGION_NUM];	//报警区域支持
	int				nCDEThreshold;				//人员聚集报警阈值
	Alarm_Out_t		stCDEAlarmOut;				//报警输出控制
}MIVP_CDE_t,*PMIVP_CDE_t;

typedef struct _CRL
{
	BOOL			bEnable;						//是否开启超员检测
	int				nRgnCnt;	
    int             nExist;                      //区域中现存人员
    int             nLimit;                      //区域中最大人员
	BOOL 			bDrawFrame;						//画出防区
	MIVPRECT_t 	    stRegion;					//报警区域
	Alarm_Out_t	    stOCLAlarmOut;					//报警输出控制
}MIVP_OCL_t,*PMIVP_OCL_t;

typedef struct _FM	//快速移动结构体
{
	BOOL 			bEnable;							//是否开启快速移动检测
	U32 			nRgnCnt;							//报警区域个数
	MIVPREGION_t 	stRegion[MAX_IVP_REGION_NUM];		//报警区域

	//高级设置
	BOOL 			bDrawFrame;							//画出拌线或防区
	BOOL 			bFlushFrame;						//报警产生时拌线或防区边线闪烁
	BOOL 			bMarkObject;						//标记报警物体--v2
	BOOL 			bMarkAll;							//标记全部运动物体

	U32 			nAlpha;								//防区透明度  库里取消不用了，采用像素格式了 20151105
	U32 			nSen;								//灵敏度
	U32 			nThreshold;							//阀值
	U32 			nStayTime;							//停留时间
	U32				nSpeedLevel;						//速度等级

	Alarm_Out_t		stAlarmOutRL;						//报警输出
}MIVP_FM_t, *PMIVP_FM_t;

typedef struct _HOVER		//徘徊结构体
{
	BOOL			bEnable;					//是否开启徘徊报警
	Alarm_Out_t		stAlarmOut;					//报警输出控制
}MIVP_HOVER_t,*PMIVP_HOVER_t;

typedef struct _FIRE		//烟火报警结构体
{
	BOOL			bEnable;					//是否开启烟火报警
	int				sensitivity;					//灵敏度
	Alarm_Out_t		stAlarmOut;					//报警输出控制
}MIVP_FIRE_t,*PMIVP_FIRE_t;

typedef struct _VF		//虚焦检测结构体
{
	BOOL			bEnable;					//是否开启虚焦检测
	int 			nThreshold;
	Alarm_Out_t		stAlarmOut;					//报警输出控制
}MIVP_VF_t,*PMIVP_VF_t;

typedef struct _SC		//场景变更结构体
{
	BOOL			bEnable;					//是否开启场景变更
	int				nThreshold;					//配置视场偏移比例阀值,[0-100]后面处理成需要的值
	int 			duration;					//报警持续时间
	Alarm_Out_t		stAlarmOut;					//报警输出控制
}MIVP_SC_t,*PMIVP_SC_t;


typedef struct _HM		//热度图结构体
{
	BOOL			bEnable;					//是否开启热度图
	int 			upCycle;					//更新周期，回调
	MMLImage		image;
}MIVP_HM_t,*PMIVP_HM_t;

typedef struct _LPR		//车牌识别结构体
{
	BOOL			bEnable;					//是否开启车牌识别
	char 			lp_num[8];					//车牌号
	IVP_LPR_PARAM_WORK_MODE work_mode;			//工作模式
	U32 			nRgnCnt;							
	MIVPRECT_t		stRegion;						//区域,只用第一个点
	IVP_LPR_PARAM_DIRECTION def_dir;			//摄像机在左侧还是右侧
	IVP_LPR_PARAM_DISPLAY_TYPE display;			//是否显示检测区域
}MIVP_LPR_t,*PMIVP_LPR_t;


typedef struct __MIVP
{
	//内部使用变量
	BOOL				bNeedRestart;				//是否需要重启

	//区域入侵和绊线检测
	MIVP_RL_t			st_rl_attr;
	//过线计数,基于拌线
	//人数统计
	MIVP_Count_t		st_count_attr;

	//占有率报警
	MIVP_VR_t			st_vr_attr;

	//遮挡报警
	MIVP_HIDE_t			st_hide_attr;				//遮挡报警

	MIVP_TL_t			st_tl_attr;					//拿取遗留报警

	MIVP_CDE_t			st_cde_attr;				//人员密度估计

    MIVP_OCL_t          st_ocl_attr;                //拥挤检测

	MIVP_FM_t			st_fm_attr;					//快速移动侦测

	MIVP_HOVER_t		st_hover_attr;				//徘徊报警

	MIVP_FIRE_t			st_fire_attr;				//烟火报警报警

	MIVP_VF_t			st_vf_attr;					//虚焦检测

	MIVP_SC_t			st_sc_attr;					//场景变更

	MIVP_HM_t			st_hm_attr;					//热度图

	MIVP_LPR_t			st_lpr_attr;					//车牌识别
	//移动目标侦测
	MIVP_DETECT_t		st_detect_attr;
	//车辆抓拍：暂未添加
	BOOL				bPlateSnap;					//车辆抓拍模式，开启后标记全部物体无效且只能划拌线--v2

	char				sSnapRes[16];
}MIVP_t, *PMIVP_t;

typedef void* IVPHandle;

/**
 *@brief 需要报警时的回调函数
 * 该函数主要用于通知分控，是否有警报发生
 *@param channelid 发生报警的通道号
 *@param bAlarmOn 报警开启或者关闭
 *
 */
typedef void (*alarming_ivp_callback_t)(int channelid, BOOL bAlarmOn);

int mivp_init();

/**
 *@brief 初始化
 *@return 0
 *
 */
int mivp_start(int chn);

/**
 *@brief 结束
 *@return 0
 *
 */
int mivp_stop(int chn);

int mivp_count_reset(int chn);
int mivp_count_get(int channelid, MIVP_t *mivp);
int mivp_count_set(int channelid, MIVP_t *mivp);
int mivp_count_in_get();
int mivp_count_out_get();
typedef struct {
	unsigned short clear; 	//底色
	unsigned short text;	//填充色
	unsigned short border;	//描边
}IVPCountColor_t;
unsigned short mivp_count_show_color_get(IVPCountColor_t *color);

/**
 *@brief 设置报警回调函数,通知分控的
 *
 */
int mivp_set_callback(alarming_ivp_callback_t callback);

/**
 *@brief 设置参数
 *@param channelid 频道号
 *@param mivp 要设置的属性结构体
 *@note 如果不能确定所有属性的值，请先#mivp_get_param获取原本的值
 *@return 0 成功，-1 失败
 *
 */
int mivp_set_param(int channelid, MIVP_t *mivp);

/**
 *@brief 获取参数
 *@param channelid 频道号
 *@param mivp 要设置的属性结构体
 *@return 0 成功，-1 失败
 *
 */
int mivp_get_param(int channelid, MIVP_t *mivp);

/*
 * @brief 设置黑天白天模式
 * bNightMode 1夜间模式，0白天模式
 */
int mivp_set_day_night_mode(int bNightMode);

/**
 *@brief 使设置生效
 *	在#mivp_set_param之后，调用本函数
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mivp_restart(int chn);

/*
 * @brief 启用禁用模式
 * @param chn 通道号
 * @param value 模式
 * @param en 1启动，0禁用
 */
int mivp_mode_en(int chn,IVP_MODE value, int en);

/*设置检测灵敏度0~1000,值越小越灵敏,缺省为1*/
int mivp_SetSensitivity(int chn, int sensitivity);

/*设置防区内停留多长时间报警,单位秒,缺省为0*/
int mivp_SetStaytime(int chn, int time);

/*设置运动检测阈值*/
int mivp_SetThreshold(int chn, int value);

/**
 *@brief 暂停/启动 智能分析
 *@param channelid 频道号
 *@param mode 0暂停，1启动
 *@return 0 成功，-1 失败
 */
int mivp_pause(int channelid, int mode);

//是否支持智能分析(总开关)
int mivp_bsupport();
//是否支持绊线和区域入侵(绊线入侵开关)
int mivp_rl_bsupport();
//是否支持智能分析人数统计(人数统计开关)
int mivp_count_bsupport();

int mivp_detect_bsupport();
int mivp_detect_start(int chn);
/****************************************************遮挡报警*****************************************/
int mivp_hide_bsupport();
int mivp_hide_start(int chn);
int mivp_hide_flush(int chn);
int mivp_hide_stop(int chn);
int mivp_hide_set_callback(alarming_ivp_callback_t callback);
int mivp_hide_get_param(int chn, MIVP_HIDE_t *attr);
int mivp_hide_set_param(int chn, MIVP_HIDE_t *attr);
/****************************************************遮挡报警*****************************************/
/*********************************************************人群密度估计****************************************************************/
int mivp_cde_bsupport();				//是否支持
int mivp_cde_correct(int chn);			//标定框设置接口
int mivp_cde_start(int chn);			//功能开启接口
int mivp_cde_stop(int chn);				//功能关闭接口
int mivp_cde_flash(int chn);			//参数动态设置接口
int mivp_cde_get_rate();
int mivp_cde_set_callback(alarming_ivp_callback_t callback);		//报警回调接口
/*********************************************************人群密度估计**************************************************************/
/*********************************************************超员检测**************************************************************/
int mivp_ocl_bsupport();               //超员检测
int mivp_ocl_start(int chn);
int mivp_ocl_stop(int chn);
int mivp_ocl_flush(int chn);
int mivp_ocl_set_callback(alarming_ivp_callback_t callback);
/*********************************************************超员检测**************************************************************/
/*********************************************************快速移动侦测**************************************************************/
int mivp_fm_bsupport();					//是否支持
int mivp_fm_start(int chn);				//功能开启接口
int mivp_fm_stop(int chn);				//功能关闭接口
int mivp_fm_flash(int chn);				//参数动态设置接口
int mivp_fm_set_callback(alarming_ivp_callback_t callback);			//报警回调接口
/*********************************************************快速移动侦测**************************************************************/
/****************************************************徘徊报警*****************************************/
int mivp_hover_bsupport();				//是否支持
int mivp_hover_start(int chn);			//功能开启接口
int mivp_hover_stop(int chn);			//功能关闭接口
int mivp_hover_set_callback(alarming_ivp_callback_t callback);		//报警回调接口
/****************************************************徘徊报警*****************************************/
/****************************************************烟火报警*****************************************/
int mivp_fire_bsupport();				//是否支持
int mivp_fire_start(int chn);			//功能开启接口
int mivp_fire_flash(int chn);			//功能刷新
int mivp_fire_stop(int chn);			//功能关闭接口
int mivp_fire_set_callback(alarming_ivp_callback_t callback);		//报警回调接口
/****************************************************烟火报警*****************************************/
/****************************************************虚焦检测*****************************************/
int mivp_vf_bsupport();
int mivp_vf_start(int chn);
int mivp_vf_flush(int chn);
int mivp_vf_stop(int chn);
int mivp_vf_set_callback(alarming_ivp_callback_t callback);
/****************************************************虚焦检测*****************************************/
/****************************************************热度图*****************************************/
int mivp_hm_bsupport();
int mivp_hm_start(int chn);
int mivp_hm_stop(int chn);
/****************************************************热度图*****************************************/
/****************************************************车牌识别*****************************************/
int mivp_lpr_bsupport();
int mivp_lpr_start(int chn);
int mivp_lpr_stop(int chn);
int mivp_lpr_flush(int chn);
/****************************************************车牌识别*****************************************/

/****************************************************拿取遗留报警*****************************************/
int mivp_tl_bsupport();
int mivp_tl_start(int chn);
int mivp_tl_stop(int chn);
int mivp_tl_set_callback(alarming_ivp_callback_t callback);
int mivp_tl_get_param(int chn, MIVP_TL_t *attr);
int mivp_tl_set_param(int chn, MIVP_TL_t *attr);
int mivp_tl_flush();
int mivp_left_set_callback(alarming_ivp_callback_t callback);
int mivp_removed_set_callback(alarming_ivp_callback_t callback);
/****************************************************拿取遗留报警*****************************************/
/****************************************************场景变更报警*****************************************/
int mivp_sc_bsupport();						//是否支持
int mivp_sc_start(int chn);					//功能开启
int mivp_sc_flush(int chn);						//刷新
int mivp_sc_stop(int chn);					//功能关闭
int mivp_sc_set_callback(alarming_ivp_callback_t callback);	//报警回调
/****************************************************场景变更报警*****************************************/




/**
 *@brief 获取参数，指针方式省内存
 *@param channelid 频道号
 */
PMIVP_t  mivp_get_info(int channelid);


/*设置报警持续多长时间报警,单位秒,缺省为20*/
int mivp_SetAlarmtime(int chn, int time);

int mivp_climb_bsupport(void);

#endif /* MIVP_H_ */
