//RoyZhang
//2014.10.21
//门磁报警相关应用

#include <stdio.h>             
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "jv_dooralarm.h"

jv_dooralarm_func_t g_dooralarm_func;


void jv_dooralarm_init(void)
{
	memset(&g_dooralarm_func, 0, sizeof(g_dooralarm_func));
}


