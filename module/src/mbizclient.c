#include "jv_common.h"
#include "BizDevLib.h"
#include "mbizclient.h"
#include "mipcinfo.h"
#include "maccount.h"
#include "mstorage.h"
#include "mcloud.h"
#include "mioctrl.h"
#include "maudio.h"
#include "utl_ifconfig.h"
#include "JvServer.h"
#include "httpclient.h"
#include "utl_timer.h"
#include "cJSON.h"
#include "mrecord.h"
#include "utl_aes.h"
#include "utl_base64.h"
#include "md5.h"
#include "msnapshot.h"
#include "utl_common.h"

#define BIZSRVCFG	"/etc/conf.d/BizSrvCfg/"
#define BIZSRVDNS	"/tmp/dns.xml"
#define BIZSTATUS	"/tmp/bizstatus"

// #define _DEBUG_TEST		// http推送测试
// #define _BIZDEV_TEST		// BizDev测试

#ifdef _DEBUG_TEST
#define HTTP_POST_ADDR	"182.92.201.219"
#define HTTP_POST_PORT	8080
static int s_PushDevInfoCnt = 0;
static int s_PushAlarmCnt = 0;
#else
#define HTTP_POST_ADDR	(hwinfo.bInternational ? "xw-ger.jovision.com" : "xwdev.cloudsee.net")
#define HTTP_POST_PORT	80
#endif

#ifdef _BIZDEV_TEST
static int s_PushContentCnt = -1;
static int s_PushErrCnt = 0;
#endif

#define HTTP_AES_KEY	"JovisIonNOiSIVOj"
#define HTTP_AES_IV		"$IujZMHFG&6AnVU@"

typedef enum
{
	BIZ_PUSH_CLOUD		= 1,	// 云存储开关存储
	BIZ_PUSH_RECORD,			// 录像
	BIZ_PUSH_CAPTURE,			// 抓图
	BIZ_PUSH_CHAT_REFUSE,		// 由设备端发起的视频对讲被拒接提示
	BIZ_PUSH_MAX,
}BIZ_PUSH_TYPE;

typedef union
{
	struct
	{
		BizDevInfo	Info;
		BOOL		bModUser;
	};
	struct
	{
		char	devname[32];
		char	yst_no[20];
		U8		channel;
		U8		amt;
		char	aguid[32];
		U8		atype;
		U32		atime;
		char	apicture[256];
		char	avideo[256];
	};
}BizClientParam;

static U64	s_LastDownloadTime = 0;
static U32	s_DownloadFailCnt = 0;


static int _mbizclient_RefreshServer(void *data)
{
	//重新下载DNS文件到指定目录下/tmp
	int ret = 0;
	BOOL bFlag = FALSE;
	char url[128] = {0};
	char* devDnsURL[3] = {
		"http://xwdns2.cloudsee.net:8088/ipManage2.0/getdevdns.do",
		"http://xwdns1.cloudsee.net:8088/ipManage2.0/getdevdns.do",
		"http://xwdns3.cloudsee.net:8088/ipManage2.0/getdevdns.do",
		};
	int i = 0;
	U64 diff_ms = utl_get_ms() - s_LastDownloadTime;

	// 2个小时内不重复下载dns，降低服务器压力
	if ((s_LastDownloadTime != 0) && (diff_ms < 2 * 3600 * 1000))
	{
		printf("mbizclient RefreshServer, no need download dns file, diff time: %lld ms\n", diff_ms);
		return 0;
	}

	if (hwinfo.bInternational)
	{
		for(i = 0; i < ARRAY_SIZE(devDnsURL); i++)
		{
			devDnsURL[i] = "http://xw-ger.jovision.com/ipManage2.0/getdevdns.do";
		}
	}

	for(i = 0; i < ARRAY_SIZE(devDnsURL); i++)
	{
		snprintf(url, sizeof(url),"%s",devDnsURL[i]);
		char path_file[64] = {0};
		strcpy(path_file, BIZSRVDNS);
		char *pfile = NULL;
		pfile = strrchr(path_file, '/');
		*pfile = 0;
		
		ret = http_download(url, path_file, pfile+1, NULL, NULL);
		if(ret != -1)
		{
			printf("http download file dns success!!! \n");
			bFlag = TRUE;
			break;
		}
	}
	if(bFlag)
	{
		s_LastDownloadTime = utl_get_ms();
		s_DownloadFailCnt = 0;
		printf("mbizclient RefreshServer, download successfully at %lldms\n", s_LastDownloadTime);
		return 0;
	}
	else
	{
		++s_DownloadFailCnt;
		// 连续12次获取失败（大约5分钟），则使用上次获取的dns
		if ((s_LastDownloadTime != 0) && (s_DownloadFailCnt > 12))
		{
			printf("mbizclient RefreshServer, download failed %d times, use last dns\n", s_DownloadFailCnt);
			s_DownloadFailCnt = 0;
			return 0;
		}
		else
		{
			printf("mbizclient RefreshServer, download failed %d times!\n", s_DownloadFailCnt);
		}
		return -1;
	}
}

