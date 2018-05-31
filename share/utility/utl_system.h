#ifndef _UTL_SYSTEM_H_
#define _UTL_SYSTEM_H_

#define IsSystemFail(ret) (-1 == (ret) || 0 == WIFEXITED((ret)) || 0 != WEXITSTATUS((ret)))

int utl_system_init(void);

int utl_system(char *cmd, ...);

int utl_system_ex(char* strCmd, char* strResult, int nSize);
//获取linux系统剩余内存大小
int utl_system_get_free_mem();

#endif

