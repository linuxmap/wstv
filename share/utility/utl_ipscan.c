#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "utl_ipscan.h"

typedef struct icmp_hdr  
{  
    unsigned char   icmp_type;      // 消息类型  
    unsigned char   icmp_code;      // 代码  
    unsigned short  icmp_checksum;  // 校验和  
  
    // 下面是回显头  
    unsigned short  icmp_id;        // 用来惟一标识此请求的ID号，通常设置为进程ID  
    unsigned short  icmp_sequence;  // 序列号  
    unsigned long   icmp_timestamp; // 时间戳  
} ICMP_HDR, *PICMP_HDR;  



static unsigned short checksum(unsigned short* buff, int size)  
{  
    unsigned long cksum = 0;  
    while(size>1)  
    {  
        cksum += *buff++;  
        size -= sizeof(unsigned short);  
    }  
    // 是奇数  
    if(size)  
    {  
        cksum += *(unsigned char *)buff;
    }  
    // 将32位的chsum高16位和低16位相加，然后取反  
    cksum = (cksum >> 16) + (cksum & 0xffff);  
    cksum += (cksum >> 16);             
    return (unsigned short)(~cksum);  
}  



unsigned char sendbuf[1500];
int nsent = 0;
int	datalen = 0;		/* data that goes with ICMP echo request */

static unsigned int __get_useconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

static int send_icmp(int fd, unsigned int ipAddr, unsigned short pid)
{
	int			len;
	struct icmp_hdr	*icmp;

	icmp = (struct icmp_hdr *) sendbuf;
	icmp->icmp_type = 8;//ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_sequence = nsent++;
//	Gettimeofday((struct timeval *) icmp->icmp_timestamp, NULL);
	icmp->icmp_timestamp = __get_useconds();

	len = 8 + datalen;		/* checksum ICMP header and data */
	icmp->icmp_checksum = 0;
	icmp->icmp_checksum = checksum((unsigned short *) icmp, len);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ipAddr;
	addr.sin_port = htons(0);

	sendto(fd, (const char *)sendbuf, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	return 0;
}

static int recv_icmp(int fd, unsigned int *addrList, int cnt, unsigned int timeoutMS, int *bUsed, unsigned short pid)
{
	int sock = fd;
	int needCheckTimeout = 0;

	unsigned int now, last;
	last = __get_useconds();
	int i;
		//receiving
	while (1)
	{
		if (needCheckTimeout)
		{
			now = __get_useconds();
			if (now > last + timeoutMS)
				break;
		}
		needCheckTimeout = 0;


		fd_set rfds;
		struct timeval tv;
		int retval;

		/* Watch stdin (fd 0) to see when it has input. */
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);

		/* Wait up to five seconds. */
		tv.tv_sec = timeoutMS / 1000;
		tv.tv_usec = (timeoutMS % 1000)*1000;

		retval = select(sock+1, &rfds, NULL, NULL, &tv);
		/* Don't rely on the value of tv now! */

		if (retval == -1)
		{
			perror("select()");
			break;
		}
		else if (retval)
		{
			struct sockaddr_in from;
			int nLen = sizeof(from);
			int nRet;
			char recvBuf[1024];
			
//			printf("Data is available now.\n");

			nRet = recvfrom(sock, recvBuf, 1024, 0, (struct sockaddr*)&from, (socklen_t *)&nLen);
			if(nRet < 0)
			{  
				printf(" recvfrom() failed: %s\n", strerror(errno));
				return -1;  
			}  

			// 下面开始解析接收到的ICMP封包  
			unsigned int nTick = __get_useconds();
			if(nRet < 16)  
			{  
				printf(" Too few bytes from %s \n", inet_ntoa(from.sin_addr));
			}

			// 接收到的数据中包含IP头，IP头大小为20个字节，所以加20得到ICMP头  
			// (ICMP_HDR*)(recvBuf + sizeof(IPHeader));  
			ICMP_HDR* pRecvIcmp = (ICMP_HDR*)(recvBuf + 20);   
			if(pRecvIcmp->icmp_type != 0)    // 回显  
			{  
//				printf(" nonecho type %d recvd \n", pRecvIcmp->icmp_type);
				needCheckTimeout = 1;
				continue;  
			}  

			if(pRecvIcmp->icmp_id != pid)  
			{  
				printf(" someone else's packet! \n");
				needCheckTimeout = 1;
				continue;  
			}

			for (i=0;i<cnt;i++)
			{
				if (addrList[i] == from.sin_addr.s_addr)
				{
					bUsed[i] = 1;
					break;
				}
			}

//			printf("从 %s 返回 %d 字节:", inet_ntoa(from.sin_addr),nRet);
//			printf(" 数据包序列号 = %d. /t", pRecvIcmp->icmp_sequence);
//			printf(" 延时大小: %ld ms", nTick - pRecvIcmp->icmp_timestamp);
//			printf(" /n");
		}
		/* FD_ISSET(0, &rfds) will be true. */
		else
		{
//			printf("No data within five seconds.\n");
			break;
		}
	}
	return 0;
}

