#ifndef __M_PTZ_H__
#define __M_PTZ_H__


#define MAX_PATROL_NOD	256
#define MAX_PRESET_CT	256	//预置点最大个数

#include "libPTZ.h"
#include "jv_common.h"


#define PTZ_SPEED_NOR	255

//云台结构体
typedef struct _PATROL_NOD
{
    U32 nPreset;      //预置点数组下标
    U32 nStayTime;    //停留时间
}PATROL_NOD, *PPATROL_NOD;

//云台结构体
typedef struct _PTZ
{
    U32          nAddr;                      //地址
    U32          nProtocol;                  //协议
    U32          nBaudRate;                  //波特率,数据位,停止位等
    NC_PORTPARAMS nHwParams;					//串口硬件设置信息
    BOOL            bLeftRightSwitch;           //左右互换
    BOOL            bUpDownSwitch;              //上下互换
    BOOL            bIrisZoomSwitch;            //光圈变倍互换
    BOOL            bFocusZoomSwitch;           //变焦变倍互换
    BOOL            bZoomSwitch;                //变倍互换
//    BOOL		    bPtzAuto;                   //是否处于自动状态
//    BOOL		    bStartPatrol;               //是否处于软巡航状态
//    BOOL		    nCurPatrolNod;              //当前软巡航点
//    time_t          tmCurPatrolNod;             //当前巡航时间
//    U32			nPresetCt;						//预置点个数
//    U32			pPreset[MAX_PRESET_CT];			//预置点数组
//    U32          nPatrolSize;                //巡航包含的预置点数
//    PATROL_NOD      aPatrol[MAX_PATROL_NOD];    //巡航
	U32		scanSpeed;	//线扫速度
}PTZ, *PPTZ;

//3D定位结构体
typedef struct _ZONE
{
	int x;		//客户端圈定区域中心 x坐标
	int y;		//客户端圈定区域中心 y坐标
	int w;		//客户端圈定区域宽
	int h;		//客户端圈定区域高
}ZONE_INFO, *PZONE_INFO;

//移动跟踪结构体
typedef struct _TRACEOBJ
{
	int x;		//x坐标步长
	int y;		//y坐标步长
	int zoom;	//zoom步长
	int focus;	//focus步长
}TRACE_INFO, *PTRACE_INFO;

typedef enum
{
	GUARD_NO,
	GUARD_PRESET,
	GUARD_PATROL,
	GUARD_TRAIL,
	GUARD_SCAN,
}PTZ_GUARD_TYPE_E;

//守望信息
typedef struct
{
	U32 guardTime;		//守望时间。0表示守望关闭
	PTZ_GUARD_TYPE_E guardType;
	union
	{
		U32 nRreset;
		U32 nPatrol;;
		U32 nTrail;
		U32 nScan;
	};
}PTZ_GUARD_T;

//预置点信息
typedef struct
{
    U32			nPresetCt;						//预置点个数
    U32			pPreset[MAX_PRESET_CT];			//预置点数组
    char			name[MAX_PRESET_CT][32];		//预置点名称
//    int 		lenzoompos[MAX_PRESET_CT];
//	int 		lenfocuspos[MAX_PRESET_CT];
}PTZ_PRESET_INFO;

//巡航信息
typedef struct
{
    U32         nPatrolSize;                //巡航包含的预置点数
    U32         nPatrolSpeed;               //巡航速度
    PATROL_NOD  aPatrol[MAX_PATROL_NOD];    //巡航
}PTZ_PATROL_INFO;

//云台定时任务
typedef struct
{
	U32 		schedule[2];		//定时任务
	struct{
    	char hour;
    	char minute;
    }schTimeStart[2], schTimeEnd[2];	//定时任务时间段
	BOOL bSchEn[2];						//使能定时任务1/2
}PTZ_SCHEDULE_INFO;

//typedef struct
//{
//	BOOL bGuardonStart;		//激活守护
//	int guardTimeCnt;		//激活计时
//}PTZ_SCHGUARD_STATUS;

typedef struct
{
	//BOOL sch_runflag[2];
	int sch_LastSch;
	int sch_LastItem;	//定时任务标志，用于任务开始和停止判断
}PTZ_SCHEDULE_STATUS;

PTZ *PTZ_GetInfo(void);

PTZ_PRESET_INFO *PTZ_GetPreset(void);

PTZ_PATROL_INFO *PTZ_GetPatrol(void);

PTZ_GUARD_T *PTZ_GetGuard(void);

