#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "utl_system.h"
//#include "p_sys.h"

const unsigned int PORT_FOR_SYSTEM =	8899;
#define MAX_CMD_LEN	1024
#define MAX_STR_RES_LEN		(10*1024)
typedef struct
{
	int ret;
	char strResult[MAX_STR_RES_LEN];
}ST_CMD_RESULT;

typedef struct
{
	int bNeedResult;
	char strCmd[MAX_CMD_LEN];
}ST_CMD;

/*
static void _system_process(void *param)
{
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	int s_fd;
	int port = PORT_FOR_SYSTEM;
	socklen_t addr_len;
	char buf[MAX_CMD_LEN];
	char resultCmd[MAX_CMD_LEN];
	char addr_p[INET_ADDRSTRLEN];
	int n;

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	s_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_fd == -1)
	{
		printf("fail to create socket: %s\n", strerror(errno));
		//exit(1);
		return ;
	}

	if(bind(s_fd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	{
		printf("cannot to bind: %s\n", strerror(errno));
		//exit(1);
		return ;
	}

	while (1)
	{
		addr_len = sizeof(sin);
		
		n = recvfrom(s_fd, buf, MAX_CMD_LEN, 0, (struct sockaddr *) &cin, &addr_len);
		if (n <= 0)
		{
			perror("fail to receive, or shutdown now");
			//exit(1);
			return ;
		}

		inet_ntop(AF_INET, &cin.sin_addr, addr_p, sizeof(addr_p));

		printf("client IP is %s, port is %d\n", addr_p, ntohs(cin.sin_port));
		printf("content is : %s\n", buf);

		//my_fun(buf);
		//防止命令不识别，反而将最后一条回复发出去了
		//strcpy(resultCmd, "unknown error happened");
		//utl_cmd_system(buf);
		system(buf);
		strcpy(resultCmd, "ok");
		n = sendto(s_fd, resultCmd, strlen(resultCmd), 0, (struct sockaddr *) &cin, addr_len);
		if (n == -1)
		{
			printf("fail to send: resultCmd: 0x%p, len: %d, error: %s\n", resultCmd, strlen(resultCmd), strerror(errno));
			//exit(1);
		}
	}
	
	if (s_fd != -1)
	{
		if(close(s_fd) == -1)
		{
			perror("fail to close");
			//exit(1);
			return ;
		}
		s_fd = -1;
	}
}

static int __utl_system_init(void)
{
	pthread_t thread;
	pthread_create(&thread, NULL, (void *)_system_process, NULL);
	return 0;
}

int utl_system_init(void)
{
	pid_t pid;
	int status;

	if ((pid = fork()) < 0)
	{
		printf("vfork failed: %s\n", strerror(errno));
		status = -1;
	}
	else if (pid == 0)	//child process
	{
		__utl_system_init();
		while(1)
			sleep(10);
	}
	else
	{
		printf("jvsystem: %d\n", getpid());
		return 0;
	}
	return status;
}
*/

static int send_recv(char *cmd, int *ret, char* strResult, int nSize)
{
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	int port = PORT_FOR_SYSTEM;
	socklen_t addr_len;
	int s_fd;
	int n;
	ST_CMD_RESULT stCmdResult = {0};
	fd_set rfds;
    ST_CMD stCmd = {0};

    if (0 >= nSize || NULL == strResult)
    	stCmd.bNeedResult = 0;
    else
    	stCmd.bNeedResult = 1;
    strncpy(stCmd.strCmd, cmd, MAX_CMD_LEN);

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
	sin.sin_port = htons(port);

	s_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(s_fd == -1)
	{
		printf("wagent fail to create socket\n");
		return -1;
	}

	n = sendto(s_fd, &stCmd, sizeof(stCmd.bNeedResult) + strlen(stCmd.strCmd) + 1, 0, (struct sockaddr *) &sin, sizeof(sin));
	if(n == -1)
	{
		printf("wagent fail to send\n");
		close(s_fd);
		return -1;
	}

	FD_ZERO(&rfds);
	FD_SET(s_fd, &rfds);

	if(stCmd.bNeedResult)
	{
		struct timeval tv_out;
		tv_out.tv_sec = 5;
		tv_out.tv_usec = 0;
		n = select(s_fd+1, &rfds, NULL, NULL, &tv_out);    //需要结果时，设置超时时间。
	}
	else
	{
		n = select(s_fd+1, &rfds, NULL, NULL, NULL);    //zwq,20130619,设成阻塞的
	}

	if (n == -1)
	{
		printf("wagent select error\n");
		close(s_fd);
		return -1;
	}
	else if(n == 0)
	{
		printf("wagent recvfrom timeout\n");
		close(s_fd);
        return -1;
	}

	addr_len = sizeof(cin);
	n = recvfrom(s_fd, &stCmdResult, sizeof(stCmdResult), 0, (struct sockaddr *) &cin, &addr_len);
	*ret = stCmdResult.ret;
	strncpy(strResult, stCmdResult.strResult, nSize);
	if(n == -1)
	{
		printf("wagent fail to recvfrom\n");
		close(s_fd);
		return -1;
	}

	close(s_fd);

	return 0;
}

