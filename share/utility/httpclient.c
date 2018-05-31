/*******   http客户端程序   httpclient.c   ************/
#include "jv_common.h"
#include <string.h>
#include "utl_map.h"
#include "httpclient.h"
//////////////////////////////httpclient.c   开始/////////////////////////////////////////// 

// #define _DEBUG_TEST

#ifdef _DEBUG_TEST
#define debugFlag 1
#endif

static HTTPDOWNLOAD_CALLBACK S_http_callback = NULL;
//#exp

#define HERROR(exp, err)	do { 														 \
								if (exp) {												\
									printf("(%s,%d) %s\n", __FILE__, __LINE__,#exp);	 \
									if(S_http_callback) S_http_callback((err), 0, 0, 0); \
									return -1;                                           \
								}                                                        \
							} while(0);				

#define HCALLBACK(a, b, c, d)		((NULL != S_http_callback) ? S_http_callback((a), (b), (c), (d)) : 0)

#define HERROR_CLOSE(exp, err)	do { 														 \
								if (exp) {												\
									closesocket(sock);									\
									printf("(%s,%d) %s\n", __FILE__, __LINE__,#exp);	 \
									if(S_http_callback) S_http_callback((err), 0, 0, 0); \
									return -1;                                           \
								}                                                        \
							} while(0);
/*
	$@@ 文件域的局部变量
*/
static const char * S_request_head = "GET %s HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nUser-Agent: Molliza/4.0(compatible;MSIE6.0;windows NT 5.0)\r\nHost: %s\r\nConnection: close\r\n\r\n";
							  		
//获取系统运行的时间，单位毫秒。不会受到修改时间的影响
static unsigned int get_tick() 
{
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return (unsigned int)(tv.tv_sec*1000 + tv.tv_nsec/1000000);

}

/*
	$@@ 从指定长度(len0)的字符串的右端开始找到第一个位ch的字符
*/
static char * r_strchr(const char *p, int len, char ch)
{
	int last_pos = -1;
	int i = 0;
	while ( *(p+i) != '\0' ) {
		if ( *(p+i) == ch ) last_pos = i;
		if ( ++i >= len ) break;	
	}
	return last_pos >= 0 ? (char *)(p+last_pos) : NULL;
}

/*
 	$@@ 解析url，分析出web地址,端口号,下载的资源名称
*/
static int parse_url(const char *url, char *web, int *port, char *filepath, char *filename)
{
	int web_len = 0;
	int web_start = 0;
	int file_len = 0;
	int file_start = 0;
	int path_len = 0;
	int path_start = 0;
	char *pa = NULL;
	char *pb = NULL;
	if ( !strncmp(url, "http://", 7) ) web_start = 7;
	else if ( !strncmp(url, "https://", 8) ) web_start = 8;
	
	pa = strchr(url+web_start, '/');
	pb = strchr(url+web_start, ':');
	
	/* 获取web */	
	if ( pa && (NULL == pb)) {
		web_len = (int)(pa - (url+web_start));
	} else {
		if ( pb ) web_len = pb - (url+web_start);
		else web_len = strlen(url+web_start);		
	}
	HERROR(0 == web_len, HDL_URLERR);
	memcpy(web, url+web_start, web_len);
	web[web_len] = '\0';	
	
	Printf("parse web : %s \n",web);
	
	/* 获取filename */
	if ( pb ) {
		pa = r_strchr(pb,strlen(pb),'/');
		HERROR(NULL == pa, HDL_URLERR);
		//pa = r_strchr(url+web_start, (int)(pb-(url+web_start)), '/');
		file_len = strlen(pa+1);	//(int)(pb - pa - 1);
		file_start = (int)(pa + 1 - url);
		*port = atoi(pb+1);
	} else {
		pa = r_strchr(url+web_start, strlen(url+web_start), '/');
		HERROR(NULL == pa, HDL_URLERR);
		file_len = strlen(pa+1);
		file_start = (int)(pa + 1 - url);
		*port = DEF_HTTP_PORT;
	}
	Printf("port : %d \n",*port);
	HERROR(NULL == pa, HDL_URLERR);
	memcpy(filename, url+file_start, file_len);
	filename[file_len] = '\0';
	Printf("filename : %s \n",filename);
	/* 获取filepath */
	if(pb)
	{
		char* port_offset = NULL;
		port_offset = strchr(pb,'/');
		path_start = web_start + web_len + (port_offset - pb);
	}
	else
		path_start = web_start + web_len;
	path_len = file_start - path_start + file_len;
	if(pb)
		memcpy(filepath, url+path_start , file_start - path_start + file_len);
	else
		memcpy(filepath, url+web_start+web_len, file_start - path_start + file_len);
	filepath[path_len] = '\0';
	Printf("filepath : %s \n",filepath);
	return 0;
}

