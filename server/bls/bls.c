#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <iconv.h>
#include <stdint.h>
#include <netdb.h>

#include "jv_common.h"
#include "jv_stream.h"
#include "mipcinfo.h"
#include "utl_ifconfig.h"
#include "bls_librtmp.h"
#include "bls_flv.h"
#include <cJSON.h>
#include "mlog.h"

#include "bls.h"
#include "pcs.h"
//#include "AmfSerializer.h"

// 开始发布流后，数据发送顺序: 视频sequence header-> [视频帧 ->] 音频sequence header -> 音频或视频数据
// 1、第一帧必须是视频sequence header
// 2、第一个音频帧必须是音频sequence header
typedef enum
{
	BLS_STATE_INIT,			// 开始发布流，未开始发送任何数据
	BLS_STATE_VIDEO_SH,		// 发送了视频sequence header，需要再发送音频sequence header后，才能开始发送音频数据
	BLS_STATE_NORMAL,		// 视频sequence header、音频sequence header均已发送
}BLS_STATE;

static jv_thread_group_t BLS_group;

static BOOL bls_inited = FALSE; 
static bls_rtmp_t ortmp = 0;
static int publish_stream_id = 0;
static BLS_STATE s_BLS_State = BLS_STATE_INIT;

#define BLS_USE_HTTP_POST			1
#define BLS_INFO_SAVE_DAT			"/etc/conf.d/jovision/BLSInfoSave.dat"
#define HTTP_BUF_SIZE				4096
#define BLS_POST_TIMEOUT			10

static int __BLSHttpPost(const char *url, const char *req, int len, char *resp);

int BLSInfoSave(BLSInfoType* info)
{
	FILE *fOut = NULL;
	
	fOut=fopen(BLS_INFO_SAVE_DAT, "wb");
	if(!fOut)
	{
		Printf("__BLSInfoSave: %s, err: %s\n", BLS_INFO_SAVE_DAT, strerror(errno));
		return -1;
	}

	fwrite(info, 1, sizeof(BLSInfoType), fOut);
		
	fclose(fOut);
	return 0;
}

static int __BLSInfoGet(BLSInfoType* info)
{
#if (!BLS_USE_HTTP_POST)
	FILE *fOut = NULL;
	int len = 0;

	if (info == NULL)
	{
		return -1;
	}
	memset(info, 0, sizeof(BLSInfoType));
	if (access(BLS_INFO_SAVE_DAT, F_OK) != 0)
	{
		return -2;
	}
	fOut=fopen(BLS_INFO_SAVE_DAT, "rb");
	if(!fOut)
	{
		Printf("__BLSInfoGet: %s, err: %s\n", BLS_INFO_SAVE_DAT, strerror(errno));
		return -3;
	}

	len = fread(info, 1, sizeof(BLSInfoType), fOut);

	fclose(fOut);
	return len;
#else
	int ret = 0;
	ipcinfo_t ipcinfo;
	char deviceID[16] = {0};
	char* pUrl = "http://alarm-gw.jovision.com:80/netalarm-rs/videoplay/query_bd_token";
	char resp[HTTP_BUF_SIZE] = {0};

	ipcinfo_get_param(&ipcinfo);
	jv_ystNum_parse(deviceID, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);

	if (info == NULL)
	{
		return -1;
	}
	memset(info, 0, sizeof(BLSInfoType));

	cJSON *pReq = cJSON_CreateObject();
	cJSON_AddNumberToObject(pReq, "mid", 0);
	cJSON_AddStringToObject(pReq, "method", "query_bd_token");

	cJSON *pParam = cJSON_CreateObject();
	cJSON_AddStringToObject(pParam, "devId", deviceID);

	cJSON_AddItemToObject(pReq, "param", pParam);
	
	char* req = cJSON_PrintUnformatted(pReq);

	ret = __BLSHttpPost(pUrl, req, strlen(req), resp);

	free(req);
	cJSON_Delete(pReq);

	if (ret > 0)
	{
		printf("Http post result: \n%s\n", resp);

		cJSON* root = cJSON_Parse(resp);
		if (root != NULL)
		{
			S32 Err = cJSON_GetObjectValueInt(root, "rt");
			if (0 == Err)
			{
				cJSON* result = cJSON_GetObjectItem(root, "result");
				if (result != NULL)
				{
					const char* AccessToken = cJSON_GetObjectValueString(result, "accessToken");
					if (AccessToken != NULL)
					{
						strncpy(info->access_token, AccessToken, sizeof(info->access_token));
					}

					const char* StreamId = cJSON_GetObjectValueString(result, "streamid");
					if (StreamId != NULL)
					{
						strncpy(info->stream_ID, StreamId, sizeof(info->stream_ID));
					}
				}
				else
				{
					printf("Invalid post result: no result!\n");
					ret = -1;
				}
			}
			else
			{
				printf("Post error: %d(%s)!\n", Err, cJSON_GetObjectValueString(root, "errmsg"));
				ret = -2;
			}
			cJSON_Delete(root);
		}
		else
		{
			printf("Invalid post result: unknown format!\n");
			ret = -3;
		}
	}
	else
	{
		printf("Post failed: %d!\n", ret);
		ret = -4;
	}

	return ret;
#endif
}

