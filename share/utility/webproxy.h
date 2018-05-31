#ifndef _WEBPROXY_H_
#define _WEBPROXY_H_

/**
 *@brief 回调函数，在WEB服务有回复时调用
 *@param buffer 收到的数据。当其值为NULL时，意味着需要关闭SOCKET了
 *@param len 收到的数据的长度
 *
 */
typedef void (*received_from_webserver_callback)(unsigned char *buffer, int len, void *callback_param);


/**
 *@brief 初始化
 *@param callback 收到数据时的回调函数
 *@param webaddr WEB服务的IP
 *@param webport WEB服务的端口
 *
 *@return 0 成功
 *
 */
int proxy_init(received_from_webserver_callback callback, char *webaddr, unsigned short webport);

/**
 *@brief 结束
 *
 *@return 0 成功
 *
 */
int proxy_deinit(void);

/**
 *@brief 将收到的数据，通过127.0.0.1 转发给WEB服务
 *@param data 要转发的数据
 *@param len 数据长度
 *
 *@return 0 成功-1 创建socket失败
 */
int proxy_send2server(unsigned char *data, int len, void *callback_param);

/**
 *@brief 关闭与WEB服务的连接
 *
 *
 */
void proxy_close_socket(void);


#endif

