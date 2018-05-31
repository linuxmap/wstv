/*
 * sp_ifconfig.h
 *
 *  Created on: 2013-11-20
 *      Author: Administrator
 */

#ifndef SP_IFCONFIG_H_
#define SP_IFCONFIG_H_

/*定义网络类型*/
#define   ETH_NET			0x01
#define   PPPOE_NET			0x02
#define   WIFI_NET			0x03
#define   PPPOE_LINKING		0X04
#define   WIFI_LINKING		0X05

#ifdef __cplusplus
extern "C" {
#endif

//以太网网络
typedef struct
{
	char name[12];            	//网络名
	int  bDHCP;               	//以太网自动获取或者手动设置标志位
	char addr[16];            	//IP地址
	char mask[16];				//子网掩码
	char gateway[16];			//网关
	char mac[20];           	//MAC地址，添加此信息防止有的路由器，设置MAC地址和IP地址绑定
	char dns[16];				//DNS
}SPEth_t;

//pppoe网络
typedef struct
{
	char name[12];

	char username[32];
	char passwd[32];
}SPPppoe_t;

//wifi 网络
typedef struct
{
	char	name[32];       	//AP名
	char	passwd[64];   		//历史记录的密码   如果为空表示历史没有记录这个节点
	int		quality;	     	//信号强度，满值100
	int		keystat;     		//是否需要密码  -1 不需要 其他值均需要密码，此处暂时先只判断不需要密码的情况
	char	iestat[8];	     	//iestat[0]:安全模式 对应AUTH_TYPE_e；iestat[1]：加密类型（规则）对应 ENCODE_TYPE_e
}SPWifiAp_t;

//服务器信息
typedef struct
{
	/*VMS*/
	char vmsServerIp[20];	//VMS服务器IP地址
	unsigned short vmsServerPort;		//VMS服务器端口

	/*RTMP*/
	int bRTMPEnable;
	int rtmpChannel;
	char rtmpURL[128];
}SPServer_t;

/**
 *@brief 获取当前使用的网络类型
 *
 *@param name 当前使用的网络的类型：static/dhcp/ppp/wifi
 *
 *@return 返回inet指针
 */
char *sp_ifconfig_get_inet(char *inet);

/**
 *@brief 获取当前使用的网络设备
 *
 *@param name 当前使用的网络设备名称：eth0/wlan0
 *
 *@return 返回iface指针
 */
char *sp_ifconfig_get_iface(char *iface);

/**
 *@brief 获取ETHERNET的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_eth_get(SPEth_t *eth);

/**
 *@brief 设置ETHERNET的连接----配置本地连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_eth_set(SPEth_t *eth);

/**
 *@brief 获取PPPOE的连接参数
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_ppp_get(SPPppoe_t *ppp);

/**
 *@brief 配置PPPOE的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_ppp_set(SPPppoe_t *ppp);

/**
 *@brief 获取wifi连接参数
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_wifi_get_current(SPWifiAp_t *wifiap);

/**
 *@brief 获取WIFI的连接
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_wifi_get(char *iface, SPEth_t *spEth);

/**
 *@brief 连接指定热点
 *
 *@return 0 成功，< 0 失败
 */
int sp_ifconfig_wifi_connect(SPWifiAp_t *ap);

/**
 *@brief 获取WIFI热点列表 
 *
 *@return 0 成功，< 0 失败
 */
SPWifiAp_t *sp_ifconfig_wifi_list_ap();

/**
 *@brief 计算wifi热点列表数量
 *
 *@param list 要计算的列表
 *
 *@return 热点个数个数
 *
 */
int sp_ifconfig_wifi_list_cnt(SPWifiAp_t *list);

//获得设备配置的STA信息
int sp_ifconfig_wifi_info_get(SPWifiAp_t *ap);

//开启wifi STA模式
int sp_ifconfig_wifi_start_sta();

//开启wifi AP模式
int sp_ifconfig_wifi_start_ap();

//网络重新初始化
void sp_ifconfig_net_deinit();

/*获取服务器配置，包括VMS, RTMP等服务器的地址信息等*/
int sp_ifconfig_server_get(SPServer_t *serverInfo);

/*服务器配置，包括VMS, RTMP等服务器的地址信息等*/
int sp_ifconfig_server_set(SPServer_t *serverInfo);

/**
 *@brief 根据给定的IP地址，自适应设置一个同网段的IP地址
 *
 *@param otherip IP地址的来源。目前是其他客户端搜索时的IP
 *
 *@return 0
 */
int sp_ifconfig_audo_ip(const char *otherip);



#ifdef __cplusplus
}
#endif

#endif /* SP_IFCONFIG_H_ */
