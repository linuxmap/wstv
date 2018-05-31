/*
 * utl_queue.c
 *
 *  Created on: 2014年3月14日
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

#include "utl_queue.h"


//队列信息
typedef struct
{
	sem_t sSem;
	int iFront,iRear;
	int iCnt;
	int iMsgSize;
	int iQueueSize;
	char **ppQueue;
	char szName[32];
} QueueInfo;

#define MAX_QUEUE_CNT	32
static QueueInfo sQueues[MAX_QUEUE_CNT];//多队列
static int iQueueCnt = 0;//队列数

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 *@brief 创建一个消息队列
 *@param name 消息队列的名字
 *@param msgsize 每个消息的尺寸
 *@param queuesize 消息队列中能保存的消息的个数
 *@retval >=0 消息队列的handle
 *@retval -1 创建消息队列时，内存分配失败
 *
 */
int utl_queue_create(char *name, int msgsize, int queuesize)
{
	int i = 0;
	pthread_mutex_lock(&_mutex);
	sQueues[iQueueCnt].iMsgSize = msgsize;
	sQueues[iQueueCnt].iQueueSize = queuesize;

	sQueues[iQueueCnt].ppQueue = (char **)malloc((sizeof(char *)*(sQueues[iQueueCnt].iQueueSize)));
	if(NULL == sQueues[iQueueCnt].ppQueue)
	{
		pthread_mutex_unlock(&_mutex);
		return -1;
	}
	for (i = 0; i < sQueues[iQueueCnt].iQueueSize; i++)
	{
		sQueues[iQueueCnt].ppQueue[i] = (char *)malloc(sQueues[iQueueCnt].iMsgSize);
		memset(sQueues[iQueueCnt].ppQueue[i], 0, sQueues[iQueueCnt].iMsgSize);
		if (NULL == sQueues[iQueueCnt].ppQueue[i])
		{
			int j = 0;
			for (j = 0; j < i ; j++)
			{
				free(sQueues[iQueueCnt].ppQueue[j]);
			}
			free(sQueues[iQueueCnt].ppQueue);
			pthread_mutex_unlock(&_mutex);
			return -1;
		}
	}

	strcpy(sQueues[iQueueCnt].szName, name);
	sQueues[iQueueCnt].iFront = 0;
	sQueues[iQueueCnt].iRear = 0;
	sQueues[iQueueCnt].iCnt = 0;
	sem_init(&sQueues[iQueueCnt].sSem, 0, 0);
	pthread_mutex_unlock(&_mutex);
	return 	iQueueCnt++;
}

/**
 *@brief 销毁一个消息队列
 *@param handle 消息队列的句柄，其值为#jv_queue_create 的返回值
 *@return 0
 *
 */
int utl_queue_destroy(int handle)
{
	pthread_mutex_lock(&_mutex);
	sem_destroy(&sQueues[handle].sSem);

	int i = 0;
	for (i = 0; i < sQueues[handle].iQueueSize; i++)
	{
		free(sQueues[handle].ppQueue[i]);
	}
	free(sQueues[handle].ppQueue);
	iQueueCnt--;
	pthread_mutex_unlock(&_mutex);
	return 0;
}

/**
 *@brief 发送消息
 *@param handle 消息队列的句柄，其值为#jv_queue_create 的返回值
 *@param msg 要发送的消息的指针
 *@note 消息的长度在 #jv_queue_create 时已指定
 *@return 0 成功，-1 队满，暂时不能发送
 *
 */
int utl_queue_send(int handle, void *msg)
{
	//pthread_mutex_lock(&sQueues[handle].mutex);
	pthread_mutex_lock(&_mutex);
	if (sQueues[handle].iCnt == sQueues[handle].iQueueSize)
	{
		//printf("the queue %s is full , can't send \n", sQueues[handle].szName);
		pthread_mutex_unlock(&_mutex);
		return -1;
	}
	memcpy(sQueues[handle].ppQueue[sQueues[handle].iFront], (char *)msg, sQueues[handle].iMsgSize);
	sQueues[handle].iFront = (sQueues[handle].iFront + 1) % (sQueues[handle].iQueueSize);
	(sQueues[handle].iCnt)++;
	pthread_mutex_unlock(&_mutex);
	sem_post(&sQueues[handle].sSem);
	return 0;
}

/**
 *@brief 接收消息
 *@param handle 消息队列的句柄，其值为#jv_queue_create 的返回值
 *@param msg 输出，用于接收数据的指针
 *@param timeout 超时时间，单位为毫秒，-1时不超时
 *@note 消息的长度在 #jv_queue_create 时已指定
 *@return 0 成功，错误号表示超时
 *
 */
int utl_queue_recv(int handle, void *msg, int timeout)
{
	struct timespec ts;
	ts.tv_sec=timeout/1000;   // 重点
	ts.tv_nsec=timeout*1000000%1000000000;
	char * buf = (char *)msg;
	//pthread_mutex_lock(&sQueues[handle].mutex);
	if (timeout == -1)
		sem_wait(&sQueues[handle].sSem);
	else
	{
		int sts =sem_timedwait(&sQueues[handle].sSem, &ts);
		if (-1 == sts)
		{
			return -1;
		}
	}
	pthread_mutex_lock(&_mutex);
	memcpy(buf, sQueues[handle].ppQueue[sQueues[handle].iRear], sQueues[handle].iMsgSize);
	sQueues[handle].iRear = (sQueues[handle].iRear + 1) % (sQueues[handle].iQueueSize); //出队列
	(sQueues[handle].iCnt)--;
	pthread_mutex_unlock(&_mutex);

	return 0;
}

int utl_queue_get_count(int handle)
{
	return sQueues[handle].iCnt;
}
