#ifndef __JV_COMMON_H__
#define __JV_COMMON_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <pthread.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <signal.h>
#include <sys/stat.h>	//mkdir函数使用
#include <sys/vfs.h>	//检测硬盘空间使用
#include <dirent.h>		//检索目录使用
#include <errno.h>		//查看错误编号使用
#include <sys/wait.h>	//检测子进程状态使用
#include <sys/socket.h>	//定义了AF_INET、SOCK_DGRAM等
#include <netinet/in.h>	//定义了网络函数
#include <arpa/inet.h>	//定义了inet_ntoa等函数
#include <sys/shm.h>	//共享内存使用
#include <sys/mount.h>	//mount
#include <sys/param.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <mqueue.h> 
#include <semaphore.h>
#include <mqueue.h>
#include <stdarg.h>

#include "defines.h"
#include "jv_product_def.h"

//是否调试版本
#define IPCAMDEBUG		debugFlag
extern int debugFlag;


//V1.0.248表示第一个SVN版本库中的第246条版本，如为第1246条则表示为V1.1246
extern char main_version[16];
extern char ipc_version[32];
#define MAIN_VERSION	main_version
#define SUB_VERSION		".5600"
#define IPCAM_VERSION	ipc_version


//软件的配置文件，用于配置编译好的软件的执行特性
#define JOVISION_CONFIG_FILE	"/etc/jovision.cfg"


#ifndef CONFIG_PATH
#define CONFIG_PATH "/etc/conf.d/jovision/"
#endif

//用于保存一些，永远不会改变的参数的目录
#define CONFIG_FIXED_PATH "/etc/conf.d/fixed/"

//永远不会改变的文件
#define CONFIG_FIXED_FILE CONFIG_FIXED_PATH"fixed"

#define CONFIG_HWCONFIG_FILE CONFIG_FIXED_PATH"hwconfig.cfg"

#define MAX_PATH			256


/**
 * errno of jovision. 
 * It should be added with JVERRBASE_XXX
 */
#define JVERR_NO						0
#define JVERR_UNKNOWN					-1
#define JVERR_DEVICE_BUSY				-2
#define JVERR_FUNC_NOT_SUPPORT		-3
#define JVERR_BADPARAM					-4
#define JVERR_NO_FREE_RESOURCE		-5
#define JVERR_NO_FREE_MEMORY			-6
#define JVERR_NO_ASSIGNED_RESOURCE	-7
#define JVERR_OPERATION_REFUSED		-8
#define JVERR_ALREADY_OPENED			-9
#define JVERR_ALREADY_CLOSED			-10
#define JVERR_TIMEOUT					-11
//#define MD_GRID_SET

/////////////////////////////////////功能宏定义///////////////////////////////////////
//可以打印文件名和行号,lck20100417
#ifndef Printf
#define Printf(fmt...)  \
do{\
	if(IPCAMDEBUG){	\
		printf("[%s]:%d ", __FILE__, __LINE__);\
		printf(fmt);} \
} while(0)
#endif

#define __FLAG__() do{printf("==FLAG== %s: %d\n", __FILE__, __LINE__);}while(0)

#ifndef CPrintf
#define CPrintf(fmt...)  \
do{\
	if(1){	\
		printf("\33[31m");\
		printf(fmt);} \
		printf("\33[0m ");\
} while(0)
#endif

#define CHECK_RET(express)\
		do{\
			S32 s32Ret = express;\
			if (0 != s32Ret)\
			{\
				printf("FUNC:%s,LINE:%d,failed! %x\n", __FUNCTION__, __LINE__, s32Ret);\
			}\
		}while(0)

#define DEBUG_LEVEL 1

#define Debug(level, fmt...) \
	do{ \
		if (DEBUG_LEVEL >= level) {\
		printf("[%s]:%d ", __FILE__, __LINE__);\
		printf(fmt);} \
	} while(0)


#define XDEBUG(express,name) CHECK_RET(express)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))
#endif


#define STRNCPY(dst, src, n)	snprintf(dst, n, "%s", src)

//将val变为min,max范围内的值
#define VALIDVALUE(val, min, max)		(val<min)?min:((val>max)?max:val)

