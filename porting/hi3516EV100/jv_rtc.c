#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "jv_rtc.h"
#include "hi_rtc.h"

#define RTC_DEV		"/dev/hi_rtc"
static int RTCFd = -1;

jv_rtc_func_t g_rtc_func;

//set:1	get:0
static int _jv_rtc_sync(int mode)
{
	rtc_time_t tim;
	time_t timep;
	struct tm t;
	struct timeval tv;
	int ret = -1;

	if(RTCFd < 0)
	{
		return -1;
	}

	if(mode)
	{
		time(&timep);
		gmtime_r(&timep, &t);
		tim.year = t.tm_year+1900;
		tim.month= t.tm_mon+1;
		tim.date= t.tm_mday;
		tim.hour = t.tm_hour;
		tim.minute= t.tm_min;
		tim.second = t.tm_sec;
		tim.weekday = t.tm_wday;

		ret = ioctl(RTCFd, HI_RTC_SET_TIME, &tim);
		if(ret < 0)
		{
			printf("rtc time set failed\n");
			return -1;
		}
		printf("rtc time set ok(%04d-%02d-%02d %02d:%02d:%02d)\n",
			tim.year, tim.month, tim.date, tim.hour, tim.minute, tim.second);
	}
	else
	{
		ret = ioctl(RTCFd, HI_RTC_RD_TIME, &tim);
		if(ret < 0)
		{
			printf("real time get failed\n");
			return -1;
		}
		printf("rtc time got: %04d-%02d-%02d %02d:%02d:%02d\n", 
			tim.year, tim.month, tim.date, tim.hour, tim.minute, tim.second);

		t.tm_hour = tim.hour;
		t.tm_min = tim.minute;
		t.tm_sec = tim.second;
		t.tm_mon = tim.month -1;
		t.tm_mday =  tim.date;
		t.tm_year = tim.year - 1900;
		timep = mktime(&t);
		if (timep == -1)
		{
			printf("RTC read time wrong\n");
			return -1;
		}
		stime(&timep);
	}

	return 0;
}

void jv_rtc_init(void)
{
	int ret = 0;

	memset(&g_rtc_func, 0, sizeof(g_rtc_func));

	RTCFd = open(RTC_DEV, O_RDWR);
	if(RTCFd < 0)
	{
		printf("sysTimeSync:open failed\n");
		return;
	}

	ret = ioctl(RTCFd, HI_RTC_AIE_ON, NULL);
	if(ret < 0)
	{
		printf("rtc on failed\n");
		close(RTCFd);
		RTCFd = -1;
		return;
	}

	g_rtc_func.fptr_rtc_sync = _jv_rtc_sync;
}

