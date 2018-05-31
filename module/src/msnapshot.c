/*
 * msnapshot.c
 *
 *  Created on: 2013-1-25
 *      Author: Administrator
 */

#include <jv_common.h>
#include "mosd.h"
#include <msnapshot.h>
#include <stdio.h>
#include <SYSFuncs.h>

#define S_FILE_MAX_SIZE 256*1024

#define KEY_ID	1234
static pthread_mutex_t snapshot_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct 
{
	long int Size;
	unsigned char acData[S_FILE_MAX_SIZE];
}shm_data_t;
int msnapshot_get_shmdata(int channelid)
{
	pthread_mutex_lock(&snapshot_mutex);
	//生成抓图文件并将抓图文件邮件
	void *shm = NULL;
	int shmid;
	shm_data_t* shmData = NULL;
	shmid = shmget((key_t)KEY_ID,sizeof(shm_data_t),0666|IPC_CREAT);
	shm = shmat(shmid,(void*)0,0);
	
	shmData = (shm_data_t*)shm; 

	shmData->Size = jv_snapshot_get(channelid, shmData->acData,S_FILE_MAX_SIZE);
	if(shmData->Size < 0)
	{
		pthread_mutex_unlock(&snapshot_mutex);
		shmdt(shm);
		return -1;
	}
	if(shmdt(shm) == -1)
	{
		printf("shmdt failed!\n");
		pthread_mutex_unlock(&snapshot_mutex);
		return -1;
	}
	pthread_mutex_unlock(&snapshot_mutex);

	return 0;

}

/**
 *@brief snap shot
 *@param channelid channel id
 *@param fname file name to store the capture
 *@retval 0 if success
 *@retval <0 if failed.
 */
int msnapshot_get_file(int channelid, char *fname)
{
	pthread_mutex_lock(&snapshot_mutex);
	//生成抓图文件并将抓图文件邮件
	FILE *fOut;
	S32 nSize = 0;
	unsigned char *acData = malloc(S_FILE_MAX_SIZE);

	if(!acData)
	{
		pthread_mutex_unlock(&snapshot_mutex);
		printf("msnapshot_get_file malloc failed\n");
		return -1;
	}

	fOut = fopen(fname, "wb+");
	if(fOut == NULL)
	{
		printf("msnapshot_get_file open file failed with %d\n", errno);
		free(acData);
		pthread_mutex_unlock(&snapshot_mutex);
		return -1;
	}

	stSnapSize size;
	jv_snapshot_get_def_size(&size);	//获取默认大小
	int tmp_w = size.nWith;
	int fontsize = 16;
	if (tmp_w > 2000)
		fontsize = 80;
	else if (tmp_w > 1800)
		fontsize = 64;
	else if (tmp_w > 1000)
		fontsize = 48;
	else if (tmp_w >= 600)
		fontsize = 32;
	else
		fontsize = 16;
	mosd_set_snap_flush(fontsize);	//设置osd字体大小

	nSize = jv_snapshot_get(channelid, acData,S_FILE_MAX_SIZE);
	if(nSize > 0)
	{
		int ret = fwrite(acData, nSize, 1, fOut);
		if(ret<=0)
		{
			printf("msnapshot_get_file write failed %d,error:%d\n",ret,errno);
			fclose(fOut);
			unlink(fname);
			free(acData);
			pthread_mutex_unlock(&snapshot_mutex);
			return -1;
		}
	}
	else
	{
		printf("Write jv_snapshot_get file failed, nSize=%d...\n", nSize);
		fclose(fOut);
		unlink(fname);
		free(acData);
		pthread_mutex_unlock(&snapshot_mutex);
		return -1;
	}
	fclose(fOut);
	free(acData);
	pthread_mutex_unlock(&snapshot_mutex);

	return 0;
}

/**
 *@brief snap shot
 *@param channelid channel id
 *@param fname file name to store the capture
 *@retval 0 if success
 *@retval <0 if failed.
 */
int msnapshot_get_file_ex(int channelid, char *fname, int width, int height)
{
	if(fname==NULL || width<=0 || height<=0)
		return -1;

	pthread_mutex_lock(&snapshot_mutex);
	//生成抓图文件并将抓图文件邮件
	FILE *fOut;
	stSnapSize size;
	S32 nSize = 0;
	S32 imgSize = S_FILE_MAX_SIZE;
	unsigned char *acData = malloc(imgSize);
	int fontsize = 16;

	if(!acData)
	{
		printf("msnapshot_get_file_ex malloc failed\n");
		pthread_mutex_unlock(&snapshot_mutex);
		return -1;
	}

	fOut = fopen(fname, "wb+");
	if(fOut == NULL)
	{
		printf("msnapshot_get_file_ex open file failed with %d\n", errno);
		free(acData);
		pthread_mutex_unlock(&snapshot_mutex);
		return -1;
	}

	jv_snapshot_get_def_size(&size);	//获取默认大小
	int tmp_w = size.nWith;
	if(width <= size.nWith)
	{
		tmp_w = width;
	}

	if (tmp_w > 2000)
		fontsize = 80;
	else if (tmp_w > 1800)
		fontsize = 64;
	else if (tmp_w > 1000)
		fontsize = 48;
	else if (tmp_w >= 600)
		fontsize = 32;
	else
		fontsize = 16;
	mosd_set_snap_flush(fontsize);	//设置osd字体大小

	size.nHeight = height;
	size.nWith = width;
	nSize = jv_snapshot_get_ex(channelid, acData, imgSize, &size);
	if(nSize > 0)
	{
		int ret = fwrite(acData, nSize, 1, fOut);
		if(ret<=0)
		{
			printf("msnapshot_get_file_ex write file failed with %d\n", errno);
			fclose(fOut);
			unlink(fname);
			free(acData);
			pthread_mutex_unlock(&snapshot_mutex);
			return -1;
		}
	}
	else
	{
		printf("Write jv_snapshot_get file failed, nSize=%d...\n", nSize);
		fclose(fOut);
		unlink(fname);
		free(acData);
		pthread_mutex_unlock(&snapshot_mutex);
		return -1;
	}
	fclose(fOut);
	free(acData);
	pthread_mutex_unlock(&snapshot_mutex);

	return 0;
}


/*
 * 获取抓图数据 snap为NULL的时候，为抓图默认大小。
 */
int msnapshot_get_data(int channelid, unsigned char *pData ,unsigned int len, stSnapSize *snap)
{
	int ret = 0;
	pthread_mutex_lock(&snapshot_mutex);
	stSnapSize size;
	jv_snapshot_get_def_size(&size);	//获取默认大小
	int tmp_w = size.nWith;
	int fontsize=16;

	if(snap)
		tmp_w = snap->nWith;

	if (tmp_w > 2000)
		fontsize = 80;
	else if (tmp_w > 1800)
		fontsize = 64;
	else if (tmp_w > 1000)
		fontsize = 48;
	else if (tmp_w >= 600)
		fontsize = 32;
	else
		fontsize = 16;
	mosd_set_snap_flush(fontsize);
	ret = jv_snapshot_get_ex(channelid,pData,len,snap);
	pthread_mutex_unlock(&snapshot_mutex);
	return ret;
}
