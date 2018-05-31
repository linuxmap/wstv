#ifndef _MIPCINFO_H_
#define _MIPCINFO_H_


/*
Locale Description // Short String //  Hex Value //  Decimal Value
Chinese-China // zh-cn // 0x0804 // 2052
Chinese-Taiwan // zh-tw // 0x0404 // 1028
English-United States // en-us // 0x0409 // 1033


*/

//#define LANGUAGE_CHINESE	0x0804
//#define LANGUAGE_ENGLISH	0x0409

//语言
#define LANGUAGE_CN			0
#define LANGUAGE_EN			1
#define LANGUAGE_MAX		2
#define LANGUAGE_DEFAULT	LANGUAGE_CN
#define MAX_TUTK_UID_NUM	21
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	REBOOT_TIMER_NEVER,
	REBOOT_TIMER_EVERYDAY,
	REBOOT_TIMER_EVERYSUNDAY,
	REBOOT_TIMER_EVERYMOUNDAY,
	REBOOT_TIMER_EVERYTUNESDAY,
	REBOOT_TIMER_EVERYWEDNESDAY,
	REBOOT_TIMER_EVERYTHURSDAY,
	REBOOT_TIMER_EVERYFRIDAY,
	REBOOT_TIMER_EVERYSATURDAY,
}RebootTimer_e;

typedef enum {
	LED_CONTROL_AUTO,		// 智能 非夜视: 开，夜视: 关
	LED_CONTROL_ON,			// 常开
	LED_CONTROL_OFF,		// 常灭
}LedControl_e;

typedef struct
{
	char type[32];
	char product[32];
	char version[20];
	char acDevName[32];
	char nickName[36];
	unsigned int sn;
	unsigned int ystID;
	
	//GUID(4),SN(1),DATE(1),GROUP(1),CARDTYPE(1),MODEL(1),ENCYR_VER(1),YSTNUM(1),DEV_VER(1),USER(1)
	U32	nDeviceInfo[13];		
	int nLanguage;
   // int nPosition;
    
	char date[32];//format yyyy-MM-dd hh:mm:ss
	BOOL bSntp;	//自动网络校时
	char ntpServer[64];//自动对时的网络服务器
	int sntpInterval;//自动网络校时间隔时间，单位为小时
	int tz;	//时区  -12 ~ 13
	int bDST; //Daylight Saving Time 夏令时

	RebootTimer_e rebootDay;
	int rebootHour; //取值0～23表示0点到23点
	LedControl_e	LedControl;	// LED控制功能
	BOOL bRestriction; //TRUE:限制部分功能;FALSE:取消限制.
	BOOL bAlarmService;	//是否启用迅卫士联网报警,TRUE:启用,FALSE:不启用
	char portUsed[512];
	char osdText[8][64];
	unsigned int osdX;
	unsigned int osdY;
	unsigned int endX;
	unsigned int endY;
	unsigned int osdSize;
	unsigned int osdPosition;
	unsigned int osdAlignment;
	BOOL needFlushOsd;
	char lcmsServer[64];
	char tutkid[MAX_TUTK_UID_NUM];

	BOOL	bWebServer;
	U32		nWebPort;
}ipcinfo_t;

int ipcinfo_init(void);
int ipcinfo_get_port_used();

/** 
 *@brief 获取IPCINFO
 *@param param 输出 IPCINFO的参数
 *
 *@return param为NULL时，返回一个内部的指针
 *		param不是NULL时，返回param的值
 */
ipcinfo_t *ipcinfo_get_param(ipcinfo_t *param);

/**
 *@brief 获取加密芯片中的型号
 */
unsigned int ipcinfo_get_model_code();

/** 
 *@brief 设置IPCINFO
 *@param param 输入 IPCINFO的参数
 *
 *@note param->date不起作用，设置时间需要另外的函数
 *@return 0 if success
 */
int ipcinfo_set_param(ipcinfo_t *param);

/** 
 *@brief 设置时间
 *@param date 要设置的时间，其格式为YYYY-MM-DD hh:mm:ss
 *
 *@return 0 if success
 */
int ipcinfo_set_time(char *date);

int ipcinfo_set_time_ex(time_t *newtime);

/**
 *@brief 设置时区
 *@param timeZone 时区 东为正，西为负，范围 -12 ~ 13
 */
int ipcinfo_timezone_set(int timeZone);

/**
 *@brief 设置时区
 *@param timeZone 时区 东为正，西为负，范围 -12 ~ 13
 */
int ipcinfo_timezone_get();

IPCType_e ipcinfo_get_type();

int ipcinfo_get_type2();

int multiosd_process(ipcinfo_t *param);

/**
 *@brief 是否已经校过时
 *
 */
BOOL ipcinfo_TimeCalibrated();

/**
 *@brief 设置参数的简化写法
 *@param bString 是否为字符串
 *@param key
 *@param value
 *
 */

#define ipcinfo_set_value(key,value)\
	do{\
		ipcinfo_t attr;\
		ipcinfo_get_param(&attr);\
		attr.key = value;\
		ipcinfo_set_param(&attr);\
	}while(0)
	
#define ipcinfo_set_str(key,value)\
	do{\
		ipcinfo_t attr;\
		ipcinfo_get_param(&attr);\
		strcpy((char *)attr.key, (char *)value);	\
		ipcinfo_set_param(&attr);\
	}while(0)
			
	
#define ipcinfo_set_buf(key,value)\
	do{\
		ipcinfo_t attr;\
		ipcinfo_get_param(&attr);\
		memcpy(attr.key , value, sizeof(attr.key));\
		ipcinfo_set_param(&attr);\
	}while(0)
#ifdef __cplusplus
		}
#endif


#endif

