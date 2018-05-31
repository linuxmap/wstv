#include "jv_io.h"
#include "jv_gpio.h"

jv_io_ctrl_func_t g_ioctrlfunc;

int jv_io_init(void)
{
	memset(&g_ioctrlfunc, 0, sizeof(g_ioctrlfunc));
	return 0;
}

int jv_set_led(int status)
{
	return 0;
}

int jv_get_led(IOLedType_e type)
{
	int ret = -1;
	return ret;
}

