#ifndef JV_GPIOCOMM_H
#define JV_GPIOCOMM_H

#include "jv_common.h"

typedef struct{
	
	int (*fptr_gpiocomm_doorID_get)(unsigned char* data);		//从jv层获得数据

}jv_gpiocomm_func_t;

extern jv_gpiocomm_func_t g_gpiocomm_func;

static int jv_gpiocomm_doorID_get(unsigned char* data)
{
	if (g_gpiocomm_func.fptr_gpiocomm_doorID_get)
		return g_gpiocomm_func.fptr_gpiocomm_doorID_get(data);
	return JVERR_FUNC_NOT_SUPPORT;
}

void jv_gpiocomm_init(void);



#endif