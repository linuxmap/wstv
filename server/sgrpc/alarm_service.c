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

#include "grpc.h"
#include "ipc.h"
#include <cJSON.h>
#include <sp_user.h>
#include <sp_ifconfig.h>
#include <sp_alarm.h>
#include <sp_devinfo.h>

#include <pthread.h>
#include "utl_iconv.h"
#include "utl_queue.h"
#include "utl_filecfg.h"
#include "utl_ifconfig.h"
#include "mipcinfo.h"
#include "alarm_service.h"
#include "m_rtmp.h"
#include "mstream.h"
#include "maudio.h"
#include "mvoicedec.h"
#include "mlog.h"
#include "jv_ai.h"
#include "jv_ao.h"
#include "malarmout.h"
#include "SYSFuncs.h"
#include "utl_common.h"
#include "mrecord.h"

#ifdef PREREC_SUPPORT
#include "mprerec_upload.h"
#endif
// #include "utl_aes.h"
// #include "utl_base64.h"
#include "httpclient.h"

// 对讲功能还未对接完成，暂不开启
#define SUPPORT_ALARM_SERVICE_TALK	0

#if SUPPORT_ALARM_SERVICE_TALK
#include "voice_client_api.h"
#endif

//#define CLOUD_TEST

#define INVALID_SOCKET	-1
#define ENDCODE	"\r\n"
#define BUFSIZE			4*1024
#define MSG_QUEUE_LEN	8
#define AES_ENCRYPT_KEY	("JovisIonNOiSIVOj")
#define AES_ENCRYPT_IV	("c3I1me2R6od2J3o@")//("ABCDEFGH12345678")

#define	REBOOTLOG	"/etc/conf.d/jovision/reboot.log"
typedef struct{
	char		DevId[16];		// 设备云视通号
	grpc_t		*grpc;
	BOOL		bRunning;
	BOOL		bConnected;
	BOOL		bLogined;
	BOOL		bDeployed;
	BOOL		bRefreshDeploy;	// 需要刷新布撤防状态
	pthread_mutex_t	Mutex;
	pthread_t	heartbeat_thread;
	pthread_t	connection_thread;
	pthread_t	protection_check_thread;
}AlarmServiceInfo_t;

typedef struct
{
	char onlineServer[128];
	unsigned short onlinePort;
	char businessServer[128];
	unsigned short businessPort;
	char recordServer[128];
	unsigned short recordPort;
	BOOL bManualConfig;
}AlarmServerConf_t;

typedef struct{
	int fd;
	struct sockaddr_in server;
}UserDefInfo;

static void _alarm_service_voice_data_callback(const char *buffer, int length);
static void _alarm_service_voice_event_callback(int event_type, int ret);

#ifdef PREREC_SUPPORT
typedef struct{
	char endpoint[128];
	char bucket[128];
	char user[128];
	char pwd[128];
}CloudAccountInfo_t;
#endif

// static pthread_mutex_t alarm_service_mutex = PTHREAD_MUTEX_INITIALIZER;
static AlarmServerConf_t s_ServerConf;
static AlarmServiceInfo_t s_ServiceInfo;
#ifdef PREREC_SUPPORT
static CloudAccountInfo_t cloudInfo;
#endif
static BOOL s_bRevKeepAliveResp;
static BOOL bVoiceConnecting;		//用于标识当前是否有连接
static char voiceClientID[64];		//用于保存当前连接客户端ID
static BOOL bTFCardErrIgnored = FALSE;		//TF卡异常报警忽略
static BOOL bAlarmServiceInited = FALSE;

static int __alarm_service_send(struct _grpc_t *grpc, void *buffer, int len)
{
	UserDefInfo *user = (UserDefInfo *)grpc->userdef;
	char send_buf[32*1024];
	sprintf(send_buf, "%s%s", (char*)buffer, ENDCODE);
	if(strstr(buffer, "keep_online") == NULL)
		printf("__alarm_service_send:\n%s\n", send_buf);
	int ret = send(user->fd, send_buf, strlen(send_buf), 0);
	return ret - strlen(ENDCODE);
}

static int __alarm_service_recv(struct _grpc_t *grpc, void *buffer, int len, int *timeoutms)
{
	UserDefInfo *user = (UserDefInfo *)grpc->userdef;
	int ret = recv(user->fd, buffer, len, 0);
	if(strstr(buffer, "keep_online") == NULL)
		printf("__alarm_service_recv:\n%s\n", (char*)buffer);
	return ret;
}

