/*
 * jvsyslog.c
 *
 *  Created on: 2014Äê1ÔÂ23ÈÕ
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */

#include <stdio.h>
#include <syslog.h>
#include  <stdarg.h>

extern int syslogValue ;
int __real_printf(const char *fmt, ...);
int __real_puts(const char *s);


int __wrap_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	if (syslogValue)
	{
		vsyslog(LOG_INFO, fmt, ap);
	}
	else
	{
		vprintf(fmt, ap);
	}
	va_end(ap);

	return 0;
}

int __wrap_puts(const char *str)
{
	if (syslogValue)
	{
		syslog(LOG_INFO, str);
	}
	else
	{
		__real_puts(str);
	}

	return 0;
}
