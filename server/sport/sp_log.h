/*
 * sp_log.h
 *
 *  Created on: 2014-12-01
 *      Author: Qin Lichao
 */

#ifndef SP_LOG_H_
#define SP_LOG_H_

#define SPLOG_VERSION			0x10    //版本号
#define SPLOG_MAX_RECORD		1000    //最大记录
#define SPLOG_RECORD_LEN		64      //LOG_ITEM大小
#define SPLOG_STR_LEN			59      //记录长度
#define SPLOGS_NEED_SAVE		1		//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned int nVersion;			//版本号
	unsigned int nRecordTotal;		//记录总数
	unsigned int nItemSize;			//记录长度
	unsigned int nCurRecord;		//当前记录
	int nSavedRecord;		//已经保存的记录
	char nReserve[16];		//保留
}SPLOG_HEAD;

typedef struct
{
	unsigned int	nTime;					//时间
	char	strLog[SPLOG_STR_LEN+1];	//事件
}SPLOG_ITEM;

typedef struct
{
	SPLOG_HEAD		head;					//日志头
	SPLOG_ITEM		item[SPLOG_MAX_RECORD];	//日志内容
	int				fd;
}SPLOG;


/**
 * @brief 获取日志
 *
 * @param date 日志日期
 * @return 0成功 -1失败
 */
int sp_log_get(SPLOG *log);

/**
 * @brief 清空日志
 *
 * @return 0成功 -1失败
 */
int sp_log_clear(void);

#ifdef __cplusplus
}
#endif


#endif /* SP_LOG_H_ */
