#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <netinet/ip_icmp.h>
#include <jv_common.h>
#include "utl_filecfg.h"
#include "utl_ifconfig.h"
#include "mipcinfo.h"
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h>
#include <netdb.h>
#include <ctype.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/sockios.h> 
#include "utl_timer.h"
#include "JvServer.h"
#include "onvif-main.h"
#include "utl_net_lan.h"
#include <mlog.h>
#include <dlfcn.h>
#include "mioctrl.h"
#include "maudio.h"
#include "wireless.h"
#include "mvoicedec.h"
#include "smartconfig.h"
#include "montconfig.h"
#include "utl_common.h"
#include "SYSFuncs.h"
#include "iw_80211_scan.h"


#define NETWORK_INTERFACES_FILE			"/etc/network/interface.cfg"
#define NETWORK_WIFI_PASSWD_FILE		"/etc/network/wifipasswd.cfg"
#define NETWORK_WIFI_CONFIG_FILE		"/dev/wpa_supplicant.conf"
#define NETWORK_WIFI_SAVE_DAT			"/etc/network/wifisave.dat"
#define NETWORK_HOSTAPD_CONF_FILE		"/dev/hostapd.conf"
#define NETWORK_UDHCPD_CONF_FILE		"/etc/network/udhcpd.conf"
#define NETWORK_MAC_FILE				CONFIG_PATH"/network/mac.cfg"
#define NETWORK_WIFI_DRIVER_PATH		"/home/wifi/"
#define NETWORK_RALINK7601_AP_FILE		"/dev/RT7601AP.dat"

#define MAX_WIFI_CNT	128
static wifiap_t wifilist[MAX_WIFI_CNT];
static wifiap_t sPowerAPlist[MAX_WIFI_CNT];
static wifi_mode_e WifiDrvierMode = WIFI_MODE_STA;
static int wifi_setting = 0;
static void __utl_reset_check();
int get_netlink_status(const char *if_name);
int utlGetWifiStatus(char * dev);
static int _getGateWay(char *iface,char *gwout);
static int checkPing(int packet_no, char *pingAddr);
pthread_mutex_t ap_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct {
	char iface[12];
	char inet[8];
	char ip[20];
	char netmask[20];
	char gateway[20];
	char dns[20];
}network_interface_t;

static WifiModelType sWifiModel = WIFI_MODEL_UNKNOWN;
static char *__fix(char *str)
{
	char *ptr = str;
	char *t;

	while(ptr && *ptr == ' ')
		ptr++;
	t = ptr;
	while(t && *t)
	{
		if (*t == '\n' || *t == '\r')
		{
			*t = '\0';
			break;
		}
		t++;
	}
	return ptr;
}

static int _utl_write_interface(char *iface, char *inet, char *ip, char *netmask, char *gateway, char *dns)
{
	FILE *fp;

	fp = fopen(NETWORK_INTERFACES_FILE, "wb");
	if (fp == NULL)
	{
		Printf("fopen %s failed!\n", NETWORK_INTERFACES_FILE);
		return -1;
	}

	printf("file fd:%d\n", fileno(fp));
#if 1
	if (!iface) iface = "eth0";
	if (!inet) inet = "dhcp";
	if (!ip) ip = "192.168.1.254";
	if (!netmask) netmask = "255.255.255.0";
	if (!gateway) gateway = "192.168.1.1";
	if (!dns) dns = "8.8.8.8";
#else
	eth_t eth;
	utl_ifconfig_build_attr("eth0",&eth, FALSE);
	
	if (!iface) iface = "eth0";
	if (!inet) inet = "dhcp";
	if (!ip) ip = eth.addr;
	if (!netmask) netmask = eth.mask;
	if (!gateway) gateway = eth.gateway;
	if (!dns) dns = eth.dns;
#endif

	fprintf(fp, "#iface is the name of the dev, such as eth0/ppp0/wlan0\n");
	fprintf(fp, "iface=%s\n", iface);
	fprintf(fp, "#inet=static/dhcp/ppp/wifi\n");
	fprintf(fp, "inet=%s\n", inet);
	fprintf(fp, "\n");
	fprintf(fp, "ip=%s\n", ip);
	fprintf(fp, "netmask=%s\n", netmask);
	fprintf(fp, "gateway=%s\n", gateway);
	fprintf(fp, "dns=%s\n", dns);

	fclose(fp);
	utl_fcfg_close(NETWORK_INTERFACES_FILE);

	return 0;
}

static int _utl_read_interface(char *iface, char *inet, char *ip, char *netmask, char *gateway, char *dns)
{
	keyvalue_t *kv;

#define GET_VALUE(name)\
				do{\
					if (name && strcmp(#name, kv->key) == 0)	\
					{\
						strcpy(name, kv->value);\
						continue;\
					}\
				}\
				while(0)

	int cur = 0;
	utl_fcfg_start_getnext(NETWORK_INTERFACES_FILE);
	while(1)
	{
		kv = utl_fcfg_get_next(NETWORK_INTERFACES_FILE, &cur);
		if (kv == NULL)
			break;
		//Printf("%s=%s\n", kv->key, kv->value);
		GET_VALUE(iface);
		GET_VALUE(inet);
		GET_VALUE(ip);
		GET_VALUE(netmask);
		GET_VALUE(gateway);
		GET_VALUE(dns);
	}
	utl_fcfg_end_getnext(NETWORK_INTERFACES_FILE);

	return 0;
}

/**
 *@brief 从文件中读取IP信息
 * 用socket获取IP的方式，多次设置IP时会有死机的情况，原因未知
 * 为避免之，特别在networkcfg.sh中，将IP保存在文件:/var/run/jvnetstatus中
 *
 *@retval 0 成功
 *@retval -1 失败
 *
 */
static int _utl_ifconfig_build_with_file(char *ifname, eth_t *attr)
{
	int ret = -1;
	FILE *ip;
	char *item;
	static char addrInfo[2048];
	int i;

	ip = fopen("/var/run/jvnetstatus", "r");
	if (ip == NULL)
		return -1;
	ret = fread(addrInfo, 1, 2048-1, ip);
	if (ret == 0)
	{
		Printf("No ifname: %s\n", ifname);
		fclose(ip);
		return -1;
	}
	addrInfo[ret] = '\0';
	//ifname
	item = strstr(addrInfo, ifname);
	if (item == NULL)
	{
		Printf("No ifname %s info\n", ifname);
		fclose(ip);
		return -1;
	}
	//mac
	item = strstr(addrInfo, "HWaddr ");
	if (item != NULL)
	{
		item += strlen("HWaddr ");
		for (i=0;i<sizeof(attr->mac);i++)
		{
			if (item[i] == '\0' || item[i] == ' ' || item[i] == '\t' || item[i] == '\r' || item[i] == '\n')
			{
				attr->mac[i] = '\0';
				break;
			}
			attr->mac[i] = item[i];
		}
	}
	//ip
	item = strstr(addrInfo, "inet addr:");
	if (item != NULL)
	{
		item += strlen("inet addr:");
		for (i=0;i<sizeof(attr->addr);i++)
		{
			if (item[i] == '\0' || item[i] == ' ' || item[i] == '\t' || item[i] == '\r' || item[i] == '\n')
			{
				attr->addr[i] = '\0';
				break;
			}
			attr->addr[i] = item[i];
		}
	}
	//ip
	item = strstr(addrInfo, "Mask:");
	if (item != NULL)
	{
		item += strlen("Mask:");
		for (i=0;i<sizeof(attr->mask);i++)
		{
			if (item[i] == '\0' || item[i] == ' ' || item[i] == '\t' || item[i] == '\r' || item[i] == '\n')
			{
				attr->mask[i] = '\0';
				break;
			}
			attr->mask[i] = item[i];
		}
	}
	fclose(ip);
	return 0;
}

/**
 *@brief 获取指定网络设备名的IP信息
 *
 *@param ifname 网络设备名，如eth0/ra0
 *@param attr IP信息
 *
 */
int utl_ifconfig_build_attr(char *ifname, eth_t *attr, BOOL bRefresh)
{
	struct ifreq temp;
	struct sockaddr_in *myaddr;
	int fd;
//	int ret;
	FILE *fp;
	char buf[128]={0};
	unsigned int ipvalue,maskvalue;

	if(!ifname || !attr || !strlen(ifname))
	{
		return -1;
	}

	memset(attr, 0, sizeof(eth_t));
	strcpy(attr->name, ifname);

	//每次都重新获取实际的IP地址
	bRefresh = TRUE;
	if (bRefresh || _utl_ifconfig_build_with_file(ifname, attr) != 0)
	{
		if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
		{
			Printf("Build socket failed.\n");
			return -1;
		}

		strcpy(temp.ifr_name,ifname);
		Printf("  @@@@@@@@@@@ Net interface    name=%s\n   ",ifname);
		//addr
		if(ioctl(fd, SIOCGIFADDR, &temp) >= 0)
		{
			myaddr = (struct sockaddr_in *)&(temp.ifr_addr);
			inet_ntop(AF_INET, &myaddr->sin_addr, attr->addr, sizeof(attr->addr));
			ipvalue = myaddr->sin_addr.s_addr;
		}
		else
			Printf("utl_ifconfig_build_attr SIOCGIFADDR failed\n");

		//mask
		if(ioctl(fd, SIOCGIFNETMASK, &temp) >= 0)
		{
			myaddr = (struct sockaddr_in *)&(temp.ifr_netmask);
			inet_ntop(AF_INET, &myaddr->sin_addr, attr->mask, sizeof(attr->mask));
			maskvalue = myaddr->sin_addr.s_addr;
//			printf("--------------------------------netmask:%x\n",maskvalue);
		}
		else
			Printf("utl_ifconfig_build_attr SIOCGIFNETMASK failed\n");

		//mac
		if(!strcmp(ifname,"ppp0"))
			strcpy(temp.ifr_name,"eth0");
		if(ioctl(fd, SIOCGIFHWADDR, &temp) >= 0)
		{
			sprintf(attr->mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			        (U8)temp.ifr_hwaddr.sa_data[0],
			        (U8)temp.ifr_hwaddr.sa_data[1],
			        (U8)temp.ifr_hwaddr.sa_data[2],
			        (U8)temp.ifr_hwaddr.sa_data[3],
			        (U8)temp.ifr_hwaddr.sa_data[4],
			        (U8)temp.ifr_hwaddr.sa_data[5]);
		}
		else
			Printf("utl_ifconfig_build_attr SIOCGIFHWADDR failed\n");
		close(fd);
	}
	
	//gateway
	fp = fopen("/proc/net/route", "r");
	if (fp != NULL)
	{
		char tname[16];
		unsigned long dest_addr, gate_addr=0;
		struct in_addr in;
		/* Skip title line */
		fgets(buf, sizeof(buf), fp);
		while (fgets(buf, sizeof(buf), fp))
		{
			if (sscanf(buf, "%s\t%lX\t%lX", tname, &dest_addr, &gate_addr) != 3 || dest_addr != 0)
			{
				continue;
			}
			if ((ipvalue & maskvalue) == (gate_addr & maskvalue))
			{
//				printf("find one\n");
				break;
			}
			else
			{
//				printf("not equal: 0x%x != 0x%x\n", gate_addr, ipvalue);
			}
		}

		in.s_addr = gate_addr;      //不能用htonl(gate_addr),文件内容已经是网络字节序了
		inet_ntop(AF_INET, &in, attr->gateway, sizeof(attr->gateway));
		fclose(fp);
	}

	//dns
	fp = fopen("/etc/resolv.conf", "r");
	if (fp != NULL)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if(1 == sscanf(buf, "nameserver %s", attr->dns))
			{
				break;
			}
		}
		fclose(fp);
	}

	_utl_read_interface(NULL, buf, NULL, NULL, NULL, NULL);
	if (strcmp(buf, "dhcp") == 0)
		attr->bDHCP = TRUE;
	else
		attr->bDHCP = FALSE;

	return 0;
}

