/*
 * sp_devinfo.h
 *
 *  Created on: 2013-11-1
 *      Author: lfx
 */
#ifndef SP_DEVINFO_H_
#define SP_DEVINFO_H_

typedef enum {
	SP_REBOOT_TIMER_NEVER,
	SP_REBOOT_TIMER_EVERYDAY,
	SP_REBOOT_TIMER_EVERYSUNDAY,
	SP_REBOOT_TIMER_EVERYMOUNDAY,
	SP_REBOOT_TIMER_EVERYTUNESDAY,
	SP_REBOOT_TIMER_EVERYWEDNESDAY,
	SP_REBOOT_TIMER_EVERYTHURSDAY,
	SP_REBOOT_TIMER_EVERYFRIDAY,
	SP_REBOOT_TIMER_EVERYSATURDAY,
}SPRebootTimer_e;

typedef struct{
	int  bDiscoverable;
	char hostname[32];
	char name[48];		//机器名称
	SPRebootTimer_e rebootDay;
	int rebootHour; //取值0～23表示0点到23点
}SPDevInfo_t;

typedef struct{
	char type[32]; //类型 DEVICE,IPC,NVR,DVR
	char hardware[32];	//硬件名称
	char sn[32];		//SN
	char firmware[64];	//软件版本
	char manufacture[32]; //生产厂商
	char model[32];

	int ptzBsupport;//是否支持PTZ
	int streamCnt;	//编码个数。原来都是3路。现在有些方案，只有2路
	char ystID[32];
}SPHWInfo_t;

#ifdef __cplusplus
extern "C" {
#endif

const SPHWInfo_t *sp_dev_get_hwinfo();

int sp_dev_get_info(SPDevInfo_t *info);

int sp_dev_set_info(SPDevInfo_t *info);


/**
 *@brief 设置当前时间
 *
 *@param tmsec 当前时间，距离1970年以来的秒数
 */
int sp_dev_stime(time_t *tmsec);

/**
 *@brief 获取当前时间
 *
 *@return 当前时间，距离1970年以来的秒数
 */
time_t sp_dev_gtime();

typedef struct {
	BOOL bNtp;
	char ntpServer[256];
	int sntpInterval;
}SPNtp_t;

int sp_dev_ntp_set(SPNtp_t *ntp);

int sp_dev_ntp_get(SPNtp_t *ntp);

int sp_dev_timezone_set(int tz);

int sp_dev_timezone_get(void);

int sp_dev_discovermode_get(void);
int sp_dev_discovermode_set(int bEnable);

int sp_dev_hostname_set(const char* hostname,int len);

/**
 *@brief 几毫秒后重启
 *
 */
int sp_dev_reboot(int delaymSec);

int sp_dev_factory_default(BOOL bHard);

int sp_dev_LetCtrl(int mode,int link);

#ifdef __cplusplus
}
#endif


#endif /* SP_DEVINFO_H_ */