//判断当前时间是否在时间段内,时间值为当天的第N秒
#define isInTimeRange(nTimeNow, nBeginTime, nEndTime) \
    ((nBeginTime < nEndTime && (nTimeNow >= nBeginTime && nTimeNow <= nEndTime)) ||\
     (nBeginTime >= nEndTime && (nTimeNow >= nBeginTime || nTimeNow <= nEndTime)) )

//对齐，align必须是2的N次方。
//向上对齐，例如：JV_ALIGN_CEILING(5,4) = 8
#define JV_ALIGN_CEILING(x,align)     ( ((x) + ((align) - 1) ) & ( ~((align) - 1) ) )
//向下对齐，例如：JV_ALIGN_FLOOR(5,4) = 4
#define JV_ALIGN_FLOOR(x,align)       (  (x) & (~((align) - 1) ) )


#define jv_strncpy(d, s, n) \
	do{\
		strncpy(d, s, n); \
		((unsigned char *)(d))[n-1] = '\0'; \
	}while(0)

#define jv_ystNum_parse(str,nDeviceInfo,ystID) \
	do{\
		char YSTG[4]; \
		memcpy(YSTG,&nDeviceInfo,4); \
		if(YSTG[1] == 0x0) \
			sprintf(str, "%c%d",YSTG[0], ystID); \
		else if(YSTG[2] == 0x0) \
			sprintf(str, "%c%c%d",YSTG[0],YSTG[1], ystID); \
		else if(YSTG[3] == 0x0) \
			sprintf(str, "%c%c%c%d",YSTG[0],YSTG[1],YSTG[2], ystID); \
		else \
			sprintf(str, "%c%c%c%c%d",YSTG[0],YSTG[1],YSTG[2],YSTG[3], ystID); \
	}while(0)
	 
///////////////////////////////////////类型定义///////////////////////////////////////
typedef unsigned char		U8;
typedef unsigned short		U16;
typedef unsigned int		U32;

typedef char			S8;
typedef short				S16;
typedef int					S32;

typedef unsigned long long	U64;
typedef long long			S64;

#ifdef OK
#undef OK
#endif
#define OK 0

#ifdef ERROR
#undef ERROR
#endif
#define ERROR -1

#define SUCCESS				0
#define FAILURE				(-1)

///////////////////////////////////////常量定义///////////////////////////////////////

#define HWINFO_STREAM_CNT hwinfo.streamCnt	//实际码流个数

#define	MAX_STREAM			3	///< 第三路是手机监控
#define MAX_MDRGN_NUM		4		//移动检测区域个数Motion Detect
#define	MAX_MDCHN_NUM		1

#define	STARTCODE			0x1153564A		//帧startcode

///////////////////////////////////////数据结构///////////////////////////////////////
//中维录像文件文件头
typedef struct tagJVS_FILE_HEADER
{
	U32	nFlag;				//录像文件标识
	U32	nFrameWidth;		//图像宽度
	U32	nFrameHeight;		//图像高度
	U32	nTotalFrames;		//总帧数
	U32	nVideoFormat;		//视频制式:0,PAL 1,NTSC
	U32	bThrowFrame_CARD;	//是否抽帧，4字节
	U32	u32IIndexAddr;		//是否抽帧，4字节
	//U32 u32Reserved;		//保留,4字节
	U32 audioType;//jv_audio_enc_type_e
}JVS_FILE_HEADER,*PJVS_FILE_HEADER;

//中维录像文件帧头
typedef struct tagJVS_FRAME_HEADER
{
	U32	nStartCode;
	U32	nFrameType:4;	//低地址
	U32	nFrameLens:20;
	U32	nTickCount:8;	//高地址
	//HI_U64	u64PTS;		//时间戳
}JVS_FRAME_HEADER, *PJVS_FRAME_HEADER;

typedef struct tagRECT
{
	U32		x;
	U32		y;
	U32		w;
	U32		h;
}RECT, *PRECT;

//中维看门狗
typedef struct tagJDOG
{
    int 	nCount;			//交互计数
    int 	nInitOK;		//JDVR是否成功初始化
    int 	nQuit;			//是否主动退出
}JDOG, *PJDOG;

typedef struct 
{
	BOOL running;
	pthread_t thread;
	int iMqHandle;
	sem_t sem;
	pthread_mutex_t mutex;
}jv_thread_group_t;

#define jv_assert(x,express) do{\
		if (!(x))\
		{\
			printf("%s:%d ==>jv_assert failed\n", __FILE__, __LINE__);\
			express;\
		}\
	}while(0)

		
