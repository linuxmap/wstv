/*
 * cgrpc.c
 *
 *  Created on: 2015年01月23日
 *      Author: Qin Lichao
 *		 Email: qinlichao@jovision.com
 */

#include <jv_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "cgrpc.h"
#include "grpc.h"
#include "ipc.h"
#include <cJSON.h>
#include <sp_user.h>
#include <sp_ifconfig.h>
#include <sp_alarm.h>

#include <pthread.h>
#include "utl_iconv.h"
#include "mipcinfo.h"


#define CGRPC_SUPPORT
#define INVALID_SOCKET	-1
#define DELIMITER	"@#$&"
static struct{
	grpc_t *grpc;
	BOOL bRunning;
	BOOL bConnected;
	pthread_t thread;
}cGrpcInfo;

static pthread_mutex_t cMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
	int fd;
	struct sockaddr_in server;
}UserDefInfo;

static int __cgrpc_send(struct _grpc_t *grpc, void *buffer, int len)
{
	UserDefInfo *user = (UserDefInfo *)grpc->userdef;
	char send_buf[32*1024];
	sprintf(send_buf, "%s%s%s", DELIMITER, (char*)buffer, DELIMITER);
	int ret = send(user->fd, send_buf, strlen(send_buf), 0);
	return ret - 2*strlen(DELIMITER);
}

static int __cgrpc_recv(struct _grpc_t *grpc, void *buffer, int len, int *timeoutms)
{
	UserDefInfo *user = (UserDefInfo *)grpc->userdef;
	int ret = recv(user->fd, buffer, len, 0);
	return ret;
}

