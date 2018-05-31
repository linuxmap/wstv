
/*	mrecord.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织录像相关函数
	更改历史详见svn版本库日志
*/

#ifndef __MRECORD_H__
#define __MRECORD_H__

#include "jv_common.h"

#define	FILE_FLAG		".mp4"

#define MAX_REC_TASK	2
#define RECORD_CHN		record_chn	//录像通道
#define RECORD_CLOUD_CHN	1

extern int record_chn;

//录像类型枚举
typedef enum tagRECTYPE
{
	REC_STOP	= 0,	//录像已停止
	REC_NORMAL	= 'N',	//正常录像 78
	REC_DISCON  = 'D',	//无连接时录像. IPC在无连接时录像
	REC_TIME	= 'T',	//定时录像 84
	REC_MOTION	= 'M',	//移动检测录像 77
	REC_ALARM	= 'A',	//报警录像 65
	REC_ONE_MIN	= 'O',	//一分钟录像79
	REC_CHFRAME	= 'C',  //抽帧录像
	REC_PUSH	= 'P',	//推送录像
	REC_IVP	= 'I',		//智能分析录像

} RECTYPE;

typedef enum 
{
	ALARM_TYPE_NONE,
	ALARM_TYPE_MOTION,
	ALARM_TYPE_ALARM,
	ALARM_TYPE_IVP,
	ALARM_TYPE_IVP_VR,
	ALARM_TYPE_IVP_HIDE,
	ALARM_TYPE_IVP_LEFT,
	ALARM_TYPE_IVP_REMOVED,
	ALARM_TYPE_BABYCRY,
	ALARM_TYPE_PIR,
	ALARM_TYPE_WIRELESS,
	ALARM_TYPE_BUTT,
}alarm_type_e;

typedef enum
{
	RECORD_MODE_STOP = 0,		// 停止录像
	RECORD_MODE_NORMAL,			// 普通录像
	RECORD_MODE_ALARM,			// 报警录像
	RECORD_MODE_CHFRAME,		// 抽帧录像
	RECORD_MODE_TIMING,			// 定时录像
	RECORD_MODE_DISCONN,		// 无连接录像

	RECORD_MODE_BUTT
}RecordMode_e;

// 录像请求类型，数值越大，优先级越高
typedef enum
{
	RECORD_REQ_NONE,			// 无请求
	RECORD_REQ_ALARM,			// 报警录像
	RECORD_REQ_PUSH,			// 录像推送
	RECORD_REQ_ONEMIN,			// 一分钟录像
}REC_REQ;

typedef void (*FuncRecFinish)();

typedef struct
{
	REC_REQ				ReqType;
	union
	{
		int				nDuration;
		alarm_type_e	AlarmType;
	};
	FuncRecFinish		pCallback;
}REC_REQ_PARAM;


/**
 *@brief 录制相关的属性。
 *
 *
 */
typedef struct
{
	BOOL		bEnable;			///< 是否启用手工录制
	unsigned int	file_length;			///< 录像文件的单个的长度，以秒为单位

	///定时相关
	BOOL		timing_enable;		///< 是否启用定时录制

	BOOL discon_enable; ///< 是否启用无连接录像
	BOOL alarm_enable; ///< 是否启用报警录像

	/**
	 * 定时录制的开始时间和结束时间，单位为秒，其取值在0 ~ 24*60*60 之间
	 * 当timing_stop > timing_start时，定时范围就是当天的timing_start到timing_stop
	 * 当timing_stop < timing_start时，定时范围为当天的timing_start到第二天的timing_stop
	 * 
	 */
	unsigned int	timing_start;		
	unsigned int	timing_stop;

	BOOL		disconnected;		///< 无连接时录制
	BOOL		detecting;			///< 移动检测录制
	BOOL		alarming;			///< 外部报警录制
	BOOL		ivping;			///< 智能分析录制
	unsigned int alarm_pre_record;		///<预录制的时间，最大为10秒
	unsigned int alarm_duration;		///<报警发生后，录制的持续时间
	BOOL		chFrame_enable;			//抽帧录像的使能开关
	unsigned int chFrameSec;				//抽帧录像间隔
}mrecord_attr_t;