/**
 *@brief 保存wifi热点的密码
 *
 *文件格式为：name=passwd，每行一个
 *
 */
static void _utl_wifi_write_passwd(wifiap_t *list, wifiap_t *ap)
{
	int i;
	FILE *fp;
	fp = fopen(NETWORK_WIFI_PASSWD_FILE, "w");
	if(fp != NULL)
	{
		for (i=0;list[i].name[0] != '\0'; i++)
		{
			if (strcmp(list[i].name, ap->name) == 0)
			{
				list[i] = *ap;
			}
			if (list[i].passwd[0] != '\0')
			{
				fprintf(fp, "%s=%s\n", list[i].name, list[i].passwd);
			}
		}
		fclose(fp);
	}
}

/**
 *@brief 读取wifi热点的密码
 *
 *文件格式为：name=passwd，每行一个
 */
static void _utl_wifi_read_passwd(wifiap_t *list)
{
	int i;
	char *key;
	char *value;
	char buf[256];
	FILE *fp;
	fp = fopen(NETWORK_WIFI_PASSWD_FILE, "r");

	if (fp == NULL)
	{
		Printf("fopen %s failed!\n", NETWORK_INTERFACES_FILE);
		return ;
	}

	while (fgets(buf, sizeof(buf), fp))
	{
		if (buf[0] == '#')
			continue;

		key = strtok(buf, "=");
		value = strtok(NULL, "\r");
		__fix(value);
		if (key == NULL || value == NULL)
			continue;
		printf("wifi list name: [%s], passwd: [%s]\n", key, value);

		for (i=0;list[i].name[0] != '\0'; i++)
		{
			if (strcmp(list[i].name, key) == 0)
			{
				strncpy(list[i].passwd, value, sizeof(list[i].passwd));
				break;
			}
		}		
	}
	fclose(fp);
}

wifiap_t* utl_ifconfig_wifi_power_list_ap()
{
	return sPowerAPlist;
}

/**
 *@brief 获取WIFI热点列表
 *
 *@param bResearch 是否强制重新扫描
 *
 *@return 0 成功，< 0 失败
 *
 */
wifiap_t *utl_ifconfig_wifi_list_ap(int bResearch)
{
	int cnt;
	wifiap_t w;
	
	if (!bResearch)
		return wifilist;
	
	if (sWifiModel == WIFI_MODEL_M88WI6700)
	{
		utl_system("ifconfig wlan0 up");
		sleep(1);
		cnt = MAX_WIFI_CNT;
		iw_80211_scan("wlan0", wifilist, &cnt);
	}
	else
	{
		utl_system("ifconfig wlan0 up");
		sleep(1);
		cnt = iwlist_get_ap_list(wifilist, "wlan0", MAX_WIFI_CNT);
		// 8188/8192的quality字段不准确，需要使用level，因此这里转换一下
		if (sWifiModel == WIFI_MODEL_REALTEK8188 ||
			sWifiModel == WIFI_MODEL_REALTEK8192)
		{
			int i = 0;

			for (i = 0; i < cnt; ++i)
			{
				wifilist[i].quality = wifilist[i].iestat[4];
			}
		}
	}
	
	if (cnt == 0)
		return NULL;
	wifilist[cnt].name[0] = '\0';
	_utl_wifi_read_passwd(wifilist);
	return wifilist;
}

//获取WIFI热点列表
wifiap_t *utl_ifconfig_wifi_get_ap()
{
	wifiap_t *list;
	if (hwinfo.bHomeIPC == 1)
	{
		if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
		{
			list = utl_ifconfig_wifi_list_ap(TRUE);
		}
		else
		{
			list = utl_ifconfig_wifi_power_list_ap();
		}
	}
	else
	{
		if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
		{
			list = utl_ifconfig_wifi_list_ap(TRUE);
		}
		else
		{
			list = utl_ifconfig_wifi_power_list_ap();
		}
	}

	return list;
}

/**
 *@brief 计算wifi热点列表数量
 *
 *@param list 要计算的列表
 *
 *@return 热点个数个数
 *
 */
int utl_ifconfig_wifi_list_cnt(wifiap_t *list)
{
	int i;
	if (list == NULL)
		return 0;
	for (i=0;list[i].name[0] != '\0'; i++)
	{
		if (list[i].name == '\0')
		{
			break;
		}
	}
	return i;
}

//1:字符串；0:hex
static int utl_ifconfig_getWifiKeyType(int encryType, char *key)
{
    int n = strlen(key);
    switch(encryType)
    {
        case 1:
            if((n == 5) ||(n == 13))
                return 1;
            if((n == 10) ||(n == 26))
                return 0;
            return -1;
        case 2:
		case 3:
            if((n <= 63) && (n >= 8))
                return 1;
            if(n == 64)
                return 0;
            return -1;
        default:
            return 1;
    }
}

/**
 * @brief 保存wifi的配置文件         原来是在wifi_connect里边，但是保存文件之后重启不需要连接
 *                             所以单独拿出来。
 * @param ap要连接的ap信息
 * @return 0成功
 */
int utl_ifconfig_wifi_conf_save(wifiap_t *ap)
{
	int fd = -1;
	char buf[512] = {0x00};
	char keybuf[128] = {0x00};
	int keyType = utl_ifconfig_getWifiKeyType(ap->iestat[1], ap->passwd);
	if (keyType == 1)
    {
        /*字符串*/
       if(ap->passwd[0])
          sprintf(keybuf, "\"%s\"", ap->passwd);
       else
          sprintf(keybuf, "\"12345678\"");
           
    }
    else
    {
        /*数字*/
        strcpy(keybuf, ap->passwd);
    }

	//记录下密码
	fd = open(NETWORK_WIFI_CONFIG_FILE, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("open config err\n");
		return -1;
	}
	
	strcpy(buf, "ctrl_interface=/var/run/wpa_supplicant\n");
	strcat(buf, "update_config=1\n");
	//wpa网络配置文件生成
	if (ap->iestat[0] == 3 || ap->iestat[0] == 4)		//wpa网络
	{
		//添加proto=WPA RSN,这样可以兼容WPA,WPA2，只有WPA时有的路由器连接不上，lk20131205.
		strcat(buf, "network={\nssid=\"");
		strcat(buf, ap->name);
		strcat(buf,	"\"\nscan_ssid=1\nkey_mgmt=WPA-PSK\nproto=WPA RSN\npairwise=TKIP CCMP\npsk=");
		strcat(buf, keybuf);
		strcat(buf, "\n}");
		write(fd, buf, strlen(buf));
	}
	else if(ap->iestat[0] == 0 || ap->iestat[0] == 1 || ap->iestat[0] == 2)//wep网络
	{
		strcat(buf, "network={\nssid=\"");
		strcat(buf, ap->name);
		strcat(buf, "\"\nscan_ssid=1\nkey_mgmt=NONE\n");
		if (ap->iestat[0] == 1)
		{
			//shared
			strcat(buf, "auth_alg=SHARED\n");
		}
		strcat(buf, "wep_key0=");
		strcat(buf, keybuf);
		strcat(buf, "\n}");
		
		write(fd, buf, strlen(buf));
	}
	else//明文网络配置文件生成
	{
		strcat(buf, "network={\nssid=\"");
		strcat(buf, ap->name);
		strcat(buf, "\"\nscan_ssid=1\nkey_mgmt=NONE\n}");
		write(fd, buf, strlen(buf));
	}
	close(fd);
	
	_utl_wifi_write_passwd(wifilist, ap);
	return 0;
}

int utl_ifconfig_wifi_save_remove()
{
	return remove(NETWORK_WIFI_SAVE_DAT);
}

WifiModelType utl_ifconfig_wifi_get_model()
{
	if (sWifiModel != WIFI_MODEL_UNKNOWN)
		return sWifiModel;

	if (access(NETWORK_WIFI_DRIVER_PATH"8188eu.ko", F_OK) == 0)
	{
		sWifiModel = WIFI_MODEL_REALTEK8188;
	}
	else if (access(NETWORK_WIFI_DRIVER_PATH"8192eu.ko", F_OK) == 0)
	{
		sWifiModel = WIFI_MODEL_REALTEK8192;
	}
	else if (access(NETWORK_WIFI_DRIVER_PATH"mt7601Usta.ko", F_OK) == 0)
	{
		sWifiModel = WIFI_MODEL_RALINK7601;
	}
	else if (access(NETWORK_WIFI_DRIVER_PATH"m88wi6700u.ko", F_OK) == 0)
	{
		sWifiModel = WIFI_MODEL_M88WI6700;
	}
	else
	{
		sWifiModel = WIFI_MODEL_NONE;
	}

	return sWifiModel;
}

static void __wifi_STA_stop()
{
	utl_system("killall wpa_supplicant;killall udhcpc;");
}

/*******************************smart connect******************************/
static BOOL smart_connecting = FALSE;
static BOOL smart_closed = FALSE;
static BOOL smart_recvsetting = FALSE;

static void utl_ifconfig_wifi_smart_connect_start()
{
	smart_connecting = TRUE;
	utl_ifconfig_wifi_start_sta();

	switch (sWifiModel)
	{
	case WIFI_MODEL_RALINK7601:
		{
			utl_system("ifconfig wlan0 up");
			utl_WaitTimeout(smart_closed, 1000);
			if (smart_closed)
			{
				return;
			}
			utl_system("echo start > /proc/elian");
		}
		break;
	case WIFI_MODEL_M88WI6700:
		{
			utl_system("ifconfig wlan0 up");
			utl_WaitTimeout(smart_closed, 1000);
			if (smart_closed)
			{
				return;
			}

			// 澜起Wifi，还有时候会开启不了，再加个重启。。。
			int ret = montconfig_start("wlan0");
			if (ret)
			{
				printf("%s, start mont config failed, now reboot!\n", __func__);
				SYSFuncs_reboot();
				sleep(100);		// 阻塞住，等待重启
			}
		}
		break;
	default:
		break;
	}
}

static void utl_ifconfig_wifi_smart_connect_stop()
{
	switch (sWifiModel)
	{
	case WIFI_MODEL_RALINK7601:
		{
			utl_system("echo stop > /proc/elian");
		}
		break;
	case WIFI_MODEL_M88WI6700:
		{
			// 澜起Wifi，总是有时候会停不了，只好加个重启
			int ret = montconfig_stop();
			if (ret)
			{
				printf("%s, stop mont config failed, now reboot!\n", __func__);
				SYSFuncs_reboot();
				sleep(100);		// 阻塞住，等待重启
			}
		}
		break;
	default:
		break;
	}
}

static int utl_ifconfig_wifi_smart_connect_getresult(char *result, int len)
{
	switch (sWifiModel)
	{
	case WIFI_MODEL_RALINK7601:
		{
			int fd = -1;

			fd = open("/proc/elian", O_RDONLY);
			if (fd == -1)
			{
				perror("open smart_connection error");
				return -1;
			}
			read(fd, result, len);
			close(fd);
		}
		break;
	case WIFI_MODEL_M88WI6700:
		strncpy(result, montconfig_getconfig(), len);
		result[len - 1] = '\0';
		break;
	default:
		break;
	}

	return 0;
}

