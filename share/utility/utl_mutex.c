/*
 * utl_mutex.c
 *
 *  Created on: 2013-11-19
 *      Author: lfx
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <pthread.h>

#include "utl_mutex.h"

static pthread_mutex_t simpleMutex;
static int bSimpleInited = 0;

/**
 *@brief 创建互斥锁
 *
 *@param 创建所需参数。此处为预留。考虑到以后可能会真的用到更复杂的参数
 *
 *@return handle or NULL
 */
MutexHandle utl_mutex_create(UtlMutex_t *param)
{
	pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
	if (!mutex)
	{
		return NULL;
	}
	pthread_mutex_init(mutex, NULL);

	return mutex;
}

/**
 *@brief 释放互斥锁
 */
int utl_mutex_destroy(MutexHandle handle)
{
	pthread_mutex_destroy((pthread_mutex_t *)handle);
	free(handle);
	return 0;
}

/**
 *@brief 锁定
 *
 *@param handle #utl_mutex_create 所创建的句柄
 */
int utl_mutex_lock(MutexHandle handle)
{
	return pthread_mutex_lock((pthread_mutex_t *)handle);
}


/**
 *@brief 释放
 *
 *@param handle #utl_mutex_create 所创建的句柄
 */
int utl_mutex_unlock(MutexHandle handle)
{
	return pthread_mutex_unlock((pthread_mutex_t *)handle);
}


/**
 *@brief 一个早已存在的简单的锁
 *@note 这个锁，是整个程序共用的。用于非常简单的互斥。
 *   由于此函数将会是一个全局的锁，所以，使用时请务必保证
 *   锁定范围内的代码，都是你自己写的，避免内部还有调用该
 *   锁从而导致死锁的发生。也要保证锁定的代码足够简单
 *
 */
int utl_mutex_simple_lock()
{
	if (!bSimpleInited)
	{
		pthread_mutex_init(&simpleMutex, NULL);
		bSimpleInited = 1;
	}

	pthread_mutex_lock(&simpleMutex);

	return 0;
}

/**
 *@brief 一个早已存在的简单的锁，解锁
 *@note 请认真参阅#utl_mutex_simple_lock
 */
int utl_mutex_simple_unlock()
{
	if (!bSimpleInited)
	{
		pthread_mutex_init(&simpleMutex, NULL);
		bSimpleInited = 1;
	}
	pthread_mutex_unlock(&simpleMutex);

	return 0;

}