static int __BLSGetUploadAddr(char* deviceID, char* stream_ID, char* addr, int addr_buf_len)
{
	int ret = -1;
	char buf[BLS_STR_LEN] = {0};
	unsigned long len = 0;
	unsigned int slen = 0;
	char* s = NULL;
	char* e = NULL;
	int i = 0;
	//获得上传地址
	for (i = 0; i < 3; i++)
	{
		len = sizeof(buf);
		ret = locate_upload(deviceID, stream_ID, buf, &len);
		if (ret == 0)
		{
			s = strstr(buf, "\"server_list\":[\"");
			if (s != NULL)
			{
				s += strlen("\"server_list\":[\"");
				e = strstr(s, "\"");
				if (e != NULL)
				{
					slen = e - s;
					memset(addr, 0, addr_buf_len);
					if (slen < addr_buf_len)
					{
						memcpy(addr, s, slen);
						Printf("PCS upload addr: %s\n", addr);
						break;
					}
					else
					{
						printf("PCS locate_upload buf out\n");
						return -1;
					}
				}
			}
		}
		usleep(100*1000);
	}
	if (i >= 3)
	{
		printf("PCS locate upload error\n");
		return -1;
	}
	return 0;
}

//获得成功返回0，结果放在acess_token中
static int __BLSGetAcessToken(char* deviceID, char* access_token, int access_buf_len)
{
	int ret = -1;
	char buf[BLS_STR_LEN] = {0};
	unsigned long len = 0;
	unsigned int slen = 0;
	char* s = NULL;
	char* e = NULL;
	int i = 0;
	//access_token 过期，重新获得
	for (i = 0; i < 5; i++)
	{
		len = sizeof(buf);
		ret = get_user_token(deviceID, access_token, buf, &len);
		if (ret == 0)
		{
			printf("PCS get_user_token: %s", buf);
			s = strstr(buf, "\"access_token\":\"");
			if (s != NULL)
			{
				s += strlen("\"access_token\":\"");
				e = strstr(s, "\"");
				if (e != NULL)
				{
					slen = e - s;
					memset(access_token, 0, access_buf_len);
					if (slen < access_buf_len)
					{
						memcpy(access_token, s, slen);
						Printf("PCS acess_token: %s\n", access_token);
						break;
					}
					else
					{
						printf("PCS get_user_token buf out\n");
						return -1;
					}
				}
			}
		}
		usleep(100*1000);
	}
	if (i >= 5)
	{
		printf("PCS get_user_token error\n");
		return -1;
	}
	return 0;
}

static int __BLSGetConnToken(char* deviceID, char* access_token, int access_buf_len, char* conn_token, int conn_buf_len)
{
	int ret = -1;
	char buf[BLS_STR_LEN] = {0};
	unsigned long len = 0;
	unsigned int slen = 0;
	char* s = NULL;
	char* e = NULL;
	int i = 0;
	char acess_token_update_succ = 0;
	char acess_token_updated = 0;
	//获得conn_token(session_token)
	for (i = 0; i < 3; i++)
	{
		len = sizeof(buf);
		ret = get_conn_token(deviceID, access_token, buf, &len);
		if (ret == 0)
		{
			printf("PCS get_conn_token: %s\n", buf);
			s = strstr(buf, "\"conn_token\":\"");
			if (s != NULL)
			{
				s += strlen("\"conn_token\":\"");
				e = strstr(s, "\"");
				if (e != NULL)
				{
					slen = e - s;
					memset(conn_token, 0, conn_buf_len);
					if (slen < conn_buf_len)
					{
						memcpy(conn_token, s, slen);
						Printf("PCS conn_token: %s\n", conn_token);
						break;
					}
					else
					{
						printf("PCS get_conn_token buf out\n");
						return -1;
					}
				}
			}
			else
			{
				if (acess_token_update_succ == 0)
				{
					acess_token_updated = 1;
					ret = __BLSGetAcessToken(deviceID, access_token, access_buf_len);
					if (ret == 0)
					{
						//access_token更新成功，需要保存在flash中，在此函数外面进行
						acess_token_update_succ = 1;
					}
				}
			}
		}
		usleep(100*1000);
	}
	if (acess_token_updated == 1 && acess_token_update_succ == 0)
	{
		//acess_token最终没有更新成功，需要亮灯，找百度解决
		printf("PCS __BLSGetAcessToken failed!!!\n");
	}
	if (i >= 3)
	{
		printf("PCS get_conn_token error\n");
		return -1;
	}
	return 0;
}