static void __cgrpc_connection(void *param)
{
	UserDefInfo *user = (UserDefInfo *)(cGrpcInfo.grpc->userdef);
	fd_set	rdfds;
	struct timeval tv;
	int ret = 0;
	SPServer_t serverInfo;
	SPServer_t serverInfoPrev;
	SPAlarmSet_t alarmInfo;
	//BOOL alarmEnable = FALSE;

	pthreadinfo_add((char*)__func__);
	
	char buffer[128*1024];
	int data_len = 0;
	memset(buffer, 0, sizeof(buffer));
	memset(&tv, 0, sizeof(tv));
	
	while(cGrpcInfo.bRunning)
	{
		pthread_mutex_lock(&cMutex);
		while(user->fd == INVALID_SOCKET)
		{
			//建立传输通道,TCP连接
			if((user->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{
				printf("create user->fd error: %s\n", strerror(errno));
				break;
			}
			//printf("connect to tcp server, sock %d\n",user->fd);
			memset(&(user->server), 0, sizeof(user->server));
			user->server.sin_family = AF_INET;
			sp_ifconfig_server_get(&serverInfo);
			user->server.sin_port = htons(serverInfo.vmsServerPort);
			if(inet_pton(AF_INET, serverInfo.vmsServerIp, &user->server.sin_addr) <= 0)
			{
				printf("inet_pton error for %s\n", serverInfo.vmsServerIp);
				close(user->fd);
				user->fd = INVALID_SOCKET;
				break;
			}
			if(connect(user->fd, (struct sockaddr*)&user->server, sizeof(user->server)) < 0)
			{
				//printf("connect to %s:%d error: %s\n", serverInfo.vmsServerIp, serverInfo.vmsServerPort, strerror(errno));
				close(user->fd);
				user->fd = INVALID_SOCKET;
				break;
			}
			cGrpcInfo.bConnected = TRUE;
		}
		pthread_mutex_unlock(&cMutex);
		if(user->fd != INVALID_SOCKET)
			memcpy(&serverInfoPrev, &serverInfo, sizeof(SPServer_t));
		while(user->fd != INVALID_SOCKET)
		{
			sp_ifconfig_server_get(&serverInfo);
			if(strcmp(serverInfo.vmsServerIp, serverInfoPrev.vmsServerIp) != 0
				|| serverInfo.vmsServerPort != serverInfoPrev.vmsServerPort)
			{
				printf("reconnect to VMS server\n");
				data_len = 0;
				close(user->fd);
				user->fd = INVALID_SOCKET;
				cGrpcInfo.bConnected = FALSE;
				break;
			}

			/*安全防护配置打开或者关闭时，向VMS服务器推送消息*/
			sp_alarm_get_param(&alarmInfo);
			/*if(alarmEnable != alarmInfo.bEnable)
			{
				alarmEnable = alarmInfo.bEnable;
				DeploymentInfo_t DeploymentInfo;
				
				DeploymentInfo.bEnable = alarmInfo.bEnable;
				strcpy(DeploymentInfo.timeRange[0].startTime, alarmInfo.alarmTime[0].tStart);
				strcpy(DeploymentInfo.timeRange[0].endTime, alarmInfo.alarmTime[0].tEnd);
				cgrpc_alarm_deployment_push(&DeploymentInfo);
			}*/
			
			FD_ZERO(&rdfds);
			FD_SET(user->fd, &rdfds);
			tv.tv_sec = 3;
			tv.tv_usec = 0;
			ret = select(user->fd + 1, &rdfds, NULL, NULL, &tv);
			//printf("select ret %d\n", ret);
			if(ret < 0)
			{
				perror("select");
				data_len = 0;
				memset(buffer, 0, sizeof(buffer));
				close(user->fd);
				user->fd = INVALID_SOCKET;
				cGrpcInfo.bConnected = FALSE;
				break;
			}
			else if(ret == 0)
			{
				//printf("timeout\n");
				cgrpc_keep_online();
				data_len = 0;
				memset(buffer, 0, sizeof(buffer));
				continue;
			}
			else
			{
				if(FD_ISSET(user->fd, &rdfds))
				{
					pthread_mutex_lock(&cMutex);
					ret = __cgrpc_recv(cGrpcInfo.grpc, buffer+data_len, sizeof(buffer)-data_len, NULL);
					if (ret > 0)
					{
						data_len += ret;
						if(data_len >= 128*1024)
						{
							printf("Error:Message too long!\n");
							data_len = 0;
							memset(buffer, 0, sizeof(buffer));
						}
						else
						{
							buffer[data_len] = '\0';
							if(strncmp(DELIMITER, buffer, strlen(DELIMITER)) != 0)
							{
								printf("Error:Message incorrect!\n");
								data_len = 0;
								memset(buffer, 0, sizeof(buffer));
							}
							else
							{
								if(strncmp(DELIMITER, buffer+data_len-strlen(DELIMITER), strlen(DELIMITER)) != 0)
								{
									printf("Warning:Message incomplete!\n");
								}
								else
								{
									buffer[data_len-strlen(DELIMITER)] = '\0';
									grpc_s_serve_direct(cGrpcInfo.grpc, buffer+strlen(DELIMITER));
									data_len = 0;
									memset(buffer, 0, sizeof(buffer));
								}
							}
						}
					}
					else
					{
						perror("recv");
						data_len = 0;
						memset(buffer, 0, sizeof(buffer));
						close(user->fd);
						user->fd = INVALID_SOCKET;
						cGrpcInfo.bConnected = FALSE;
					}
					pthread_mutex_unlock(&cMutex);
				}
			}
		}
		sleep(3);
	}
}

/*初始化grpc，创建连接线程与grpc服务器建立连接*/
int cgrpc_init()
{
#ifdef CGRPC_SUPPORT
	cGrpcInfo.bRunning = TRUE;
	cGrpcInfo.bConnected = FALSE;
	cGrpcInfo.grpc = grpc_new();
	grpcInitParam_t initParam;
	initParam.userdef = malloc(sizeof(UserDefInfo));
    ((UserDefInfo*)initParam.userdef)->fd = INVALID_SOCKET;
    memset(&(((UserDefInfo*)initParam.userdef)->server), 0, sizeof(struct sockaddr_in));
	initParam.fptr_net.recv = __cgrpc_recv;
	initParam.fptr_net.send = __cgrpc_send;
	initParam.methodList_s = ipc_methodList_s;
	grpc_init(cGrpcInfo.grpc, &initParam);
	pthread_create(&cGrpcInfo.thread, NULL, (void *)__cgrpc_connection, NULL);
#endif
	return 0;
}

/*发送grpc报警心跳*/
int cgrpc_keep_online()
{
	if(cGrpcInfo.bRunning)
	{
		//UserDefInfo *user = (UserDefInfo *)(cGrpcInfo.grpc->userdef);
		if(cGrpcInfo.bConnected == TRUE)
		{
			pthread_mutex_lock(&cMutex);
			PARAM_REQ_ipc_keep_online req;
			ipcinfo_t ipcinfo;
			ipcinfo_get_param(&ipcinfo);
			char ystID[16];
			sprintf(ystID, "%c%d",ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
			req.dev_id= ystID;
			CLIENT_REQ_ipc_keep_online(cGrpcInfo.grpc, &req);
			grpc_end(cGrpcInfo.grpc);
			pthread_mutex_unlock(&cMutex);
		}
	}
	return 0;
}

/*发送grpc报警消息*/
int cgrpc_alarm_report(AlarmInfo_t *alarmInfo)
{
	if(cGrpcInfo.bRunning)
	{
	
		//UserDefInfo *user = (UserDefInfo *)(cGrpcInfo.grpc->userdef);
		if(cGrpcInfo.bConnected == TRUE && alarmInfo != NULL)
		{
			pthread_mutex_lock(&cMutex);
			PARAM_REQ_ipc_alarm_report alarmReport;
			alarmReport.type = alarmInfo->type;
			alarmReport.subtype = alarmInfo->subtype;
			alarmReport.pir_code = alarmInfo->pir_code;
			alarmReport.detector_id = alarmInfo->detector_id;
			alarmReport.dev_id = alarmInfo->dev_id;
			alarmReport.dev_type = alarmInfo->dev_type;
			alarmReport.channel = alarmInfo->channel;
			printf("send alarm\n");
			CLIENT_REQ_ipc_alarm_report(cGrpcInfo.grpc, &alarmReport);
			grpc_end(cGrpcInfo.grpc);
			pthread_mutex_unlock(&cMutex);
		}
	}
	return 0;
}

/*向VMS服务器发送布控撤控消息*/
int cgrpc_alarm_deployment_push(DeploymentInfo_t *deploymentInfo)
{
	if(cGrpcInfo.bRunning)
	{
	
		//UserDefInfo *user = (UserDefInfo *)(cGrpcInfo.grpc->userdef);
		if(cGrpcInfo.bConnected == TRUE && deploymentInfo != NULL)
		{
			pthread_mutex_lock(&cMutex);
			PARAM_REQ_ipc_alarm_deployment_push deploymentPush;
			deploymentPush.enable = deploymentInfo->bEnable;
			deploymentPush.timeRange_cnt = 1;
			deploymentPush.timeRange = grpc_malloc(cGrpcInfo.grpc, deploymentPush.timeRange_cnt * sizeof(*deploymentPush.timeRange));
			//deploymentPush.timeRange[0].startTime = grpc_strdup(cGrpcInfo.grpc, deploymentInfo->timeRange[0].startTime);
			//deploymentPush.timeRange[0].endTime = grpc_strdup(cGrpcInfo.grpc, deploymentInfo->timeRange[0].endTime);
			printf("send deployment\n");
			CLIENT_REQ_ipc_alarm_deployment_push(cGrpcInfo.grpc, &deploymentPush);
			grpc_end(cGrpcInfo.grpc);
			pthread_mutex_unlock(&cMutex);
		}
	}
	return 0;
}

