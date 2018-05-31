/***********************************************************
*ivp.h - Intelligent video parse head file
*
* Copyright(c) 2014~ 
*
*$Date: $ 
*$Revision: $
*
*-----------------------
*$Log: $
*
*
*01a, 14-10-13, Zhushuchao created
*
************************************************************/
#ifndef __IVP_H_
#define __IVP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"
#include "zlist.h"

#include "../../../porting/hi3516D/include/hi_comm_video.h"

#define IVP_RULE_MAX 4

typedef enum
{
    e_IVP_MODE_DRAW_FRAME = 0x1,   //画出拌线或防区
    e_IVP_MODE_FLUSH_FRAME = 0x2,  //报警产生时拌线或防区边线闪烁
    e_IVP_MODE_MARK_OBJECT = 0x4,  //标记报警物体
    e_IVP_MODE_MARK_SMOOTH = 0x8, //平滑模式
    e_IVP_MODE_MARK_ALL = 0x10,      //标记全部运动物体
    e_IVP_MODE_COUNT_SHOW = 0x20, //显示抓拍/人数统计数量 
    e_IVP_MODE_COUNT_INOUT = 0x40, //客流统计
    e_IVP_MODE_PLATE_SNAP = 0x80,  //车辆抓拍模式，开启后标记全部物体无效且只能划拌线
    e_IVP_MODE_PLATE_RECOGNIZING = 0x100, //车牌识别 
    e_IVP_MODE_FIRE_DETECT = 0x200, //火焰检测
    e_IVP_MODE_ABANDONED_OBJ_DETECTION = 0x400, //遗留物体检测
    e_IVP_MODE_REMOVED_OBJ_DETECTION = 0x800, //被移走物体检测
    e_IVP_MODE_CROWD_DENSITY_ESTIMATE = 0x1000, //人群密度估计
    e_IVP_MODE_HOVER_DETECTION = 0x2000, //人员徘徊检测
    e_IVP_MODE_HEAT_MAP = 0x4000, //热度图
    e_IVP_MODE_FAST_MOVE = 0x8000, //快速移动
    e_IVP_MODE_VAR_RATE = 0x10000, //占有率检测模式，检测周期较长时可与其他模式同时运行
    e_IVP_MODE_SCENE_CHANGE = 0x40000,
    e_IVP_MODE_LPR = 0x1000000, //车牌识别 
}IVP_MODE;

typedef enum
{
    /*防区检测*/
    e_IVP_CHECK_MODE_AREA_IN = 0x1,  
    e_IVP_CHECK_MODE_AREA_OUT = 0x2, 
    e_IVP_CHECK_MODE_AREA = 0x3, 
    /*遮挡检测*/
    e_IVP_CHECK_MODE_HIDE = 0x4,
    /*虚焦检测*/
    e_IVP_CHECK_MODE_VIRTUAL_FOCUS = 0x8,
    /*拌线检测*/
    e_IVP_CHECK_MODE_LINE_L2R = 0x10,  /*从左向右跨拌线报警*/
    e_IVP_CHECK_MODE_LINE_R2L = 0x20,  /*从右向左跨拌线报警*/  
    e_IVP_CHECK_MODE_LINE_LR = 0x30,   /*左右双向报警*/
    e_IVP_CHECK_MODE_LINE_U2D = 0x40,
    e_IVP_CHECK_MODE_LINE_D2U = 0x80,
    e_IVP_CHECK_MODE_LINE_UD = 0xc0,
    /*车辆抓拍*/
    e_IVP_CHECK_MODE_CAR_SNAP = 0x100,    
    /*遗留拿取检测*/
    e_IVP_CHECK_MODE_ABANDONED_OBJ = 0x400, //遗留物体检测
    e_IVP_CHECK_MODE_REMOVED_OBJ = 0x800, //被移走物体检测
    /*人群密度估计*/
    e_IVP_CHECK_MODE_CROWD_DENSITY = 0x1000, //人群密度估计
    /*人员徘徊检测*/
    e_IVP_CHECK_MODE_HOVER = 0x2000, //人员徘徊检测
    e_IVP_CHECK_MODE_HEAT_MAP = 0x4000, //热度图
    /*快速移动*/
    e_IVP_CHECK_MODE_FAST_MOVE = 0x8000, //快速移动
    e_IVP_CHECK_MODE_MOTION_RATE = 0x20000,  //移动目标比率
    e_IVP_CHECK_MODE_SCENE_CHANGE = 0x40000,
}IVP_CHECK_MODE;

