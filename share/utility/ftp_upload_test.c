#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ftp_client.h"

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		printf("Usage: %s FTPServer port filename\n", argv[0]);
		exit(0);
	}
	int ret = 0;
	char *host = argv[1];
	int port = atoi(argv[2]);
	char *filename = argv[3];
	int sockfd = ftp_connect(host, port, "anonymous", "");
	ret = ftp_storfile(sockfd, filename, "test.file", NULL, NULL);
	printf("ftp_storfile return %d\n", ret);
	ret = ftp_quit(sockfd);
	printf("ftp_quit return %d\n", ret);
return 0;
}