#include <jv_common.h>
#include <utl_cmd.h>
#include <utl_iconv.h>
#include <msnapshot.h>
#include <SYSFuncs.h>
#include "weber.h"

static jv_thread_group_t weberGroup;
static int cmdfd = -1;

static char resultCmd[MAX_CMD_LEN];
int webcmd_init(void);

int weber_set_result(char *result)
{
	int len;
	len = utl_iconv_gb2312toutf8(result, resultCmd, MAX_CMD_LEN);
	resultCmd[len] = '\0';
//	printf("%s\n", resultCmd);
//	strncpy(resultCmd, result, MAX_CMD_LEN);
	//This is a test.
	return 0;
}

char *weber_extern(char *cmd, char *result)
{
	strcpy(resultCmd, "unknown error happened");
	utl_cmd_system(cmd);
	strcpy(result, resultCmd);
	return result;
}

//对buf内数据的预处理，jvsweb.cgi脚本中，每个参数都用引号引起来，这样做可以让
//参数中带有空格而解析不出错，但是也导致为空的参数也会传到这里来，该函数的作用
//就是把为空的参数删掉，防止后面统计参数个数和获取参数值时出现问题
void weber_prehandle(char *buf)
{
	int i = 0;
	int j = 0;
	
	if(NULL == buf)
	{
		return;
	}
	
	for(i = 0 ; i < MAX_CMD_LEN-1 ; i++) //将所有两个连续的单引号都置为空格
	{
		if(*(buf+i) == '\'' && *(buf+i+1) == '\'')
		{
			*(buf+i) = ' ';
			*(buf+i+1) = ' ';
		}
	}
	
	for(i = 0 ; i < MAX_CMD_LEN-1 ; )  //将所有两个连续的空格删掉一个
	{
		if(*(buf+i) == ' ' && *(buf+i+1) == ' ')
		{
			for(j = i ; j < MAX_CMD_LEN-1 ; j++)
			{
				*(buf+j) = *(buf+j+1);
			}
		}
		else
			i++;
	}
	
	return;
}

static void _weber_process(void *param)
{
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	int s_fd;
	int port = 8732;
	socklen_t addr_len;
	char buf[MAX_CMD_LEN];
	char buf_utf8[MAX_CMD_LEN];
	char buf_gb2312[MAX_CMD_LEN];
	char addr_p[INET_ADDRSTRLEN];
	int n;
	pthreadinfo_add((char *)__func__);

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	s_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_fd == -1)
	{
		Printf("fail to create socket: %s\n", strerror(errno));
		//exit(1);
		return ;
	}

	if(bind(s_fd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	{
		Printf("cannot to bind: %s\n", strerror(errno));
		close(s_fd);
		//exit(1);
		return ;
	}
	long int start;
	cmdfd = s_fd;
	while (weberGroup.running)
	{
		addr_len = sizeof(sin);

		n = recvfrom(s_fd, buf, MAX_CMD_LEN, 0, (struct sockaddr *) &cin, &addr_len);
//		struct timeval t_start, t_end;
//		gettimeofday(&t_start, NULL);

		memset(buf_gb2312,0,MAX_CMD_LEN);
		weber_prehandle(buf);
		
		utl_iconv_utf8togb2312(buf,buf_gb2312,strlen(buf));
		if (n <= 0)
		{
			perror("fail to receive, or shutdown now");
			close(s_fd);
			//exit(1);
			return ;
		}

		inet_ntop(AF_INET, &cin.sin_addr, addr_p, sizeof(addr_p));

		printf("client IP is %s, port is %d\n", addr_p, ntohs(cin.sin_port));
		printf("content is : %s\n", buf_gb2312);

		//my_fun(buf);
		//防止命令不识别，反而将最后一条回复发出去了
		strcpy(resultCmd, "unknown error happened");
		utl_cmd_system(buf_gb2312);
//		strcat(resultCmd,"测试中");
//		strcpy(resultCmd, buf);

		n = sendto(s_fd, resultCmd, strlen(resultCmd), 0, (struct sockaddr *) &cin, addr_len);
		if (n == -1)
		{
			printf("fail to send: resultCmd: 0x%p, len: %d, error: %s\n", resultCmd, strlen(resultCmd), strerror(errno));
			//exit(1);
		}
	}

	if (cmdfd != -1)
	{
		if(close(s_fd) == -1)
		{
			perror("fail to close");
			//exit(1);
			return ;
		}
		cmdfd = -1;
	}
}

int weber_init(void)
{
	weberGroup.running = TRUE;
	pthread_mutex_init(&weberGroup.mutex, NULL);
	pthread_create(&weberGroup.thread, NULL, (void *)_weber_process, NULL);
	webcmd_init();
	return 0;
}

int weber_deinit(void)
{
	weberGroup.running = FALSE;
	Printf("cmdfd: %d\n", cmdfd);
	if (cmdfd != -1)
	{
		shutdown(cmdfd, SHUT_RDWR);
		close(cmdfd);
		cmdfd = -1;
	}

	pthread_join(weberGroup.thread, NULL);
	pthread_mutex_destroy(&weberGroup.mutex);
	return 0;
}