typedef enum
{
    e_IPV_ALARM_TYPE_CLEAR = 0,
    e_IPV_ALARM_TYPE_IN = 0x1,
    e_IPV_ALARM_TYPE_OUT = 0x2,
    e_IPV_ALARM_TYPE_HIDE = 0x4,
    e_IPV_ALARM_TYPE_VIRTUAL_FOCUS = 0x8,
    e_IPV_ALARM_TYPE_L2R = 0x10,
    e_IPV_ALARM_TYPE_R2L = 0x20,
    e_IPV_ALARM_TYPE_LR = 0x30,
    e_IPV_ALARM_TYPE_U2D = 0x40,
    e_IPV_ALARM_TYPE_D2U = 0x80,
    e_IPV_ALARM_TYPE_UD = 0xc0,
    e_IPV_ALARM_TYPE_COUNT = 0x100,
    e_IPV_ALARM_TYPE_FIRE = 0x200, //火焰/烟雾报警, areaIndex=0为火焰，areaIndex=1为烟雾
    e_IPV_ALARM_TYPE_ABANDONED_OBJ = 0x400, //遗留物体检测
    e_IPV_ALARM_TYPE_REMOVED_OBJ = 0x800, //被移走物体检测
    e_IPV_ALARM_TYPE_CROWD_DENSITY = 0x1000, //人群密度估计
    e_IPV_ALARM_TYPE_HOVER = 0x2000, //人员徘徊检测
    e_IPV_ALARM_TYPE_OCL = 0x3000, //超员报警
    e_IPV_ALARM_TYPE_HEAT_MAP = 0x4000, //热度图
    e_IPV_ALARM_TYPE_FAST_MOVE = 0x8000, //快速移动
    e_IVP_ALARM_TYPE_SCENE_CHANGE = 0x40000,
}IVP_ALARM_TYPE;

typedef enum {
    e_IVP_RES_16_9 = 1,
    e_IVP_RES_4_3,
    e_IVP_RES_BUTT,
} IVP_RES_TYPE;

typedef enum
{
    e_IVP_ABANDON_CTL_TYPE_ALARM_TIME = 0,  // 遗留/拿取物体报警时间
    e_IVP_ABANDON_CTL_TYPE_SENSITIVITY = 1,  //遗留拿取检测灵敏度,可设置为0-5,默认为3
}IVP_ABANDON_CTL_TYPE;

typedef struct
{
    int x;
    int y;
}IVP_POINT;

typedef struct
{
    int x;
    int y;
    int x2;
    int y2;
}IVP_RECT;

typedef struct
{
    _UINT8 st; /*为1的位代表哪一个规则触发报警*/
    _UINT8 mode; /*IVP_ALARM_TYPE组合*/
    _UINT8 count;  
    _UINT8 count1;
    _UINT8 count2;
    _UINT8 count3;
    _UINT8 id;
    _UINT8 type; /*IVP_ALARM_TYPE for trace area out*/
    _INT16 xm;
    _INT16 ym;
    _UINT8 st0;
    _INT8 lst;
    _UINT16 stayTime;
    _UINT32 np;
    int x;
    int y;
    int x2;
    int y2;
    IVP_RECT dr;
    void * v;
    //void *prev;
}IVP_AREA;

typedef struct
{
    _UINT32 u32PhyAddr[3];
    void  *pVirAddr[3];
    _UINT32 u32Stride[3];
    void *res; //外部传入，释放时传出
}IVP_FRAME;

/*占用区域轮廓数据结构，占用区域使用Z_LIST存储，每个Z_NODE 的 data 指向一下数据结构*/
/*该轮廓与摄像头传感器画面尺寸比为3:1
  调试时源图像画面320*240，得到该轮廓范围为106*80
*/
typedef struct
{
  int row;
  int start;
  int end;
}ivpINTERVAL;
/*报警回调
    areaIndex=报警区域索引
    type>0报警发生type=0报警清除
    areas为报警区域IVP_AREA列表

    客流统计时,
        areaInddex = 进入数量<<16|出数量
        type = e_IPV_ALARM_TYPE_COUNT
        areas = null
*/
typedef void (*ON_IVP_ALARM)(void *ivp, int areaIndex, _UINT32 type, Z_LIST *areas);

