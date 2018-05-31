/******************************************************************************

  Copyright (c) 2015 . All rights reserved.

  ******************************************************************************
  File Name     : BizCpLinuxConfig.h
  Version       : Daomeng Han <itangwang@126.com>
  Author        : Daomeng Han <itangwang@126.com>
  Created       : 2014-12-02 14:05:32
  Description   : 
  History       : 
  1.Date        : 2014-12-02 14:05:33
    Author      : Daomeng Han <itangwang@126.com>
	Version     : 0.1
	Modification: 
******************************************************************************/

#ifndef _BIZ_CP_LINUX_CONFIG_H_
#define _BIZ_CP_LINUX_CONFIG_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* pread(), pwrite(), gethostname() */
#endif

#define _FILE_OFFSET_BITS  64

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>             /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
//#include <glob.h>

#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>        /* TCP_NODELAY, TCP_CORK */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include <time.h>               /* tzset() */
#include <limits.h>             /* IOV_MAX */
#include <sys/ioctl.h>
//#include <crypt.h>
#include <sys/utsname.h>        /* uname() */
#include <sys/time.h> 
#include <semaphore.h>


#define BIZ_CP_INLINE		inline
#define BIZ_CP_CDECL
#define BIZ_CP_STDCALL
#define BIZ_CP_CDECL

	typedef char			ZINT8;
	typedef int16_t			ZINT16;
	typedef int32_t			ZINT32;
	typedef int64_t			ZINT64;
	typedef int32_t			ZINT;
	typedef unsigned char	ZUINT8;
	typedef uint16_t		ZUINT16;
	typedef uint32_t		ZUINT32;
	typedef uint64_t		ZUINT64;
	typedef uint32_t		ZUINT;
	typedef	long			ZLONG;
	typedef	unsigned long	ZULONG;
	typedef int				ZBOOL;

#define BIZ_CP_LF			(u_char) 10
#define BIZ_CP_CR			(u_char) 13
#define BIZ_CP_CRLF			"\x0d\x0a"

#define BIZ_CP_ABS(value)       (((value) >= 0) ? (value) : - (value))
#define BIZ_CP_MAX(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define BIZ_CP_MIN(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define BIZ_CP_INT64(n)		n##LL

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _BIZ_CP_LINUX_CONFIG_H_ */