static int inited = 0;

int utl_system_init(void)
{
#if (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100)
	{
		//已放在bashrc中启动
		return 0;
	}
#endif

	if (inited)
	{
		return 0;
	}
	else
		inited = 1;
	//这里要注意，不要试图用killall来避免重复调用。会因为端口占用而启动失败的。相反，重复启动时，会因为端口占用，而保持只有一个进程启动
	int ret = system("/progs/bin/mySystem&");
	usleep(100*1000);
	return ret;
}

//system()函数内部的实现方式是fork,exec。当应用程序占用内存很多时fork会是很大的负担。
//例如: 3520d,程序占用25M以上是system就会失败.
//请使用utl_system代替system
//请用宏IsSystemFail()判断执行是否成功。!!!!!!!!如果要执行的命令返回值非0也表示成功的话不能用(需要去掉WEXITSTATUS((ret)，它得到的是返回值)
//#define IsSystemFail(ret) (-1 == (ret) || 0 == WIFEXITED((ret)) || 0 != WEXITSTATUS((ret)))
int utl_system(char *cmd, ...)
{
	va_list ap;
	char cmdstr[1024] = "";

	va_start(ap, cmd);
	vsnprintf(cmdstr, sizeof(cmdstr), cmd, ap);
	va_end(ap);
//	return system(cmd);
	utl_system_init();
	//char temp[MAX_CMD_LEN];
	//strcpy(temp, cmd);
	int ret = -1;
//	CPrintf("cmd start: %s\n", cmd);
	send_recv(cmdstr, &ret, NULL, 0);
    //printf("==================cmd:%s========ret:%d====fail:%d===\n", cmd, ret, IsSystemFail(ret));
//	CPrintf("cmd end\n");
	return ret;
}

//能获取到命令输出的执行函数
//请用宏IsSystemFail()判断执行是否成功。!!!!!!!!如果要执行的命令返回值非0也表示成功的话不能用(需要去掉WEXITSTATUS((ret)，它得到的是返回值)
//#define IsSystemFail(ret) (-1 == (ret) || 0 == WIFEXITED((ret)) || 0 != WEXITSTATUS((ret)))
int utl_system_ex(char* strCmd, char* strResult, int nSize)
{
	int ret = -1;

	send_recv(strCmd, &ret, strResult, nSize);
	return ret;
}



//打开fileName指定的文件，从中读取第lineNumber行
//返回值：成功返回1，失败返回0
static int utl_system_get_file_line(char *result,char *fileName,int lineNumber)
{
    FILE *filePointer;
    int i=0;
    char buffer[256];

    if((fileName==NULL)||(result==NULL))
    {
        return -1;
    }

    if(!(filePointer=fopen(fileName,"rb")))
    {
		return -1;
	}
    while((!feof(filePointer))&&(i<lineNumber))
    {
        if(!fgets(buffer,256,filePointer))
        {
            return -1;
        }
        i++;
    }

    strcpy(result,buffer);

    if(0!=fclose(filePointer))
    {
        return -1;
    }

    return 0;
}

//获取linux系统剩余内存大小，单位 KB 
int utl_system_get_free_mem()
{
	char buffer[256];
	char actmp[32];
	int  nfreeKb;
	if(0 == utl_system_get_file_line(buffer,"/proc/meminfo",2))//读取第二行
	{
		 sscanf(buffer,"%*s %s",actmp);
		 nfreeKb = atoi(actmp);
		 return nfreeKb;
	}
		

    return -1;
}