static int __BLSMetadataSend(bls_rtmp_t ortmp)
{
	int ret = 0;
   	int label_size = 0; 
   	int info_size = 0; 
	char *meta_data = NULL;
	int meta_data_size = 0;
	bls_amf0_t label = bls_amf0_create_str("onMetaData");
	bls_amf0_t meta_info = bls_amf0_create_ecma_array();
	// creator, width, height, absRecordTime information: string or number

	/*video*/
	bls_amf0_t width = bls_amf0_create_number(1280);
	bls_amf0_ecma_array_property_set(meta_info, "width", width);
	
	bls_amf0_t height = bls_amf0_create_number(720);
	bls_amf0_ecma_array_property_set(meta_info, "height", height);

	bls_amf0_t videodatarate = bls_amf0_create_number(440);
	bls_amf0_ecma_array_property_set(meta_info, "videodatarate", videodatarate);

	bls_amf0_t framerate = bls_amf0_create_number(20);
	bls_amf0_ecma_array_property_set(meta_info, "framerate", framerate);

	bls_amf0_t videocodecid = bls_amf0_create_number(7);
	bls_amf0_ecma_array_property_set(meta_info, "videocodecid", videocodecid);

	/*audio*/
	bls_amf0_t audiodatarate = bls_amf0_create_number(43.06640625);
	bls_amf0_ecma_array_property_set(meta_info, "audiodatarate", audiodatarate);

	bls_amf0_t audiosamplerate = bls_amf0_create_number(44100);
	bls_amf0_ecma_array_property_set(meta_info, "audiosamplerate", audiosamplerate);

	bls_amf0_t audiosamplesize = bls_amf0_create_number(16);
	bls_amf0_ecma_array_property_set(meta_info, "audiosamplesize", audiosamplesize);

	bls_amf0_t stereo = bls_amf0_create_number(0);
	bls_amf0_ecma_array_property_set(meta_info, "stereo", stereo);

	bls_amf0_t audiocodecid = bls_amf0_create_number(160);
	bls_amf0_ecma_array_property_set(meta_info, "audiocodecid", audiocodecid);

	unsigned long long time = 0;
	struct timeval t;
	gettimeofday(&t, NULL);
	time = (unsigned long long)t.tv_sec*1000 + t.tv_usec/1000;
	printf("absRecordTime = %llu\n", time);
	bls_amf0_t absRecordTime = bls_amf0_create_number(time);
	bls_amf0_ecma_array_property_set(meta_info, "absRecordTime", absRecordTime);

	//now we can encode such meta info to a byte stream
	label_size = bls_amf0_size(label);
	info_size = bls_amf0_size(meta_info);
	//atention: the memory leak!
	meta_data_size = label_size + info_size;
	meta_data = malloc(meta_data_size);
	printf("meta_data_size: %d\n", meta_data_size);
	if(meta_data == NULL){
		printf("malloc error\n");
		exit(-1);
	}
	bls_amf0_serialize(label, meta_data, label_size);
	bls_amf0_serialize(meta_info, meta_data + label_size, info_size);
	
	int size = 0;
	printf("ecma count: %d\n", bls_amf0_ecma_array_property_count(meta_info));
	printf("the encoded label info: %s\n", bls_amf0_human_print(label, NULL, &size));
	printf("the encoded meta info: %s\n", bls_amf0_human_print(meta_info, NULL, &size));
	
	ret = bls_write_meta(ortmp, meta_data, meta_data_size, publish_stream_id);
	if (ret != 0)
	{
		printf("bls_write_meta failed!!\n");
	}
	
	//the label and meta_info should be freed
	bls_amf0_free(label);
	bls_amf0_free(meta_info);
	free(meta_data);
	
	return 0;
}