PTZ_SCHEDULE_INFO *PTZ_GetSchedule(void);	//获取定时任务信息

S32 PTZ_Init();
S32 PTZ_Release();

/**
 * 设置预置点
 * @param nCh	通道号，默认 0
 * @param nPreset 预置点号
 */
void inline PtzSetPreset(U32 nCh, S32 nPreset);
S32 PTZ_AddPreset(U32 nCh, U32 nPreset, char *name);
S32 PTZ_DelPreset(U32 nCh, U32 nPreset);
S32 PTZ_StartPatrol(U32 nCh, U32 nPatrol);	//开启巡航线 1、2
S32 PTZ_StopPatrol(U32 nCh);
S32 Ptz_Guard_Pause(U32 nCh);

/////////////////////////////////////云台控制///////////////////////////////////
S32 inline PTZ_Auto(U32 nCh, U32 nSpeed);
//开始向上
void inline PtzUpStart(U32 nCh, S32 nSpeed);
//停止向上
void inline PtzUpStop(U32 nCh);
//开始向下
void inline PtzDownStart(U32 nCh, S32 nSpeed);
//停止向下
void inline PtzDownStop(U32 nCh);
//开始向左
void inline PtzLeftStart(U32 nCh, S32 nSpeed);
//停止向左
void inline PtzLeftStop(U32 nCh);
//开始向右
void inline PtzRightStart(U32 nCh, S32 nSpeed);
//停止向右
void inline PtzRightStop(U32 nCh);

//多方位移动
//bLeft 为真时左移，为假是右移，leftSpeed为0时不移动
//bUp 为真是上移，为假时下移，upSpeed为0时不移动
void PtzPanTiltStart(U32 nCh, BOOL bLeft, BOOL bUp, int leftSpeed, int upSpeed);
void PtzPanTiltStop(U32 nCh);


//开始光圈+
void inline PtzIrisOpenStart(U32 nCh);
//停止光圈+
void inline PtzIrisOpenStop(U32 nCh);
//开始光圈-
void inline PtzIrisCloseStart(U32 nCh);
//停止光圈-
void inline PtzIrisCloseStop(U32 nCh);
//开始变焦+
void inline PtzFocusNearStart(U32 nCh);
//停止变焦+
void inline PtzFocusNearStop(U32 nCh);
//开始变焦-
void inline PtzFocusFarStart(U32 nCh);
//停止变焦-
void inline PtzFocusFarStop(U32 nCh);
//开始变倍+
void inline PtzZoomOutStart(U32 nCh);
//停止变倍+
void inline PtzZoomOutStop(U32 nCh);
//开始变倍-
void inline PtzZoomInStart(U32 nCh);
//停止变倍-
void inline PtzZoomInStop(U32 nCh);
//转到预置点
void inline PtzLocatePreset(U32 nCh, S32 nPreset);
//自动
void inline PtzAutoStart(U32 nCh ,S32 nSpeed);
//停止
void inline PtzAutoStop(U32 nCh);

S32 AddPatrolNod(PTZ_PATROL_INFO *pPatrol, U32 nPreset, U32 nStayTime);
S32 DelPatrolNod(PTZ_PATROL_INFO *pPatrol, U32 nPreset);
S32 ModifyPatrolNod(PTZ_PATROL_INFO *pPatrol, U32 nOldPreset, U32 nNewPreset, U32 nNewStayTime);


//开始录制轨迹
S32 PtzTrailStartRec(U32 nCh, U32 nTrail);

//停止录制轨迹
S32 PtzTrailStopRec(U32 nCh, U32 nTrail);

//开始轨迹
S32 PtzTrailStart(U32 nCh, U32 nTrail);

//停止轨迹
S32 PtzTrailStop(U32 nCh, U32 nTrail);

//启用或者停止守望
S32 PtzGuardSet(U32 nCh, PTZ_GUARD_T *guard);

//设置线扫左边界
S32 PtzLimitScanLeft(U32 nCh);

//设置线扫右边界
S32 PtzLimitScanRight(U32 nCh);

//设置线扫速度
S32 PtzLimitScanSpeed(U32 nCh, int nScan, int nSpeed);

//开始线扫
S32 PtzLimitScanStart(U32 nCh, int nScan);

//停止线扫
S32 PtzLimitScanStop(U32 nCh, int nScan);

//开始花样扫描
S32 PtzWaveScanStart(U32 nCh, int nSpeed);

//结束花样扫描
S32 PtzWaveScanStop(U32 nCh, int nSpeed);

