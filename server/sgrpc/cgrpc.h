/*
 * cgrpc.h
 *
 *  Created on: 2015年01月23日
 *      Author: QinLichao
 *		 Email: qinlichao@jovision.com
 */

#ifndef CGRPC_H_
#define CGRPC_H_
#ifdef __cplusplus
extern "C" {
#endif

/*初始化grpc，创建连接线程与grpc服务器建立连接*/
int cgrpc_init();

typedef struct{
	char startTime[16];		// 安全防护开始时间(格式: hh:mm:ss)
	char endTime[16];			// 安全防护结束时间(格式: hh:mm:ss)	
}timeRange_t;

typedef struct{
	char type[32]; //video,io,analyse
	char subtype[32]; //videoLost,motionDetect,videoBlock,hardDriverLost,doorAlarm,smoke,pir,invasion
	char pir_code[32]; //alarm details
	char detector_id[32]; //xxx
	char dev_id[16]; //A402153844
	char dev_type[8]; //card,dvr,ipc,nvr
	int channel;
} AlarmInfo_t;

typedef struct{
	int bEnable; //是否开启安全防护
	timeRange_t timeRange[7];	// 安全防护时间段设置
} DeploymentInfo_t;

/*发送grpc报警心跳*/
int cgrpc_keep_online();

/*发送grpc报警消息*/
int cgrpc_alarm_report(AlarmInfo_t *alarmInfo);

/*向VMS服务器发送布控撤控消息*/
int cgrpc_alarm_deployment_push(DeploymentInfo_t *deploymentInfo);

#ifdef __cplusplus
}
#endif

#endif /* CGRPC_H_ */