/*
	$@@ 封装request报头
*/
static int package_request(char *request, const char *web, const char *filepath)
{
	int n = sprintf(request, S_request_head, filepath, web);
	request[n] = '\0';
	return n;
}
/*
	$@@ 发送请求报头
*/
static int send_request(HSOCKET sock, const char *buf, int n)
{
	int already_bytes = 0;
	int per_bytes = 0;	
	
	while ( already_bytes < n ) {
		per_bytes = send(sock, buf+already_bytes, n-already_bytes, 0);
		if ( per_bytes < 0 ) {
			break;
		} else if ( 0 == per_bytes ) {
			HCALLBACK(HDL_DISCONN, 0, 0, 0);
			break;
		} else
			already_bytes += per_bytes;	
	}
	return already_bytes;
}

/*
	$@@ 获取响应报头(状态行和消息报头)
*/
static int get_response_head(HSOCKET sock, char *response, int max)
{
	int total_bytes = 0;
	int per_bytes = 0;
	int over_flag = 0;
	char ch;

	do {
		per_bytes = recv(sock, &ch, 1, 0);
		if ( per_bytes < 0 ) 
		{
			break;
		}
		else if ( 0 == per_bytes ) {
			HCALLBACK(HDL_DISCONN, 0, 0, 0);
			break;
		} else {
			if ( '\r' == ch || '\n' == ch ) over_flag++;
			else over_flag = 0;
			
			response[total_bytes] = ch;			
			if ( total_bytes >= max ) break;
			else total_bytes += per_bytes;
		}
	} while ( over_flag < 4 );
	response[total_bytes] = '\0';
	return total_bytes;
}
static void parse_response_head(char *response, int n, int *server_status, unsigned int *down_total)
{
	int i = 1;
	char *pstatus = strstr(response, "HTTP/1.1");
	char *plen = strstr(response, "Content-Length:");	
	printf("%s\n", response);
	
	if ( pstatus ) {
		pstatus += strlen("HTTP/1.1");
		i = 1;	
		while ( *(pstatus+i) == ' ' ) ++i;
		*server_status = atoi(pstatus+i);
	} else
		*server_status = 0;
	if ( plen ) {
		plen += strlen("Content-Length:");
		i = 1;
		while ( *(plen+i) == ' ' ) ++i;
		*down_total = (unsigned int)atoi(plen+i); 
	} else
		*down_total = 0;
	printf("status: %d\n", *server_status);
	printf("total: %u\n", *down_total);
}
/*
	$@@ 创建本地文件
*/
static FILE * create_local_file(const char *local_path, const char *local_file, const char *remote_file)
{
	int len = strlen(local_path);
	char filename[HTTP_MAX_PATH+1];
	if ( local_path[len-1] != PATH_SPLIT ) {
		if ( local_file ) len = sprintf(filename, "%s%c%s", local_path, PATH_SPLIT, local_file);	
		else len = sprintf(filename, "%s%c%s", local_path, PATH_SPLIT, remote_file);		
 	} else {
		if ( local_file ) len = sprintf(filename, "%s%s", local_path, local_file);	
		else len = sprintf(filename, "%s%s", local_path, remote_file);	
	}
	filename[len] = '\0'; 
	printf("ready to create local file: %s\n", filename);
	return fopen(filename, "wb+");
}
/*
	$@@ 开始下载
*/
static int start_download(HSOCKET sock, unsigned int total_bytes, FILE *stream, 
	unsigned int *already_rec)
{
	unsigned int already_bytes = 0;
	int per_bytes = 0;
	int flush_bytes = 0;
	unsigned int total_tim = 0;
	unsigned int per_tim = 0;
	char buf[HTTP_DOWN_PERSIZE] = {0};
	
	do {
		per_tim = get_tick();
		per_bytes = recv(sock, buf, HTTP_DOWN_PERSIZE, 0);
		per_tim = get_tick() - per_tim;
		Printf("recv data length : %d \n",per_bytes);
		if ( per_bytes < 0 ) break;
		else if ( 0 == per_bytes ) {
			HCALLBACK(HDL_DISCONN, total_bytes, already_bytes, 0);
			break;		
		} else {
			fwrite(buf, 1, per_bytes, stream);
			already_bytes += (unsigned int)per_bytes;
			Printf("already_bytes : %d \n",already_bytes);
			if (already_rec != NULL)
				*already_rec = already_bytes;
			flush_bytes += per_bytes;
			if ( flush_bytes >= HTTP_FLUSH_BLOCK ) {
				fflush(stream);
				flush_bytes = 0;	
			}
			HCALLBACK(HDL_DOWNING, total_bytes, already_bytes, per_tim);
		}
	} while ( already_bytes < total_bytes );
	
	if ( flush_bytes > 0 )
		fflush(stream);

	HCALLBACK(HDL_FINISH, total_bytes, already_bytes, total_tim);
	if (already_bytes == total_bytes)
	{
		return total_bytes;
	}
	else
	{
		return -1;
	}
}