static int utl_ifconfig_wifi_smart_connect_getvalue(char *src, char *dst, char *value, char end)
{
	char* pPos = strstr(src, dst);

	if (!pPos)
		return -1;

	pPos += strlen(dst);

	while (pPos && *pPos && *pPos != end)
	{
		*value++ = *pPos++;
		if (*pPos == '\0' || *pPos == end)
		{
			return 0;
		}
	}

	return -1;
}

static int utl_ifconfig_wifi_smart_connect_get(wifiap_t *ap)
{
	char buf[128] = {0x00};
	char* ssidflag = "ssid=";
	char* pwdflag = "pwd=";
	char endflag = ',';

	utl_ifconfig_wifi_smart_connect_getresult(buf, sizeof(buf));
	// printf("\n\n%s\n", buf);

	if (strstr(buf, ssidflag) == NULL)
	{
		ssidflag = "SSID	: ";
		pwdflag = "PASSWORD	: ";
		endflag = '\n';
	}

	if (utl_ifconfig_wifi_smart_connect_getvalue(buf, ssidflag, ap->name, endflag) != 0)
	{
		printf("SSID NULL\n");
		return -1;
	}
	if (utl_ifconfig_wifi_smart_connect_getvalue(buf, pwdflag, ap->passwd, endflag) != 0)
	{
		printf("PASSWORD NULL\n");
	}
	printf("  SSID %s\n  PWD %s\n", ap->name, ap->passwd);

	return 0;
}

static void utl_ifconfig_wifi_smart_connect_thread(void* para)
{
	int i = 0;
	int j = 0;
	int ret = -1;
	wifiap_t ap;
	wifiap_t * ap_t = NULL;
	int k = 0;
	wifiap_t w;
	static int s_config_cnt;

	pthreadinfo_add((char *)__func__);

	wifi_setting = 1;

	memset(&w, 0, sizeof(w));
	utl_ifconfig_wifi_conf_save(&w);
	
	net_deinit();
	while(!smart_closed)
	{
		mio_set_net_st(DEV_ST_WIFI_SETTING);

		printf("%s, =====start smart config: %d\n", __func__, ++s_config_cnt);
		// 开启配置前，退出wpa_supplicant和udhcpc，避免与智联配置中对wifi节点的操作发生冲突
		__wifi_STA_stop();
		utl_ifconfig_wifi_smart_connect_start();
		for (j = 0; j < 10; j++)
		{
			memset(&ap, 0, sizeof(ap));

			utl_WaitTimeout(smart_closed, 1000);
			if (smart_closed)
			{
				break;
			}

			ret = utl_ifconfig_wifi_smart_connect_get(&ap);
		
			if (ret != -1)
			{
				break;
			}
		}
		utl_ifconfig_wifi_smart_connect_stop();

		if (smart_closed)
		{
			goto end;
		}
		
		if (ret != -1)
		{
			smart_recvsetting = TRUE;
			
			if(isVoiceRecvAndSetting())
				goto end;

			maudio_resetAIAO_mode(2);
			maudio_speaker(VOICE_REV_NET_SETTING,TRUE, TRUE, TRUE);
			mio_set_net_st(DEV_ST_WIFI_CONNECTING);

			mvoicedec_disable();
			
			for (k = 0; k < 3; k++)
			{
				ap_t = utl_ifconfig_wifi_get_by_ssid(ap.name);
				if (ap_t)
				{
					break;
				}
				sleep(1);
			}
			if (!ap_t)
			{
				maudio_speaker(VOICE_NOT_FIND_NET,TRUE, TRUE, TRUE);
				goto again;
			}
			
			Start_timer_for_count();						//收到网络配置时开始计时

			memcpy(ap.iestat, ap_t->iestat, sizeof(ap.iestat));
			if (utl_ifconfig_wifi_connect(&ap) == 0)
			{
				//同时关闭声波配wifi
				mvoicedec_disable();
				break;
			}
again:
			smart_recvsetting = FALSE;
			mvoicedec_enable();
		}
		utl_WaitTimeout(smart_closed, 2000);
	}

end:
	wifi_setting = 0;
	smart_connecting = FALSE;
	smart_recvsetting = FALSE;

	printf("utl_ifconfig_wifi_smart_connect_thread exit\n");
}

void utl_ifconfig_wifi_smart_connect()
{
	pthread_t pid_smart;

	if(smart_connecting)
		return;
	
	smart_closed = FALSE;

	pthread_create_detached(&pid_smart, NULL, (void*)utl_ifconfig_wifi_smart_connect_thread, NULL);
}

void utl_ifconfig_wifi_smart_connect_close(BOOL bBlock)
{
	smart_closed = TRUE;

	if (bBlock)
	{
		while (smart_connecting)
		{
			usleep(100 * 1000);
		}
	}
}

BOOL utl_ifconfig_wifi_smart_get_recvandsetting()
{
	return smart_recvsetting;
}

// 尝试连接热点，如果ap中的安全模式、加密类型为-1，则先搜索后连接，否则直接连接
// bInConfig表示是否在WiFi配置中，TRUE表示正在配置，会有停止配置和连接失败重启配置的操作
// 返回值：0，成功，<0，失败
int utl_ifconfig_wifi_connect_ap(wifiap_t* ap, BOOL bInConfig)
{
	int k;
	int nRet = 0;

	if (bInConfig)
	{
		mvoicedec_disable();
		if (utl_ifconfig_bsupport_smartlink(utl_ifconfig_wifi_get_model()))
		{
			utl_ifconfig_wifi_smart_connect_stop();
		}
		maudio_resetAIAO_mode(2);
		maudio_speaker(VOICE_REV_NET_SETTING, TRUE, TRUE, TRUE);
	}

	if (WIFI_MODE_AP == utl_ifconfig_wifi_get_mode())
	{
		utl_ifconfig_wifi_start_sta();
	}

	printf("utl_ifconfig_wifi_connect_ap, ssid: %s, pwd: %s\n", ap->name, ap->passwd);

	mio_set_net_st(DEV_ST_WIFI_CONNECTING);

	// 重新搜索AP
	if ((signed char)ap->iestat[0] < 0 && (signed char)ap->iestat[1] < 0)
	{
		wifiap_t* search_ap = NULL;
		for (k = 0; k < 3; k++)
		{
			search_ap = utl_ifconfig_wifi_get_by_ssid(ap->name);
			if (search_ap)
			{
				// 这两个暂时不用
				// ap->quality = search_ap->quality;
				// ap->keystat = search_ap->keystat;
				memcpy(ap->iestat, search_ap->iestat, sizeof(ap->iestat));
				break;
			}
			sleep(1);
		}

		if (!search_ap)
		{
			nRet = -1;
			if (bInConfig)
			{
				maudio_speaker(VOICE_NOT_FIND_NET, TRUE, TRUE, TRUE);
				goto StartAP;
			}
			else
			{
				// 没有搜索到热点
				return nRet;
			}
		}
	}

	if (utl_ifconfig_wifi_connect(ap) != 0)
	{
		nRet = -2;
		if (bInConfig)
		{
			goto ReConfig;
		}
		else
		{
			// 连接失败
			return nRet;
		}
	}

	Start_timer_for_count();						// 网络连接成功后再开始计时

	return 0;

StartAP:
	printf("**************connect wifi fail, Start AP Mode**************\n");
	utl_ifconfig_wifi_start_ap();

ReConfig:
	mvoicedec_enable();
	if (utl_ifconfig_bsupport_smartlink(utl_ifconfig_wifi_get_model()))
	{
		utl_ifconfig_wifi_smart_connect_start();
	}

	return nRet;
}