typedef void (*ON_IVP_RATE)(void *ivp, double rate, struct timeval tv);

/* Generic Callback Function */
typedef void (*ON_IVP)(void *ivp, void * parg);

/*车牌识别回调函数*/
/**
 * ON_IVP_LPR ivp lpr callback type
 * @param ivp [in] ivp handle
 * @param parg [in] 按设定的回调类型解析
 */
typedef void (*ON_IVP_LPR)(void *ivp, const void * parg);

/*
云台控制状态读取回调
    当控制时返回1,否则返回0
*/
typedef int (* IVP_PTZ_CALLBK)(int chn);

/*画图回调,用于nvr等需要外部画线、框的设备
    线框坐标为ivpGetImageParseSize获取的分辨率，外部需要转化为实际显示分辨率
    IVP_DRAW_RECT_CALLBK:画2像素线宽的矩形框
    IVP_DRAW_LINE_CALLBK:画width线宽的直线
    clr 为RGB4444格式, 高4位为透明度,clr = 0时清除
    IVP_DRAW_RECT_CALLBK参数
       chn = ivpStart传入的chn
       mode = 0实心框，mode = 1边线框
       rt = 矩形左上、右下坐标
       clr = RGB4444格式颜色
    IVP_DRAW_LINE_CALLBK参数
       chn = ivpStart传入的chn
       x1,y1,x2,y2 = 两点坐标
       clr = RGB4444格式颜色
       width = 线宽
    返回值0=成功，非0为错误码
    坐标转换算法，假设输出为坐标x1,y1，转换后坐标为x,y：
        int ivpWidth, ivpHeight, x, y;
        ivpGetImageParseSize(ivp, &ivpWidth, &ivpHeight);
        x = x1*实际分辨率宽度/ivpWidth;
        y = y1*实际分辨率高度/ivpHeight;
*/
typedef int (*IVP_DRAW_RECT_CALLBK)(int chn, int mode, IVP_RECT *rt, _UINT16 clr);
typedef int (*IVP_DRAW_LINE_CALLBK)(int chn, int x1, int y1,  int x2,  int y2, _UINT16 clr, int width);

/*初始化
    mode = 为IVP_MODE的组合
    chn = 为要检测的vi通道
    pixelfmt = PIXEL_FORMAT_RGB_4444或者PIXEL_FORMAT_RGB_1555
    失败返回NULL，成功返回句柄*/
void *ivpStart(_UINT32 mode, int chn, int pixelfmt);

/*
ivpStart2需要的回调
*/
/*获取yuv数据回调
chn >= 0获取对应通道数据，chn<0释放已获取的数据
frame = yuv数据参数指针，由外部填充
timeout = 毫秒超时时间，-1为阻塞
成功返回0，失败返回错误码*/
typedef int (*IVP_GET_FRAME_CALLBK)(int chn, IVP_FRAME *frame, int timeout); 
/*申请/释放物理内存回调，注册后通过回调申请或释放内存(*phyAddr不为零为释放)，
    成功返回0并填充phyAddr、virAddr、mapAddr 
    失败返回非0
*/
typedef int (*IVP_MEM_CALLBK)(_UINT32 size, void **phyAddr, void **virAddr, void **mapAddr); 

/*初始化函数2，用于从外部获取yuv数据的情况(例如nvr)
    mode = 为IVP_MODE的组合
    chn = 为要检测的通道
    width,height=输入图像宽高
    getFrame = 获取yuv帧回调
    memCallBk = MMZ内存申请回调
    失败返回NULL，成功返回句柄*/
void *ivpStart2(_UINT32 mode, int chn, int width, int height, IVP_GET_FRAME_CALLBK getFrame, IVP_MEM_CALLBK memCallBk);

/*释放*/
int ivpRelease(void *ivp);

/*注册报警回调*/
int ivpRegisterCallbk(void *ivp, ON_IVP_ALARM callbk);

