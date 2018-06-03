

#include "jv_common.h"
#include <utl_filecfg.h>
#include "mipcinfo.h"
#include "mlog.h"

//读取log日志文件，如果不存在则创建，并且往内存和文件中写入第一条"creat"记录,如果成功，返回日志内存地址，否则返回0
LOG stLog;
LOG * mlog_init(char *strLogFile)
{
	stLog.fd=open(strLogFile, O_RDWR|O_CREAT, 0777);
	if(stLog.fd < 0)
	{
		printf("Load logs failed\n");
		return NULL;
	}

	if(lseek(stLog.fd, 0, SEEK_END))	//存在log文件
	{
		lseek(stLog.fd, 0, SEEK_SET);
		read(stLog.fd, &stLog.head, sizeof(LOG_HEAD));
		read(stLog.fd, stLog.item, sizeof(stLog.item));
		Printf("Load log file\n");
	}
	else				//否则在内存中构造第一条记录，并写入log文件
	{
		Printf("New log file created.\n");
		stLog.head.nVersion		= LOG_VERSION;
		stLog.head.nRecordTotal	= 1;
		stLog.head.nItemSize	= LOG_RECORD_LEN;
		stLog.head.nCurRecord	= 0;
		stLog.head.nSavedRecord	= 0;
		write(stLog.fd, &stLog.head, sizeof(LOG_HEAD));

		//创建文件时写入第一条日志
		stLog.item[0].nTime = time(NULL);
		sprintf(stLog.item[0].strLog, "%s", mlog_translate("No Log File, Create It."));
		write(stLog.fd, &stLog.item[0], sizeof(LOG_ITEM));
		Printf("Create-");
	}
	Printf("Ver=%x, Total=%d, nCur=%d, nSaved=%d\n", stLog.head.nVersion,
		stLog.head.nRecordTotal, stLog.head.nCurRecord, stLog.head.nSavedRecord);

	pthread_mutex_init(&stLog.mutexLog, NULL);

	return &stLog;
}

VOID mlog_deinit()
{
	if(stLog.fd>0)
	{
		close(stLog.fd);
	}
	pthread_mutex_destroy(&stLog.mutexLog);
}
static BOOL s_enable = TRUE;
void mlog_enable(BOOL bEnable)
{
	s_enable = bEnable;
}

//向内存中记录日志，ptr是将要记录的日志
VOID mlog_write(char *strLog, ...)
{
	va_list list;
	U32 nIndex, nSaved;

	if (!s_enable)
		return ;
	pthread_mutex_lock(&stLog.mutexLog);
	strLog = mlog_translate(strLog);

	va_start(list, strLog);

	//如果存满1000个日志，回到首地址，覆盖较久的日志
	if(++stLog.head.nCurRecord >= LOG_MAX_RECORD)
	{
		stLog.head.nCurRecord = 0;
	}
	nIndex	= stLog.head.nCurRecord;
	stLog.item[nIndex].nTime = time(NULL);
	vsnprintf(stLog.item[nIndex].strLog, sizeof(stLog.item[nIndex].strLog), strLog, list);
	va_end(list);

	//日志总条数累加，最大到LOG_MAX_RECORD条
	if(++stLog.head.nRecordTotal >=  LOG_MAX_RECORD)
	{
		stLog.head.nRecordTotal=LOG_MAX_RECORD;
	}

	nSaved	= stLog.head.nSavedRecord;
	if(nIndex%LOGS_NEED_SAVE == 0)
	{
		stLog.head.nSavedRecord=nIndex;
		//写入日志文件头信息,lck20120112
		lseek(stLog.fd, 0, SEEK_SET);
		write(stLog.fd, &stLog.head, sizeof(LOG_HEAD));
		Printf("nTotal=%d\n", stLog.head.nRecordTotal);
		//如果需要保存的日志达到文件尾并且需要覆盖较久的日志
		if(nIndex < nSaved)
		{
			//写入日志
			write(stLog.fd, stLog.item, sizeof(stLog.item));
			Printf("Save logs\n");
		}
		else	//需要保存的日志不需要覆盖文件头部的日志
		{
			//写入所有未保存的日志记录
			lseek(stLog.fd, sizeof(LOG_HEAD)+(sizeof(LOG_ITEM)*(nSaved+1)), SEEK_SET);
			write(stLog.fd, &stLog.item[nIndex], sizeof(LOG_ITEM)*(nIndex-nSaved));
			Printf("Save items %s\n", stLog.item[nIndex].strLog);
		}
	}

	//写入日志后打印调试信息,lck20120112
	Printf("Ver=%x, Total=%d, nCur=%d, nSaved=%d\n", stLog.head.nVersion,
		stLog.head.nRecordTotal, stLog.head.nCurRecord, stLog.head.nSavedRecord);

	pthread_mutex_unlock(&stLog.mutexLog);
}

/**
 *@brief 获取当前语言对应的字符串
 *
 *@param pStr 英文字符串
 *
 *@return 返回将英文翻译后的字符串
 *
 */
char* mlog_translate(char *pStr)
{
	char *ret;

	if (ipcinfo_get_param(NULL)->nLanguage == LANGUAGE_CN)
	{
		ret = utl_fcfg_get_value("/progs/language/zh-cn.lan", pStr);
		if (ret)
			return ret;
	}
	return pStr;
}