static int __hostapd_conf_save(char *name, int chn, char *pwd, char hwMode)
{
	int fd = -1;
	char buf[1024] = {0};
	const char file_realtek8188[] = {
			"interface=wlan1\n"
	        "ctrl_interface=/var/run/hostapd\n"
	        "ssid=%s\n"
	        "channel=%d\n"
	        "wpa=3\n"
	        "wpa_passphrase=%s\n"
	        "eap_server=1\n"
	        "wps_state=0\n"
	        "manufacturer=Realtek\n"
	        "model_name=RTW_SOFTAP\n"
	        "model_number=WLAN_CU\n"
	        "config_methods=label display push_button keypad\n"
	        "beacon_int=100\n"
	        "hw_mode=%c\n"
	        "ieee80211n=1\n"
	        "wme_enabled=1\n"
	        "wpa_key_mgmt=WPA-PSK\n"
	        "wpa_pairwise=CCMP TKIP\n"
	        "max_num_sta=4\n"
	        "wpa_group_rekey=86400\n"
	        "dtim_period=1\n" 
	        "rts_threshold=2347\n" 
	        "fragm_threshold=2346\n" 
		};
	const char file_realtek8192[] = {
			"interface=wlan0\n"
	        "ctrl_interface=/var/run/hostapd\n"
	        "ssid=%s\n"
	        "channel=%d\n"
	        "wpa=3\n"
	        "wpa_passphrase=%s\n"
	        "eap_server=1\n"
	        "wps_state=0\n"
	        "manufacturer=Realtek\n"
	        "model_name=RTW_SOFTAP\n"
	        "model_number=WLAN_CU\n"
	        "config_methods=label display push_button keypad\n"
	        "beacon_int=100\n"
	        "hw_mode=%c\n"
	        "ieee80211n=1\n"
	        "wme_enabled=1\n"
	        "wpa_key_mgmt=WPA-PSK\n"
	        "wpa_pairwise=CCMP TKIP\n"
	        "max_num_sta=4\n"
	        "wpa_group_rekey=86400\n"
	        "dtim_period=1\n" 
	        "rts_threshold=2347\n" 
	        "fragm_threshold=2346\n" 
		};
	const char file_realtek8188_GCW1020[] = {
			"interface=wlan1\n"
	        "ctrl_interface=/var/run/hostapd\n"
	        "ssid=%s\n"
	        "channel=%d\n"
	        "wpa=0\n"
	        "eap_server=1\n"
	        "wps_state=0\n"
	        "manufacturer=Realtek\n"
	        "model_name=RTW_SOFTAP\n"
	        "model_number=WLAN_CU\n"
	        "config_methods=label display push_button keypad\n"
	        "beacon_int=100\n"
	        "hw_mode=%c\n"
	        "ieee80211n=1\n"
	        "wme_enabled=1\n"
	        "max_num_sta=4\n"
	        "wpa_group_rekey=86400\n"
	        "dtim_period=1\n" 
	        "rts_threshold=2347\n" 
	        "fragm_threshold=2346\n" 
		};
	const char* file_ralink7601[5] = {
			"Default\nCountryRegion=5\nCountryRegionABand=7\nCountryCode=\nBssidNum=1\n",

			"SSID=%s\n",

			"WirelessMode=9\nTxRate=0\nChannel=0\nBasicRate=15\nBeaconPeriod=100\nDtimPeriod=1\nTxPower=100\nDisableOLBC=0\nBGProtection=0\nTxAntenna=\n"
	        "RxAntenna=\nTxPreamble=0\nRTSThreshold=2347\nFragThreshold=2346\nTxBurst=1\nPktAggregate=0\nTurboRate=0\nWmmCapable=0\n"
	        "APSDCapable=0\nDLSCapable=0\nAPAifsn=3;7;1;1\nAPCwmin=4;4;3;2\nAPCwmax=6;10;4;3\nAPTxop=0;0;94;47\nAPACM=0;0;0;0\n"
	        "BSSAifsn=3;7;2;2\nBSSCwmin=4;4;3;2\nBSSCwmax=10;10;4;3\nBSSTxop=0;0;94;47\nBSSACM=0;0;0;0\nAckPolicy=0;0;0;0\n"
	        "NoForwarding=0\nNoForwardingBTNBSSID=0\nHideSSID=0\nStationKeepAlive=0\nShortSlot=1\nAutoChannelSelect=0\nIEEE8021X=0\n"
	        "IEEE80211H=0\nCSPeriod=10\nWirelessEvent=0\nIdsEnable=0\nAuthFloodThreshold=32\nAssocReqFloodThreshold=32\nReassocReqFloodThreshold=32\n"
	        "ProbeReqFloodThreshold=32\nDisassocFloodThreshold=32\nDeauthFloodThreshold=32\nEapReqFooldThreshold=32\nPreAuth=0\n"
	        "AuthMode=WPA2PSK\nEncrypType=AES\nRekeyInterval=0\nRekeyMethod=DISABLE\nPMKCachePeriod=10\n",

			"WPAPSK=%s\n",

			"DefaultKeyID=2\nKey1Type=1\nKey1Str=12345678\nKey2Type=0\nKey2Str=\nKey3Type=0\nKey3Str=\nKey4Type=0\nKey4Str=\nHSCounter=0\n"
	        "AccessPolicy0=0\nAccessControlList0=\nAccessPolicy1=0\nAccessControlList1=\nAccessPolicy2=0\nAccessControlList2=\nAccessPolicy3=0\n"
	        "AccessControlList3=\nWdsEnable=0\nWdsEncrypType=NONE\nWdsList=\nWdsKey=\nRADIUS_Server=192.168.2.3\nRADIUS_Port=1812\n"
	        "RADIUS_Key=ralink\nown_ip_addr=192.168.5.234\nEAPifname=br0\nPreAuthifname=br0\nHT_HTC=0\nHT_RDG=0\nHT_EXTCHA=0\nHT_LinkAdapt=0\n"
	        "HT_OpMode=0\nHT_MpduDensity=5\nHT_BW=1\nHT_AutoBA=1\nHT_AMSDU=0\nHT_BAWinSize=64\nHT_GI=1\nHT_MCS=33\nMeshId=MESH\nMeshAutoLink=1\n"
	        "MeshAuthMode=OPEN\nMeshEncrypType=NONE\nMeshWPAKEY=\nMeshDefaultkey=1\nMeshWEPKEY=\nWscManufacturer=\nWscModelName=\nWscDeviceName=\n"
	        "WscModelNumber=\nWscSerialNumber=\nRadioOn=1\nPMFMFPC=0\nPMFMFPR=0\nPMFSHA256=0\n"
		};
	
	

	if (sWifiModel == WIFI_MODEL_REALTEK8188)
	{
		fd = open(NETWORK_HOSTAPD_CONF_FILE, O_RDWR | O_CREAT | O_TRUNC);
		if (fd == -1)
		{
			Printf(("open AP conffile error\n"));
			return -1;
		}
		sprintf(buf, file_realtek8188, name, chn, pwd, hwMode);
		write(fd, buf, strlen(buf));
	}
	else if (sWifiModel == WIFI_MODEL_REALTEK8192
			|| (sWifiModel == WIFI_MODEL_M88WI6700))
	{
		fd = open(NETWORK_HOSTAPD_CONF_FILE, O_RDWR | O_CREAT | O_TRUNC);
		if (fd == -1)
		{
			Printf(("open AP conffile error\n"));
			return -1;
		}
		sprintf(buf, file_realtek8192, name, chn, pwd, hwMode);
		write(fd, buf, strlen(buf));
	}
	else
	{
		fd = open(NETWORK_RALINK7601_AP_FILE, O_RDWR | O_CREAT | O_TRUNC);
		if (fd == -1)
		{
			Printf(("open AP conffile error\n"));
			return -1;
		}
		write(fd, file_ralink7601[0], strlen(file_ralink7601[0]));
		sprintf(buf, file_ralink7601[1], name);
		write(fd, buf, strlen(buf));
		write(fd, file_ralink7601[2], strlen(file_ralink7601[2]));
		sprintf(buf, file_ralink7601[3], pwd);
		write(fd, buf, strlen(buf));
		write(fd, file_ralink7601[4], strlen(file_ralink7601[4]));
	}

	close(fd);
    return 0;
}

int utl_ifconfig_wifi_STA_configured()
{
	if ((access(NETWORK_WIFI_SAVE_DAT, F_OK) == 0) || 
		(access("./rec/00/wpa_supplicant.conf", F_OK) == 0))
		return 1;

	return 0;
}

wifi_mode_e utl_ifconfig_wifi_get_mode()
{
	return WifiDrvierMode;
}

int utl_ifconfig_wifi_mode_change(wifi_mode_e mode)
{
	printf("wifi change mode,cur:%d,last:%d\n",mode,WifiDrvierMode);
	char cmd[64];
	if (mode == WifiDrvierMode)
	{
		return 0;
	}
	WifiDrvierMode = mode;
	if ((sWifiModel == WIFI_MODEL_REALTEK8188)
			|| (sWifiModel == WIFI_MODEL_REALTEK8192))
	{
		
	}
	else if (sWifiModel == WIFI_MODEL_RALINK7601)
	{
		if (mode == WIFI_MODE_STA)
		{
			utl_system("rmmod mt7601Uap");
			sleep(2);
#if (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100) //18E加载的模块偶尔会挂掉，怀疑内存不够造成，这里主动清理缓存
			utl_system("sync");
			utl_system("echo 3 > /proc/sys/vm/drop_caches");
			sleep(1);
#endif
			utl_system("insmod "NETWORK_WIFI_DRIVER_PATH"mtutil7601Usta.ko");
   	 		utl_system("insmod "NETWORK_WIFI_DRIVER_PATH"mt7601Usta.ko");
    		utl_system("insmod "NETWORK_WIFI_DRIVER_PATH"mtnet7601Usta.ko");
			sleep(2);
		}
		else
		{
			utl_system("rmmod mtnet7601Usta");
			utl_system("rmmod mt7601Usta");
			utl_system("rmmod mtutil7601Usta");
			sleep(2);
#if (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100) //18E加载的模块偶尔会挂掉，怀疑内存不够造成，这里主动清理缓存
			utl_system("sync");
			utl_system("echo 3 > /proc/sys/vm/drop_caches");
			sleep(1);
#endif
			utl_system("insmod "NETWORK_WIFI_DRIVER_PATH"mt7601Uap.ko");
			sleep(2);
		}
	}
	
	return 0;
}

//检查某个网卡是否存在
//ifname 网卡名称。bAll 全部网卡，为0时只检查打开的网卡。
int __bNetCard_Exist(char *ifname,int bAll)
{
	char cmd[64];
	if(ifname==NULL)
		return -1;
	sprintf(cmd,"ifconfig %s|grep %s\n",bAll?"-a":"",ifname);
//	printf(cmd);
	FILE * fd = popen(cmd, "r");
	char buf[1024];
	if (fd == NULL)
	{
		pclose(fd);
		return 0;	//popen 失败无法判断连接状态
	}
	int ret = fread(buf, 1, 1024, fd);
	if (ret <= 0)
	{
		Printf("ERROR: Failed fread\n");
		pclose(fd);
		return 0;
	}
	buf[ret] = '\0';
	pclose(fd);
	if (ret > 0)
		return 1;
	return 0;
}

BOOL utl_ifconfig_bSupport_ETH_check()
{
	if (PRODUCT_MATCH("H210") || 
		PRODUCT_MATCH("H210C") || 
		PRODUCT_MATCH("H600") ||
		PRODUCT_MATCH("BMDQ") ||
		PRODUCT_MATCH("AY") ||
		PRODUCT_MATCH("MF") ||
		PRODUCT_MATCH("H210-S") ||
		PRODUCT_MATCH("H301") ||
		PRODUCT_MATCH("HA320-H1") ||
		PRODUCT_MATCH("HA320-H1-A") || 
		PRODUCT_MATCH("HV120-H1") ||
		PRODUCT_MATCH("HA121-H2")  ||
		PRODUCT_MATCH("H303") ||
		PRODUCT_MATCH("A1") ||
		PRODUCT_MATCH("S530") ||
		PRODUCT_MATCH("HXBJRB") ||
		PRODUCT_MATCH("JW-501DM") ||
		PRODUCT_MATCH("JW-502DM") ||
		PRODUCT_MATCH("DR-H20910") || 
		PRODUCT_MATCH("JD-H40810") ||
		PRODUCT_MATCH("HW-H21110") || 
		HWTYPE_MATCH(HW_TYPE_HA210) ||
		HWTYPE_MATCH(HW_TYPE_A4) ||
		HWTYPE_MATCH(HW_TYPE_V3))
	{
		return FALSE;
	}

	// 20180402后的内核版本，如果没有有线网卡，不会出现eth0网卡结点
	return __bNetCard_Exist("eth0", TRUE);
}

static int __bSupport_WIFI_check()
{
	static int wifiSupport = -1;

	if(wifiSupport != -1)
		return wifiSupport;

	if(__bNetCard_Exist("wlan0",1)>0)
	{
		wifiSupport = 1;
	}

	return wifiSupport;
}

int utl_ifconfig_wifi_bsupport()
{
	return __bSupport_WIFI_check();
}

int utl_ifconfig_wifi_start_ap()
{
	char apName[32];
	char cmd[100] = { 0 };
	static int bfirst=1;
	int timer = 15;//15s 拿不到云视通号，视为0

	pthreadinfo_add((char *)__func__);

	if(!__bSupport_WIFI_check())
	{
		printf("DO NOT SUPPORT WIFI!\n");
		return 0;
	}
	if (WifiDrvierMode == WIFI_MODE_AP)
	{
		return 0;
	}
	
	if(pthread_mutex_trylock(&ap_mutex)<0)//磊科的私协已经获取到id和密码，不用再开启AP了。
	{
		printf("LEIKE DEBUG WIFI!\n");
		return 0;
	}

	if(bfirst)
	{
		bfirst = 0;
		ipcinfo_t ipcinfo;
		ipcinfo_get_param(&ipcinfo);
		while (!ipcinfo.ystID && timer)
		{
			ipcinfo_get_param(&ipcinfo);
			sleep(1);
			timer--;
		}

		char ystNo[32] = {0};
		jv_ystNum_parse(ystNo, ipcinfo.nDeviceInfo[6], ipcinfo.ystID);
		if (gp.bNeedYST)
		{
			snprintf(apName, sizeof(apName), "IPC-C-%s", ystNo);
		}
		__hostapd_conf_save(apName, 6, "12345678", 'g');
	}
	if ((sWifiModel == WIFI_MODEL_REALTEK8188)
			|| (sWifiModel == WIFI_MODEL_REALTEK8192)
			|| (sWifiModel == WIFI_MODEL_M88WI6700))
	{
		utl_system("killall wpa_supplicant");
		sleep(2);
		utl_system("ifconfig wlan0 0.0.0.0");
		utl_system("ifconfig wlan0 down");
		utl_ifconfig_wifi_save_remove();
		Printf("\n\nAP APStart\n\n");
		utl_ifconfig_wifi_mode_change(WIFI_MODE_AP);
		utl_system("killall hostapd && killall udhcpd");
		sprintf(cmd, "/home/wifi/hostapd %s -B", NETWORK_HOSTAPD_CONF_FILE);
		utl_system(cmd);
		sleep(2);
		if(sWifiModel == WIFI_MODEL_REALTEK8188)
			utl_system("ifconfig wlan1 10.10.0.1 netmask 255.255.255.0 &");
		else
			utl_system("ifconfig wlan0 10.10.0.1 netmask 255.255.255.0 &");
		usleep(100*1000);
		if (0 == access("/home/wifi/udhcpd.conf", F_OK))
			utl_system("udhcpd /home/wifi/udhcpd.conf &");
	}
	else
	{
		utl_system("ifconfig ra0 down");
		sleep(2);
		utl_ifconfig_wifi_save_remove();
		Printf("\n\nAP APStart\n\n");
		utl_ifconfig_wifi_mode_change(WIFI_MODE_AP);
		utl_system("killall udhcpd");
		utl_system("ifconfig ra0 10.10.0.1 netmask 255.255.255.0 &");
		usleep(100*1000);
		if (0 == access("/home/wifi/udhcpd.conf", F_OK))
			utl_system("udhcpd /home/wifi/udhcpd.conf &");
	}

	mio_set_net_st(DEV_ST_WIFI_SETTING);
	printf("APStart, time=%ld\n", time(NULL));

	printf("\n\nAP APStart End\n\n");
	pthread_mutex_unlock(&ap_mutex);
	return 0;
}

