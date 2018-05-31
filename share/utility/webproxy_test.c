
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include "webproxy.h"


#define PROXY_PRINTF(fmt...)  \
do{\
	if(1){	\
		printf("[%s]:%d ", __FILE__, __LINE__);\
		printf(fmt);} \
} while(0)
	
	
#define PROXY_PRINTF1(fmt...)  \
	do{\
		if(1){	\
			printf("[%s]:%d ", __FILE__, __LINE__);\
			printf(fmt);} \
	} while(0)

#define TEST_PROXY

#ifdef TEST_PROXY

static int cliet_sock = -1;

static void test_callback(unsigned char *buffer, int len, void *param)
{
	if (cliet_sock != -1)
	{
		if (buffer)
			send(cliet_sock, buffer, len, 0);
		else
		{
			if (cliet_sock != -1)
			{
				shutdown(cliet_sock, SHUT_RDWR);
				close(cliet_sock);
				cliet_sock = -1;
			}
		}
	}
	else
	{
		PROXY_PRINTF1("cliet_sock is closed, so, failed send message!\n");
	}
}

/**
 *@brief 测试，用于接收数据，然后转发
 */
static int test_proxy(unsigned short port)
{
	int ret;
	int sl;//sock listen
//	int sc;//sock client
	struct sockaddr_in addrl, addrc;
	socklen_t addrlen;
	
	sl = socket(AF_INET, SOCK_STREAM, 0);
	if (sl == -1)
	{
		PROXY_PRINTF("create socket failed: %s\n", strerror(errno));
		return -1;
	}
	addrl.sin_family = AF_INET;
	addrl.sin_port = htons(port);
	addrl.sin_addr.s_addr = INADDR_ANY;
	if (0 != bind(sl, (const struct sockaddr *)&addrl, sizeof(struct sockaddr)))
	{
		PROXY_PRINTF("bind error: %s\n", strerror(errno));
		return -1;
	}
	if (0 != listen(sl, 10))
	{
		PROXY_PRINTF("listen error: %s\n", strerror(errno));
		return -1;
	}

	while(1)
	{
		fd_set rfds;
		struct timeval tv;
		
		FD_ZERO(&rfds);
		FD_SET(sl, &rfds);
		
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		ret = select(sl+1, &rfds, NULL, NULL, &tv);
		/* Don't rely on the value of tv now! */
		
		if (ret == -1)
		   perror("select()");
		else if (ret)
		{
			PROXY_PRINTF1("Data is available now.\n");
			if (FD_ISSET(sl, &rfds))
			{
//				int receved;
				unsigned char buffer[5*1024];
				addrlen = sizeof(struct sockaddr);
				memset(&addrc, 0, addrlen);
				cliet_sock = accept(sl, (struct sockaddr *)&addrc, &addrlen);
				if (cliet_sock == -1)
				{
					PROXY_PRINTF("ERROR: accept failed: %s\n", strerror(errno));
					return -1;
				}
				PROXY_PRINTF1("test socket created\n");
				while(1)
				{
					ret = recv(cliet_sock, buffer, sizeof(buffer), 0);
					if (ret == -1)
					{
						PROXY_PRINTF("error: recv failed : %s\n", strerror(errno));
						//shutdown(cliet_sock, SHUT_RDWR);
						close(cliet_sock);
						cliet_sock = -1;
						break;
					}
					else if (ret == 0)
					{
						PROXY_PRINTF1("test socket is closed\n");
						//shutdown(cliet_sock, SHUT_RDWR);
						close(cliet_sock);
						cliet_sock = -1;
						proxy_close_socket();
						break;
					}
					buffer[ret] = '\0';
					PROXY_PRINTF1("test socket received cmd: [%s]\n", buffer);
					proxy_send2server(buffer, ret, NULL);
				}
			}
		}
		   /* FD_ISSET(0, &rfds) will be true. */
		else
		   PROXY_PRINTF1("No data within five seconds.\n");
	}
	return 0;}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("usage:\n    %s [CLIENT PORT]\n", argv[0]);
		return -1;
	}
	proxy_init(test_callback, "127.0.0.1", 80);
	test_proxy(atoi(argv[1]));
	return 0;
}
#endif

