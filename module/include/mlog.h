

#ifndef __MLOG_H__
#define __MLOG_H__

#define LOG_VERSION			0x10    //版本号
#define LOG_MAX_RECORD		1000    //最大记录
#define LOG_RECORD_LEN		64      //LOG_ITEM大小
#define LOG_STR_LEN			59      //记录长度
#define LOGS_NEED_SAVE		1		//

typedef struct
{
	unsigned int nVersion;			//版本号
	unsigned int nRecordTotal;		//记录总数
	unsigned int nItemSize;			//记录长度
	unsigned int nCurRecord;		//当前记录
	int nSavedRecord;		//已经保存的记录
	char nReserve[16];		//保留
}LOG_HEAD;

typedef struct
{
	unsigned int	nTime;					//时间
	char	strLog[LOG_STR_LEN+1];	//事件
}LOG_ITEM;

typedef struct
{
	LOG_HEAD		head;					//日志头
	LOG_ITEM		item[LOG_MAX_RECORD];	//日志内容
	int				fd;

	pthread_mutex_t mutexLog;				//互斥琐
}LOG;

extern LOG stLog;

LOG *mlog_init(char *strLogFile);
void mlog_deinit();

/**
 *@使能或者禁止LOG
 *
 *
 */
void mlog_enable(BOOL bEnable);

void mlog_write(char *strLog, ...);

/**
 *@brief 获取当前语言对应的字符串
 *
 *@param pStr 英文字符串
 *
 *@return 返回将英文翻译后的字符串
 *
 */
extern char* mlog_translate(char *pStr);

#endif