/*
	$@@ 开始下载
*/
static int start_download_toMem(HSOCKET sock, unsigned int total_bytes, char* mem_addr, unsigned int *already_rec)
{
	unsigned int already_bytes = 0;
	int per_bytes = 0;
	unsigned int total_tim = 0;
	unsigned int per_tim = 0;
	char buf[HTTP_DOWN_PERSIZE] = {0};
	
	do {
		per_tim = get_tick();
		per_bytes = recv(sock, buf, HTTP_DOWN_PERSIZE, 0);
		per_tim = get_tick() - per_tim;
		//Printf("recv data length : %d \n",per_bytes);
		if ( per_bytes < 0 ) 
		{
			break;
		}
		else if ( 0 == per_bytes ) {
			HCALLBACK(HDL_DISCONN, total_bytes, already_bytes, 0);
			break;		
		} else {
			memcpy(mem_addr + already_bytes, buf, per_bytes);
			already_bytes += (unsigned int)per_bytes;
			if (already_rec != NULL)
				*already_rec = already_bytes;
			//printf("already_bytes : %d \n",already_bytes);
			HCALLBACK(HDL_DOWNING, total_bytes, already_bytes, per_tim);
		}
	} while ( already_bytes < total_bytes );

	if (already_bytes == total_bytes)
	{
		return total_bytes;
	}
	else
	{
		return -1;
	}
	
	HCALLBACK(HDL_FINISH, total_bytes, already_bytes, total_tim);
}


int pq_http_download(const char *url, const char *local_path, const char *local_file, 
		char *mem_addr, unsigned int *already_rec, HTTPDOWNLOAD_CALLBACK http_func)
{
	int ret = -1;
	int port = 0;
	char web[HTTP_MAX_PATH+1];
	char remote_path[HTTP_MAX_PATH+1];
	char remote_file[HTTP_MAX_PATH+1];
	char request[HTTP_MAX_REQUESTHEAD+1];
	char response_head[HTTP_MAX_RESPONSEHEAD+1];
	int request_len = 0;
	int response_len = 0;
	unsigned int down_total = 0;
	int server_status = 404;
	HSOCKET sock = -1;
	struct hostent *host = NULL, hostinfo;
	struct sockaddr_in server_addr;	
	FILE *stream = NULL;
	S_http_callback = http_func;
	parse_url(url, web, &port, remote_path, remote_file);
	
	Printf("web : %s ,port : %d , remote_path : %s,remote_file : %s \n",web,port,remote_path,remote_file);
	
	request_len = package_request(request, web, remote_path);
	gethostbyname_r(web, &hostinfo, response_head, sizeof(response_head), &host, &ret);
	HERROR(NULL==host, HDL_SERVERFAL);
	Printf("gethostIp : %s \n",inet_ntoa(*((struct in_addr *)host->h_addr)));
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	HERROR(-1==sock, HDL_SOCKFAL);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	struct timeval timeout = {30,0}; 
	setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));

	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	HERROR_CLOSE(-1==connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)), HDL_TIMEOUT);

	HERROR_CLOSE(request_len != send_request(sock, request, request_len), HDL_SNDREQERR);
	response_len = get_response_head(sock, response_head, HTTP_MAX_RESPONSEHEAD);
	HERROR_CLOSE(response_len <= 0, HDL_NORESPONSE);
	parse_response_head(response_head, response_len, &server_status, &down_total);

	switch ( server_status ) {
		case 200:
			if ( down_total > 0 ) {
				if (local_path != NULL && local_file != NULL)
				{
					stream = create_local_file(local_path, local_file, remote_file);
					HERROR_CLOSE(NULL == stream, HDL_CRLOCFILEFAL);
					HCALLBACK(HDL_READYOK, down_total, 0, 0);
					ret = start_download(sock, down_total, stream, already_rec);
					fclose(stream);	
				}
				else if (mem_addr != NULL)
				{
					ret = start_download_toMem(sock, down_total, mem_addr, already_rec);
				}
			} 
			else 
			{
				HCALLBACK(HDL_FINISH, 0, 0, 0);
			}
		default:
			HCALLBACK(HDL_SERVERERR, 0, 0, 0);
	}
	closesocket(sock);
	return ret;
}

static int http_callback(int status, unsigned int total_bytes, unsigned int already_bytes, unsigned int tim)
{
	switch ( status ) {
		case HDL_READYOK:
			printf("ready to download, total.bytes=%u\n", total_bytes);
			break;
		case HDL_DOWNING:
			//printf("downloading... (%u/%u) bytes\n", already_bytes, total_bytes);
			break;
		case HDL_FINISH:
			printf("download finish! download total.bytes=%u\n", already_bytes);
			break;
		default:
			printf("status: %#x\n", status);	
	}	
	return 0;
}

//下载到文件,成功返回数据size，否则-1
int http_download(const char* url,const char* localpath,const char *local_file, 
		char *mem_addr, unsigned int *already_rec)
{
	return pq_http_download(url,localpath,local_file,mem_addr,already_rec,http_callback);
}

/* END */


