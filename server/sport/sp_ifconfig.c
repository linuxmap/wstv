/*
 * sp_ifconfig.c
 *
 *  Created on: 2013-11-20
 *      Author: LK
 */
#include "sp_define.h"
#include <jv_common.h>
#include "sp_ifconfig.h"
#include "utl_ifconfig.h"
#include "iwlist.h"
#include <mlog.h>
#include "malarmout.h"
#include "SYSFuncs.h"
/**
 *@brief 获取当前使用的网络类型
 *
 *@param name 当前使用的网络的类型：static/dhcp/ppp/wifi
 *
 *@return 返回inet指针
 */
char *sp_ifconfig_get_inet(char *inet)
{
	return utl_ifconfig_get_inet(inet);
}

/**
 *@brief 获取当前使用的网络设备
 *
 *@param name 当前使用的网络设备名称：eth0/wlan0/ppp
 *
 *@return 返回iface指针
 */
char *sp_ifconfig_get_iface(char *iface)
{
	return utl_ifconfig_get_iface(iface);
}

/**
 *@brief 获取PPPOE的连接参数
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_ppp_get(SPPppoe_t *ppp)
{
	pppoe_t pppoe;
	int ret;
	ret = utl_ifconfig_ppp_get(&pppoe);
	if (ret != 0)
		return -1;
	strncpy(ppp->name, pppoe.name, 12);
	strncpy(ppp->username, pppoe.username, 32);
	strncpy(ppp->passwd, pppoe.passwd, 32);

	return 0;
}

/**
 *@brief 获取连接的热点
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_wifi_get_current(SPWifiAp_t *wifiap)
{
	wifiap_t uwifiap;
	int ret = utl_ifconfig_wifi_get_current(&uwifiap);
	if (0 != ret)
		return -1;
	strncpy(wifiap->iestat, uwifiap.iestat, 8);
	wifiap->keystat = uwifiap.keystat;
	strncpy(wifiap->name, uwifiap.name, 32);
	strncpy(wifiap->passwd, uwifiap.passwd, 16);
	wifiap->quality = uwifiap.quality;

	return 0;
}

/**
 *@brief 获取ETHERNET的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_eth_get(SPEth_t *eth)
{
	eth_t ueth;
	char inet[32];
	int ret = -1;
	utl_ifconfig_get_inet(inet);
	if (0 == strcmp(inet, "wifi"))
	{
		ret = utl_ifconfig_wifi_get(&ueth);
	}
	else if (0 == strcmp(inet, "ppp"))
	{
		ret = -1;
	}
	else
	{
		ret = utl_ifconfig_eth_get(&ueth);
	}

	if (0 != ret)
		return -1;
	eth->bDHCP = ueth.bDHCP;
	strncpy(eth->addr, ueth.addr, 16);
	strncpy(eth->dns, ueth.dns, 16);
	strncpy(eth->gateway, ueth.gateway, 16);
	strncpy(eth->mac, ueth.mac, 20);
	strncpy(eth->mask, ueth.mask, 16);
	strncpy(eth->name, ueth.name, 12);

	return 0;
}

/**
 *@brief 获取WIFI的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_wifi_get(char *iface, SPEth_t *spEth)
{
	eth_t eth;
	int ret = 0;
	ret = utl_ifconfig_build_attr(iface, &eth, FALSE);
	if(ret != 0)
		return -1;
	strcpy(spEth->name, eth.name);
	spEth->bDHCP = eth.bDHCP;
	strcpy(spEth->addr, eth.addr);
	strcpy(spEth->mask, eth.mask);
	strcpy(spEth->gateway, eth.gateway);
	strcpy(spEth->mac, eth.mac);
	strcpy(spEth->dns, eth.dns);
	return 0;
}


/**
 *@brief 设置PPPOE的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_ppp_set(SPPppoe_t *ppp)
{
	pppoe_t upppoe;
	int ret;

	strncpy(upppoe.name, ppp->name, 12);
	strncpy(upppoe.username, ppp->username, 32);
	strncpy(upppoe.passwd, ppp->passwd, 32);

	ret = utl_ifconfig_ppp_set(&upppoe);
	if (0 != ret)
		return -1;

	return 0;
}

/**
 *@brief 设置ETHERNET的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_eth_set(SPEth_t *eth)
{
	eth_t ueth;
	int ret;

	ueth.bDHCP = eth->bDHCP;
	strncpy(ueth.addr, eth->addr, 16);
	strncpy(ueth.dns, eth->dns, 16);
	strncpy(ueth.gateway, eth->gateway, 16);
	strncpy(ueth.mac, eth->mac, 20);
	strncpy(ueth.mask, eth->mask, 16);
	strncpy(ueth.name, eth->name, 12);

	ret = utl_ifconfig_eth_set(&ueth);
	mlog_write("Set EthIP by Server Port:%s",ueth.bDHCP?"DHCP":ueth.addr);
	if (0 != ret)
		return -1;

	return 0;
}

//开启wifi STA模式
int sp_ifconfig_wifi_start_sta()
{
	utl_ifconfig_wifi_start_sta();
	return 0;
}

//开启wifi AP模式
int sp_ifconfig_wifi_start_ap()
{
	utl_ifconfig_wifi_start_ap();
	return 0;
}

void sp_ifconfig_net_deinit()
{
	net_deinit();
}


/**
 *@brief 连接指定热点
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_wifi_connect(SPWifiAp_t *ap)
{
	wifiap_t uap;
	int ret;

	strncpy(uap.name, ap->name, 32);
	strncpy(uap.passwd, ap->passwd, 64);
	uap.quality = ap->quality;
	uap.keystat = ap->keystat;
	strncpy(uap.iestat, ap->iestat, 8);

	ret = utl_ifconfig_wifi_connect(&uap);
	if (0 != ret)
		return -1;

	return 0;
}

/**
 *@brief 获取WIFI热点列表---暂时先不做lk20131121
 *
 *@return NULL 失败,否则成功
 */
