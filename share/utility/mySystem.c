#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <stdarg.h>
#include <syslog.h>


#define LINUX_THREAD_STACK_SIZE (512*1024)
#define REDIRECT_FLAG			("/progs/rec/00/redirect_log.flag")

static int s_bOpenLog = 0;

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

typedef struct
{
	int fd;
	int bNeedResult;
	struct sockaddr_in *cin;
	socklen_t *addr_len;
	char *pCmd;
}_SYS_PARM;

int bThreadRun = 0;

//获取命令的输出信息，结果存放在strResult中
//strCmd    :命令
//strResult :输出结果
//nSize     :输出缓冲区的最大长度
//返回值	:RET_SUCC/RET_ERR
//static int GetCmdResult(char* strCmd, char* strResult, int nSize)
//{
//	int i=0;
//
//	if(strResult)
//	{
//		FILE *fd;
//		int ch;
//
//		if((fd = popen(strCmd, "r")) == NULL)
//		{
//			printf("ERROR: get_system(%s),because:%s\n", strCmd, strerror(errno));
//			return -1;
//		}
//		for(i=0; i<nSize-1; i++)
//		{
//			if((ch = fgetc(fd)) != EOF)
//			{
//				strResult[i] = (char)ch;
//			}
//			else
//			{
//			    strResult[i] = 0;
//			    break;
//			}
//		}
//
//		strResult[nSize-1] = 0;
//		int child_status = pclose(fd);
//		if (0 == WIFEXITED(child_status))
//		{
//			printf("call shell failed\n");
//			return -1;
//		}
//		if (0 != WEXITSTATUS(child_status))
//		{
//			printf("command exec failed\n");
//			return -1;
//		}
//	}
//
//	return 0;
//}

//获取命令的输出信息，结果存放在strResult中
//strCmd    :命令
//strResult :输出结果
//nSize     :输出缓冲区的最大长度
//返回值
static int GetCmdResult(char* strCmd, char* strResult, int nSize)
{
	FILE *fd;
	int ch;
	int i;

	if ((fd = popen(strCmd, "r")) == NULL) {
		printf("ERROR: get_system(%s),because:%s\n", strCmd, strerror(errno));
		return -1;
	}
	for (i = 0; i < nSize - 1; i++) {
		if ((ch = fgetc(fd)) != EOF) {
			strResult[i] = (char) ch;
		} else {
			strResult[i] = 0;
			break;
		}
	}

	strResult[nSize - 1] = 0;
	pclose(fd);

	if(i==0)
	{
		printf("GetCmdResult,has no result\n");
		return -1;	//没有数据返回错误
	}

	return 0;
}

//!!!!!!!下面的有内存泄漏问题，没找到原因!!!!!!!!!!!!!!!!!!
//获取命令的输出信息，结果存放在strResult中
//strCmd    :命令
//strResult :输出结果
//nSize     :输出缓冲区的最大长度
//返回值	:
//static int GetCmdResult(char* strCmd, char* strResult, int nSize)
//{
//#define LINUX_SHELL   "/bin/sh"
//	int fds[2];
//	pid_t pid;
//	/* 创建管道。标识管道两端的文件描述符会被放置在fds 中。*/
//	pipe(fds);
//	/* 产生子进程。*/
//	pid = fork();
//	if (pid == (pid_t) 0)
//	{
//		/* 这里是子进程。关闭我们的读取端描述符。*/
//		close(fds[0]);
//		dup2(fds[1], STDOUT_FILENO);
//		execl(LINUX_SHELL, "sh", "-c", strCmd, (char *) 0);
//		/* execvp 函数仅当出现错误的时候才返回。*/
//		_exit(127);
//	}
//	else
//	{
//		int child_status;
//		/* 这是父进程。*/
//		/* 关闭我们的写入端描述符。*/
//		close(fds[1]);
//
//		/* 将写入端描述符转换为一个FILE 对象，然后将信息写入。*/
//		FILE* stream = fdopen(fds[0], "r");
//
//		int i;
//		int ch;
//		for(i=0; i<nSize-1; i++)
//		{
//			if((ch = fgetc(stream)) != EOF)
//			{
//				strResult[i] = (char)ch;
//			}
//			else
//			{
//			    strResult[i] = 0;
//			    break;
//			}
//		}
//		strResult[nSize-1] = 0;
//
//		close(fds[0]);
//		/* 等待子进程结束。*/
//		waitpid(pid, &child_status, 0);
//
//		//将进程返回状态return，由调用者判断是否执行成功
//		return child_status;
////		if (0 == WIFEXITED(child_status))
////		{
////			printf("call shell failed,cmd:%s, because:%s\n", strCmd, strerror(errno));
////			return -1;
////		}
//		//查看返回值，不是所有cmd执行成功都返回0，所以不能根据它来判断是否执行成功。注释掉
////		if (0 != WEXITSTATUS(child_status))
////		{
////			printf("command exec failed,cmd:%s, WEXITSTATUS:%d\n", strCmd, WEXITSTATUS(child_status));
////			return -1;
////		}
//
//	}
//}