int mbizclient_updateuserinfo_server(char *username, char *password)
{
	ipcinfo_t ipcinfo;

	ipcinfo_get_param(&ipcinfo);
	if (ipcinfo.nDeviceInfo[6] != 'H')
		return -1;
	if (!ipcinfo.ystID)		// 云视通号码未初始化前不推送
		return -1;

	printf("mbizclient update user is %s, pwd is %s \n", username, password);

	// 下面的推送暂时保留，后续更换新库后再行删除
	// BizDevPushUserInfo(username,password);

	BizDevInfo Info = {{0}};

	jv_ystNum_parse(Info.yst_no, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
	snprintf(Info.username, sizeof(Info.username), "%s", username);
	snprintf(Info.password, sizeof(Info.password), "%s", password);

	return mbizclient_PushDevInfo(&Info, TRUE);
}

static int _mbizclient_PushUserInfo(ZINT8 status, void *data)
{
	printf("mbizclient motify username success!! \n");
	return 0;
}


static int _mbizclient_UpdateUserInfo(ZINT8 *username, ZINT8 *password, void *data)
{
	//更改用户信息
	ACCOUNT account;
	
	account.nIndex = 0;
	strcpy(account.acID,username);
	strcpy(account.acPW,password);

	printf("bizclient motify userinfo , acID : %s, pw : %s \n",account.acID,account.acPW);

	return maccount_modify(&account);
}

static int _mbizclient_Alarm(ZINT8 status, void *data)
{
	//报警上传状态
	if(0 == status)
		printf("mbizclient Alarm send success!! \n");
	else
		printf("mbizclient Alarm Send failed,and status is %d \n",status);
	return 0;
}

static int _mbizclient_PushCustomAlarm(BIZ_PUSH_TYPE Type, const char* payload)
{
	if (!hwinfo.bXWNewServer)
	{
		return -1;
	}

	JV_ALARM alarm;
	int ret = -1;
	char *ppic = "";
	char *pvideo = "";

	if (Type == BIZ_PUSH_RECORD)
	{
		cJSON* root = cJSON_Parse(payload);
		int duration = cJSON_GetObjectValueInt(root, "dur");

		if (duration <= 0 || duration > 300)
		{
			printf("%s, invalid duration: %d!!!\n", __func__, duration);
			duration = 10;
		}
		REC_REQ_PARAM req = {.ReqType = RECORD_REQ_PUSH, .nDuration = duration, .pCallback = NULL};
		ret = mrecord_request_start_record(0, &req);
		printf("%s, mrecord_request_start_record %s!\n", __func__, (ret == 0) ? "successful" : "failed");
	}

	malarm_build_info(&alarm, (Type == BIZ_PUSH_CAPTURE) ? ALARM_SHOT_PUSH : ALARM_REC_PUSH);
	mrecord_alarm_get_attachfile(REC_PUSH, &alarm);
	
	if (msnapshot_get_file(0, alarm.PicName) != 0)
	{
		alarm.PicName[0] = '\0';
	}
	if (alarm.PicName[0])
	{
		ppic = alarm.PicName;
	}
	if (alarm.VideoName[0])
	{
		pvideo = alarm.VideoName;
	}
	ret = mbizclient_PushAlarm("PUSH", alarm.ystNo, alarm.nChannel, ALARM_TEXT, alarm.uid, alarm.alarmType, alarm.time, ppic, pvideo);
	printf("PUSH mbizclient_PushAlarm %s(%d)!!!\n", (ret == 0) ? "successful" : "failed", ret);

	return ret;
}

static int _mbizclient_OnPush(ZUINT8 *payload, ZUINT32 len, ZUINT32 type, void *data)
{
	printf("mbizclient OnPush, type: %d, payload %s\n", type, payload);

#ifndef _BIZDEV_TEST
	switch (type)
	{
	case BIZ_PUSH_CLOUD:
		#ifdef OBSS_CLOUDSTORAGE
		mcloud_parse_obssstate((char*)payload);
		#endif
		break;
	case BIZ_PUSH_RECORD:
	case BIZ_PUSH_CAPTURE:
		_mbizclient_PushCustomAlarm(type, (char*)payload);
		break;
	case BIZ_PUSH_CHAT_REFUSE:
		mio_chat_refuse();
		break;
	default:
		break;
	}
#else
	cJSON *json = cJSON_Parse((char*)payload);
	if (!json)
	{
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
	}
	else
	{
		const char *count = cJSON_GetObjectValueString(json, "count");
		if (count != NULL)
		{
			int NowCnt = atoi(count);
			if (s_PushContentCnt != -1)
			{
				if (NowCnt - s_PushContentCnt != 1)
				{
					++s_PushErrCnt;
					printf("%s, ==========Count error!!! Last: %d, Now: %d, ErrCnt: %d\n", 
									__func__, s_PushContentCnt, NowCnt, s_PushErrCnt);
				}
			}
			s_PushContentCnt = NowCnt;
		}
		else
		{
			printf("cJSON_GetObjectValueString count failed!!!\n");
		}
	}

	cJSON_Delete(json);
#endif

	return 0;
}

static int _mbizclient_OnlineStatus(ZUINT8 online, void *data)
{
	printf("%s\n", online ? "Biz Online!!!" : "Biz Offline!!");

#define MAX_RECORD_CNT		5
	static int		Cnt = 0;
	static char		buf[MAX_RECORD_CNT][42] = {{0}};
	time_t now = time(NULL);
	struct tm nowtm;

	++Cnt;
	localtime_r(&now, &nowtm);
	snprintf(buf[(Cnt - 1) % MAX_RECORD_CNT], sizeof(buf[0]), 
				"%-4d  %04d/%02d/%02d %02d:%02d:%02d  %s\n", Cnt, 
				nowtm.tm_year + 1900, nowtm.tm_mon + 1, nowtm.tm_mday, nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec,
				online ? "Biz Online!!!" : "Biz Offline!!");

	FILE*	fp = fopen(BIZSTATUS, "w");
	if (!fp)
	{
		printf("%s, open file %s error: %s(%d)!\n", __func__, BIZSTATUS, strerror(errno), errno);
		return 0;
	}

	fwrite(buf[Cnt % MAX_RECORD_CNT], 1, (MAX_RECORD_CNT - Cnt % MAX_RECORD_CNT) * sizeof(buf[0]), fp);
	fwrite(buf[0], 1, (Cnt % MAX_RECORD_CNT) * sizeof(buf[0]), fp);

	fclose(fp);

	return 0;
}

static int _mbizclient_CheckResponse(const char* JsonResp, int isAlarm)
{
	int ret = -1;
	cJSON *root = cJSON_Parse(JsonResp);
	if(root == NULL)
	{
		printf("cJSON_Parse err!\n");
		return ret;
	}

	// 中航设备只有报警推送按中航方式校验，上线信息还是走小维服务器
	if (PRODUCT_MATCH(PRODUCT_ZHDZ) && isAlarm)
	{
		int code = cJSON_GetObjectValueInt(root, "code");
		if(code == 200)
		{
			ret = 0;		// 成功
		}
		else
		{
			printf("cJSON_GetObjectValueString code: %d\n", code);
		}
	}
	else
	{
		cJSON* data = cJSON_GetObjectItem(root, "root");
		if (data == NULL)
		{
			printf("cJSON_GetObjectItem root failed!\n");
			goto Err;
		}

		const char* result = cJSON_GetObjectValueString(data, "result");
		if (result == NULL)
		{
			printf("cJSON_GetObjectValueString result failed!\n");
			goto Err;
		}

		if(!strcmp(result, "true"))
		{
			ret = 0;		// 成功
		}
		else
		{
			printf("cJSON_GetObjectValueString result: %s\n", result);
		}

	}

Err:
	cJSON_Delete(root);

	return ret;
}

#ifdef _DEBUG_TEST
static int _gen_num(int min, int max)
{
	return min + (rand() % (max - min + 1));
}

static char _gen_char()
{
	return _gen_num(' ' + 1, '~');
}

static char* _gen_string(char* dst, unsigned int len)
{
	char* p = dst;

	if (len <= 0)
	{
		return NULL;
	}

	dst[len] = '\0';
	while (--len)
	{
		*p++ = _gen_char();
	}

	return dst;
}

static void* _mbizclient_test(void* arg)
{
	BizDevInfo info;
	char tmp[256] = {0};
	char tmp2[256] = {0};
	char tmp3[256] = {0};
	int count = 0;

	pthreadinfo_add((char *)__func__);

	while (1)
	{
		memset(&info, 0, sizeof(info));

		// snprintf(info.yst_no, sizeof(info.yst_no), "T%s%d", _gen_string(tmp, _gen_num(0, 3)), _gen_num(0x80000000, 0x7F000000));
		snprintf(info.yst_no, sizeof(info.yst_no), "%c%u", _gen_num('A', 'Z'), _gen_num(0, 0x7F000000));
		_gen_string(info.dev_type, _gen_num(1, sizeof(info.dev_type)));
		_gen_string(info.dev_version, _gen_num(1, sizeof(info.dev_version)));
		_gen_string(info.username, _gen_num(1, sizeof(info.username)));
		_gen_string(info.password, _gen_num(1, sizeof(info.password)));
		info.dev_chans = _gen_num(0, 100);
		info.sdcard = _gen_num(0, 1);
		info.cloud = _gen_num(0, 1);
		_gen_string(info.netlibversionstr, _gen_num(1, sizeof(info.netlibversionstr)));
		info.netlibversion = _gen_num(0x80000000, 0x7F000000);

		count = _gen_num(1, 5);
		while (count)
		{
			if (s_PushDevInfoCnt < 5)
			{
				mbizclient_PushDevInfo(&info, _gen_num(0, 1));
				--count;
			}
			usleep(100* 1000);
		}

		count = _gen_num(1, 5);
		while (count)
		{
			if (s_PushAlarmCnt < 5)
			{
				mbizclient_PushAlarm("Dummy", info.yst_no, _gen_num(0, info.dev_chans), _gen_num(0, 255), 
								_gen_string(tmp, _gen_num(1, 20)), _gen_num(0, 255), _gen_num(0x80000000, 0x7F000000), 
								_gen_string(tmp2, _gen_num(1, 100)), 
								_gen_string(tmp3, _gen_num(1, 100)));
				--count;
			}
			usleep(100* 1000);
		}
	}

	return NULL;
}
#endif

static int _mbizclient_append_signature(PARAM_MGR mgr)
{
	if (!mgr)
	{
		return -1;
	}

	int random;
	char buf[1024] = {0};
	char md5_enc[16] = {0};
	char signature[33] = {0};
	int seed = ipcinfo_get_param(NULL)->ystID;
	
#ifdef _DEBUG_TEST
	const char* deviceid = Http_get_param_val(mgr, "deviceGuid");
	if (deviceid)
	{
		while (*deviceid && (*deviceid > '9' || *deviceid < '0'))
			deviceid++;

		seed = atoi(deviceid);
	}
#endif

	srand((U32)utl_get_ms());
	random = 0x10000 + rand() % 0x1000000;
	seed += random;
	Http_add_param_int(mgr, "deviceCategory", (seed * 22 + seed % 18) ^ 16755421);

	Http_remove_param(mgr, "sig");	// 移除上一次的签名

	// 将所有参数的value字段拼接，并计算md5，作为签名
	Http_param_generate_value(mgr, buf, sizeof(buf));
	Http_url_decode(buf, buf);

	// printf("%s, md5 buf: \n%s\n", __func__, buf);

	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (U8*)buf, strlen(buf));
	MD5Final(&ctx, (U8*)md5_enc);

	int i = 0;
	for (i = 0; i < 16; ++i)
	{
		sprintf(signature + i * 2, "%02X", md5_enc[i]);
	}

	Http_add_param_string(mgr, "sig", signature);
	Http_add_param_int(mgr, "deviceCategory", random);

	return 0;
}

