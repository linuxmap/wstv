// This file is generated auto. Do not modify it anytime.
#ifndef _ipc_H_
#define _ipc_H_

#include "grpc.h"

#ifndef GRPC_ipc_API
	#if (defined WIN32 || defined WIN64)
		#define GRPC_ipc_API __declspec(dllexport)
	#else
		#define GRPC_ipc_API
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern grpcMethod_t ipc_methodList_s[];

extern grpcMethod_t ipc_methodList_c[];

//--- account_get_users definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_account_get_users;

typedef struct{
	int users_cnt;
	struct{
		char *name; //username
		char *level; //admin,operator,user,anonymous,extended
		char *description; //This is Adiministrator
	} *users;
} PARAM_RESP_ipc_account_get_users;
int USERDEF_ipc_account_get_users(grpc_t *grpc, PARAM_REQ_ipc_account_get_users *req, PARAM_RESP_ipc_account_get_users *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_account_get_users(grpc_t *grpc, PARAM_REQ_ipc_account_get_users *req);
GRPC_ipc_API int CLIENT_RESP_ipc_account_get_users(grpc_t *grpc, PARAM_RESP_ipc_account_get_users *resp);
GRPC_ipc_API int CLIENT_ipc_account_get_users(grpc_t *grpc, PARAM_REQ_ipc_account_get_users *req, PARAM_RESP_ipc_account_get_users *resp);

//--- account_add_user definition ----

typedef struct{
	char *name; //username
	char *passwd; //123456
	char *level; //admin,operator,user,anonymous,extended
	char *description; //This is Adiministrator
} PARAM_REQ_ipc_account_add_user;

typedef struct{
	int idle;
} PARAM_RESP_ipc_account_add_user;
int USERDEF_ipc_account_add_user(grpc_t *grpc, PARAM_REQ_ipc_account_add_user *req, PARAM_RESP_ipc_account_add_user *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_account_add_user(grpc_t *grpc, PARAM_REQ_ipc_account_add_user *req);
GRPC_ipc_API int CLIENT_RESP_ipc_account_add_user(grpc_t *grpc, PARAM_RESP_ipc_account_add_user *resp);
GRPC_ipc_API int CLIENT_ipc_account_add_user(grpc_t *grpc, PARAM_REQ_ipc_account_add_user *req, PARAM_RESP_ipc_account_add_user *resp);

//--- account_del_user definition ----

typedef struct{
	char *name; //username
} PARAM_REQ_ipc_account_del_user;

typedef struct{
	int idle;
} PARAM_RESP_ipc_account_del_user;
int USERDEF_ipc_account_del_user(grpc_t *grpc, PARAM_REQ_ipc_account_del_user *req, PARAM_RESP_ipc_account_del_user *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_account_del_user(grpc_t *grpc, PARAM_REQ_ipc_account_del_user *req);
GRPC_ipc_API int CLIENT_RESP_ipc_account_del_user(grpc_t *grpc, PARAM_RESP_ipc_account_del_user *resp);
GRPC_ipc_API int CLIENT_ipc_account_del_user(grpc_t *grpc, PARAM_REQ_ipc_account_del_user *req, PARAM_RESP_ipc_account_del_user *resp);

//--- account_modify_user definition ----

typedef struct{
	char *name; //username
	char *passwd; //123456
	char *level; //admin,operator,user,anonymous,extended
	char *description; //This is Adiministrator
} PARAM_REQ_ipc_account_modify_user;

typedef struct{
	int idle;
} PARAM_RESP_ipc_account_modify_user;
int USERDEF_ipc_account_modify_user(grpc_t *grpc, PARAM_REQ_ipc_account_modify_user *req, PARAM_RESP_ipc_account_modify_user *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_account_modify_user(grpc_t *grpc, PARAM_REQ_ipc_account_modify_user *req);
GRPC_ipc_API int CLIENT_RESP_ipc_account_modify_user(grpc_t *grpc, PARAM_RESP_ipc_account_modify_user *resp);
GRPC_ipc_API int CLIENT_ipc_account_modify_user(grpc_t *grpc, PARAM_REQ_ipc_account_modify_user *req, PARAM_RESP_ipc_account_modify_user *resp);

//--- account_login definition ----
/***********************************************
grpc supply login ,but some device may need login cmd.
************************************************/

typedef struct{
	char *username; //abc
	char *password; //123
} PARAM_REQ_ipc_account_login;

typedef struct{
	int idle;
} PARAM_RESP_ipc_account_login;
int USERDEF_ipc_account_login(grpc_t *grpc, PARAM_REQ_ipc_account_login *req, PARAM_RESP_ipc_account_login *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_account_login(grpc_t *grpc, PARAM_REQ_ipc_account_login *req);
GRPC_ipc_API int CLIENT_RESP_ipc_account_login(grpc_t *grpc, PARAM_RESP_ipc_account_login *resp);
GRPC_ipc_API int CLIENT_ipc_account_login(grpc_t *grpc, PARAM_REQ_ipc_account_login *req, PARAM_RESP_ipc_account_login *resp);

//--- account_login_force definition ----
/***********************************************
if client force login, server must disconnect other clients.
************************************************/

typedef struct{
	char *username; //abc
	char *password; //123
} PARAM_REQ_ipc_account_login_force;

typedef struct{
	int idle;
} PARAM_RESP_ipc_account_login_force;
int USERDEF_ipc_account_login_force(grpc_t *grpc, PARAM_REQ_ipc_account_login_force *req, PARAM_RESP_ipc_account_login_force *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_account_login_force(grpc_t *grpc, PARAM_REQ_ipc_account_login_force *req);
GRPC_ipc_API int CLIENT_RESP_ipc_account_login_force(grpc_t *grpc, PARAM_RESP_ipc_account_login_force *resp);
GRPC_ipc_API int CLIENT_ipc_account_login_force(grpc_t *grpc, PARAM_REQ_ipc_account_login_force *req, PARAM_RESP_ipc_account_login_force *resp);

//--- alarmin_start definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarmin_start;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarmin_start;
int USERDEF_ipc_alarmin_start(grpc_t *grpc, PARAM_REQ_ipc_alarmin_start *req, PARAM_RESP_ipc_alarmin_start *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_start(grpc_t *grpc, PARAM_REQ_ipc_alarmin_start *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_start(grpc_t *grpc, PARAM_RESP_ipc_alarmin_start *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_start(grpc_t *grpc, PARAM_REQ_ipc_alarmin_start *req, PARAM_RESP_ipc_alarmin_start *resp);

//--- alarmin_stop definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarmin_stop;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarmin_stop;
int USERDEF_ipc_alarmin_stop(grpc_t *grpc, PARAM_REQ_ipc_alarmin_stop *req, PARAM_RESP_ipc_alarmin_stop *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_stop(grpc_t *grpc, PARAM_REQ_ipc_alarmin_stop *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_stop(grpc_t *grpc, PARAM_RESP_ipc_alarmin_stop *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_stop(grpc_t *grpc, PARAM_REQ_ipc_alarmin_stop *req, PARAM_RESP_ipc_alarmin_stop *resp);

//--- alarmin_get_param definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarmin_get_param;

typedef struct{
	GBOOL bEnable;
	GBOOL bNormallyClosed;
	GBOOL bEnableRecord;
	GBOOL bStarting;
	GBOOL bBuzzing;
	GBOOL bSendtoClient;
	GBOOL bSendEmail;
	int u8AlarmNum;
	int nSOS;
	int nDelay;
	int nGuardChn;
	int type_cnt;
	struct{
		int type;
		GBOOL bAlarmOut;
	} *type;
} PARAM_RESP_ipc_alarmin_get_param;
int USERDEF_ipc_alarmin_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_get_param *req, PARAM_RESP_ipc_alarmin_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_get_param(grpc_t *grpc, PARAM_RESP_ipc_alarmin_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_get_param *req, PARAM_RESP_ipc_alarmin_get_param *resp);

//--- alarmin_set_param definition ----

typedef struct{
	int channelid;
	GBOOL bEnable;
	GBOOL bNormallyClosed;
	GBOOL bEnableRecord;
	GBOOL bStarting;
	GBOOL bBuzzing;
	GBOOL bSendtoClient;
	GBOOL bSendEmail;
	int u8AlarmNum;
	int nSOS;
	int nDelay;
	int nGuardChn;
	int type_cnt;
	struct{
		int type;
		GBOOL bAlarmOut;
	} *type;
} PARAM_REQ_ipc_alarmin_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarmin_set_param;
int USERDEF_ipc_alarmin_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_set_param *req, PARAM_RESP_ipc_alarmin_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_set_param(grpc_t *grpc, PARAM_RESP_ipc_alarmin_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarmin_set_param *req, PARAM_RESP_ipc_alarmin_set_param *resp);

//--- alarmin_b_onduty definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarmin_b_onduty;

typedef struct{
	GBOOL bOnduty;
} PARAM_RESP_ipc_alarmin_b_onduty;
int USERDEF_ipc_alarmin_b_onduty(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_onduty *req, PARAM_RESP_ipc_alarmin_b_onduty *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_b_onduty(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_onduty *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_b_onduty(grpc_t *grpc, PARAM_RESP_ipc_alarmin_b_onduty *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_b_onduty(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_onduty *req, PARAM_RESP_ipc_alarmin_b_onduty *resp);

//--- alarmin_b_alarming definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarmin_b_alarming;

typedef struct{
	GBOOL bAlarming;
} PARAM_RESP_ipc_alarmin_b_alarming;
int USERDEF_ipc_alarmin_b_alarming(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_alarming *req, PARAM_RESP_ipc_alarmin_b_alarming *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_b_alarming(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_alarming *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_b_alarming(grpc_t *grpc, PARAM_RESP_ipc_alarmin_b_alarming *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_b_alarming(grpc_t *grpc, PARAM_REQ_ipc_alarmin_b_alarming *req, PARAM_RESP_ipc_alarmin_b_alarming *resp);

//--- alarm_get_param definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_alarm_get_param;

typedef struct{
	int delay;
	int port;
	char *sender; //lfx@jovision.com
	char *server; //lfx@jovision.com
	char *username; //lfx
	char *passwd; //123456
	int receiver_cnt;
	char **receiver; //qlc@jovision.com
} PARAM_RESP_ipc_alarm_get_param;
int USERDEF_ipc_alarm_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_param *req, PARAM_RESP_ipc_alarm_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_get_param(grpc_t *grpc, PARAM_RESP_ipc_alarm_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_get_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_param *req, PARAM_RESP_ipc_alarm_get_param *resp);

//--- alarm_set_param definition ----

typedef struct{
	int delay;
	int port;
	char *sender; //lfx@jovision.com
	char *server; //lfx@jovision.com
	char *username; //lfx
	char *passwd; //123456
	int receiver_cnt;
	char **receiver; //qlc@jovision.com
} PARAM_REQ_ipc_alarm_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_set_param;
int USERDEF_ipc_alarm_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_set_param *req, PARAM_RESP_ipc_alarm_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_set_param(grpc_t *grpc, PARAM_RESP_ipc_alarm_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_set_param(grpc_t *grpc, PARAM_REQ_ipc_alarm_set_param *req, PARAM_RESP_ipc_alarm_set_param *resp);

//--- alarm_link_preset_get definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarm_link_preset_get;

typedef struct{
	int presetno;
} PARAM_RESP_ipc_alarm_link_preset_get;
int USERDEF_ipc_alarm_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_get *req, PARAM_RESP_ipc_alarm_link_preset_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_link_preset_get(grpc_t *grpc, PARAM_RESP_ipc_alarm_link_preset_get *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_get *req, PARAM_RESP_ipc_alarm_link_preset_get *resp);

//--- alarm_link_preset_set definition ----

typedef struct{
	int channelid;
	int presetno;
} PARAM_REQ_ipc_alarm_link_preset_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_link_preset_set;
int USERDEF_ipc_alarm_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_set *req, PARAM_RESP_ipc_alarm_link_preset_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_link_preset_set(grpc_t *grpc, PARAM_RESP_ipc_alarm_link_preset_set *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_preset_set *req, PARAM_RESP_ipc_alarm_link_preset_set *resp);

//--- alarmin_link_preset_get definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarmin_link_preset_get;

typedef struct{
	int link_cnt;
	struct{
		GBOOL bWireless;
		int alarmin;
		int presetno;
	} *link;
} PARAM_RESP_ipc_alarmin_link_preset_get;
int USERDEF_ipc_alarmin_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_get *req, PARAM_RESP_ipc_alarmin_link_preset_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_link_preset_get(grpc_t *grpc, PARAM_RESP_ipc_alarmin_link_preset_get *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_link_preset_get(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_get *req, PARAM_RESP_ipc_alarmin_link_preset_get *resp);

//--- alarmin_link_preset_set definition ----

typedef struct{
	int channelid;
	int link_cnt;
	struct{
		GBOOL bWireless;
		int alarmin;
		int presetno;
	} *link;
} PARAM_REQ_ipc_alarmin_link_preset_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarmin_link_preset_set;
int USERDEF_ipc_alarmin_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_set *req, PARAM_RESP_ipc_alarmin_link_preset_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarmin_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarmin_link_preset_set(grpc_t *grpc, PARAM_RESP_ipc_alarmin_link_preset_set *resp);
GRPC_ipc_API int CLIENT_ipc_alarmin_link_preset_set(grpc_t *grpc, PARAM_REQ_ipc_alarmin_link_preset_set *req, PARAM_RESP_ipc_alarmin_link_preset_set *resp);

//--- alarm_link_out_get definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_alarm_link_out_get;

typedef struct{
	int link_cnt;
	struct{
		int alarm_type;
		int alarm_out_cnt;
		int *alarm_out;
	} *link;
} PARAM_RESP_ipc_alarm_link_out_get;
int USERDEF_ipc_alarm_link_out_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_get *req, PARAM_RESP_ipc_alarm_link_out_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_link_out_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_link_out_get(grpc_t *grpc, PARAM_RESP_ipc_alarm_link_out_get *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_link_out_get(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_get *req, PARAM_RESP_ipc_alarm_link_out_get *resp);

//--- alarm_link_out_set definition ----

typedef struct{
	int channelid;
	int link_cnt;
	struct{
		int alarm_type;
		int alarm_out_cnt;
		int *alarm_out;
	} *link;
} PARAM_REQ_ipc_alarm_link_out_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_link_out_set;
int USERDEF_ipc_alarm_link_out_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_set *req, PARAM_RESP_ipc_alarm_link_out_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_link_out_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_link_out_set(grpc_t *grpc, PARAM_RESP_ipc_alarm_link_out_set *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_link_out_set(grpc_t *grpc, PARAM_REQ_ipc_alarm_link_out_set *req, PARAM_RESP_ipc_alarm_link_out_set *resp);

//--- alarm_report definition ----

typedef struct{
	char *dev_id; //A402153844
	char *dev_type; //card,dvr,ipc,nvr
	char *type; //video,io,analyse
	char *subtype; //videoLost,motionDetect,videoBlock,hardDriverLost,doorAlarm,smoke,pir,invasion
	char *pir_code; //alarm details
	char *detector_id; //xxx
	int channel;
} PARAM_REQ_ipc_alarm_report;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_report;
int USERDEF_ipc_alarm_report(grpc_t *grpc, PARAM_REQ_ipc_alarm_report *req, PARAM_RESP_ipc_alarm_report *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_report(grpc_t *grpc, PARAM_REQ_ipc_alarm_report *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_report(grpc_t *grpc, PARAM_RESP_ipc_alarm_report *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_report(grpc_t *grpc, PARAM_REQ_ipc_alarm_report *req, PARAM_RESP_ipc_alarm_report *resp);

//--- login definition ----

typedef struct{
	char *dev_id; //A402153844
	int data_cnt;
	int *data;
} PARAM_REQ_ipc_login;

typedef struct{
	char *tm; //20150901120000
} PARAM_RESP_ipc_login;
int USERDEF_ipc_login(grpc_t *grpc, PARAM_REQ_ipc_login *req, PARAM_RESP_ipc_login *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_login(grpc_t *grpc, PARAM_REQ_ipc_login *req);
GRPC_ipc_API int CLIENT_RESP_ipc_login(grpc_t *grpc, PARAM_RESP_ipc_login *resp);
GRPC_ipc_API int CLIENT_ipc_login(grpc_t *grpc, PARAM_REQ_ipc_login *req, PARAM_RESP_ipc_login *resp);

//--- keep_online definition ----

typedef struct{
	char *dev_id; //A402153844
} PARAM_REQ_ipc_keep_online;

typedef struct{
	int idle;
} PARAM_RESP_ipc_keep_online;
int USERDEF_ipc_keep_online(grpc_t *grpc, PARAM_REQ_ipc_keep_online *req, PARAM_RESP_ipc_keep_online *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_keep_online(grpc_t *grpc, PARAM_REQ_ipc_keep_online *req);
GRPC_ipc_API int CLIENT_RESP_ipc_keep_online(grpc_t *grpc, PARAM_RESP_ipc_keep_online *resp);
GRPC_ipc_API int CLIENT_ipc_keep_online(grpc_t *grpc, PARAM_REQ_ipc_keep_online *req, PARAM_RESP_ipc_keep_online *resp);

//--- get_streamserver_addr definition ----

typedef struct{
	int channelid;
	int streamid;
} PARAM_REQ_ipc_get_streamserver_addr;

typedef struct{
	int channelid;
	int streamid;
	GBOOL enable;
	char *url; //rtmp://42.157.5.251:1935/live/B153080736_0_1?token=YJYCnRgmnidCLlMHhXATtoT5dkn5pIqDHILVtZ1xu3RcZ3
	char *videoType; //live,record
	char *file; //Aystnum_20151111120000_0_1.mp4
	char *userId; //
} PARAM_RESP_ipc_get_streamserver_addr;
int USERDEF_ipc_get_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_get_streamserver_addr *req, PARAM_RESP_ipc_get_streamserver_addr *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_get_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_get_streamserver_addr *req);
GRPC_ipc_API int CLIENT_RESP_ipc_get_streamserver_addr(grpc_t *grpc, PARAM_RESP_ipc_get_streamserver_addr *resp);
GRPC_ipc_API int CLIENT_ipc_get_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_get_streamserver_addr *req, PARAM_RESP_ipc_get_streamserver_addr *resp);

//--- set_streamserver_addr definition ----

typedef struct{
	int channelid;
	int streamid;
	GBOOL enable;
	char *url; //rtmp://42.157.5.251:1935/live/B153080736_0_1?token=YJYCnRgmnidCLlMHhXATtoT5dkn5pIqDHILVtZ1xu3RcZ3
	char *videoType; //live,record
	char *file; //Aystnum_20151111120000_0_1.mp4
	char *userId; //nouse
	char *streamToken; //
} PARAM_REQ_ipc_set_streamserver_addr;

typedef struct{
	int idle;
} PARAM_RESP_ipc_set_streamserver_addr;
int USERDEF_ipc_set_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_set_streamserver_addr *req, PARAM_RESP_ipc_set_streamserver_addr *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_set_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_set_streamserver_addr *req);
GRPC_ipc_API int CLIENT_RESP_ipc_set_streamserver_addr(grpc_t *grpc, PARAM_RESP_ipc_set_streamserver_addr *resp);
GRPC_ipc_API int CLIENT_ipc_set_streamserver_addr(grpc_t *grpc, PARAM_REQ_ipc_set_streamserver_addr *req, PARAM_RESP_ipc_set_streamserver_addr *resp);

//--- alarm_deployment definition ----

typedef struct{
	GBOOL enable;
	GBOOL bCloudRecord;
	int timeRange_cnt;
	struct{
		int dayOfWeek;
		GBOOL bProtection;
		char *time; //23:59:59
	} *timeRange;
} PARAM_REQ_ipc_alarm_deployment;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_deployment;
int USERDEF_ipc_alarm_deployment(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment *req, PARAM_RESP_ipc_alarm_deployment *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_deployment(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_deployment(grpc_t *grpc, PARAM_RESP_ipc_alarm_deployment *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_deployment(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment *req, PARAM_RESP_ipc_alarm_deployment *resp);

//--- alarm_deployment_query definition ----

typedef struct{
	int cid;
} PARAM_REQ_ipc_alarm_deployment_query;

typedef struct{
	int cid;
	GBOOL enable;
	GBOOL bCloudRecord;
	int timeRange_cnt;
	struct{
		int dayOfWeek;
		GBOOL bProtection;
		char *time; //23:59:59
	} *timeRange;
} PARAM_RESP_ipc_alarm_deployment_query;
int USERDEF_ipc_alarm_deployment_query(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_query *req, PARAM_RESP_ipc_alarm_deployment_query *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_deployment_query(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_query *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_deployment_query(grpc_t *grpc, PARAM_RESP_ipc_alarm_deployment_query *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_deployment_query(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_query *req, PARAM_RESP_ipc_alarm_deployment_query *resp);

//--- alarm_deployment_push definition ----

typedef struct{
	GBOOL enable;
	GBOOL bCloudRecord;
	int timeRange_cnt;
	struct{
		int dayOfWeek;
		GBOOL bProtection;
		char *time; //23:59:59
	} *timeRange;
} PARAM_REQ_ipc_alarm_deployment_push;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_deployment_push;
int USERDEF_ipc_alarm_deployment_push(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_push *req, PARAM_RESP_ipc_alarm_deployment_push *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_deployment_push(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_push *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_deployment_push(grpc_t *grpc, PARAM_RESP_ipc_alarm_deployment_push *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_deployment_push(grpc_t *grpc, PARAM_REQ_ipc_alarm_deployment_push *req, PARAM_RESP_ipc_alarm_deployment_push *resp);

//--- alarm_out definition ----

typedef struct{
	int port;
	GBOOL status;
} PARAM_REQ_ipc_alarm_out;

typedef struct{
	int idle;
} PARAM_RESP_ipc_alarm_out;
int USERDEF_ipc_alarm_out(grpc_t *grpc, PARAM_REQ_ipc_alarm_out *req, PARAM_RESP_ipc_alarm_out *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_out(grpc_t *grpc, PARAM_REQ_ipc_alarm_out *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_out(grpc_t *grpc, PARAM_RESP_ipc_alarm_out *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_out(grpc_t *grpc, PARAM_REQ_ipc_alarm_out *req, PARAM_RESP_ipc_alarm_out *resp);

//--- alarm_get_status definition ----

typedef struct{
	int port;
} PARAM_REQ_ipc_alarm_get_status;

typedef struct{
	int port;
	GBOOL status;
} PARAM_RESP_ipc_alarm_get_status;
int USERDEF_ipc_alarm_get_status(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_status *req, PARAM_RESP_ipc_alarm_get_status *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_alarm_get_status(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_status *req);
GRPC_ipc_API int CLIENT_RESP_ipc_alarm_get_status(grpc_t *grpc, PARAM_RESP_ipc_alarm_get_status *resp);
GRPC_ipc_API int CLIENT_ipc_alarm_get_status(grpc_t *grpc, PARAM_REQ_ipc_alarm_get_status *req, PARAM_RESP_ipc_alarm_get_status *resp);

//--- ai_get_param definition ----
/***********************************************
sampleRate : 8000,11025,16000,22050,24000,32000,44100,48000;
bitWidth : 8,16,32
************************************************/

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ai_get_param;

typedef struct{
	int sampleRate;
	int bitWidth;
	char *encType; //pcm,g711a,g711u,g726_16k,g726_24k,g726_32k,g726_40k,aac,adpcm
} PARAM_RESP_ipc_ai_get_param;
int USERDEF_ipc_ai_get_param(grpc_t *grpc, PARAM_REQ_ipc_ai_get_param *req, PARAM_RESP_ipc_ai_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ai_get_param(grpc_t *grpc, PARAM_REQ_ipc_ai_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ai_get_param(grpc_t *grpc, PARAM_RESP_ipc_ai_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_ai_get_param(grpc_t *grpc, PARAM_REQ_ipc_ai_get_param *req, PARAM_RESP_ipc_ai_get_param *resp);

//--- ai_set_param definition ----
/***********************************************
sampleRate : 8000,11025,16000,22050,24000,32000,44100,48000;
bitWidth : 8,16,32
************************************************/

typedef struct{
	int channelid;
	struct{
		int sampleRate;
		int bitWidth;
		char *encType; //pcm,g711a,g711u,g726_16k,g726_24k,g726_32k,g726_40k,aac,adpcm
	} audioAttr;
} PARAM_REQ_ipc_ai_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ai_set_param;
int USERDEF_ipc_ai_set_param(grpc_t *grpc, PARAM_REQ_ipc_ai_set_param *req, PARAM_RESP_ipc_ai_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ai_set_param(grpc_t *grpc, PARAM_REQ_ipc_ai_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ai_set_param(grpc_t *grpc, PARAM_RESP_ipc_ai_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_ai_set_param(grpc_t *grpc, PARAM_REQ_ipc_ai_set_param *req, PARAM_RESP_ipc_ai_set_param *resp);

//--- ao_get_param definition ----
/***********************************************
sampleRate : 8000,11025,16000,22050,24000,32000,44100,48000;
bitWidth : 8,16,32
************************************************/

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ao_get_param;

typedef struct{
	int sampleRate;
	int bitWidth;
	char *encType; //pcm,g711a,g711u,g726_16k,g726_24k,g726_32k,g726_40k,aac,adpcm
} PARAM_RESP_ipc_ao_get_param;
int USERDEF_ipc_ao_get_param(grpc_t *grpc, PARAM_REQ_ipc_ao_get_param *req, PARAM_RESP_ipc_ao_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ao_get_param(grpc_t *grpc, PARAM_REQ_ipc_ao_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ao_get_param(grpc_t *grpc, PARAM_RESP_ipc_ao_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_ao_get_param(grpc_t *grpc, PARAM_REQ_ipc_ao_get_param *req, PARAM_RESP_ipc_ao_get_param *resp);

//--- ao_set_param definition ----
/***********************************************
sampleRate : 8000,11025,16000,22050,24000,32000,44100,48000;
bitWidth : 8,16,32
************************************************/

typedef struct{
	int channelid;
	struct{
		int sampleRate;
		int bitWidth;
		char *encType; //pcm,g711a,g711u,g726_16k,g726_24k,g726_32k,g726_40k,aac,adpcm
	} audioAttr;
} PARAM_REQ_ipc_ao_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ao_set_param;
int USERDEF_ipc_ao_set_param(grpc_t *grpc, PARAM_REQ_ipc_ao_set_param *req, PARAM_RESP_ipc_ao_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ao_set_param(grpc_t *grpc, PARAM_REQ_ipc_ao_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ao_set_param(grpc_t *grpc, PARAM_RESP_ipc_ao_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_ao_set_param(grpc_t *grpc, PARAM_REQ_ipc_ao_set_param *req, PARAM_RESP_ipc_ao_set_param *resp);

//--- connection_get_list definition ----

typedef struct{
	char *conType; //all,jovision,rtsp,gb28181,psia,other
} PARAM_REQ_ipc_connection_get_list;

typedef struct{
	int connectionList_cnt;
	struct{
		char *conType; //jovision
		int key;
		char *addr; //192.168.7.160
		char *user; //admin
	} *connectionList;
} PARAM_RESP_ipc_connection_get_list;
int USERDEF_ipc_connection_get_list(grpc_t *grpc, PARAM_REQ_ipc_connection_get_list *req, PARAM_RESP_ipc_connection_get_list *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_connection_get_list(grpc_t *grpc, PARAM_REQ_ipc_connection_get_list *req);
GRPC_ipc_API int CLIENT_RESP_ipc_connection_get_list(grpc_t *grpc, PARAM_RESP_ipc_connection_get_list *resp);
GRPC_ipc_API int CLIENT_ipc_connection_get_list(grpc_t *grpc, PARAM_REQ_ipc_connection_get_list *req, PARAM_RESP_ipc_connection_get_list *resp);

//--- connection_breakoff definition ----

typedef struct{
	int connectionList_cnt;
	struct{
		char *conType; //jovision
		int key;
	} *connectionList;
} PARAM_REQ_ipc_connection_breakoff;

typedef struct{
	int idle;
} PARAM_RESP_ipc_connection_breakoff;
int USERDEF_ipc_connection_breakoff(grpc_t *grpc, PARAM_REQ_ipc_connection_breakoff *req, PARAM_RESP_ipc_connection_breakoff *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_connection_breakoff(grpc_t *grpc, PARAM_REQ_ipc_connection_breakoff *req);
GRPC_ipc_API int CLIENT_RESP_ipc_connection_breakoff(grpc_t *grpc, PARAM_RESP_ipc_connection_breakoff *resp);
GRPC_ipc_API int CLIENT_ipc_connection_breakoff(grpc_t *grpc, PARAM_REQ_ipc_connection_breakoff *req, PARAM_RESP_ipc_connection_breakoff *resp);

//--- dev_get_hwinfo definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_dev_get_hwinfo;

typedef struct{
	char *type; //IPC
	char *hardware; //V1.0
	char *sn; //112233
	char *firmware; //V1.0
	char *manufacture; //JOVISION
	char *model; //JVS-N71-HD
	GBOOL bPtzSupport;
	GBOOL bWifiSupport;
	int channelCnt;
	int streamCnt;
	char *ystID; //A402153844
} PARAM_RESP_ipc_dev_get_hwinfo;
int USERDEF_ipc_dev_get_hwinfo(grpc_t *grpc, PARAM_REQ_ipc_dev_get_hwinfo *req, PARAM_RESP_ipc_dev_get_hwinfo *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_get_hwinfo(grpc_t *grpc, PARAM_REQ_ipc_dev_get_hwinfo *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_get_hwinfo(grpc_t *grpc, PARAM_RESP_ipc_dev_get_hwinfo *resp);
GRPC_ipc_API int CLIENT_ipc_dev_get_hwinfo(grpc_t *grpc, PARAM_REQ_ipc_dev_get_hwinfo *req, PARAM_RESP_ipc_dev_get_hwinfo *resp);

//--- dev_get_info definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_dev_get_info;

typedef struct{
	GBOOL bDiscoverable;
	char *hostname; //HD IPC
	char *name; //HD IPC
	char *rebootDay; //never,everyday,everysunday,everymonday,everytuesday,everywednesday,everythursday,everyfriday,everysaturday
	int rebootHour;
} PARAM_RESP_ipc_dev_get_info;
int USERDEF_ipc_dev_get_info(grpc_t *grpc, PARAM_REQ_ipc_dev_get_info *req, PARAM_RESP_ipc_dev_get_info *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_get_info(grpc_t *grpc, PARAM_REQ_ipc_dev_get_info *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_get_info(grpc_t *grpc, PARAM_RESP_ipc_dev_get_info *resp);
GRPC_ipc_API int CLIENT_ipc_dev_get_info(grpc_t *grpc, PARAM_REQ_ipc_dev_get_info *req, PARAM_RESP_ipc_dev_get_info *resp);

//--- dev_set_info definition ----

typedef struct{
	GBOOL bDiscoverable;
	char *hostname; //HD IPC
	char *name; //HD IPC
	char *rebootDay; //never,everyday,everysunday,everymonday,everytuesday,everywednesday,everythursday,everyfriday,everysaturday
	int rebootHour;
} PARAM_REQ_ipc_dev_set_info;

typedef struct{
	int idle;
} PARAM_RESP_ipc_dev_set_info;
int USERDEF_ipc_dev_set_info(grpc_t *grpc, PARAM_REQ_ipc_dev_set_info *req, PARAM_RESP_ipc_dev_set_info *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_set_info(grpc_t *grpc, PARAM_REQ_ipc_dev_set_info *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_set_info(grpc_t *grpc, PARAM_RESP_ipc_dev_set_info *resp);
GRPC_ipc_API int CLIENT_ipc_dev_set_info(grpc_t *grpc, PARAM_REQ_ipc_dev_set_info *req, PARAM_RESP_ipc_dev_set_info *resp);

//--- dev_stime definition ----
/***********************************************
Param tmsec means the seconds elapsed since 1970.
************************************************/

typedef struct{
	int tmsec;
	char *tz; //8.5
} PARAM_REQ_ipc_dev_stime;

typedef struct{
	int idle;
} PARAM_RESP_ipc_dev_stime;
int USERDEF_ipc_dev_stime(grpc_t *grpc, PARAM_REQ_ipc_dev_stime *req, PARAM_RESP_ipc_dev_stime *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_stime(grpc_t *grpc, PARAM_REQ_ipc_dev_stime *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_stime(grpc_t *grpc, PARAM_RESP_ipc_dev_stime *resp);
GRPC_ipc_API int CLIENT_ipc_dev_stime(grpc_t *grpc, PARAM_REQ_ipc_dev_stime *req, PARAM_RESP_ipc_dev_stime *resp);

//--- dev_gtime definition ----
/***********************************************
Param tmsec means the seconds elapsed since 1970.
************************************************/

typedef struct{
	int idle;
} PARAM_REQ_ipc_dev_gtime;

typedef struct{
	int tmsec;
	char *tz; //8.5
} PARAM_RESP_ipc_dev_gtime;
int USERDEF_ipc_dev_gtime(grpc_t *grpc, PARAM_REQ_ipc_dev_gtime *req, PARAM_RESP_ipc_dev_gtime *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_gtime(grpc_t *grpc, PARAM_REQ_ipc_dev_gtime *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_gtime(grpc_t *grpc, PARAM_RESP_ipc_dev_gtime *resp);
GRPC_ipc_API int CLIENT_ipc_dev_gtime(grpc_t *grpc, PARAM_REQ_ipc_dev_gtime *req, PARAM_RESP_ipc_dev_gtime *resp);

//--- dev_ntp_set definition ----

typedef struct{
	GBOOL bEnableNtp;
	int sntpInterval;
	int servers_cnt;
	char **servers; //ntp.fudan.edu.cn
} PARAM_REQ_ipc_dev_ntp_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_dev_ntp_set;
int USERDEF_ipc_dev_ntp_set(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_set *req, PARAM_RESP_ipc_dev_ntp_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_ntp_set(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_ntp_set(grpc_t *grpc, PARAM_RESP_ipc_dev_ntp_set *resp);
GRPC_ipc_API int CLIENT_ipc_dev_ntp_set(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_set *req, PARAM_RESP_ipc_dev_ntp_set *resp);

//--- dev_ntp_get definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_dev_ntp_get;

typedef struct{
	GBOOL bEnableNtp;
	int sntpInterval;
	int servers_cnt;
	char **servers; //ntp.fudan.edu.cn
} PARAM_RESP_ipc_dev_ntp_get;
int USERDEF_ipc_dev_ntp_get(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_get *req, PARAM_RESP_ipc_dev_ntp_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_ntp_get(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_ntp_get(grpc_t *grpc, PARAM_RESP_ipc_dev_ntp_get *resp);
GRPC_ipc_API int CLIENT_ipc_dev_ntp_get(grpc_t *grpc, PARAM_REQ_ipc_dev_ntp_get *req, PARAM_RESP_ipc_dev_ntp_get *resp);

//--- dev_reboot definition ----

typedef struct{
	int delaymSec;
} PARAM_REQ_ipc_dev_reboot;

typedef struct{
	int idle;
} PARAM_RESP_ipc_dev_reboot;
int USERDEF_ipc_dev_reboot(grpc_t *grpc, PARAM_REQ_ipc_dev_reboot *req, PARAM_RESP_ipc_dev_reboot *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_reboot(grpc_t *grpc, PARAM_REQ_ipc_dev_reboot *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_reboot(grpc_t *grpc, PARAM_RESP_ipc_dev_reboot *resp);
GRPC_ipc_API int CLIENT_ipc_dev_reboot(grpc_t *grpc, PARAM_REQ_ipc_dev_reboot *req, PARAM_RESP_ipc_dev_reboot *resp);

//--- dev_factory_default definition ----

typedef struct{
	GBOOL bHard;
} PARAM_REQ_ipc_dev_factory_default;

typedef struct{
	int idle;
} PARAM_RESP_ipc_dev_factory_default;
int USERDEF_ipc_dev_factory_default(grpc_t *grpc, PARAM_REQ_ipc_dev_factory_default *req, PARAM_RESP_ipc_dev_factory_default *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_factory_default(grpc_t *grpc, PARAM_REQ_ipc_dev_factory_default *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_factory_default(grpc_t *grpc, PARAM_RESP_ipc_dev_factory_default *resp);
GRPC_ipc_API int CLIENT_ipc_dev_factory_default(grpc_t *grpc, PARAM_REQ_ipc_dev_factory_default *req, PARAM_RESP_ipc_dev_factory_default *resp);

//--- dev_update_check definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_dev_update_check;

typedef struct{
	GBOOL bNeedUpdate;
	char *version; //V2.2.4035 - 20151010     11:01:05
	char *phase; //download,erase,write,free
	int progress;
} PARAM_RESP_ipc_dev_update_check;
int USERDEF_ipc_dev_update_check(grpc_t *grpc, PARAM_REQ_ipc_dev_update_check *req, PARAM_RESP_ipc_dev_update_check *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_update_check(grpc_t *grpc, PARAM_REQ_ipc_dev_update_check *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_update_check(grpc_t *grpc, PARAM_RESP_ipc_dev_update_check *resp);
GRPC_ipc_API int CLIENT_ipc_dev_update_check(grpc_t *grpc, PARAM_REQ_ipc_dev_update_check *req, PARAM_RESP_ipc_dev_update_check *resp);

//--- dev_update definition ----
/***********************************************
method: usb, ftp, http
url: usb, no need; http, no need; FTP like this: ftp://192.168.8.110
************************************************/

typedef struct{
	char *method; //usb,ftp,http
	char *url; //ftp://192.168.8.110/
} PARAM_REQ_ipc_dev_update;

typedef struct{
	int idle;
} PARAM_RESP_ipc_dev_update;
int USERDEF_ipc_dev_update(grpc_t *grpc, PARAM_REQ_ipc_dev_update *req, PARAM_RESP_ipc_dev_update *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_dev_update(grpc_t *grpc, PARAM_REQ_ipc_dev_update *req);
GRPC_ipc_API int CLIENT_RESP_ipc_dev_update(grpc_t *grpc, PARAM_RESP_ipc_dev_update *resp);
GRPC_ipc_API int CLIENT_ipc_dev_update(grpc_t *grpc, PARAM_REQ_ipc_dev_update *req, PARAM_RESP_ipc_dev_update *resp);

//--- ifconfig_get_inet definition ----
/***********************************************
The value of quality ranges from 0 to 100.
************************************************/

typedef struct{
	int idle;
} PARAM_REQ_ipc_ifconfig_get_inet;

typedef struct{
	char *iface; //eth,ppp,wifi
	struct{
		char *name; //eth0
		GBOOL bDHCP;
		char *addr; //192.168.7.160
		char *mask; //255.255.255.0
		char *gateway; //192.168.7.1
		char *mac; //E0:62:90:33:58:C7
		char *dns; //202.102.128.68
	} eth;
	struct{
		char *name; //adsl
		char *username; //qlc
		char *passwd; //123456
	} ppp;
	struct{
		char *name; //hehe
		int quality;
		int keystat;
		char *iestat; //wpa,wpa2,wep,plain
		GBOOL bDHCP;
		char *addr; //192.168.7.160
		char *mask; //255.255.255.0
		char *gateway; //192.168.7.1
		char *mac; //E0:62:90:33:58:C7
		char *dns; //202.102.128.68
	} wifi;
} PARAM_RESP_ipc_ifconfig_get_inet;
int USERDEF_ipc_ifconfig_get_inet(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_get_inet *req, PARAM_RESP_ipc_ifconfig_get_inet *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_get_inet(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_get_inet *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_get_inet(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_get_inet *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_get_inet(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_get_inet *req, PARAM_RESP_ipc_ifconfig_get_inet *resp);

//--- ifconfig_eth_set definition ----

typedef struct{
	char *name; //eth0
	GBOOL bDHCP;
	char *addr; //192.168.7.160
	char *mask; //255.255.255.0
	char *gateway; //192.168.7.1
	char *dns; //202.102.128.68
} PARAM_REQ_ipc_ifconfig_eth_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ifconfig_eth_set;
int USERDEF_ipc_ifconfig_eth_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_eth_set *req, PARAM_RESP_ipc_ifconfig_eth_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_eth_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_eth_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_eth_set(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_eth_set *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_eth_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_eth_set *req, PARAM_RESP_ipc_ifconfig_eth_set *resp);

//--- ifconfig_ppp_set definition ----

typedef struct{
	char *name; //adsl
	char *username; //qlc
	char *passwd; //123456
} PARAM_REQ_ipc_ifconfig_ppp_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ifconfig_ppp_set;
int USERDEF_ipc_ifconfig_ppp_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_ppp_set *req, PARAM_RESP_ipc_ifconfig_ppp_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_ppp_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_ppp_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_ppp_set(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_ppp_set *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_ppp_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_ppp_set *req, PARAM_RESP_ipc_ifconfig_ppp_set *resp);

//--- ifconfig_wifi_connect definition ----

typedef struct{
	char *name; //hehe
	char *passwd; //hehe12345
} PARAM_REQ_ipc_ifconfig_wifi_connect;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ifconfig_wifi_connect;
int USERDEF_ipc_ifconfig_wifi_connect(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_connect *req, PARAM_RESP_ipc_ifconfig_wifi_connect *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_wifi_connect(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_connect *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_wifi_connect(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_wifi_connect *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_wifi_connect(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_connect *req, PARAM_RESP_ipc_ifconfig_wifi_connect *resp);

//--- ifconfig_wifi_list_ap definition ----
/***********************************************
The value of quality ranges from 0 to 100.
************************************************/

typedef struct{
	GBOOL bResearch;
} PARAM_REQ_ipc_ifconfig_wifi_list_ap;

typedef struct{
	int apList_cnt;
	struct{
		char *name; //hehe
		char *passwd; //hehe12345
		int quality;
		int keystat;
		char *iestat; //wpa,wpa2,wep,plain
	} *apList;
} PARAM_RESP_ipc_ifconfig_wifi_list_ap;
int USERDEF_ipc_ifconfig_wifi_list_ap(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_list_ap *req, PARAM_RESP_ipc_ifconfig_wifi_list_ap *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_wifi_list_ap(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_list_ap *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_wifi_list_ap(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_wifi_list_ap *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_wifi_list_ap(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_wifi_list_ap *req, PARAM_RESP_ipc_ifconfig_wifi_list_ap *resp);

//--- ifconfig_server_set definition ----

typedef struct{
	struct{
		char *ipaddr; //192.168.7.160
		int port;
	} vmsServer;
	struct{
		GBOOL bEnable;
		int channel;
		char *serverURL; //rtmp://192.168.100.10:1935/a381_1
	} rtmpServer;
} PARAM_REQ_ipc_ifconfig_server_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ifconfig_server_set;
int USERDEF_ipc_ifconfig_server_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_set *req, PARAM_RESP_ipc_ifconfig_server_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_server_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_server_set(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_server_set *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_server_set(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_set *req, PARAM_RESP_ipc_ifconfig_server_set *resp);

//--- ifconfig_server_get definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_ifconfig_server_get;

typedef struct{
	struct{
		char *ipaddr; //192.168.7.160
		int port;
	} vmsServer;
	struct{
		GBOOL bEnable;
		int channel;
		char *serverURL; //rtmp://192.168.100.10:1935/a381_1
	} rtmpServer;
} PARAM_RESP_ipc_ifconfig_server_get;
int USERDEF_ipc_ifconfig_server_get(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_get *req, PARAM_RESP_ipc_ifconfig_server_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ifconfig_server_get(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ifconfig_server_get(grpc_t *grpc, PARAM_RESP_ipc_ifconfig_server_get *resp);
GRPC_ipc_API int CLIENT_ipc_ifconfig_server_get(grpc_t *grpc, PARAM_REQ_ipc_ifconfig_server_get *req, PARAM_RESP_ipc_ifconfig_server_get *resp);

//--- image_get_param definition ----
/***********************************************
The value of contrast, brightness, saturation and sharpen ranges from 0 to 255.
************************************************/

typedef struct{
	int channelid;
} PARAM_REQ_ipc_image_get_param;

typedef struct{
	int contrast;
	int brightness;
	int saturation;
	int sharpen;
	int exposureMax;
	int exposureMin;
	char *scene; //indoor,outdoor,default,soft
	char *daynightMode; //auto,alwaysDay,alwaysNight,timer
	struct{
		int hour;
		int minute;
	} dayStart;
	struct{
		int hour;
		int minute;
	} dayEnd;
	GBOOL bEnableAWB;
	GBOOL bEnableMI;
	GBOOL bEnableST;
	GBOOL bEnableNoC;
	GBOOL bEnableWDynamic;
	GBOOL bAutoLowFrameEn;
	GBOOL bNightOptimization;
} PARAM_RESP_ipc_image_get_param;
int USERDEF_ipc_image_get_param(grpc_t *grpc, PARAM_REQ_ipc_image_get_param *req, PARAM_RESP_ipc_image_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_image_get_param(grpc_t *grpc, PARAM_REQ_ipc_image_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_image_get_param(grpc_t *grpc, PARAM_RESP_ipc_image_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_image_get_param(grpc_t *grpc, PARAM_REQ_ipc_image_get_param *req, PARAM_RESP_ipc_image_get_param *resp);

//--- image_set_param definition ----
/***********************************************
The value of contrast, brightness, saturation and sharpen ranges from 0 to 255.
************************************************/

typedef struct{
	int channelid;
	int contrast;
	int brightness;
	int saturation;
	int sharpen;
	int exposureMax;
	int exposureMin;
	char *scene; //indoor,outdoor,default,soft
	char *daynightMode; //auto,alwaysDay,alwaysNight,timer
	struct{
		int hour;
		int minute;
	} dayStart;
	struct{
		int hour;
		int minute;
	} dayEnd;
	GBOOL bEnableAWB;
	GBOOL bEnableMI;
	GBOOL bEnableST;
	GBOOL bEnableNoC;
	GBOOL bEnableWDynamic;
	GBOOL bAutoLowFrameEn;
	GBOOL bNightOptimization;
} PARAM_REQ_ipc_image_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_image_set_param;
int USERDEF_ipc_image_set_param(grpc_t *grpc, PARAM_REQ_ipc_image_set_param *req, PARAM_RESP_ipc_image_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_image_set_param(grpc_t *grpc, PARAM_REQ_ipc_image_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_image_set_param(grpc_t *grpc, PARAM_RESP_ipc_image_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_image_set_param(grpc_t *grpc, PARAM_REQ_ipc_image_set_param *req, PARAM_RESP_ipc_image_set_param *resp);

//--- log_get definition ----
/***********************************************
type: enum{SYSTEM,CONFIG,DATA_MANGE,ALARM,RECORD,USER,FILE_OPER,ALL}.
nSub: affiliated log counts
************************************************/

typedef struct{
	char *date; //2014-11-18
	char *type; //SYSTEM
	int page;
} PARAM_REQ_ipc_log_get;

typedef struct{
	int log_pages_cnt;
	int log_items_cnt;
	struct{
		char *time; //2014-11-18 08:43:57
		char *strlog; //the happened events
		GBOOL bNetuser;
		GBOOL bmain;
		int nSub;
		char *type; //SYSTEM
		char *username; //system
	} *log_items;
} PARAM_RESP_ipc_log_get;
int USERDEF_ipc_log_get(grpc_t *grpc, PARAM_REQ_ipc_log_get *req, PARAM_RESP_ipc_log_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_log_get(grpc_t *grpc, PARAM_REQ_ipc_log_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_log_get(grpc_t *grpc, PARAM_RESP_ipc_log_get *resp);
GRPC_ipc_API int CLIENT_ipc_log_get(grpc_t *grpc, PARAM_REQ_ipc_log_get *req, PARAM_RESP_ipc_log_get *resp);

//--- log_clear definition ----
/***********************************************
type: enum{SYSTEM,CONFIG,DATA_MANGE,ALARM,RECORD,USER,FILE_OPER,ALL}.
************************************************/

typedef struct{
	int idle;
} PARAM_REQ_ipc_log_clear;

typedef struct{
	int idle;
} PARAM_RESP_ipc_log_clear;
int USERDEF_ipc_log_clear(grpc_t *grpc, PARAM_REQ_ipc_log_clear *req, PARAM_RESP_ipc_log_clear *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_log_clear(grpc_t *grpc, PARAM_REQ_ipc_log_clear *req);
GRPC_ipc_API int CLIENT_RESP_ipc_log_clear(grpc_t *grpc, PARAM_RESP_ipc_log_clear *resp);
GRPC_ipc_API int CLIENT_ipc_log_clear(grpc_t *grpc, PARAM_REQ_ipc_log_clear *req, PARAM_RESP_ipc_log_clear *resp);

//--- mdetect_set_param definition ----
/***********************************************
The max number of rects is 4, 0 means full screen motion detect.
Param nSensitivity ranges from 0 to 255.
Param x,y,w,h is measured in pixels.
************************************************/

typedef struct{
	int channelid;
	struct{
		GBOOL bEnable;
		GBOOL bEnableRecord;
		int sensitivity;
		int delay;
		GBOOL bOutClient;
		GBOOL bOutEmail;
		int rects_cnt;
		struct{
			int x;
			int y;
			int w;
			int h;
		} *rects;
	} md;
} PARAM_REQ_ipc_mdetect_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_mdetect_set_param;
int USERDEF_ipc_mdetect_set_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_set_param *req, PARAM_RESP_ipc_mdetect_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_mdetect_set_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_mdetect_set_param(grpc_t *grpc, PARAM_RESP_ipc_mdetect_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_mdetect_set_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_set_param *req, PARAM_RESP_ipc_mdetect_set_param *resp);

//--- mdetect_get_param definition ----
/***********************************************
The max number of rects is 4, 0 means full screen motion detect.
Param nSensitivity ranges from 0 to 255.
Param x,y,w,h is measured in pixels.
************************************************/

typedef struct{
	int channelid;
} PARAM_REQ_ipc_mdetect_get_param;

typedef struct{
	GBOOL bEnable;
	GBOOL bEnableRecord;
	int sensitivity;
	int delay;
	GBOOL bOutClient;
	GBOOL bOutEmail;
	int rects_cnt;
	struct{
		int x;
		int y;
		int w;
		int h;
	} *rects;
} PARAM_RESP_ipc_mdetect_get_param;
int USERDEF_ipc_mdetect_get_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_get_param *req, PARAM_RESP_ipc_mdetect_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_mdetect_get_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_mdetect_get_param(grpc_t *grpc, PARAM_RESP_ipc_mdetect_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_mdetect_get_param(grpc_t *grpc, PARAM_REQ_ipc_mdetect_get_param *req, PARAM_RESP_ipc_mdetect_get_param *resp);

//--- mdetect_balarming definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_mdetect_balarming;

typedef struct{
	GBOOL bMdetectAlarming;
} PARAM_RESP_ipc_mdetect_balarming;
int USERDEF_ipc_mdetect_balarming(grpc_t *grpc, PARAM_REQ_ipc_mdetect_balarming *req, PARAM_RESP_ipc_mdetect_balarming *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_mdetect_balarming(grpc_t *grpc, PARAM_REQ_ipc_mdetect_balarming *req);
GRPC_ipc_API int CLIENT_RESP_ipc_mdetect_balarming(grpc_t *grpc, PARAM_RESP_ipc_mdetect_balarming *resp);
GRPC_ipc_API int CLIENT_ipc_mdetect_balarming(grpc_t *grpc, PARAM_REQ_ipc_mdetect_balarming *req, PARAM_RESP_ipc_mdetect_balarming *resp);

//--- chnosd_get_param definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_chnosd_get_param;

typedef struct{
	GBOOL bShowOSD;
	char *timeFormat; //YYYY-MM-DD hh:mm:ss
	char *position; //left_top,left_bottom,right_top,right_bottom
	char *timePos; //left_top,left_bottom,right_top,right_bottom
	char *channelName; //HD IPC
	GBOOL bOsdInvColEn;
	GBOOL bLargeOSD;
} PARAM_RESP_ipc_chnosd_get_param;
int USERDEF_ipc_chnosd_get_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_get_param *req, PARAM_RESP_ipc_chnosd_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_chnosd_get_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_chnosd_get_param(grpc_t *grpc, PARAM_RESP_ipc_chnosd_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_chnosd_get_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_get_param *req, PARAM_RESP_ipc_chnosd_get_param *resp);

//--- chnosd_set_param definition ----

typedef struct{
	int channelid;
	struct{
		GBOOL bShowOSD;
		char *timeFormat; //YYYY-MM-DD hh:mm:ss
		char *position; //left_top,left_bottom,right_top,right_bottom
		char *timePos; //left_top,left_bottom,right_top,right_bottom
		char *channelName; //HD IPC
		GBOOL bOsdInvColEn;
		GBOOL bLargeOSD;
	} attr;
} PARAM_REQ_ipc_chnosd_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_chnosd_set_param;
int USERDEF_ipc_chnosd_set_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_set_param *req, PARAM_RESP_ipc_chnosd_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_chnosd_set_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_chnosd_set_param(grpc_t *grpc, PARAM_RESP_ipc_chnosd_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_chnosd_set_param(grpc_t *grpc, PARAM_REQ_ipc_chnosd_set_param *req, PARAM_RESP_ipc_chnosd_set_param *resp);

//--- privacy_get_param definition ----
/***********************************************
The max number of rects is 4.
Param x,y,w,h is measured in pixels.
************************************************/

typedef struct{
	int channelid;
} PARAM_REQ_ipc_privacy_get_param;

typedef struct{
	GBOOL bEnable;
	int rects_cnt;
	struct{
		int x;
		int y;
		int w;
		int h;
	} *rects;
} PARAM_RESP_ipc_privacy_get_param;
int USERDEF_ipc_privacy_get_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_get_param *req, PARAM_RESP_ipc_privacy_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_privacy_get_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_privacy_get_param(grpc_t *grpc, PARAM_RESP_ipc_privacy_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_privacy_get_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_get_param *req, PARAM_RESP_ipc_privacy_get_param *resp);

//--- privacy_set_param definition ----
/***********************************************
The max number of rects is 4.
Param x,y,w,h is measured in pixels.
************************************************/

typedef struct{
	int channelid;
	struct{
		GBOOL bEnable;
		int rects_cnt;
		struct{
			int x;
			int y;
			int w;
			int h;
		} *rects;
	} region;
} PARAM_REQ_ipc_privacy_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_privacy_set_param;
int USERDEF_ipc_privacy_set_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_set_param *req, PARAM_RESP_ipc_privacy_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_privacy_set_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_privacy_set_param(grpc_t *grpc, PARAM_RESP_ipc_privacy_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_privacy_set_param(grpc_t *grpc, PARAM_REQ_ipc_privacy_set_param *req, PARAM_RESP_ipc_privacy_set_param *resp);

//--- ptz_move_start definition ----
/***********************************************
ptz speed ranges from -254 to 254;
 > 0, pan left or tilt up or zoom in, otherwise the opposite direction.
************************************************/

typedef struct{
	int channelid;
	int panLeft;
	int tiltUp;
	int zoomIn;
} PARAM_REQ_ipc_ptz_move_start;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_move_start;
int USERDEF_ipc_ptz_move_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_start *req, PARAM_RESP_ipc_ptz_move_start *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_move_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_start *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_move_start(grpc_t *grpc, PARAM_RESP_ipc_ptz_move_start *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_move_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_start *req, PARAM_RESP_ipc_ptz_move_start *resp);

//--- ptz_move_stop definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ptz_move_stop;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_move_stop;
int USERDEF_ipc_ptz_move_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_stop *req, PARAM_RESP_ipc_ptz_move_stop *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_move_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_stop *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_move_stop(grpc_t *grpc, PARAM_RESP_ipc_ptz_move_stop *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_move_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_move_stop *req, PARAM_RESP_ipc_ptz_move_stop *resp);

//--- ptz_fi_start definition ----
/***********************************************
fi speed ranges from -254 to 254;
 > 0, focus far or iris open, otherwise the opposite direction.
************************************************/

typedef struct{
	int channelid;
	int focusFar;
	int irisOpen;
} PARAM_REQ_ipc_ptz_fi_start;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_fi_start;
int USERDEF_ipc_ptz_fi_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_start *req, PARAM_RESP_ipc_ptz_fi_start *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_fi_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_start *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_fi_start(grpc_t *grpc, PARAM_RESP_ipc_ptz_fi_start *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_fi_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_start *req, PARAM_RESP_ipc_ptz_fi_start *resp);

//--- ptz_fi_stop definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ptz_fi_stop;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_fi_stop;
int USERDEF_ipc_ptz_fi_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_stop *req, PARAM_RESP_ipc_ptz_fi_stop *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_fi_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_stop *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_fi_stop(grpc_t *grpc, PARAM_RESP_ipc_ptz_fi_stop *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_fi_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_fi_stop *req, PARAM_RESP_ipc_ptz_fi_stop *resp);

//--- ptz_preset_set definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal preset number;
 -2 : preset already exists;
 -3 : no enough preset number.
************************************************/

typedef struct{
	int channelid;
	int presetno;
	char *name; //preset1
} PARAM_REQ_ipc_ptz_preset_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_preset_set;
int USERDEF_ipc_ptz_preset_set(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_set *req, PARAM_RESP_ipc_ptz_preset_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_preset_set(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_preset_set(grpc_t *grpc, PARAM_RESP_ipc_ptz_preset_set *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_preset_set(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_set *req, PARAM_RESP_ipc_ptz_preset_set *resp);

//--- ptz_preset_locate definition ----

typedef struct{
	int channelid;
	int presetno;
} PARAM_REQ_ipc_ptz_preset_locate;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_preset_locate;
int USERDEF_ipc_ptz_preset_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_locate *req, PARAM_RESP_ipc_ptz_preset_locate *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_preset_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_locate *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_preset_locate(grpc_t *grpc, PARAM_RESP_ipc_ptz_preset_locate *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_preset_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_locate *req, PARAM_RESP_ipc_ptz_preset_locate *resp);

//--- ptz_preset_delete definition ----

typedef struct{
	int channelid;
	int presetno;
} PARAM_REQ_ipc_ptz_preset_delete;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_preset_delete;
int USERDEF_ipc_ptz_preset_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_delete *req, PARAM_RESP_ipc_ptz_preset_delete *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_preset_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_delete *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_preset_delete(grpc_t *grpc, PARAM_RESP_ipc_ptz_preset_delete *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_preset_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_preset_delete *req, PARAM_RESP_ipc_ptz_preset_delete *resp);

//--- ptz_presets_get definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ptz_presets_get;

typedef struct{
	int presetsList_cnt;
	struct{
		int presetno;
		char *name; //preset1
	} *presetsList;
} PARAM_RESP_ipc_ptz_presets_get;
int USERDEF_ipc_ptz_presets_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_presets_get *req, PARAM_RESP_ipc_ptz_presets_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_presets_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_presets_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_presets_get(grpc_t *grpc, PARAM_RESP_ipc_ptz_presets_get *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_presets_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_presets_get *req, PARAM_RESP_ipc_ptz_presets_get *resp);

//--- ptz_patrol_create definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ptz_patrol_create;

typedef struct{
	int index;
} PARAM_RESP_ipc_ptz_patrol_create;
int USERDEF_ipc_ptz_patrol_create(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_create *req, PARAM_RESP_ipc_ptz_patrol_create *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_create(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_create *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_create(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_create *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_create(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_create *req, PARAM_RESP_ipc_ptz_patrol_create *resp);

//--- ptz_patrol_delete definition ----

typedef struct{
	int channelid;
	int index;
} PARAM_REQ_ipc_ptz_patrol_delete;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_delete;
int USERDEF_ipc_ptz_patrol_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_delete *req, PARAM_RESP_ipc_ptz_patrol_delete *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_delete *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_delete(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_delete *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_delete(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_delete *req, PARAM_RESP_ipc_ptz_patrol_delete *resp);

//--- ptz_patrols_get definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ptz_patrols_get;

typedef struct{
	int patrolsList_cnt;
	struct{
		int patrolid;
	} *patrolsList;
} PARAM_RESP_ipc_ptz_patrols_get;
int USERDEF_ipc_ptz_patrols_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrols_get *req, PARAM_RESP_ipc_ptz_patrols_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrols_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrols_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrols_get(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrols_get *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrols_get(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrols_get *req, PARAM_RESP_ipc_ptz_patrols_get *resp);

//--- ptz_patrol_get_nodes definition ----

typedef struct{
	int channelid;
	int patrolid;
} PARAM_REQ_ipc_ptz_patrol_get_nodes;

typedef struct{
	int patrolNodesList_cnt;
	struct{
		struct{
			int presetno;
			char *name; //preset1
		} preset;
		int staySeconds;
	} *patrolNodesList;
} PARAM_RESP_ipc_ptz_patrol_get_nodes;
int USERDEF_ipc_ptz_patrol_get_nodes(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_get_nodes *req, PARAM_RESP_ipc_ptz_patrol_get_nodes *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_get_nodes(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_get_nodes *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_get_nodes(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_get_nodes *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_get_nodes(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_get_nodes *req, PARAM_RESP_ipc_ptz_patrol_get_nodes *resp);

//--- ptz_patrol_add_node definition ----

typedef struct{
	int channelid;
	int patrolid;
	int presetno;
	int staySeconds;
} PARAM_REQ_ipc_ptz_patrol_add_node;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_add_node;
int USERDEF_ipc_ptz_patrol_add_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_add_node *req, PARAM_RESP_ipc_ptz_patrol_add_node *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_add_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_add_node *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_add_node(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_add_node *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_add_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_add_node *req, PARAM_RESP_ipc_ptz_patrol_add_node *resp);

//--- ptz_patrol_del_node definition ----
/***********************************************
presetindex: the index of this node in patrol, start from 0. if -1, delete all the nodes in patrol.
************************************************/

typedef struct{
	int channelid;
	int patrolid;
	int presetindex;
} PARAM_REQ_ipc_ptz_patrol_del_node;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_del_node;
int USERDEF_ipc_ptz_patrol_del_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_del_node *req, PARAM_RESP_ipc_ptz_patrol_del_node *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_del_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_del_node *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_del_node(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_del_node *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_del_node(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_del_node *req, PARAM_RESP_ipc_ptz_patrol_del_node *resp);

//--- ptz_patrol_set_speed definition ----
/***********************************************
Speed ranges from 0 to 254.
************************************************/

typedef struct{
	int channelid;
	int patrolid;
	int speed;
} PARAM_REQ_ipc_ptz_patrol_set_speed;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_set_speed;
int USERDEF_ipc_ptz_patrol_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_speed *req, PARAM_RESP_ipc_ptz_patrol_set_speed *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_speed *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_set_speed(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_set_speed *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_speed *req, PARAM_RESP_ipc_ptz_patrol_set_speed *resp);

//--- ptz_patrol_set_stay_seconds definition ----

typedef struct{
	int channelid;
	int patrolid;
	int staySeconds;
} PARAM_REQ_ipc_ptz_patrol_set_stay_seconds;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_set_stay_seconds;
int USERDEF_ipc_ptz_patrol_set_stay_seconds(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_stay_seconds *req, PARAM_RESP_ipc_ptz_patrol_set_stay_seconds *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_set_stay_seconds(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_stay_seconds *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_set_stay_seconds(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_set_stay_seconds *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_set_stay_seconds(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_set_stay_seconds *req, PARAM_RESP_ipc_ptz_patrol_set_stay_seconds *resp);

//--- ptz_patrol_locate definition ----

typedef struct{
	int channelid;
	int patrolid;
} PARAM_REQ_ipc_ptz_patrol_locate;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_locate;
int USERDEF_ipc_ptz_patrol_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_locate *req, PARAM_RESP_ipc_ptz_patrol_locate *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_locate *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_locate(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_locate *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_locate(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_locate *req, PARAM_RESP_ipc_ptz_patrol_locate *resp);

//--- ptz_patrol_stop definition ----

typedef struct{
	int channelid;
	int patrolid;
} PARAM_REQ_ipc_ptz_patrol_stop;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_patrol_stop;
int USERDEF_ipc_ptz_patrol_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_stop *req, PARAM_RESP_ipc_ptz_patrol_stop *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_patrol_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_stop *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_patrol_stop(grpc_t *grpc, PARAM_RESP_ipc_ptz_patrol_stop *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_patrol_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_patrol_stop *req, PARAM_RESP_ipc_ptz_patrol_stop *resp);

//--- ptz_scan_set_left definition ----

typedef struct{
	int channelid;
	int groupid;
} PARAM_REQ_ipc_ptz_scan_set_left;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_scan_set_left;
int USERDEF_ipc_ptz_scan_set_left(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_left *req, PARAM_RESP_ipc_ptz_scan_set_left *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_scan_set_left(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_left *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_scan_set_left(grpc_t *grpc, PARAM_RESP_ipc_ptz_scan_set_left *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_scan_set_left(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_left *req, PARAM_RESP_ipc_ptz_scan_set_left *resp);

//--- ptz_scan_set_right definition ----

typedef struct{
	int channelid;
	int groupid;
} PARAM_REQ_ipc_ptz_scan_set_right;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_scan_set_right;
int USERDEF_ipc_ptz_scan_set_right(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_right *req, PARAM_RESP_ipc_ptz_scan_set_right *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_scan_set_right(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_right *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_scan_set_right(grpc_t *grpc, PARAM_RESP_ipc_ptz_scan_set_right *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_scan_set_right(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_right *req, PARAM_RESP_ipc_ptz_scan_set_right *resp);

//--- ptz_scan_start definition ----

typedef struct{
	int channelid;
	int groupid;
} PARAM_REQ_ipc_ptz_scan_start;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_scan_start;
int USERDEF_ipc_ptz_scan_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_start *req, PARAM_RESP_ipc_ptz_scan_start *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_scan_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_start *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_scan_start(grpc_t *grpc, PARAM_RESP_ipc_ptz_scan_start *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_scan_start(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_start *req, PARAM_RESP_ipc_ptz_scan_start *resp);

//--- ptz_scan_stop definition ----

typedef struct{
	int channelid;
	int groupid;
} PARAM_REQ_ipc_ptz_scan_stop;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_scan_stop;
int USERDEF_ipc_ptz_scan_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_stop *req, PARAM_RESP_ipc_ptz_scan_stop *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_scan_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_stop *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_scan_stop(grpc_t *grpc, PARAM_RESP_ipc_ptz_scan_stop *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_scan_stop(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_stop *req, PARAM_RESP_ipc_ptz_scan_stop *resp);

//--- ptz_scan_set_speed definition ----
/***********************************************
Speed ranges from 0 to 254.
************************************************/

typedef struct{
	int channelid;
	int groupid;
	int speed;
} PARAM_REQ_ipc_ptz_scan_set_speed;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_scan_set_speed;
int USERDEF_ipc_ptz_scan_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_speed *req, PARAM_RESP_ipc_ptz_scan_set_speed *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_scan_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_speed *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_scan_set_speed(grpc_t *grpc, PARAM_RESP_ipc_ptz_scan_set_speed *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_scan_set_speed(grpc_t *grpc, PARAM_REQ_ipc_ptz_scan_set_speed *req, PARAM_RESP_ipc_ptz_scan_set_speed *resp);

//--- ptz_auto definition ----
/***********************************************
Speed ranges from 0 to 254.
************************************************/

typedef struct{
	int channelid;
	int speed;
} PARAM_REQ_ipc_ptz_auto;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_auto;
int USERDEF_ipc_ptz_auto(grpc_t *grpc, PARAM_REQ_ipc_ptz_auto *req, PARAM_RESP_ipc_ptz_auto *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_auto(grpc_t *grpc, PARAM_REQ_ipc_ptz_auto *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_auto(grpc_t *grpc, PARAM_RESP_ipc_ptz_auto *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_auto(grpc_t *grpc, PARAM_REQ_ipc_ptz_auto *req, PARAM_RESP_ipc_ptz_auto *resp);

//--- ptz_aux_on definition ----

typedef struct{
	int channelid;
	int auxid;
} PARAM_REQ_ipc_ptz_aux_on;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_aux_on;
int USERDEF_ipc_ptz_aux_on(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_on *req, PARAM_RESP_ipc_ptz_aux_on *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_aux_on(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_on *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_aux_on(grpc_t *grpc, PARAM_RESP_ipc_ptz_aux_on *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_aux_on(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_on *req, PARAM_RESP_ipc_ptz_aux_on *resp);

//--- ptz_aux_off definition ----

typedef struct{
	int channelid;
	int auxid;
} PARAM_REQ_ipc_ptz_aux_off;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_aux_off;
int USERDEF_ipc_ptz_aux_off(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_off *req, PARAM_RESP_ipc_ptz_aux_off *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_aux_off(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_off *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_aux_off(grpc_t *grpc, PARAM_RESP_ipc_ptz_aux_off *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_aux_off(grpc_t *grpc, PARAM_REQ_ipc_ptz_aux_off *req, PARAM_RESP_ipc_ptz_aux_off *resp);

//--- ptz_zoom_zone definition ----

typedef struct{
	int channelid;
	struct{
		int x;
		int y;
		int w;
		int h;
	} zoneinfo;
	int cmd;
} PARAM_REQ_ipc_ptz_zoom_zone;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ptz_zoom_zone;
int USERDEF_ipc_ptz_zoom_zone(grpc_t *grpc, PARAM_REQ_ipc_ptz_zoom_zone *req, PARAM_RESP_ipc_ptz_zoom_zone *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ptz_zoom_zone(grpc_t *grpc, PARAM_REQ_ipc_ptz_zoom_zone *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ptz_zoom_zone(grpc_t *grpc, PARAM_RESP_ipc_ptz_zoom_zone *resp);
GRPC_ipc_API int CLIENT_ipc_ptz_zoom_zone(grpc_t *grpc, PARAM_REQ_ipc_ptz_zoom_zone *req, PARAM_RESP_ipc_ptz_zoom_zone *resp);

//--- record_get definition ----
/***********************************************
enum{ALL,Mon,Tues,Wed,Thur,Fri,Sat,Sun}
************************************************/

typedef struct{
	int channelid;
} PARAM_REQ_ipc_record_get;

typedef struct{
	int record_params_cnt;
	struct{
		GBOOL brecording;
		GBOOL normal_record;
		GBOOL time_record;
		int date_cnt;
		struct{
			char *week; //Mon
			int time_cnt;
			struct{
				int begin_hour;
				int begin_min;
				int end_hour;
				int end_min;
			} *time;
		} *date;
	} *record_params;
} PARAM_RESP_ipc_record_get;
int USERDEF_ipc_record_get(grpc_t *grpc, PARAM_REQ_ipc_record_get *req, PARAM_RESP_ipc_record_get *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_record_get(grpc_t *grpc, PARAM_REQ_ipc_record_get *req);
GRPC_ipc_API int CLIENT_RESP_ipc_record_get(grpc_t *grpc, PARAM_RESP_ipc_record_get *resp);
GRPC_ipc_API int CLIENT_ipc_record_get(grpc_t *grpc, PARAM_REQ_ipc_record_get *req, PARAM_RESP_ipc_record_get *resp);

//--- record_set definition ----
/***********************************************
enum{ALL,Mon,Tues,Wed,Thur,Fri,Sat,Sun}
************************************************/

typedef struct{
	int record_params_cnt;
	struct{
		int channelid;
		GBOOL normal_record;
		GBOOL time_record;
		int date_cnt;
		struct{
			char *week; //Mon
			int time_cnt;
			struct{
				int begin_hour;
				int begin_min;
				int end_hour;
				int end_min;
			} *time;
		} *date;
	} *record_params;
} PARAM_REQ_ipc_record_set;

typedef struct{
	int idle;
} PARAM_RESP_ipc_record_set;
int USERDEF_ipc_record_set(grpc_t *grpc, PARAM_REQ_ipc_record_set *req, PARAM_RESP_ipc_record_set *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_record_set(grpc_t *grpc, PARAM_REQ_ipc_record_set *req);
GRPC_ipc_API int CLIENT_RESP_ipc_record_set(grpc_t *grpc, PARAM_RESP_ipc_record_set *resp);
GRPC_ipc_API int CLIENT_ipc_record_set(grpc_t *grpc, PARAM_REQ_ipc_record_set *req, PARAM_RESP_ipc_record_set *resp);

//--- storage_get_info definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_storage_get_info;

typedef struct{
	int size;
	int cylinder;
	int partSize;
	int partition;
	int entryCount;
	int status;
	int curPart;
	int mediaType;
	GBOOL bMounted;
	int partSpace_cnt;
	int *partSpace;
	int freeSpace_cnt;
	int *freeSpace;
} PARAM_RESP_ipc_storage_get_info;
int USERDEF_ipc_storage_get_info(grpc_t *grpc, PARAM_REQ_ipc_storage_get_info *req, PARAM_RESP_ipc_storage_get_info *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_storage_get_info(grpc_t *grpc, PARAM_REQ_ipc_storage_get_info *req);
GRPC_ipc_API int CLIENT_RESP_ipc_storage_get_info(grpc_t *grpc, PARAM_RESP_ipc_storage_get_info *resp);
GRPC_ipc_API int CLIENT_ipc_storage_get_info(grpc_t *grpc, PARAM_REQ_ipc_storage_get_info *req, PARAM_RESP_ipc_storage_get_info *resp);

//--- storage_format definition ----

typedef struct{
	int diskNum;
} PARAM_REQ_ipc_storage_format;

typedef struct{
	int idle;
} PARAM_RESP_ipc_storage_format;
int USERDEF_ipc_storage_format(grpc_t *grpc, PARAM_REQ_ipc_storage_format *req, PARAM_RESP_ipc_storage_format *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_storage_format(grpc_t *grpc, PARAM_REQ_ipc_storage_format *req);
GRPC_ipc_API int CLIENT_RESP_ipc_storage_format(grpc_t *grpc, PARAM_RESP_ipc_storage_format *resp);
GRPC_ipc_API int CLIENT_ipc_storage_format(grpc_t *grpc, PARAM_REQ_ipc_storage_format *req, PARAM_RESP_ipc_storage_format *resp);

//--- storage_error_ignore definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_storage_error_ignore;

typedef struct{
	int idle;
} PARAM_RESP_ipc_storage_error_ignore;
int USERDEF_ipc_storage_error_ignore(grpc_t *grpc, PARAM_REQ_ipc_storage_error_ignore *req, PARAM_RESP_ipc_storage_error_ignore *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_storage_error_ignore(grpc_t *grpc, PARAM_REQ_ipc_storage_error_ignore *req);
GRPC_ipc_API int CLIENT_RESP_ipc_storage_error_ignore(grpc_t *grpc, PARAM_RESP_ipc_storage_error_ignore *resp);
GRPC_ipc_API int CLIENT_ipc_storage_error_ignore(grpc_t *grpc, PARAM_REQ_ipc_storage_error_ignore *req, PARAM_RESP_ipc_storage_error_ignore *resp);

//--- set_oss_folder definition ----

typedef struct{
	char *folderName; //7days
	int fileTimeLen;
} PARAM_REQ_ipc_set_oss_folder;

typedef struct{
	int idle;
} PARAM_RESP_ipc_set_oss_folder;
int USERDEF_ipc_set_oss_folder(grpc_t *grpc, PARAM_REQ_ipc_set_oss_folder *req, PARAM_RESP_ipc_set_oss_folder *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_set_oss_folder(grpc_t *grpc, PARAM_REQ_ipc_set_oss_folder *req);
GRPC_ipc_API int CLIENT_RESP_ipc_set_oss_folder(grpc_t *grpc, PARAM_RESP_ipc_set_oss_folder *resp);
GRPC_ipc_API int CLIENT_ipc_set_oss_folder(grpc_t *grpc, PARAM_REQ_ipc_set_oss_folder *req, PARAM_RESP_ipc_set_oss_folder *resp);

//--- stream_get_param definition ----

typedef struct{
	int channelid;
	int streamid;
} PARAM_REQ_ipc_stream_get_param;

typedef struct{
	int width;
	int height;
	int frameRate;
	int bitRate;
	int ngop_s;
	int quality;
	char *rcMode; //cbr,vbr,fixQP
} PARAM_RESP_ipc_stream_get_param;
int USERDEF_ipc_stream_get_param(grpc_t *grpc, PARAM_REQ_ipc_stream_get_param *req, PARAM_RESP_ipc_stream_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_get_param(grpc_t *grpc, PARAM_REQ_ipc_stream_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_get_param(grpc_t *grpc, PARAM_RESP_ipc_stream_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_stream_get_param(grpc_t *grpc, PARAM_REQ_ipc_stream_get_param *req, PARAM_RESP_ipc_stream_get_param *resp);

//--- stream_get_params definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_stream_get_params;

typedef struct{
	int streams_cnt;
	struct{
		int channelid;
		int streamid;
		int width;
		int height;
		int frameRate;
		int bitRate;
		int ngop_s;
		int quality;
		char *rcMode; //cbr,vbr,fixQP
	} *streams;
} PARAM_RESP_ipc_stream_get_params;
int USERDEF_ipc_stream_get_params(grpc_t *grpc, PARAM_REQ_ipc_stream_get_params *req, PARAM_RESP_ipc_stream_get_params *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_get_params(grpc_t *grpc, PARAM_REQ_ipc_stream_get_params *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_get_params(grpc_t *grpc, PARAM_RESP_ipc_stream_get_params *resp);
GRPC_ipc_API int CLIENT_ipc_stream_get_params(grpc_t *grpc, PARAM_REQ_ipc_stream_get_params *req, PARAM_RESP_ipc_stream_get_params *resp);

//--- stream_set_param definition ----

typedef struct{
	int channelid;
	int streamid;
	int width;
	int height;
	int frameRate;
	int bitRate;
	int ngop_s;
	int quality;
	char *rcMode; //cbr,vbr,fixQP
} PARAM_REQ_ipc_stream_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_stream_set_param;
int USERDEF_ipc_stream_set_param(grpc_t *grpc, PARAM_REQ_ipc_stream_set_param *req, PARAM_RESP_ipc_stream_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_set_param(grpc_t *grpc, PARAM_REQ_ipc_stream_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_set_param(grpc_t *grpc, PARAM_RESP_ipc_stream_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_stream_set_param(grpc_t *grpc, PARAM_REQ_ipc_stream_set_param *req, PARAM_RESP_ipc_stream_set_param *resp);

//--- stream_get_ability definition ----

typedef struct{
	int channelid;
	int streamid;
} PARAM_REQ_ipc_stream_get_ability;

typedef struct{
	int resolutions_cnt;
	struct{
		int width;
		int height;
	} *resolutions;
	struct{
		int width;
		int height;
	} inputRes;
	int maxFramerate;
	int minFramerate;
	int maxQuality;
	int minQuality;
	int maxNGOP;
	int minNGOP;
	int maxKBitrate;
	int minKBitrate;
} PARAM_RESP_ipc_stream_get_ability;
int USERDEF_ipc_stream_get_ability(grpc_t *grpc, PARAM_REQ_ipc_stream_get_ability *req, PARAM_RESP_ipc_stream_get_ability *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_get_ability(grpc_t *grpc, PARAM_REQ_ipc_stream_get_ability *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_get_ability(grpc_t *grpc, PARAM_RESP_ipc_stream_get_ability *resp);
GRPC_ipc_API int CLIENT_ipc_stream_get_ability(grpc_t *grpc, PARAM_REQ_ipc_stream_get_ability *req, PARAM_RESP_ipc_stream_get_ability *resp);

//--- stream_snapshot definition ----

typedef struct{
	int channelid;
	int width;
	int height;
} PARAM_REQ_ipc_stream_snapshot;

typedef struct{
	char *snapshot; //url of snapshot
} PARAM_RESP_ipc_stream_snapshot;
int USERDEF_ipc_stream_snapshot(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot *req, PARAM_RESP_ipc_stream_snapshot *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_snapshot(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_snapshot(grpc_t *grpc, PARAM_RESP_ipc_stream_snapshot *resp);
GRPC_ipc_API int CLIENT_ipc_stream_snapshot(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot *req, PARAM_RESP_ipc_stream_snapshot *resp);

//--- stream_snapshot_base64 definition ----

typedef struct{
	int channelid;
	int width;
	int height;
} PARAM_REQ_ipc_stream_snapshot_base64;

typedef struct{
	char *format; //jpg
	char *snapshot; //base64code
} PARAM_RESP_ipc_stream_snapshot_base64;
int USERDEF_ipc_stream_snapshot_base64(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot_base64 *req, PARAM_RESP_ipc_stream_snapshot_base64 *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_snapshot_base64(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot_base64 *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_snapshot_base64(grpc_t *grpc, PARAM_RESP_ipc_stream_snapshot_base64 *resp);
GRPC_ipc_API int CLIENT_ipc_stream_snapshot_base64(grpc_t *grpc, PARAM_REQ_ipc_stream_snapshot_base64 *req, PARAM_RESP_ipc_stream_snapshot_base64 *resp);

//--- stream_request_idr definition ----

typedef struct{
	int channelid;
	int streamid;
} PARAM_REQ_ipc_stream_request_idr;

typedef struct{
	int idle;
} PARAM_RESP_ipc_stream_request_idr;
int USERDEF_ipc_stream_request_idr(grpc_t *grpc, PARAM_REQ_ipc_stream_request_idr *req, PARAM_RESP_ipc_stream_request_idr *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_stream_request_idr(grpc_t *grpc, PARAM_REQ_ipc_stream_request_idr *req);
GRPC_ipc_API int CLIENT_RESP_ipc_stream_request_idr(grpc_t *grpc, PARAM_RESP_ipc_stream_request_idr *resp);
GRPC_ipc_API int CLIENT_ipc_stream_request_idr(grpc_t *grpc, PARAM_REQ_ipc_stream_request_idr *req, PARAM_RESP_ipc_stream_request_idr *resp);

//--- get_record_list definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal channelid;
 -2 : record not exists;
************************************************/

typedef struct{
	int channelid;
	char *starttime; //20151112102512
	char *endtime; //20151112194020
} PARAM_REQ_ipc_get_record_list;

typedef struct{
	int recordlist_cnt;
	struct{
		char *date; //
		char *filename; //
		int channelid;
	} *recordlist;
} PARAM_RESP_ipc_get_record_list;
int USERDEF_ipc_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_get_record_list *req, PARAM_RESP_ipc_get_record_list *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_get_record_list *req);
GRPC_ipc_API int CLIENT_RESP_ipc_get_record_list(grpc_t *grpc, PARAM_RESP_ipc_get_record_list *resp);
GRPC_ipc_API int CLIENT_ipc_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_get_record_list *req, PARAM_RESP_ipc_get_record_list *resp);

//--- get_audio_status definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal channelid;
 -2 : record not exists;
************************************************/

typedef struct{
	char *session; //
} PARAM_REQ_ipc_get_audio_status;

typedef struct{
	GBOOL playing;
} PARAM_RESP_ipc_get_audio_status;
int USERDEF_ipc_get_audio_status(grpc_t *grpc, PARAM_REQ_ipc_get_audio_status *req, PARAM_RESP_ipc_get_audio_status *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_get_audio_status(grpc_t *grpc, PARAM_REQ_ipc_get_audio_status *req);
GRPC_ipc_API int CLIENT_RESP_ipc_get_audio_status(grpc_t *grpc, PARAM_RESP_ipc_get_audio_status *resp);
GRPC_ipc_API int CLIENT_ipc_get_audio_status(grpc_t *grpc, PARAM_REQ_ipc_get_audio_status *req, PARAM_RESP_ipc_get_audio_status *resp);

//--- set_audio_status definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal channelid;
 -2 : record not exists;
************************************************/

typedef struct{
	char *session; //
	GBOOL play;
} PARAM_REQ_ipc_set_audio_status;

typedef struct{
	int idle;
} PARAM_RESP_ipc_set_audio_status;
int USERDEF_ipc_set_audio_status(grpc_t *grpc, PARAM_REQ_ipc_set_audio_status *req, PARAM_RESP_ipc_set_audio_status *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_set_audio_status(grpc_t *grpc, PARAM_REQ_ipc_set_audio_status *req);
GRPC_ipc_API int CLIENT_RESP_ipc_set_audio_status(grpc_t *grpc, PARAM_RESP_ipc_set_audio_status *resp);
GRPC_ipc_API int CLIENT_ipc_set_audio_status(grpc_t *grpc, PARAM_REQ_ipc_set_audio_status *req, PARAM_RESP_ipc_set_audio_status *resp);

//--- play_record definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal channelid;
 -2 : record not exists;
************************************************/

typedef struct{
	char *session; //
	int status;
	int speed;
	int frame;
} PARAM_REQ_ipc_play_record;

typedef struct{
	int idle;
} PARAM_RESP_ipc_play_record;
int USERDEF_ipc_play_record(grpc_t *grpc, PARAM_REQ_ipc_play_record *req, PARAM_RESP_ipc_play_record *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_play_record(grpc_t *grpc, PARAM_REQ_ipc_play_record *req);
GRPC_ipc_API int CLIENT_RESP_ipc_play_record(grpc_t *grpc, PARAM_RESP_ipc_play_record *resp);
GRPC_ipc_API int CLIENT_ipc_play_record(grpc_t *grpc, PARAM_REQ_ipc_play_record *req, PARAM_RESP_ipc_play_record *resp);

//--- play_record_over definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal channelid;
 -2 : record not exists;
************************************************/

typedef struct{
	int idle;
} PARAM_REQ_ipc_play_record_over;

typedef struct{
	char *devid; //
	char *session; //
	int speed;
} PARAM_RESP_ipc_play_record_over;
int USERDEF_ipc_play_record_over(grpc_t *grpc, PARAM_REQ_ipc_play_record_over *req, PARAM_RESP_ipc_play_record_over *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_play_record_over(grpc_t *grpc, PARAM_REQ_ipc_play_record_over *req);
GRPC_ipc_API int CLIENT_RESP_ipc_play_record_over(grpc_t *grpc, PARAM_RESP_ipc_play_record_over *resp);
GRPC_ipc_API int CLIENT_ipc_play_record_over(grpc_t *grpc, PARAM_REQ_ipc_play_record_over *req, PARAM_RESP_ipc_play_record_over *resp);

//--- get_record_info definition ----
/***********************************************
errorcode : 
 0 : success;
 -1 : illegal channelid;
 -2 : record not exists;
************************************************/

typedef struct{
	char *session; //
} PARAM_REQ_ipc_get_record_info;

typedef struct{
	int totalframe;
	int currframe;
} PARAM_RESP_ipc_get_record_info;
int USERDEF_ipc_get_record_info(grpc_t *grpc, PARAM_REQ_ipc_get_record_info *req, PARAM_RESP_ipc_get_record_info *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_get_record_info(grpc_t *grpc, PARAM_REQ_ipc_get_record_info *req);
GRPC_ipc_API int CLIENT_RESP_ipc_get_record_info(grpc_t *grpc, PARAM_RESP_ipc_get_record_info *resp);
GRPC_ipc_API int CLIENT_ipc_get_record_info(grpc_t *grpc, PARAM_REQ_ipc_get_record_info *req, PARAM_RESP_ipc_get_record_info *resp);

//--- ivp_start definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ivp_start;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_start;
int USERDEF_ipc_ivp_start(grpc_t *grpc, PARAM_REQ_ipc_ivp_start *req, PARAM_RESP_ipc_ivp_start *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_start(grpc_t *grpc, PARAM_REQ_ipc_ivp_start *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_start(grpc_t *grpc, PARAM_RESP_ipc_ivp_start *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_start(grpc_t *grpc, PARAM_REQ_ipc_ivp_start *req, PARAM_RESP_ipc_ivp_start *resp);

//--- ivp_stop definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ivp_stop;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_stop;
int USERDEF_ipc_ivp_stop(grpc_t *grpc, PARAM_REQ_ipc_ivp_stop *req, PARAM_RESP_ipc_ivp_stop *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_stop(grpc_t *grpc, PARAM_REQ_ipc_ivp_stop *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_stop(grpc_t *grpc, PARAM_RESP_ipc_ivp_stop *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_stop(grpc_t *grpc, PARAM_REQ_ipc_ivp_stop *req, PARAM_RESP_ipc_ivp_stop *resp);

//--- ivp_get_param definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ivp_get_param;

typedef struct{
	GBOOL bEnable;
	int nDelay;
	GBOOL bStarting;
	int nRgnCnt;
	int stRegion_cnt;
	struct{
		int nCnt;
		int stPoints_cnt;
		struct{
			int x;
			int y;
		} *stPoints;
		int nIvpCheckMode;
	} *stRegion;
	GBOOL bDrawFrame;
	GBOOL bFlushFrame;
	GBOOL bMarkObject;
	GBOOL bMarkAll;
	GBOOL bOpenCount;
	GBOOL bShowCount;
	GBOOL bPlateSnap;
	int nAlpha;
	int nSen;
	int nThreshold;
	int nStayTime;
	GBOOL bEnableRecord;
	GBOOL bOutAlarm1;
	GBOOL bOutClient;
	GBOOL bOutEMail;
	GBOOL bOutVMS;
	GBOOL bNeedRestart;
	int eCountOSDPos;
	int nCountOSDColor;
	int nCountSaveDays;
	int nTimeIntervalReport;
	char *sSnapRes; //
	GBOOL bLPREn;
	int ivpLprDir;
	GBOOL bIvpLprDisplay;
	int ivpLprPos;
	struct{
		int x;
		int y;
		int width;
		int height;
	} ivpLprROI;
	struct{
		char *ivpLprHttpIP; //192.168.1.243
		int ivpLprHttpPort;
		char *ivpLprHttpAddr; ///devicemanagement/php/plateresult.php
	} ivpLprHttpServer;
	struct{
		char *ivpLprFtpIP; //192.168.1.243
		int ivpLprFtpPort;
		char *ivpLprFtpAccount; //admin
		char *ivpLprFtpPasswd; //123456
		char *ivpLprFtpDir; ///lpr/
	} ivpLprFtpServer;
	GBOOL bIvpLprImgFull;
	GBOOL bIvpLprImgLP;
	int ivpLprReUploadInt;
	int uploadTimeout;
} PARAM_RESP_ipc_ivp_get_param;
int USERDEF_ipc_ivp_get_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_get_param *req, PARAM_RESP_ipc_ivp_get_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_get_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_get_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_get_param(grpc_t *grpc, PARAM_RESP_ipc_ivp_get_param *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_get_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_get_param *req, PARAM_RESP_ipc_ivp_get_param *resp);

//--- ivp_set_param definition ----

typedef struct{
	int channelid;
	GBOOL bEnable;
	int nDelay;
	GBOOL bStarting;
	int nRgnCnt;
	int stRegion_cnt;
	struct{
		int nCnt;
		int stPoints_cnt;
		struct{
			int x;
			int y;
		} *stPoints;
		int nIvpCheckMode;
	} *stRegion;
	GBOOL bDrawFrame;
	GBOOL bFlushFrame;
	GBOOL bMarkObject;
	GBOOL bMarkAll;
	GBOOL bOpenCount;
	GBOOL bShowCount;
	GBOOL bPlateSnap;
	int nAlpha;
	int nSen;
	int nThreshold;
	int nStayTime;
	GBOOL bEnableRecord;
	GBOOL bOutAlarm1;
	GBOOL bOutClient;
	GBOOL bOutEMail;
	GBOOL bOutVMS;
	GBOOL bNeedRestart;
	int eCountOSDPos;
	int nCountOSDColor;
	int nCountSaveDays;
	int nTimeIntervalReport;
	char *sSnapRes; //
	GBOOL bLPREn;
	int ivpLprDir;
	GBOOL bIvpLprDisplay;
	int ivpLprPos;
	struct{
		int x;
		int y;
		int width;
		int height;
	} ivpLprROI;
	struct{
		char *ivpLprHttpIP; //192.168.1.243
		int ivpLprHttpPort;
		char *ivpLprHttpAddr; ///devicemanagement/php/plateresult.php
	} ivpLprHttpServer;
	struct{
		char *ivpLprFtpIP; //192.168.1.243
		int ivpLprFtpPort;
		char *ivpLprFtpAccount; //admin
		char *ivpLprFtpPasswd; //123456
		char *ivpLprFtpDir; ///lpr/
	} ivpLprFtpServer;
	GBOOL bIvpLprImgFull;
	GBOOL bIvpLprImgLP;
	int ivpLprReUploadInt;
	int uploadTimeout;
} PARAM_REQ_ipc_ivp_set_param;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_set_param;
int USERDEF_ipc_ivp_set_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_set_param *req, PARAM_RESP_ipc_ivp_set_param *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_set_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_set_param *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_set_param(grpc_t *grpc, PARAM_RESP_ipc_ivp_set_param *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_set_param(grpc_t *grpc, PARAM_REQ_ipc_ivp_set_param *req, PARAM_RESP_ipc_ivp_set_param *resp);

//--- ivp_lpr_trigger definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_ivp_lpr_trigger;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_lpr_trigger;
int USERDEF_ipc_ivp_lpr_trigger(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_trigger *req, PARAM_RESP_ipc_ivp_lpr_trigger *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_trigger(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_trigger *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_trigger(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_trigger *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_trigger(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_trigger *req, PARAM_RESP_ipc_ivp_lpr_trigger *resp);

//--- ivp_lpr_import_wblist definition ----

typedef struct{
	int whiteList_cnt;
	struct{
		char *lpstr; //³A88888
		char *expDate; //20150630
	} *whiteList;
	int blackList_cnt;
	struct{
		char *lpstr; //³A88888
		char *expDate; //20150630
	} *blackList;
} PARAM_REQ_ipc_ivp_lpr_import_wblist;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_lpr_import_wblist;
int USERDEF_ipc_ivp_lpr_import_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_import_wblist *req, PARAM_RESP_ipc_ivp_lpr_import_wblist *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_import_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_import_wblist *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_import_wblist(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_import_wblist *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_import_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_import_wblist *req, PARAM_RESP_ipc_ivp_lpr_import_wblist *resp);

//--- ivp_lpr_export_wblist definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_ivp_lpr_export_wblist;

typedef struct{
	int whiteList_cnt;
	struct{
		char *lpstr; //³A88888
		char *expDate; //20150630
	} *whiteList;
	int blackList_cnt;
	struct{
		char *lpstr; //³A88888
		char *expDate; //20150630
	} *blackList;
} PARAM_RESP_ipc_ivp_lpr_export_wblist;
int USERDEF_ipc_ivp_lpr_export_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_export_wblist *req, PARAM_RESP_ipc_ivp_lpr_export_wblist *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_export_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_export_wblist *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_export_wblist(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_export_wblist *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_export_wblist(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_export_wblist *req, PARAM_RESP_ipc_ivp_lpr_export_wblist *resp);

//--- ivp_lpr_manual_open_gate definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_ivp_lpr_manual_open_gate;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_lpr_manual_open_gate;
int USERDEF_ipc_ivp_lpr_manual_open_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_open_gate *req, PARAM_RESP_ipc_ivp_lpr_manual_open_gate *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_manual_open_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_open_gate *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_manual_open_gate(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_manual_open_gate *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_manual_open_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_open_gate *req, PARAM_RESP_ipc_ivp_lpr_manual_open_gate *resp);

//--- ivp_lpr_manual_close_gate definition ----

typedef struct{
	int idle;
} PARAM_REQ_ipc_ivp_lpr_manual_close_gate;

typedef struct{
	int idle;
} PARAM_RESP_ipc_ivp_lpr_manual_close_gate;
int USERDEF_ipc_ivp_lpr_manual_close_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_close_gate *req, PARAM_RESP_ipc_ivp_lpr_manual_close_gate *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_manual_close_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_close_gate *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_manual_close_gate(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_manual_close_gate *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_manual_close_gate(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_manual_close_gate *req, PARAM_RESP_ipc_ivp_lpr_manual_close_gate *resp);

//--- ivp_lpr_get_last_record definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ivp_lpr_get_last_record;

typedef struct{
	int recordID;
	int bright;
	int carBright;
	int carColor;
	int colorType;
	int colorValue;
	int confidence;
	int direction;
	char *imagePath; ///mmc/LPRCap/2015_09_09/lpr_1714224504_full.jpg
	char *lpImagePath; ///mmc/LPRCap/2015_09_09/lpr_1714224504_lp.jpg
	char *license; //NULL
	struct{
		struct{
			int bottom;
			int left;
			int right;
			int top;
		} RECT;
	} location;
	struct{
		struct{
			int sec;
			int usec;
		} Timeval;
	} timeStamp;
	int timeUsed;
	int triggerType;
	int type;
} PARAM_RESP_ipc_ivp_lpr_get_last_record;
int USERDEF_ipc_ivp_lpr_get_last_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_last_record *req, PARAM_RESP_ipc_ivp_lpr_get_last_record *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_get_last_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_last_record *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_get_last_record(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_get_last_record *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_get_last_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_last_record *req, PARAM_RESP_ipc_ivp_lpr_get_last_record *resp);

//--- ivp_lpr_get_max_record_id definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ivp_lpr_get_max_record_id;

typedef struct{
	char *maxID; //1024
} PARAM_RESP_ipc_ivp_lpr_get_max_record_id;
int USERDEF_ipc_ivp_lpr_get_max_record_id(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_max_record_id *req, PARAM_RESP_ipc_ivp_lpr_get_max_record_id *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_get_max_record_id(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_max_record_id *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_get_max_record_id(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_get_max_record_id *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_get_max_record_id(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_max_record_id *req, PARAM_RESP_ipc_ivp_lpr_get_max_record_id *resp);

//--- ivp_lpr_get_record_list definition ----

typedef struct{
	int channelid;
} PARAM_REQ_ipc_ivp_lpr_get_record_list;

typedef struct{
	int recordList_cnt;
	struct{
		int recordID;
		int bright;
		int carBright;
		int carColor;
		int colorType;
		int colorValue;
		int confidence;
		int direction;
		char *imagePath; ///mmc/LPRCap/2015_09_09/lpr_1714224504_full.jpg
		char *lpImagePath; ///mmc/LPRCap/2015_09_09/lpr_1714224504_lp.jpg
		char *license; //NULL
		struct{
			struct{
				int bottom;
				int left;
				int right;
				int top;
			} RECT;
		} location;
		struct{
			struct{
				int sec;
				int usec;
			} Timeval;
		} timeStamp;
		int timeUsed;
		int triggerType;
		int type;
	} *recordList;
} PARAM_RESP_ipc_ivp_lpr_get_record_list;
int USERDEF_ipc_ivp_lpr_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record_list *req, PARAM_RESP_ipc_ivp_lpr_get_record_list *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record_list *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_get_record_list(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_get_record_list *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_get_record_list(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record_list *req, PARAM_RESP_ipc_ivp_lpr_get_record_list *resp);

//--- ivp_lpr_get_record definition ----

typedef struct{
	int channelid;
	int recordID;
} PARAM_REQ_ipc_ivp_lpr_get_record;

typedef struct{
	int recordID;
	int bright;
	int carBright;
	int carColor;
	int colorType;
	int colorValue;
	int confidence;
	int direction;
	char *imagePath; ///mmc/LPRCap/2015_09_09/lpr_1714224504_full.jpg
	char *lpImagePath; ///mmc/LPRCap/2015_09_09/lpr_1714224504_lp.jpg
	char *license; //NULL
	struct{
		struct{
			int bottom;
			int left;
			int right;
			int top;
		} RECT;
	} location;
	struct{
		struct{
			int sec;
			int usec;
		} Timeval;
	} timeStamp;
	int timeUsed;
	int triggerType;
	int type;
} PARAM_RESP_ipc_ivp_lpr_get_record;
int USERDEF_ipc_ivp_lpr_get_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record *req, PARAM_RESP_ipc_ivp_lpr_get_record *resp);
GRPC_ipc_API int CLIENT_REQ_ipc_ivp_lpr_get_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record *req);
GRPC_ipc_API int CLIENT_RESP_ipc_ivp_lpr_get_record(grpc_t *grpc, PARAM_RESP_ipc_ivp_lpr_get_record *resp);
GRPC_ipc_API int CLIENT_ipc_ivp_lpr_get_record(grpc_t *grpc, PARAM_REQ_ipc_ivp_lpr_get_record *req, PARAM_RESP_ipc_ivp_lpr_get_record *resp);
#ifdef __cplusplus
}
#endif
#endif
