#ifndef JV_DOORALARM_H
#define JV_DOORALARM_H

#include "jv_common.h"

typedef struct{
	
	int (*fptr_dooralarm_get)(unsigned char* data);		//从jv层获得数据

}jv_dooralarm_func_t;

extern jv_dooralarm_func_t g_dooralarm_func;

static int jv_dooralarm_get(unsigned char* data)
{
	if (g_dooralarm_func.fptr_dooralarm_get)
		return g_dooralarm_func.fptr_dooralarm_get(data);
	return JVERR_FUNC_NOT_SUPPORT;
}

void jv_dooralarm_init(void);


#endif