/*
功能：搜索字符串右边起的第一个匹配字符 
*/
char* Rstrchr(char* s, char x)    
{ 
    int i = strlen(s); 
    if(!(*s)) 
    {
        return 0; 
    }
    while(s[i-1])  
    {
        if(strchr(s+(i-1), x))     
        {
            return (s+(i-1));    
        }
        else  
        {
            i--;
        }
    }
    return 0; 
} 
 unsigned int M_Param(char *pParam, int nMaxLen, char *pBuffer)
{
	int nLen	= 0;
	while(pBuffer && *pBuffer && *pBuffer != ',')
	{
		*pParam++ = *pBuffer++;
		nLen++;
	}
	return nLen;
}
 void GetMessage(char* src, mhttp_attr_t *attr )  
 {
 	mhttp_attr_t http_attr;
	char acBuff[256]= {0};
	unsigned int nLen = 1;	
	char *pItem;
	char *pValue;
	char *mpValue;
 	memset(&http_attr, 0, sizeof(mhttp_attr_t)); 
	src=src+1;
	if(!(*src))  
    {
        return; 
    }
	while ((nLen = M_Param(acBuff, sizeof(acBuff), src)) > 0)
	{
        
		acBuff[nLen]	= 0;
		acBuff[nLen-1]	= '\r';
		src += nLen+1;
		pItem = strtok(acBuff, ":");
		pValue = strtok(NULL, "\r");
		//printf("%s=%s\n", pItem, pValue);
		
		if (strncmp(pItem, "\"camera_code\"", 12) == 0)
		{
			strncpy(http_attr.camera_code, pValue+1, sizeof(http_attr.camera_code));
		}
		else if(strncmp(pItem, "\"secret_key\"", 10) == 0)
		{
			strncpy(http_attr.secret_key, pValue+1, sizeof(http_attr.secret_key));
		}
		else if(strncmp(pItem, "\"publish_url\"", 10) == 0)
		{
			int Len = 0;
			mpValue= pValue+10;
	        while(mpValue && *mpValue && *mpValue != '\\')
				{
					mpValue++;
					Len++;
				}
			strncpy(http_attr.publish_web, pValue+10,Len);
		}
	}
		//printf("http_attr.camera_code=%s\n",http_attr.camera_code);
		//printf("http_attr.secret_key=%s\n",http_attr.secret_key);
		//printf("http_attr.publish_web=%s\n",http_attr.publish_web);
		memcpy(attr, &http_attr,sizeof(mhttp_attr_t)); 
	
 }
/*
功能：从字符串src中分析出网站地址和端口，并得到用户要下载的文件 
*/
void GetHost(char* src, char* web, char* file, int* port)    
{ 
    char* pA; 
    char* pB; 
   // memset(web, 0, sizeof(web)); 
   // memset(file, 0, sizeof(file)); 
    *port = 0; 
    if(!(*src))  
    {
        return; 
    }
    pA = src; 
    if(!strncmp(pA, "http://", strlen("http://")))   
    {
        pA = src+strlen("http://"); 
    }
    else if(!strncmp(pA, "https://", strlen( "https://")))     
    {
        pA = src+strlen( "https://"); 
    }
    pB = strchr(pA, '/'); 
    if(pB)     
    { 
        memcpy(web, pA, strlen(pA)-strlen(pB)); 
        if(pB+1)   
        { 
            memcpy(file, pB+1, strlen(pB)-1); 
            file[strlen(pB)-1] = 0; 
        } 
    } 
    else    
    {
        memcpy(web, pA, strlen(pA)); 
    }
    if(pB)
    {
        web[strlen(pA) - strlen(pB)] = 0; 
    }
    else    
    {
        web[strlen(pA)] = 0; 
    }
    pA = strchr(web, ':'); 
    if(pA)    
    {
        *port = atoi(pA + 1); 
    }
    else  
    {
        *port = 80; 
    }
} 
 
