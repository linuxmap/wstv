/*
 * jv_gpio.h
 *
 *  Created on: 2014年8月6日
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */
#ifndef JV_GPIO_H_
#define JV_GPIO_H_

typedef struct{
	unsigned int addr;		//muxctrl addr
	int value;				//addr 为value的时候被复用为gpio
	int group;				//被复用为gpio：group_bit
	int bit;
}jv_gpio_muxctrl_reg;

typedef struct{
	int (*fptr_gpio_read)(int group, int bit);
	int (*fptr_gpio_write)(int group, int bit, int value);

	int (*fptr_gpio_dir_get)(int group);

	int (*fptr_gpio_dir_set)(int group, int dir);

	int (*fptr_gpio_dir_set_bit)(int group, int bit, int dir);

	int (*fptr_gpio_muxctrl)(unsigned int addr, int value);
	int (*fptr_gpio_muxctrl2io)(int group, int bit);
}jv_gpio_func_t;

extern jv_gpio_func_t g_gpiofunc;
/**
 *@brief 初始化
 */
int jv_gpio_init();

/**
 *@brief 读取一位
 *@param group GPIO的组
 *@param bit GPIO的位
 *
 *@return value of bit
 */
static int jv_gpio_read(int group, int bit)
{
	if(group<0||bit<0)
		return -1;
	if (g_gpiofunc.fptr_gpio_read)
		return g_gpiofunc.fptr_gpio_read(group, bit);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 设置一位
 */
static int jv_gpio_write(int group, int bit, int value)
{
	if(group<0||bit<0||value<0)
		return -1;
	if (g_gpiofunc.fptr_gpio_write)
		return g_gpiofunc.fptr_gpio_write(group, bit, value);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_gpio_dir_get(int group)
{
	if(group<0)
		return -1;
	if (g_gpiofunc.fptr_gpio_dir_get)
		return g_gpiofunc.fptr_gpio_dir_get(group);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_gpio_dir_set(int group, int dir)
{
	if(group<0||dir<0)
		return -1;
	if (g_gpiofunc.fptr_gpio_dir_set)
		return g_gpiofunc.fptr_gpio_dir_set(group, dir);
	return JVERR_FUNC_NOT_SUPPORT;
}

static int jv_gpio_dir_set_bit(int group, int bit, int dir)
{
	if(group<0||bit<0||dir<0)
		return -1;
	if (g_gpiofunc.fptr_gpio_dir_set_bit)
		return g_gpiofunc.fptr_gpio_dir_set_bit(group, bit, dir);
	return JVERR_FUNC_NOT_SUPPORT;
}

/**
 *@brief 设置复用
 */
static int jv_gpio_muxctrl(unsigned int addr, int value)
{
	if(value<0)
		return -1;
	if (g_gpiofunc.fptr_gpio_muxctrl)
		return g_gpiofunc.fptr_gpio_muxctrl(addr, value);
	return JVERR_FUNC_NOT_SUPPORT;
}

#endif /* JV_GPIO_H_ */
