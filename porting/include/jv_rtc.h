#ifndef JV_RTC_H
#define JV_RTC_H

#include "jv_common.h"

typedef struct{
	
	int (*fptr_rtc_sync)(int);		//时间同步

}jv_rtc_func_t;

extern jv_rtc_func_t g_rtc_func;

//set:1	read:0
static int jv_rtc_sync(int mode)
{
	if (g_rtc_func.fptr_rtc_sync)
		return g_rtc_func.fptr_rtc_sync(mode);
	return JVERR_FUNC_NOT_SUPPORT;
}

void jv_rtc_init(void);


#endif