//音频帧类型，与NvcAudioEnc_e一样
typedef enum{
	AUDIO_TYPE_UNKNOWN,
	AUDIO_TYPE_PCM,
	AUDIO_TYPE_G711_A,
	AUDIO_TYPE_G711_U,
	AUDIO_TYPE_AAC,
	AUDIO_TYPE_G726_40
}AUDIO_TYPE_E;

typedef enum{
	VIDEO_TYPE_UNKNOWN=0,
	VIDEO_TYPE_H264=1,
	VIDEO_TYPE_H265,
}VIDEO_TYPE_E;
	
extern int MAX_FRAME_RATE;

extern int VI_WIDTH;
extern int VI_HEIGHT;

//函数返回值
#define RET_SUCC					0
#define RET_ERR						-1
#define RET_DEVICE_BUSY				-2
#define RET_FUNC_NOT_SUPPORT		-3
#define RET_BADPARAM				-4
#define RET_NO_FREE_RESOURCE		-5
#define RET_NO_FREE_MEMORY			-6
#define RET_NO_ASSIGNED_RESOURCE	-7
#define RET_OPERATION_REFUSED		-8

#define ENCRYPT_100W 0x5010
#define ENCRYPT_130W 0x5013
#define ENCRYPT_200W 0x5020
#define ENCRYPT_300W 0x5030
#define ENCRYPT_500W 0x5050

typedef enum{
	SENSOR_UNKNOWN,
	SENSOR_OV9712      ,
	SENSOR_OV9732      ,
	SENSOR_AR0130      ,
	SENSOR_AR0330      ,
	SENSOR_OV2710      ,
	SENSOR_OV9750      ,
	SENSOR_IMX178      ,
	SENSOR_OV4689      ,    
	SENSOR_AR0230      ,
	SENSOR_IMX290      ,
	SENSOR_IMX123      ,
	SENSOR_OV5658      ,
	SENSOR_IMX225      ,
	SENSOR_BG0701      ,
	SENSOR_BT601       ,	//动力机芯模拟sensor型号
	SENSOR_AR0237	   ,   //AR0237MIPI
	SENSOR_AR0237DC	   ,   //AR0237DC
	SENSOR_OV9750m	   ,  //mipi接口的ov9750
	SENSOR_SC2045      ,
	SENSOR_MN34227     ,
	SENSOR_SC2135      ,
	SENSOR_OV2735	   ,
	SENSOR_IMX291      ,//50
	SENSOR_SC2235,
	SENSOR_MAX
}SensorType_e;

/**
 * @brief 本类型定义IRCUT切换的判断模式
 */
typedef enum
{
	IRCUT_SW_BY_GPIO=0,		//目前枪机模组的传统方式，用GPIO检测切换临界点
	IRCUT_SW_BY_AE,  		//变焦一体机，默认用AE切换IRCUT
	IRCUT_SW_BY_UART,  		//变焦一体机,通过串口接收IRCUT切换信号
	IRCUT_SW_BY_ADC0,		//ADC通道0，枪机、云台、最好是通过ADC采集光敏电阻压降来实现IRCUT切换
	IRCUT_SW_BY_ADC1,		//ADC通道1
	IRCUT_SW_BY_ADC2,		//ADC通道2
	IRCUT_SW_BY_ISP ,      //软硬结合或者纯软光敏
}ENUM_IRSW_MODE;

typedef enum{
	JVSENSOR_ROTATE_NONE = 0,
	JVSENSOR_ROTATE_90   = 1,
	JVSENSOR_ROTATE_180  = 2,
	JVSENSOR_ROTATE_270  = 3,
	JVSENSOR_ROTATE_MAX
}JVRotate_e;

typedef enum{
	IPCTYPE_JOVISION, //中维机
	IPCTYPE_SW, //尚维机
	IPCTYPE_JOVISION_GONGCHENGJI,//中维工程机
	IPCTYPE_IPDOME,//IPC高速球
	IPCTYPE_MAX
}IPCType_e;

typedef struct{
	JVRotate_e rotate;
	BOOL bEnableWdr;
	BOOL bSupportWdr;
	IPCType_e ipcType;
	U32 ipcGroup;
}JVCommonParam_t;

