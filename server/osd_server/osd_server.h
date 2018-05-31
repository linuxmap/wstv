#ifndef _OSD_SERVER_H_
#define _OSD_SERVER_H_

typedef struct _GQMsgHeader_t
{
	unsigned char head_flag;
	unsigned char version;
	unsigned char reserv01;
	unsigned char reserv02;
	unsigned int sessionId;
	unsigned int sequence_num;
	unsigned char channel;
	unsigned char end_flag;
	unsigned short msg_id;
	unsigned int data_len;
}GQMsgHeader_t;

/**
 *@brief 初始化osd_server模块
 * 本模块用于与济南智嵌的字符叠加器通讯，完成OSD字符叠加的功能
 *
 */
int osd_server_init(void);

/**
 *@brief 结束
 *
 */
int osd_server_deinit(void);

/**
 *@brief 处理符合一定JSON格式的字符叠加命令
 *@param buf JSON格式数据
 *@param detail	处理返回结果
 *@param bAccessCheck	是否检查用户权限
 */
int process_osd_msg(char *buf, char *detail, int bAccessCheck);

#endif


