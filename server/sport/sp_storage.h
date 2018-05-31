/*
 * sp_storage.h
 *
 *  Created on: 2013-11-18
 *      Author: Administrator
 */

#ifndef SP_STORAGE_H_
#define SP_STORAGE_H_

#include "sp_define.h"

#define MAX_DEV_NUM			4
#define PARTS_PER_DEV		16

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned int		nSize;			//设备容量(MB)
	unsigned int		nCylinder;		//硬盘柱面数
	unsigned int		nPartSize;		//每分区柱面数
	unsigned int		nPartition;	//可用分区数量
	unsigned int		nEntryCount;
	unsigned int		nStatus;
	unsigned int		nCurPart;
	BOOL	nocard;
	unsigned int		nPartSpace[PARTS_PER_DEV];	//分区总空间,MB
	unsigned int		nFreeSpace[PARTS_PER_DEV];	//分区空闲空间,MB
	BOOL	mounted;	//是否已挂载
}SPStorage_t;

/**
 *@brief 获取存储器状态
 *
 *@param 存储器信息结构输出
 *@return 0 成功
 */
int sp_storage_get_info(SPStorage_t *storage);

/**
 * @brief 格式化sd卡
 * @param ndisk
 *
 * return 0 成功
 */
int sp_storage_format(unsigned int nDisk);

#ifdef __cplusplus
}
#endif

#endif /* SP_STORAGE_H_ */
