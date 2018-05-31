
/*	SYSFuncs.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织系统相关代码
	更改历史详见svn版本库日志
*/

#ifndef __SYSFUNCS_H__
#define __SYSFUNCS_H__
#include "jv_common.h"


#define CONFIG_VER	1

typedef struct{
	int nLanguage;
	unsigned int nTimeFormat;
	unsigned int ystID;
}ImportantCfg_t;

#define IMPORTANT_CFG_FILE	CONFIG_PATH"/important.cfg"	//重要配置文件，包含了语言、云视通号等恢复系统时不丢失的信息
#define DEFAULT_FORCE_FILE	CONFIG_PATH"/default/force.cfg"	//指示文件。存在该文件，则强制使用system.cfg
#define DEFAULT_CFG_FILE	CONFIG_PATH"/default/system.cfg"	//默认文件。恢复系统时，使用本文件做初始化配置
#define RESTRICTION_FILE	CONFIG_PATH"bRestriction.cfg"	//限制功能是否打开，如果文件存在表示打开

U32 ParseParam(char *pParam, int nMaxLen, char *pBuffer);

int ReadImportantInfo(ImportantCfg_t* im);

VOID ReadConfigInfo();
VOID WriteConfigInfo();

char *sysfunc_get_key_value(char *data, char *key, char *value, int maxlen);

/**
 *@brief 从一个Buffer中获取指定Key对应的Value值
 */
char *SYSFuncs_GetKeyValue(const char *pBuffer, const char *key, char *valueBuf, int maxValueBuf);

/**
 *@brief 获取指定的参数
 */
char *SYSFuncs_GetValue(const char *fname, const char *key, char *value, int maxValueLen);

/**
 *@brief 重启系统
 */
int SYSFuncs_reboot(void);

/**
 *@brief 恢复系统默认值
 */
int SYSFuncs_factory_default(int bHardReset);

/**
 *@brief 恢复系统默认值，不需要重启设备
 */
int SYSFuncs_factory_default_without_reboot();

int __CalcBitrate(int w, int h, int framerate, int vencType);

#endif




//svn test