/*注册云台状态读取回调,无云台的设备不需要注册*/
int ivpRegisterPtzCallbk(void * ivp, IVP_PTZ_CALLBK callbk);

/*注册画图回调*/
int ivpRegisterDrawCallbk(void * ivp, IVP_DRAW_RECT_CALLBK cbRect, IVP_DRAW_LINE_CALLBK cbLine);

/*设置检测灵敏度0~100,值越小越灵敏,缺省为6*/
int ivpSetSensitivity(void *ivp, int sensitivity);

/*设置防区内停留多长时间报警,单位秒,缺省为0*/
int ivpSetStaytime(void *ivp, int time);
/*设置防区内报警多长时间后停止报警,单位100ms,缺省为20*/
int ivpSetAlarmtime(void *ivp, int time);
/*设置运动检测阈值,缺省40*/
int ivpSetThreshold(void *ivp, int value);

/*启用/禁用标识*/
int ivpEnMode(void *ivp, IVP_MODE value, int en);

/*复位计数器等*/
int ivpReset(void *ivp);

/*
暂停/启动分析，当控制云台时，可调用此接口暂停或恢复分析功能
    mode = 0 启动;
    mode = 1 停止;
*/
int ivpPause(void *ivp, int mode);

/*
配置检测规则
   index = 规则索引,
   ponitNum = 坐标点数(拌线检测为2,区域检测>2),
   points = 坐标列表,
   orgW/orgH = 坐标点依据的图像宽高
*/
int ivpAddRule(void *ivp,  int index, IVP_CHECK_MODE mode,  int pointNum, IVP_POINT *points, int orgW, int orgH);

/*
设置白天夜间模式
 mode = 0; 白天
 mode = 1;夜间
*/
int ivpSetDayNightMode(void *ivp, int mode);

/*发送一帧数据，nvr上使用，初始化完成后，发送需要分析的通道的子码流H264数据
   buf=帧缓冲区
   len=帧长
   ts=时戳
   返回0表示成功，非0为错误代码
*/
int ivpSendFream(void *ivp, _UINT8 *buf, int len, _UINT32 ts);

/*获取ivpStart时传入的chn*/
int ivpGetChn(void *ivp);

/*获取分析图像分辨率，用于外部画框时进行坐标转换等
   width = 宽度输出指针
   height = 高度输出指针
   返回0表示成功*/
int ivpGetImageParseSize(void *ivp, int *width, int *height);

/*遗留、拿取参数设置
    param: type IVP_ABANDON_CTL_TYPE
    value: IVP_ABANDON_CTL_TYPE 对应建议值*/
int ivpAbandonDctl(void *ivp, int type, int value);

/*注册占有率变化报警回调*/
int ivpRegVRAlarmCB(void *ivp, ON_IVP_RATE callback);
/*注册占有率变化计算完成回调*/
int ivpRegVRFinishCB(void *ivp, ON_IVP_RATE callback);
/*设置占有率变化报警阈值*/
int ivpSetVRBias(void *ivp, double bias);
/*设置占有率检测周期，单位:s*/
int ivpSetVRCircleT(void *ivp, int sec);
/*设置占有率检测基准值更新周期，单位:s*/
int ivpSetVRBaseUpT(void *ivp, int sec);
/*获取占有率变化值*/
int ivpGetVR(void *ivp,double * vr,struct timeval *tv);
/*外部触发占有率分析算法执行*/
int ivpVRExternTriger(void *ivp);
/*更新占有率分析基准值*/
int ivpUpdateVRef(void *ivp);
/*获取占用范围轮廓*/
Z_LIST *ivpGetVRContour(void *ivp);
/*注册人群聚集报警回调*/
int ivpRegDensityEstimateAlarmCB(void *ivp, ON_IVP_RATE callback);
/*注册人群密度计算完成回调*/
int ivpRegDensityEstimateFinishCB(void *ivp, ON_IVP_RATE callback);
/*设置人群密度报警阈值*/
int ivpSetDensityEstimateAlarmBias(void *ivp, double value);
/*添加人群密度估计权重标定矩形框*/
int ivpAddDensityEstimateRect(void *ivp, IVP_RECT *rect, int rect_num, int orgW, int orgH);