typedef enum
{
	ALARM_DISKERROR=0,
	ALARM_DISKFULL,
	ALARM_DISCONN,
	ALARM_UPGRADE,
	ALARM_GPIN,
	ALARM_VIDEOLOSE=5,		// 视频丢失
	ALARM_HIDEDETECT,		// 视频遮挡
	ALARM_MOTIONDETECT,		// 移动侦测
	ALARM_POWER_OFF,
	ALARM_MANUALALARM,
	ALARM_GPS=10,
	ALARM_DOOR,				// 433报警
	ALARM_IVP,
	ALARM_BABYCRY,
	ALARM_PIR = 15,			// PIR
	ALARM_ONE_MIN = 17,		// 一分钟录像推送报警
	ALARM_SHOT_PUSH,		// 拍照推送
	ALARM_REC_PUSH,			// 录像推送
	ALARM_CHAT_START=20, 	//开启视频对讲
	ALARM_CHAT_STOP,		//关闭视频对讲
	ALARM_CAT_HIDEDETECT=22,		// 视频遮挡,猫眼共用
	ALARM_TOTAL
}ALARM_TYPEs;

typedef enum
{
	ALARM_OBSS_MOTIONDETECT = '0',
	ALARM_OBSS_ELECEYE = '1',
	ALARM_OBSS_THIRD = '2'
	
}ALARM_OBSS_TYPES;

enum
{
	AUDIO_MODE_ONE_WAY	= 1 << 0,		// 单向
	AUDIO_MODE_TWO_WAY	= 1 << 1,		// 双向
	AUDIO_MODE_NO_WAY	= 1 << 2,		// 无音频^_^
};

typedef struct
{
	struct{
		BOOL ptzBsupport;//是否支持PTZ
		int ptzprotocol;
		int ptzbaudrate;
	};
	BOOL wdrBsupport;	//是否支持宽动态
	BOOL rotateBSupport; //是否支持旋转
	char product[32]; // /etc/jovision.cfg中保存的，用于升级的product值
	char type[32];
	unsigned int encryptCode;
	SensorType_e sensor;
	IPCType_e ipcType;

	//码流相关
	struct{
		int streamCnt;	//编码个数。原来都是3路。现在有些方案，只有2路
	};
	BOOL bHomeIPC; //add for homeipc
	BOOL bSoftPTZ;	//是否支持软云台
	BOOL bSupport3DLocate;	//是否支持3D定位
	BOOL bEMMC;
	HW_TYPE_e HwType;	// 硬件平台
	char devName[32];	//IPC设备名称
	BOOL remoteAudioMode;	// AUDIO_MODE_XXX
	BOOL bNewVoiceDec;		//是否使能新的声波配置
	BOOL bXWNewServer;		//是否使能新的小维APP服务器
	struct{
		BOOL bMirror;	//是否是水平翻转
		BOOL bFlip; //是否是垂直翻转
	};
	BOOL bSupportLDC;		//是否支持畸变校正功能
	BOOL bSupportXWCloud;	//是否支持小维云存储
	BOOL bSupportAESens;	//是否支持软光敏灵敏度调节
	BOOL bSupportVoiceConf;	//是否支持声波配置
	BOOL bSupportPatrol;	//是否支持云台预置点和巡航
	BOOL bInternational;	//是否为小维国际版
	BOOL bCloudSee;			//是否为cloudsee版
	BOOL bSupportAVBR;		//是否支持AVBR
	BOOL bSupportSmtVBR;	//是否支持智能VBR，在静止画面时降低图像质量，降低码率
	BOOL bSupportMCU433;	//是否支持单片机433功能
	BOOL bVgsDrawLine;		//是否使用VGS画线，如需画线，则VB使用需要相应增加
	BOOL bSupportMVA;		//是否支持小维智能化
	BOOL bSupportReginLine;		//是否支持区域入侵和绊线
	BOOL bSupportLineCount;		//是否支持过线计数
	BOOL bSupportHideDetect;	//是否支持遮挡检测
	BOOL bSupportClimbDetect;	//是否支持爬高侦测
	BOOL bSupportRedWhiteCtrl;  //是否支持全彩模式控制
	struct
	{
		ENUM_IRSW_MODE	ir_sw_mode;				//IR切换的模式
		unsigned char 	ir_sensitive;			//IR切换的灵敏度,用AE判断或者ADC判断时一般有用。
		int				ir_power_holdtime;		//IR通电保持时间，单位:us
	};
}hwinfo_t;

