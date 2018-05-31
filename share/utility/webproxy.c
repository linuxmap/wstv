/**
 *@file 用于测试代理WEB服务器之用
 *
 * 可使用本程序，在已有内网穿透功能的基础之上，使用其建立的连接访问WEB服务
 *
 * 实现方式之一：
 *1，云视通建立连接
 *2，云视通主控端将建立连接时的端口号告知本程序（或者启动它）
 *3，云视通断开这个连接
 *4，云视通分控端，使用之前连接时的端口和IP访问WEB页面
 *5，访问成功
 *
 */

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
#include <arpa/inet.h>
#include "webproxy.h"

#define PROXY_PRINTF(fmt...)  \
do{\
	if(1){	\
		printf("[%s]:%d ", __FILE__, __LINE__);\
		printf(fmt);} \
} while(0)
	
	
#define PROXY_PRINTF1(fmt...)  \
	do{\
		if(0){	\
			printf("[%s]:%d ", __FILE__, __LINE__);\
			printf(fmt);} \
	} while(0)

typedef struct{
	int sockver;		//sockweb的版本。主要是避免_proxy_process中出现的，错误的将新建立的socket关闭的问题
	int sockweb;		//与web服务连接的socket
	received_from_webserver_callback received_from_webserver_ptr;
	void *callback_param;

	unsigned char webaddr[20];//web服务的IP
	unsigned short webport;	//web服务的端口

	//以下是线程、同步相关
	pthread_mutex_t mutex;
	pthread_t thread;
	int running;
}proxy_privacy_t;

static proxy_privacy_t sProxy;
#if 0

static int _proxy_recv(int sock, unsigned char *buffer, int maxlen)
{
	int ret;
	fd_set rfds;
	struct timeval tv;
	
	if (sock == -1)
	{
		PROXY_PRINTF(("ERROR: -1 socket\n"));
		return -1;
	}

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	
	ret = select(sock+1, &rfds, NULL, NULL, &tv);
	/* Don't rely on the value of tv now! */
	
	if (ret == -1)
	   perror("select()");
	else if (ret)
	{
		ret = recv(sock, buffer, maxlen, 0);
	}

	return ret;
}

static void _proxy_process(void)
{
	int ret;
	int sockver;
	unsigned char buffer[10*1024];
	while(sProxy.running)
	{
		if (sProxy.sockweb == -1)
		{
			usleep(1000);
			continue;
		}
		sockver = sProxy.sockver;
		ret = recv(sProxy.sockweb, buffer, sizeof(buffer), 0);
		if(ret == -1)
		{
			PROXY_PRINTF("recv error: %s\n", strerror(errno));
		}
		else if (ret == 0)
		{
			pthread_mutex_lock(&sProxy.mutex);
			if (sProxy.sockweb != -1 && sockver == sProxy.sockver)
			{
				PROXY_PRINTF1("sProxy.sockweb closed: %d\n", sProxy.sockweb);
				//shutdown(sProxy.sockweb, SHUT_RDWR);
				close(sProxy.sockweb);
				sProxy.sockweb = -1;

				//通知上层，关闭SOCKET
				if (sProxy.received_from_webserver_ptr)
					sProxy.received_from_webserver_ptr(NULL, 0);
			}
			pthread_mutex_unlock(&sProxy.mutex);

		}
		else
		{
			buffer[ret] = '\0';
			PROXY_PRINTF1("sockweb received data: [%s]\n", buffer);
			if (sProxy.received_from_webserver_ptr)
				sProxy.received_from_webserver_ptr(buffer, ret);
		}
	}
}
#endif

/**
 *@brief 初始化
 *@param callback 收到数据时的回调函数
 *@param webaddr WEB服务的IP
 *@param webport WEB服务的端口
 *
 *@return 0 成功
 *
 */