typedef struct
{
	char	type;		// RECTYPE
	char	part;		// 分区编号
	int		date;		// 20170101
	int		fileTime;	// 235959
	int		stime;		// 235959
	int		etime;		// 235959
}mrecord_item_t;

/**
 *@brief 初始化
 *
 *@return 0
 */
int mrecord_init(void);

/**
 *@brief 结束
 *
 *@return 0
 */
int mrecord_deinit(void);

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param attr 属性指针
 *
 *@return 0 或者是错误号
 */
int mrecord_set_param(int channelid, mrecord_attr_t *attr);

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param attr 属性指针
 *
 *@return 0 或者是错误号
 */
int mrecord_get_param(int channelid, mrecord_attr_t *attr);

/**
 *@brief 强制停止录像
 *
 *	功能主要针对当分辨率发生变化时，重启录像功能
 *@param channelid 通道号
 *
 *@return 0 或者是错误号
 */
int mrecord_stop(int channelid);

void mrecord_onestop_sound();


/**
 *@brief 刷新某通道的设置，使之生效。
 *
 *	功能主要针对定时器
 *@param channelid 通道号
 *
 *@return 0 或者是错误号
 */
int mrecord_flush(int channelid);

/**
 *@brief 保存一帧数据
 *@param channelid 通道号
 *@param buffer 数据所在的内存指针
 *@param len 数据长度
 *@param frametype 数据类型，参考#JVS_TYPE_P 等
 *
 */
int mrecord_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timestamp);

/**
 *@brief 发生外部报警或者是移动检测报警
 *@param channelid 通道号
 *@param type 报警类型
 *@param param 附加参数
 *@return 0 或者错误号
 *
 */
int mrecord_alarming(int channelid, alarm_type_e type, void *param);

/**
 *@brief 获取报警关联的录像文件及图片名称
 *@param[IN]  type
 *@param[OUT] alarmInfo 报警信息，包含报警关联的录像文件及图片名称
 *@return 0 或者错误号return -1 没有SD卡
 */
int mrecord_alarm_get_attachfile(RECTYPE type, void *alarmInfo);

const char* mrecord_get_now_recfile();

int mrecord_one_min_rec(int channelid);

/*
 * 干掉云存储。主要用来释放预录制占用的资源（云存储使用的）
 * 目前只用在云存储设备，进行文件升级的时候，下载升级软件之前，释放资源防止死机
 */
void mrecord_cloud_destroy(int channelid);

/*
云存储上传第二码流
*/
void mrecord_cloud_write(int channelid, unsigned char *buffer, int len, unsigned int frametype, unsigned long long timeStamp);

int mrecord_get_recmode(int* nChFrameSec);

int mrecord_set_recmode(int Type, int nChFrameSec);


/*
 * 增加录像请求机制，用于处理正常录像以外的临时录像请求，如报警录像、一键录像、推送录像等
 */
int mrecord_request_start_record(int channelid, const REC_REQ_PARAM* ReqParam);

int mrecord_request_stop_record(int channelid, REC_REQ Request);


void MP4_GetIndexFile(char *strFile, char *strIndex);
/**
 *@brief 设置参数的简化写法
 *
 */
#define mrecord_set(channelid, key,value)\
do{\
	mrecord_attr_t attr;\
	mrecord_get_param(channelid, &attr);\
	attr.key = value;\
	mrecord_set_param(channelid, &attr);\
}while(0)

void mrecord_pre_reinit();

int mrecord_search_file(int startdate, int starttime, int enddate, int endtime, mrecord_item_t* pResult, int maxcnt);

const char* mrecord_get_filename(const mrecord_item_t* pResult, char* name, int len);

#endif