/*
*filename:   httpclient.c 
*purpose:   HTTP协议客户端程序
*/
int   Http_get_message(char   *argv,mhttp_attr_t *attr) 
{ 
    int sockfd = 0; 
    char buffer[1024] = ""; 
    struct sockaddr_in   server_addr; 
    struct hostent   *host = NULL, hostinfo; 
    int portnumber = 0;
    int nbytes = 0; 
    char host_addr[256] = ""; 
    char host_file[1024] = ""; 
    FILE *fp; 
    char request[1024] = ""; 
    int send = 0;
    int totalsend = 0; 
    int i = 0; 
	int j = 0;
	int ret = 0;
    char *pt; 
	mhttp_attr_t m_http_attr;
    printf( "parameter.1 is: %s\n ", argv); 
    //ToLowerCase(argv[1]);/*将参数转换为全小写*/ 
    //printf( "lowercase   parameter.1   is:   %s\n ",   argv[1]); 
 
    GetHost(argv, host_addr, host_file, &portnumber);/*分析网址、端口、文件名等*/
    //printf( "webhost:%s\n ", host_addr); 
   // printf( "hostfile:%s\n ", host_file); 
    //printf( "portnumber:%d\n\n ", portnumber); 

	gethostbyname_r(host_addr, &hostinfo, buffer, sizeof(buffer), &host, &ret);
    if(host == NULL)/*取得主机IP地址*/
    { 
        printf("Gethostname   error, \n "); 
        return -1;
    } 
 
    /*   客户程序开始建立   sockfd描述符   */
    if((sockfd=socket(AF_INET,SOCK_STREAM,0)) == -1)/*建立SOCKET连接*/
    { 
        printf("Socket   Error\a\n "); 
        return -1;
    } 
 
    /*   客户程序填充服务端的资料   */
    bzero(&server_addr,sizeof(server_addr)); 
    server_addr.sin_family=AF_INET; 
    server_addr.sin_port=htons(portnumber); 
    server_addr.sin_addr=*((struct in_addr*)host->h_addr); 
 
    /*   客户程序发起连接请求   */
    if(connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1)/*连接网站*/
    { 
        printf("Connect   Error\a\n "); 
        close(sockfd);
        return -1;
    } 
 
    sprintf(request,   "GET   /%s   HTTP/1.1\r\nAccept:   */*\r\nAccept-Language:   zh-cn\r\n\
User-Agent:   Mozilla/4.0   (compatible;   MSIE   5.01;   Windows   NT   5.0)\r\n\
Host:   %s:%d\r\nConnection:   Close\r\n\r\n ", host_file, host_addr, portnumber); 
     
    //printf( "%s\n", request);/*准备request，将要发送给主机*/
 
    /*取得真实的文件名*/
    if(host_file!=NULL&& *host_file)     
    {
        pt = Rstrchr(host_file, '/'); 
    }
    else  
    {
        pt = 0; 
    }
 
    /*发送http请求request*/
    send = 0;
    totalsend = 0; 
    nbytes=strlen(request); 
    while(totalsend < nbytes)  
    { 
        send = write(sockfd, request+totalsend, nbytes-totalsend); 
        if(send == -1)     
        {
            printf( "send error!%s\n ", strerror(errno));
            close(sockfd);
            return -1;
        } 
        totalsend += send; 
        printf("%d bytes send OK!\n ", totalsend); 
    } 
 
    //printf( "The   following   is   the   response   header:\n ");  
    /*   连接成功了，接收http响应，response   */
	nbytes=read(sockfd,buffer,sizeof(buffer));
	if(nbytes!=418)
	{
		close(sockfd);
		return -1;
	}
  	printf("response = %d\n", nbytes);
    /*   结束通讯   */
    close(sockfd); 
	j=0;
	for(i=0;i<nbytes;i++)
	{
		 if(j < 4)    
		{ 
            if(buffer[i] == '\r' || buffer[i] == '\n')     
			{
				j++; 
			}
            else   
			{
				j = 0; 
			}
		 }
		 else
		 	break;
	}
	//printf("response = %s\n", &buffer[i]);
	GetMessage(&buffer[i], &m_http_attr)  ;
	memcpy(attr, &m_http_attr,sizeof(mhttp_attr_t)); 
   	return 0;
} 

int GetHostEx(char* url, char* domain, char* ipaddr, int iplen, int* port, char** service)    
{
	// http://127.0.0.1:80/test
	char *ptrDomain = strchr(url, ':');
	if(ptrDomain == NULL)
	{
		printf("Invalid url:%s\n", url);
		return -1;
	}
	char *ptrPort = strchr(ptrDomain+1, ':');
	if(ptrPort == NULL)
	{
		// http://127.0.0.1/test
		*port = 80;
		ptrPort = strchr(ptrDomain+3, '/');
	}
	else
	{
		*port = atoi(ptrPort+1);
	}

	strncpy(domain, ptrDomain+3, ptrPort-ptrDomain-3);

	struct hostent *host = NULL, hostinfo;
	char buffer[1024] = "";
	int ret = 0;
	gethostbyname_r(domain, &hostinfo, buffer, sizeof(buffer), &host, &ret);
	if(host == NULL)
	{
		// herror("Can't get IP address");
		printf("gethostbyname_r failed  domain:%s\n", domain);
		return -1;
	}
	//for(pptr=host->h_addr_list; *pptr!=NULL; pptr++)
	//	printf("address:%s\n", inet_ntop(host->h_addrtype, *pptr, str, sizeof(str)));
	if(inet_ntop(host->h_addrtype, host->h_addr, ipaddr, iplen) != NULL)
	{
		printf("ipaddr:%s\n", ipaddr);
	}

	*service = strstr(ptrPort, "/");

	printf("domain: %s, ipaddr: %s, port: %d, service: %s\n", domain, ipaddr, *port, *service);

	return 0;
} 