/* 热度图 配置区域 */
typedef enum {
    e_IVP_HEATMAP_CTL_TYPE_TIME = 0, // 热度图回调时间间隔, 单位 秒 s
    e_IVP_HEATMAP_CTL_TYPE_BUTT,
} IVP_HEATMAP_CTL_TYPE;

/* 热度图 配置 */
int ivpHeatMapctl(void *ivp, int type, int value);

/*注册热度图回调*/
int ivpRegHeatMapCb(void *ivp, ON_IVP cb);
/* End of 热度图 配置区域 */

/* 快速移动 配置区域 */
typedef enum {
    /* 快速移动速度等级 0 - 5,
       等级为 n 时, 代表小于 6 - n 秒横向穿过摄像机视野的目标触发报警 */
    e_IVP_FAST_MOVE_CTL_SPEED_LEVEL = 0,
    e_IVP_FAST_MOVE_CTL_TYPE_BUTT,
} IVP_FAST_MOVE_CTL_TYPE;

/* 快速移动 配置 */
int ivpFastMovectl(void *ivp, int type, int value);
/* End of 快速移动 配置区域 */

/* motion rate 配置 */
/**
 * ivpRegMotionRateCb 移动目标比率的注册回调函数
 * @param ivp [in] ivp handle pointer
 * @param onMotionRate [in] callback function of motion rate
 * @return errorcode
 */
int ivpRegMotionRateCb(void *ivp, ON_IVP_RATE onMotionRate);
/* End of motion rate 配置区域 */

/* 场景变更 配置区域 */
/* 配置视场偏移比例阀值，有效值[0.0125,0.0625] */
int ivpSceneChangeSetThreshold(void *ivp, float threshold);
/* 配置报警持续时长，单位为秒 */
int ivpSceneChangeSetDuration(void *ivp, int duration);
/* End of 场景变更 配置区域 */

/* 烟火检测 配置区域 */
/* 配置烟火检测灵敏度，有效范围(0-100],值越小越灵敏, 缺省为30 */
int ivpFireSmokeSetSensitivity(void *ivp, int sensitivity);
/* End of 烟火检测 配置区域 */

/*设置遮挡检测灵敏度0~100,值越小越灵敏,缺省为6*/
int ivpSetHideSensitivity(void *ivp, int sensitivity);

/*设置虚焦检测灵敏度0~100,值越小越灵敏,缺省为6*/
int ivpSetFocusSensitivity(void *ivp, int sensitivity);

/*设置车牌识别参数类型*/
typedef enum {
    e_IVP_LPR_PARAM_WORK_MODE = 0,
    e_IVP_LPR_PARAM_ROI,
    e_IVP_LPR_PARAM_DIRECTION,
    e_IVP_LPR_PARAM_DISPLAY_ROI,
    e_IVP_LPR_PARAM_TRIGGER_TIMEOUT,
    e_IVP_LPR_PARAM_DEFAULT_PROVINCE,
} IVP_LPR_PARAM_TYPE;

/*车牌识别工作模式*/
typedef enum {
    e_IVP_LPR_WORK_MODE_AUTO = 0,
    e_IVP_LPR_WORK_MODE_OUTSIDE_TRIGGER,
    e_IVP_LPR_WORK_MODE_INSIDE_TRIGGER,
} IVP_LPR_PARAM_WORK_MODE;

/* 目前必须 width = 1680, height = 288 */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} IVP_LPR_RECT;

/* 车牌倾斜方向 */
typedef enum {
    e_IVP_LPR_DIR_CCW = 0,
    e_IVP_LPR_DIR_CW = 1,
} IVP_LPR_PARAM_DIRECTION;

/* 是否显示LPR ROI */
typedef enum {
    e_IVP_LPR_HIDE_ROI = 0,
    e_IVP_LPR_SHOW_ROI = 1,
} IVP_LPR_PARAM_DISPLAY_TYPE;

