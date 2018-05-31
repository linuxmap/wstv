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
#include <linux/un.h>
#include <arpa/inet.h>

#define MAX_CMD_LEN	(1024*50)

#define _SOCK_WITH_IN	1
static int sPort = 8099;

#if 1
#define DEBUG(fmt...)  \
do{\
	char cmd[2560];\
	char cmd2[2560]\
	sprintf(cmd, fmt); \
	sprintf(cmd2, "echo %s >> /tmp/abc.txt", cmd);\
	system(cmd2);\
} while(0)
#else
#define DEBUG(fmt...)
#endif

int send_recv(char *cmd)
{
	int ret;
	int sock;
#if _SOCK_WITH_IN
	struct sockaddr_in addr;
	int namespace = AF_INET;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(sPort);

#else
	struct sockaddr_un addr;
	int namespace = AF_UNIX;
	bzero(&addr, sizeof(addr));
	addr.sun_family = namespace;
	strcpy(addr.sun_path , "/var/abc.sck");
#endif

	while(1)
	{
		sock = socket(namespace, SOCK_STREAM, 0);
		if (sock == -1)
		{
			static int cnt = 0;
			//printf("create socket failed: %s\n", strerror(errno));
			strcpy(cmd, "create socket failed\n");
			//return -1;
			sPort++;
			addr.sin_port = htons(sPort);
			if (cnt++ > 5)
			{
				printf("create socket failed: %s\n", strerror(errno));
				return -1;
			}
		}
		else
			break;
	}
	ret = connect(sock, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un));
	if (ret == -1)
	{
		//printf("connect error: %s, addr: %s\n", strerror(errno), argv[1]);
		sprintf(cmd, "connect fail: %s\n", strerror(errno));
		close(sock);
		return -1;
	}

	ret = send(sock, cmd, strlen(cmd), 0);
	if (ret == -1)
	{
		strcpy(cmd, "Failed Send to Server\n");
		close(sock);
		return -1;
	}

	ret = recv(sock, cmd, MAX_CMD_LEN, 0);
	if(ret == -1)
	{
		strcpy(cmd, "wagent fail to recvfrom\n");
	}
	else
		cmd[ret] = '\0';


	if(close(sock) == -1)
	{
		//perror("fail to close.\n");
		//exit(1);
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int i;
	char data[MAX_CMD_LEN];
	char *temp = data;
	int maxlen;
	char *env;
	int len;
	int ret;

	//当名字为transfer_时，后面的数字作为端口号
	char *p = strstr(argv[0], "transfer_");
	if (p)
	{
		p += 9;
		int port = atoi(p);
		if (port != 0)
			sPort = port;
	}

	env = getenv("CONTENT_LENGTH");
	if (env == NULL)
	{
		printf("Failed get CONTENT_LENGTH");
		return 0;
	}
	maxlen = atoi(env);

	if (maxlen > MAX_CMD_LEN)
		temp = malloc(maxlen+1);
	temp[0] = '\0';

	len = fread(temp, 1, maxlen, stdin);
	if (len != maxlen)
	{
		printf("Failed Read stdin: %d != %d\n", len, maxlen);
		free(temp);
		return 0;
	}

	ret = send_recv(temp);
//	const char* httpok_str = "HTTP/1.1 200 OK\r\n";
	const char* httpok_str = "HTTP/";
	if (strncmp(httpok_str, temp, strlen(httpok_str)) == 0)
	{
		char *p = strstr(temp, "\r\n");

		printf("%s", &p[2]);
	}
	else
		printf("%s", temp);

	if (maxlen > MAX_CMD_LEN)
		free(temp);
	return 0;
}
