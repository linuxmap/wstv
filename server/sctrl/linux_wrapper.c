#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "os_wrapper.h"

#define gettid()   syscall(__NR_gettid) 


int os_getpid()
{
	return gettid();
}

void os_get_thread_name(int pid, char name[TASK_NAMELEN])
{
	prctl(PR_GET_NAME, name);
}

