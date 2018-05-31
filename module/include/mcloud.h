#ifndef __MCLOUD_H__
#define __MCLOUD_H__

#include "jv_common.h"
#include "malarmout.h"

typedef enum
{
	CLOUD_ON,
	CLOUD_RUNNING,
	CLOUD_OFF,
	CLOUD_BUTT
}CloudStatus_e;

typedef struct
{
	BOOL bEnable;			//是否开启云存储
	char acID[64];			//云服务用户名
	char acPwd[64];			//云服务密码
	char host[64];			//云存储服务器
	char bucket[32];		//云空间
	int  type;				//云类型
	char expireTime[20];	//到期时间
	long deadline;			//到期时间
}CLOUD;

typedef struct
{
	char bucketName[32];
	char endpoint[64];
	char deviceGuid[16];
	char keyId[64];
	char expiration[64];
	char keySecret[128];
	char token[1024];
	char days[16];
	char type[8];
}OBSS_INFO;

extern OBSS_INFO obss_info;

/**
 *@brief 初始化
 *
 *@return 0
 */
int mcloud_init(void);

/**
 *@brief 结束
 *
 *@return 0
 */
int mcloud_deinit(void);

/**
 *@brief 获取参数
 *@param cloud 存放属性的结构体
 *@return 0 成功，-1 失败
 *
 */
int mcloud_get_param(CLOUD *cloud);

/**
 *@brief 设置参数
 *@param cloud 要设置的属性结构体
 *@return 0 成功，-1 失败
 *
 */
int mcloud_set_param(CLOUD *cloud);

/**
 *@brief 检查云存储功能是否可用
 *@param 
 *@return TRUE 可用，FALSE 禁用
 *
 */
BOOL mcloud_check_enable();

int mcloud_parse_obssstate(char *data);

void mcloud_upload_alarm_file(JV_ALARM *jvAlarm);


#endif  //end of __MCLOUD_H__