// 增加用户名密码信息
static int _mbizclient_append_userinfo(PARAM_MGR mgr)
{
	if (!mgr)
	{
		return -1;
	}

	// 用户名\r密码，并使用aes加密
	ACCOUNT stAcc = *(maccount_get(0));
	char buf[64] = {0};
	char encrypt[64] = {0};
	char base64[128] = {0};

#ifdef _DEBUG_TEST
	extern char* _gen_string(char* dst, unsigned int len);
	extern int _gen_num(int min, int max);
	_gen_string(stAcc.acID, _gen_num(1, sizeof(stAcc.acID)));
	_gen_string(stAcc.acPW, _gen_num(1, sizeof(stAcc.acPW)));
	Http_add_param_string(mgr, "username", stAcc.acID);
	Http_add_param_string(mgr, "passwd", stAcc.acPW);
#endif

	snprintf(buf, sizeof(buf), "%s\r%s", stAcc.acID, stAcc.acPW);

	int len = ((strlen(buf) + 15) & (~15));					// Align up to 16

	AES128_CBC_encrypt_buffer((uint8_t*)encrypt, (uint8_t*)buf, len, (uint8_t*)HTTP_AES_KEY, (uint8_t*)HTTP_AES_IV);
	utl_base64_encode_m((U8*)encrypt, len, base64);
	Http_add_param_string(mgr, "deviceUsername", base64);

	// printf("%s, yst: %s, user: %s, pwd: %s, base: %s\n", __func__, Http_get_param_val(mgr, "deviceGuid"), stAcc.acID, stAcc.acPW, base64);

	return 0;
}

