/**
 *@file utl_cmd.h 用于调试的一个命令处理工具
 * 
 *@author Liu Fengxiang
 */

#ifndef _JV_CMD_H_
#define _JV_CMD_H_

/**
 *@brief 是否使能cmd
 */
void utl_cmd_enable(int bEnable);

int utl_cmd_init();

int utl_cmd_deinit();

/**
 *@brief 插入一条命令，用于调试
 *@param cmd 命令名称
 *@param help_general 命令的大概说明
 *@param help_detail 命令的详细介绍
 *@param func 命令运行的函数指针，其返回值为0时表示正确，
 *	其它值都被认为执行失败
 *
 *@note cmd,help_general,help_detail 在本模块内部并未为其分配内存，只保存了其指针
 *
 *@return 0 成功，JVERR_NO_RESOURCE 命令添加的太多了
 */
int utl_cmd_insert(char *cmd, char *help_general, char *help_detail, int (*func)(int argc, char *argv[]));

/**
 *@brief 与标准函数system类似，执行指定的命令
 *
 *@param cmd 要执行的命令
 *
 *
 */
int utl_cmd_system(char *cmd);

/**
 *@brief 获取帮助信息
 *@param cmd 要获取帮助信息的命令名，当为NULL时表示获取帮助列表
 *
 *@return 返回帮助字符串，失败返回NULL
 *@note 返回的字符串，是malloc申请的内存，所以需要释放
 *
 */
char *utl_cmd_get_help(char *cmd);

//测试用函数。添加线程信息
void pthreadinfo_add(const char *threadName);
void pthreadinfo_list(void);
int utl_thread_getpid();

#endif

