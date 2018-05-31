#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "utl_thread.h"


//创建分离线程,参数同pthread_create
//注意:忽略了pthread_attr_t参数
int pthread_create_detached(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg)
{
	int ret;
	size_t stacksize = LINUX_THREAD_STACK_SIZE;
	pthread_attr_t attr;
	pthread_t tmpid;
	pthread_t* p_tid = NULL;

	p_tid = (pid == NULL) ? &tmpid : pid;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	//分离线程
	pthread_attr_setstacksize(&attr, stacksize);					//栈大小
	
	ret = pthread_create(p_tid, &attr, (void*)pFunction, (void *)(arg));
	if (ret != 0)
	{
		printf("Error!!!!! pthread_create failed with %d, %s(%d)\n", ret, strerror(errno), errno);
	}
	
	pthread_attr_destroy (&attr);
	return ret;
}

//创建线程,栈大小设为LINUX_THREAD_STACK_SIZE。参数同pthread_create
//注意:忽略了pthread_attr_t参数
int pthread_create_normal(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg)
{
	int ret;
	size_t stacksize = LINUX_THREAD_STACK_SIZE;
	pthread_attr_t attr;
	
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stacksize);					//栈大小
	
	ret = pthread_create(pid, &attr, (void*)pFunction, (void *)(arg));
	
	pthread_attr_destroy (&attr);
	return ret;
}

//创建线程,栈大小设为LINUX_THREAD_STACK_SIZE。参数同pthread_create
//优先级设置为51
//注意:忽略了pthread_attr_t参数
int pthread_create_normal_priority(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg)
{
	int ret;
	size_t stacksize = LINUX_THREAD_STACK_SIZE;
	pthread_attr_t attr;
	struct sched_param param;
	
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stacksize);					//栈大小

	param.sched_priority = 51;	//优先级
	pthread_attr_setschedpolicy(&attr,SCHED_RR);
	pthread_attr_setschedparam(&attr,&param);
	pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);//要使优先级其作用必须要有这句话

	ret = pthread_create(pid, &attr, (void*)pFunction, (void *)(arg));
	
	pthread_attr_destroy (&attr);
	return ret;
}