int proxy_init(received_from_webserver_callback callback, char *webaddr, unsigned short webport)
{
	memset(&sProxy, 0, sizeof(sProxy));
	//pthread_mutex_init(&sProxy.mutex, NULL);
	//sProxy.running = 1;
	sProxy.sockweb = -1;
	sProxy.received_from_webserver_ptr = callback;
	strncpy((char *)sProxy.webaddr, webaddr, sizeof(sProxy.webaddr));
	sProxy.webport = webport;
	//pthread_create(&sProxy.thread, NULL, (void *)_proxy_process, NULL);
	return 0;
}

/**
 *@brief 结束
 *
 *@return 0 成功
 *
 */
int proxy_deinit(void)
{
	//sProxy.running = 0;
	//pthread_join(sProxy.thread, NULL);
	//pthread_mutex_destroy(&sProxy.mutex);
	return 0;
}


/**
 *@brief 将收到的数据，通过127.0.0.1 转发给WEB服务
 *@param data 要转发的数据
 *@param len 数据长度
 *
 *@return 0 成功-1 创建socket失败
 */
int proxy_send2server(unsigned char *data, int len, void *callback_param)
{
	int ret = 0;
	int sock;
	unsigned char buffer[10*1024];
	struct sockaddr_in addr;

	sProxy.callback_param = callback_param;
	if (sProxy.sockweb == -1)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == -1)
		{
			PROXY_PRINTF("create socket failed: %s\n", strerror(errno));
			ret = -1;
			goto END;
		}
		addr.sin_family = AF_INET;
		addr.sin_port = htons(sProxy.webport);
		addr.sin_addr.s_addr = inet_addr((const char *)sProxy.webaddr);//INADDR_LOOPBACK;
		
		PROXY_PRINTF1("test %d\n", __LINE__);
		ret = connect(sock, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		PROXY_PRINTF1("test %d\n", __LINE__);
		if (ret == -1)
		{
			PROXY_PRINTF("connect error: %s\n", strerror(errno));
			close(sock);
			goto END;
		}
		sProxy.sockweb  = sock;
		sProxy.sockver++;
	}
	PROXY_PRINTF1("send: %s\n", data);
	ret = send(sProxy.sockweb, data, len, 0);
	PROXY_PRINTF1("sockweb sended\n");
	if (ret == -1)
	{
		PROXY_PRINTF("send failed: %s\n", strerror(errno));
	}
	while(1)
	{
		//ret = _proxy_recv(sProxy.sockweb, buffer, sizeof(buffer));
		PROXY_PRINTF1("test\n");
		ret = recv(sock, buffer, sizeof(buffer), 0);
		if (ret == -1)
		{
			PROXY_PRINTF("proxy_recv failed\n");
			close(sProxy.sockweb);
			sProxy.sockweb = -1;
			//usleep(1000*1000);
			//if (sProxy.received_from_webserver_ptr)
			//	sProxy.received_from_webserver_ptr(NULL, 0, sProxy.callback_param);
			break;
		}
		else if (ret == 0)
		{
			PROXY_PRINTF1("web closed the socket!\n");
			close(sProxy.sockweb);
			sProxy.sockweb = -1;
			//usleep(100*1000);
			if (sProxy.received_from_webserver_ptr)
				sProxy.received_from_webserver_ptr(NULL, 0, sProxy.callback_param);
			break;
		}
		else
		{
			PROXY_PRINTF1("web received data success!\n");
			buffer[ret] = '\0';
			PROXY_PRINTF1("[%s]\n", buffer);
			if (sProxy.received_from_webserver_ptr)
				sProxy.received_from_webserver_ptr(buffer, ret, sProxy.callback_param);
			ret = 0;
		}
	}
END:
	return ret;
}

/**
 *@brief 关闭与WEB服务的连接
 *
 *
 */
void proxy_close_socket(void)
{
	
	//pthread_mutex_lock(&sProxy.mutex);
	if (sProxy.sockweb != -1)
	{
		PROXY_PRINTF1("sProxy.sockweb closed: %d\n", sProxy.sockweb);
		shutdown(sProxy.sockweb, SHUT_RDWR);
		close(sProxy.sockweb);
		sProxy.sockweb = -1;
	}
	//pthread_mutex_unlock(&sProxy.mutex);
}


