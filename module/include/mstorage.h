
/*	mstorage.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织存储设备管理相关函数
	更改历史详见svn版本库日志
*/

#ifndef __MSTORAGE_H__
#define __MSTORAGE_H__


//挂载目录不应该定义在存储模块内,lck20120305
#define MOUNT_PATH			"./rec/00/"

#define MAX_DEV_NUM			4
#define PARTS_PER_DEV		16

#define STG_NONE			0	//未发现设备
#define STG_NOFORMAT		1	//未格式化
#define STG_FULL			2	//满了
#define STG_USING			3	//使用中
#define STG_IDLE			4	//空闲（也是挂载上了）


typedef struct tagSTORAGE
{
	U32		nSize;			//设备容量(MB)
	S32		nCylinder;		//硬盘柱面数
	S32		nPartSize;		//每分区柱面数
	S32		nPartition;	//可用分区数量
	S32		nEntryCount;
	S32		nStatus;
	U32		nCurPart;
	BOOL	nocard;
	U32		nPartSpace[PARTS_PER_DEV];	//分区总空间,MB
	U32		nFreeSpace[PARTS_PER_DEV];	//分区空闲空间,MB
	BOOL	mounted;	//是否已挂载
}STORAGE, *PSTORAGE;


/**
 *@brief 初始化
 *
 */
int mstorage_init(void);

/**
 *@brief 结束
 *
 */
int mstorage_deinit(void);

/**
 *@brief 获取存储器状态
 *
 */
int mstorage_get_info(STORAGE *storage);

/**
 *@brief 获取录像保存的路径
 *
 *
 */
void mstorage_get_basepath(char *szPath);

/**
 *@brief 进入使用状态
 *
 */
int mstorage_enter_using();

/**
 *@brief 离开使用状态
 *
 */
int mstorage_leave_using();

// 获取磁盘挂载路径，pBuffer可以为NULL，但此时函数不可重入
const char* mstorage_get_partpath(U32 DiskNoUse, U32 PartNoUse, char* pBuffer, U32 nSize);

const char* mstorage_get_cur_recpath(char* strPath, int nSize);

/**
 *@brief 要求足够的空间
 *@param size 所要求的空间大小，以M字节为单位
 *@note 如果没有足够的空间，本函数将会自动清除最早的录像记录
 *
 *@return 实际剩余空间的大小
 *
 */
int mstorage_allocate_space(int size);

S32 mstorage_format(U32 nDisk);

S32 mstorage_mount();

#endif