static int __alarm_service_connect()
{
	int sockFd = INVALID_SOCKET;
	int ret = 0;
	struct sockaddr_in server;
	
	//建立传输通道,TCP连接
	if((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("create socket error: %s\n", strerror(errno));
		return -1;
	}
	printf("connect to server %s:%d\n", s_ServerConf.onlineServer, s_ServerConf.onlinePort);
	struct timeval t;
	t.tv_sec = 10;
	t.tv_usec = 0;

	ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t));	//printf("connect to tcp server, sock %d\n",user->fd);
	if(ret != 0)
	{
		perror("setsockopt");
		close(sockFd);
		return -1;
	}

	char buffer[1024] = "";
	struct hostent *host = NULL, hostinfo;
	gethostbyname_r(s_ServerConf.onlineServer, &hostinfo, buffer, sizeof(buffer), &host, &ret);
	if (host == NULL)
	{
		printf("%s, gethostbyname_r %s failed: %s(%d)\n", __func__, s_ServerConf.onlineServer, strerror(errno), errno);
		return -1;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(s_ServerConf.onlinePort);
	memcpy(&server.sin_addr, host->h_addr, host->h_length);

	if(connect(sockFd, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		printf("connect to server %s:%d failed\n", s_ServerConf.onlineServer, s_ServerConf.onlinePort);
		perror("connect");
		close(sockFd);
		return -1;
	}
	printf("connect to server %s:%d ok\n", s_ServerConf.onlineServer, s_ServerConf.onlinePort);
	return sockFd;
}

static void __alarm_service_login()
{
	if(s_ServiceInfo.bRunning)
	{
		pthread_mutex_lock(&s_ServiceInfo.Mutex);
		if(s_ServiceInfo.bConnected == TRUE)
		{
			PARAM_REQ_ipc_login req;
			int i = 0;
			ipcinfo_t *pIpcInfo = ipcinfo_get_param(NULL);
			
			req.dev_id = s_ServiceInfo.DevId;
			req.data_cnt = 13;
			req.data = grpc_malloc(s_ServiceInfo.grpc, req.data_cnt * sizeof(int));
			for(i = 0; i < req.data_cnt; ++i)
				req.data[i] = pIpcInfo->nDeviceInfo[i];
			CLIENT_REQ_ipc_login(s_ServiceInfo.grpc, &req);
			grpc_end(s_ServiceInfo.grpc);
		}
		pthread_mutex_unlock(&s_ServiceInfo.Mutex);
	}
}

/*发送报警心跳*/
static void __alarm_service_keepalive()
{
	if(s_ServiceInfo.bRunning)
	{
		//UserDefInfo *user = (UserDefInfo *)(s_ServiceInfo.grpc->userdef);
		pthread_mutex_lock(&s_ServiceInfo.Mutex);
		if(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined)
		{
			char heartbeat[BUFSIZE];
			
			s_ServiceInfo.grpc->sentcnt++;
			sprintf(heartbeat, "{\"sentcnt\" : %d,"
							"\"method\" : \"keep_online\","
							"\"param\" : {"
							"\"dev_id\" : \"%s\""
							"}}", 
							s_ServiceInfo.grpc->sentcnt, s_ServiceInfo.DevId);
			__alarm_service_send(s_ServiceInfo.grpc, heartbeat, strlen(heartbeat));
		}
		pthread_mutex_unlock(&s_ServiceInfo.Mutex);
	}
}

/*设备信息上传*/
int __alarm_service_ipcinfo_report()
{
	int ret = -1;
	int len = 0;
	char url[1024] = {0};//"http://192.168.10.10:8081/netalarm-rs/alarm/alarm_report";
	char req[BUFSIZE];
	char resp[BUFSIZE];

	//http://alarm-gw.jovision.com/netalarm-rs/device_data/info_report
	printf("alarm_service_ipcinfo_report\n");
	sprintf(url, "http://%s:%d/netalarm-rs/dev/info_report", s_ServerConf.businessServer, s_ServerConf.businessPort);
	if(s_ServiceInfo.bRunning)
	{
		if(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined)
		{
			s_ServiceInfo.grpc->sentcnt++;
			memset(req, 0, sizeof(req));
			memset(resp, 0, sizeof(resp));
			const SPHWInfo_t *hwipcinfo = sp_dev_get_hwinfo();
			SPDevInfo_t info;
			sp_dev_get_info(&info);
			
			sprintf(req, "{\"sentcnt\" : %d,"
							"\"method\" : \"info_report\","
							"\"param\" : {"
							"\"type\" : \"IPC\","
							"\"hardware\" : \"%s\","
							"\"sn\" : \"%s\","
							"\"firmware\" : \"%s\","
							"\"manufacture\" : \"%s\","
							"\"model\" : \"%s\","
							"\"bPtzSupport\" : %s,"
							"\"channelCnt\" : %d,"
							"\"channelName\": [\"%s\"],"
							"\"streamCnt\" : %d,"
							"\"ystID\" : \"%s\""
							"}}", 
							s_ServiceInfo.grpc->sentcnt,
							//hwipcinfo->type,
							hwipcinfo->type,
							hwipcinfo->sn,
							hwipcinfo->firmware,
							hwipcinfo->manufacture,
							hwipcinfo->model,
							hwipcinfo->ptzBsupport ? "true" : "false",
							1,
							info.name,
							hwipcinfo->streamCnt,
							hwipcinfo->ystID);
							
			len = strlen(req);
			ret = Http_post_message(url, req, len, resp, 10);
			if(ret > 0)
			{
				printf("alarm_service_ipcinfo_report, resp=%s\n", resp);
				cJSON *root = cJSON_Parse(resp);
				if(root != NULL)
				{
					cJSON *error = cJSON_GetObjectItem(root, "error");
					if(error != NULL)
					{
						int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
						if(errorcode == 0)
						{
							printf("alarm_service_ipcinfo_report ok\n");
						}
						else if(errorcode == 1)
							printf("alarm_service_ipcinfo_report ok, device	already exists\n");
						else
							printf("cJSON_GetObjectValueInt err:%d\n", errorcode);
					}
					else
						printf("cJSON_GetObjectItem err\n");
					cJSON_Delete(root);
				}
				else
					printf("cJSON_Parse err\n");
			}
		}
	}
	return ret;
}

int _alarm_service_getserver(const char* url, char* resp)
{
	ipcinfo_t param;
	int ret = -1;
	int len = 0;
	//const char *url = "http://alarm-gw.jovision.com:80/netalarm-rs/balancing/get_servers";

	//char url[1024];
	//sprintf(url, "http://%s:80/netalarm-rs/balancing/get_servers", inet_ntop(host->h_addrtype, host->h_addr, str, sizeof(str)));
	//sprintf(url, "http://%s:8081/netalarm-rs/balancing/get_servers", "192.168.10.10");
	char req[BUFSIZE];
	char ystID[16];
	memset(req, 0, sizeof(req));
	memset(ystID, 0, sizeof(ystID));
	
	ipcinfo_get_param(&param);
	sprintf(ystID, "%c%d",param.nDeviceInfo[6], param.ystID);
	sprintf(req, "{\"mid\" : 0,"
					"\"ct\" : 1,"
					"\"method\" : \"get_servers\","
					"\"param\" : {"
					"\"id\" : \"%s\""
					"}}", ystID);
					
	len = strlen(req);
	ret = Http_post_message(url, req, len, resp, 10);
	printf("httppost resp:\n%s\n", resp);

	return ret;
}

#ifdef PREREC_SUPPORT
static int _alarm_service_getcloudinfo()
{
	int ret = -1;
	char resp[BUFSIZE];

	memset(resp, 0, sizeof(resp));

	char url[512];
	sprintf(url, "http://%s:%d/netalarm-rs/oss/get_ali_info", s_ServerConf.businessServer, s_ServerConf.businessPort);
	printf("url: %s\n", url);
	ret = _alarm_service_getserver(url, resp);

	if(ret <= 0)
	{
		printf("http resp %d\n",ret);
		return ret;
	}

	cJSON *root = cJSON_Parse(resp);
	if(NULL == root)
	{
		printf("invalid json resp\n");
	}

	int result = cJSON_GetObjectValueInt(root, "rt");
	if(result == 0)
	{
		cJSON *param = cJSON_GetObjectItem(root, "param");
		const char* tmp = NULL;
		if(param != NULL)
		{
			tmp = cJSON_GetObjectValueString(param, "endpoint");
			if (tmp != NULL)
			{
				strncpy(cloudInfo.endpoint, tmp, 127);
			}

			tmp = cJSON_GetObjectValueString(param, "bucket");
			if (tmp != NULL)
			{
				strncpy(cloudInfo.bucket, tmp, 127);
			}

			tmp = cJSON_GetObjectValueString(param, "id");
			if (tmp != NULL)
			{
				strncpy(cloudInfo.user, tmp, 127);
			}

			tmp = cJSON_GetObjectValueString(param, "aesSecret");
			if (tmp != NULL)
			{
				char base64_encode[128] = {0};
				char aes_encrypt[128] = {0};
				char secret[128] = {0};

				printf("============ yun_aesSecret: %s =============\n", tmp);

				strncpy(base64_encode, tmp, 127);
				// Base64解码
				strncpy(aes_encrypt, utl_base64_decode(base64_encode), 127);
				// AES解密
				AES128_CBC_decrypt_buffer((unsigned char*)secret, (unsigned char*)aes_encrypt, strlen(aes_encrypt), (unsigned char*)AES_ENCRYPT_KEY, (unsigned char*)AES_ENCRYPT_IV);

				// AES加密需要16字节对齐，末尾会补'\r'，此处需去掉多余的'\r'
				char* pEnd = strchr(secret, '\r');
				if (pEnd != NULL)
				{
					*pEnd = '\0';
				}
				strncpy(cloudInfo.pwd, secret, 127);
			}
			else
			{
				tmp = cJSON_GetObjectValueString(param, "secret");
				if (tmp != NULL)
				{
					strncpy(cloudInfo.pwd, tmp, 127);
				}
			}

			ret = 0;
		}
	}
	cJSON_Delete(root);

	return ret;
}
#endif

//从负载均衡服务器获取上线服务器地址
static int _alarm_service_redirection()
{
	int ret = -1;
	char resp[BUFSIZE];

	memset(resp, 0, sizeof(resp));

	ret = _alarm_service_getserver("http://alarm-gw.jovision.com:80/netalarm-rs/balancing/get_servers", resp);

	if(ret <= 0)
	{
		printf("_alarm_service_redirection, ret: %d\n", ret);
		return ret;
	}
	
	cJSON *root = cJSON_Parse(resp);
	if(root == NULL)
	{
		printf("_alarm_service_redirection, parse resp failed\n");
		return -1;
	}

	ret = cJSON_GetObjectValueInt(root, "rt");
	if(ret == 0)
	{
		cJSON *param = cJSON_GetObjectItem(root, "param");
		if(param != NULL)
		{
			const char *onlineServer = cJSON_GetObjectValueString(param, "online_server_host");
			if(onlineServer != NULL)
				snprintf(s_ServerConf.onlineServer, sizeof(s_ServerConf.onlineServer), "%s", onlineServer);
			s_ServerConf.onlinePort = cJSON_GetObjectValueInt(param, "online_server_port");

			const char *businessServer = cJSON_GetObjectValueString(param, "business_server_host");
			if(businessServer != NULL)
				snprintf(s_ServerConf.businessServer, sizeof(s_ServerConf.businessServer), "%s", businessServer);
			s_ServerConf.businessPort = cJSON_GetObjectValueInt(param, "business_server_port");

			const char *recordServer = cJSON_GetObjectValueString(param, "record_server_host");
			if(recordServer != NULL)
				snprintf(s_ServerConf.recordServer, sizeof(s_ServerConf.recordServer), "%s", recordServer);
			s_ServerConf.recordPort = cJSON_GetObjectValueInt(param, "record_server_port");
		}
	}

	cJSON_Delete(root);
	
	return ret;
}


//{"sentcnt":1,"method":"login","error":{"errorcode":0},"rt":{"tm":"20150914143410"}}
static int __alarm_service_process_login_resp(char *resp)
{
	cJSON *root = cJSON_Parse(resp);
	if(root == NULL)
	{
		printf("__alarm_service_process_login_resp, invalid format!\n");
		return -1;
	}
	
	cJSON *error = cJSON_GetObjectItem(root, "error");
	if(error == NULL)
	{
		goto LoginFailed;
	}

	int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
	if (errorcode != 0)
	{
		goto LoginFailed;
	}

	cJSON *rt = cJSON_GetObjectItem(root, "rt");
	if(rt == NULL)
	{
		goto LoginFailed;
	}
	char tmnow[32];
	const char *timestr = cJSON_GetObjectValueString(rt, "tm");
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	sscanf(timestr, "%04d%02d%02d%02d%02d%02d", &year, &month, &day, &hour, &min, &sec);
	sprintf(tmnow, "%d-%d-%d-%d:%d:%d", year, month, day, hour, min, sec);
	ipcinfo_set_time(tmnow);
	mlog_write("迅卫士平台上线成功!");
	s_ServiceInfo.bLogined = TRUE;

	cJSON_Delete(root);

	printf("__alarm_service_process_login_resp, login OK!!!\n");

	__alarm_service_ipcinfo_report();
	return 0;

LoginFailed:
	cJSON_Delete(root);
	printf("__alarm_service_process_login_resp, login failed!\n");

	return -1;
}

#if SUPPORT_ALARM_SERVICE_TALK
int __alarm_service_voice_setaudio(BOOL bSpeaking)
{
	if(bSpeaking)
	{
		jv_audio_attr_t ai_attr;
		
		if (speakerowerStatus < JV_SPEAKER_OWER_CHAT)
		{
			//如果没有开对讲，则硬件上打开对讲
			jv_ao_mute(0);
			jv_ai_get_attr(0, &ai_attr);		
			
			speakerowerStatus = JV_SPEAKER_OWER_CHAT;
			
			malarm_sound_stop();
			while(!malarm_get_speakerFlag())
			{
				usleep(100*1000);
			}
		
			jv_ai_setChatStatus(TRUE);
			//jv_ai_stop(0);
			//修改一下逻辑，加个判断条件，因为jv_audio的结构中增加了静音和音量的设置
			//先判断结构中的值，如果不合法，再按照下面的默认处理
			if(ai_attr.level != -1)
			{
				jv_ao_ctrl(ai_attr.level);
			}
		}
		if(hwinfo.bHomeIPC)
			maudio_readfiletoao(VOICE_CHATSTARTTIP);
	}
	else
	{
		
		//jv_audio_attr_t ai_attr;
		//jv_ai_get_attr(0, &ai_attr);		
		//jv_ai_start(0, &ai_attr);
		jv_ai_setChatStatus(FALSE); 
		
		if(speakerowerStatus == JV_SPEAKER_OWER_CHAT)	
			speakerowerStatus = JV_SPEAKER_OWER_NONE;
		if(hwinfo.bHomeIPC)
			maudio_readfiletoao(VOICE_CHATCLOSETIP);
		usleep(1000*1000);
		//if(!strcmp(hwinfo.devName,"VH2011"))
		//	jv_ao_ctrl(0x02);
		//else
			jv_ao_ctrl(0x09);
		jv_ao_mute(1);
		
	}
	return 0;
}

static int __alarm_service_parse_vcbuild(char *buffer)
{
	char resp[BUFSIZE];
	cJSON *root = cJSON_Parse(buffer);
	if(root != NULL)
	{
		int sentcnt = cJSON_GetObjectValueInt(root, "sentcnt");
		cJSON *param = cJSON_GetObjectItem(root, "param");
		if(param != NULL)
		{
			char voiceserver_ip[32];
			unsigned short voiceserver_port = 0;
			const char *client_id = cJSON_GetObjectValueString(param, "client_id");
			if(client_id != NULL)
			{
				printf("client_id = %s\n", client_id);
				if(strcmp(voiceClientID, client_id) != 0 && bVoiceConnecting == TRUE)
				{
					printf("another client is connecting\n");
					return -1;
				}
				else
					strncpy(voiceClientID, client_id, sizeof(voiceClientID)-1);				
			}
			else
				return -1;
			const char *voiceserverIP = cJSON_GetObjectValueString(param, "voiceserver_ip");
			if(voiceserverIP != NULL)
				strncpy(voiceserver_ip, voiceserverIP, 19);
			else
				return -1;
			voiceserver_port = cJSON_GetObjectValueInt(param, "voiceserver_port");
			// 关闭音频输入，开启音频输出
			//__alarm_service_voice_setaudio(TRUE);
			ipcinfo_t ipcinfo;
			ipcinfo_get_param(&ipcinfo);
			char ystID[16];
			sprintf(ystID, "%c%d",ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
			int ret;
			bVoiceConnecting = TRUE;
			//printf("%s ------------- %d voiceserver_ip:%s voiceserver_port:%d  ystID:%s\n", __FUNCTION__, __LINE__, voiceserver_ip, voiceserver_port,  ystID);	
			ret = VoiceClient_BuildVoiceChannel(voiceserver_ip, voiceserver_port,  ystID);    
			if (ret != 0)    
			{       
				printf("connect voice server error, ret is %d\n", ret);  
				mlog_write("语音对讲开启失败!");
				return -1;
			}
			printf("VoiceClient_BuildVoiceChannel ok\n");
			sprintf(resp, "{\"sentcnt\":%d,\"method\":\"on_push_vc_build\",\"error\":{\"errorcode\":0}}", sentcnt);
			__alarm_service_send(s_ServiceInfo.grpc, resp, strlen(resp));
			mlog_write("开始语音对讲!");
		}
	}
	return 0;
}
#endif

static void __alarm_service_update_server()
{
	ALARMSET info;
	int SleepTime = 10;

	malarm_get_param(&info);

	if (!info.bManualOnlineServer
		|| !info.bManualBusinessServer
		|| !info.bManualRecordServer)
	{
		// 只要有一个不是手动配置，就需要从负载均衡服务器获取服务器地址
		while(_alarm_service_redirection() != 0)
		{
			printf("__alarm_service_update_server failed\n");
			mlog_write("获取迅卫士上线服务器地址失败!");
			malarm_get_param(&info);
			// if(info.bManualOnlineServer && info.bManualBusinessServer && info.bManualRecordServer)
			if(info.bManualOnlineServer && info.bManualBusinessServer)
			{
				break;
			}
			sleep(SleepTime);
			if(SleepTime < 120)
				SleepTime += 10;
			//__alarm_service_check_reboot(1);
		}
	}

	if(info.bManualOnlineServer || info.bManualBusinessServer || info.bManualRecordServer)
	{
		s_ServerConf.bManualConfig = TRUE;
	}
	else
	{
		s_ServerConf.bManualConfig = FALSE;
	}

	if(info.bManualOnlineServer && strlen(info.onlineServer) != 0 && info.onlinePort != 0)
	{
		snprintf(s_ServerConf.onlineServer, sizeof(s_ServerConf.onlineServer), "%s", info.onlineServer);
		s_ServerConf.onlinePort = info.onlinePort;
	}
	if(info.bManualBusinessServer && strlen(info.businessServer) != 0 && info.businessPort != 0)
	{
		snprintf(s_ServerConf.businessServer, sizeof(s_ServerConf.businessServer), "%s", info.businessServer);
		s_ServerConf.businessPort = info.businessPort;
	}
	if(info.bManualRecordServer && strlen(info.recordServer) != 0 && info.recordPort != 0)
	{
		snprintf(s_ServerConf.recordServer, sizeof(s_ServerConf.recordServer), "%s", info.recordServer);
		s_ServerConf.recordPort = info.recordPort;
	}
}

static void __alarm_service_disconnect()
{
	UserDefInfo* pUserInfo = (UserDefInfo*)s_ServiceInfo.grpc->userdef;

	pthread_mutex_lock(&s_ServiceInfo.Mutex);
	close(pUserInfo->fd);
	pUserInfo->fd = INVALID_SOCKET;
	s_ServiceInfo.bConnected = FALSE;
	s_ServiceInfo.bLogined = FALSE;
	pthread_mutex_unlock(&s_ServiceInfo.Mutex);
}

// 自动布撤防处理线程
static void _alarm_service_protection_check(void *param)
{
	BOOL bDeployed = FALSE;

	pthreadinfo_add((char *)__func__);

	while(1)
	{
		while(s_ServiceInfo.bRunning)
		{
			bDeployed = malarm_check_deploy_state(time(NULL));
			if (bDeployed != s_ServiceInfo.bDeployed)
			{
				printf("Deploy status changed to : %d\n", bDeployed);
				if(bDeployed)
				{
					mlog_write("设备布防");
				}
				else
				{
					malarm_sound_stop();
					mlog_write("设备撤防");			
				}

				alarm_service_protection_notice(bDeployed);
			}
			s_ServiceInfo.bRefreshDeploy = FALSE;
			utl_WaitTimeout(((!s_ServiceInfo.bRunning) || (s_ServiceInfo.bRefreshDeploy)), 10 * 1000);
		}
		utl_WaitTimeout(s_ServiceInfo.bRunning, 15 * 1000);
	}
}

//心跳线程
static void _alarm_service_heartbeat(void *param)
{
	UserDefInfo *user = (UserDefInfo *)(s_ServiceInfo.grpc->userdef);
	int errCnt;

	pthreadinfo_add((char *)__func__);

	while(1)
	{
		while(s_ServiceInfo.bRunning)
		{
			errCnt = 0;
			while(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined)
			{
				s_bRevKeepAliveResp = FALSE;
				__alarm_service_keepalive();

				utl_WaitTimeout(!(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined), 10000);
				
				if(!s_bRevKeepAliveResp)
					errCnt++;
				else
					errCnt = 0;

				// printf("errCnt = %d\n", errCnt);
				if(errCnt >= 2)
				{
					printf("timeout close connection\n");
					pthread_mutex_lock(&s_ServiceInfo.Mutex);
					close(user->fd);
					user->fd = INVALID_SOCKET;	
					s_ServiceInfo.bConnected = FALSE;
					s_ServiceInfo.bLogined = FALSE;
					pthread_mutex_unlock(&s_ServiceInfo.Mutex);
					mlog_write("服务器响应超时，迅卫士连接断开!");
				}
			}
			utl_WaitTimeout((s_ServiceInfo.bConnected && s_ServiceInfo.bLogined), 15000);
		}
		utl_WaitTimeout(s_ServiceInfo.bRunning, 15000);
	}
}
/*
//服务器响应消息处理线程
static void _alarm_service_process(void *param)
{
	pthreadinfo_add((char *)__func__);
	char *msg = NULL;
	int ret = 0;
	
	while(1)
	{
		while(s_ServiceInfo.bRunning)
		{
			if (msg != NULL)
			{
				//free(msg);
				msg = NULL;
			}
			ret = utl_queue_recv(s_ServiceInfo.iMqHandle, &msg, -1);
			if (ret != 0)
			{
				printf("timeout happened when mq_receive data\n");
				usleep(1);
				continue;
			}
			if (msg == NULL)
				continue;
			if(strstr(msg, "login") != NULL)
			{
				ret = __alarm_service_parse_login_resp(msg);
				if(ret == 0)
				{
					printf("login ok\n");
					__alarm_service_ipcinfo_report();
				}
				else
				{
					printf("login failed\n");
				}
			}
			else if(strstr(msg, "keep_online") != NULL)
			{
				//printf("keep_online\n");
				time_keepalive_resp = time(NULL);
			}
			else if(strstr(msg, "push_vc_build") != NULL)
			{
				printf("push_vc_build\n");
				__alarm_service_parse_vcbuild(msg);
			}
			else
			{
				printf("msg:%s\n", msg);
				grpc_s_serve_direct(s_ServiceInfo.grpc, msg);
			}
		}
		sleep(15);
	}
}*/

/**
 * \brief 检查是否需要重启
 *
 * \param opt [in] 是否检查TRUE:检查是否需要重启, FALSE:复位计数器
 *
 * \return NULL
 *
 */
/*static void __alarm_service_check_reboot(BOOL opt)
{
	static int connCnt = 0;
	if(opt)
		connCnt++;
	else
	{
		connCnt = 0;
		utl_system("rm "REBOOTLOG);
		return;
	}
	if((connCnt%20) == 0)
	{
		int reboot_times = 0;
		char *val = utl_fcfg_get_value(REBOOTLOG, "reboot");
		if(val == NULL)
			reboot_times = 1;
		else
			reboot_times = atoi(val)+1;
		printf("connCnt=%d, reboot_times=%d\n",connCnt,reboot_times);
		if(connCnt == reboot_times*20 && reboot_times < 5)
		{
			char valstr[8];
			sprintf(valstr, "%d", reboot_times);
			utl_fcfg_set_value(REBOOTLOG, "reboot", valstr);
			utl_fcfg_flush(REBOOTLOG);
			mlog_write("迅卫士平台上线失败，设备重启!");
			SYSFuncs_reboot();
		}
	}

}*/

//迅卫士平台连接线程
static void _alarm_service_connection(void *param)
{
	UserDefInfo *user = (UserDefInfo *)(s_ServiceInfo.grpc->userdef);
	fd_set	rdfds;
	struct timeval tv;
	//SPAlarmSet_t alarmInfo;
	//BOOL alarmEnable = FALSE;
	
	char buffer[128*1024];
	int dataLen = 0;
	int ret = 0;
	char *msgEnd = NULL;
	pthreadinfo_add((char *)__func__);
	memset(buffer, 0, sizeof(buffer));
	memset(&tv, 0, sizeof(tv));

	// __alarm_service_update_server();

#ifdef PREREC_SUPPORT
#ifndef CLOUD_TEST
	sleepTime = 10;
	while(_alarm_service_getcloudinfo() != 0)
	{
		printf("_alarm_service_getcloudinfo failed\n");
		mlog_write("获取云存储服务器信息失败!");
		//__alarm_service_check_reboot(1);
		sleep(sleepTime);
		if(sleepTime < 120)
			sleepTime += 10;
	}
#else
	strncpy(cloudInfo.user, "01Uebp2ECqBrgAj1", 127);
	strncpy(cloudInfo.pwd, "tA2AOrHQZLUJLX3JPwqzVjhnNunfD3", 127);
	strncpy(cloudInfo.endpoint, "oss-cn-beijing.aliyuncs.com", 127);
	strncpy(cloudInfo.bucket, "xunweitest1", 127);
#endif
	mprerec_upload_init(cloudInfo.user, cloudInfo.pwd, cloudInfo.endpoint, cloudInfo.bucket, JVYUN_TYPE_ALI);
#endif

	while(1)
	{
		while(s_ServiceInfo.bRunning)
		{
			__alarm_service_update_server();
			pthread_mutex_lock(&s_ServiceInfo.Mutex);
			while(user->fd == INVALID_SOCKET)
			{
				int SleepTime = 10;
				//建立传输通道,TCP长连接
				if((user->fd = __alarm_service_connect()) < 0)
				{
					printf("time:%ld, create user->fd error: %s\n", time(NULL), strerror(errno));
					sleep(SleepTime);
					if(SleepTime < 120)
						SleepTime += 10;
					//__alarm_service_check_reboot(1);
					break;
				}
				s_ServiceInfo.bConnected = TRUE;
				
				//__alarm_service_check_reboot(0);
			}
			pthread_mutex_unlock(&s_ServiceInfo.Mutex);
			if(user->fd != INVALID_SOCKET)
			{
				__alarm_service_login();
			}
			while(user->fd != INVALID_SOCKET)
			{			
				FD_ZERO(&rdfds);
				FD_SET(user->fd, &rdfds);

				tv.tv_sec = 5;
				tv.tv_usec = 0;

				ret = select(user->fd + 1, &rdfds, NULL, NULL, &tv);
				if(ret < 0)
				{
					perror("select");
					dataLen = 0;
					memset(buffer, 0, sizeof(buffer));
					__alarm_service_disconnect();
					mlog_write("连接异常，迅卫士连接断开!");
					break;
				}
				else if(ret == 0)
				{
					dataLen = 0;
					memset(buffer, 0, sizeof(buffer));
					continue;
				}
				else
				{
					pthread_mutex_lock(&s_ServiceInfo.Mutex);
					if(user->fd != INVALID_SOCKET)
					{
						if(FD_ISSET(user->fd, &rdfds))
						{
							pthread_mutex_unlock(&s_ServiceInfo.Mutex);
							ret = __alarm_service_recv(s_ServiceInfo.grpc, buffer + dataLen, sizeof(buffer) - dataLen, NULL);
							if (ret > 0)
							{
								dataLen += ret;
								if(dataLen >= 128*1024)
								{
									printf("Error: Message too long!\n");
									dataLen = 0;
									memset(buffer, 0, sizeof(buffer));
								}
								else
								{
									while((msgEnd = strstr(buffer, ENDCODE)) != NULL)
									{
										*msgEnd = '\0';
										
										if(strstr(buffer, "login") != NULL)
										{
											ret = __alarm_service_process_login_resp(buffer);
											if(ret == 0)
											{
												printf("login ok\n");
											}
											else
											{
												printf("login failed\n");
											}
										}
										else if(strstr(buffer, "keep_online") != NULL)
										{
											s_bRevKeepAliveResp = TRUE;
										}
										else if(strstr(buffer, "push_vc_build") != NULL)
										{
											printf("push_vc_build\n");
											#if SUPPORT_ALARM_SERVICE_TALK
											__alarm_service_parse_vcbuild(buffer);
											#endif
										}
										else
										{
											// printf("msg:%s\n", buffer);
											grpc_s_serve_direct(s_ServiceInfo.grpc, buffer);
										}
										
										memmove(buffer, msgEnd+2, dataLen-(msgEnd-buffer)-2);
										//printf("memmove buffer=%s\n", buffer);
										dataLen = dataLen-(msgEnd-buffer)-2;
									}
								}
							}
							else
							{
								perror("recv");
								dataLen = 0;
								memset(buffer, 0, sizeof(buffer));
								pthread_mutex_lock(&s_ServiceInfo.Mutex);
								close(user->fd);
								user->fd = INVALID_SOCKET;
								s_ServiceInfo.bConnected = FALSE;
								s_ServiceInfo.bLogined = FALSE;
								pthread_mutex_unlock(&s_ServiceInfo.Mutex);
								mlog_write("接收消息异常，迅卫士连接断开!");
							}
						}
						else
						{
							pthread_mutex_unlock(&s_ServiceInfo.Mutex);
						}
					}
					else
					{
						pthread_mutex_unlock(&s_ServiceInfo.Mutex);						
					}
				}
			}
			printf("time:%ld, reconnection\n", time(NULL));
			sleep(10);
		}
		sleep(15);
	}
}

/*初始化，创建连接线程与服务器建立连接*/
int alarm_service_init()
{
	if(bAlarmServiceInited)
	{
		s_ServiceInfo.bRunning = TRUE;
		return 0;
	}

	bAlarmServiceInited = TRUE;

	printf("time=%ld, alarm_service_init\n", time(NULL));

	ipcinfo_t* p_IpcInfo = ipcinfo_get_param(NULL);
	jv_ystNum_parse(s_ServiceInfo.DevId, p_IpcInfo->nDeviceInfo[6], p_IpcInfo->ystID);

	Rtmp_Init();

	s_ServiceInfo.bRunning = TRUE;
	s_ServiceInfo.bConnected = FALSE;
	s_ServiceInfo.bLogined = FALSE;
	s_ServiceInfo.grpc = grpc_new();

	grpcInitParam_t initParam;
	UserDefInfo* pUserInfo = malloc(sizeof(UserDefInfo));

    pUserInfo->fd = INVALID_SOCKET;
    memset(&(pUserInfo->server), 0, sizeof(struct sockaddr_in));
	initParam.userdef = pUserInfo;
	initParam.bEnableTimeout = FALSE;
	initParam.fptr_net.recv = __alarm_service_recv;
	initParam.fptr_net.send = __alarm_service_send;
	initParam.methodList_s = ipc_methodList_s;
	//initParam.methodList_c = ipc_methodList_c;

	grpc_init(s_ServiceInfo.grpc, &initParam);

	pthread_create(&s_ServiceInfo.heartbeat_thread, NULL, (void *)_alarm_service_heartbeat, NULL);
	pthread_create(&s_ServiceInfo.connection_thread, NULL, (void *)_alarm_service_connection, NULL);
	pthread_create(&s_ServiceInfo.protection_check_thread, NULL, (void *)_alarm_service_protection_check, NULL);
	pthread_mutex_init(&s_ServiceInfo.Mutex, NULL);

	#if SUPPORT_ALARM_SERVICE_TALK
	// 初始化语音对讲库
	VoiceClient_Init();
	VoiceClient_RegisterEventCallback(_alarm_service_voice_event_callback, _alarm_service_voice_data_callback);
	#endif

	printf("time=%ld, alarm_service_init ok\n", time(NULL));

	return 0;
}

int alarm_service_deinit()
{
	printf("time=%ld, alarm_service_deinit\n", time(NULL));

	pthread_mutex_lock(&s_ServiceInfo.Mutex);
	UserDefInfo *pUserInfo = (UserDefInfo *)(s_ServiceInfo.grpc->userdef);
	close(pUserInfo->fd);
	pUserInfo->fd = INVALID_SOCKET;
	s_ServiceInfo.bConnected = FALSE;
	s_ServiceInfo.bLogined = FALSE;
	pthread_mutex_unlock(&s_ServiceInfo.Mutex);

	s_ServiceInfo.bRunning = FALSE;

	// 这里不需要释放线程、内存等资源，下次启用时，也不会再创建或申请

	mlog_write("迅卫士平台下线成功!");
	printf("time=%ld, alarm_service_deinit ok\n", time(NULL));

	return 0;
}

//手动配置迅卫士服务器地址接口
int alarm_service_manual_config(int bManualOnlineServer, int bManualBusinessServer, int bManualRecordServer, 
	const char *onlineServer, const int onlinePort,
	const char *businessServer, const int businessPort,
	const char *recordServer, const int recordPort)
{
	if(s_ServerConf.bManualConfig == FALSE 
		&& bManualOnlineServer == FALSE 
		&& bManualBusinessServer == FALSE
		&& bManualRecordServer == FALSE)
	{
		printf("server auto config\n");
		return 0;
	}
	pthread_mutex_lock(&s_ServiceInfo.Mutex);
	printf("alarm_service_manual_config: %s:%d, %s:%d, %s:%d\n",onlineServer,onlinePort,businessServer,businessPort,recordServer,recordPort);
	mlog_write("迅卫士重新配置上线服务器地址!");
	if(bManualOnlineServer == FALSE || bManualBusinessServer == FALSE || bManualRecordServer == FALSE)
		_alarm_service_redirection();
	if(bManualOnlineServer == TRUE && onlineServer != NULL && onlinePort != 0)
	{
		strncpy(s_ServerConf.onlineServer, onlineServer, 127);
		s_ServerConf.onlinePort = onlinePort;
	}
	if(bManualBusinessServer == TRUE && businessServer != NULL && businessPort != 0)
	{
		strncpy(s_ServerConf.businessServer, businessServer, 127);
		s_ServerConf.businessPort = businessPort;
	}
	if(bManualRecordServer == TRUE && recordServer != NULL && recordPort != 0)
	{
		strncpy(s_ServerConf.recordServer, recordServer, 127);
		s_ServerConf.recordPort = recordPort;
	}
	if(s_ServiceInfo.grpc && s_ServiceInfo.grpc->userdef)
	{
		UserDefInfo *user = (UserDefInfo *)(s_ServiceInfo.grpc->userdef);
		if(user->fd > 0)
		{
			close(user->fd);
			user->fd = INVALID_SOCKET;
		}
	}
	s_ServiceInfo.bConnected = FALSE;
	s_ServiceInfo.bLogined = FALSE;
	pthread_mutex_unlock(&s_ServiceInfo.Mutex);
	return 0;
}

void alarm_service_refresh_deployment()
{
	s_ServiceInfo.bRefreshDeploy = TRUE;
}

/*发送布撤防消息*/
int alarm_service_protection_notice(BOOL protectionStatus)
{
	int ret = -1;
	int len = 0;
	
	printf("alarm_service_protection_notice: %d\n", protectionStatus);
	s_ServiceInfo.bDeployed = protectionStatus;

	if (!s_ServiceInfo.bRunning)
	{
		return -1;
	}
	if(!s_ServiceInfo.bConnected || !s_ServiceInfo.bLogined)
	{
		return -1;
	}

	char url[1024] = {0};//"http://192.168.10.10:8081/netalarm-rs/alarm/alarm_report";
	char req[BUFSIZE];
	char resp[BUFSIZE];
	sprintf(url, "http://%s:%d/netalarm-rs/protect_state/device_protect_report", s_ServerConf.businessServer, s_ServerConf.businessPort);
	s_ServiceInfo.grpc->sentcnt++;
	memset(req, 0, sizeof(req));
	memset(resp, 0, sizeof(resp));
	ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	char ystID[16];
	sprintf(ystID, "%c%d",ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
	sprintf(req, "{\"sentcnt\" : %d,"
					"\"method\" : \"device_protect_report\","
					"\"param\" : {"
					"\"dev_id\" : \"%s\","
					"\"state\" : %d"
					"}}", s_ServiceInfo.grpc->sentcnt,
					ystID, protectionStatus);
					
	len = strlen(req);
	printf("alarm_service_protection_notice, req=\n%s\n", req);
	ret = Http_post_message(url, req, len, resp, 10);
	if(ret > 0)
	{
		printf("alarm_service_protection_notice, resp=\n%s\n", resp);
		cJSON *root = cJSON_Parse(resp);
		if(root != NULL)
		{
			cJSON *error = cJSON_GetObjectItem(root, "error");
			if(error != NULL)
			{
				int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
				if(errorcode == 0)
				{
					printf("alarm_service_protection_notice ok\n");
				}
				else
					printf("cJSON_GetObjectValueInt err:%d\n", errorcode);
			}
			else
				printf("cJSON_GetObjectItem err\n");
			cJSON_Delete(root);
		}
		else
			printf("cJSON_Parse err\n");
	}

	return ret;
}

void __alarm_service_get_alarm_info(AlarmReportInfo_t* pInfo, int channelid, int Type)
{
	char* pType[ALARM_TOTAL][2] = 
	{
		[ALARM_GPIN]			= {"io",		"pir"},
		[ALARM_VIDEOLOSE]		= {"video",		"videoLost"},
		[ALARM_HIDEDETECT]		= {"analyse",	"videoBlock"},
		[ALARM_MOTIONDETECT]	= {"video",		"motionDetect"},
		[ALARM_DOOR]			= {"io",		"doorAlarm"},
		[ALARM_IVP]				= {"analyse",	"invasion"},
		[ALARM_PIR]				= {"io",		"pir"},
	};

	strcpy(pInfo->type, pType[Type][0] == NULL ? "Unknown" : pType[Type][0]);
	strcpy(pInfo->subtype, pType[Type][1] == NULL ? "Unknown" : pType[Type][1]);
	pInfo->pir_code = 0;
	pInfo->detector_id = 0;
	strcpy(pInfo->dev_id, s_ServiceInfo.DevId);
	strcpy(pInfo->dev_type, "ipc");
	pInfo->channel = channelid;
	pInfo->cloud_record_info[0] = '\0';
}



// 鹏博士报警推送
int alarm_service_push_alarm_pbs(int channelid, int Type)
{
	char url[1024] = {0};//"http://push.idrpeng.com:8080/cgi-bin/httpmsg/SendMsgAction?function=SendPushMsg&devId=xxx&type=xxx&alertTime=xxx&upName=xxx&alertType=xxx";
	char DevID[32] = {0};
	int DevType = 49153;
	char AlarmTime[32] = {0};
	const char* pVideoName = mrecord_get_now_recfile();
	int AlarmType = Type;
	int nLen = 0;
	int ret = 0;
	char resp[1024] = {0};
	time_t Now = time(NULL);
	struct tm Nowtm;

/*
	// 鹏博士报警定义
#define ALARM_TYPE_MOTION		"motiondetect"		// 视频丢失
#define ALARM_TYPE_VIDEOLOSS	"videoloss"			// 视频遮挡
#define ALARM_TYPE_SHILTER		"videoshilter"		// 移动侦测
#define ALARM_TYPE_INVISION		"regioninvision"	// 区域入侵
#define ALARM_TYPE_SOUND_DB		"soundrise"			// 分贝检测
*/
	char*	AlarmName[ALARM_TOTAL] = 
	{
		[ALARM_VIDEOLOSE]		= "videoloss",			// 视频丢失
		[ALARM_HIDEDETECT]		= "videoshilter",		// 视频遮挡
		[ALARM_MOTIONDETECT]	= "motiondetect",		// 移动侦测
	};

	if (Type >= ALARM_TOTAL)
	{
		printf("%s, Invalid alarm type: %d\n", __func__, Type);
		return -1;
	}

	localtime_r(&Now, &Nowtm);

	snprintf(DevID, sizeof(DevID), "%s", s_ServiceInfo.DevId);
	snprintf(AlarmTime, sizeof(AlarmTime), "%.4d-%.2d-%.2d%%20%.2d:%.2d:%.2d", 
								Nowtm.tm_year + 1900, Nowtm.tm_mon + 1, Nowtm.tm_mday, 
								Nowtm.tm_hour, Nowtm.tm_min, Nowtm.tm_sec);

	nLen += snprintf(url + nLen, sizeof(url) - nLen, 
							"http://%s:%d/cgi-bin/httpmsg/SendMsgAction?function=SendPushMsg", 
							// "push.idrpeng.com", 8080			// 正式环境
							// "test.idrpeng.com", 8088			// 测试环境
							"push2.idrpeng.com", 8088			// 正式环境2
							);
	nLen += snprintf(url + nLen, sizeof(url) - nLen, "&devId=%s", DevID);
	nLen += snprintf(url + nLen, sizeof(url) - nLen, "&type=%d", DevType);
	nLen += snprintf(url + nLen, sizeof(url) - nLen, "&alertTime=%s", AlarmTime);
	nLen += snprintf(url + nLen, sizeof(url) - nLen, "&upName=%s", pVideoName);
	nLen += snprintf(url + nLen, sizeof(url) - nLen, "&alertType=%s", AlarmName[Type] ? AlarmName[Type] : "Unknown");

	printf("\n%s, url=\n%s\n", __func__, url);

	int retryCnt = 0;
	ret = -1;
	while(ret <= 0)
	{
		//尝试发送两次，如果都失败停止发送
		retryCnt++;
		if(retryCnt > 2)
		{
			break;
		}
		memset(resp, 0, sizeof(resp));
		ret = Http_post_message(url, "", 0, resp, 10);
		printf("%s, ret: %d, retry_cnt: %d, resp=\n%s\n", __func__, ret, retryCnt, resp);
		if(ret > 0)
		{
			cJSON *root = cJSON_Parse(resp);
			
			ret = -1;
			if(root != NULL)
			{
				const char* errorcode = cJSON_GetObjectValueString(root, "errcode");
				if(errorcode != NULL)
				{
					if (atoi(errorcode) == 0)
					{
						ret = 1;		// 成功
					}
					else
					{
						printf("cJSON_GetObjectValueString errcode: %s\n", errorcode);
					}
				}
				cJSON_Delete(root);
			}
			else
			{
				printf("cJSON_Parse err\n");
			}
		}
		sleep(1);
	}

	return ret;
}

/*发送报警消息*/
int alarm_service_alarm_report(int channelid, int Type)
{
	int ret = -1;
	int len = 0;
	char url[1024] = {0};//"http://192.168.10.10:8081/netalarm-rs/alarm/alarm_report";
	char req[BUFSIZE];
	char resp[BUFSIZE];
	AlarmReportInfo_t Info;
	
	printf("alarm_service_report\n");

	// 推送鹏博士报警
	if (PRODUCT_MATCH(PRODUCT_PBS))
	{
		alarm_service_push_alarm_pbs(channelid, Type);
	}

	if (!s_ServiceInfo.bRunning)
	{
		return -1;
	}
	if (!s_ServiceInfo.bConnected || !s_ServiceInfo.bLogined)
	{
		return -1;
	}
	if (!s_ServiceInfo.bDeployed)
	{
		printf("alarm_service_report, do not send alarm if not deployed: %d!\n", s_ServiceInfo.bDeployed);
		return -2;
	}

	__alarm_service_get_alarm_info(&Info, channelid, Type);

	sprintf(url, "http://%s:%d/netalarm-rs/alarm/alarm_report", s_ServerConf.businessServer, s_ServerConf.businessPort);
	
	s_ServiceInfo.grpc->sentcnt++;
	memset(req, 0, sizeof(req));
	sprintf(req, "{\"sentcnt\" : %d,"
					"\"method\" : \"alarm_report\","
					"\"param\" : {"
					"\"dev_id\" : \"%s\","
					"\"dev_type\" : \"%s\","
					"\"channel\" : %d,"
					"\"type\" : \"%s\","
					"\"subtype\" : \"%s\","
					"\"detector_id\" : \"%d\","
					"\"pir_code\" : \"%d\","
					"\"cloud_record_info\" : \"%s\""
					"}}", s_ServiceInfo.grpc->sentcnt,
					Info.dev_id, Info.dev_type, 
					Info.channel, Info.type, 
					Info.subtype, Info.detector_id, 
					Info.pir_code, Info.cloud_record_info);
						
	len = strlen(req);
	printf("alarm_service_report, req=\n%s\n", req);
	int retryCnt = 0;
	ret = -1;
	while(ret <= 0)
	{
		//尝试发送两次，如果都失败停止发送
		retryCnt++;
		if(retryCnt > 2)
		{
			break;
		}
		memset(resp, 0, sizeof(resp));
		ret = Http_post_message(url, req, len, resp, 10);
		if(ret > 0)
		{
			printf("alarm_service_report, cnt: %d, resp=\n%s\n", retryCnt, resp);
			cJSON *root = cJSON_Parse(resp);
			if(root != NULL)
			{
				cJSON *error = cJSON_GetObjectItem(root, "error");
				if(error != NULL)
				{
					int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
					if(errorcode == 0)
					{
					}
					else
						printf("cJSON_GetObjectValueInt err:%d\n", errorcode);
				}
				else
					printf("cJSON_GetObjectItem err\n");
				cJSON_Delete(root);
			}
			else
				printf("cJSON_Parse err\n");
		}
	}
	
	return ret;
}

//TF 卡异常忽略
int alarm_service_tfcard_notice_ignore()
{
	printf("alarm_service_tfcard_notice_ignore\n");
	bTFCardErrIgnored = TRUE;
	return 0;
}

/*发送存储异常消息*/
int alarm_service_tfcard_notice(CardState_e eState, const char *desc)
{
	if(bTFCardErrIgnored)
	{
		printf("TF card error ignored\n");
		return 0;
	}
	int ret = -1;
	int len = 0;
	//http://alarm-gw.jovision.com/netalarm-rs/device_data/info_report
	//printf("time=%ld, alarm_service_tfcard_notice\n", time(NULL));
	if(s_ServiceInfo.bRunning)
	{
		if(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined)
		{
			char url[1024] = {0};//"http://192.168.10.10:8081/netalarm-rs/alarm/alarm_report";
			char req[BUFSIZE];
			char resp[BUFSIZE];
			char ystID[16] = {0};
			sprintf(url, "http://%s:%d/netalarm-rs/dev/dev_tfcard_notice", s_ServerConf.businessServer, s_ServerConf.businessPort);
			s_ServiceInfo.grpc->sentcnt++;
			memset(req, 0, sizeof(req));
			memset(resp, 0, sizeof(resp));
			ipcinfo_t ipcinfo;
			ipcinfo_get_param(&ipcinfo);
			char timeStr[32] = {0};
			struct tm NowTm;
			time_t ttNow = time(NULL);
			localtime_r(&ttNow, &NowTm);
			sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", NowTm.tm_year+1900, NowTm.tm_mon+1, 
			NowTm.tm_mday, NowTm.tm_hour, NowTm.tm_min, NowTm.tm_sec);
			//printf("timeStr=%s\n", timeStr);
			
			sprintf(ystID, "%c%d",ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
			sprintf(req, "{\"mid\":%d,"
							"\"method\":\"dev_tfcard_notice\","
							"\"param\":{"
							"\"state\": \"%d\","
							"\"devGuid\": \"%s\","
							"\"time\": \"%s\","
							"\"description\": \"%s\""
							"}}", 
							s_ServiceInfo.grpc->sentcnt, eState, ystID, timeStr, desc);
							
			len = strlen(req);
			ret = Http_post_message(url, req, len, resp, 10);
			if(ret > 0)
			{
				//printf("alarm_service_tfcard_notice, resp=%s\n", resp);
				cJSON *root = cJSON_Parse(resp);
				if(root != NULL)
				{
					cJSON *error = cJSON_GetObjectItem(root, "error");
					if(error != NULL)
					{
						int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
						if(errorcode == 0)
						{
							//printf("alarm_service_tfcard_notice ok\n");
						}
						else
							printf("alarm_service_tfcard_notice err:%d\n", errorcode);
					}
					else
						printf("cJSON_GetObjectItem err\n");
					cJSON_Delete(root);
				}
				else
					printf("cJSON_Parse err\n");
			}
		}
	}
	return ret;
}

/*发送客流量统计消息*/
int alarm_service_ivp_count_notice(const char *start_time, const char *end_time, int in_count, int out_count)
{
	int ret = -1;
	int len = 0;
	//http://alarm-gw.jovision.com/netalarm-rs/device_data/info_report
	printf("alarm_service_ivp_count_notice\n");
	if(s_ServiceInfo.bRunning)
	{
		if(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined)
		{
			char url[1024] = {0};//"http://192.168.10.10:8081/netalarm-rs/alarm/alarm_report";
			char req[BUFSIZE];
			char resp[BUFSIZE];
			sprintf(url, "http://%s:%d/netalarm-rs/head_counting/count_by_time", s_ServerConf.businessServer, s_ServerConf.businessPort);
			s_ServiceInfo.grpc->sentcnt++;
			memset(req, 0, sizeof(req));
			memset(resp, 0, sizeof(resp));
			ipcinfo_t ipcinfo;
			ipcinfo_get_param(&ipcinfo);
			char ystID[16];
			sprintf(ystID, "%c%d",ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
			sprintf(req, "{\"sentcnt\" : %d,"
							"\"method\" : \"head_counting\","
							"\"param\" : {"
							"\"dev_id\" : \"%s\","
							"\"channel\" : 0,"
							"\"start_time\" : \"%s\","
							"\"end_time\" : \"%s\","
							"\"in_count\" : %d,"
							"\"out_count\" : %d"
							"}}", s_ServiceInfo.grpc->sentcnt,
							ystID, start_time, end_time, in_count, out_count);
							
			len = strlen(req);
			printf("req:\n%s\n", req);
			ret = Http_post_message(url, req, len, resp, 10);
			if(ret > 0)
			{
				printf("alarm_service_ivp_count_notice, resp=%s\n", resp);
				cJSON *root = cJSON_Parse(resp);
				if(root != NULL)
				{
					cJSON *error = cJSON_GetObjectItem(root, "error");
					if(error != NULL)
					{
						int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
						if(errorcode == 0)
						{
							printf("alarm_service_ivp_count_notice ok\n");
						}
						else
							printf("cJSON_GetObjectValueInt err:%d\n", errorcode);
					}
					else
						printf("cJSON_GetObjectItem err\n");
					cJSON_Delete(root);
				}
				else
					printf("cJSON_Parse err\n");
			}
		}
	}
	return ret;
}

/*发送热度图消息*/
#if 0
int alarm_service_ivp_heatmap_notice(const char *start_time, const char *end_time, MMLImage* img)
{
	int ret = -1;
	int len = 0;
	if(img == NULL)
		return -1;
	//http://alarm-gw.jovision.com/netalarm-rs/device_data/info_report
	printf("alarm_service_ivp_heatmap_notice\n");
	if(s_ServiceInfo.bRunning)
	{
		if(s_ServiceInfo.bConnected && s_ServiceInfo.bLogined)
		{
			char url[1024] = {0};//"http://192.168.10.10:8081/netalarm-rs/alarm/alarm_report";
			char req[BUFSIZE];
			char resp[BUFSIZE];
			sprintf(url, "http://%s:%d/netalarm-rs/heatmap/heatmap", s_ServerConf.businessServer, s_ServerConf.businessPort);
			s_ServiceInfo.grpc->sentcnt++;
			memset(req, 0, sizeof(req));
			memset(resp, 0, sizeof(resp));
			ipcinfo_t ipcinfo;
			ipcinfo_get_param(&ipcinfo);
			char ystID[16];
			sprintf(ystID, "%c%d",ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
			char imgBuf[BUFSIZE-1024];
			memset(imgBuf, 0, sizeof(imgBuf));
			int imgLen = 0;
			utl_base64_encode_m(img->image_data, img->image_size, imgBuf);
			sprintf(req, "{\"sentcnt\" : %d,"
							"\"method\" : \"heatmap_upload\","
							"\"param\" : {"
							"\"dev_id\" : \"%s\","
							"\"channel\" : 0,"
							"\"start_time\" : \"%s\","
							"\"end_time\" : \"%s\","
							"\"depth\" : %d,"
							"\"width\" : %d,"
							"\"height\" : %d,"
							"\"img_size\" : %d,"
							"\"img_data\" : \"%s\""
							"}}", s_ServiceInfo.grpc->sentcnt,
							ystID, start_time, end_time, img->depth, 
							img->width, img->height, strlen(imgBuf), imgBuf);
							
			len = strlen(req);
			//printf("req:\n%s\n", req);
			ret = Http_post_message(url, req, len, resp, 10);
			if(ret > 0)
			{
				printf("alarm_service_ivp_heatmap_notice, resp=%s\n", resp);
				cJSON *root = cJSON_Parse(resp);
				if(root != NULL)
				{
					cJSON *error = cJSON_GetObjectItem(root, "error");
					if(error != NULL)
					{
						int errorcode = cJSON_GetObjectValueInt(error, "errorcode");
						if(errorcode == 0)
						{
							printf("alarm_service_ivp_heatmap_notice ok\n");
						}
						else
							printf("cJSON_GetObjectValueInt err:%d\n", errorcode);
					}
					else
						printf("cJSON_GetObjectItem err\n");
					cJSON_Delete(root);
				}
				else
					printf("cJSON_Parse err\n");
			}
		}
	}
	return ret;
}
#endif

#if SUPPORT_ALARM_SERVICE_TALK
static void _alarm_service_voice_event_callback(int event_type, int ret)
{
	switch (event_type)
	{
	case EVENT_ON_BUILD_VOICE_CHANNEL:
		Printf("EVENT_ON_BUILD_VOICE_CHANNEL, ret %d\n", ret);
		if (ret == 0)
		{
			__alarm_service_voice_setaudio(TRUE);
		}
		else
		{
			printf("%s, build voice channel error, ret %d\n", __func__, ret);
		}
		break;
	case EVENT_ON_START_SEND_VOICE:
		Printf("EVENT_ON_START_SEND_VOICE, ret %d\n", ret);
		if (ret != 0)
		{
			printf("%s, recv start send error, ret %d\n", __func__, ret);
		}
		break;
	case EVENT_ON_END_SEND_VOICE:
		Printf("EVENT_ON_END_SEND_VOICE, ret %d\n", ret);
		//__alarm_service_voice_setaudio(FALSE);
		if (ret != 0)
		{
			printf("%s, recv end send error, ret %d\n", __func__, ret);
		}
		break;
	case EVENT_ON_CLOSE:
		Printf("EVENT_ON_CLOSE, ret %d\n", ret);
		__alarm_service_voice_setaudio(FALSE);
		bVoiceConnecting = FALSE;
		break;
	default:
		printf("%s, bad cmd\n", __func__);
		break;
	}
	
	return;
}

static void _alarm_service_voice_data_callback(const char *buffer, int length)
{  
	jv_audio_frame_t frame;
	memcpy(frame.aData, buffer, length);

	frame.u32Len = length;

	int ret = jv_ao_send_frame(0, &frame);
	if(ret < 0)
	{
		printf("jv_ao_send_frame failed\n");
	}

	return;
}
#endif