int BLSSendAVCSH(BLS_SPS_PPS* p)
{
	int ret = -1;
	static BLS_SPS_PPS spspps;
	if (!bls_inited)
	{
		return -1;
	}

	//相同则不发
	if ((s_BLS_State != BLS_STATE_INIT) && (memcmp(p, &spspps, sizeof(BLS_SPS_PPS)) == 0))
	{
		return -1;
	}
	ret = bls_write_raw_avc_sh(ortmp, 0,
        66, 31, 0,
        p->pps_data, p->pps_size,
        p->sps_data, p->sps_size,
        publish_stream_id);
	if (ret != 0)
	{
		printf("BLSSendAVCSH failed!!\n");
	}
	else
	{
		printf("BLSSendAVCSH OK!\n");
	}
	
	spspps = *p;

	return 0;
}

static unsigned int s_TimestampBase = 0;

int BLSSendVideoPacket(unsigned char* data, int size, unsigned long long timestamp, int is_key)
{
	int ret = -1;

	if (!bls_inited)
	{
		return -1;
	}
	if (BLS_STATE_INIT == s_BLS_State)
	{
		if (!is_key)
		{
			return -1;
		}
		s_TimestampBase = timestamp;
		s_BLS_State = BLS_STATE_VIDEO_SH;
	}
	if (is_key)
	{
		//printf("is_key %d, timestamp: %d\n", is_key, timestamp - s_TimestampBase);
	}
	ret = bls_write_raw_video(ortmp, timestamp - s_TimestampBase,
        (char*)data, size, publish_stream_id, is_key);
	if (ret != 0)
	{
		Printf("BLSSendVideoPacket failed!\n");
	}

	return ret;
}

int BLSSendAudioPacket(unsigned char* data, int size, unsigned long long timestamp)
{
	int ret = -1;
#if 1
	// 0xAE00: AAC sequence header, 0xAE01: AAC raw
	static unsigned char PacketData[MAX_FRAME_LEN] = {0xAE, 0x01};

	//printf("=========BLSSendAudioPacket, bls_inited: %d, s_BLS_State: %d, size: %d, ts: %lld!\n", bls_inited, s_BLS_State, size, timestamp);

	if (!bls_inited)
	{
		return -1;
	}
	if (BLS_STATE_INIT == s_BLS_State)
	{
		return -1;
	}

	if (BLS_STATE_VIDEO_SH == s_BLS_State)
	{
		unsigned char AudioSH[] = {0xAE, 0x00, 0x12, 0x10};
		
		// 发送音频Sequence Header
		ret = bls_write_audio(ortmp, timestamp - s_TimestampBase,
	        (char*)AudioSH, sizeof(AudioSH), publish_stream_id);
		if (ret != 0)
		{
			printf("BLS Send Audio Header failed!\n");
		}
		else
		{
			printf("BLS Send Audio Header OK!\n");
		}

		s_BLS_State = BLS_STATE_NORMAL;
	}

	// 增加2字节包头，并去掉adts包头
	memcpy(PacketData + 2, data + 7, size - 7);

	ret = bls_write_audio(ortmp, timestamp - s_TimestampBase,
        (char*)PacketData, size + 2 - 7, publish_stream_id);
	if (ret != 0)
	{
		printf("BLSSendAudioPacket failed!\n");
	}
#else
	static FILE* pfd = NULL;

	if (pfd == NULL)
	{
		pfd = fopen("/progs/test.aac", "w+");
	}

    if (NULL == pfd)
    {
        printf("%s: open file test.aac failed\n", __FUNCTION__);
        return ret;
    }
	else
	{
        printf("========================%s: open file test.aac OK!\n", __FUNCTION__);
	}

    fwrite(data, 1, size, pfd);
#endif

	return ret;
}

/**
 *@brief 查找字符串的值
 *
 *@param body 消息体。类似这样：PLAY RTSP/1.0\r\n CSeq: 3\r\n Scale: 0.5\r\n Range: npt=0-
 *@param key 要查找的键值
 *@param seg 分割符，可以是':', '=' 等
 *@param data 返回结果
 *@param maxLen 提供的value的最大保存长度
 */