//创建分离线程,参数同pthread_create
//注意:忽略了pthread_attr_t参数
int pthread_create_detached(pthread_t *pid, const pthread_attr_t *no_use, void*(*pFunction)(void*), void *arg)
{
	int ret;
	size_t stacksize = LINUX_THREAD_STACK_SIZE;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stacksize);					//栈大小
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	//分离线程

	ret = pthread_create(pid, &attr, (void*)pFunction, (void *)(arg));
	pthread_attr_destroy (&attr);

	return ret;
}

int __real_printf(const char * fmt,...);
int __real_puts(const char * s);

int __wrap_printf(const char * fmt,...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

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

	ret = __real_puts(str);

	if (s_bOpenLog)
	{
		syslog(LOG_INFO, str);
	}

	return ret;
}

int my_system(char* cmd)
{
    int status = 0;
    pid_t pid;
	
	if(cmd == NULL)  
    {  
		printf("cmd == NULL\n");
        return 1;  
    }  

	pid = vfork();
    if (pid < 0)
    {
        printf("vfork process error! errno: %d\n", errno);
		perror("vfork");
        status = -1;
    }
    else if (pid == 0)
    {
        if (execl("/bin/sh", "sh", "-c",cmd, (char*)0)<0) 
        {
            printf("fail to execl %s! errno: %d\n",cmd, errno);
			perror("execl");
            exit(1);
        }
        else
        {
			//printf("success to execl %s! errno: %d\n",cmd, errno);
            exit(0);
        }
    }
    else
    {
        while(waitpid(pid, &status, 0) < 0)  
        {  
			//printf("fail to waitpid %s! errno: %d\n",cmd, errno);
			perror("waitpid");
            if(errno != EINTR)  
            {  
                status = -1;  
                break;  
            }
			usleep(1000);
        }
    }
	
    return status;
}

void _work_thread(_SYS_PARM *parg)
{
	int n;
	int s_fd;
	struct sockaddr_in cin;
	socklen_t addr_len;
	char buf[MAX_CMD_LEN];
	int bNeedResult = 0;

	prctl(PR_SET_NAME, (char*)__func__);

	if(parg == NULL)
	{
		bThreadRun = 1;
		return;
	}

	s_fd = parg->fd;
	memcpy(&cin, parg->cin, sizeof(cin));
	memcpy(&addr_len, parg->addr_len, sizeof(addr_len));
	strcpy(buf, parg->pCmd);
	bNeedResult = parg->bNeedResult;

	bThreadRun = 1;	//线程已经运行
	
	//printf("client IP is %s, port is %d\n", addr_p, ntohs(cin.sin_port));
	// printf("system: %s\n", buf);

	ST_CMD_RESULT stCmdResult = {0};
	if (bNeedResult)
		stCmdResult.ret = GetCmdResult(buf, stCmdResult.strResult, MAX_STR_RES_LEN);
	else
		// stCmdResult.ret = system(buf);
		stCmdResult.ret = my_system(buf);

	char addr_p[INET_ADDRSTRLEN] = {0};
	inet_ntop(AF_INET, &cin.sin_addr, addr_p, sizeof(addr_p));

	printf("system: %s, ret: %#x, target: %s:%d\n", buf, stCmdResult.ret, addr_p, ntohs(cin.sin_port));

//	ret = system(buf);
	n = sendto(s_fd, &stCmdResult, sizeof(stCmdResult.ret) + strlen(stCmdResult.strResult) + 1, 0, (struct sockaddr *)&cin, addr_len);
	if (n == -1)
	{
		printf("fail to send: result: 0x%d, error: %s\n", stCmdResult.ret, strerror(errno));
	}
}

