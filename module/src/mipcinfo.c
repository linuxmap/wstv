#include <jv_common.h>

#include <mstream.h>
#include <mipcinfo.h>
#include <mlog.h>
#include <mosd.h>
#include <mivp.h>
#include <mrecord.h>
#include <utl_filecfg.h>
#include <utl_timer.h>
#include <utl_iconv.h>
#include <utl_ifconfig.h>
#include <jv_isp.h>
#include <SYSFuncs.h>
#include <sp_connect.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "jv_rtc.h"
#include <osd_server.h>

static ipcinfo_t ipcinfo;
static int ntpTimer = -1;
static int s_reboot_timer = -1;
static BOOL s_bTimeCalibrated = FALSE;
extern hwinfo_t hwinfo;

static BOOL __ipcinfo_reboot_timer(int tid, void *param);

static BOOL __ipcinfo_fd_check(int tid,void *param)
{
	FILE * fd;
	char buf[8] =
	{ 0 };
	int ret;
	char cmd[128];
	sprintf(cmd,"tmp=`ps|grep %s|grep -v grep`;tmp=`echo $tmp|cut -d \" \" -f1`;ls /proc/$tmp/fd|wc -l","sctrl");
	fd = popen(cmd, "r");
	if (fd == NULL)
	{
		printf("popen error ：%s\n", cmd);
		return TRUE;	//popen 失败无法判断连接状态
	}
	ret = fread(buf, 1, 8, fd);
	if (ret <= 0)
	{
		Printf("ERROR: Failed fread\n");
		pclose(fd);
		return TRUE;
	}
	buf[ret-1] = '\0';
	pclose(fd);
	int fdcnt = atoi(buf);
	if(fdcnt>200 && fdcnt <500)
		mlog_write("Handle exception:handle count [%d]",fdcnt);
	else if(fdcnt>=500 && fdcnt<=800)
		mlog_write("Handle exception:[%d].May cause a restart",fdcnt);
	else if(fdcnt>800)
	{
		mlog_write("Handle exception:[%d].Cause a restart",fdcnt);
		utl_system("reboot");
	}
	return TRUE;
}

int ipcinfo_init(void)
{
	memset(&ipcinfo, 0, sizeof(ipcinfo));
	ipcinfo.tz = -123; //随便一个非法值

	//40秒检查一次
	s_reboot_timer = utl_timer_create("reboot timer", 1*10*1000, __ipcinfo_reboot_timer, NULL);

	//句柄数量检查，一个小时检查一次，超出200提示，超出800重启。最多1024个句柄
	utl_timer_create("fd check", 60*60*1000, __ipcinfo_fd_check, NULL);
	return 0;
}

