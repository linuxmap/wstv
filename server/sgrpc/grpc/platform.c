/*
 * platform.c
 *
 *  Created on: 2015Äê4ÔÂ22ÈÕ
 *      Author: LiuFengxiang
 *		 Email: lfx@jovision.com
 */

#include <stdlib.h>

#if defined WIN32 || defined WIN64
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#endif


#if defined WIN32 || defined WIN64

void
gettimeofday(struct timeval *tp, void *pp)
{
    unsigned long long  intervals;
    FILETIME  ft;

    GetSystemTimeAsFileTime(&ft);

    /*
     * A file time is a 64-bit value that represents the number
     * of 100-nanosecond intervals that have elapsed since
     * January 1, 1601 12:00 A.M. UTC.
     *
     * Between January 1, 1970 (Epoch) and January 1, 1601 there were
     * 134744 days,
     * 11644473600 seconds or
     * 11644473600,000,000,0 100-nanosecond intervals.
     *
     * See also MSKB Q167296.
     */

    intervals = ((unsigned long long) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    intervals -= 116444736000000000;

    tp->tv_sec = (long) (intervals / 10000000);
    tp->tv_usec = (long) ((intervals % 10000000) / 10);
}
#endif


unsigned long long pt_gettimeofday()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void *pt_mutex_create()
{
#if defined WIN32 || defined WIN64
		return CreateMutex(NULL, FALSE, NULL);
#else
		pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(mutex, NULL);
		return mutex;
#endif
}

void pt_mutex_delete(void *mutex)
{
#if defined WIN32 || defined WIN64
		CloseHandle(mutex);
#else
	pthread_mutex_destroy(mutex);
	free(mutex);
#endif
}
void pt_mutex_lock(void *mutex)
{
#if defined WIN32 || defined WIN64
	WaitForSingleObject(mutex, INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif
}

void pt_mutex_unlock(void *mutex)
{
#if defined WIN32 || defined WIN64
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
}

#if defined WIN32 || defined WIN64
void *pt_thread_create(const char *name, PTHREAD_START_ROUTINE thread_ptr, void *arg)
{
	HANDLE handle = CreateThread(NULL,0,thread_ptr,NULL,0,NULL);
	return handle;
}
#else

void *pt_thread_create(const char *name, void *(*thread_ptr)(void *), void *arg)
{
	pthread_t *threadid = malloc(sizeof(pthread_t));
	pthread_create(threadid, NULL, thread_ptr, arg);
	return threadid;
}
#endif

int pt_thread_join(void *thread, void **retval)
{
#if defined WIN32 || defined WIN64
	Sleep(1000);
	return 0;
#else
	pthread_t *p = thread;
	return pthread_join(*p, retval);
#endif
}

int pt_sleep(unsigned int microsecond)
{
#if defined WIN32 || defined WIN64
	Sleep(microsecond/1000);
#else
	usleep(microsecond);
#endif

	return 0;

}

