#ifndef _M_DEBUG_H_
#define _M_DEBUG_H_

#include "jv_common.h"
#include "mstorage.h"


BOOL mdebug_checkredirectflag();

int mdebug_redirectprintf();

int mdebug_recoverprintf();

int mdebug_starttelnetd(U16 Port);

int mdebug_stoptelnetd();

const char* mdebug_get_redirect_logfilename();

int mdebug_getlatestlog(char* buf, long size);

#endif

