/*
 * utl_queue.h
 *
 *  Created on: 2014年3月14日
 *      Author: lfx  20451250@qq.com
 *      Company:  www.jovision.com
 */
#ifndef UTL_QUEUE_H_
#define UTL_QUEUE_H_



/**
 *@brief 创建一个消息队列
 *@param name 消息队列的名字
 *@param msgsize 每个消息的尺寸
 *@param queuesize 消息队列中能保存的消息的个数
 *@retval >=0 消息队列的handle
 *@retval -1 创建消息队列时，内存分配失败
 *
 */
int utl_queue_create(char *name, int msgsize, int queuesize);

/**
 *@brief 销毁一个消息队列
 *@param handle 消息队列的句柄，其值为#jv_queue_create 的返回值
 *@return 0
 *
 */
int utl_queue_destroy(int handle);

/**
 *@brief 发送消息
 *@param handle 消息队列的句柄，其值为#jv_queue_create 的返回值
 *@param msg 要发送的消息的指针
 *@note 消息的长度在 #jv_queue_create 时已指定
 *@return 0 成功，-1 队满，暂时不能发送
 *
 */
int utl_queue_send(int handle, void *msg);

/**
 *@brief 接收消息
 *@param handle 消息队列的句柄，其值为#jv_queue_create 的返回值
 *@param msg 输出，用于接收数据的指针
 *@param timeout 超时时间，单位为毫秒，-1时不超时
 *@note 消息的长度在 #jv_queue_create 时已指定
 *@return 0 成功，错误号表示超时
 *
 */
int utl_queue_recv(int handle, void *msg, int timeout);
//查询还有多少个队列没处理,调试使用
int utl_queue_get_count(int handle);

#endif /* UTL_QUEUE_H_ */
