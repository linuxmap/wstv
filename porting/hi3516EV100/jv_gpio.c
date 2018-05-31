/*
 * jv_gpio.c
 *
 *  Created on: 2014年8月6日
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */
#include <jv_common.h>
#include <jv_gpio.h>
#include "hi_gpio.h"

#define GPIO_DEV "/dev/gpio"

static int gpiofd = -1;


/**
 *@brief 读取一位
 *@param group GPIO的组
 *@param bit GPIO的位
 *
 *@return value of bit
 */
static int _gpio_read(int group, int bit)
{
	if (gpiofd < 0)
		return -1;
	gpio_groupbit_info info;

	info.group = group;
	info.bitmask = 1 << bit;
	if (ioctl(gpiofd, GPIO_READ_DATA, &info) < 0)
	{
		return -1;
	}
	return info.value;
}

/**
 *@brief 设置一位
 */
static int _gpio_write(int group, int bit, int value)
{
	if (gpiofd < 0)
		return -1;
	gpio_groupbit_info info;

	info.group = group;
	info.bitmask = 1 << bit;
	if (value)
	{
		info.value = 1 << bit;
	}
	else
		info.value = 0;
	if (ioctl(gpiofd, GPIO_WRITE_DATA, &info) < 0)
	{
		return -1;
	}
	return 0;
}

static int _gpio_dir_get(int group)
{
	if (gpiofd < 0)
		return -1;
	gpio_groupbit_info info;

	info.group = group;
	if (ioctl(gpiofd, GPIO_GET_DIR, &info) < 0)
	{
		return -1;
	}
	return info.value;
}

static int _gpio_dir_set(int group, int dir)
{
	if (gpiofd < 0)
		return -1;
	gpio_groupbit_info info;

	info.group = group;
	info.value = dir;
	if (ioctl(gpiofd, GPIO_SET_DIR, &info) < 0)
	{
		return -1;
	}
	return 0;
}

static int _gpio_dir_set_bit(int group, int bit, int dir)
{
	if (gpiofd < 0)
		return -1;
	gpio_groupbit_info info;
	info.group = group;
	info.bitmask= 1 << bit;
	info.value = dir;
	if (ioctl(gpiofd, GPIO_SET_DIR_BIT, &info) < 0)
	{
		return -1;
	}
	return 0;
}

/**
 *@brief 设置复用
 */
static int _gpio_muxctrl(unsigned int addr, int value)
{
	if (gpiofd < 0)
		return -1;
	gpio_groupbit_info info;

	info.group= addr;
	info.value= value;
	info.bitmask= 0;
	if (ioctl(gpiofd, GPIO_MUX_CTRL, &info) < 0)
	{
		return -1;
	}
	return 0;
}

jv_gpio_func_t g_gpiofunc;

/**
 *@brief 初始化
 */
int jv_gpio_init()
{
	gpiofd = open(GPIO_DEV, O_RDWR);
	if (gpiofd < 0)
	{
		printf("ERROR: Failed open %s because: %s\n", GPIO_DEV, strerror(errno));
		return -1;
	}
	memset(&g_gpiofunc, 0, sizeof(g_gpiofunc));
//	g_gpiofunc.
	g_gpiofunc.fptr_gpio_read       = _gpio_read       ;
	g_gpiofunc.fptr_gpio_write      = _gpio_write      ;
	g_gpiofunc.fptr_gpio_dir_get    = _gpio_dir_get    ;
	g_gpiofunc.fptr_gpio_dir_set    = _gpio_dir_set    ;
	g_gpiofunc.fptr_gpio_dir_set_bit= _gpio_dir_set_bit;
	g_gpiofunc.fptr_gpio_muxctrl    = _gpio_muxctrl    ;

	return 0;
}