static char *__get_line_value(const char *body, const char *key, char seg, char *data, int maxLen)
{
	char *p;
	char *dst;
	int len;

	p = (char *)strstr(body, key);
	if (data)
		data[0] = '\0';
	if (!p)
		return NULL;

	p += strlen(key);
	while (*p && *p != seg)
		p++;
	p++;
	dst = p;
	len = 0;
	while (*p && *p != '\r' && *p != '\n')
	{
		p++;
		len++;
	}

	if (*dst == '"')
	{
		dst++;
		len-=2; // " 一定是成对出现的
	}

	if (data)
		p = data;
	else
		p = (char *)malloc(len+1);
	memcpy(p, dst, len);
	p[len] = '\0';

	return p;
}

/**
 *@brief 检测Content-Length 字段，获取数据总长度
 *@param predata 已收到的数据
 *@return 将要收到的数据的总长度。
 *@note 数据总长度为：HTTP头的长度 + Content-Length所指定的数据长度
 */
static int __get_content_length(const char *predata)
{
	char *tstr;
	char temp[32];
	int contentLen;
	if (!predata)
	{
		return -1;
	}
	tstr = __get_line_value(predata, "Content-Length", ':', temp, 32);
	if (tstr)
	{
		contentLen = atoi(tstr);
		const char *p = strstr(predata, "\r\n\r\n");
		if (p)
		{
			return contentLen + p - predata + 4;
		}
		return contentLen + strlen(predata);
	}
	else
	{
		return 0;
	}

}

/**
 *@brief 向指定服务地址发送post请求
 *@param url 服务地址
 *@param req 请求数据
 *@param len 请求数据长度
 *@param resp 返回的响应数据
 *@return 返回的数据的总长度。
 *@note 数据总长度为：HTTP头的长度 + Content-Length所指定的数据长度
 */
