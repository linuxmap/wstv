#include <jv_common.h>
#include <utl_cmd.h>
#include <utl_iconv.h>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <fcntl.h>
#include <cJSON.h>
#include <mosd.h>
#include "osd_server.h"
#include "sp_user.h"
#include "utl_filecfg.h"
#define OSD_SERVER_PORT 34567
#define BACKLOG 1
static jv_thread_group_t osdServerGroup;

static int sHandle=-1;
static int posX;
static int posY;
static int endX;
static int endY;
static int mainFontSize = 32;
static int subFontSize = 16;
#if 0
#define OSD_DEBUG printf
#else
#define OSD_DEBUG
#endif

/*
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

*/
static void process_login_msg(int sockfd,cJSON *json,int *timeout_interval)
{
	OSD_DEBUG("process_login_msg\n");
	
	GQMsgHeader_t header;
	char sendbuf[1024];
	header.head_flag=0xFF;
	header.version=1;
	header.reserv01=0x00;
	header.reserv02=0x00;
	header.sessionId=0x00000002;
	header.sequence_num=0;
	header.channel=1;
	header.end_flag=0x00;
	header.msg_id=0x03E9;
	
	cJSON *resp;
	char *data;
	int len=0;
	int ret=0;
	/*
{
"AliveInterval" : 20, 			// PU 与CU进行保活的时间周期，秒为单位
"ChannelNum" : 1, 			//通道数：1
"DeviceType " : "IPC", 		//设备类型：IPC
"ExtraChannel" : 0, 			//扩展通道数, 即组合编码通道 0
"Ret" : 100, 					//返回值，其他参数只有在返回成功时才有意义 登陆成功：100
"SessionID" : "0x00000002" 	//消息ID  与报文头一致
}


	 */
	resp=cJSON_CreateObject();	
	cJSON_AddStringToObject(resp,"DeviceType","IPC");
	cJSON_AddStringToObject(resp,"SessionID","0x00000002");
	cJSON_AddNumberToObject(resp,"AliveInterval",20);
	cJSON_AddNumberToObject(resp,"ChannelNum",1);
	cJSON_AddNumberToObject(resp,"ExtraChannel",0);
	cJSON_AddNumberToObject(resp,"Ret",100);
	*timeout_interval=20;
	data=cJSON_Print(resp);
	OSD_DEBUG("resp: %s\n", data);
	
	header.data_len=strlen(data)+1;
	memcpy(sendbuf,&header,sizeof(GQMsgHeader_t));
	len+=sizeof(GQMsgHeader_t);
	memcpy(sendbuf+sizeof(GQMsgHeader_t),data,header.data_len);
	len+=header.data_len;
	ret=send(sockfd,sendbuf,len,0);
	if(ret != len)
	{
		perror("send");
	}
	cJSON_Delete(resp);
	free(data);
	OSD_DEBUG("process_login_msg ok\n");
//int i=0;
//for(i=0;i<len;i++)
//{
//	if ((i%8) == 0)
//		printf("\n");
//	printf("0x%02x, ",sendbuf[i]);
//}
//printf("\n");

}
static void process_heartbeat_msg(int sockfd,cJSON *json)
{
	OSD_DEBUG("process_heartbeat_msg\n");
	GQMsgHeader_t header;
	char sendbuf[1024];
	header.head_flag=0xFF;
	header.version=1;
	header.reserv01=0x00;
	header.reserv02=0x00;
	header.sessionId=0x00000002;
	header.sequence_num=0;
	header.channel=1;
	header.end_flag=0x00;
	header.msg_id=0x03EF;
	
	cJSON *resp;
	char *data;
	int len=0;
	int ret=0;
	resp=cJSON_CreateObject();	
	cJSON_AddStringToObject(resp,"Name","KeepAlive");
	cJSON_AddStringToObject(resp,"SessionID","0x00000002");
	cJSON_AddNumberToObject(resp,"Ret",100);
	data=cJSON_Print(resp);
	OSD_DEBUG("heartbeat: %s\n", data);
	header.data_len=strlen(data);
	memcpy(sendbuf,&header,sizeof(GQMsgHeader_t));
	len+=sizeof(GQMsgHeader_t);
	memcpy(sendbuf+sizeof(GQMsgHeader_t),data,header.data_len);
	len+=header.data_len;
	ret=send(sockfd,sendbuf,len,0);
	if(ret<=0)
	{
		perror("send");
	}
	cJSON_Delete(resp);
	free(data);

}
static void send_response(int sockfd, struct  sockaddr_in * client, char *detail)
{
	
	OSD_DEBUG("send_response\n");
	char sendbuf[1024];
	
	cJSON *resp;
	char *data;
	int datalen=0;
	int ret=0;
	resp=cJSON_CreateObject();	
	cJSON_AddStringToObject(resp, "Name", "SetOSD");
	cJSON_AddStringToObject(resp, "Ret", detail);
	data=cJSON_Print(resp);
	datalen=strlen(data);
	OSD_DEBUG("%s\n", data);
	OSD_DEBUG("datalen: %d\n", datalen);
	memcpy(sendbuf, data, datalen);
	ret=sendto(sockfd, sendbuf, datalen, 0, (struct sockaddr*)client, sizeof(struct sockaddr_in));
	if(ret <= 0)
	{
		perror("send");
	}
	cJSON_Delete(resp);
	free(data);
}
int process_osd_msg(char *buf, char *detail, int bAccessCheck)
{
	static int pre_text_line_num;
	OSD_DEBUG("process_osd_msg\n");
	//got a whole message,parse it
	cJSON *root;
	OSD_DEBUG("%s\n",buf);
	root = cJSON_Parse(buf);
	if (NULL == root)
	{
		OSD_DEBUG("Error before: %s\n", cJSON_GetErrorPtr());
		if(detail != NULL)strcpy(detail, "Invalid JSON Message");
		cJSON_Delete(root);
		return -1;
	}
	
	mchnosd_section_attr sec_attr;
	memset(&sec_attr, 0, sizeof(sec_attr));
	
	if(bAccessCheck)
	{
		cJSON *LoginInfo=cJSON_GetObjectItem(root, "Login");
		if(LoginInfo == NULL)
		{
			OSD_DEBUG("no login info\n");
			if(detail != NULL)strcpy(detail, "No Login Info");
			cJSON_Delete(root);
			return -1;
		}
		
		cJSON *username=cJSON_GetObjectItem(LoginInfo, "UserName");
		if(username->type != cJSON_String)
		{
			OSD_DEBUG("username type incorrect\n");
			if(detail != NULL)strcpy(detail, "Invalid UserName Type");
			cJSON_Delete(root);
			return -1;
		}
		cJSON *password=cJSON_GetObjectItem(LoginInfo, "PassWord");
		if(password->type != cJSON_String)
		{
			OSD_DEBUG("password type incorrect\n");
			if(detail != NULL)strcpy(detail, "Invalid PassWord Type");
			cJSON_Delete(root);
			return -1;
		}
		int ret=sp_user_check_power(username->valuestring, password->valuestring);
		OSD_DEBUG("%s:%s,ret=%d\n",username->valuestring, password->valuestring, ret);
		if(ret < 0)
		{
			OSD_DEBUG("invalid username or password\n");
			if(detail != NULL)strcpy(detail, "Invalid UserName or PassWord");
			cJSON_Delete(root);
			return -1;
		}
	}
	cJSON *OSDInfo=cJSON_GetObjectItem(root, "OSDInfo");
	if (OSDInfo != NULL && OSDInfo->type == cJSON_Array)
	{
		int i;
		for (i=0;i<1;i++)
		{
			cJSON *infoN = cJSON_GetArrayItem(OSDInfo, i);
			if (infoN == NULL)
			{
				OSD_DEBUG("no infoN\n");
				break;
			}
			cJSON *Info = cJSON_GetObjectItem(infoN, "Info");
			if (Info != NULL && Info->type == cJSON_Array)
			{
				int j;
				for (j=0;j<TEXT_LINE_NUM;j++)
				{
					cJSON *text = cJSON_GetArrayItem(Info, j);
					if (text != NULL && text->valuestring)
					{
						strcpy(sec_attr.text[sec_attr.text_line_num++], text->valuestring);
						OSD_DEBUG("text.value: %s\n", text->valuestring);
						/*int jjj=0;
						for(jjj=0;jjj<strlen(text->valuestring);jjj++)
						{
							printf("0x%02x ",text->valuestring[jjj]);
						}
						printf("\n");*/
					}
					else
						OSD_DEBUG("no text\n");
				}
			}
			else
				OSD_DEBUG("Info not array\n");
			
			cJSON *OSDInfoWidget = cJSON_GetObjectItem(infoN, "OSDInfoWidget");
			if (OSDInfoWidget != NULL && OSDInfoWidget->type == cJSON_Object)
			{
				cJSON *fontInfo= cJSON_GetObjectItem(OSDInfoWidget, "FontSize");
				if(fontInfo != NULL && fontInfo->type == cJSON_Array)
				{
					cJSON *fontInfoMainStream = cJSON_GetArrayItem(fontInfo, 0);
					if (fontInfoMainStream != NULL)
					{
						if(sHandle >= 0 && fontInfoMainStream->valueint != mainFontSize)
						{
							mchnosd_region_destroy(sHandle);
							sHandle = -1;
						}
						mainFontSize = fontInfoMainStream->valueint;
					}

					cJSON *fontInfoSubStream = cJSON_GetArrayItem(fontInfo, 1);
					if (fontInfoSubStream != NULL)
					{
						if(sHandle >= 0 && fontInfoSubStream->valueint != subFontSize)
						{
							mchnosd_region_destroy(sHandle);
							sHandle = -1;
						}
						subFontSize = fontInfoSubStream->valueint;
					}
				}
				else if(fontInfo != NULL && fontInfo->type == cJSON_Number)
				{
					if(sHandle >= 0 && (fontInfo->valueint * 15 / 16 != mainFontSize 
						|| ((fontInfo->valueint * 8  + fontInfo->valueint / 2) / 16 ) != subFontSize))
					{
						mchnosd_region_destroy(sHandle);
						OSD_DEBUG("fontInfo changed mchnosd_region_destroy\n");
						sHandle = -1;
					}
					mainFontSize = fontInfo->valueint * 15 / 16;	
					subFontSize = (fontInfo->valueint * 8  + fontInfo->valueint / 2) / 16;
				}
				
				cJSON *posInfo= cJSON_GetObjectItem(OSDInfoWidget, "RelativePos");
				
				if (posInfo != NULL && posInfo->type == cJSON_Array)
				{
					cJSON *posItemX = cJSON_GetArrayItem(posInfo, 0);
					if (posItemX != NULL)
					{
						if(sHandle >= 0 && posItemX->valueint != posX)
						{
							mchnosd_region_destroy(sHandle);
							OSD_DEBUG("pos X changed mchnosd_region_destroy\n");
							sHandle = -1;
						}
						posX = posItemX->valueint;
					}
					else
						OSD_DEBUG("no posItem X\n");
					cJSON *posItemY = cJSON_GetArrayItem(posInfo, 1);
					if (posItemY != NULL)
					{
						if(sHandle >= 0 && posItemY->valueint != posY)
						{
							mchnosd_region_destroy(sHandle);
							OSD_DEBUG("pos Y changed mchnosd_region_destroy\n");
							sHandle = -1;
						}
						posY = posItemY->valueint;
					}
					else
						OSD_DEBUG("no posItem Y\n");
					cJSON *posIendX = cJSON_GetArrayItem(posInfo, 2);
					if (posIendX != NULL)
					{
						if(sHandle >= 0 && posIendX->valueint != endX)
						{
							mchnosd_region_destroy(sHandle);
							OSD_DEBUG("end X changed mchnosd_region_destroy\n");
							sHandle = -1;
						}
						printf("=====>x:%d\n",posItemY->valueint);
						endX = posIendX->valueint;
					}
					else
						OSD_DEBUG("no posIend X\n");
					cJSON *posIendY = cJSON_GetArrayItem(posInfo, 3);
					if (posIendY != NULL)
					{
						if(sHandle >= 0 && posIendY->valueint != endY)
						{
							mchnosd_region_destroy(sHandle);
							OSD_DEBUG("end Y changed mchnosd_region_destroy\n");
							sHandle = -1;
						}
						endY = posIendY->valueint;
					}
					else
						OSD_DEBUG("no posIend X\n");
				}
				else
					OSD_DEBUG("posInfo not array\n");
			}
			else
				OSD_DEBUG("No OSDInfoWidget\n");
		}
	}
	else
		OSD_DEBUG("OSDInfo not array,OSDInfo->type=%d\n", OSDInfo->type);
	mchnosd_region_t region;
	region.columns = 48;
	region.rows = sec_attr.text_line_num;
	region.x = posX;
	region.y = posY;
	region.endx = endX;
	region.endy = endY;
	region.mainFontSize = mainFontSize > 64 ? 64 : (mainFontSize & 0xFFFFFFFE);
	region.subFontSize = subFontSize > 64 ? 64 : (subFontSize & 0xFFFFFFFE);
	int i;
	for(i = 0;i < TEXT_LINE_NUM;i++)
		strcpy(region.text[i],sec_attr.text[i]);
	printf("osd pos x=%d,y=%d,endx=%d,endy=%d,fontSize=%d,%d\n",posX,posY,endX,endY,region.mainFontSize,region.subFontSize);
	printf("linenum=%d\n", sec_attr.text_line_num);
	if (sHandle < 0)
	{
		sHandle = mchnosd_region_create(-1, &region);
		OSD_DEBUG("mchnosd_region_create,sHandle=%d\n",sHandle);
		if (sHandle < 0)
		{
			OSD_DEBUG("ERROR: Failed create region\n");
			if(detail != NULL)strcpy(detail, "Failed create region");
			cJSON_Delete(root);
			return -1;
		}
	}
	if(sHandle >= 0)
	{
		mchnosd_region_destroy(sHandle);
		pre_text_line_num = sec_attr.text_line_num;
		sHandle = -1;
		sHandle = mchnosd_region_create(-1, &region);
		OSD_DEBUG("mchnosd_region_create,sHandle=%d\n",sHandle);
		if (sHandle < 0)
		{
			OSD_DEBUG("ERROR: Failed create region\n");
			if(detail != NULL)strcpy(detail, "Failed create region");
			cJSON_Delete(root);
			return -1;
		}
	}
	mchnosd_region_draw(sHandle, &sec_attr);
	if(sec_attr.text_line_num == 0)
	{	
		mchnosd_region_destroy(sHandle);
		sHandle = -1;
	}
	if(detail != NULL)strcpy(detail, "OK");
	cJSON_Delete(root);
	return 0;
}

