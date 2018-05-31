/*
 * sp_init.c
 *
 *  Created on: 2014-7-21
 *      Author: LK
 */
#include "sp_define.h"
#include "sp_init.h"
#include <dlfcn.h>

/*
 * @brief sdkInit
 */
int sp_init()
{
	void *handle =NULL;
	void (*ipc_init)();
	handle = dlopen("/etc/conf.d/libjvsipcsdk.so",RTLD_LAZY);
	if(handle==NULL)
	{
	     printf("ipc sdk dll loading error.\n");
	     return -1;
	}
	ipc_init=(void(*)())dlsym(handle,"ipc_init");
	if(dlerror()!=NULL)
	{
	     printf("fun load error.\n");
	     return -1;
	}
	ipc_init();

	return 0;
}
