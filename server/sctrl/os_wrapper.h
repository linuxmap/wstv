#ifndef _OS_WRAPPER_H_
#define _OS_WRAPPER_H_

#define TASK_NAMELEN	32


typedef enum{false, true}bool;


int os_getpid();

void os_get_thread_name(int pid, char name[TASK_NAMELEN]);

#endif