static void _osd_server_process(void *param)
{

	pthreadinfo_add((char *)__func__);
	fd_set fds;
	struct timeval timeout;
	int timeout_interval=30;
	int ret=0;
	int bNeedDestroy=0;
	char recvbuf[10*1024];
	int  listenfd;
	struct  sockaddr_in server;
	struct  sockaddr_in client;
	socklen_t  addrlen;
	addrlen=sizeof(struct  sockaddr_in);
	if((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Creating  listenfd failed.");
	}
	bzero(&server, sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(OSD_SERVER_PORT);
	server.sin_addr.s_addr= htonl (INADDR_ANY);
	if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		perror("bind");
	}
	
	while (osdServerGroup.running)
	{
		while(1)
		{
			FD_ZERO(&fds);
			FD_SET(listenfd, &fds);
			timeout.tv_sec=timeout_interval*1;
			timeout.tv_usec=0;
			ret=select(listenfd+1, &fds, NULL, NULL, &timeout);
			if(ret<0)
			{
				perror("select");
				break;
			}
			else if(ret==0)
			{
				OSD_DEBUG("select timeout\n");
				break;
			}
			else
			{
				memset(recvbuf, 0, sizeof(recvbuf));
				bzero(&client, sizeof(client));
				ret = recvfrom(listenfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client, &addrlen);
				if(ret < 0)
				{
					OSD_DEBUG("recv empty\n");
					break;
				}
				recvbuf[ret] = '\0';
			}
			char detail[32];
			memset(detail, 0, sizeof(detail));
			ret=process_osd_msg(recvbuf, detail, 1);
			OSD_DEBUG("after process msg,ret=%d\n", ret);
			OSD_DEBUG("%s:%d\n", inet_ntoa(client.sin_addr), htons(client.sin_port));
			if(ret==0)
				bNeedDestroy=1;
			send_response(listenfd, &client, detail);
		}
		if(sHandle>=0 && bNeedDestroy == 1)
		{
			mchnosd_region_destroy(sHandle);
			OSD_DEBUG("mchnosd_region_destroy\n");
			sHandle=-1;
			bNeedDestroy=0;
		}
	}
	close(listenfd);
}

int osd_server_init(void)
{

	char *val=utl_fcfg_get_value(CONFIG_HWCONFIG_FILE, "enable_osdprint");
	if(!val)
		return -1;
	if(1 != atoi(val))
		return -1;

	printf("start osd server\n");
	osdServerGroup.running = TRUE;
	pthread_mutex_init(&osdServerGroup.mutex, NULL);
	pthread_create(&osdServerGroup.thread, NULL, (void *)_osd_server_process, NULL);
	return 0;
}

int osd_server_deinit(void)
{
	if(osdServerGroup.running)
	{
		osdServerGroup.running = FALSE;
	
		pthread_join(osdServerGroup.thread, NULL);
		pthread_mutex_destroy(&osdServerGroup.mutex);
	}
	return 0;
}