int utl_ifconfig_wifi_start_sta()
{
	utl_system("killall hostapd");
	utl_system("killall udhcpd");
	utl_system("ifconfig wlan0 down");
	Printf("\n\nSTA Start\n\n");
	utl_ifconfig_wifi_mode_change(WIFI_MODE_STA);
	
	return 0;
}

void utl_ifconfig_wifi_power_search()
{
	int i = 0;
	int j = 0;
	int k = 0;
	int cnt_p = 0;
	int cnt_s = 0;
	wifiap_t* p = NULL;
	
	for (i = 0; i < 3; i++)			// 有搜索不全的问题，暂改成3次搜索
	{
		p = utl_ifconfig_wifi_list_ap(TRUE);
		cnt_p = utl_ifconfig_wifi_list_cnt(p);
		cnt_s = utl_ifconfig_wifi_list_cnt(sPowerAPlist);
		for (j = 0; j < cnt_p; j++)
		{
			for (k = 0; k < cnt_s; k++)
			{
				if (strcmp(p[j].name, sPowerAPlist[k].name) == 0)
				{
					break;
				}
			}
			if (k == cnt_s)
			{
				sPowerAPlist[cnt_s++] = p[j];
			}
		}
	}
	cnt_s = utl_ifconfig_wifi_list_cnt(sPowerAPlist);
	printf("wifi AP list, cnt: %d\n", cnt_s);
	for (i = 0; i < cnt_s; i++)
	{
		printf("%s\n", sPowerAPlist[i].name);
	}
	printf("\n\n");
}


static int sOnvifResetTimer = -1;
static BOOL __onvif_reset_timer(int tid, void *param)
{
#ifdef TINY_ONVIF_SUPPORT
	if (gp.bNeedOnvif)
	{
		onvif_reset();
	}
#endif
	return FALSE;
}

static void __onvif_reset_with_timer()
{
	if (sOnvifResetTimer == -1)
		sOnvifResetTimer = utl_timer_create("onvif reset", 0, __onvif_reset_timer, NULL);
	else
		utl_timer_reset(sOnvifResetTimer, 0, __onvif_reset_timer, NULL);
}

void check_SD_netCfg()
{
	Printf("check_SD_netCfg ......\n");
	
	char devblk[32] = "/dev/mmcblk0p1";
	char mount_path[MAX_PATH] = "./rec/00";
	char mkdir_cmd[128] = {0};
	if(!access(devblk , F_OK))//如果分区存在
	{
		if (access(mount_path, F_OK))
		{
			sprintf(mkdir_cmd,"mkdir -p %s",mount_path);
			utl_system(mkdir_cmd);	
		}	
		mount(devblk, mount_path, "vfat", 0, NULL);
		if(access("./rec/00/wpa_supplicant.conf", F_OK) == 0)
	    {
	        Printf("wpa_supplicant.conf found in sd card\n\n\n\n");
	        utl_system("cp ./rec/00/wpa_supplicant.conf "NETWORK_WIFI_CONFIG_FILE);
	    }		
	}
}

/************************************************wpa_supplicant连接状态查询****************************/
static int _utlSocket(int mode)
{
	int sockfd;
	sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);
	return sockfd;
}
static int _utlWait(int sock,int timeoutMS)
{
	struct timeval tv;
	int ret;
	fd_set rfds;
	tv.tv_sec = timeoutMS / 1000;
	tv.tv_usec = (timeoutMS % 1000)*1000;
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	if(0 != ret)
	{
		printf("getwifistatus:setsockopt error\n");
		return ret;		
	}
	else
		return ret;
	//ret = select(sock+1, &rfds, NULL, NULL, &tv);
	/*if (ret == -1)
	{
		printf("getwifistatus:select error\n");
		return ret;
	}
	else if(ret == 0)
	{
		printf("getwifistatus:recvfrom timeout\n");
	    return ret;
	}
	else
		return ret;*/
}
static int _utlRecv(int sockfd,char *buf,int len)
{
	int nRet;
	nRet = recv(sockfd, buf, len, 0);
	if(nRet <0 )
	{
		printf(" recvfrom() failed!\n");
		return -1;
	}
	return nRet;
}

int cmGetNetLinkStatus1(const char *ethName)
{
#define  RT_OID_GEN_MEDIA_CONNECT_STATUS        0x060B 

    int skfd;
    struct  iwreq wrq;
    int data = 0;

    if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) == 0)
        return -1;
    
    memset(&wrq, 0, sizeof(wrq));
    strcpy(wrq.ifr_name, ethName); 
    wrq.u.data.length = sizeof(data);
    wrq.u.data.pointer = &data; 
    wrq.u.data.flags = RT_OID_GEN_MEDIA_CONNECT_STATUS; 
    if(ioctl(skfd, 0x8BEE, &wrq) < 0) 
    {
        close(skfd);
        return -2;
    }
    
    close(skfd);

    return data;
}

int utlGetWifiStatus(char * dev)
{
	char *p, buf[1024] = "STATUS";
	int len;
	struct sockaddr_un local, dest;
	static int counter = 0;
	int s;
	int size;
	int ret = 0;

	//DBG_INFO(("getWifiStatus:\n"));
	s = _utlSocket(2);
	if (s < 0)
	{
		printf(("getWifiStatus: sock failed\n"));
		return -1;
	}

	memset(&local, 0, sizeof(local));
	local.sun_family = AF_UNIX;
	sprintf(local.sun_path, "/var/wpa_ctrl_%d-%d", (int) getpid(), ++counter);
	if (bind(s, (struct sockaddr *) &local, sizeof(local)) < 0)
	{
		printf(("getWifiStatus: bind failed\n"));
		close(s);
		return -2;
	}

	memset(&dest, 0, sizeof(dest));
	dest.sun_family = AF_UNIX;

	strcpy(dest.sun_path, "/var/run/wpa_supplicant/wlan0");

	if (connect(s, (struct sockaddr *) &dest, sizeof(dest)) < 0)
	{
		printf(("getWifiStatus: connect failed\n"));
		close(s);
		unlink(local.sun_path);
		return -3;
	}

	if (send(s, buf, strlen(buf), 0) < 0)
	{
		printf(("getWifiStatus: send failed\n"));
		unlink(local.sun_path);
		close(s);
		return -4;
	}
	//DBG_INFO(("getWifiStatus: wait\n"));
	if (_utlWait(s, 1000) != 0)
	{
		printf(("getWifiStatus: wait failed\n"));
		unlink(local.sun_path);
		close(s);
		return -5;
	}

	//DBG_INFO(("getWifiStatus: read\n"));
	len = _utlRecv(s, buf, sizeof(buf));
	if (len <= 0)
	{
		printf(("getWifiStatus: read failed\n"));
		unlink(local.sun_path);
		close(s);
		return -6;
	}

	unlink(local.sun_path);
	close(s);

	p = strstr(buf, "wpa_state=");
	if (p)
	{
		p += 10;
		if (!strncmp(p, "COMPLETED", 9))
			return 1;

		if (!strncmp(p, "DISCONNECTED", 12))
			return 0;

		if (!strncmp(p, "ASSOCIATING", 11))
			return 3;

		if (!strncmp(p, "ASSOCIATED", 10))
			return 4;

		if (!strncmp(p, "SCANNING",8))
			return 5;
	}

	return 2;
}

/************************************************wpa_supplicant连接状态查询****************************/

int __wifi_sta_auth()//wifi sta 认证，只认证路由器密码，不获取IP地址。
{
	int i, st = 1;
	int failT = 0;
	wifiap_t w;
	
	//STA连接路由器前清一下缓存，3518E内存太小了
#if (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100)
	utl_system("sync");
	utl_system("echo 3 > /proc/sys/vm/drop_caches");
	sleep(1);
#endif
	//从cfg读出STA配置信息，写入conf文件
	memset(&w, 0, sizeof(w));
	if (utl_ifconfig_wifi_info_get(&w) > 0)
	{
		utl_ifconfig_wifi_conf_save(&w);
	}

	printf("__wifi_sta auth start, ssid: %s, pwd: %s\n", w.name, w.passwd);

	utl_system("killall wpa_supplicant");
	sleep(1);
	utl_system("killall udhcpc");
	sleep(1);
	// 澜起6700使用80211驱动
	utl_system("/home/wifi/wpa_supplicant -B -iwlan0 -c "NETWORK_WIFI_CONFIG_FILE" -D%s", (sWifiModel == WIFI_MODEL_M88WI6700) ? "nl80211" : "wext");
	
	for (i = 0; i < 20; i++)
	{
		sleep(2);
		if (sWifiModel == WIFI_MODEL_RALINK7601)
		{
			st = cmGetNetLinkStatus1("wlan0");
		}
		else
		{
			st = utlGetWifiStatus("wlan0");
		}

		printf("WPA status: %d\n", st);
		if (st == 1) {
			break;
		}
		if(st <= 0)
	    {
	        failT++;
	        if(failT > 5)
	        {
	        	st = 2;
				break;
			}    
	    }
	}
	
	return st;
}

void __wifi_sta_start_dhcp()
{
	eth_t eth;
	char iface[12];
	int beth=0;

	if (utl_ifconfig_bSupport_ETH_check())
	{
		utl_ifconfig_get_iface(iface);
		if(strncmp(iface,"eth0",4)==0)
		{
			beth=1;
			utl_ifconfig_eth_get(&eth);	//保存之前的static的IP地址，再次插网线时用
		}
		utl_system("ifconfig eth0 0.0.0.0");	//clear eth0 ipaddress
		mlog_write("Set EthIP 0.0.0.0 By Wi-Fi");
	}

	if(beth)
	{
		if(eth.bDHCP)
			_utl_write_interface("wlan0", "dhcp", NULL, NULL, NULL, NULL);
		else
			_utl_write_interface("wlan0", "static", eth.addr, eth.mask, eth.gateway, eth.dns);
	}
	if(utl_ifconfig_bSupport_ETH_check()==FALSE)
		_utl_write_interface("wlan0", "dhcp", NULL, NULL, NULL, NULL);
	utl_system("route del default;route del 255.255.255.255");
	utl_system("udhcpc -iwlan0 -q&");
	utl_system("route add -net 255.255.255.255 netmask 255.255.255.255 dev wlan0");

	if (sWifiModel == WIFI_MODEL_REALTEK8188)
	{
		utl_system("ifconfig wlan1 down");
	}
}

