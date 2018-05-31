#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>


//#define MAX_CMD_LEN	(1024*5)
#define MAX_CMD_LEN	(1024*10)

void send_recv(char *cmd)
{
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	int port = 8732;
	socklen_t addr_len;
	int s_fd;
	char add_p[INET_ADDRSTRLEN];
	int n;
	fd_set rfds;
	struct timeval timeout;

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
	sin.sin_port = htons(port);

	s_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(s_fd == -1)
	{
		strcpy(cmd, "wagent fail to create socket");
		return ;
	}

	n = sendto(s_fd, cmd, strlen(cmd) + 1, 0, (struct sockaddr *) &sin, sizeof(sin));
	if(n == -1)
	{
		strcpy(cmd, "wagent fail to send\n");
		close(s_fd);
		return ;
	}

	FD_ZERO(&rfds);
	FD_SET(s_fd, &rfds);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	n = select(s_fd+1, &rfds, NULL, NULL, &timeout);

	if (n == -1)
	{
		strcpy(cmd, "wagent select error");
		close(s_fd);
		return ;
	}
	else if(n == 0)
	{
		strcpy(cmd, "wagent recvfrom timeout");
		close(s_fd);
		return ;
	}

	addr_len = sizeof(cin);
	n = recvfrom(s_fd, cmd, MAX_CMD_LEN, 0, (struct sockaddr *) &cin, &addr_len);
	if(n == -1)
	{
		strcpy(cmd, "wagent fail to recvfrom\n");
	}
	else
		cmd[n] = '\0';
	//else
	//	printf("recive from server: %s\n", buf);

	if(close(s_fd) == -1)
	{
		//perror("fail to close.\n");
		//exit(1);
	}

	//return 0;
}


int main(int argc, char *argv[])
{
	int i;
	char data[MAX_CMD_LEN];
	char *temp = data;
	int maxlen;

	temp[0] = '\0';
	if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		printf("Usage: wagent PARAM\n"\
		       "  weber agent transpond the PARAM to ipcam main exec weber cmdline and got the result message.\n"\
		      );
		return 0;
	}

	maxlen = 1;// for the last '\0';
	for (i=1; i<argc; i++)
	{
		maxlen += strlen(argv[i]);
		maxlen += 3;// length of param is :['param' ]
	}

	if (maxlen > MAX_CMD_LEN)
		temp = malloc(maxlen);
	temp[0] = '\0';
	//printf("maxlen: %d\n", maxlen);
	for (i=1; i<argc; i++)
	{
		if (argv[i][0] == '#')
			break;
		if (argv[i][0] == '\'' || argv[i][0] == '"')
			strcat(temp, argv[i]);
		else
		{
			strcat(temp, "'");
			strcat(temp, argv[i]);
			strcat(temp, "' ");
		}
	}
	send_recv(temp);
	printf("\n%s", temp);

	if (maxlen > MAX_CMD_LEN)
		free(temp);
	return 0;
}
