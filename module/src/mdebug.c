#include <errno.h>
#include <syslog.h>
#include "mdebug.h"
#include "mstorage.h"
#include "utl_system.h"
#include "utl_timer.h"

#define DEFAULT_OUTPUT_NODE		"/dev/console"
#define ENABLE_FFLUSH			(0)


int __real_printf(const char * fmt,...);
int __real_puts(const char * s);


static BOOL s_bOpenLog = FALSE;
static BOOL s_AddTimeStamp = FALSE;
static char	s_RedirectFileName[256] = "";

BOOL mdebug_checkredirectflag()
{
	char* pFlagName = "/progs/rec/00/redirect_log.flag";
	if (access(pFlagName, F_OK) == 0)
	{
		printf("%s, find redirect flag: %s\n", __func__, pFlagName);
		strcpy(s_RedirectFileName, "/progs/rec/00/printf_save.log");
		return TRUE;
	}

	pFlagName = "/progs/rec/01/redirect_log.flag";
	if (access(pFlagName, F_OK) == 0)
	{
		printf("%s, find redirect flag: %s\n", __func__, pFlagName);
		strcpy(s_RedirectFileName, "/progs/rec/01/printf_save.log");
		return TRUE;
	}
	
	printf("%s, not find redirect flag\n", __func__);

	return FALSE;
}

int mdebug_redirectprintf()
{
	// freopen(REDIRECT_FILENAME, "w", stdout);
	// freopen(REDIRECT_FILENAME, "w", stderr);
	utl_system("syslogd -O %s; klogd", s_RedirectFileName);
	openlog("<sctrl>", LOG_CONS | LOG_PID, LOG_USER);
	s_bOpenLog = TRUE;

	return 0;
}

int mdebug_recoverprintf()
{
	// freopen(DEFAULT_OUTPUT_NODE, "w", stdout);
	// freopen(DEFAULT_OUTPUT_NODE, "w", stderr);
	utl_system("killall syslogd; killall klogd");
	s_bOpenLog = FALSE;
	closelog();

	return 0;
}

int mdebug_starttelnetd(U16 Port)
{
	utl_system("telnetd -p %d", Port);
	return 0;
}

int mdebug_stoptelnetd()
{
	utl_system("killall telnetd");
	return 0;
}

const char* mdebug_get_redirect_logfilename()
{
	return s_RedirectFileName;
}

int mdebug_getlatestlog(char* buf, long size)
{
	FILE* fp = NULL;
	int ret = -1;

#if ENABLE_FFLUSH
	fflush(stdout);
	fflush(stderr);
	sync();
#endif

	if (!buf || !size)
	{
		return -1;
	}

	fp = fopen(s_RedirectFileName, "r");
	if (!fp)
	{
		printf("%s, open %s failed: %s\n", __func__, s_RedirectFileName, strerror(errno));
		return ret;
	}

	fseek(fp, -size, SEEK_END);

	ret = fread(buf, 1, size, fp);
	if (ret < 0)
	{
		printf("%s, read %s failed: %s\n", __func__, s_RedirectFileName, strerror(errno));
		fclose(fp);
		return ret;
	}

	fclose(fp);

	return 0;
}

static int mdebug_printftimestamp()
{
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);

	return __real_printf("<%5d.%03d000> ", tv.tv_sec, tv.tv_nsec / 1000000);
}

int __wrap_printf(const char * fmt,...)
{
	va_list ap;
	int ret;

	if (s_AddTimeStamp)
	{
		mdebug_printftimestamp();
	}

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);
#if ENABLE_FFLUSH
	fflush(stdout);
#endif

	if (s_bOpenLog)
	{
		va_start(ap, fmt);
		vsyslog(LOG_INFO, fmt, ap);
		va_end(ap);
	}

	return ret;
}

int __wrap_puts(const char * str)
{
	int ret;

	if (s_AddTimeStamp)
	{
		mdebug_printftimestamp();
	}

	ret = __real_puts(str);
#if ENABLE_FFLUSH
	fflush(stdout);
#endif

	if (s_bOpenLog)
	{
		syslog(LOG_INFO, str);
	}

	return ret;
}

