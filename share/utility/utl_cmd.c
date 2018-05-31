#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "utl_cmd.h"
#include <mwdt.h>
#include <jv_common.h>

typedef struct{
	char *cmd;
	char *help_general;
	char *help_detail;
	int (*func)(int argc, char *argv[]);
}utl_cmd_t;
#define MAX_CMD_CNT	100
static utl_cmd_t cmd_list[MAX_CMD_CNT];
static int cmd_cnt = 0;
static int cmd_running = 0;

static char *__cmd_strip(char *str)
{
	int len;
	while(str && *str)
	{
		if (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
			str++;
		else
			break;
	}

	len = strlen(str);
	while(len--)
	{
		if (str[len] == ' ' || str[len] == '\t' || str[len] == '\n' || str[len] == '\r')
			str[len] = '\0';
		else
			break;
	}
	return str;
}

/**
 *@brief 跳过用分隔符隔起来的一组参数
 *比如'123 4  6 7 89'，当作一个参数
 *比如'123 "4" 5 "6" 7 8 9'，当作一个参数
 *比如"123 '4'5  6 7 '8 ' 9"，当作一个参数
 *
 *@param cmd 要处理的字符串
 *@param quotes 用作分隔符的字符
 *@return 跳过分隔符后的字符串地址
 * 例如'123'45，则返回的是45的地址
 *
 */
static char *__utl_cmd_jump_quotes(char *cmd, char quotes)
{
	if (*cmd == quotes)
	{
		while(cmd 
			&& *cmd
			&& *++cmd != quotes)
			;
	}
	return cmd;
}

static int _utl_cmd_parser(char *cmd, int *argc, char *argv[], int max_argc)
{
	int i;
	char *qs,*qe;//quotes start, quotes end
	int cnt = 0;

	*argc = 0;
	cmd = __cmd_strip(cmd);

	if (cmd == NULL)
		return -1;
	
	while(cmd && *cmd && cnt <= max_argc)
	{
		argv[cnt] = cmd;

		qs = cmd;
		qe = __utl_cmd_jump_quotes(qs, '\'');
		qe = __utl_cmd_jump_quotes(qe, '"');
		cmd = qe;
		while(cmd && *cmd)
		{
			cmd++;
			if (*cmd == ' ' || *cmd == '\t')
			{
				*cmd = '\0';
				cmd++;
				cmd = __cmd_strip(cmd);
				break;
			}
		}
		//printf("qs: %d, qe: %d, cmd:%d\n", qs-old, qe-old, cmd-old);
		if (qs != qe)
		{
			argv[cnt]++;
			if (qe != cmd)
				strcpy(qe, qe+1);
		}
		cnt++;
	}
	for (i=0;i<cnt;i++)
	{
		argv[i] = __cmd_strip(argv[i]);
	}
	*argc = cnt;
	return cnt;
}

int utl_cmd_system(char *cmd)
{
	int argc;
	char *argv[30];
	int i;
	
	_utl_cmd_parser(cmd, &argc, argv, 30);
	if (argc == 0)
		return -1;

	for (i=0;i<cmd_cnt;i++)
	{
		if (!strcmp(argv[0], cmd_list[i].cmd))
		{
			return cmd_list[i].func(argc, argv);
		}
	}
	if (i == cmd_cnt)
	{
		if(strcmp(argv[0], "exit") == 0)
		{
			CloseWatchDog();
			printf("Now exit the thread\n");
			exit(0);
		}
		else
		{
			printf("cmd not found.[%s]\n",argv[0]);
			for (i=0;i<argc;i++)
				printf("%s \n",argv[i]);
			printf("\n");
		}
	}
	return 0;
}

static void _utl_cmd_process(void *param)
{
	char cmd[1024];
	
	pthreadinfo_add((char *)__func__);

	while(cmd_running)
	{
		printf("debug#");
		utl_cmd_system(fgets(cmd, 256, stdin));
	}
}

static int _jv_help(int argc, char *argv[])
{
	int i;

	if (argc == 1)
	{
		printf("cmd list and help information\n\n");
		for (i=0;i<cmd_cnt;i++)
		{
			printf("  %s\n\t\t%s\n",cmd_list[i].cmd,cmd_list[i].help_general);
		}
		return 0;
	}
	else if (strcmp(argv[1], "*") == 0)
	{
		for (i=0;i<cmd_cnt;i++)
		{
			{
				printf("  %s\n\t\t%s\n",cmd_list[i].cmd,cmd_list[i].help_general);
				printf("%s\n",cmd_list[i].help_detail);
			}
		}
		return 0;
	}
	else if (argc == 2)
	{
		for (i=0;i<cmd_cnt;i++)
		{
			if (!strcmp(argv[1], cmd_list[i].cmd))
			{
				printf("%s\n",cmd_list[i].help_detail);
				return 0;
			}
		}
	}
	printf("Bad param\n");
	return -1;
}

static unsigned int hex_atoi(char *str)
{
	if (str[0] == '0' 
		&& (str[1] == 'x' || str[1] == 'X'))
	{
		return strtol(&str[2], NULL, 16);
	}
	else
	{
		return atoi(str);
	}
}

#define JUMP_GENERAL	"jump to the pointed addr and run"
#define JUMP_DETAIL		\
						"\tjump addr TYPE1 VALUE1 TYPE2 VALUE2 ...\n"\
						"\tGroup of TYPEX and VALUEX means one argv of the function\n"\
						"\tADDR the address want to jump\n"\
						"\n\tTYPE defined as below:\n"\
						"\t\t-d integer. the value maybe 20, 0x20 and so on.\n"\
						"\t\t-s string.\n"\
						"\n\tIf we call func_temp(int a, char *b);\n"\
						"\tFirst, objdump -d bin > bin.txt and find the addr of func_temp\n"\
						"\tWe suppose it is 0x2f8c, and then we do like this:\n"\
						"\t\tjump 0x2ff8c -d 123 -s abc\n"

/**
 *@brief 跳到指定地址去运行，用于调用某函数
 *使用方法为：
 *1，假如要调用函数：tmp_func(int argc, char *thlk, int c);
 *2，正常调用方式为：tmp_func(12,"temp",3);
 *3，objdump -d elf_file > file.txt
 *4，从file.txt中查找函数的地址，假如是0x2ffbc
 *5，我们的调用方法为：jump 0x2ffbc -d 12 -s temp -d 3
 *
 */
static int _jv_jump(int argc, char *argv[])
{
	unsigned int param[8];
	void (*ptr)(
			unsigned int a0,
			unsigned int a1,
			unsigned int a2,
			unsigned int a3,
			unsigned int a4,
			unsigned int a5,
			unsigned int a6,
			unsigned int a7
			);
	int i;
	if (argc < 2)
	{
		printf("Bad param\n");
		return -1;
	}

	ptr = (void *)hex_atoi(argv[1]);

	int j;
	for (i=2,j=0;i<argc;i++)
	{
		if (argv[i][1] == 'd')
		{
			param[j++] = hex_atoi(argv[++i]);
		}
		else if (argv[i][1] == 's')
		{
			param[j++] = (unsigned int)argv[++i];
		}
		else
		{
			printf("Param error...\n");
			return -1;
		}
	}
	printf("ptr is: 0x%x, arg cnt : %d\n",(unsigned int)ptr, j);
	ptr(param[0],param[1],param[2],param[3],param[4],param[5],param[6],param[7]);
	return 0;
}


#define MEM_GENERAL	"memory display or set"
#define MEM_DETAIL		"\tmem display addr [len]\n"\
						"\tmem set addr value\n"

static int _jv_memory(int argc, char *argv[])
{
	unsigned int *ptr;
	int value;
	int i;

	ptr = (unsigned int *)hex_atoi(argv[2]);
	if (argc == 3)
		value = 1;
	else
		value = hex_atoi(argv[3]);
	if (!strcmp("display", argv[1]))
	{
		for (i=0;i<value;i++)
		{
			if ((i % 8 ) == 0)
				printf("\n");
			printf("%08x,",ptr[i]);
		}
		printf("\n");
	}
	else if (!strcmp("set", argv[1]))
	{
		*ptr = value;
		printf("set *0x%x value: 0x%x\n",(unsigned int)ptr,*ptr);
	}
	return 0;
}

#define SYSTEM_GENERAL	"system cmd"
#define SYSTEM_DETAIL		"\tsystem ls\n"\
						"\tsystem ls\n"

static int _jv_system(int argc, char *argv[])
{
	char cmd[1024];
	int i;

	cmd[0] = '\0';
	for (i=1;i<argc;i++)
	{
		strcat(cmd, argv[i]);
		strcat(cmd, " ");
	}
	printf("CMD: [%s]\n", cmd);
	utl_system(cmd);
	return 0;
}

#define __PTHREAD_INFO__	1
#if __PTHREAD_INFO__

#define THREAD_NUM		30

typedef struct
{
	int pid;
	char name[64];
}threadInfo_t;

static int threadCnt = 0;
static threadInfo_t threadInfo[THREAD_NUM];
static pthread_mutex_t threadCntMutex = PTHREAD_MUTEX_INITIALIZER;

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#define gettid()   syscall(__NR_gettid) 

//获取线程的PID
int utl_thread_getpid()
{
	return (int)gettid();
}

void pthreadinfo_add(const char *threadName)
{
	pthread_mutex_lock(&threadCntMutex);
	if(threadCnt >= THREAD_NUM)
	{
		memmove(threadInfo, threadInfo+1, (THREAD_NUM-1)*sizeof(threadInfo_t));
		threadCnt--;
	}

	prctl(PR_SET_NAME,threadName);
	strncpy(threadInfo[threadCnt].name, threadName, sizeof(threadInfo[threadCnt].name));
	threadInfo[threadCnt].pid = utl_thread_getpid();
	threadCnt++;
	pthread_mutex_unlock(&threadCntMutex);
	printf("Thread: %s, id: %d\n", threadName, utl_thread_getpid());
}

void pthreadinfo_list(void)
{
	pthread_mutex_lock(&threadCntMutex);
	int i;
	printf("pthreadinfo_list\n==============================\n");
	for (i=0;i<threadCnt;i++)
	{
		printf("id: %.8d,     name: %s\n", threadInfo[i].pid, threadInfo[i].name);
	}
	printf("\n\n");
	pthread_mutex_unlock(&threadCntMutex);
}

static int _jv_threadinfo(int argc, char *argv[])
{
	pthreadinfo_list();
	return 0;
}

#endif

static pthread_t cmd_thread;
static int inited = 0;

static int sCmdEnable = 0;

void utl_cmd_enable(int bEnable)
{
	printf("utl_cmd_enable(%d);\n", bEnable);
	sCmdEnable = bEnable;
}

int utl_cmd_init()
{
	if (!sCmdEnable)
		return -1;
	if (inited)
		return -1;
	else
		inited = 1;
	cmd_running = 1;
	pthread_create(&cmd_thread, NULL, (void *)_utl_cmd_process, NULL);
	utl_cmd_insert("help", 
		"display help information", 
		"\tdisplay help information"
		"\n\thelp [cmd]\t display cmd help information",
		_jv_help);
	utl_cmd_insert("jump",JUMP_GENERAL,JUMP_DETAIL,_jv_jump);
	utl_cmd_insert("mem", MEM_GENERAL, MEM_DETAIL, _jv_memory);
	utl_cmd_insert("system", SYSTEM_GENERAL, SYSTEM_DETAIL, _jv_system);
	utl_cmd_insert("threadinfo", "list the thread info", "threadinfo", _jv_threadinfo);

	return 0;
}

int utl_cmd_deinit()
{
	if (!inited)
		return -1;
	cmd_running = 0;
	//fwrite("exit\n\r", 1, 6, stdin);
	//pthread_join(cmd_thread, NULL);
	return 0;
}

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
int utl_cmd_insert(char *cmd, char *help_general, char *help_detail, int (*func)(int argc, char *argv[]))
{
	int i;
	utl_cmd_init();
	if (cmd_cnt >= MAX_CMD_CNT)
	{
		printf("too much cmd now!\n");
		return -1;
	}
	for(i=0;i<cmd_cnt;i++)
	{
		if (strcmp(cmd, cmd_list[i].cmd) == 0)
		{
			printf("cmd has exit!!!\n");
			return -1;
		}
	}
	cmd_list[cmd_cnt].cmd = cmd;
	cmd_list[cmd_cnt].help_general = help_general;
	cmd_list[cmd_cnt].help_detail = help_detail;
	cmd_list[cmd_cnt].func = func;
	cmd_cnt++;
	return 0;
}


char *utl_cmd_get_help(char *cmd)
{
	int i;
	if (cmd == NULL)
	{
		int len;
		char item[255];
		char *help;
		len = (cmd_cnt * 255);
		help = malloc(len);
		if (help == NULL)
		{
			printf("Failed get memory with size: %d\n",len);
			return NULL;
		}
		help[0] = '\0';
		strcat(help, "cmd list and help information\n\n");
		for (i=0;i<cmd_cnt;i++)
		{
			snprintf(item, 255, "  %s\r\t\t%s\n",cmd_list[i].cmd,cmd_list[i].help_general);
			strcat(help, item);
		}
		return help;
	}
	else
	{
		for (i=0;i<cmd_cnt;i++)
		{
			if (!strcmp(cmd, cmd_list[i].cmd))
			{
				return strdup(cmd_list[i].help_detail);
			}
		}
	}
	return strdup("Bad param\n");;
}