//设置线扫上边界
S32 PtzLimitScanUp(U32 nCh);

//设置线扫上边界
S32 PtzLimitScanDown(U32 nCh);

//开始垂直扫描
S32 PtzVertScanStart(U32 nCh, int nSpeed);

//开始随机扫描
S32 PtzRandomScanStart(U32 nCh, int nSpeed);

//开始帧扫描
S32 PtzFrameScanStart(U32 nCh, int nSpeed);

//函数说明 : 把波特率转换字符串
//参数     : HI_U32 nBaudrate:波特率
//返回值   : COMBO的索引 ; -1:失败
char *Ptz_BaudrateToStr(U32 nBaudrate);

//函数说明 : 字符串转换成波特率
//参数     : HI_U32 nIndex :COMBO的索引
//返回值   : 波特率 ; -1:失败
U32 Ptz_StrToBaudrate(char *cStr);

//函数说明 : 设置波特率时,把波特率转换成COMBO的索引号
//参数     : U32 nBaudrate:波特率
//返回值   : COMBO的索引 ; -1:失败
S32 Ptz_BaudrateToIndex(U32 nBaudrate);

//函数说明 : 设置波特率时,把COMBO的索引号转换成波特率
//参数     : U32 nIndex :COMBO的索引
//返回值   : 波特率 ; -1:失败
S32 Ptz_IndexToBaudrate(U32 nIndex);

//函数说明 : 设置协议时,把协议转换成COMBO的索引号
//参数     : U32 nProtocol:libPTZ.a中的协议编号
//返回值   : COMBO的索引 ; -1:失败
S32 Ptz_ProtocolToIndex(U32 nProtocol);

//函数说明 : 设置协议时,把COMBO的索引号转换成libPTZ.a中的协议编号
//参数     : U32 nIndex :COMBO的索引
//返回值   : 协议编号 ; -1:失败
S32 Ptz_IndexToProtocol(U32 nIndex);

//辅助N 开始
//n 第n项辅助功能
void PtzAuxAutoOn(U32 nCh, int n);

//辅助N 结束
//n 第n项辅助功能
void PtzAuxAutoOff(U32 nCh, int n);
//云台参数设置
void PtzReSetup(NC_PORTPARAMS param);

//函数说明 : 一键批量设置预置点、巡航函数
//参数     :  无
//返回值   :  无
void PTZ_PresetsPatrolStart();

//函数说明 : 一键批量删除预置点、退出巡航函数
//参数     :   无
//返回值   :  无
void PTZ_PresetsPatrolStop();

//函数说明 : 设定开机启动项
//参数     :nCh 通道   item  启动项
//返回值   : 

void PTZ_SetBootConfigItem(U32 nCh, U32 item);

//函数说明 : 获取开机启动项
//参数     :
//返回值   : 启动项

U32 PTZ_GetBootConfigItem();

//函数说明 : 启动开机启动项
//参数     :
//返回值   : 

void PTZ_StartBootConfigItem();

//函数说明 : 启动定时计划
//参数     :
//返回值   : 
void PTZ_StartSchedule();

//函数说明 : 获取定时任务守护状态
//参数     :
//返回值   : 守护状态
//PTZ_SCHGUARD_STATUS *PTZ_GetGuardStatus(void);

//函数说明 : 获取巡航状态
//参数     :
//返回值   : 巡航状态, 运行:TRUE/ 停止:FALSE
BOOL PTZ_Get_PatrolStatus(U32 nCh);

//函数说明 : 获取轨迹状态
//参数     :
//返回值   : 轨迹状态, 运行:TRUE/ 停止:FALSE
BOOL PTZ_Get_TrailStatus(U32 nCh);

//函数说明 : 获取云台控制状态
//参数     :
//返回值   : 云台控制, 运行:TRUE/ 停止:FALSE
BOOL PTZ_Get_Status(U32 nCh);

//函数说明 : 3D定位函数
//参数     : 通道号、定位区域、放大/缩小
//返回值   : 
void PtzZoomZone(U32 nCh, ZONE_INFO *zone, int width, int height, int zoom);

//函数说明 : 移动跟踪函数
//参数     : 通道号、跟踪步长、位置跟踪指令
//返回值   : 
void PtzTraceObj(U32 nCh, TRACE_INFO *trace, int cmd);

//函数说明 : 设定红外模式
//参数     : 模式:自动/手动开/手动关
//返回值   : 
void PTZ_SetIrMod(int mode);
#endif