int __wifi_sta_after_auth(int st, wifiap_t *ap)
{
	if (st == 1)
	{	
		if(utl_ifconfig_b_linkdown("eth0") == FALSE)
		{
			//关闭STA
			__wifi_STA_stop();
			//配置成功，但不申请IP
			maudio_resetAIAO_mode(2);
			maudio_speaker(VOICE_WIFI_SET_SUCC, TRUE,TRUE, TRUE);
			return 0;
		}

		// 开启DHCP
		__wifi_sta_start_dhcp();

		char gw[17]={0};
		int IsGet = 0;	
		int count = 0;
		do
		{
			printf("================================> check ipaddr!! \n");
			sleep(1);
			IsGet=_getGateWay("wlan0",gw);
			if (IsGet)
			{
				if(checkPing(1,gw) > 0)  //有一次ping通网关认为已经分配处IP地址
				{
					mio_set_net_st(DEV_ST_WIFI_CONNECTED);
					break;
				}	
			}

			count++;
			if(count >= 15)  //15次没有获取到ip地址，认为网络配置失败
				break;
		}while(1);

		if(count < 15)
		{
			maudio_resetAIAO_mode(2);
			if(NULL != ap)
			{
				maudio_speaker(VOICE_WIFI_SET_SUCC, TRUE,TRUE, TRUE);
			}
			__utl_reset_check();
			//没有插网线，分配出了IP地址，配置成功
			return 0;
		}
	}

	//以下为配置或连接失败处理
	if(NULL != ap)
	{
		maudio_resetAIAO_mode(2);
		maudio_speaker(VOICE_WIFI_SET_FAIL,TRUE, TRUE, TRUE);
	}
	
	if(utl_ifconfig_b_linkdown("eth0") == FALSE)
	{
		utl_ifconfig_wifi_save_remove();		//插着网线配置失败也把配置信息删掉
		//关闭STA
		__wifi_STA_stop();
		//配置不成功，但不开启AP
		return -1;
	}
	//无线情况下，如果STA认证不成功，切回AP模式，并播放切回AP模式音频
	if(ap)//实际配置AP的情况下，失败时才开启AP，插拔网线时候不开启AP
	{
		if(utl_ifconfig_bsupport_apmode(sWifiModel))
		{
			printf("**************connect wifi fail, set net to ap**************\n");
			utl_ifconfig_wifi_start_ap();
		}
		else
		{
			utl_ifconfig_wifi_save_remove();
		}
	}
	else
	{
		//没插网线连接路由器时，如果没连接上也要开启udhcpc，当连接上路由器后可以获取IP
		__wifi_sta_start_dhcp();
	}
	return -1;
}

/**
 *@brief 连接指定热点
 *
 *@return 0 成功，< 0 失败
 */
int utl_ifconfig_wifi_connect(wifiap_t *ap)
{
	if(ap==NULL && utl_ifconfig_wifi_get_mode()!=WIFI_MODE_STA)
		return -1;
	mio_set_net_st(DEV_ST_WIFI_CONNECTING);

	if(ap)
	{
		utl_ifconfig_wifi_info_save(ap);
	}
	printf("wifi_connect start\n");
	int st = -1;
	st = __wifi_sta_auth();
	st = __wifi_sta_after_auth(st, ap);
	printf("wifi_connect end\n");
	return st;
}

/**
 *@brief 根据SSID获取热点结构体
 *
 *
 */
wifiap_t *utl_ifconfig_wifi_get_by_ssid(char *ssid)
{
	int i;
	wifiap_t *list;
	if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
	{
		list = utl_ifconfig_wifi_list_ap(TRUE);
	}
	else
	{
		list = utl_ifconfig_wifi_power_list_ap();
	}
	
	if (list == NULL)
		return NULL;

	for (i=0;list[i].name[0] != '\0'; i++)
	{
		printf("name: %s,ssid: %s\n", list[i].name, ssid);
		if (strcmp(list[i].name, ssid) == 0)
		{
			return &list[i];
		}
	}
	return NULL;
}

/**
 *@brief 获取连接的热点
 *
 *@return 0 成功，< 0 失败
 */
int utl_ifconfig_wifi_get_current(wifiap_t *wifiap)
{
	int i;
//	char *name;
	FILE *fp;
	char *ssid;
	char buffer[1024];
	wifiap_t *tmp;

	fp = fopen(NETWORK_WIFI_CONFIG_FILE, "r");
	if (fp == NULL)
		return -1;
	i = fread(buffer, 1, sizeof(buffer)-1, fp);
	fclose(fp);
	buffer[i] = '\0';
	//格式：ssid="dlink"
	ssid = strstr(buffer, "ssid");
	if (ssid == NULL)
		return -1;
	ssid += 6;
	for (i=0;ssid && ssid[i]; i++)
	{
		if (ssid[i] == '\"')
		{
			ssid[i] = '\0';
			break;
		}
	}

	tmp = utl_ifconfig_wifi_get_by_ssid(ssid);
	if (tmp == NULL)
		return -1;
	memcpy(wifiap, tmp, sizeof(wifiap_t));
	return 0;
}

/**
 *@brief 检查是否合法
 *@return 1 合法，0，不合法
 *
 */
static BOOL _utl_ifconfig_eth_b_param_valid(eth_t *eth)
{
	int ret;
	struct in_addr inp;
	unsigned int ip, gw, mask;
	
	ret = inet_aton(eth->addr, &inp);
	if (ret == 0)
		return FALSE;
	ip = ntohl(inp.s_addr);
	ret = inet_aton(eth->mask, &inp);
	if (ret == 0)
		return FALSE;
	mask = ntohl(inp.s_addr);
	ret = inet_aton(eth->gateway, &inp);
	if (ret == 0)
		return FALSE;
	gw = ntohl(inp.s_addr);
	ret = inet_aton(eth->dns, &inp);
	if (ret == 0)
		return FALSE;

	printf("ip: %x, gw: %x, mask:%x\n", ip, gw, mask);

	if ((ip & mask) != (gw & mask))
	{
		printf("not correct gateway or ip addr\n");
		return FALSE;
	}

	return TRUE;
}

int __ifconfig_eth_set(eth_t *eth)
{
	char *inet;
	if (eth->bDHCP)
	{
		inet = "dhcp";
		_utl_write_interface(eth->name, inet, NULL, NULL, NULL, NULL);
	}
	else
	{
		if (!_utl_ifconfig_eth_b_param_valid(eth))
		{
			Printf("ERROR: param error!\n");
			return JVERR_BADPARAM;
		}
		inet = "static";
		_utl_write_interface(eth->name, inet, eth->addr, eth->mask, eth->gateway, eth->dns);
	}
	utl_system("killall udhcpc; killall wpa_supplicant;");
	sleep(1);	//sleep 一下杀死wpa之后再启动有线，否则出现网关加载不上的情况。
	utl_system("/progs/networkcfg.sh");
	if (eth->bDHCP)
		__utl_reset_check();
	return 0;
}

int utl_ifconfig_eth_set(eth_t *eth)
{
	int ret = 0;
	ret = __ifconfig_eth_set(eth);
	__onvif_reset_with_timer();

	return ret;
}

int utl_ifconfig_eth_get(eth_t *eth)
{
	return utl_ifconfig_build_attr("eth0", eth, FALSE);
}

int utl_ifconfig_wifi_get(eth_t *wifi)
{
	return utl_ifconfig_build_attr("wlan0", wifi, FALSE);
}

int utl_ifconfig_wifi_info_save(wifiap_t *ap)
{
	FILE *fOut = NULL;
	
	fOut=fopen(NETWORK_WIFI_SAVE_DAT, "wb");
	if(!fOut)
	{
		Printf("save wifi dat: %s, err: %s\n", NETWORK_WIFI_SAVE_DAT, strerror(errno));
		return -1;
	}

	fwrite(ap, 1, sizeof(wifiap_t), fOut);
		
	fclose(fOut);
	return 0;
}

int utl_ifconfig_wifi_info_get(wifiap_t *ap)
{
	FILE *fOut = NULL;
	int len = 0;

	if (ap == NULL)
	{
		return -1;
	}
	memset(ap, 0, sizeof(wifiap_t));
	if (access(NETWORK_WIFI_SAVE_DAT, F_OK) != 0)
	{
		return -1;
	}
	fOut=fopen(NETWORK_WIFI_SAVE_DAT, "rb");
	if(!fOut)
	{
		Printf("get wifi dat: %s, err: %s\n", NETWORK_WIFI_SAVE_DAT, strerror(errno));
		return -1;
	}

	len = fread(ap, 1, sizeof(wifiap_t), fOut);

	fclose(fOut);
	return len;
}

void utl_ifconfig_wifi_sta_ip_clear()
{
	utl_system("ifconfig wlan0 0.0.0.0");
}

static int __utl_ifconfig_wifi_ParaGet(char *buf, char *name, char *out)
{
    char *p1 = out, *p = strstr(buf, name);
    
    if(!p)
        return 0;
    
    p += strlen(name);
    while(*p != '\n')
    {
        *p1++ = *p++;
    }
    *p1 = 0;
    
    return p-out;
}

/**
 *@brief 获取网络的状态
 *@param iface 网络名
 *
 *@return 1 连接成功，  0 连接失败
 *
 */
int utl_ifconfig_check_status(char *iface)
{
	FILE * fd;
	char buf[1024];
	int ret;

	fd=popen("route","r");
	if(fd == NULL)
	{
		pclose(fd);
		return 0;//popen 失败无法判断连接状态
	}
	ret = fread(buf,1,1024,fd);
	if (ret <= 0)
	{
		Printf("ERROR: Failed fread\n");
		pclose(fd);
		return 0;
	}
	buf[ret] = '\0';
	pclose(fd);
	if(strstr(buf,iface))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

#define PPPoE_PAP_SEC				"/etc/ppp/pap-secrets"

/**
 *@brief 设置PPPOE的连接
 *
 *@return 0 成功，< 0 失败
 */
int utl_ifconfig_ppp_set(pppoe_t *ppp)
{
	FILE * fp;
	fp = popen("pppoe-setup" , "w");
	if(fp==NULL)
	{
		Printf("Failed Set pppoe... %s\n", strerror(errno));
		return -1;
	}
	fprintf(fp, "%s\n%s\n\nserver\n%s\n%s\n0\ny\n", 
		ppp->username, "eth0", ppp->passwd, ppp->passwd);
	pclose(fp);

	_utl_write_interface("ppp0", "ppp", NULL, NULL, NULL, NULL);
	utl_system("killall networkcfg.sh;/progs/networkcfg.sh&");
	__onvif_reset_with_timer();

	return 0;
}

int utl_ifconfig_ppp_get(pppoe_t *ppp)
{
	FILE *fp;
	char buf[64];
	jv_assert(ppp, return JVERR_BADPARAM;);

	fp = fopen(PPPoE_PAP_SEC, "r");
	if (fp != NULL)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			if (buf[0] == '#')
				continue;
			char *p;
			p = strrchr(buf, '"');
			p[0] = '\0';
			p = strrchr(buf,'"');
			strcpy(ppp->passwd, p+1);

			p[0] = '\0';
			p = strrchr(buf, '"');
			p[0] = '\0';
			p = strrchr(buf,'"');
			strcpy(ppp->username, p+1);
			break;
		}
		//printf("username: %s, passwd: %s\n", ppp->username, ppp->passwd);
		fclose(fp);
		return 0;
	}
	memset(ppp, 0, sizeof(pppoe_t));
	//Printf("ERROR: open %s failed: %s\n", PPPoE_PAP_SEC, strerror(errno));
	return 0;
}

