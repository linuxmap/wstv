#ifndef _IWLIST_H_
#define _IWLIST_H_

typedef enum
{
	AUTH_TYPE_OPEN,			//wep_open
	AUTH_TYPE_SHARED,		//wep_shared
	AUTH_TYPE_AUTO,			//wep_auto..
	AUTH_TYPE_WPA,			//wpa-psk
	AUTH_TYPE_WPA2,			//wpa2-psk
	AUTH_TYPE_NONE,			//明文
	AUTH_TYPE_MAX
}AUTH_TYPE_e;

typedef enum
{
	ENCODE_TYPE_NONE,		//明文
	ENCODE_TYPE_WEP,
	ENCODE_TYPE_TKIP,		//wpa
	ENCODE_TYPE_AES,
	ENCODE_TYPE_MAX
}ENCODE_TYPE_e;

typedef struct  
{
	char			name[32];       	//AP名
	char			passwd[16];	   		//历史记录的密码   如果为空表示历史没有记录这个节点
	int				quality;	     	//信号强度，满值100
	int				keystat;     		//0不需要密码，非0需要密码
//	AUTH_TYPE_e		auth_type;			//安全模式支持明文、wpa-psk、wpa2-psk、open、shared
//	ENCODE_TYPE_e 	encode_type;		//加密类型（规则）对应关系：明文：none；wpa-psk,wpa2-psk:TKIP,AES(CCMP);open,shared:WEP
	char			iestat[8];	     	//iestat[0]:安全模式 对应AUTH_TYPE_e；iestat[1]：加密类型（规则）对应 ENCODE_TYPE_e
}wifiapOld_t;

typedef struct  
{
	char			name[32];       	//AP名
	char			passwd[64];	   		//历史记录的密码   如果为空表示历史没有记录这个节点
	int				quality;	     	//信号强度，满值100
	int				keystat;     		//0不需要密码，非0需要密码
//	AUTH_TYPE_e		auth_type;			//安全模式支持明文、wpa-psk、wpa2-psk、open、shared
//	ENCODE_TYPE_e 	encode_type;		//加密类型（规则）对应关系：明文：none；wpa-psk,wpa2-psk:TKIP,AES(CCMP);open,shared:WEP
	char			iestat[8];	     	//iestat[0]:安全模式 对应AUTH_TYPE_e；iestat[1]：加密类型（规则）对应 ENCODE_TYPE_e
										//iestat[4]:信号强度，8188模块的quality存在问题，需要用此字段做level，但此结构不可增加字段，只好先如此对应
}wifiap_t;

//说明:wifiapOld_t只用作给分控传输wifi列表用(兼容现有分控)，其他均用wifiap_t


//以太网网络
typedef struct 
{
	int		flags;               //以太网自动获取或者手动设置标志位
	char	name[12];            //网络名
	char	Gateway[16];		  
	char	IP[16];              //IP地址
	char	Mac[20];             //MAC地址，添加此信息防止有的路由器，设置MAC地址和IP地址绑定
	char	DNS[16];			  
	char	Netmask[16];
}ETHERNET;

//pppoe网络
typedef struct 
{
	int		flags;
	int		state;
	char	name[12];
	char	Gateway[16];
	char	IP[16];
	char	Mac[20];
	char	DNS[16];
	char	Netmask[16];
	
	char	username[32];
	char	passwd[32];
}PPPOE;

//wifi网络
typedef struct
{
	char	name[12];
	int		state;
	char	Gateway[16];
	char	IP[16];
	char	Mac[20];
	char	DNS[16];
	char	Netmask[16];
    int		current_ap;      //当前连接的wifi节点在wifiap中的位置 从零开始
	wifiap_t	wifiap[100];
}WIFINET;

//ipc 所有网络
typedef struct 
{
	int			net_alive;        //当前网络激活标志，初始为0，ETHERNET为0x1，PPPOE为0x2，WIFI为0x3 

	ETHERNET	stETH;		//以太网配置信息,lck20120529
	ETHERNET	ethernet;    //以太网
	PPPOE		pppoe;          //pppoe
	WIFINET		wifi;         //wifi

	int			bRunning;
	int			bInThread;
}IPC_NET;

extern IPC_NET ipc_net;


int iwlist_get_ap_list(wifiap_t *aplist, char* wifi, int maxApCnt);

#endif