int main(int argc, char *argv[])
{
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	int s_fd;
	int port = PORT_FOR_SYSTEM;
	socklen_t addr_len;
	//char resultCmd[MAX_CMD_LEN];
	char addr_p[INET_ADDRSTRLEN];
	int n;
	_SYS_PARM stParm;
	pthread_t pid;
	ST_CMD stCmd = {0};

	if (access(REDIRECT_FLAG, F_OK) == 0)
	{
		openlog("<system>", LOG_CONS | LOG_PID, LOG_USER);
		s_bOpenLog = 1;
	}

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
	sin.sin_port = htons(port);

	s_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_fd == -1)
	{
		printf("fail to create socket: %s\n", strerror(errno));
		//exit(1);
		return -1;
	}

	if(bind(s_fd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	{
		printf("cannot to bind: %s\n", strerror(errno));
		close(s_fd);
		//exit(1);
		return -1;
	}

	stParm.cin = &cin;
	stParm.fd = s_fd;
	stParm.pCmd = stCmd.strCmd;
	stParm.addr_len = &addr_len;
	
	while (1)
	{
		addr_len = sizeof(sin);
		
		n = recvfrom(s_fd, &stCmd, sizeof(stCmd), 0, (struct sockaddr *)&cin, &addr_len);
		if (n <= 0)
		{
			perror("fail to receive, or shutdown now");
			//exit(1);
			return -1;
		}
		stParm.bNeedResult = stCmd.bNeedResult;

		inet_ntop(AF_INET, &cin.sin_addr, addr_p, sizeof(addr_p));

		bThreadRun = 0;
		
#if 1
		//用线程执行system
		n = pthread_create_detached(&pid, NULL, (void *)_work_thread, (void*)&stParm);

		// 线程创建失败则立即重启
		if (n != 0)
		{
			char time_str[32] = {0};
			char cmd_str[150] = {0};
			time_t timeNow = time(NULL);

			printf(	"mySystem, cmd: %s, create thread failed: %s(%#x)\n"
					"System need reboot!\n", 
					stCmd.strCmd, strerror(n), n);

			// 记录一下
			strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", localtime(&timeNow));
			snprintf(cmd_str, sizeof(cmd_str), 
						"echo \"reboot at %s, cmd: %s, error: %s(%#x)\" >> /etc/conf.d/jovision/reboot.log", 
						time_str, stCmd.strCmd, strerror(n), n);
			my_system(cmd_str);
			my_system("reboot");
		}
#else
		//printf("client IP is %s, port is %d\n", addr_p, ntohs(cin.sin_port));
		printf("system: %s\n", buf);

		int ret = system(buf);
		n = sendto(s_fd, &ret, sizeof(int), 0, (struct sockaddr *) &cin, addr_len);
		if (n == -1)
		{
			printf("fail to send: result: 0x%d, error: %s\n", ret, strerror(errno));
		}
#endif

		//线程运行后，才接收下一条命令
		while(0 == bThreadRun)
		{
			usleep(10000);
		}
	}
	
	if (s_fd != -1)
	{
		if(close(s_fd) == -1)
		{
			perror("fail to close");
			//exit(1);
			return -1;
		}
		s_fd = -1;
	}
	
	return 0;
}
