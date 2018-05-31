#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__


/*
	$Id: httpdownload.h, v1.0.0, 2011.6.28, YellowBug $
	$@@: http下载 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#define PATH_SPLIT			'/'

typedef int	HSOCKET;
typedef void* PARAM_MGR;

#define closesocket(s)		close(s)

#define BUFSIZE 4*1024


#define DEF_HTTP_PORT			80
#define HTTP_MAX_PATH			260
#define HTTP_MAX_REQUESTHEAD	1024
#define HTTP_MAX_RESPONSEHEAD	2048
#define HTTP_DOWN_PERSIZE		128	
#define HTTP_FLUSH_BLOCK		1024

/*
	$@@ 下载状态
	$@ HDL_URLERR: 提供的url非法
	$@ HDL_SERVERFAL: 根据host地址无法找到主机 
	$@ HDL_TIMEOUT: 连接下载地址超时
	$@ HDL_READYOK: 连接下载地址成功,准备下载
	$@ HDL_DOWNING: 正在下载
	$@ HDL_DISCONN: 断开连接了
	$@ HDL_FINISH: 下载完成
*/
enum 
{
	HDL_URLERR = 0xe0,
	HDL_SERVERFAL = 0xe1,
	HDL_SOCKFAL = 0xe2,
	HDL_SNDREQERR = 0xe3,
	HDL_SERVERERR = 0xe4,
	HDL_CRLOCFILEFAL = 0xe5,
	HDL_TIMEOUT = 0x100,
	HDL_NORESPONSE = 0x101,
	HDL_READYOK = 0x104,
	HDL_DOWNING = 0x105,
	HDL_DISCONN = 0x106,
	HDL_FINISH = 0x107
};

/*
	$@@ 下载回调函数,主要用于监听下载过程
	$@ 第一个参数: 下载标识符号(上面枚举中的值)
	$@ 第二个参数: 需要下载的总字节数
	$@ 第三个参数: 已经下载的总字节数
	$@ 第四个参数: 距离上一次下载的时间戳(毫秒)
	$@ 返回值: 下次下载的字节数(0: 表示默认值, 小于0则下载中断, 大于0则位下次下载指定的字节)
*/
typedef int (*HTTPDOWNLOAD_CALLBACK)(int, unsigned int, unsigned int, unsigned int);

/*
	$@@ 通过指定的url下载资源
	$@ url: 下载资源地址
	$@ filepath: 存储在本地的路径(如果为NULL则存储在当前目录)
	$@ filename: 存储的文件名(如果为NULL则位url中分析的文件名)
	$@ http_func: 下载过程监听函数(如果为NULL则不监听下载过程)
*/
int pq_http_download(const char *url, const char *local_path, const char *local_file, 
			char *mem_addr, unsigned int *already_rec, HTTPDOWNLOAD_CALLBACK http_func);

int http_download(const char* url,const char* localpath,const char *local_file, 
		char *mem_addr, unsigned int *already_rec);
	

typedef struct __HTTP_T
{
	char camera_code[64]; 	
	char secret_key[64];
	char publish_web[64];				
}mhttp_attr_t;

int Http_get_message(char   *argv,mhttp_attr_t *attr) ;

int Http_get_message_ex(char *url, char *res, int len);

// URL编码，将' '、%、\等字符转义
char* Http_url_encode(char* dst, const char* url);

// URL解码，将%开头的转义字符转换为原始字符，允许dst与url相同
char* Http_url_decode(char* dst, const char* url);

/**
 *@brief 向指定服务地址发送post请求
 *@param url 服务地址
 *@param req 请求数据
 *@param len 请求数据长度
 *@param resp 返回的响应数据，大小应为BUFSIZE大小
 *@param timeout 等待响应超时时间
 *@return 返回的数据的总长度。
 *@note 数据总长度为：HTTP头的长度 + Content-Length所指定的数据长度
 */
int Http_post_message(const char *url, const char *req, int len, char *resp, int timeout);


// HTTP_GET参数管理
PARAM_MGR Http_create_param_mgr();

void Http_destroy_param_mgr(PARAM_MGR mgr);

int Http_add_param_string(PARAM_MGR mgr, const char* key, const char* value);

int Http_add_param_int(PARAM_MGR mgr, const char * key, int value);

int Http_param_sort(PARAM_MGR mgr);

int Http_remove_param(PARAM_MGR mgr, const char* key);

const char* Http_get_param_val(PARAM_MGR mgr, const char* key);

void Http_clear_param(PARAM_MGR mgr);

int Http_param_generate_value(const PARAM_MGR mgr, char* dst, int len);

int Http_param_generate_string(const PARAM_MGR mgr, char* dst, int len);

#endif