static void* _mbizclient_init_thread(void *p)
{
	int ret = 0;
	BizDevInfo info;
	BizDevCb   DevCb;
	ipcinfo_t ipcinfo;

	pthreadinfo_add((char *)__func__);

	ipcinfo_get_param(&ipcinfo);

	memset(&info, 0, sizeof(info));
	
	jv_ystNum_parse(info.yst_no, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
	strcpy(info.dev_type, hwinfo.devName);
	strcpy(info.dev_version, IPCAM_VERSION);

	ACCOUNT *pstrAcc = maccount_get(0);

	strcpy(info.username,pstrAcc->acID);
	strcpy(info.password,pstrAcc->acPW);

	info.dev_chans = 1;
	
	STORAGE storage;
	memset(&storage, 0, sizeof(STORAGE));
	if (0 != mstorage_get_info(&storage))
	{
		info.sdcard = 0;
	}
	else
		info.sdcard = 1;
	
	info.cloud = hwinfo.bSupportXWCloud;

#ifdef YST_SVR_SUPPORT
	if (gp.bNeedYST)
	{
		if (hwinfo.bInternational)
		{
			//国外的云视通走网络优化前的逻辑
			info.netlibversion = 0;
			strcpy(info.netlibversionstr, "0");
		}
		else
		{
			if (JVN_GetSDKVer(&info.netlibversion, info.netlibversionstr) != 0)
			{
				printf("%s, JVN GetSDKVer failed!\n", __func__);
			}
		}
		printf("%s, JVN GetSDKVer: %d, %s!\n", __func__, info.netlibversion, info.netlibversionstr);
	}
#endif

	DevCb.OnRefreshServer	= _mbizclient_RefreshServer;
	DevCb.OnUpdateUserInfo	= _mbizclient_UpdateUserInfo;
	DevCb.OnPush			= _mbizclient_OnPush;
	DevCb.OnOnlineStatus	= _mbizclient_OnlineStatus;
	
	while (!utl_ifconfig_net_prepared())
	{
		sleep(1);
	}

	mbizclient_PushDevInfo(&info, FALSE);

	// 在PushDevInfo推送时完成防串货判定，如下推送废弃
	#if 0
	char url[256] = {0};
	snprintf(url, sizeof(url), "http://%s:%d/publicService/fchManage/fchInfo.do?deviceGuid=%s&deviceType=%s", 
								"182.92.201.219", 8080, info.yst_no, hwinfo.devName);
	http_download(url, "/tmp/", "prevent_info", NULL, NULL);
	#endif

	_mbizclient_RefreshServer(NULL);

	ret = BizDevInit(info.yst_no, &DevCb, BIZSRVCFG, BIZSRVDNS, NULL);
	if (ret == -1)
	{
		printf("biz client init failed !! \n");
	}
	else
	{
		Printf("BizDevInit successful!!!! \n\n");
	}
	return NULL;
}

// 推送成功，返回>0，否则失败
static void* __mbizclient_PushDevInfo(void* arg)
{
	BizClientParam* Param = (BizClientParam*)arg;
	BizDevInfo* info = &Param->Info;
	BOOL bModifyUserInfo = Param->bModUser;
	
	pthreadinfo_add((char *)__func__);
#ifdef _DEBUG_TEST
	++s_PushDevInfoCnt;
#endif

	char url[1024] = {0};	// http://172.16.34.151:8080/publicService/monitorManage/monitorIpcPutInfo.do
	char resp[1024] = {0};
	char yst_no[20] = {0};
	int nLen = 0;
	ipcinfo_t* ipcinfo = ipcinfo_get_param(NULL);

	nLen += snprintf(url + nLen, sizeof(url) - nLen, "http://%s:%d/publicService/monitorManage/monitorIpcPutInfo.do?", 
								HTTP_POST_ADDR, HTTP_POST_PORT);

	jv_ystNum_parse(yst_no, ipcinfo->nDeviceInfo[6], ipcinfo->ystID);

	PARAM_MGR mgr = Http_create_param_mgr();

	Http_add_param_string(mgr, "deviceGuid", info->yst_no[0] ? info->yst_no : yst_no);
	if (bModifyUserInfo)
	{
		Http_add_param_string(mgr, "type", "moduser");
	}
	else
	{
		Http_add_param_string(mgr, "deviceType", info->dev_type);
		Http_add_param_string(mgr, "deviceVersion", info->dev_version);
		Http_add_param_int(mgr, "deviceChannel", info->dev_chans);
		Http_add_param_int(mgr, "sdcardExist", info->sdcard);
		Http_add_param_int(mgr, "connectStyle", gp.bNeedYST ? 0 : 0);
		Http_add_param_int(mgr, "netIntVersion", info->netlibversion);
		Http_add_param_string(mgr, "netStrVersion", info->netlibversionstr);
		Http_add_param_int(mgr, "cloudstorage", info->cloud);
	}

	if (gp.bNeedYST)
		Http_add_param_int(mgr, "timePoint", 1); /* 云视通默认全部支持 */
	else
		Http_add_param_int(mgr, "timePoint", 0); /* 其它暂时默认不支持 */

	int nTryCnt = 0;
	int nSleepTime = 10;
	int ret = -1;
	while(ret <= 0)
	{
		++nTryCnt;
		if (bModifyUserInfo && nTryCnt > 3)
		{
			printf("%s, push user info failed after %d times!!!!!\n", __func__, nTryCnt);
			break;
		}

		// 添加用户名密码信息
		_mbizclient_append_userinfo(mgr);

		// 生成签名和URL
		_mbizclient_append_signature(mgr);
		Http_param_generate_string(mgr, url + nLen, sizeof(url) - nLen);
		printf("\n%s, url=\n%s\n", __func__, url);

		// 一直发送，直到发送成功
		memset(resp, 0, sizeof(resp));
		ret = Http_post_message(url, "", 0, resp, 10);
		printf("%s, ret: %d, retry_cnt: %d, resp=\n%s\n", __func__, ret, nTryCnt, resp);
		if(ret > 0)
		{
			if (0 == _mbizclient_CheckResponse(resp, FALSE))
			{
				break;
			}
			ret = -1;
		}

		sleep(nSleepTime);
		if (nSleepTime < 120)
		{
			nSleepTime += 10;
		}
	}

	Http_destroy_param_mgr(mgr);
	free(Param);
	
#ifdef _DEBUG_TEST
	--s_PushDevInfoCnt;
#endif

	return NULL;
}

/** 
 *	@brief 推送报警信息
 *	@param yst_no	 报警云视通号
 *	@param channel	 报警通道
 *	@param amt		 报警消息类型 0文本消息，1图片上传完成，2视频上传完成。对于云视通方案为0)
 *	@param aguid	 报警GUID
 *	@param atype	 报警类型
 *	@param atime	 报警时间
 *	@param apicture  报警图片地址
 *	@param avideo	 报警视频地址
 *	@return >0 成功，否则失败.
 */
static void* __mbizclient_PushAlarm(void* arg)
{
	BizClientParam* Param = (BizClientParam*)arg;
	PARAM_MGR mgr = Http_create_param_mgr();

	char url[1024] = {0};	// http://172.16.34.151:8080/publicService/monitorManage/monitorSendAlarm.do?deviceGuid=C1035023&channel=2&infoType=1&alarmGuid=C1035023112255&alarmType=14&alarmTime=1468396496&picturePath=&videoPath=&alarmInfo=
	char resp[1024] = {0};
	int nLen = 0;
	
	pthreadinfo_add((char *)__func__);
	
#ifdef _DEBUG_TEST
	++s_PushAlarmCnt;
#endif

	if (PRODUCT_MATCH(PRODUCT_ZHDZ))
	{
		char text[1024] = "";
		char md5val[16] = "";
		char pass[33] = "";

		// yst#video#image#channelNumber#guid#alarmType#alarmTime+salt
		snprintf(text, sizeof(text), "%s#%s#%s#%d#%s#%d#%u%s", 
										Param->yst_no, Param->avideo, Param->apicture, Param->channel, 
										Param->aguid, Param->atype, Param->atime + utl_GetTZDiffSec(), 
										"46cc8885a5d04573a704520adf591abf");

		MD5_CTX ctx;
		MD5Init(&ctx);
		MD5Update(&ctx, (U8*)text, strlen(text));
		MD5Final(&ctx, (U8*)md5val);

		int i = 0;
		for (i = 0; i < 16; ++i)
		{
			sprintf(pass + i * 2, "%02x", md5val[i]);
		}

		// printf("text: %s\nmd5: %s\n", text, pass);

		nLen += snprintf(url + nLen, sizeof(url) - nLen, 
							"http://%s:%d/gateway/yst/alarm?yst=%s&video=%s&image=%s&channelNumber=%d&guid=%s&alarmType=%d&alarmTime=%u&pass=%s", 
							"aviconicshome.aviccxzx.com", 8180, Param->yst_no, Param->avideo, Param->apicture, Param->channel, 
							Param->aguid, Param->atype, Param->atime + utl_GetTZDiffSec(), pass);
	}
	else
	{
		nLen += snprintf(url + nLen, sizeof(url) - nLen, "http://%s:%d/publicService/monitorManage/monitorSendAlarmIPC.do?", 
									HTTP_POST_ADDR, HTTP_POST_PORT);

		Http_add_param_string(mgr, "deviceGuid", Param->yst_no);
		Http_add_param_int(mgr, "channel", Param->channel);
		Http_add_param_int(mgr, "infoType", Param->amt);
		Http_add_param_string(mgr, "alarmGuid", Param->aguid);
		Http_add_param_int(mgr, "alarmType", Param->atype);
		Http_add_param_int(mgr, "alarmTime", Param->atime + utl_GetTZDiffSec());	// atime默认是0时区，这里改成本地时区时间
		Http_add_param_string(mgr, "picturePath", Param->apicture);
		Http_add_param_string(mgr, "videoPath", Param->avideo);
		Http_add_param_string(mgr, "alarmInfo", "");
		Http_add_param_int(mgr, "connectStyle", gp.bNeedYST ? 0 : 0);
	}

	if (PRODUCT_NOT_MATCH(PRODUCT_ZHDZ))
	{
		// 生成签名和URL
		_mbizclient_append_signature(mgr);
		Http_param_generate_string(mgr, url + nLen, sizeof(url) - nLen);
	}
	printf("\n%s, url=\n%s\n", __func__, url);
	
	int nTryCnt = 0;
	int ret = -1;
	while(ret <= 0)
	{
		//尝试发送两次，如果都失败停止发送
		nTryCnt++;
		if(nTryCnt > 2)
		{
			break;
		}

		memset(resp, 0, sizeof(resp));
		ret = Http_post_message(url, "", 0, resp, 10);
		printf("%s, ret: %d, retry_cnt: %d, resp=\n%s\n", __func__, ret, nTryCnt, resp);
		if(ret > 0)
		{
			if (0 == _mbizclient_CheckResponse(resp, TRUE))
			{
				break;
			}
			ret = -1;
		}
		sleep(5);
	}

	Http_destroy_param_mgr(mgr);
	free(Param);
		
#ifdef _DEBUG_TEST
	--s_PushAlarmCnt;
#endif

	return NULL;
}

int mbizclient_PushDevInfo(BizDevInfo *info, BOOL bModifyUserInfo)
{
	if (!hwinfo.bXWNewServer)
	{
		return -1;
	}

	int ret = -1;
	BizClientParam* Param = calloc(1, sizeof(BizClientParam));

	memcpy(&Param->Info, info, sizeof(Param->Info));
	Param->bModUser = bModifyUserInfo;

	ret = pthread_create_detached(NULL, NULL, __mbizclient_PushDevInfo, Param);
	if (ret != 0)
	{
		free(Param);
	}

	return ret;
}

int mbizclient_PushAlarm(char* devname, char* yst_no, U8 channel, U8 amt,
							char* aguid, U8 atype, U32 atime, char* apicture, char* avideo)
{
	if (!hwinfo.bXWNewServer)
	{
		return -1;
	}

	int ret = -1;
	BizClientParam* Param = calloc(1, sizeof(BizClientParam));

	strcpy(Param->devname, devname);
	strcpy(Param->yst_no, yst_no);
	Param->channel = channel;
	Param->amt = amt;
	strcpy(Param->aguid, aguid);
	Param->atype = atype;
	Param->atime = atime;
	strcpy(Param->apicture, apicture);
	strcpy(Param->avideo, avideo);

	ret = pthread_create_detached(NULL, NULL, __mbizclient_PushAlarm, Param);
	if (ret != 0)
	{
		free(Param);
	}

	return ret;
}

int mbizclient_init()
{
	if (hwinfo.bXWNewServer == TRUE)
	{
		pthread_t pid;
		pthread_create(&pid, NULL, (void*)_mbizclient_init_thread, NULL);
		pthread_detach(pid);
		
#ifdef _DEBUG_TEST
		pthread_create_detached(NULL, NULL, _mbizclient_test, NULL);
		// pthread_create_detached(NULL, NULL, _mbizclient_test, NULL);
#endif
	}
	else
		printf("Device is not XiaoWei, biz client no support\n");
	return 0;
}
