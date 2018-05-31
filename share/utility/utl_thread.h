#ifndef _UTL_THREAD_H_
#define _UTL_THREAD_H_

#define LINUX_THREAD_STACK_SIZE (512*1024)
//创建分离线程,参数同pthread_create
//如果使用pthread_create，必须有pthread_join对应，否则会有内存泄漏。如果没有pthread_join，请调用
//注意:忽略了pthread_attr_t参数
int pthread_create_detached(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg);

//精简版本
#define pthread_create_simple(pFun, parm) \
do{\
	pthread_t pid;\
	pthread_create_detached(&pid, NULL, (void*)pFun, (void*)parm);\
}while(0)

//创建普通线程,栈大小设为LINUX_THREAD_STACK_SIZE。参数同pthread_create
//注意:忽略了pthread_attr_t参数
int pthread_create_normal(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg);

//创建线程,栈大小设为LINUX_THREAD_STACK_SIZE。参数同pthread_create
//优先级设置为51
//注意:忽略了pthread_attr_t参数
int pthread_create_normal_priority(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg);

#endif

