/*
 * platform.h
 *
 *  Created on: 2015Äê4ÔÂ22ÈÕ
 *      Author: LiuFengxiang
 *		 Email: lfx@jovision.com
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

unsigned long long pt_gettimeofday();

void *pt_mutex_create();

void pt_mutex_delete(void *mutex);

void pt_mutex_lock(void *mutex);

void pt_mutex_unlock(void *mutex);

#if defined WIN32 || defined WIN64
#include <windows.h>
typedef DWORD (WINAPI *PTHREAD_START_ROUTINE)(
	LPVOID lpThreadParameter
	);

void *pt_thread_create(const char *name, PTHREAD_START_ROUTINE thread_ptr, void *arg);
#else
void *pt_thread_create(const char *name, void *(*thread_ptr)(void *), void *arg);

#endif

int pt_thread_join(void *thread, void **retval);

int pt_sleep(unsigned int microsecond);

#endif /* PLATFORM_H_ */
