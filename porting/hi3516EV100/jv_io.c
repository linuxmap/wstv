#include "jv_io.h"
#include "jv_gpio.h"

jv_io_ctrl_func_t g_ioctrlfunc;

static IOKey_e _io_key_get()
{
	if(higpios.resetkey.group != -1)
	{
		if (jv_gpio_read(higpios.resetkey.group, higpios.resetkey.bit) == 0)
		{
			return IO_KEY_RESET;
		}
	}

	return IO_KEY_BUTT;
}

static int _io_led_set(IOLedType_e type, IOStatus_e status)
{
	int ret = -1;

	switch(type)
	{
		case IO_LED_INFRARED:
			if(higpios.redlight.group == -1)
				break;
			if(status == IO_ON)
			{
				ret = jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 1);		//碕翌菊疏
			}
			else
			{
				ret = jv_gpio_write(higpios.redlight.group, higpios.redlight.bit, 0);		//碕翌菊註
			}
			break;
		case IO_LED_WHITE:
			if(higpios.whitelight.group == -1)
				break;
			ret = jv_gpio_write(higpios.whitelight.group, higpios.whitelight.bit, status == IO_ON ? 1 : 0);
			break;
		case IO_LED_RED:
			if(higpios.statusledR.group == -1)
				break;
			ret = jv_gpio_write(higpios.statusledR.group, higpios.statusledR.bit, 
							(status == IO_ON) ? higpios.statusledR.onlv : !higpios.statusledR.onlv);
			break;
		case IO_LED_GREEN:
			if(higpios.statusledG.group == -1)
				break;
			ret = jv_gpio_write(higpios.statusledG.group, higpios.statusledG.bit, 
						(status == IO_ON) ? higpios.statusledG.onlv : !higpios.statusledG.onlv);
			break;
		case IO_LED_BLUE:
			if(higpios.statusledB.group == -1)
				break;
			ret = jv_gpio_write(higpios.statusledB.group, higpios.statusledB.bit, 
					(status == IO_ON) ? higpios.statusledB.onlv : !higpios.statusledB.onlv);
			break;
		default:
			break;
	}
	return ret;
}

int _io_dev_status(IODEV_TYPE type)
{
	int retval = 0;
	
	switch(type)
	{
		case DEV_REDBOARD:
			break;
		case DEV_LIGHTBOARD:
			break;
		case DEV_WIFI:
			break;
		case DEV_RESET_KEY:
			if(higpios.resetkey.group != -1)
			{
				if (jv_gpio_read(higpios.resetkey.group, higpios.resetkey.bit) != 0)
					retval =  1;
				else
					retval = 0;
			}					
			break;
		default:
			break;
	}

	return retval;
}
int jv_io_init(void)
{
	memset(&g_ioctrlfunc, 0, sizeof(g_ioctrlfunc));
	g_ioctrlfunc.fptr_key_get = _io_key_get;
	g_ioctrlfunc.fptr_led_set = _io_led_set;
	g_ioctrlfunc.fptr_dev_status = _io_dev_status;
	return 0;
}

