/******************************************************************************

  Copyright (c) 2015 Jovision Technology Co., Ltd. All rights reserved.

  File Name     : alarm_service.h
  Version       : 
  Author        : Qin Lichao <qinlichao@jovision.com>
  Created       : 2015-08-31
  Description   : Network alarm service.
  History       : 
  1.Date        : 2015-08-31
    Author      : Qin Lichao <qinlichao@jovision.com>
    Modification: Created file
******************************************************************************/

#ifndef ALARM_SERVICE_H_
#define ALARM_SERVICE_H_
#ifdef __cplusplus
extern "C" {
#endif
// #include "mml/core/types_c.h"



typedef struct{
	char type[32]; //video,io,analyse
	char subtype[32]; //videoLost,motionDetect,videoBlock,hardDriverLost,doorAlarm,smoke,pir,invasion
	int pir_code; //alarm details
	int detector_id; //xxx
	char dev_id[16]; //A402153844
	char dev_type[8]; //card,dvr,ipc,nvr
	int channel;
	char cloud_record_info[128]; //information of record uploaded to cloud
} AlarmReportInfo_t;

// TF卡状态
typedef enum
{
	CARD_REC_OK,
	CARD_REC_ERR,

	CARD_STATE_MAX
} CardState_e;


/*报警服务初始化，创建连接线程与报警服务器建立连接*/
int alarm_service_init();

/*报警服务结束*/
int alarm_service_deinit();

/*手动配置上线服务器和报警服务器地址*/
int alarm_service_manual_config(int bManualOnlineServer, int bManualBusinessServer, int bManualRecordServer,
			const char *onlineServer, const int onlinePort,
			const char *businessServer, const int businessPort,
			const char *recordServer, const int recordPort);

/*发送报警消息*/
int alarm_service_alarm_report(int channelid, int Type);

/*刷新布撤防状态*/
void alarm_service_refresh_deployment();

/*发送布撤防消息*/
int alarm_service_protection_notice(BOOL protectionStatus);

int alarm_service_tfcard_notice_ignore();

/*发送存储异常消息*/
int alarm_service_tfcard_notice(CardState_e eState, const char *desc);

/*发送人流量统计结果消息*/
int alarm_service_ivp_count_notice(const char *start_time, const char *end_time, int in_count, int out_count);

/*发送热度图消息*/
// int alarm_service_ivp_heatmap_notice(const char *start_time, const char *end_time, MMLImage* img);
#ifdef __cplusplus
}
#endif

#endif /* ALARM_SERVICE_H_ */
