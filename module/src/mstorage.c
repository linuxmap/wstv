

#include "jv_common.h"
#include "mlog.h"
#include "mrecord.h"

#include "mstorage.h"

static STORAGE stStorage;

#define MMC_DEV		"/dev/mmcblk%d"
#define MMC_PART	"/dev/mmcblk%dp%d"

//获取命令的输出信息，结果存放在strResult中
static S32 GetCmdResult(char* strCmd, char* strResult, U32 nSize)
{
	if(strResult)
	{
		FILE *fd;
		U32 ch,i;

		if((fd = popen(strCmd, "r")) == NULL)
		{
			Printf("ERROR: get_system(%s)", strCmd);
			return -1;
		}
		for(i=0; i<nSize;i++)//单字节取数据，此处需要优化,lck20111227
		{
			if((ch = fgetc(fd)) != EOF)
			{
				strResult[i] = (char)ch;
			}
			else break;
		}
		pclose(fd);
	}
    return 0;
}

//格式化SD卡
S32 mstorage_format(U32 nDisk)
{
	char	strCmd[128]={0};

	//停止录像，将设备让出来
	mrecord_stop(0);
	int j = 0;
	char strDev[128] = {0};
	utl_system("umount -l "MOUNT_PATH);
	for(j=0; j<MAX_DEV_NUM; j++)
	{
		sprintf(strDev , MMC_DEV, j);
		if(access(strDev, F_OK)==0)
		{
			break;
		}
	}

	if (!hwinfo.bEMMC)
	{
		//分区并格式化
		sprintf(strCmd , "fdisk "MMC_DEV, j);
		FILE *fd=popen(strCmd , "w");
		if(fd == NULL)
		{
			mlog_write("Format Storage Failed");
			return -1;
		}

		//先删除分区
		fputs("o\n", fd);
		//创建part1分区
		fputs("n\n p\n 1\n \n \n t\n c\n" , fd);

		//将修改写入磁盘
		fputs("w\n", fd);
		pclose(fd);

		// 写完分区表后，会触发自动挂载，如果在格式化之前触发自动挂载，会导致格式化失败
		// 所以，这里延时一下，等自动挂载完后，再卸载
		sleep(2);
		utl_system("umount -l "MOUNT_PATH);
		
		//格式化分区
		sprintf(strCmd , "mkdosfs -F 32 "MMC_PART" -s 128", nDisk, 1);
		utl_system(strCmd);
	}
	else
	{
		//格式化分区
		sprintf(strCmd , "mkdosfs -F 32 "MMC_PART" -s 128", nDisk, 7);
		utl_system(strCmd);
	}

	mlog_write("Format Storage OK");
	return 0;
}

/***************************************************************
*功能	:判断指定路径空间是否不足nReserved
*参数	:1.strPath，要判断的路径
*		 2.nReserved，要求剩余的空间大小，单位：MB
*返回值	:空间不足表示硬盘已满返回TRUE,硬盘不满返回FALSE
****************************************************************/
static BOOL IsDiskFull(char *strPath , U32 nReserved)
{
	struct statfs statDisk;

	if(statfs(strPath, &statDisk) == 0)
	{
		if(statDisk.f_blocks >0)
		{
			float fFreeSize=statDisk.f_bfree*(statDisk.f_bsize/1024.0/1024.0);
	        if((U32)fFreeSize > nReserved)
	        {
	        	return FALSE;
	        }
	    }
	}

	return TRUE;
}

static VOID GetPathInfo(char *strPath , U32* nTotalSpace, U32 *nFreeSpace)
{
	//如果有空指针则直接退出
	if(!strPath||!nTotalSpace||!nFreeSpace) return ;

	struct statfs statDisk;
	if(statfs(strPath, &statDisk) == 0)
	{
		#if 1	//MB
			float fBlockSize = statDisk.f_bsize/1024.0/1024.0;
			*nTotalSpace	= statDisk.f_blocks * fBlockSize;
			*nFreeSpace		= statDisk.f_bfree * fBlockSize;
		#else	//B
			*nTotalSpace	= statDisk.f_blocks * statDisk.f_bsize;
			*nFreeSpace		= statDisk.f_bfree * statDisk.f_bsize;
		#endif
	}
}

