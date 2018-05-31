#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "termios.h"
#include "jv_common.h"
#include "jv_gpiocomm.h"

jv_gpiocomm_func_t g_gpiocomm_func;

void jv_gpiocomm_init()
{
	memset(&g_gpiocomm_func, 0, sizeof(g_gpiocomm_func));
}


