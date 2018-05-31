#ifndef _UTL_IFCONFIG_H_
#define _UTL_IFCONFIG_H_

#include "iwlist.h"

#define 	PACKET_SIZE 4096
#define 	ROUTE_LINE_SIZE 77

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
	char name[12];            //网络名
	int  bDHCP;               //以太网自动获取或者手动设置标志位
	char addr[16];              //IP地址
	char	mask[16];
	char gateway[16];		  
	char mac[20];             //MAC地址，添加此信息防止有的路由器，设置MAC地址和IP地址绑定
	char dns[16];
}eth_t;


//pppoe网络
typedef struct 
{
	char name[12];
	
	char username[32];
	char passwd[32];
}pppoe_t;

typedef enum
{
	WIFI_MODE_STA=1,
	WIFI_MODE_AP=2
}wifi_mode_e;

typedef enum
{
	WIFI_MODEL_UNKNOWN,
	WIFI_MODEL_REALTEK8188,
	WIFI_MODEL_REALTEK8192,
	WIFI_MODEL_RALINK7601,
	WIFI_MODEL_SCI9083,
	WIFI_MODEL_M88WI6700,

	WIFI_MODEL_NONE,
}WifiModelType;


/**
 *@brief 获取指定网络设备名的IP信息
 *
 *@param ifname 网络设备名，如eth0/ra0
 *@param attr IP信息
 *
 */
int utl_ifconfig_build_attr(char *ifname, eth_t *attr, BOOL bRefresh);

//获得上电搜索的AP热点列表
wifiap_t* utl_ifconfig_wifi_power_list_ap();

/**
 *@brief 获取WIFI热点列表
 *
 *@param bResearch 是否强制重新扫描
 *
 *@return 0 成功，< 0 失败 
 *
 */
wifiap_t *utl_ifconfig_wifi_list_ap(int bResearch);

//获取WIFI热点列表
wifiap_t *utl_ifconfig_wifi_get_ap();

int utl_ifconfig_wifi_bsupport();

/*
 * @brief 开启AP模式
 */
int utl_ifconfig_wifi_start_ap();

/*
 * @brief 开启STA模式
 */
int utl_ifconfig_wifi_start_sta();

/**
 *@brief 计算wifi热点列表数量
 *
 *@param list 要计算的列表
 *
 *@return 热点个数个数
 *
 */
int utl_ifconfig_wifi_list_cnt(wifiap_t *list);
/**
 * @brief 保存wifi的配置文件         原来是在wifi_connect里边，但是保存文件之后重启不需要连接
 *                             所以单独拿出来。
 * @param ap要连接的ap信息
 * @return 0成功
 */
int utl_ifconfig_wifi_conf_save(wifiap_t *ap);
/**
 * @brief 删除wifi的配置文件
 * @param
 * @return 0成功  -1失败
 */
int utl_ifconfig_wifi_save_remove();

//获得目前使用哪种wifi模块
WifiModelType utl_ifconfig_wifi_get_model();

//获得目前wifi的使用模式
wifi_mode_e utl_ifconfig_wifi_get_mode();

//变换wifi的使用模式
int utl_ifconfig_wifi_mode_change();

//变换AP模式前，使用STA进行搜索
void utl_ifconfig_wifi_power_search();

/**
 *@brief 连接指定热点
 *
 *@return 0 成功，< 0 失败 
 */
int utl_ifconfig_wifi_connect(wifiap_t *ap);

/**
 *@brief 根据SSID获取热点结构体
 *
 *
 */
wifiap_t *utl_ifconfig_wifi_get_by_ssid(char *ssid);

/**
 *@brief 获取连接的热点
 *
 *@return 0 成功，< 0 失败
 */
int utl_ifconfig_wifi_get_current(wifiap_t *wifiap);

/**
 *@brief 设置ETHERNET的连接
 *
 *@return 0 成功，< 0 失败 
 */
int utl_ifconfig_eth_set(eth_t *eth);

/**
 *@brief 获取ETHERNET的连接
 *
 *@return 0 成功，< 0 失败 
 */
int utl_ifconfig_eth_get(eth_t *eth);

/**
 *@brief 获取wifi的连接
 *
 *@return 0 成功，< 0 失败
 */
int utl_ifconfig_wifi_get(eth_t *wifi);

//存储用户配置的wifi信息
int utl_ifconfig_wifi_info_save(wifiap_t *ap);

//获取用户配置的wifi信息
int utl_ifconfig_wifi_info_get(wifiap_t *ap);

//清除wifi STA的IP地址
void utl_ifconfig_wifi_sta_ip_clear();

//wifi WPS
void utl_ifconfig_wifi_WPS_start();

//smart connect
void utl_ifconfig_wifi_smart_connect();

//smart close
void utl_ifconfig_wifi_smart_connect_close(BOOL bBlock);

//get wether received wifi info
BOOL utl_ifconfig_wifi_smart_get_recvandsetting();

// 尝试连接热点，如果ap中的安全模式、加密类型为-1，则先搜索后连接，否则直接连接
// bInConfig表示是否在WiFi配置中，TRUE表示正在配置，会有停止配置和连接失败重启配置的操作
// 返回值：0，成功，<0，失败
int utl_ifconfig_wifi_connect_ap(wifiap_t* ap, BOOL bInConfig);

/**
 *@brief 设置PPPOE的连接
 *
 *@return 0 成功，< 0 失败 
 */
int utl_ifconfig_ppp_set(pppoe_t *ppp);

/**
 *@brief 获取PPPOE的连接参数
 *
 *@return 0 成功，< 0 失败 
 */
int utl_ifconfig_ppp_get(pppoe_t *ppp);

/**
 *@brief 获取网络的状态
 *@param iface 网络名
 *
 *@return 1 连接成功，  0 连接失败
 *
 */
int utl_ifconfig_check_status(char *iface);

/**
 *@brief 设置当前使用的网络
 *
 *@param name 当前使用的网络的名称：eth0/ra0/ppp0
 *
 *@return 0 成功，< 0 失败 
 *
 */
//int utl_ifconfig_set_current(char *name);

/**
 *@brief 获取当前使用的网络类型
 *
 *@param name 当前使用的网络的类型：static/dhcp/ppp/wifi
 *
 *@return 返回inet指针 
 *
 */
char *utl_ifconfig_get_inet(char *inet);

char *utl_ifconfig_get_iface(char *iface);

/**
 *@brief 检查网络是否准备好.暂未完善
 *
 *@return 0 未准备好
 *@return >0 已准备好
 */
int utl_ifconfig_net_prepared(void);

void net_init(unsigned int macBase);
void net_deinit();
void net_check_destroy();

/**
 *@brief 等待网络准备好，主要是DHCP时的IP地址
 *
 *@param timeoutSec 超时时间，单位为秒
 *
 *@return 0 成功， -1 未能成功等到
 */
int utl_ifconfig_waiting_network_prepare(int timeoutSec);

/**
 *@brief 连接是否断开
 */
BOOL utl_ifconfig_b_linkdown(const char *if_name);

int __bNetCard_Exist(char *ifname,int bAll);

//设备是否支持网口
BOOL utl_ifconfig_bSupport_ETH_check();

BOOL utl_ifconfig_bsupport_apmode();

BOOL utl_ifconfig_bsupport_smartlink();

// 设备是否已配置sta
int utl_ifconfig_wifi_STA_configured();

#ifdef __cplusplus
}
#endif

#endif

