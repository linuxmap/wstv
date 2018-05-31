/*
 * sp_log.c
 *
 *  Created on: 2014-12-01
 *      Author: Qin Lichao
 */
#include "sp_define.h"
#include "mlog.h"
#include "sp_log.h"

/**
 * @brief 获取日志
 *
 * @param log 日志结构体
 * @return 0成功 -1失败
 */
int sp_log_get(SPLOG *log)
{
	int i = 0;
	log->fd = stLog.fd;
	log->head.nVersion = stLog.head.nVersion;
	log->head.nRecordTotal = stLog.head.nRecordTotal;
	log->head.nItemSize = stLog.head.nItemSize;
	log->head.nCurRecord = stLog.head.nCurRecord;
	log->head.nSavedRecord = stLog.head.nSavedRecord;
	for(i=0; i<log->head.nRecordTotal; i++)
	{
		log->item[i].nTime = stLog.item[i].nTime;
		strcpy(log->item[i].strLog, stLog.item[i].strLog);
	}
	return 0;
}

/**
 * @brief 清空日志
 *
 * @return 0成功 -1失败
 */
int sp_log_clear()
{
	return 0;
}