static S32 _mstorage_refresh()
{
	int i, j;
	char strDev[MAX_PATH];
	BOOL bSdHasPartFlag = FALSE;
	
	for(j=0; j<MAX_DEV_NUM; j++)
	{
		sprintf(strDev , MMC_DEV, j);
		if(access(strDev, F_OK)==0)
		{
			break;
		}
	}
	
	if(STG_USING == stStorage.nStatus
		|| STG_IDLE == stStorage.nStatus)
	{
		GetPathInfo(MOUNT_PATH, &stStorage.nPartSpace[stStorage.nCurPart], &stStorage.nFreeSpace[stStorage.nCurPart]);
		Printf("Storage using, return, space:%d, free:%d...\n", 
			stStorage.nPartSpace[stStorage.nCurPart], stStorage.nFreeSpace[stStorage.nCurPart]);
		return 0;
	}

	i = 0;
	if (hwinfo.bEMMC)
	{
		i = 6;	// EMMC的第7分区做存储
	}
	for(; i<PARTS_PER_DEV; i++)
	{
		sprintf(strDev , MMC_PART, j, i+1);
		if(!access(strDev , F_OK))//如果分区存在
		{
			bSdHasPartFlag = TRUE;
			break;
		}
	}

	if(i == PARTS_PER_DEV)//如果分区不存在
	{
		if(-1 != mstorage_format(j))
		{
			int nPart = 1;
			if (hwinfo.bEMMC)
			{
				nPart = 7;
			}
			sprintf(strDev , MMC_PART, j, nPart);
			bSdHasPartFlag= TRUE;
		}	
	}
	
	if(bSdHasPartFlag)
	{
		if (access(MOUNT_PATH, F_OK))
			utl_system("mkdir -p "MOUNT_PATH);

		char tmp_buf[64] = {0};
		snprintf(tmp_buf, sizeof(tmp_buf), "mount %s %s", strDev, MOUNT_PATH);
		if(0 == utl_system(tmp_buf))
		{
			stStorage.mounted = TRUE;
			stStorage.nStatus = STG_IDLE;
			GetPathInfo(MOUNT_PATH, &stStorage.nPartSpace[i], &stStorage.nFreeSpace[i]);
			Printf("nPart%d, TotalSpace:%d, FreeSpace:%d\n", i+1, stStorage.nPartSpace[i], stStorage.nFreeSpace[i]);
		}
		else
		{
			stStorage.nPartSpace[i] = FALSE;
			Printf("mount dev:%s failed, errno: %s\n", strDev, strerror(errno));
		}

	}

	return 0;
}

//获取存储器信息
S32 mstorage_mount()
{
	char strDev[MAX_PATH]={0};
	char strCmd[MAX_PATH]={0};
	char strResult[MAX_PATH*10]={0};
	char *pStart;
	U32 i,j;

	memset(&stStorage, 0, sizeof(STORAGE));
	stStorage.nStatus = STG_NONE;

	for(i=0; i<MAX_DEV_NUM; i++)
	{
		sprintf(strDev , MMC_DEV, i);
		if(access(strDev, F_OK)==0)
		{
			Printf("Disk FOUND:%s\n", strDev);
			break;
		}

	}
	if(i == MAX_DEV_NUM)
	{
		stStorage.nocard=TRUE;
		Printf("No Disk\n");
		return -1;
	}

	if (hwinfo.bEMMC)
	{
		// EMMC使用第7分区进行存储
		i = 7;
		snprintf(strDev, sizeof(strDev), "/dev/mmcblk0p7");
	}
	
	//使用fdisk获取磁盘的信息
	sprintf(strCmd , "fdisk %s -l" , strDev);
	GetCmdResult(strCmd, strResult, MAX_PATH*10);

	//根据命令打印信息获取硬盘的相关信息
	if((pStart = strstr(strResult, "B,")))//获取磁盘总容量,单位:字节
	{
		U64 ulSize;
		sscanf(pStart+strlen("B,"), "%lld", &ulSize);
		stStorage.nSize = ulSize*1.0/1024/1024;
		Printf("Disk Size:%d MB\n" , stStorage.nSize);
	}

	if((pStart = strstr(strResult, "sectors/track,")))//获取磁盘总柱面
	{
		pStart += strlen("sectors/track,");
		sscanf(pStart, "%d", &stStorage.nCylinder);
		Printf("nTotalCylinder:%d\n", stStorage.nCylinder);
	}

	for(j=0;j<10;j++)               //gyd20120405 卸载时需要等待卸载成功
	{
		utl_system("umount -l "MOUNT_PATH);
		break;
	}
	stStorage.nStatus = STG_NOFORMAT;
	_mstorage_refresh();
	//记录挂载成功的分区
	stStorage.nCurPart	= i;
    stStorage.nocard=FALSE;
	return 0;
}