/**
 *@brief 获取当前使用的网络类型
 *
 *@param name 当前使用的网络的类型：static/dhcp/ppp/wifi
 *
 *@return 返回inet指针 
 *
 */
char *utl_ifconfig_get_inet(char *inet)
{
	char iface[16] = {0};
	_utl_read_interface(iface, NULL, NULL, NULL, NULL, NULL);
	if(strcmp(iface,"eth0"))
	{
		strcpy(inet,"wifi");
	}
	else
		_utl_read_interface(NULL, inet, NULL, NULL, NULL, NULL);
	return inet;
}

char *utl_ifconfig_get_iface(char *iface)
{
	_utl_read_interface(iface, NULL, NULL, NULL, NULL, NULL);
	return iface;
}

BOOL utl_ifconfig_bsupport_smartlink(WifiModelType Type)
{
	switch (Type)
	{
	case WIFI_MODEL_REALTEK8188:
	case WIFI_MODEL_REALTEK8192:
		return FALSE;
	case WIFI_MODEL_RALINK7601:
	case WIFI_MODEL_M88WI6700:
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL utl_ifconfig_bsupport_apmode(WifiModelType Type)
{
	switch (Type)
	{
	case WIFI_MODEL_REALTEK8188:
	case WIFI_MODEL_REALTEK8192:
	// case WIFI_MODEL_M88WI6700:
		return TRUE;
	case WIFI_MODEL_RALINK7601:
		return FALSE;
	default:
		return FALSE;
	}
}

//它的返回值，不具备通用性
//在3507，NXP上，有效值为第2BIT 即ret & 2
//在3516，3518上，有效值为第1BIT 即 ret & 1
//其值为真时，表示网线正常，返之，网线被拔出
static int __detect_ethtool(int skfd, const char *if_name)
{
#define ETH_ON_FLAG (1 << 0)

	struct ifreq ifr;
	struct ethtool_value edata;
	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char *) &edata;

	if(ioctl(skfd, SIOCETHTOOL, &ifr) == -1)
	{
		//printf("%s: SIOCETHTOOL on %s failed: %s\n", __func__, if_name, strerror(errno));
		return -1;
	}

	//printf("%s: edata.data=%u\n", __func__, edata.data);
	//return (edata.data ? 1 : 0);
	return edata.data & ETH_ON_FLAG;
}

static int __detect_mii(int skfd, const char *if_name)
{
	struct ifreq ifr;
	struct mii_ioctl_data *mii_val;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

	if(ioctl(skfd, SIOCGMIIPHY, &ifr) < 0)
	{
		//printf("%s: SIOCGMIIPHY on %s failed: %s\n", __func__, if_name, strerror(errno));
		return -1;
	}

	mii_val = (struct mii_ioctl_data *)(&ifr.ifr_data);
	mii_val->reg_num = MII_BMSR;

	if(ioctl(skfd, SIOCGMIIREG, &ifr) < 0)
	{
		//printf("%s: SIOCGMIIREG on %s failed: %s\n", __func__, ifr.ifr_name, strerror(errno));
		return -1;
	}

	if((!(mii_val->val_out & BMSR_RFAULT)) &&
		(mii_val->val_out & BMSR_LSTATUS) &&
		(!(mii_val->val_out & BMSR_JCD)))
	{
		//printf("%s: net is linked\n", __func__);
		return 1;
	}

	//printf("%s: net is not linked\n", __func__);
	return 0;
}

int get_netlink_status(const char *if_name)
{
	int skfd = -1;
	int link_status;

	if(if_name == NULL)
	{
		return -1;
	}

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
	{
		printf("%s: socket error: %s\n", __func__, strerror(errno));
		return -1;
	}

	link_status = __detect_ethtool(skfd, if_name);
	if(link_status < 0)
		link_status = __detect_mii(skfd, if_name);

	close(skfd);
	return link_status;
}

/********************检查wifi在线状态，使用4部的方法，ping网关方法********************************************************/
static int _icmpSocket()
{
	int fd,iMode=1;
	fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	fcntl(fd, F_SETFL, O_NONBLOCK);//设置非阻塞方式
	return fd;
}

/*
char *icmp_data是发数据,准备的缓存区
*/
/*校验和算法*/
static unsigned short _cal_chksum(unsigned short *addr, int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;

	/*把ICMP报头二进制数据以2字节为单位累加起来*/
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	/*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
	if (nleft == 1)
	{
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}
static int _icmpPack(int i,char *sendpacket,pid_t pid)
{
	int packsize;
	struct icmp *icmp;
	struct timeval *tval;

	icmp = (struct icmp*) sendpacket;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = i;
	icmp->icmp_id = pid;
	packsize = 8 + 56;
	tval = (struct timeval *) icmp->icmp_data;
	gettimeofday(tval, NULL); /*记录发送时间*/
	icmp->icmp_cksum = _cal_chksum((unsigned short *) icmp, packsize); /*校验算法*/
	return packsize;
}

static int _getGateWay(char *iface,char *gwout)
{
	FILE * fd;
	char buf[1024];
	char *routeLine,*gw;
	int ret;

	fd = popen("route", "r");
	if (fd == NULL)
	{
		return 0;	//popen 失败无法判断连接状态
	}
	ret = fread(buf, 1, 1024, fd);
	if (ret <= 0)
	{
		Printf("ERROR: Failed fread\n");
		pclose(fd);
		return 0;
	}
	buf[ret] = '\0';
	pclose(fd);
	if ((routeLine=strstr(buf, "default")))
	{
		routeLine[ROUTE_LINE_SIZE]='\0';
		if(strstr(routeLine,iface))
		{
			gw = routeLine+16;
			gw[16]='\0';
			//printf("get gateway gw:%s\n",gw);
			memcpy(gwout,gw,17);
			return 1;
		}
	}
	return 0;
}

static int checkPing(int packet_no, char *pingAddr)
{
    char sendpacket[PACKET_SIZE];
    char recvpacket[PACKET_SIZE];
    struct sockaddr_in from;
    struct sockaddr_in dest_addr;
    int fromlen;
    pid_t pid;
    fromlen=sizeof(from);
    int imcpSock = -1;
    int recvPackNum;
    pid=getpid();
    int packetsize;
    int i = 0;
    int n = 0;

    imcpSock = _icmpSocket();
    if(imcpSock <=0)
    {
    	printf(("imcpSock failed\n"));
        return -1;
    }

    recvPackNum = 0;
    for(i=0; i<packet_no; i++)
    {
        packetsize=_icmpPack(i, sendpacket, pid);

        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = inet_addr(pingAddr);
        if(sendto(imcpSock, sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr) )<0)
        {
        	printf(("sendto %s failed\n", pingAddr));
            continue;
        }
    	sleep(1);
    }

    for(i=0; i<packet_no; i++)
    {
		//sleep(1);
        if((n=recvfrom(imcpSock, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) <0)
        {
            if(errno == EINTR)
            {
            	printf(("recvfrom %s failed\n", pingAddr));
                continue;
            }
            printf(("recvfrom %s failed\n", pingAddr));
            continue;
        }
        if(from.sin_addr.s_addr != dest_addr.sin_addr.s_addr)
        {
            printf(("pkt addr invalid\n"));
            continue;
        }
        recvPackNum++;
    }
    close(imcpSock);
    return recvPackNum;
}

static jv_thread_group_t WifiAliveGroup;

static void __WifiAliveCheck()
{
	pthreadinfo_add((char *)__func__);

	printf("wifi alive check process in!\n\n");

	while(WifiAliveGroup.running)
	{
		char gw[17]={0};
		int IsGet = _getGateWay("wlan0", gw);

		if (IsGet)
		{
			int count = 60;

			while (--count && WifiAliveGroup.running)
			{
				if(checkPing(1, gw) > 0)
				{
					break;
				}
			}

			if (count > 0)
			{
				mio_set_net_st(DEV_ST_WIFI_OK);
				utl_WaitTimeout(!WifiAliveGroup.running, 10 * 1000);
				continue;
			}

			//网络异常
			//判断有线连接。如果有线有连接，则更换为有线，否则，重新尝试无线的连接
			if (utl_ifconfig_b_linkdown("eth0") == FALSE)
			{
				printf("wifi error. now switch to cable link\n");
				return ;
			}
			else
			{
				printf("wifi disconnect! wait wpa_supplicant auto relink!\n");
				mio_set_net_st(DEV_ST_WIFI_CONNECTING);
			}
		}

		utl_WaitTimeout(!WifiAliveGroup.running, 10 * 1000);
	}
	printf("wifi alive check process out!\n\n");
}

static void __StartWifiAliveCheck()
{
	if(!WifiAliveGroup.running && utl_ifconfig_wifi_STA_configured())
	{
		WifiAliveGroup.running = TRUE;
		pthread_create_normal(&WifiAliveGroup.thread, NULL, (void *)__WifiAliveCheck, NULL);
	}
}

static void __StopWifiAliveCheck()
{
	if(WifiAliveGroup.running)
	{
		WifiAliveGroup.running = FALSE;
		pthread_join(WifiAliveGroup.thread,NULL);
	}
}

static int timer_ip = -1;
static jv_thread_group_t netlink_check_group = {0};
static void *_netlink_check_process(void *param)
{
	pthreadinfo_add((char *)__func__);

	int bOldLinkDown=-1;
	int bLinkDown = 0;
	int link = 0;
	int check_count = 0;
	int check_num = 0;
	static char first = 0;
	if (__bSupport_WIFI_check() && (utl_ifconfig_bsupport_apmode(sWifiModel)))
	{
		//上电使用STA搜3次路由器热点列表保存下来，如果在AP模式下，搜索功能就用这个列表
		if(first == 0)
		{
			sleep(7);
		}
		utl_ifconfig_wifi_power_search();
	}
	else
		sleep(2);

	while(netlink_check_group.running)
	{
		//有线切无线要判断4次，无线切有线立马切换
		link = utl_ifconfig_b_linkdown("eth0");
		if(link == -1)
		{
			return NULL;
		}

		if (bOldLinkDown != -1)
		{
			if (link != bLinkDown)
			{
				if (bLinkDown == 0)
				{
					check_num = 4;
				}
				else
				{
					check_num = 0;
				}
				check_count++;
				//printf("check count %d\n", check_count);
				if (check_count > check_num)
				{
					check_count = 0;
					bLinkDown = link;
					//printf("bLinkDown set %d, old %d\n", bLinkDown, bOldLinkDown);
				}
			}
			else
			{
				//printf("the same \n");
				check_count = 0;
			}
		}
		else
		{
			bLinkDown = link;
		}
		
		
		if(!bLinkDown)
		{
			eth_t eth;
			utl_ifconfig_eth_get(&eth);
			if(inet_addr(eth.addr) != INADDR_NONE)
			{
				mio_set_net_st(DEV_ST_ETH_OK);
			}
			else
			{
				mio_set_net_st(DEV_ST_ETH_DISCONNECT);
			}
		}
		
		if ((bOldLinkDown != -1 && bOldLinkDown != bLinkDown)||(bOldLinkDown == -1))
		{
			if (timer_ip >= 0 && bOldLinkDown != -1)//已经开始切换网络，去掉上次切换网络的timer，防止有时候太频繁会重复。
			{
				utl_timer_destroy(timer_ip);
				timer_ip = -1;
			}
			if(bOldLinkDown != -1)//上电第一次，不写日志，此时中英文日志状态还未初始化日志语言是错误的
			{
				if (bLinkDown)
					mlog_write("Net Cable Link is Down");
				else
					mlog_write("Net Cable Link is Up");
			}
			if(bLinkDown && __bSupport_WIFI_check())
			{
				if (utl_ifconfig_wifi_STA_configured() == 1)
				{
					printf("**************STA连接**************\n");
					if(0 == utl_ifconfig_wifi_connect(NULL))//耗时操作，执行完毕之前不会检查下次插拔。
					{
					}
				}				
				else
				{
					mio_chat_interrupt();
					sleep(1);
					//等待配置
					if(bOldLinkDown != -1 || (first == 0))
					{
						maudio_resetAIAO_mode(2);
						maudio_speaker(VOICD_WAITSET, TRUE, TRUE, TRUE);
					}
					if (hwinfo.bSupportVoiceConf && strcmp(hwinfo.devName, "HXBJRB") != 0)
					{
						printf("**************声波配置**************\n");
						speakerowerStatus = JV_SPEAKER_OWER_VOICE;
						mvoicedec_enable(); 
					}
					if(utl_ifconfig_bsupport_apmode(sWifiModel))
					{
						printf("**************AP配置**************\n");
						utl_ifconfig_wifi_start_ap();	//耗时操作
					}
					else
					{
						printf("**************智联路由**************\n");
						utl_ifconfig_wifi_smart_connect();
					}
				}
			}
			else if(!bLinkDown)
			{
				printf("**************set net to eth0**************\n");
				//防止造成的无线有线都有IP地址
				if(__bSupport_WIFI_check())
				{
					if(utl_ifconfig_bsupport_smartlink(sWifiModel) && (smart_connecting)) //7601wifi插上网线后如果直连路由或者声波配置开启则关闭
					{
						utl_ifconfig_wifi_smart_connect_close(TRUE);	
					}
					if(isVoiceSetting())			//任何wifi模块如果声波配置开启中则关闭
					{
						mvoicedec_disable();
					}
					__wifi_STA_stop();
				}
				eth_t eth;
				char inet[12];
				char iface[12];
				utl_ifconfig_eth_get(&eth);
				_utl_read_interface(iface, inet, eth.addr,eth.mask,eth.gateway,eth.dns);
				if(!strncmp(iface,"eth0",4))//没有切换网卡的情况
				{
					// if(bOldLinkDown != -1)		// 加了这个判断会导致特定条件下，设备无法获取IP的问题
					{
						if(!strncmp(inet,"dhcp",4))//dhcp重新获取，static无动作
						{
							printf("**************eth0 setting dhcp**************\n");
							eth.bDHCP = TRUE;
							utl_ifconfig_eth_set(&eth);
							if(bOldLinkDown != -1)
							{
								mlog_write("Set Ethernet by Link Down Up : %s","DHCP");
							}
						}
					}
				}
				else//切换无线网卡的情况
				{
					net_deinit();
					if(!strncmp(inet,"dhcp",4))//拔掉网线之前是DHCP
					{
						printf("**************eth0 setting dhcp**************\n");
						mlog_write("Set Ethernet by Link Down Up : %s","DHCP");
						eth.bDHCP = TRUE;
						__ifconfig_eth_set(&eth);
					}
					else if(!strncmp(inet,"static",6))//拔掉网线之前是static
					{
						printf("**************eth0 setting static**************\n");
						printf("name:%s\ndhcp:%d\nip:%s\nmask:%s\ngw:%s\n", eth.name,
								eth.bDHCP, eth.addr, eth.mask, eth.gateway);
						mlog_write("Set Ethernet by Link Down Up : %s",eth.addr);
						__ifconfig_eth_set(&eth);
					}
				}

				if(__bSupport_WIFI_check())
					utl_ifconfig_wifi_start_sta();//放在有线获取DHCP后边，延时太大放在前边会阻塞有线获取IP地址。
			}
			bOldLinkDown = bLinkDown;
		}
		first = 1;
		if(!utl_ifconfig_bSupport_ETH_check())
		{
			return NULL;
		}
		sleep(1);
	}
	printf("out of _netlink_check_process\n");
	return NULL;
}
static int wifi_init()
{
	wifiap_t w;
	memset(sPowerAPlist, 0, sizeof(sPowerAPlist));

	utl_ifconfig_wifi_get_model();

	//此处：不支持WiFi的机器，会用到_netlink_check_process线程中的部分功能：自动获取IP地址的时候插拔网线需要重新获取IP地址，所以也需要启动该线程LK
	if(__bSupport_WIFI_check())
		check_SD_netCfg();

	if(netlink_check_group.running == FALSE)
	{
		netlink_check_group.running = TRUE;
		pthread_create_normal(&netlink_check_group.thread, NULL, (void *)_netlink_check_process, NULL);
	}
	return 0;
}

/**************************check wifi end *********************************/
static int sCheckTimeout=12;
static int bPrepared = 0;

static BOOL net_init_check(int tid, void *param)
{
	eth_t eth,wifi;
	int eth_tile,net_state,i;
	char inet[20];
	char eth_ip[50]={0},cmd[200]={0};
	ipcinfo_t ipcinfo;
	bPrepared = 0;

	memset(&wifi,0,sizeof(eth_t));
	if(0 == strcmp(utl_ifconfig_get_inet(inet), "wifi") &&wifi_setting == 0)
	{
		utl_ifconfig_build_attr("wlan0", &wifi, TRUE);
		if(strlen(wifi.addr)>0)	//wifi DHCP获取到IP了
		{
			printf("@@@@@@@@@@@Check  WIFI IP: %s  CheckTimeout=%d\n ",wifi.addr,sCheckTimeout); 
			sCheckTimeout = 0;
		}
		printf("@@@@@@@@@@@Check  WIFI IP: %s \n ",wifi.addr); 
	}
	else  if(0 == strcmp(utl_ifconfig_get_inet(inet), "dhcp") )
    	{
		utl_ifconfig_build_attr("eth0", &eth, TRUE);
		if(strlen(eth.addr)>0)	//wifi DHCP获取到IP了
		{
			printf("@@@@@@@@@@@Check  ETH0  IP: %s  CheckTimeout=%d\n ",eth.addr,sCheckTimeout); 
			sCheckTimeout = 0;
		}
		printf("@@@@@@@@@@@Check  ETH0 IP: %s \n ",eth.addr); 

		
    	}
	sCheckTimeout--;
	if(sCheckTimeout <= 0)
	{
	    printf("15 s has gone\n");
		{
	    	char iface[20];
	    	utl_ifconfig_get_iface(iface);
#ifdef YST_SVR_SUPPORT
			if (gp.bNeedYST)
			{
				JVN_SetLocalNetCard(iface);//设置网卡
			}
#endif
	    	utl_ifconfig_build_attr(iface, &eth, FALSE);
	        if((inet_addr(eth.addr) == INADDR_NONE))
	        {
				if (utl_ifconfig_bSupport_ETH_check() == TRUE &&
					utl_ifconfig_b_linkdown("eth0") == FALSE)
				{
					printf("no ip set\n");
		            //这里只改IP，但不保存。不能保存。不能影响下次开机正常获取IP
		            ipcinfo_get_param(&ipcinfo);
		            eth_tile= (ipcinfo.sn % 254)+1;
		            sprintf(eth_ip,"192.168.1.%d",eth_tile);
		            sprintf(cmd,"ifconfig %s %s", "eth0",eth_ip);
		            utl_system(cmd);
		            utl_system("route add default gw 192.168.1.1");

		            printf("set eth0:%s\n",eth_ip);

		            utl_system("echo \"nameserver 192.168.1.1\" > /etc/resolv.conf");
		            utl_system("route add -net 255.255.255.255 netmask 255.255.255.255 dev eth0");
		            mlog_write("Failed get ip, set as default: %s", eth_ip);
				}
	        }
		}
	    utl_timer_destroy(timer_ip);
	    timer_ip = -1;
	    bPrepared = 1;

		__onvif_reset_with_timer();

	    if(0 == strcmp(utl_ifconfig_get_inet(inet), "wifi") &&
			wifi_setting == 0)
	    {
	    	__StartWifiAliveCheck();
	    }

	    return 0;
	}
	utl_timer_reset(timer_ip, 2*1000, (utl_timer_callback_t)net_init_check, NULL);
	return 0;
}

/**
 *@brief 检查网络是否准备好
 *
 *@return 0 未准备好
 *@return >0 已准备好
 */
int utl_ifconfig_net_prepared(void)
{
	return bPrepared;
}

/**
 *@brief 连接是否断开
 */
BOOL utl_ifconfig_b_linkdown(const char *if_name)
{
	if(!utl_ifconfig_bSupport_ETH_check())
		return TRUE;
	
	int b=get_netlink_status("eth0");
	if(b==-1)
		return b;

	return !b;
}

static void __utl_reset_check()
{
	char inet[20];
	utl_ifconfig_get_inet(inet);
	printf("check inet: %s\n", inet);
	if (0 == strcmp(inet, "wifi"))
	{
		sCheckTimeout=24;
	}
	else if (0 == strcmp(inet, "dhcp"))
	{
		sCheckTimeout=10;
	}
	else //for static and ppp, no need to wait.
	{
		sCheckTimeout=3;  //有TF卡时，设置2会导致重启，具体原因还不清楚，3没问题，暂时现改为3
	}
	if (timer_ip == -1)
		timer_ip=utl_timer_create("net_init timer", 2*1000, (utl_timer_callback_t)net_init_check, NULL);
	else
		utl_timer_reset(timer_ip, 2*1000, (utl_timer_callback_t)net_init_check, NULL);
}

void net_deinit()
{
	__StopWifiAliveCheck();
}

void net_check_destroy()
{
	__StopWifiAliveCheck();

	if(netlink_check_group.running)
	{
		netlink_check_group.running = FALSE;
		pthread_join(netlink_check_group.thread,NULL);
	}

	smart_closed = FALSE;
	utl_WaitTimeout(!wifi_setting, 1000);

	utl_fcfg_close(NETWORK_INTERFACES_FILE);
}

void net_init(unsigned int macBase)
{
	if (utl_ifconfig_bSupport_ETH_check() == FALSE)
	{
		//H210这种没有网口的设备，关闭eth0
		utl_system("ifconfig eth0 down");
		utl_system("killall udhcpc");
	}
	//WIFI 相关初始化
	wifi_init();

	//mac check and set
	do
	{
		FILE *fp;
		char cmd[128];
		unsigned char *p = (unsigned char *)&macBase;
		//printf("mac base: %x\n", macBase);
		sprintf(cmd, "ifconfig eth0 down;ifconfig eth0 hw ether E0:62:90:%02X:%02X:%02X;ifconfig eth0 up", p[0], p[1], p[2]);

		char oldCmd[128];
		fp = fopen(NETWORK_MAC_FILE, "rb");
		if (fp)
		{
			fgets(oldCmd, sizeof(oldCmd), fp);
			fclose(fp);
			//printf("getted: [%s], cmd: [%s]\n", oldCmd, cmd);
			if (0 == strcmp(cmd, oldCmd))
			{
				//printf("the same\n");
				break;
			}
		}
		printf("not same, now saving\n");
		fp = fopen(NETWORK_MAC_FILE, "wb");
		if (fp)
		{
			fputs(cmd, fp);
			fclose(fp);
		}
		utl_system(". "NETWORK_MAC_FILE);
		utl_system("/progs/networkcfg.sh"); //ifconfig down时被清掉路由表
		if (utl_ifconfig_bSupport_ETH_check() == FALSE)
		{
			//H210这种没有网口的设备，关闭eth0
			utl_system("ifconfig eth0 down");
			
			utl_system("killall udhcpc");
			
		}
	}while(0);
	memset(&wifilist, 0, sizeof(wifilist));
	__utl_reset_check();
}
