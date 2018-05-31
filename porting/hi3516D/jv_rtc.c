#include <stdio.h>             
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "jv_rtc.h"

jv_rtc_func_t g_rtc_func;

void jv_rtc_init(void)
{
	memset(&g_rtc_func, 0 , sizeof(g_rtc_func));
}

