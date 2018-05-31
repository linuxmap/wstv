/*
 * gb28181.h
 *
 *  Created on: 2013-10-30
 *      Author: lfx
 */

#ifndef GB28181_H_
#define GB28181_H_

#ifndef MAX_CHN_CNT
	#define MAX_CHN_CNT 32
#endif

#ifndef MAX_ALARMIN_CNT
	#define MAX_ALARMIN_CNT 16
#endif

#define MAX_ID_LEN	21

typedef struct{
	int bEnable;	//是否使能
	char serverip[16];
	char localip[16];
	unsigned short serverport;
	unsigned short localport;

	char devid[MAX_ID_LEN]; //dev id. 34020000001320000001
	char devpasswd[32]; //12345678
	int expires;	//注册消息过期时间，单位为秒
	int keepalive;	//心跳周期
	int expires_refresh;
	int keepalive_outtimes;

	int chnCnt;	//通道数量
	char chnID[MAX_CHN_CNT][MAX_ID_LEN];
	int alarminCnt;
	char alarminID[MAX_ALARMIN_CNT][MAX_ID_LEN];
}GBRegInfo_t;

int gb_get_default_param(GBRegInfo_t *info);

int gb_init(GBRegInfo_t *info);

int gb_deinit();

/**
 * 相关参数发生变化。内部可能需要重启
 */
int gb_reset_param(GBRegInfo_t *info);

int gb_send_data_i(int channel, unsigned char *buffer, int len, int framerate, int width, int height, int bitrate, unsigned long long timestamp);

int gb_send_data_p(int channel, unsigned char *buffer, int len, unsigned long long timestamp);

int gb_send_data_a(int channel, unsigned char *buffer, int len, int btalk, unsigned long long timestamp);

/**
 *@brief 发送报警信息
 *
 *@param channel 通道号。当method为2时，表示alarmin号
 *@param priority 1为一级警情，2为二级警情，3为三级警情，4为四级警情
 *@param method 1为电话报警，2为设备报警，3为短信报警，4为GPS报警，5为视频报警，6为设备故障报警，7其它报警；
 */
int gb_send_alarm(int channel, int priority, int method);

#endif /* GB28181_H_ */
