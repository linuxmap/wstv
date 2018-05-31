#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/shm.h>

#define MAX_BUF_LEN	(1024)
#define SHM_DATA (1024*1024)
#define KEY_ID 1234
typedef struct 
{
	long int Size;
	unsigned char acData[SHM_DATA];
}shm_data_t;

static void _send_recv(char *cmd)
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
	n = recvfrom(s_fd, cmd, MAX_BUF_LEN, 0, (struct sockaddr *) &cin, &addr_len);
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

int main()
{
	char weekday[][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char month[][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	char content[512] = {0};
	void *shm = NULL;
	int shmid;

	shm_data_t* shmData = NULL;
	shmid = shmget((key_t)KEY_ID,sizeof(shm_data_t),0666|IPC_CREAT);
	shm = shmat(shmid,0,0);
	shmData = (shm_data_t*)shm;
	char cmd[MAX_BUF_LEN];
	snprintf(cmd, sizeof(cmd), "'websnapshot' '1' 'snapshot'");
	_send_recv(cmd);

	struct tm timeinfo;
	time_t t = time(0);
	localtime_r(&t, &timeinfo);

	//printf("Content-Type: text/html\n\n");
	sprintf(content,"HTTP/1.1 200 OK\r\n"
					"Server: thttpd/2.25b 29dec2003\r\n"
					"Date: %s, %d %s %d %d:%d:%d GMT\r\n"
					"Last-Modified: %s, %d %s %d %d:%d:%d GMT\r\n"
					"Content-Type: image/jpeg\r\n"
					"Accept-Ranges: bytes\r\n"
					"Connection: close\r\n"
					"Content-Length: %ld\r\n\r\n",
					weekday[timeinfo.tm_wday],
					timeinfo.tm_mday,
					month[timeinfo.tm_mon],
					timeinfo.tm_year+1900,
					timeinfo.tm_hour,
					timeinfo.tm_min,
					timeinfo.tm_sec,
					weekday[timeinfo.tm_wday],
					timeinfo.tm_mday,
					month[timeinfo.tm_mon],
					timeinfo.tm_year+1900,
					timeinfo.tm_hour,
					timeinfo.tm_min,
					timeinfo.tm_sec,
					shmData->Size
					 );
	printf("%s",content);


	fwrite(shmData->acData, shmData->Size, 1, stdout);
	
	shmdt(shm);
	shmctl(shmid,IPC_RMID,0);
	return 0;
}