/*
 * ivplprctl 车牌识别参数设置
 * @param ivp [in] ivp 句柄
 * @param type [in] 参数类型
 *      e_IVP_LPR_PARAM_WORK_MODE,对应 parg 类型 IVP_LPR_PARAM_WORK_MODE *
 *      e_IVP_LPR_PARAM_ROI,对应 parg 类型 IVP_LPR_RECT *
 *      e_IVP_LPR_PARAM_DIRECTION,对应 parg 类型 IVP_LPR_PARAM_DIRECTION *
 *      e_IVP_LPR_PARAM_DISPLAY_ROI,对应 parg 类型 IVP_LPR_PARAM_DISPLAY_TYPE *
 *      e_IVP_LPR_PARAM_TRIGGER_TIMEOUT,对应 parg 类型 int *, 单位 ms
 *      e_IVP_LPR_PARAM_DEFAULT_PROVINCE,对应 parg 类型 char *
 * @param parg [in] 参数指针
 * @return status 设置成功状态
 */

int ivplprctl(void * ivp, IVP_LPR_PARAM_TYPE type, void * parg);

/* LPR 回调 YUV420 图像结构 */
typedef struct {
    int width;
    int height;
    void* yuv[3];
} IVP_LPR_YUV420;

/* LPR default 回调参数结构 */
typedef struct {
    const char * lp;
    IVP_LPR_YUV420 * org_yuv;
    IVP_LPR_RECT * lp_location;
    VIDEO_FRAME_INFO_S * org_frame;
} IVP_LPR_CB_DEFAULT_PARAM;
 
/*设置车牌回调函数是否携带对应数据类型*/
/**
 * IVP_LPR_CB_PARAM_TYPE 设置LPR回调函数携带的数据类型,回调函数中参数包含的数据由此确定
 *  e_IVP_LPR_CB_PARAM_RESULT 车牌识别结果,类型为char*
 *  e_IVP_LPR_CB_PARAM_ORG_IMAGE 车牌识别结果对应原图,类型为IVP_LPR_YUV420*
 *  e_IVP_LPR_CB_PARAM_RESULT 车牌识别结果对应车牌坐标,类型为IVP_LPR_RECT*
 */
typedef enum {
    e_IVP_LPR_CB_PARAM_RESULT = 0,
    e_IVP_LPR_CB_PARAM_ORG_IMAGE,
    e_IVP_LPR_CB_PARAM_LOCATION,
    e_IVP_LPR_CB_PARAM_ORG_FRAME,
} IVP_LPR_CB_PARAM_TYPE;

/*设置车牌回调函数类型*/
/**
 * IVP_LPR_CB_TYPE 设置LPR回调函数类型,回调函数中参数解析方式由此确定
 *  e_IVP_LPR_CB_DEFAULT 回调为车牌识别结果,类型为char*,以及在ivplprsetcbparam中设定的类型
 */
typedef enum {
    e_IVP_LPR_CB_DEFAULT = 0,
} IVP_LPR_CB_TYPE;

/**
 * ivplprregcb 车牌识别回调注册
 * @param ivp [in] ivp 句柄
 * @param type [in] LPR 回调函数类型 IVP_LPR_CB_TYPE
 * @param cb [in] 回调函数指针
 * @return status 设置成功状态
 */
int ivplprregcb(void * ivp, IVP_LPR_CB_TYPE type, void * cb);

/**
 * ivplprcbenparam 车牌识别回调携带数据类型使能
 * @param ivp [in] ivp 句柄
 * @param type [in] LPR 回调函数类型 IVP_LPR_CB_TYPE
 * @param param [in] LPR 回调数据类型 IVP_LPR_CB_PARAM_TYPE
 * @return status 设置成功状态
 */
int ivplprcbenparam(void * ivp, IVP_LPR_CB_TYPE type, IVP_LPR_CB_PARAM_TYPE param);

/**
 * ivplprcbdisparam 车牌识别回调携带数据类型去使能
 * @param ivp [in] ivp 句柄
 * @param type [in] LPR 回调函数类型 IVP_LPR_CB_TYPE
 * @param param [in] LPR 回调数据类型 IVP_LPR_CB_PARAM_TYPE
 * @return status 设置成功状态
 */
int ivplprcbdisparam(void * ivp, IVP_LPR_CB_TYPE type, IVP_LPR_CB_PARAM_TYPE param);

/**
 * ivpLPRTrigger 车牌识别外部触发接口
 * @param ivp [in] ivp 句柄
 * @return status 触发成功状态
 */
int ivplprtrigger(void * ivp);



#ifdef __cplusplus
}
#endif
#endif