static int __BLSHttpPost(const char *url, const char *req, int len, char *resp)
{
	int sockfd = 0;
	int ret = 0;
	int port = 0;
	int contentLen = 0;
	int chunkLen = 0;
	int offset = 0;
	int bChunked = 0;
	struct sockaddr_in servaddr;
	char sendbuf[HTTP_BUF_SIZE] = {0};
	char recvbuf[HTTP_BUF_SIZE] = {0};
	char domain[128] = {0};
	char ipaddr[16] = {0};
	
	memset(sendbuf, 0, sizeof(sendbuf));
	memset(ipaddr, 0, sizeof(ipaddr));
	
	if(url == NULL)
	{
		printf("NULL url\n");
		return -1;
	}
	printf("url=%s\n", url);

	char *ptrDomain = strchr(url, ':');
	if(ptrDomain == NULL)
	{
		printf("Invalid url:%s\n", url);
		return -1;
	}
	char *ptrPort = strchr(ptrDomain+1, ':');
	if(ptrPort == NULL)
	{
		printf("Invalid url:%s\n", url);
		return -1;
	}

	strncpy(domain, ptrDomain+3, ptrPort-ptrDomain-3);
	
	struct hostent *host = gethostbyname(domain);
	if(host == NULL)
	{
		printf("gethostbyname failed\n");
		return -1;		
	}
	//for(pptr=host->h_addr_list; *pptr!=NULL; pptr++)
	//	printf("address:%s\n", inet_ntop(host->h_addrtype, *pptr, str, sizeof(str)));
	if(inet_ntop(host->h_addrtype, host->h_addr, ipaddr, sizeof(ipaddr)) != NULL)
	{
		printf("ipaddr:%s\n", ipaddr);
	}

	port = atoi(ptrPort+1);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}

	struct timeval t;
	t.tv_sec = BLS_POST_TIMEOUT;
	t.tv_usec = 0;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t));	//printf("connect to tcp server, sock %d\n",user->fd);
	if(ret != 0)
	{
		perror("setsockopt");
		close(sockfd);
		return -1;
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if(inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0)
	{
		printf("inet_pton error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("connect error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	char *service = strstr(ptrPort, "/");
	sprintf(sendbuf, "POST %s HTTP/1.1\r\n"
					"Host: %s\r\n"
					"Content-Type: application/json;charset=UTF-8\r\n"
					"Content-Length: %d\r\n"
					"\r\n",
					service, domain, len);
		
	strcat(sendbuf, req);
	printf("%s\n",sendbuf);

	ret = send(sockfd, sendbuf, strlen(sendbuf), 0);
	if (ret < 0)
	{
		printf("send error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	while(1)
	{
		ret = recv(sockfd, recvbuf+offset, sizeof(recvbuf) - offset, 0);
		if (ret == -1)
		{
			printf("error: recv failed : %s\n", strerror(errno));
			close(sockfd);
			return -1;
		}
		else if (ret == 0)
		{
			printf("socket is closed\n");
			close(sockfd);
			return -1;
		}
		else if (ret >= sizeof(recvbuf) - offset)
		{
			printf("recv data too big\n");
			close(sockfd);
			return -1;
		}
		offset += ret;
		recvbuf[offset] = '\0';
		//printf("recv:\n%s\n", recvbuf);
		if(strstr(recvbuf, "Content-Length:") != NULL)
		{
			if (contentLen == 0)
			{
				contentLen = __get_content_length(recvbuf);
			}
			if (contentLen == 0 || contentLen <= offset)
			{
				if(contentLen > 0)
				{
					char *httpBody = strstr(recvbuf, "\r\n\r\n");
					if(httpBody == NULL)
					{
						printf("http resp incorrect\n");
						return 0;
					}
					strcpy(resp, httpBody+4);
				}
				break;
			}
		}
		else if(strstr(recvbuf, "Transfer-Encoding: chunked") != NULL)
		{
			bChunked = 1;
			char *httpBody = strstr(recvbuf, "\r\n\r\n");
			//printf("httpBody:\n%s\n", httpBody);
			offset = 0;
			if(httpBody != NULL)
			{
				char size[8] = {0};
				char *bodySize = httpBody+4;
				char *bodyStart = strstr(bodySize, "\r\n");
				if(bodyStart != NULL)
				{
					//printf("bodySize:\n%s\n", bodySize);
					strncpy(size, bodySize, (int)(bodyStart-bodySize));
					sscanf(size,"%x",&chunkLen);
					//printf("chunkLen:%d\n", chunkLen);
					
					bodyStart+=2;
					//printf("bodyStart:\n%s\n", bodyStart);
					strncpy(resp+contentLen, bodyStart, chunkLen);

					contentLen += chunkLen;
					//printf("contentLen:%d\n", contentLen);
					if(strstr(bodyStart, "0\r\n\r\n") != NULL)//recvd all the chunks
					{
						//printf("1resp:\n%s\n", resp);
						break;
					}
				}
				else
				{
					printf("1HTTP resp incorrect!\n");
					close(sockfd);
					return -1;
				}
			}
			else
			{
				printf("2HTTP resp incorrect!\n");
				close(sockfd);
				return -1;
			}
		}
		else
		{
			if(bChunked == 1)
			{
				char *end = strstr(recvbuf, "0\r\n\r\n");
				if(end != NULL)//recvd all the chunks
				{
					strncpy(resp+contentLen, recvbuf, end-recvbuf);
					//printf("2resp:\n%s\n", resp);
					break;
				}
				else
				{
					strncpy(resp+contentLen, recvbuf, ret);
				}
			}
			else
			{
				printf("3HTTP resp incorrect!\n");
				close(sockfd);
				return -1;
			}
		}
	}
	close(sockfd);
	return contentLen;
}

static int __BLSBuild(BLSInfoType* info)
{
	//access_token stream_id需要客户端通过声波配置过来
	char access_token[BLS_STR_LEN] = {0};
	char* stream_ID = info->stream_ID;
	char deviceID[32] = {0};
	ipcinfo_t ipcinfo;
	char addr[BLS_STR_LEN] = {0};
	char conn_token[BLS_STR_LEN] = {0};
	int ret = 0;

	memcpy(access_token, info->access_token, sizeof(access_token));
	
	ipcinfo_get_param(&ipcinfo);
	jv_ystNum_parse(deviceID, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
	//char access_token[BLS_STR_LEN] = "23.66787bcd8f7ece39d3eca9ec77e3d2e9.2592000.1454137134.2013387716-7499131";
	//char* stream_ID = "0aea6643b5dc11e5bbdff80f41fbe890";
	//strcpy(deviceID, "B129052494");
	ret =__BLSGetUploadAddr(deviceID, stream_ID, addr, sizeof(addr));
	if (ret != 0)
	{
		return -1;
	}
	ret = __BLSGetConnToken(deviceID, access_token, sizeof(access_token), conn_token, sizeof(conn_token));
	if (ret != 0)
	{
		return -1;
	}

	if (memcmp(info->access_token, access_token, sizeof(access_token)) != 0)
	{
		memcpy(info->access_token, access_token, sizeof(access_token));
		BLSInfoSave(info);
	}

	strcat(addr, "/live/");
	strcat(addr, stream_ID);
	printf("[addr: %s\n access_token: %s\n conn_token: %s\n", addr, access_token, conn_token);
	ortmp = bls_rtmp_create(addr, deviceID, access_token, conn_token, "extjson");
	unsigned int server_time = 0;
	ret = bls_connect_app(ortmp, &server_time);
	if (ret)
	{
		printf("connect error, ret=%d\n", ret);
		return ret;
	}
	printf("connect:server time=%d", server_time);

	
	ret = bls_publish_stream(ortmp, stream_ID, &publish_stream_id);
	if(ret)
	{
		printf("publish error, ret=%d", ret);
		return ret;
	}
	s_BLS_State = BLS_STATE_INIT;
	printf("publish: publish_stream_id=%d", publish_stream_id);
	ret = bls_set_streamproperty(ortmp, 7);
	if(ret){
		printf("set streamproperty error, ret=%d", ret);
		return ret;
	}

	__BLSMetadataSend(ortmp);
	bls_inited = TRUE;

	return 0;
}

static void* __BLSBuildThread(void* p)
{
	int ret = -1;
	BLSInfoType info;
	ipcinfo_t ipcinfo;

	pthreadinfo_add((char *)__func__);

	BLS_group.running = TRUE;
	while (BLS_group.running)
	{
		ipcinfo_get_param(&ipcinfo);

		while (utl_ifconfig_net_prepared() <= 0)
		{
			sleep(2);
		}

		while (__BLSInfoGet(&info) < 0)
		{
			printf("%s: Get BLS info failed!!! Retry...\n", __func__);
			sleep(5);
		}

		// 开启网络校时时，等待校时完成后再连接百度云
		if (ipcinfo.bSntp)
		{
			int nCnt = 150;

			printf("%s: Wait for NTP...\n", __func__);
			while ((!ipcinfo_TimeCalibrated()) && (nCnt > 0))
			{
				sleep(1);
				--nCnt;
			}
		}
		else
		{
			printf("%s: NTP is OFF.\n", __func__);
		}

		ret = __BLSBuild(&info);
		if (ret == 0)
		{
#if 0
			char* pUrl = "http://alarm-gw.jovision.com:80/netalarm-rs/videoplay/bd_play";
			char req[HTTP_BUF_SIZE] = "{\"mid\":1,\"param\":{\"deviceid\":\"H1242173\"}}";
			char resp[HTTP_BUF_SIZE] = {0};

			ret = __BLSHttpPost(pUrl, req, strlen(req), resp);
			if (ret > 0)
			{
				printf("Http post result: \n%s\n", resp);
			}
			else
			{
				printf("Http post failed: %d\n", ret);
			}
#endif
			
			mlog_write("百度云连接成功!");

			while (bls_isconnected(ortmp))
			{
				//printf("%s: bls ================Connected!!!!!!!!!!\n", __func__);
				sleep(5);
			}

			mlog_write("百度云连接断开!");
		}
		else
		{
			mlog_write("百度云连接失败!");
		}

		bls_inited = FALSE;

		if (NULL != ortmp)
		{
			bls_unpublish_stream(ortmp, publish_stream_id);
			publish_stream_id = 0;
			bls_rtmp_destroy(ortmp);
			ortmp = NULL;
			sleep(15);
		}

		sleep(2);
		printf("%s: bls ================Reconnect..........\n", __func__);
		mlog_write("百度云重新连接...");
	}
	BLS_group.running = FALSE;
	return 0;
}


int BLSBuildStart()
{
	if (BLS_group.running == TRUE)
	{
		return 0;
	}
	pthread_create(&BLS_group.thread, NULL, __BLSBuildThread, NULL);
	pthread_detach(BLS_group.thread);
	
	return 0;
}

void BLSBuildEnd()
{
	bls_inited = FALSE;
	BLS_group.running = FALSE;

	if (NULL != ortmp)
	{
		bls_unpublish_stream(ortmp, publish_stream_id);
		publish_stream_id = 0;
		bls_rtmp_destroy(ortmp);
		ortmp = NULL;
	}
}