ipcinfo_t *ipcinfo_get_param(ipcinfo_t *param)
{
	char *product;
	char *type;
	int value;
	time_t ttNow = time(NULL);
	struct tm	*pTm	= localtime(&ttNow);

	snprintf(ipcinfo.date, 32, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", pTm->tm_year+1900, pTm->tm_mon+1, pTm->tm_mday, 
		pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
	strcpy(ipcinfo.product, hwinfo.product);
	//添加产品型号
	if(hwinfo.bHomeIPC)
	{
		strcpy(ipcinfo.type, hwinfo.devName);
		if(!strcmp(ipcinfo.type,"H210V2-S") || !strcmp(ipcinfo.type,"H210-S"))
			ipcinfo.type[strlen(ipcinfo.type)-2] = '\0';
	}
	
	snprintf(ipcinfo.version, 20, "%s", IPCAM_VERSION);
	if(ipcinfo.tz != 530)
		ipcinfo.tz = ipcinfo_timezone_get();
	else
		ipcinfo.tz = 530;

	if (param)
	{
		memcpy(param, &ipcinfo, sizeof(ipcinfo_t));
		return param;
	}
	else
		return &ipcinfo;
}

unsigned int ipcinfo_get_model_code()
{
	return ipcinfo.nDeviceInfo[8];
}

//设置时区
//param timeZone 时区值。Localtime- UTC 得到的值
int ipcinfo_timezone_set(int timeZone)
{
	static char localtime[] =
	{
		0x54, 0x5A, 0x69, 0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0C, 0xB0, 0xFE, 0x9A, 0xA0,
		0xC8, 0x5C, 0x01, 0x80, 0xC8, 0xFA, 0x27, 0x70, 0xC9, 0xD5, 0x0E, 0x80, 0xCA, 0xDB, 0x5A, 0xF0,
		0x1E, 0xBA, 0x36, 0x00, 0x1F, 0x69, 0x7F, 0x70, 0x20, 0x7E, 0x68, 0x80, 0x21, 0x49, 0x61, 0x70,
		0x22, 0x5E, 0x4A, 0x80, 0x23, 0x29, 0x43, 0x70, 0x24, 0x47, 0x67, 0x00, 0x25, 0x12, 0x5F, 0xF0,
		0x26, 0x27, 0x49, 0x00, 0x26, 0xF2, 0x41, 0xF0, 0x28, 0x07, 0x2B, 0x00, 0x28, 0xD2, 0x23, 0xF0,
		0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01,
		0x02, 0x00, 0x00, 0x71, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x90, 0x01, 0x04, 0x00, 0x00, 0x70,
		0x80, 0x00, 0x08, 0x4C, 0x4D, 0x54, 0x00, 0x43, 0x44, 0x54, 0x00, 0x43, 0x53, 0x54, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x5A, 0x69, 0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x0C, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0xFE, 0x9A, 0xA0, 0xFF, 0xFF, 0xFF, 0xFF, 0xC8, 0x5C, 0x01,
		0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xC8, 0xFA, 0x27, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0xD5, 0x0E,
		0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0xDB, 0x5A, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x1E, 0xBA, 0x36,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x69, 0x7F, 0x70, 0x00, 0x00, 0x00, 0x00, 0x20, 0x7E, 0x68,
		0x80, 0x00, 0x00, 0x00, 0x00, 0x21, 0x49, 0x61, 0x70, 0x00, 0x00, 0x00, 0x00, 0x22, 0x5E, 0x4A,
		0x80, 0x00, 0x00, 0x00, 0x00, 0x23, 0x29, 0x43, 0x70, 0x00, 0x00, 0x00, 0x00, 0x24, 0x47, 0x67,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x12, 0x5F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x26, 0x27, 0x49,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0xF2, 0x41, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x28, 0x07, 0x2B,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0xD2, 0x23, 0xF0, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
		0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x00, 0x00, 0x71, 0xE0, 0x00, 0x00,
		0x00, 0x00, 0x7E, 0x90, 0x01, 0x04, 0x00, 0x00, 0x70, 0x80, 0x00, 0x08, 0x4C, 0x4D, 0x54, 0x00,
		0x43, 0x44, 0x54, 0x00, 0x43, 0x53, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x43,
		0x53, 0x54, 0x2D, 0x38, 0x0A, 0x0A
	};

	char tz[32];

	//一种设置时区的方式
	FILE *fp;
	fp = fopen("/etc/TZ", "wb");
	if (fp == NULL)
		return -1;
	if(timeZone != 530)
		sprintf(tz, "UTC%d\n", 0-timeZone);
	else
		sprintf(tz, "UTC-0530\n");

	fwrite(tz, 1, strlen(tz), fp);
	fclose(fp);

	//另一种设置时区的方式
	fp = fopen("/etc/localtime", "wb");
	if (fp == NULL)
		return -1;
	int len = sizeof(localtime);
	if(timeZone != 530)
		sprintf(tz, "%d", 0-timeZone);
	else
		sprintf(tz, "-0530");
	strncpy(&localtime[len-4], tz, 4);

	fwrite(localtime, 1, len, fp);
	fclose(fp);

	return 0;
}

//获取时区
//return timeZone 时区值。Localtime- UTC 得到的值
int ipcinfo_timezone_get()
{
	return -(timezone / 3600);
}

IPCType_e ipcinfo_get_type()
{
	static IPCType_e ipcType = IPCTYPE_MAX;
	if (IPCTYPE_MAX == ipcType)
	{
		unsigned int mc = ipcinfo.nDeviceInfo[8];
		if ((0xFFFF & mc) == 0xFFF2)
		{
            ipcType = IPCTYPE_IPDOME;   
		}
		else if ((0xFF00 & mc) == 0xFF00)
		{
			char *product = hwinfo.product;
			if (product && strstr(product, "GC"))
				ipcType = IPCTYPE_JOVISION_GONGCHENGJI;
			else
				ipcType = IPCTYPE_JOVISION;
		}
		else if ((0xFF00 & mc) == 0x4F00)
		{
			// jovision home ipc
			ipcType = IPCTYPE_JOVISION;
		}
		else
			ipcType = IPCTYPE_SW;
	}

	return ipcType;
}

int ipcinfo_get_type2()
{
	int ipcType = 0;
	unsigned int mc = ipcinfo.nDeviceInfo[8];
	if ((0xFF00 & mc) == 0x4F00||(0xFFFF & mc) == 0x4011||(0xFFFF & mc) == 0x2011||(0xFFFF & mc) == 0x4020||(0xFFFF & mc) == 0x4013||(0xFFFF & mc) == 0x4010)
	{
		ipcType = 1;
	}

	return ipcType;
}

BOOL ipcinfo_TimeCalibrated()
{
	return s_bTimeCalibrated;
}

#define JAN_1970		0x83aa7e80		/* 2208988800 1970 - 1900 in seconds */
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4 
#define PREC -6
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

int _zntpGet(char *server, int port)
{
	int ret = 0, sockfd;
	U32 buf[12] = {0};
	int bufLen = sizeof(buf);
	char ntpSrvIP[16] = {0};

	if(!server)
		return -1;

	if(!port)
		port = 123;

	struct addrinfo *pAddrInfo = NULL;
	struct sockaddr_in *pAddr = NULL;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; /* Allow IPv4 */
	hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
	hints.ai_protocol = 0; /* Any protocol */
	hints.ai_socktype = SOCK_DGRAM;

	ret = getaddrinfo(server, NULL, &hints, &pAddrInfo);
	if(ret != 0)
	{
		Printf("get ntp server addr failed\n");
		return -1;
	}

	pAddr = (struct sockaddr_in *)pAddrInfo->ai_addr;
	if(inet_ntop(AF_INET, &pAddr->sin_addr, ntpSrvIP, sizeof(ntpSrvIP)) == NULL)
	{
		freeaddrinfo(pAddrInfo);
		return -1;
	}

	freeaddrinfo(pAddrInfo);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
	{
		Printf("create socket fail\n");
		return -1;
	}

	struct timeval tv_out;
	tv_out.tv_sec = 3;
	tv_out.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

	struct sockaddr_in addr;
	memset(&addr, 0, addr_len);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ntpSrvIP);

	memset(buf, 0, bufLen);
	buf[0] = htonl(( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
				( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
	buf[1] = htonl(1<<16);	/* Root Delay (seconds) */
	buf[2] = htonl(1<<16);	/* Root Dispersion (seconds) */

	struct timeval tv;
	gettimeofday(&tv, NULL);
	buf[10] = htonl(tv.tv_sec + JAN_1970); /* Transmit Timestamp coarse */
	buf[11] = htonl(NTPFRAC(tv.tv_usec));  /* Transmit Timestamp fine	*/

	ret = sendto(sockfd, (char *)buf, bufLen, 0, (struct sockaddr *)&addr, addr_len);
	if(ret != bufLen)
	{
		Printf("send failed\n");
		close(sockfd);
		return -1;
	}

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	fd_set read;
	FD_ZERO(&read);
	FD_SET(sockfd, &read);
	ret = select(sockfd+1, 0, &read, 0, &tv);
	if(ret > 0)
	{
		memset(buf, 0, bufLen);
		ret = recvfrom(sockfd, buf, bufLen, 0, (struct sockaddr *)&addr, &addr_len);
		if(ret == bufLen)
		{
			tv.tv_sec = ntohl(buf[10]) - JAN_1970;
			tv.tv_usec = USEC(ntohl(buf[11]));
			// settimeofday(&tv, NULL);
			if (tv.tv_usec >= 500000)
			{
				++tv.tv_sec;
			}
			ipcinfo_set_time_ex(&tv.tv_sec);
			close(sockfd);
			return 0;
		}
	}

	close(sockfd);
	return -1;
}

static BOOL _ipcinfo_ntp_timer(int tid, void *param);

static void _ipcinfo_ntp_start_timer(int timesec)
{
	if (ntpTimer == -1)
	{
		ntpTimer = utl_timer_create("ntp timer", timesec, _ipcinfo_ntp_timer, NULL);
	}
	else
	{
		utl_timer_reset(ntpTimer, timesec, _ipcinfo_ntp_timer, NULL);
	}

}

static BOOL bNtpRunning = FALSE;

static void *_ipcinfo_ntp_thread(void *arg)
{
	int i = 0;
	int ret;
	static int times = 0;
	
	pthreadinfo_add((char *)__func__);

#define NTP_SERVER_NUM	(sizeof(ntpservers)/sizeof(ntpservers[0]))

	char ntpservers[][64] = {
		"",
		"time.pool.aliyun.com",
		"subitaneous.cpsc.ucalgary.ca",
		"ntp3.fau.de",
		"time.windows.com",
		"tw.ntp.org.cn",
		"kr.ntp.org.cn",
		"jp.ntp.org.cn",
		"de.ntp.org.cn",
		"swisstime.ethz.ch",
		"ntp0.fau.de",
		"time.twc.weather.com",
		"sgp.ntp.org.cn",
		"us.ntp.org.cn",
		"edu.ntp.org.cn",
		"cn.ntp.org.cn",
		"time-b.nist.gov",
		"time-a.nist.gov"
	};
	strcpy(ntpservers[0], ipcinfo.ntpServer);
	
	Printf("start ntp time!!!\n\n");
	for (i = 0; i < NTP_SERVER_NUM; i++)
	{
		ret = _zntpGet(ntpservers[i], 0);
		Printf("ntp time receive %d, %d\n\n", i, ret);
		if (ret == 0)
			break;
	}
	
	if(i < NTP_SERVER_NUM)
	{
		if (ipcinfo.bSntp)
			_ipcinfo_ntp_start_timer(ipcinfo.sntpInterval * 60*60 * 1000);
		else
		{
			utl_timer_destroy(ntpTimer);
			ntpTimer = -1;
			usleep(50*1000);
		}
		// mrecord_stop(0);
		// mrecord_flush(0);
		times = 0;
		// jv_rtc_sync(1);
		s_bTimeCalibrated = TRUE;
		printf("校时成功\n");
	}
	else
	{
		times++;
		if(times > 5)
			times = 5;
		_ipcinfo_ntp_start_timer(60 * times * 1000);
		printf("校时失败\n");
	}

	bNtpRunning = FALSE;
	return NULL;
}

static BOOL _ipcinfo_ntp_timer(int tid, void *param)
{
	if (!utl_ifconfig_net_prepared())
	{
		_ipcinfo_ntp_start_timer(1000);
		return TRUE;
	}

	if(bNtpRunning)
	{
		_ipcinfo_ntp_start_timer(10*1000);
		return TRUE;
	}

	bNtpRunning = TRUE;

	pthread_t id;
	pthread_create_detached(&id, NULL, _ipcinfo_ntp_thread, NULL);

	return TRUE;

	/*
	实际测试，国家授时中心与复旦大学服务器差不多，都很快
	

	时间服务器：						//在公司逐个测试：
	210.72.145.44 //国家授时中心		//不好使

	time.nist.gov (美国)
	ntp.fudan.edu.cn (复旦)　　）（国内用户推荐使用此服务器）
	timekeeper.isi.edu				//不好使
	subitaneous.cpsc.ucalgary.ca	//不好使
	usno.pa-x.dec.com				//不好使
	time.twc.weather.com			//不好使
	swisstime.ethz.ch    			//可以
	ntp0.fau.de						//可以
	ntp3.fau.de						//可以但不如上一个
	time-a.nist.gov					//可以
	time-b.nist.gov					//可以
	time-nw.nist.gov				//不好使
	nist1-sj.glassey.com			//这个也很快
	time.windows.com				//不太好使
	
	*/
    return TRUE;
}

static unsigned int __getruntime(struct timespec *tp)
{
	clock_gettime(CLOCK_MONOTONIC, tp);
	return tp->tv_sec;
}

static BOOL __ipcinfo_reboot_timer(int tid, void *param)
{
	int day;
//	printf("reboot timer day: %d\n", ipcinfo.rebootDay);
	switch (ipcinfo.rebootDay)
	{
	default:
	case REBOOT_TIMER_NEVER:
		return TRUE;
	case REBOOT_TIMER_EVERYDAY:
		day = -1;
		break;
	case REBOOT_TIMER_EVERYSUNDAY:
		day = 0;
		break;
	case REBOOT_TIMER_EVERYMOUNDAY:
		day = 1;
		break;
	case REBOOT_TIMER_EVERYTUNESDAY:
		day = 2;
		break;
	case REBOOT_TIMER_EVERYWEDNESDAY:
		day = 3;
		break;
	case REBOOT_TIMER_EVERYTHURSDAY:
		day = 4;
		break;
	case REBOOT_TIMER_EVERYFRIDAY:
		day = 5;
		break;
	case REBOOT_TIMER_EVERYSATURDAY:
		day = 6;
		break;
	}

	//开机5分钟之内不重启
	struct timespec tp;
	int runsec = __getruntime(&tp);
	if(runsec <= 300)
	{
		Printf("IPC Linux OS run time ： %d < 300s\n",runsec);
		return TRUE;
	}

	struct tm tm;
	time_t tt = time(NULL);
	localtime_r(&tt, &tm);

	if (day == -1 || day == tm.tm_wday)
	{
		if ((ipcinfo.rebootHour==tm.tm_hour) && (tm.tm_min==0) 
		    && (tm.tm_sec<20))  //十秒检查一次，避免重启多次
		{
			SYSFuncs_reboot();
		}
	}

	return TRUE;
}

int multiosd_process(ipcinfo_t *param)
{
	char buf[2048];
	char osdTextTotal[512];
	char osdTextTmp[64];
	char osdsize[16];
	char osdpos[64];
	int count = 0;
	int i = 0;
	memset(buf, 0, sizeof(buf));
	memset(osdTextTotal, 0, sizeof(osdTextTotal));
	memset(osdTextTmp, 0, sizeof(osdTextTmp));
	memset(osdsize, 0, sizeof(osdsize));
	memset(osdpos, 0, sizeof(osdpos));
	if(param != NULL)
	{
		for(i=0; i<8; i++)
		{
			if(strlen(param->osdText[i]) > 0)
			{
				sprintf(osdTextTmp, "\"%s\",", param->osdText[i]);
				strcat(osdTextTotal, osdTextTmp);
			}
		}
		osdTextTotal[strlen(osdTextTotal)-1] = '\0';
		sprintf(osdsize, "\"FontSize\": %d,", param->osdSize);
		sprintf(osdpos, "\"RelativePos\": [%d,%d,%d,%d]", param->osdX, param->osdY, param->endX, param->endY);
		sprintf(buf, "%s%s%s%s%s%s", "{\"Login\": {\"EncryptType\": \"None\",\"LoginType\": \"DVRIP-Web\",\"PassWord\": \"12345\",\"UserName\": \"admin\"},\"strEnc\": \"GB2312\",\"OSDInfo\": [{\"Info\": [", osdTextTotal, "],\"OSDInfoWidget\": {\"BackColor\": \"0x00000000\",\"EncodeBlend\": true,\"FrontColor\":\"0xFFFF0000\",\"PreviewBlend\": true,", osdsize, osdpos, "}}]}");
		process_osd_msg(buf, NULL, 0);
	}
	return 0;
}

int ipcinfo_set_param(ipcinfo_t *param)
{
#if 0x0
	printf("\n\n-----------------current param is:---------------------------\n");
	printf("type=%s\n", ipcinfo.type);
	printf("product=%s\n", ipcinfo.product);
	printf("version=%s\n", ipcinfo.version);
	printf("acDevName=%s\n", ipcinfo.acDevName);
	printf("nickName=%s\n", ipcinfo.nickName);
	printf("ntpServer=%s\n", ipcinfo.ntpServer);
	printf("\n\n-----------------new param is:---------------------------\n");
	printf("type=%s\n", param->type);
	printf("product=%s\n", param->product);
	printf("version=%s\n", param->version);
	printf("acDevName=%s\n", param->acDevName);
	printf("nickName=%s\n", param->nickName);
	printf("ntpServer=%s\n", param->ntpServer);
	printf("--------------------------------------------\n\n");
#endif
	int phoneServerFlag = 0;
	int timedelay ;
	static int bFirst = 1;//第一次时，延迟执行，以便留出时间获取IP，准备网络
	static char ntpFirst = 1;	//去掉RTC后，上电默认ntp校时

	if (bFirst)
	{
		bFirst = 0;
		timedelay = 60000;
	}
	else
		timedelay = 1000;

	//有些不能设置的参数
	if (ipcinfo.sn != 0)
	{
		param->sn = ipcinfo.sn;
	}
	if (ipcinfo.ystID != 0)
	{
		param->ystID = ipcinfo.ystID;
	}
	if (ipcinfo.nDeviceInfo[6] != 0)
	{
		memcpy(param->nDeviceInfo , ipcinfo.nDeviceInfo, sizeof(ipcinfo.nDeviceInfo));
	}


	if (strcmp(ipcinfo.acDevName, param->acDevName) != 0)
	{
		utl_iconv_gb2312_fix(param->acDevName, sizeof(param->acDevName));
		int i = 0;
		mchnosd_attr attr;
		param->needFlushOsd = TRUE;
		
		mchnosd_set_name(-1, param->acDevName); 
		mivp_restart(0);
		mlog_write("Device Name Changed: [%s]", param->acDevName);
	}
	
	if (memcmp(ipcinfo.osdText, param->osdText, sizeof(ipcinfo.osdText)) != 0
		|| ipcinfo.osdX != param->osdX
		|| ipcinfo.osdY != param->osdY
		|| ipcinfo.osdSize != param->osdSize
		|| ipcinfo.osdPosition != param->osdPosition
		|| ipcinfo.osdAlignment != param->osdAlignment || param->needFlushOsd)
	{
		int i = 0;
		param->needFlushOsd = FALSE;
		if(ipcinfo.osdPosition != param->osdPosition || ipcinfo.osdAlignment != param->osdAlignment)
		{
			multiple_set_param(param->osdPosition,param->osdAlignment);
		}
		for(i=0; i<8; i++)
		{
			utl_iconv_gb2312_fix(param->osdText[i], 49);
			param->osdText[i][48] = '\0';
		}
		multiosd_process(param);
	}
	
	if (ipcinfo.nLanguage != param->nLanguage)
	{
		char *str;
		ipcinfo.nLanguage = param->nLanguage;
		if (param->nLanguage == LANGUAGE_EN)
			str = "English";
		else
			str = "中文";
		mlog_write("Device Language Changed: [%s]", str);

		//更换时间格式
		int i;
		mchnosd_attr osd;
		for (i=0;i<HWINFO_STREAM_CNT;i++)
		{
			if (param->nLanguage == LANGUAGE_CN)
			{
				strcpy(osd.timeFormat, "YYYY-MM-DD hh:mm:ss");
			}
			else
			{
				strcpy(osd.timeFormat, "MM/DD/YYYY hh:mm:ss");
			}
		}
	}

	//printf("param->tz%d ipcinfo.tz%d\n",param->tz,ipcinfo.tz);
	//printf("param->bSntp%d ipcinfo.bSntp%d\n",param->bSntp,ipcinfo.bSntp);
	if (param->tz != ipcinfo.tz)
	{
		ipcinfo_timezone_set(param->tz);
		ipcinfo.tz=param->tz;
	}
	if (ipcinfo.bSntp != param->bSntp
		|| ipcinfo.sntpInterval != param->sntpInterval
		|| 0 != strcmp(ipcinfo.ntpServer, param->ntpServer))
	{
		ipcinfo.bSntp = param->bSntp;
		ipcinfo.sntpInterval = param->sntpInterval;
		memcpy(ipcinfo.ntpServer, param->ntpServer, sizeof(ipcinfo.ntpServer));
		if (ntpFirst == 1)
		{
			ntpFirst = 0;
			_ipcinfo_ntp_start_timer(timedelay);
		}
		else
		{
			if (ipcinfo.bSntp)
			{
				_ipcinfo_ntp_start_timer(timedelay);
			}
			else if (ntpTimer != -1)
			{
				utl_timer_destroy(ntpTimer);
				ntpTimer = -1;
				usleep(50*1000);
	//			utl_system("killall msntp");
			}
		}
	}
	
	memcpy(&ipcinfo, param, sizeof(ipcinfo_t));
	//ipcinfo.sn = ipcinfo.nDeviceInfo[4];
	return 0;
}

/** 
 *@brief 设置时间
 *@param date 要设置的时间，其格式为YYYY-MM-DD hh:mm:ss
 *
 */
int ipcinfo_set_time(char *date)
{
	struct tm	stTm, stOldTm;

	sscanf(date, "%d-%d-%d-%d:%d:%d", &stTm.tm_year, &stTm.tm_mon, &stTm.tm_mday, 
		&stTm.tm_hour, &stTm.tm_min, &stTm.tm_sec);
	stTm.tm_year	-= 1900;
	stTm.tm_mon -= 1;
	//验证输入的日期是否正确
	memcpy(&stOldTm, &stTm, sizeof(stTm));	  
	time_t now = mktime(&stTm);
	if (   stTm.tm_year != stOldTm.tm_year 
		|| stTm.tm_mon	!= stOldTm.tm_mon 
		|| stTm.tm_mday != stOldTm.tm_mday
		|| stTm.tm_hour != stOldTm.tm_hour 
		|| stTm.tm_min	!= stOldTm.tm_min 
		|| stTm.tm_sec	!= stOldTm.tm_sec
		|| now == -1
		)
	{
		Printf("ERROR: time format error!\n");
		return -1; 	   
	}
	ipcinfo_set_time_ex(&now);

	return 0;
}

int ipcinfo_set_time_ex(time_t *newtime)
{
	time_t now = time(NULL);
	int timediff = abs(now - *newtime);

	// 相差1s以内不校时，避免频繁校时
	if (timediff <= 1)
	{
		return -1;
	}

	// 暂时做一下限制
	// 只有设置的时间与系统当前时间相差至少30s时，才重新打包录像文件，
	// 以免发生像JNVR类的客户端一直设置时间导致无限创建新录像文件的问题
	if (timediff >= 30)
	{
		mrecord_stop(0);
		stime(newtime);//设置时间
		mrecord_flush(0);
	}
	else
	{
		stime(newtime);//设置时间
	}

	//utl_system("hwclock -w -u&");//将新的时间写入到硬件实时时钟当中
	jv_rtc_sync(1);
	mlog_write("set time: succeed");
	return 0;
}