SPWifiAp_t *sp_ifconfig_wifi_list_ap()
{
	wifiap_t *p = NULL;
	p = utl_ifconfig_wifi_get_ap();
	if(p == NULL)
		return NULL;
	return (SPWifiAp_t *)p;
}

/**
 *@brief 计算wifi热点列表数量---暂时先不做lk20131121
 *
 *@param list 要计算的列表
 *
 *@return 热点个数个数
 */
int sp_ifconfig_wifi_list_cnt(SPWifiAp_t *list)
{
	int cnt_p = 0;
	if(list == NULL)
		return 0;
	cnt_p = utl_ifconfig_wifi_list_cnt((wifiap_t *)list);
	return cnt_p;
}

//获得设备配置的STA信息
int sp_ifconfig_wifi_info_get(SPWifiAp_t *ap)
{
	int ret = 0;
	if (ap == NULL)
	{
		return -1;
	}
	ret = utl_ifconfig_wifi_info_get((wifiap_t *)ap);
	return ret;
}

//获得服务器配置信息
int sp_ifconfig_server_get(SPServer_t *serverInfo)
{
	ALARMSET alarm;
	if(serverInfo == NULL)
		return -1;
	malarm_get_param(&alarm);
	strcpy(serverInfo->vmsServerIp, alarm.vmsServerIp);
	serverInfo->vmsServerPort = alarm.vmsServerPort;
	return 0;
}

//配置服务器信息
int sp_ifconfig_server_set(SPServer_t *serverInfo)
{
	ALARMSET alarm;
	if(serverInfo == NULL)
		return -1;
	malarm_get_param(&alarm);
	strcpy(alarm.vmsServerIp, serverInfo->vmsServerIp);
	alarm.vmsServerPort = serverInfo->vmsServerPort;
	malarm_set_param(&alarm);
	WriteConfigInfo();
	return 0;
}


