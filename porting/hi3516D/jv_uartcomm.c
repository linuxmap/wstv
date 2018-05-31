#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "termios.h"
#include "jv_common.h"
#include "jv_uartcomm.h"

jv_uartcomm_func_t g_uartcomm_func;

void jv_uartcomm_init()
{
	memset(&g_uartcomm_func, 0, sizeof(g_uartcomm_func));
}