/*
*filename:   httpclient.c 
*purpose:   HTTP协议客户端程序
*/
int Http_get_message_ex(char *url, char *res, int len)
{
    int sockfd = 0;
    char buffer[1024] = "";
    struct sockaddr_in   server_addr;
    struct hostent   *host;
    int port = 0;
    int nbytes = 0;
    char domain[256] = "";
	char ipaddr[16] = "";
    char* service = NULL;
    char request[1024] = "";
    int send = 0;
    int totalsend = 0;
	struct in_addr	sin_addr;

    printf( "url is: %s\n", url);

    if (GetHostEx(url, domain, ipaddr, sizeof(ipaddr), &port, &service) != 0)
    {
		return -1;
    }

    /*   客户程序开始建立   sockfd描述符   */
    if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1)/*建立SOCKET连接*/
    { 
        printf("Socket error: %s\n", strerror(errno)); 
        return -1;
    } 
 
    /*   客户程序填充服务端的资料   */
    bzero(&server_addr,sizeof(server_addr)); 
    server_addr.sin_family=AF_INET; 
    server_addr.sin_port=htons(port); 
	if(inet_pton(AF_INET, ipaddr, &server_addr.sin_addr) <= 0)
	{
		printf("inet_pton error:%s\n", strerror(errno));
        close(sockfd);
        return -1;
	}
 
    /*   客户程序发起连接请求   */
    if(connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1)/*连接网站*/
    { 
        printf("Connect error: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    } 
 
    snprintf(request, sizeof(request), 
				"GET %s HTTP/1.1\r\n"
				"Host:   %s:%d\r\n"
				"\r\n", 
				service, domain, port); 
     
    // printf("%s\n", request);/*准备request，将要发送给主机*/
 
    /*发送http请求request*/
    send = 0;
    totalsend = 0; 
    nbytes=strlen(request); 
    while(totalsend < nbytes)  
    { 
        send = write(sockfd, request+totalsend, nbytes-totalsend); 
        if(send == -1)     
        {
            printf( "send error!%s\n ", strerror(errno));
            close(sockfd);
            return -1;
        } 
        totalsend += send; 
        printf("%d bytes send OK!\n ", totalsend); 
    } 
 
	if (res == NULL)
	{
	    close(sockfd);
		return 0;
	}

    //printf( "The   following   is   the   response   header:\n ");  
    /*   连接成功了，接收http响应，response   */
	nbytes=read(sockfd, res, len);			// 接收部分，暂时不关注，先按接收一次做
  	printf("response = %d\n%s\n", nbytes, buffer);

    /*   结束通讯   */
    close(sockfd);
   	return nbytes;
}


/**
 *@brief 查找字符串的值
 *
 *@param body 消息体。类似这样：PLAY RTSP/1.0\r\n CSeq: 3\r\n Scale: 0.5\r\n Range: npt=0-
 *@param key 要查找的键值
 *@param seg 分割符，可以是':', '=' 等
 *@param data 返回结果
 *@param maxLen 提供的value的最大保存长度
 */
static char *__get_line_value(const char *body, const char *key, char seg, char *data, int maxLen)
{
	char *p;
	char *dst;
	int len;

	p = (char *)strstr(body, key);
	if (data)
		data[0] = '\0';
	if (!p)
		return NULL;

	p += strlen(key);
	while (*p && *p != seg)
		p++;
	p++;
	dst = p;
	len = 0;
	while (*p && *p != '\r' && *p != '\n')
	{
		p++;
		len++;
	}

	if (*dst == '"')
	{
		dst++;
		len-=2; // " 一定是成对出现的
	}

	if (data)
		p = data;
	else
		p = (char *)malloc(len+1);
	memcpy(p, dst, len);
	p[len] = '\0';

	return p;
}

/**
 *@brief 检测Content-Length 字段，获取数据总长度
 *@param predata 已收到的数据
 *@return 将要收到的数据的总长度。
 *@note 数据总长度为：HTTP头的长度 + Content-Length所指定的数据长度
 */
static int __get_content_length(const char *predata)
{
	char *tstr;
	//int ret;
	char temp[32];
	int contentLen;
	if (!predata)
	{
		return -1;
	}
	tstr = __get_line_value(predata, "Content-Length", ':', temp, 32);
	if (tstr)
	{
		contentLen = atoi(tstr);
		const char *p = strstr(predata, "\r\n\r\n");
		if (p)
		{
			return contentLen + p - predata + 4;
		}
		return contentLen + strlen(predata);
	}
	else
	{
		return 0;
	}

}

static char _http_get_hexchar(char src)
{
	src = src & 0x0F;

	if (src > 9)
	{
		return src - 10 + 'A';
	}
	else
	{
		return src + '0';
	}
}

static char _http_get_hexval(char src)
{
	if (src >= 'A' && src <= 'F')
	{
		return src - 'A' + 10;
	}
	else if (src >= 'a' && src <= 'f')
	{
		return src - 'a' + 10;
	}
	else if (src >= '0' && src <= '9')
	{
		return src - '0';
	}
	else
	{
		return 0;
	}
}

// URL编码，将' '、%、\等字符转义，允许dst与url相同
char* Http_url_encode(char* dst, const char* url)
{
	const char* pend = url;
	char* p = dst;
	char escape[] = " +%=/?#&";
	char* pesc = NULL;
	char samestr = 0;		// 自转换

	if (dst == url)
	{
		samestr = 1;
		while (*pend++);
			--pend;
	}

	while (*url)
	{
		pesc = escape;
		while (*pesc)
		{
			if (*url == *pesc++)
			{
				--pesc;
				++url;
				if (samestr)
				{
					memmove(p + 3, url, pend - url + 1);
					url += 2;
					pend += 2;
				}
				*p++ = '%';
				*p++ = _http_get_hexchar(*pesc >> 4);
				*p++ = _http_get_hexchar(*pesc & 0xF);
				break;
			}
		}

		if (!*pesc)
			*p++ = *url++;
	}

	*p = '\0';

	return dst;
}

// URL解码，将%开头的转义字符转换为原始字符，允许dst与url相同
char* Http_url_decode(char* dst, const char* url)
{
	const char* pend = url;
	char* p = dst;
	char samestr = 0;		// 自转换

	if (dst == url)
	{
		samestr = 1;
		while (*pend++);
			--pend;
	}

	while (*url)
	{
		if (*url == '%')
		{
			*p++ = (_http_get_hexval(*(url + 1)) << 4) | (_http_get_hexval(*(url + 2)));
			url += 3;
			if (samestr)
			{
				memmove(p, url, pend - url + 1);
				url -= 2;
				pend -= 2;
			}
			continue;
		}

		*p++ = *url++;
	}

	*p++ = '\0';

	return dst;
}

int Http_post_message(const char *url, const char *req, int len, char *resp, int timeout)
{
	int sockfd = 0;
	int ret = 0;
	int port = 0;
	int contentLen = 0;
	int chunkLen = 0;
	int offset = 0;
	int bChunked = 0;
	struct sockaddr_in servaddr;
	char sendbuf[BUFSIZE] = {0};
	char recvbuf[BUFSIZE] = {0};
	char domain[128] = {0};
	char ipaddr[16] = {0};
	
	memset(sendbuf, 0, sizeof(sendbuf));
	memset(ipaddr, 0, sizeof(ipaddr));
	
	if(url == NULL)
	{
		printf("NULL url\n");
		return -1;
	}
	//printf("url=%s\n", url);

	char *ptrDomain = strchr(url, ':');
	if(ptrDomain == NULL)
	{
		printf("Invalid url:%s\n", url);
		return -1;
	}
	char *ptrPort = strchr(ptrDomain+1, ':');
	if(ptrPort == NULL)
	{
		printf("Invalid url:%s\n", url);
		return -1;
	}

	strncpy(domain, ptrDomain+3, ptrPort-ptrDomain-3);
	
	struct hostent *host = NULL, hostinfo;
	gethostbyname_r(domain, &hostinfo, sendbuf, sizeof(sendbuf), &host, &ret);
	if(host == NULL)
	{
		printf("gethostbyname_r failed\n");
		return -1;		
	}
	//for(pptr=host->h_addr_list; *pptr!=NULL; pptr++)
	//	printf("address:%s\n", inet_ntop(host->h_addrtype, *pptr, str, sizeof(str)));
	if(inet_ntop(host->h_addrtype, host->h_addr, ipaddr, sizeof(ipaddr)) != NULL)
	{
		//printf("ipaddr:%s\n", ipaddr);
	}

	port = atoi(ptrPort+1);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}

	struct timeval t;
	t.tv_sec = timeout;
	t.tv_usec = 0;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t));
	if(ret != 0)
	{
		perror("setsockopt");
		close(sockfd);
		return -1;
	}
	t.tv_sec = timeout;
	t.tv_usec = 0;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
	if(ret != 0)
	{
		perror("setsockopt");
		close(sockfd);
		return -1;
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if(inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0)
	{
		printf("inet_pton error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("connect error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	char *service = strstr(ptrPort, "/");
	sprintf(sendbuf, "POST %s HTTP/1.1\r\n"
					"Host: %s:%d\r\n"
					"Content-Type: application/json;charset=utf-8\r\n"
					"Content-Length: %d\r\n"
					"\r\n",
					service, domain, port, len);
		
	strcat(sendbuf, req);
	//printf("http send:\n%s\n",sendbuf);

	ret = send(sockfd, sendbuf, strlen(sendbuf), 0);
	if (ret < 0)
	{
		printf("send error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	while(1)
	{
		// 超出Buffer(需要包括1字节'\0')
		if (offset + 1 >= sizeof(recvbuf))
		{
			printf("Buffer overflow! offset: %d, Recv:\n%s\n", offset, recvbuf);
			close(sockfd);
			return -1;
		}
		ret = recv(sockfd, recvbuf+offset, sizeof(recvbuf)-offset-1, 0);
		if (ret < 0 && ((errno == EAGAIN) || (errno == EINTR)))
		{
			usleep(1000);
			continue;
		}
		if (ret == -1)
		{
			printf("error: recv failed : %s\n", strerror(errno));
			close(sockfd);
			return -1;
		}
		else if (ret == 0)
		{
			printf("socket is closed\n");
			close(sockfd);
			return -1;
		}
		offset += ret;
		recvbuf[offset] = '\0';
		//printf("recv:\n%s\n", recvbuf);
		if(strstr(recvbuf, "Content-Length:") != NULL)
		{
			if (contentLen == 0)
			{
				contentLen = __get_content_length(recvbuf);
			}
			if (contentLen == 0 || contentLen <= offset)
			{
				if(contentLen > 0)
				{
					char *httpBody = strstr(recvbuf, "\r\n\r\n");
					if(httpBody == NULL)
					{
						printf("http resp incorrect\n");
						close(sockfd);
						return 0;
					}
					strcpy(resp, httpBody+4);
				}
				break;
			}
		}
		else if(strstr(recvbuf, "Transfer-Encoding: chunked") != NULL)
		{
			bChunked = 1;
			char *httpBody = strstr(recvbuf, "\r\n\r\n");
			//printf("httpBody:\n%s\n", httpBody);
			offset = 0;
			if(httpBody != NULL)
			{
				char size[8] = {0};
				char *bodySize = httpBody+4;
				char *bodyStart = strstr(bodySize, "\r\n");
				if(bodyStart != NULL)
				{
					//printf("bodySize:\n%s\n", bodySize);
					strncpy(size, bodySize, (int)(bodyStart-bodySize));
					sscanf(size,"%x",&chunkLen);
					//printf("chunkLen:%d\n", chunkLen);
					
					bodyStart+=2;
					//printf("bodyStart:\n%s\n", bodyStart);
					if (contentLen + chunkLen + 1 >= BUFSIZE)
					{
						printf("recvbuf:\n%s\n", recvbuf);
						printf("Chunked over flow0! contentLen:%d, chunkLen: %d\n", contentLen, chunkLen);
						close(sockfd);
						return -1;
					}
					strncpy(resp+contentLen, bodyStart, chunkLen);

					contentLen += chunkLen;
					//printf("contentLen:%d\n", contentLen);
					if(strstr(bodyStart, "0\r\n\r\n") != NULL)//recvd all the chunks
					{
						//printf("1resp:\n%s\n", resp);
						break;
					}
				}
				else
				{
					printf("1HTTP resp incorrect!\n");
					close(sockfd);
					return -1;
				}
			}
			else
			{
				printf("2HTTP resp incorrect!\n");
				close(sockfd);
				return -1;
			}
		}
		else
		{
			if(bChunked == 1)
			{
				char *end = strstr(recvbuf, "0\r\n\r\n");
				if(end != NULL)//recvd all the chunks
				{
					if (contentLen + (end - recvbuf) + 1 >= BUFSIZE)
					{
						printf("recvbuf:\n%s\n", recvbuf);
						printf("Chunked over flow1! contentLen:%d, chunkLen: %d\n", contentLen, end-recvbuf);
						close(sockfd);
						return -1;
					}
					strncpy(resp+contentLen, recvbuf, end-recvbuf);
					//printf("2resp:\n%s\n", resp);
					break;
				}
				else
				{
					if (contentLen + chunkLen + 1 >= BUFSIZE)
					{
						printf("recvbuf:\n%s\n", recvbuf);
						printf("Chunked over flow2! contentLen:%d, chunkLen: %d\n", contentLen, ret);
						close(sockfd);
						return -1;
					}
					strncpy(resp+contentLen, recvbuf, ret);
				}
			}
			else
			{
				// 有些http服务器没有长度标识，也解析
				char *httpBody = strstr(recvbuf, "\r\n\r\n");
				close(sockfd);
				if(httpBody == NULL)
				{
					return sprintf(resp, "%s", recvbuf);
				}
				return sprintf(resp, "%s", httpBody+4);
			}
		}
	}
	close(sockfd);
	return contentLen;
}

#ifndef _DEBUG_TEST
PARAM_MGR Http_create_param_mgr()
{
	return utl_map_create();
}

void Http_destroy_param_mgr(PARAM_MGR mgr)
{
	return utl_map_destory(mgr);
}

int Http_add_param_string(PARAM_MGR mgr, const char* key, const char* value)
{
	char tmp_val[1024] = {0};
	Http_url_encode(tmp_val, value);		// 对value值进行url编码
	return utl_map_add_pair(mgr, key, tmp_val);
}

int Http_add_param_int(PARAM_MGR mgr, const char * key, int value)
{
	char buf[16] = {0};

	snprintf(buf, sizeof(buf), "%d", value);

	return utl_map_add_pair(mgr, key, buf);
}

int Http_remove_param(PARAM_MGR mgr, const char* key)
{
	return utl_map_remove_pair(mgr, key);
}

const char* Http_get_param_val(PARAM_MGR mgr, const char* key)
{
	return utl_map_get_val(mgr, key);
}

void Http_clear_param(PARAM_MGR mgr)
{
	return utl_map_clear(mgr);
}

int Http_param_sort(PARAM_MGR mgr)
{
	return utl_map_sort(mgr);
}

int Http_param_generate_value(const PARAM_MGR mgr, char* dst, int len)
{
	return utl_map_generate_value(mgr, dst, len, "");
}

int Http_param_generate_string(const PARAM_MGR mgr, char* dst, int len)
{
	return utl_map_generate_string(mgr, dst, len, "=", "&");
}
#endif

#ifdef _DEBUG_TEST
int main()
{
	char url[1024] = "http://172.16.34.222:8080/publicService/monitorManage/monitorIpcPutInfo.do?cloudstorage=1&connectStyle=1&deviceCategory=11250732&deviceChannel=1&deviceGuid=H15326892&deviceType=PBS3801&deviceUsername=jkDYfRGGnuSVTzchMoLdng%3D%3D&deviceVersion=V2.2.5483_test5&netIntVersion=2007918&netStrVersion=v2.0.80.9[private:v2.0.79.18%2020160921]&sdcardExist=0&sig=8DDF39C51A97968A4D6A61B8E1B74E66";
	char dst[1024] = {0};

	printf("before: %s\n", url);
	Http_url_encode(dst, url);
	Http_url_encode(url, url);
	printf("after1: %s\n", url);
	printf("after1: %s\n", dst);

	Http_url_decode(dst, dst);
	printf("after2: %s\n", dst);
	Http_url_decode(dst, url);
	printf("after2: %s\n", dst);

	return 0;
}
#endif
//////////////////////////////httpclient.c   结束///////////////////////////////////////////