extern hwinfo_t hwinfo;

// 将GPARAM定义放到jv_common.h中，便于在porting中访问gp
typedef struct tm		TM;
typedef struct
{
	U32		nYTSpeed;			// 云台转速，0~255，可以高于255，但需要测试效果
	U32		nYTCheckTimes;		// 云台自检圈数，1: 转半圈，2: 转一圈
	U32		nInterval;			// 云台自检间隔时间
	BOOL	bYTOriginReset;		// 云台自检后是否回正，1: 回正，0: 不回正
	U32		nYTLRSteps;			// 云台左右总步数
	U32		nYTUDSteps;			// 云台上下总步数
	U32		nYTLREndStep;		// 云台左右回正位置
	U32		nYTUDEndStep;		// 云台上下回正位置
}FactoryTestCfg;

typedef struct tagGPARAM
{
	//主线程
	BOOL	bRunning;
	time_t	ttNow;
	TM		*ptmNow;

	//是否显示OSD
	//BOOL	bShowOSD;
	//日期时间格式
	U32		nTimeFormat;

	//对话框版本号
	U32		nIPCamCfg;

	//退出标志
	U32		nExit;

	// 工厂检测标志
	BOOL	bFactoryFlag;
	char	FactoryFlagPath[256];
	FactoryTestCfg TestCfg;

	struct{
		BOOL bTalking;
		int	nClientID;			// 云视通\流媒体方式的ID
		int nLocalChannel;
	} ChatInfo;

	//是否启动云视通
	BOOL bNeedYST;
	//是否启动Zrtsp
	BOOL bNeedZrtsp;
	//是否启动web
	BOOL bNeedWeb;
	//是否启动onvif
	BOOL bNeedOnvif;
}GPARAM, *PGPRAM;

extern GPARAM gp;

typedef struct{
	int group;
	int bit;
	int onlv;
}GpioValue_t;

typedef struct{
	GpioValue_t resetkey; //Reset按键
	GpioValue_t statusledR; //状态指示灯R
	GpioValue_t statusledG; //状态指示灯G
	GpioValue_t statusledB; //状态指示灯B
	GpioValue_t cutcheck; //夜视检测
	GpioValue_t cutday; //切白天
	GpioValue_t cutnight; //切晚上
	GpioValue_t redlight; //红外灯开关
	GpioValue_t alarmin1; //报警输入1
	GpioValue_t alarmin2; //报警输入2
	GpioValue_t alarmout1; //报警输出1
	GpioValue_t audioOutMute;
	GpioValue_t wifisetkey;		//wifi配置键
	GpioValue_t cutcheck_ext; //外扩夜视检测
	GpioValue_t redlight_ext; //外扩红外点阵灯
	GpioValue_t ledlight_in;	//led灯板插入检测
	GpioValue_t muxlight_in;	//点阵灯板插入检测
	GpioValue_t sensorreset; //sensor复位
	GpioValue_t pir; // PIR检测
	GpioValue_t oneminrec;		//一分钟录像按键
	GpioValue_t whitelight;		//白光LED灯
}higpio_values_t;

extern higpio_values_t higpios;

/**
 *@brief 平台相关的总初始化
 *
 */
int jv_common_init(JVCommonParam_t *param);

/**
 *@brief 平台相关的结束
 *
 */
int jv_common_deinit(void);

// 完全退出平台，用于退出媒体业务后，彻底退出平台
void jv_common_deinit_platform(void);


/*
 *@brief DEBUG用的yst库
 */
unsigned int jv_yst(unsigned int *a,unsigned int b);

/*
 *@brief 使用VI的mirror和flip功能
 */
 void VI_Mirror_Flip(BOOL bMirror, BOOL bFlip);

 //测试用函数。添加线程信息
extern void pthreadinfo_add(const char *threadName);
 
#define IsSystemFail(ret) (-1 == (ret) || 0 == WIFEXITED((ret)) || 0 != WEXITSTATUS((ret)))
extern int utl_system_ex(char* strCmd, char* strResult, int nSize);
extern int utl_system(char *cmd, ...);

#define LINUX_THREAD_STACK_SIZE (512*1024)

extern int pthread_create_detached(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg);
extern int pthread_create_normal(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg);
extern int pthread_create_normal_priority(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg);

#endif