#include <sys/sysinfo.h>

#include <mipcinfo.h>
#include <utl_ipscan.h>
#include "sp_connect.h"
#include <utl_net_lan.h>

static BOOL __sp_ifconfig_b_ipselfadpt()
{
	//开机2分钟内，不响应自动获取IP
	{
		struct sysinfo sinfo;
	    sysinfo(&sinfo);
//	    printf("uptime: %ld\n", sinfo.uptime);
	    if (sinfo.uptime < 60)
	    	return FALSE;
	}

    ipcinfo_t ipcinfo;
	ipcinfo_get_param(&ipcinfo);
	return FALSE;
}

/**
 *@brief 根据给定的IP地址，自适应设置一个同网段的IP地址
 *
 *@param otherip IP地址的来源。目前是其他客户端搜索时的IP
 *
 *@return 0
 */
int sp_ifconfig_audo_ip(const char *otherip)
{
	const char *ipstr = otherip;
	unsigned int addrList[255];
	int cnt;
	eth_t eth;
	char iface[20];
	if (!__sp_ifconfig_b_ipselfadpt())
		return 0;

//	if (ipcinfo_get_type() == IPCTYPE_SW)
//	{
////		printf("hardware version not support\n");
//		return 0;
//	}

	cnt = sp_connect_get_cnt(SP_CON_ALL);
	if (cnt > 0)
	{
//		printf("ipc had been connected.\n");
		return 0;
	}

	utl_ifconfig_get_iface(iface);
	utl_ifconfig_build_attr(iface, &eth, FALSE);
    if((inet_addr(eth.addr) == INADDR_NONE))
    {
    	printf("ip had not ready now...\n");
    	return 0;
    }
//	if (eth.addr)
	unsigned int server, local, mask;
	server = inet_network(ipstr);
	local = inet_network(eth.addr);
	mask = inet_network(eth.mask);
	if ((server & mask) == (local & mask))
	{
//		printf("same network\n");
		return 0;
	}

//	printf("not same network, server: %x, local: %x, mask : %x\n", server, local, mask);

	unsigned char ipEnd = random() % 255;
	unsigned char serverEnd = server & 0xFF;
	ipEnd += serverEnd;
	if (ipEnd == serverEnd)
		ipEnd += 10;
	if (ipEnd == 1 || ipEnd == 255)
		ipEnd += 10;

	unsigned int tempIP;
	tempIP = server & 0xFFFFFF00;
	tempIP |= ipEnd;
	struct in_addr addr;
	addr.s_addr = htonl(tempIP);
	char tempAddr[16];
	inet_ntop(AF_INET, &addr, tempAddr, sizeof(tempAddr));
//	printf("now ip addr: %s\n", tempAddr);
	char setip[128];
	sprintf(setip, "ifconfig %s %s", iface, tempAddr);
	utl_system(setip);

	usleep(300*1000);
//	printf("now check ipaddr\n");

	cnt = utl_ipscan_not_used(tempAddr, 2000, addrList, 255);
	printf("cnt: %d\n", cnt);
	if (cnt > 0)
	{
		int index = random() % cnt;
//		printf("index: %d\n", index);
		addr.s_addr = addrList[index];
		inet_ntop(AF_INET, &addr, eth.addr, sizeof(eth.addr));
		{
			unsigned int gw = ntohl(addr.s_addr);
			gw &= 0xFFFFFF00;
			gw |= 1;
			addr.s_addr = htonl(gw);
			inet_ntop(AF_INET, &addr, eth.gateway, sizeof(eth.gateway));
			eth.bDHCP = 0;
			printf("Now set ip as: %s\n", eth.addr);
			printf("route: %s\n", eth.gateway);
			utl_ifconfig_eth_set(&eth);
			return 0;
		}
	}

	printf("no ip to use\n");
	sprintf(setip, "ifconfig %s %s", iface, eth.addr);
	utl_system(setip);

	return -1;
}

