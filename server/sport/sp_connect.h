/*
 * sp_connect.h
 *
 *  本文件用于连接管理。
 *  管理建立了多少连接，都是什么类型的，连接的具体信息等
 *  不同模块，在有连接建立时，调用#sp_connect_add 添加。无连接时，使用其中的KEY删除之
 *
 *  Created on: 2013-11-19
 *      Author: lfx
 */

#ifndef SP_CONNECT_H_
#define SP_CONNECT_H_

typedef enum{
	SP_CON_ALL,
	SP_CON_JOVISION,
	SP_CON_RTSP,
	SP_CON_GB28181,
	SP_CON_PSIA,
	SP_CON_OTHER,
}SPConType_e;

typedef struct{

	//conType 和 key，唯一确定一个连接
	SPConType_e conType;
	unsigned int key;

	char protocol[32];	//such as yst, onvif, 28181 and so on

	char addr[16];	//连接者的IP地址
	char user[32];	//连接者的登陆帐户

	unsigned char param[128];	//用户数据
}SPConnection_t;

typedef struct{
	BOOL RTMP_EN; 				//rtmp 功能开关
	char url[64];				//rtmp url
}SPConnectionRtmp_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief 增加一个连接
 */
int sp_connect_add(SPConnection_t *connection);

/**
 *@关键值为protocol和key，其它值没关系
 */
int sp_connect_del(SPConnection_t *connection);

/**
 *@关键值为conType和key，其它值没关系
 */
int sp_connect_breakoff(SPConnection_t *connection);

/**
 *@brief 获取连接数
 */
int sp_connect_get_cnt(SPConType_e conType);

/**
 *@brief 将读指针，指向list的开头
 */
int sp_connect_seek_set();

SPConnection_t *sp_connect_get_next(SPConType_e conType);

/**
 * @brief 设置rtmp
 * param chn 通道名称
 * param url rtmp结构体，包括是否启动以及服务器地址
 */
int sp_connect_set_rtmp(int chn,SPConnectionRtmp_t *attr);

/**
 * @brief 判断是否支持全网通协议
 * param void
 */
int sp_connect_extsdk_support(void);
#if 1
int sp_connect_init_semaphore();
int sp_connect_send_semaphore();
int sp_connect_recv_semaphore(int timeout);
int sp_connect_uninit_semaphore();

#endif

#ifdef __cplusplus
}
#endif


#endif /* SP_CONNECT_H_ */
