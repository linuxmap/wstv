/*
 * utl_mutex.h
 *
 *  Created on: 2013-11-19
 *      Author: lfx
 */

#ifndef UTL_MUTEX_H_
#define UTL_MUTEX_H_

typedef struct{
	char dummy;
}UtlMutex_t;

typedef void * MutexHandle;

/**
 *@brief 创建互斥锁
 *
 *@param 创建所需参数。此处为预留。考虑到以后可能会真的用到更复杂的参数
 *
 *@return handle or NULL
 */
MutexHandle utl_mutex_create(UtlMutex_t *param);

/**
 *@brief 释放互斥锁
 */
int utl_mutex_destroy(MutexHandle handle);

/**
 *@brief 锁定
 *
 *@param handle #utl_mutex_create 所创建的句柄
 */
int utl_mutex_lock(MutexHandle handle);


/**
 *@brief 释放
 *
 *@param handle #utl_mutex_create 所创建的句柄
 */
int utl_mutex_unlock(MutexHandle handle);


/**
 *@brief 一个早已存在的简单的锁
 *@note 这个锁，是整个程序共用的。用于非常简单的互斥。
 *   由于此函数将会是一个全局的锁，所以，使用时请务必保证
 *   锁定范围内的代码，都是你自己写的，避免内部还有调用该
 *   锁从而导致死锁的发生。也要保证锁定的代码足够简单
 *
 */
int utl_mutex_simple_lock();

/**
 *@brief 一个早已存在的简单的锁，解锁
 *@note 请认真参阅#utl_mutex_simple_lock
 */
int utl_mutex_simple_unlock();

#endif /* UTL_MUTEX_H_ */