#define MAX_DEL_FILE_NUM	6

int compare(const void *a, const void *b)
{
	char *str1 = (char *)a;
	char *str2 = (char *)b;

	return strcmp(str1+1, str2+1);
}

//FTW的回调函数，处理遍历到的目录以及里面的文件
static S32 FTWProc(const char *file, const struct stat* sb, int flag)
{
	char acFile[MAX_DEL_FILE_NUM+1][32]={{0}};
	char acTarget[MAX_PATH]={0};
	char delFile[MAX_PATH] = {0};
	int count = 0;
	int hadDelNum = 0;
	DIR *dir;
	struct dirent *ptr;
	char cmd[128] = {0};
	int i;

	Printf("FTWProc: %s\n", file);

	dir = opendir(file);

	//如果打不开目录则直接删除
	if(NULL == dir)
	{
		//remove(file);
		sprintf(delFile,"rm -rf %s",file);
		utl_system(delFile);
		Printf("Not dir, remove...\n");
		return -1;
	}

	//找出最早的MAX_DEL_FILE_NUM个文件(含无效文件)
	while((ptr = readdir(dir)) != NULL)
	{
		//忽略.和..两个目录
		if(strcmp(ptr->d_name, ".") && strcmp(ptr->d_name, ".."))
		{
			if( (strstr(ptr->d_name, ".jpg") || strstr(ptr->d_name, ".mp4") || strstr(ptr->d_name, ".jdx")) && 
				(ptr->d_name[0]==REC_NORMAL || ptr->d_name[0]==REC_DISCON || ptr->d_name[0]==REC_TIME || 
				ptr->d_name[0]==REC_MOTION || ptr->d_name[0]==REC_ALARM || ptr->d_name[0]==REC_IVP) )
			{
				strcpy(acFile[count++], ptr->d_name);
				qsort(acFile, count, 32*sizeof(char), compare);
				if(count > MAX_DEL_FILE_NUM)
					count = MAX_DEL_FILE_NUM;
			}
			else
			{
				sprintf(acTarget, "%s/%s", file, ptr->d_name);
				snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", acTarget);
				utl_system(cmd);
				Printf("remove target(invalid):%s\n", acTarget);
				hadDelNum++;
				if(hadDelNum >= MAX_DEL_FILE_NUM)
					break;
			}
		}
	}
	closedir(dir);

	for(i=0; i<count&&hadDelNum<MAX_DEL_FILE_NUM; i++,hadDelNum++)
	{
		sprintf(acTarget, "%s/%s", file, acFile[i]);
		snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", acTarget);
		utl_system(cmd);
		Printf("remove target(oldest):%s\n", acTarget);
	}

	if(hadDelNum < MAX_DEL_FILE_NUM)
	{
		snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", file);
		utl_system(cmd);
		Printf("remove target(empty):%s\n", file);
		hadDelNum++;
	}

	Printf("FTWProc: remove %d file(s) or dir(s)\n", hadDelNum);
	return 0;
}

//自动清理函数，清理早期的录像文件
static S32 AutoCleanup()
{
	char acFile[MAX_PATH]={0};
	char acDir[MAX_PATH]={0};
	BOOL bFindDir = FALSE;
	int year, mon, day;

	DIR *pDir	= opendir(MOUNT_PATH);
	if (pDir == NULL)
	{
		Printf("open dir [%s] error \n",MOUNT_PATH);
		return -1;
	}
	struct dirent *pDirent	= NULL;
	while((pDirent=readdir(pDir)) != NULL)
	{
		//忽略.和..两个目录
		if(strcmp(pDirent->d_name, ".") && strcmp(pDirent->d_name, ".."))
		{
			bFindDir = TRUE;
			
			if(hwinfo.bHomeIPC)
			{//家用产品只删除TF卡中的录像文件，其他文件保留
				if(sscanf(pDirent->d_name, "%04d%02d%02d", &year, &mon, &day) == 3)
				{
					if(0 == acFile[0] || strcmp(acFile, pDirent->d_name) > 0)
					{
						strcpy(acFile, pDirent->d_name);				
					}
				}
			}
			else
			{	//如果不是以年月日命名的目录则首先删除
				if(sscanf(pDirent->d_name, "%04d%02d%02d", &year, &mon, &day) != 3)
				{
					strcpy(acFile, pDirent->d_name);		
					break;
				}
				else if(0 == acFile[0] || strcmp(acFile, pDirent->d_name) > 0)
				{
					strcpy(acFile, pDirent->d_name);
				}	
			}
		}
	}
	closedir(pDir);

	//如果发现了目录，则处理这个目录
	if(bFindDir)
	{
		Printf("Find dir:%s\n", acFile);
		sprintf(acDir, "%s/%s", MOUNT_PATH, acFile);
		return FTWProc(acDir, NULL, DT_DIR);
	}
	else
	{
		Printf("AutoCleanup not find dir...\n");
		return -1;
	}

	return 0;
}