int utl_ipscan(unsigned int *addrList, int cnt, unsigned int timeoutMS, int *bUsed)
{
	int sock;
	unsigned short pid;

	memset(bUsed, 0, sizeof(int) * cnt);

	sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0)
	{
		printf("Failed create socket\n");
		return -1;
	}
	
	pid = 123;
	int i;
	for (i=0;i<cnt;i++)
	{
		send_icmp(sock, addrList[i], pid);
//		printf("after send : %d\n", i);
		if ((i%8) == 0)
		{
			recv_icmp(sock, addrList, cnt, 0, bUsed, pid);
		}
	}

	recv_icmp(sock, addrList, cnt, timeoutMS, bUsed, pid);

	close(sock);

	return 0;
}


int utl_ipscan_local(const char *ipstr, unsigned int timeoutMS, unsigned int *ipList, int maxCnt)
{
	int ret;
	unsigned int ip;
	unsigned char *p = (unsigned char *)&ip;

	ip = inet_addr(ipstr);

	unsigned int localList[255];
	int localUsed[255];
	int i;
	int cnt = 0;
	for (i=0;i<255;i++)
	{
		localList[i] = ip;
		p = (unsigned char *)&localList[i];
		p[3] = i;
		cnt++;
	}
	ret = utl_ipscan(localList, cnt, timeoutMS, localUsed);
	if (ret != 0)
	{
		return -1;
	}

	int usedCnt = 0;
	for (i=0;i<cnt;i++)
	{
		if (localUsed[i])
		{
			ipList[usedCnt++] = localList[i];
		}
		if (usedCnt >= maxCnt)
		{
			break;
		}
	}
	return usedCnt;
}

int utl_ipscan_not_used(const char *ipstr, unsigned int timeoutMS, unsigned int *ipList, int maxCnt)
{
	int ret;
	unsigned int ip;
	unsigned char *p = (unsigned char *)&ip;

	ip = inet_addr(ipstr);

	unsigned int localList[255];
	int localUsed[255];
	int i;
	int cnt = 0;
	for (i=0;i<253;i++)
	{
		localList[i] = ip;
		p = (unsigned char *)&localList[i];
		p[3] = i+1;
		cnt++;
	}
	ret = utl_ipscan(localList, cnt, timeoutMS, localUsed);
	if (ret != 0)
	{
		return -1;
	}

	int freedCnt = 0;
	for (i=0;i<cnt;i++)
	{
		if (!localUsed[i])
		{
			ipList[freedCnt++] = localList[i];
		}
		if (freedCnt >= maxCnt)
		{
			break;
		}
	}
	return freedCnt;
}
/**
 *@param ipstr 本地IP地址。作为参考使用，形如192.168.11.35
 *
 *@return 空闲的IP地址。网络字节序的UINT值 
 *
 */
unsigned int utl_get_free_ip(const char *ipstr, unsigned int timeoutMS)
{
	int ret;
	unsigned int ip;
	unsigned char *p = (unsigned char *)&ip;

	ip = inet_addr(ipstr);

#define UTL_IPSCAN_STARTIP	100

	unsigned int ipList[255]={0};
	int bUsed[255]={0};
	int i=0;
	int cnt = 0;
	for (i=UTL_IPSCAN_STARTIP;i<255;i++)
	{
		ipList[i] = ip;
		p = (unsigned char *)&ipList[i];
		p[3] = i;
		cnt++;
	}
	ret = utl_ipscan(ipList, cnt, timeoutMS, bUsed);
	if (ret != 0)
	{
		return -1;
	}

	for (i=UTL_IPSCAN_STARTIP;i<cnt;i++)
	{
		if (!bUsed[i])
		{
			return ipList[i];
		}
	}
	return 0;
}
