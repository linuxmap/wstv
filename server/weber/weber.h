#ifndef _WEBER_H_
#define _WEBER_H_

#define WEBER_CMD_PIPE		CONFIG_PATH"/pipe/weber.sck"

//#define MAX_CMD_LEN	1024*5
#define MAX_CMD_LEN	1024*10  //由于增加了webrecord的event_get功能，原有的消息长度不够用

/**
 *@brief 初始化web浏览器的辅助模块
 * 本模块用于与浏览器或者CGI程序通讯，完成浏览器功能
 *
 */
int weber_init(void);

/**
 *@brief 结束
 *
 */
int weber_deinit(void);

/**
 *@brief 设置返回值字符串
 *
 */
int weber_set_result(char *result);

#endif