/**
 *@brief 初始化
 *
 */
int mstorage_init(void)
{
	mstorage_mount();
	// 开机删除云存储目录(暂时不考虑设备重启/断电的重传)
	char alarmPath[32];
	char cmd[128];
	snprintf(alarmPath, sizeof(alarmPath), "%salarm", MOUNT_PATH);
	if(access(alarmPath, F_OK) == 0)
	{
		snprintf(cmd, sizeof(cmd), "rm -rf %s", alarmPath);
		utl_system(cmd);
	}
	return 0;
}

/**
 *@brief 结束
 *
 */
int mstorage_deinit(void)
{
	//卸载分区
	umount(MOUNT_PATH);
	stStorage.nStatus = STG_NONE;
	return 0;
}

/**
 *@brief 获取存储器状态
 *
 */
int mstorage_get_info(STORAGE *storage)
{
	int ret;
	memset(storage, 0, sizeof(STORAGE));
	if (!stStorage.mounted)
	{
		ret = mstorage_mount();
		if (ret != 0)
			return ret;
	}
	else
	{
		int i;
		char strDev[256];
		for(i=0; i<PARTS_PER_DEV; i++)
		{
			sprintf(strDev , MMC_PART, 0, i+1);
			if(!access(strDev , F_OK))//如果分区存在
			{
				GetPathInfo(MOUNT_PATH, &stStorage.nPartSpace[i], &stStorage.nFreeSpace[i]);
			}
		}

	}
	memcpy(storage, &stStorage, sizeof(stStorage));
	return 0;
}


/**
 *@brief 获取录像保存的路径
 *
 *
 */
void mstorage_get_basepath(char *szPath)
{
	sprintf(szPath, "%s", MOUNT_PATH);
	strcat(szPath, "/record/");
	return ;
}

/**
 *@brief 进入使用状态
 *
 */
int mstorage_enter_using()
{
	char acDev[MAX_PATH]={0};

	if(stStorage.mounted
			&& STG_NOFORMAT != stStorage.nStatus && stStorage.nCurPart >= 0)
	{
		stStorage.nEntryCount++;
		stStorage.nStatus = STG_USING;
	}
	return 0;
}

/**
 *@brief 离开使用状态
 *
 */
int mstorage_leave_using()
{
	stStorage.nEntryCount--;
	if(stStorage.nEntryCount <= 0)
	{
		stStorage.nEntryCount = 0;
		stStorage.nStatus = STG_IDLE;
	}
	return 0;
}

// 获取磁盘挂载路径，pBuffer可以为NULL，但此时函数不可重入
const char* mstorage_get_partpath(U32 DiskNoUse, U32 PartNoUse, char* pBuffer, U32 nSize)
{
	static char path[256] = "";

	if (NULL == pBuffer)
	{
		pBuffer = path;
		nSize = sizeof(path);
	}

	snprintf(pBuffer, nSize, MOUNT_PATH);

	return pBuffer;
}

const char* mstorage_get_cur_recpath(char* strPath, int nSize)
{
	return mstorage_get_partpath(0, 0, strPath, nSize);
}

/**
 *@brief 要求足够的空间
 *@param size 所要求的空间大小，以M字节为单位
 *@note 如果没有足够的空间，本函数将会自动清除最早的录像记录
 *
 *@return 实际剩余空间的大小
 *
 */
int mstorage_allocate_space(int size)
{
	int nTimes = 10;

	if(stStorage.nStatus == STG_NONE)
	{
		return -2;
	}

	while(IsDiskFull(MOUNT_PATH, size))
	{
		if(0 != AutoCleanup())
		{
			Printf("AutoCleanup failed, nTimes=%d...\n", nTimes);
			return -1;
		}
	}
	return 0;
}

