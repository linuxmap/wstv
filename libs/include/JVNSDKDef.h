#ifndef _JVNSDKDEF_H
#define _JVNSDKDEF_H

#ifndef WIN32
   #include <sys/socket.h>
   #include <netinet/in.h>
   typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
    } GUID;
    #define MAX_PATH 256
	typedef  struct sockaddr_in  SOCKADDR_IN;
	typedef  struct sockaddr  SOCKADDR;
    #define FALSE 0
    #define TRUE  1
    
#else
	#include <winsock2.h>	
#endif

/*协议版本号*/
#define JVN_YSTVER6 20170214//协议版本 该版本增加了 新远程回放请求(json格式），新的seek指令 新远程检索指令 
#define JVN_YSTVER5 20160707//协议版本 该版本发送时间戳  按照时间点远程回放(ini 格式) udt丢包远程控制
#define JVN_YSTVER4 20140319//协议版本 该版本采用sendmsg方式发送
#define JVN_YSTVER3 20131019//协议版本
#define JVN_YSTVER2 20130731//协议版本
#define JVN_YSTVER1 20130116//协议版本

/*实时监控数据类型*/
#define JVN_DATA_I           0x01//视频I帧
#define JVN_DATA_B           0x02//视频B帧
#define JVN_DATA_P           0x03//视频P帧
#define JVN_DATA_A           0x04//音频
#define JVN_DATA_S           0x05//帧尺寸
#define JVN_DATA_OK          0x06//视频帧收到确认
#define JVN_DATA_DANDP       0x07//下载或回放收到确认
#define JVN_DATA_O           0x08//其他自定义数据
#define JVN_DATA_SKIP        0x09//视频S帧
#define JVN_DATA_SPEED		 0x64//主控码率
#define JVN_DATA_HEAD        0x66//视频解码头，该数据出现的同时将清空缓存
#define JVN_DATA_VI          0x16 // 虚拟I帧
/*请求类型*/
#define JVN_REQ_CHECK        0x10//请求录像检索
#define JVN_REQ_CHECK_TAL    0x3f//请求录像检索,检索时间和长度
#define JVN_REQ_DOWNLOAD     0x20//请求录像下载
#define JVN_REQ_PLAY         0x30//请求远程回放
#define JVN_REQ_CHAT         0x40//请求语音聊天
#define JVN_REQ_TEXT         0x50//请求文本聊天
#define JVN_REQ_CHECKPASS    0x71//请求身份验证
#define JVN_REQ_RECHECK      0x13//预验证
#define JVN_REQ_RATE		 0x63//分控请求码率
#define JVN_REQ_TIMEPOINT_PLAY	0x3a	//根据时间点请求远程回放
#define JVN_REQ_TIMEJSON_PLAY	0x3c //根据时间点请求远程回放，时间格式为JSON
/*请求返回结果类型*/
#define JVN_RSP_CHECKDATA    0x11//检索结果
#define JVN_RSP_CHECKOVER    0x12//检索完成
#define JVN_RSP_DOWNLOADDATA 0x21//下载数据
#define JVN_RSP_DOWNLOADOVER 0x22//下载数据完成
#define JVN_RSP_DOWNLOADE    0x23//下载数据失败

#define JVN_CMD_LOADGOON	 0x26//继续下载
#define JVN_CMD_LOADPAUSE	 0x27//暂停下载
#define JVN_CMD_UPLOADBREAK  0x28//断点续传

#define JVN_RSP_DOWNLOADSVHEAD 0x25 //下载sv5数据头
#define JVN_RSP_DOWNLOADSVDATA 0x29 //下载svr数据

#define JVN_RSP_PLAYDATA     0x31//回放数据
#define JVN_RSP_PLAYOVER     0x32//回放完成
#define JVN_RSP_PLAYE        0x39//回放失败
#define JVN_RSP_CHATDATA     0x41//语音数据
#define JVN_RSP_CHATACCEPT   0x42//同意语音请求
#define JVN_RSP_TEXTDATA     0x51//文本数据
#define JVN_RSP_TEXTACCEPT   0x52//同意文本请求
#define JVN_RSP_CHECKPASS    0x72//身份验证
#define JVN_RSP_CHECKPASST   0x72//身份验证成功 为TCP保留
#define JVN_RSP_CHECKPASSF   0x73//身份验证失败 为TCP保留
#define JVN_RSP_NOSERVER     0x74//无该通道服务
#define JVN_RSP_INVALIDTYPE  0x7A//连接类型无效
#define JVN_RSP_OVERLIMIT    0x7B//连接超过主控允许的最大数目
#define JVN_RSP_DLTIMEOUT    0x76//下载超时
#define JVN_RSP_PLTIMEOUT    0x77//回放超时
#define JVN_RSP_PLNOCONN     0x67//正在录像回放，分控不能连接
#define JVN_RSP_RECHECK      0x14//预验证
#define JVN_RSP_OLD          0x15//旧版主控回复

/*命令类型*/
#define JVN_CMD_LOADGOON	 0x26//继续下载
#define JVN_CMD_LOADPAUSE	 0x27//暂停下载
#define JVN_CMD_UPLOADBREAK  0x28//断点续传
#define JVN_CMD_DOWNLOADSTOP 0x24//停止下载数据
#define JVN_CMD_PLAYUP       0x33//快进
#define JVN_CMD_PLAYDOWN     0x34//慢放
#define JVN_CMD_PLAYDEF      0x35//原速播放
#define JVN_CMD_PLAYSTOP     0x36//停止播放
#define JVN_CMD_PLAYPAUSE    0x37//暂停播放
#define JVN_CMD_PLAYGOON     0x38//继续播放
#define JVN_CMD_CHATSTOP     0x43//停止语音聊天
#define JVN_CMD_PLAYSEEK     0x44//播放定位		按帧定位 需要参数 帧数(1~最大帧)
#define JVN_CMD_PLAYSEEK_TIME     0x3e//播放定位		按帧定位 需要参数 帧数(1~最大帧)
#define JVN_CMD_TIMEPOINT_PLAYSTOP	0x3b	//停止时间点远程回放
#define JVN_CMD_TIMEJSON_PLAYSTOP	0x3d	//停止时间点远程回放,json 格式
#define JVN_CMD_TEXTSTOP     0x53//停止文本聊天
#define JVN_CMD_YTCTRL       0x60//云台控制
#define JVN_CMD_VIDEO        0x70//实时监控
#define JVN_CMD_VIDEOPAUSE   0x75//暂停实时监控
#define JVN_CMD_TRYTOUCH     0x78//打洞包
#define JVN_CMD_FRAMETIME    0x79//帧发送时间间隔(单位ms)
#define JVN_CMD_DISCONN      0x80//断开连接
#define JVN_CMD_MOTYPE		 0x72//UDP手机类型 注：此值与下面一类型定义值相同，本应避免，暂保持这样
#define JVN_CMD_ONLYI        0x61//该通道只发关键帧
#define JVN_CMD_FULL         0x62//该通道恢复满帧
#define JVN_CMD_ALLAUDIO	 0x65//音频全转发
/*与云视通服务器的交互消息*/
#define JVN_CMD_RELOGIN		 0x89//通知主控重新登陆
#define JVN_CMD_CLEAR		 0x8A//通知主控下线并清除网络信息包括云视通号码
#define JVN_CMD_REGCARD		 0x8B//主控注册板卡信息到服务器



#define JVN_CMD_CARDCHECK    0x91//板卡验证
#define JVN_CMD_CHANNELCOUNT 0x97//分控查询主控所具有的通道数目
#define JVN_CMD_PUSHMSG		 0x106//服务器推送消息

/*客户版流媒体服务器相关*/
#define JVN_REQ_CONNSTREAM_SVR    0xD0//请求连接流媒体服务器主控
#define JVN_REQ_CONNSTREAM_CLT    0xD1//请求连接流媒体服务器分控
#define JVN_RSP_CONNSTREAM_SVR    0xD2//回复
#define JVN_RSP_CONNSTREAM_CLT    0xD3
#define JVN_NOTIFY_ID			  0xD4
#define JVN_RSP_ID				  0xD5
#define JVN_CMD_CONNSSERVER		  0xD6
#define JVN_RSP_NEWCLIENT         0xD9

/*局域网设备搜索*/
#define JVN_REQ_LANSERCH  0x01//局域网设备搜索命令
#define JVN_CMD_LANSALL   1//局域网搜索所有中维设备
#define JVN_CMD_LANSYST   2//局域网搜索指定云视通号码的设备
#define JVN_CMD_LANSTYPE  3//局域网搜索指定卡系的设备
#define JVN_CMD_LANSNAME  4//局域网搜索指定别名的设备
#define JVN_RSP_LANSERCH  0x02//局域网设备搜索响应命令

#define JVN_DEVICENAMELEN  100//设备别名长度限制

/*局域网广播*/
#define JVN_REQ_BC  0x03//局域网广播命令
#define JVN_RSP_BC  0x04//局域网广播响应命令

/*局域网管理工具*/
#define JVN_REQ_TOOL 0x05//工具消息
#define JVN_RSP_TOOL 0x06//设备响应

#define JVN_ALLSERVER     0//所有服务
#define JVN_ONLYNET       1//只局域网服务

#define JVN_NOTURN        0//云视通方式时禁用转发
#define JVN_TRYTURN       1//云视通方式时启用转发
#define JVN_ONLYTURN      2//云视通方式时仅用转发

#define JVN_CONNTYPE_LOCAL  1//局域网连接
#define JVN_CONNTYPE_P2P    2//P2P穿透连接
#define JVN_CONNTYPE_TURN   3//转发

#define JVN_LANGUAGE_ENGLISH  1
#define JVN_LANGUAGE_CHINESE  2

#define JVN_TRANS_ONLYI   1//关键帧转发
#define JVN_TRANS_ALL     2//完整转发/完整传输

#define TYPE_PC_UDP      1//连接类型 UDP 支持UDP收发完整数据
#define TYPE_PC_TCP      2//连接类型 TCP 支持TCP收发完整数据
#define TYPE_MO_TCP      3//连接类型 TCP 支持TCP收发简单数据,普通视频帧等不再发送，只能采用专用接口收发数据(适用于手机监控)
#define TYPE_MO_UDP      4//连接类型 UDP 
#define TYPE_3GMO_UDP    5//连接类型 3GUDP
#define TYPE_3GMOHOME_UDP 6//连接类型 3GHOME


/*特殊命令类型*/
#define CMD_TYPE_CLEARBUFFER    1//主控和分控清空缓存，重新进行缓存


/*云台控制类型*/
#define JVN_YTCTRL_U      1//上
#define JVN_YTCTRL_D      2//下
#define JVN_YTCTRL_L      3//左
#define JVN_YTCTRL_R      4//右
#define JVN_YTCTRL_A      5//自动
#define JVN_YTCTRL_GQD    6//光圈大
#define JVN_YTCTRL_GQX    7//光圈小
#define JVN_YTCTRL_BJD    8//变焦大
#define JVN_YTCTRL_BJX    9//变焦小
#define JVN_YTCTRL_BBD    10//变倍大
#define JVN_YTCTRL_BBX    11//变倍小
/*********新增 左上、右上、左下、右下四个接口*****/
#define JVN_YTCTRL_LU    12//	左上
#define JVN_YTCTRL_RU    13//	右上
#define JVN_YTCTRL_LD    14//	左下
#define JVN_YTCTRL_RD    15//	右下

#define JVN_YTCTRL_UT     21//上停止
#define JVN_YTCTRL_DT     22//下停止
#define JVN_YTCTRL_LT     23//左停止
#define JVN_YTCTRL_RT     24//右停止
#define JVN_YTCTRL_AT     25//自动停止
#define JVN_YTCTRL_GQDT   26//光圈大停止
#define JVN_YTCTRL_GQXT   27//光圈小停止
#define JVN_YTCTRL_BJDT   28//变焦大停止
#define JVN_YTCTRL_BJXT   29//变焦小停止
#define JVN_YTCTRL_BBDT   30//变倍大停止
#define JVN_YTCTRL_BBXT   31//变倍小停止
#define JVN_YTCTRL_FZ1    32//辅助1
#define JVN_YTCTRL_FZ1T   33//辅助1停止
#define JVN_YTCTRL_FZ2    34//辅助2
#define JVN_YTCTRL_FZ2T   35//辅助2停止
#define JVN_YTCTRL_FZ3    36//辅助3
#define JVN_YTCTRL_FZ3T   37//辅助3停止
#define JVN_YTCTRL_FZ4    38//辅助4
#define JVN_YTCTRL_FZ4T   39//辅助4停止

/*********左上、右上、左下、右下的停止命令同上停止*****/
#define JVN_YTCTRL_LUT    JVN_YTCTRL_UT//	左上停止，同上停止
#define JVN_YTCTRL_RUT    JVN_YTCTRL_UT//	右上停止，同上停止
#define JVN_YTCTRL_LDT    JVN_YTCTRL_UT//	左下停止，同上停止
#define JVN_YTCTRL_RDT    JVN_YTCTRL_UT//	右下停止，同上停止

#define JVN_YTCTRL_RECSTART  41//远程录像开始
#define JVN_YTCTRL_RECSTOP	 42//远程录像开始

/*远程控制指令(主分控应用层约定)*/
#define RC_DISCOVER		0x01 
#define RC_GETPARAM		0x02 
#define RC_SETPARAM		0x03 
#define RC_VERITY		0x04 
#define RC_SNAPSLIST	0x05 
#define RC_GETFILE		0x06 
#define RC_USERLIST		0x07 
#define RC_PRODUCTREG	0X08 
#define RC_GETSYSTIME	0x09 
#define RC_SETSYSTIME	0x0a 
#define RC_DEVRESTORE	0x0b 
#define RC_SETPARAMOK	0x0c 
#define RC_DVRBUSY		0X0d 
#define RC_GETDEVLOG	0x0e 
#define RC_DISCOVER2	0x0f	//zwq20111206,csst云视通号码直接登录，局域网广播搜索

#define JVN_VC_BrightUp			0x10 //视频调节
#define JVN_VC_BrightDown		0x11
#define JVN_VC_ContrastUp		0x12
#define JVN_VC_ContrastDown		0x13
#define JVN_VC_SaturationUp		0x14
#define JVN_VC_SaturationDown	0x15
#define JVN_VC_HueUp			0x16
#define JVN_VC_HueDown			0x17
#define JVN_VC_SharpnessUp		0x18
#define JVN_VC_SharpnessDown	0x19
#define JVN_VC_PRESENT          0x20 //预置位调用

typedef struct _PLAY_INFO_
{
#define MAX_USR_PKG_SIZE 2048
	unsigned char ucCommand;//命令字
	int nClientID;//对应着回放

	int nConnectionType;

	char strFileName[MAX_PATH];//文件名

	int nSeekPos;//定位时需要定位的位置 帧

    unsigned char chPkgBuf[MAX_USR_PKG_SIZE]; //应用层原始数据缓存
    int nPkgSize;
}PLAY_INFO;

typedef struct STLANTOOLINFO 
{
	unsigned char  uchType;      //消息类型，1来自工具的广播；2来自工具的配置；3设备回应；
	
	/*工具信息*/
	char chPName[256]; //用户名，用于提高IPC安全性，防止恶意配置
	char chPWord[256]; //密码，用于提高IPC安全性，防止恶意配置
	int nYSTNUM;       //云视通号码，用于工具向设备发送配置
	char chCurTime[20];//系统时间，用于工具向设备发送配置 xxxx-xx-xx xx:xx:xx
	char *pchData;     //配置内容，用于工具向设备发送配置
	int nDLen;         //配置内容长度，用于工具向设备发送配置
	
	/*应答信息*/
	int nCardType;     //设备类型，用于设备应答时的附加信息
	int	nDate;         //出厂日期 形如 20091011
	int	nSerial;       //出厂序列号
	GUID guid;         //唯一GUID
	char chGroup[4];   //设备便组号，用于设备应答时的附加信息
	
	char chIP[16];
	int nPort;
	
}STLANTOOLINFO;//局域网生产工具消息内容


#define  NET_MOD_UNKNOW 0 // 未名
#define  NET_MOD_WIFI   1 //wifi 模式
#define  NET_MOD_WIRED  2 // 有线模式

#define  DEV_SET_ALL      0 // 设置全部
#define  DEV_SET_NET      1 //设置设备支持的网络模式
#define  DEV_SET_CUR_NET  2 //设置设备当前网络模式
#define  DEV_SET_NAME     3 // 设置别名

typedef struct  
{
	char chDeviceName[100];//设备别名
	int nCurNetMod;// 设备当前网络模式，有线，wifi或其它
	int nNetMod; //设备支持的网络模式，为 几种模式或起来的值  nNetMod = NET_MOD_WIFI;
}STDEVINFO; //设备参数
typedef struct STBASEYSTNO
{
	char chGroup[4];
	int nYSTNO;
	int nChannel;
	char chPName[MAX_PATH];
	char chPWord[MAX_PATH];
	int nConnectStatus;//连接状态 设置时=0，查询时表示状态 0 未连接 1 内网 2 转发 3外网	
}STBASEYSTNO;//云视通号码基本信息，用于初始化小助手的虚连接

typedef struct 
{
	char chGroup[4];
	int nYSTNO;
	int nCardType;
	int nChannelCount;
	char chClientIP[16];
	int nClientPort;
	int nVariety;
	char chDeviceName[100];
#ifndef WIN32
	int bTimoOut;
#else
	BOOL bTimoOut;
#endif
	int nNetMod;//例如 是否具有wifi功能: nNetMod & NET_MOD_WIFI
	int nCurMod;//例如 当前使用的(wifi或有线)：nCurMod(NET_MOD_WIFI 或 NET_MOD_WIRED)
	char chNickName[36];//昵称
	int nPrivateSize;//自定义数据实际长度
	char chPrivateInfo[500];//自定义数据内容
}STLANSRESULT;//局域网设备搜索结果

/*主控回调函数*/
#ifndef WIN32//LINUX
	typedef int (*FUNC_SCHECKPASS_CALLBACK)(int nLocalChannel, char chClientIP[16], int nClientPort, char *pName, char *pWord);
	typedef void (*FUNC_SCONNECT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, char *pName, char *pWord,int nUserDataLen,char* pUserData);
	typedef void (*FUNC_SONLINE_CALLBACK)(int nLocalChannel, unsigned char uchType);
	typedef void (*FUNC_SCHECKFILE_CALLBACK)(int nLocalChannel, int nClientID, char chClientIP[16], int nClientPort, char chStartTime[14], char chEndTime[14], unsigned char *pBuffer, int *nSize);
    typedef void (*FUNC_SCHECKFILENEW_CALLBACK)(int nLocalChannel, int nClientID, char chClientIP[16], int nClientPort,unsigned char *pRawBuffer, int nRawSize, unsigned char **pResultBuffer, int *nResultSize);
	typedef void (*FUNC_SCHAT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, unsigned char  *pBuffer, int nSize);
	typedef void (*FUNC_STEXT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, unsigned char  *pBuffer, int nSize);
	typedef void (*FUNC_SYTCTRL_CALLBACK)(int nLocalChannel, int nClientID, int nType, char chClientIP[16], int nClientPort);
	typedef void (*FUNC_SBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], int nPort);
	typedef void (*FUNC_SBCSELFDATA_CALLBACK)(unsigned char *pBuffer, int nSize, char chIP[16], int nPort);
	typedef int (*FUNC_SFPLAYCTRL_CALLBACK)(PLAY_INFO* pData);
	typedef void (*FUNC_DLFNAME_CALLBACK)(char chFilePathName[256]);
	typedef int (*FUNC_LANTOOL_CALLBACK)(STLANTOOLINFO* pData);//0不需相应1响应
	typedef void (*FUNC_RECVSERVERMSG_CALLBACK)(int nType, char *chPushMsg, int nMsgLen, struct sockaddr_in *svrAddr);//别名注册返回消息 nType = 0表示注册返回信息，=1表示请求主控参数，=2删除别名返回值，=3查询别名返回值
	//nType = 0时：chPushMsg为数值字符串，-1号码未上线、0注册成功、1请求注册服务器地址超时、2绑定本地端口失败、3连接注册服务器失败、
				   //4注册别名重复、5注册信息无效、6注册别名返回值接收超时、7接收数据失败、8向服务器发送数据失败
	//nType = 1时：推送消息或请求参数
	//nType = 2时：删除别名返回值，chPushMsg为数值字符串，-1删除失败、0删除成功、1请求注册服务器地址超时、2绑定本地端口失败、3连接注册服务器失败、7接收数据失败、8向服务器发送数据失败
	//nType = 3时：查询别名返回值，chPushMsg为别名字符串，6<=nMsgLen<=32时查询成功，chPushMsg为别名字符串；其他值表示查询失败或不存在（nMsgLen=0）、1请求注册服务器地址超时、2绑定本地端口失败、3连接注册服务器失败、7接收数据失败、8向服务器发送数据失败
	//nType = 4时：云存储开启/关闭状态主动通知到应用层，nMsgLen = 1 关闭；nMsgLen > 1 开启，其值表示chPushMsg长度，字符串结构：bucket:host
	typedef void (*FUNC_RECVCLIENTMSG_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort);//分控控制信息回调，uchType 回调信息类型，JVN_CMD_VIDEO 视频传输，JVN_CMD_VIDEOPAUSE 暂停传输
	typedef void (*FUNC_ACCEPTCLIENT_CALLBACK)(int *nType, char *chPushMsg, int *nMsgLen);//主控处理是否接收连接的回调
#else//WINDOWS
	typedef bool (*FUNC_SCHECKPASS_CALLBACK)(int nLocalChannel, char chClientIP[16], int nClientPort, char *pName, char *pWord);
	typedef void (*FUNC_SCONNECT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, char *pName, char *pWord,int nUserDataLen,char* pUserData);
	typedef void (*FUNC_SONLINE_CALLBACK)(int nLocalChannel, unsigned char uchType);
	typedef void (*FUNC_SCHECKFILE_CALLBACK)(int nLocalChannel, int nClientID, char chClientIP[16], int nClientPort, char chStartTime[14], char chEndTime[14], unsigned char *pBuffer, int &nSize);
    typedef void (*FUNC_SCHECKFILENEW_CALLBACK)(int nLocalChannel, int nClientID, char chClientIP[16], int nClientPort,unsigned char *pRawBuffer, int nRawSize, unsigned char **pResultBuffer, int *nResultSize);
	typedef void (*FUNC_SCHAT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, unsigned char  *pBuffer, int nSize);
	typedef void (*FUNC_STEXT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, unsigned char  *pBuffer, int nSize);
	typedef void (*FUNC_SYTCTRL_CALLBACK)(int nLocalChannel, int nClientID, int nType, char chClientIP[16], int nClientPort);
	typedef void (*FUNC_SBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], int nPort);
	typedef void (*FUNC_SBCSELFDATA_CALLBACK)(unsigned char *pBuffer, int nSize, char chIP[16], int nPort);
	typedef bool (*FUNC_SFPLAYCTRL_CALLBACK)(PLAY_INFO* pData);
	typedef void (*FUNC_DLFNAME_CALLBACK)(char chFilePathName[256]);
	typedef int (*FUNC_LANTOOL_CALLBACK)(STLANTOOLINFO* pData);//0不需相应1响应
	typedef void (*FUNC_RECVSERVERMSG_CALLBACK)(int nType, char *chPushMsg, int nMsgLen, SOCKADDR_IN *svrAddr);//别名注册返回消息 nType = 0表示注册返回信息，=1表示请求主控参数，=2删除别名返回值，=3查询别名返回值
	//nType = 0时：chPushMsg为数值字符串，-1号码未上线、0注册成功、1请求注册服务器地址超时、2绑定本地端口失败、3连接注册服务器失败、
				   //4注册别名重复、5注册信息无效、6注册别名返回值接收超时、7接收数据失败、8向服务器发送数据失败
	//nType = 1时：推送消息或请求参数
	//nType = 2时：删除别名返回值，chPushMsg为数值字符串，-1删除失败、0删除成功、1请求注册服务器地址超时、2绑定本地端口失败、3连接注册服务器失败、7接收数据失败、8向服务器发送数据失败
	//nType = 3时：查询别名返回值，chPushMsg为别名字符串，6<=nMsgLen<=32时查询成功，chPushMsg为别名字符串；其他值表示查询失败或不存在（nMsgLen=0）、1请求注册服务器地址超时、2绑定本地端口失败、3连接注册服务器失败、7接收数据失败、8向服务器发送数据失败
	//nType = 4时：云存储开启/关闭状态主动通知到应用层，nMsgLen = 1 关闭；nMsgLen > 1 开启，其值表示chPushMsg长度，字符串结构：bucket:host
	typedef void (*FUNC_RECVCLIENTMSG_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort);//分控控制信息回调，uchType 回调信息类型，JVN_CMD_VIDEO 视频传输，JVN_CMD_VIDEOPAUSE 暂停传输
	typedef void (*FUNC_ACCEPTCLIENT_CALLBACK)(int *nType, char *chPushMsg, int *nMsgLen);//主控处理是否接收连接的回调
#endif

typedef void ( * FUNC_EXAM_CALLBACK )(int nExamID,int nErrorID,char *pErrorText);//回调函数

/*分控回调函数*/
#ifndef WIN32//LINUX
	typedef void (*FUNC_CCONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData,int nUserDataLen,char* pUserData);
	typedef void (*FUNC_CNORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nWidth, int nHeight);
	typedef void (*FUNC_CCHECKRESULT_CALLBACK)(int nLocalChannel,unsigned char *pBuffer, int nSize);
	typedef void (*FUNC_CCHATDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize);
	typedef void (*FUNC_CTEXTDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize);
	typedef void (*FUNC_CDOWNLOAD_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nFileLen);
	typedef void (*FUNC_CPLAYDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nWidth, int nHeight, int nTotalFrame,unsigned long long ulTime,int nFrameSeq);

	typedef void (*FUNC_CBUFRATE_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nRate);
	typedef void (*FUNC_CLANSDATA_CALLBACK)(STLANSRESULT stLSResult);
	typedef void (*FUNC_CBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], int bTimeOut);
	typedef int (*FUNC_CLANTDATA_CALLBACK)(STLANTOOLINFO *pLANTData);
	typedef void (*FUNC_COMM_DATA_CALLBACK)(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize);//nType = 1是写 2是读
	typedef void (*FUNC_RECV_FROM_SERVER)(char* pIP,int nPort,unsigned char *pMsg,int nLen);//参数 服务器返回的地址 端口 内容 长度
#else//WINDOWS
	typedef void (*FUNC_CCONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData,int nUserDataLen,char* pUserData);
	typedef void (*FUNC_CNORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nWidth, int nHeight);
	typedef void (*FUNC_CCHECKRESULT_CALLBACK)(int nLocalChannel,unsigned char *pBuffer, int nSize);
	typedef void (*FUNC_CCHATDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize);
	typedef void (*FUNC_CTEXTDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize);
	typedef void (*FUNC_CDOWNLOAD_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nFileLen);
	typedef void (*FUNC_CPLAYDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nWidth, int nHeight, int nTotalFrame,double ulTime,int nFrameSeq);
	typedef void (*FUNC_CBUFRATE_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nRate);
	typedef void (*FUNC_CLANSDATA_CALLBACK)(STLANSRESULT stLSResult);
	typedef void (*FUNC_CBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], BOOL bTimeOut);
	typedef int (*FUNC_CLANTDATA_CALLBACK)(STLANTOOLINFO *pLANTData);
	typedef void (*FUNC_COMM_DATA_CALLBACK)(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize);//nType = 1是写 2是读
	typedef void (*FUNC_RECV_FROM_SERVER)(char* pIP,int nPort,unsigned char *pMsg,int nLen);//参数 服务器返回的地址 端口 内容 长度
#endif
typedef void (*FUNC_CBCSELFDATA_CALLBACK)(unsigned char *pBuffer, int nSize, char chIP[16], int nPort,int nType);

typedef int (*FUNC_DEVICE_CALLBACK)(STLANSRESULT* stLSResult);

//流媒体回调函数
typedef void (*FUNC_CRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);//1 成功 2 失败 3 断开 4 异常断开
typedef void (*FUNC_CRTMP_NORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

typedef void (*FUNC_SRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg);//1 成功 2 失败 3 断开 4 异常断开

/*
0	检测成功
1	无法获取本机的ip地址
2	ping网关不通
3	dns无法解析
4	获取服务器列表错误，无法使用http或没有正确的编组
5	与服务器通信不是很畅通
6	YST端口异常，需要更换端口
7	号码上线错误，参考返回的具体信息
*/
typedef void (*FUNC_POSITION_CALLBACK)(int nPosID,char* pKey,char* pCountry,char* pProvince,char* pCity);
/************主控回调函数参数类型***************/
/*FUNC_SCONNECT_CALLBACK*/
#define JVN_SCONNECTTYPE_PCCONNOK  0X01//有PC分控连接成功
#define JVN_SCONNECTTYPE_DISCONNOK 0X02//有分控断开连接成功
#define JVN_SCONNECTTYPE_DISCONNE  0X03//连接异常断开
#define JVN_SCONNECTTYPE_MOCONNOK  0X04//有移动设备分控连接成功
/*FUNC_SONLINE_CALLBACK*/
#define JVN_SONLINETYPE_ONLINE     0x01//上线
#define JVN_SONLINETYPE_OFFLINE    0x02//下线
#define JVN_SONLINETYPE_CLEAR      0x03//YST号码无效，应清空重新申请

/************分控回调函数参数类型***************/
/*FUNC_CCONNECT_CALLBACK*/
#define JVN_CCONNECTTYPE_CONNOK    0X01//连接成功
#define JVN_CCONNECTTYPE_DISOK     0X02//断开连接成功
#define JVN_CCONNECTTYPE_RECONN    0X03//不必重复连接
#define JVN_CCONNECTTYPE_CONNERR   0X04//连接失败
#define JVN_CCONNECTTYPE_NOCONN    0X05//没连接
#define JVN_CCONNECTTYPE_DISCONNE  0X06//连接异常断开
#define JVN_CCONNECTTYPE_SSTOP     0X07//服务停止，连接断开
#define JVN_CCONNECTTYPE_DISF      0x08//断开连接失败

#define JVN_CCONNECTTYPE_ERR_UNKNOWN		"Unknown error"	//未知错误(已停用)
#define JVN_CCONNECTTYPE_ERR_IP_TIMEOUT		"connect ip timeout"//IP地址连接超时
#define JVN_CCONNECTTYPE_ERR_INDEX			"Index not online"//检索不在线
#define JVN_CCONNECTTYPE_ERR_RECHECK		"Recheck error"//预验证错误
#define JVN_CCONNECTTYPE_ERR_VER			"Local Version too old"//主控版本太老，已不支持连接
#define JVN_CCONNECTTYPE_ERR_PARTNER		"Request partner list failed"//请求伙伴列表错误(多播时才有)
#define JVN_CCONNECTTYPE_ERR_NICK_TIME		"Nickname query timeout"//查询别名超时(已停用)


#define JVN_CCONNECTTYPE_ERR_CHANNEL		"Channel error[1,65535]"//本地通道号错误
#define JVN_CCONNECTTYPE_ERR_IP				"IP error"//ip地址错误
#define JVN_CCONNECTTYPE_ERR_YST			"YST Error"//云视通号码错误
#define JVN_CCONNECTTYPE_ERR_NICK			"NickName error"//别名错误(已停用)
#define JVN_CCONNECTTYPE_ERR_PORT			"Port error"//端口错误
#define JVN_CCONNECTTYPE_ERR_PARAM			"param error"//参数错误

#define JVN_CCONNECTTYPE_ERR_QUICK			"connect failed. quick connect failed"//连接失败(未在用)
#define JVN_CCONNECTTYPE_ERR_NO_CHANNEL		"No Channel service"//无通道服务
#define JVN_CCONNECTTYPE_ERR_LIMIT			"client count limit"//超过最大连接数
#define JVN_CCONNECTTYPE_ERR_CONNECT_TYPE	"connect type invalid"//连接类型错误
#define JVN_CCONNECTTYPE_ERR_TIMEOUT		"Connect Timeout"//连接超时
#define JVN_CCONNECTTYPE_ERR_REQUEST		"Request video failed"//请求视频失败
#define JVN_CCONNECTTYPE_ERR_THREAD			"Start work thread failed"//启动线程失败
#define JVN_CCONNECTTYPE_ERR_PWD			"Passwords is wrong"//密码错误
#define JVN_CCONNECTTYPE_ERR_PWD_TIME		"check Passwords timeout"//密码验证超时
#define JVN_CCONNECTTYPE_ERR_OFFLINE		"YST is Offline"//设备离线
#define JVN_CCONNECTTYPE_ERR_NOT_GET_SERVER_T			"not find turn server"//没有找到转发地址
#define JVN_CCONNECTTYPE_ERR_NOT_SERVER_T	"connect turn server timeout"//连接转发超时
#define JVN_CCONNECTTYPE_ERR_MEMERY			"Memery error"//连接时，内存不足
#define JVN_CCONNECTTYPE_ERR_FORBIDDEN		"connect forbidden"//服务器禁止连接
#define JVN_CCONNECTTYPE_ERR_NICK_NO		"NickName not exist"//别名不存在(已停用)


/*返回值*/
#define JVN_RETURNOK    0//成功
#define JVN_PARAERROR   1//参数错误
#define JVN_RETURNERROR 2//失败
#define JVN_NOMEMERROR  3//内存或硬盘空间不足

//增加IP段，搜索局域网设备时候用。
typedef struct
{
	char startip[16];//起始IP
	char endip[16];//终止IP
}IPSECTION;

typedef struct 
{
	char chIP[16];
	int nPort;
	unsigned char uchStatus;// 0 1
	unsigned char uchType;
	unsigned char uchProcType;
	int nDownSpeed;// KB/s
	float fDownTotal;// M
	float fUpTotal;// M
}STPTINFO;

/*初始化SDK时传入设备相关的一些信息，用于内部根据不同产品设置不同的处理*/
typedef struct 
{
	int nType;//产品大类 0普通默认产品；1板卡; 2DVR; 3IPC; 4JNVR; 5NVR; 6家用版小IPC;
	int nMemLimit;//内存充足度 0普通默认产品；1内存充足(网络至少有20M可用)；2内存基本够用(网络约20M可用)；3内存紧张(网络小于15M可用)；
	              //内存允许的前提下，内存充足度越好，网传处理效果会越好；如果不确定，请置为0；
}STDeviceType;

/*激活号码时调用者传入结构*/
typedef struct 
{
	char chGroup[4];//分组号，形如"A" "AAAA"
	int nCardType;  //卡系
	int	nDate;      //出厂日期 形如 20091011
	int	nSerial;    //出厂序列号
	GUID guid;      //唯一GUID MAPIGUID.H
}STGetNum;
/*初始化时调用者传入的结构*/
typedef struct
{
    int nCardType; /*产品类型，例如：0xE71A, 0xD800,0xB800, 960*/
    int    nDate;     //出厂日期
    int    nSerial;   //出厂序列号
    GUID gLoginKey;//板卡GUID
    int  nYstNum;  //云视通号码
    char chGroup[4];//分组号，形如"A" "AAAA"
    int  nChannelCount; //主控通道总数目  //old

	unsigned long dwProductType;//产品完整类型/*例如：0xE71A4010,0xD8006001,0xB8007001,0xc896ffff*/
    unsigned long dwEncryVer; //加密版本, 0xB567709F：加密芯片中包含云视通号码，有数据库;0xF097765B：不包含号码，有数据库;0xB56881B0:包含号码，无数据库;
    unsigned long dwDevVer; //加密芯片硬件版本
    int   nUIVer;//主控版本
    unsigned long dwOemid;//厂家id
    unsigned long dwUser;//加密人员ID
    
   
    int nMaxConnNum;      //最大连接数
    int nZone;//区域-086中国
    int nSystemType;//系统型号-高1字节系统类型(0x1:windows 0x2:linux 0x3:MacOS 0x4:安卓 0x5:其他)，低3字节系统版本号，各系统版本号详见注释
    /*Windows:6.1; 6.0; 5.2; 5.1; 5.0; 4.9; 4.1; 4.0; 3.1; 3.0; 2.0; 1.0 等
    linux :2.6; 2.4; 2.2; 2.0; 1.2; 1.1; 1.0 等
    MAC OS:10.7; 10.6; 10.5; 10.4; 10.3; 10.2; 10.1; 10.0 等;
    Android 1.1; 1.5; 1.6; 2.0; 2.1; 2.2; 2.3; 2.4; 3.0; 3.1; 3.2; 4.0; 4.1; 4.2; 4.4等    */
    /*例如：win7: 0x103d, win XP: 0x1033; linux2.6: 0x201a, linux2.5: 0x2019; MacOS10.7: 0x306B, MacOS10.6:0x306A; Android4.0: 0x4028; Android4.4: 0x402c*/
    
    char chProducType[64];//产品型号-字符串
    char chDevType[16];//硬件型号-字符串
}STOnline;





typedef struct
{
	unsigned int nTypeLens; //YST_AS_REGCARD	
	//净载数据
	int nSize;
	char *pacData;
}SOCKET_DATA_TRAN;

//主动上传数据结构，第一次运行时上传或者设置变化后上传
typedef struct _CHANNEL_INFO
{
	unsigned char uchChannel;	 //通道序号 从1开始
	unsigned char uchChnInfo;	//0x0011 0001 高四位0011表示码流个数，低四位中00表示数据类型、01是否开启云视通服务 
	long wBitRate[5];	//码流值，最大支持5个，单位kbps
}CHANNEL_INFO;//17字节

/***************** 收到回调请求时发送 *****************/


typedef struct
{
	unsigned char uchMemoryUsage;//内存使用率 百分比
	unsigned char uchCPUUsage;//CPU使用率 百分比
	unsigned char uchCurrNetMode;//网络模式-/*#define NET_MOD_UNKNOW 0 未名；#define NET_MOD_WIFI 1 wifi模式；#define NET_MOD_WIRED 2 有线模式*/
	unsigned char uchCount;//CHANNEL_CONNCNT个数
	char chChnConnData[500];//uchCount个CHANNEL_CONNCNT结构体
}JVSERVER_INFO;

#define JVRTMP_DATATYPE_METADATA				0x00
#define JVRTMP_DATATYPE_H264_I					0x01
#define JVRTMP_DATATYPE_H264_BP					0x02
#define JVRTMP_DATATYPE_ALAW					0x11
#define JVRTMP_DATATYPE_ULAW					0x12

typedef struct _JVRTMP_Metadata_t
{
	int nVideoWidth;//帧长度
	int nVideoHeight;//帧宽度
	int nVideoFrameRateNum;//帧率
	int nVideoFrameRateDen;//默认填1
	int nAudioDataType;//编码类型，
	int nAudioSampleRate;//采样率
	int nAudioSampleBits;//采样位数
	int nAudioChannels; //声道数
}JVRTMP_Metadata_t;

typedef struct _DownFile_INFO_
{
	unsigned char ucCommand;//命令字
	int nClientID;//
	int nConnectionType;
	char strFileName[MAX_PATH];//文件名
	int nReadPos;//定位时需要定位的位置 帧
}DownFile_INFO;
typedef void (*FUNC_REMOTEDL_CALLBACK)(DownFile_INFO* pData);
typedef struct
{
	unsigned char usChannel;//报警通道号
	unsigned char uchAlarmSolution;	//报警解决方案标识 0云视通方案默认 1云存储方案 2云存储+普通混合报警
	unsigned char uchMsgType;		//报警消息类型 0文本消息
	unsigned char uchAlarmType;		//报警类别
	char chAlarmTime[16];	//报警时间	"20150508090405"年月日时分秒 字符串
	char chDevName[64];		//设备名称
	char chCloudPicPath[128];	//云存储图片本地路径[云存储使用][本地图片路径]
	char chCloudVideoName[128];	//云存储视频名称[云存储使用][本地视频名称]
	char chPicPath[128];	//图片文件路径[普通报警使用][/rec/00/20150414/filename]
	char chVideoPath[128];	//视频文件路径[普通报警使用][/rec/00/20150414/filename]
	char chOtherMsg[128];	//可选 报警其他信息 比如外设报警(报警类型为4)，此字段代表客户端设置的报警外设名字
}ALARM_TEXT_PARA;

typedef struct
{
	int nIpcDevType;	//设备类型 家用设备赋值2  59162:普通IPC 55296:DVR 47104:NVR 其他值板卡
	int nIpcSubDevType;	//设备型号(针对家用设备)436:H411_H1  431:H210-H1等张帅制定的编号对应的设备型号，
	char chDevName[64];	//设备昵称
	char chDevVision[32];	//设备软件版本 
}IPC_INIT_PARA;//主要针对家用IPC,设备和账号服务器TCP连接取消后，设备上报的参数



//自定义回调函数,主分控自定义发送
//(nClientID 客户端ID  uchType 自定义类型 pRecvBuffer 收到的数据 nRecvSize 收到的数据长度 pSendBuffer 需要立即发送的数据 nSendMaxSize 当前立即发送的最大数)
//返回 立即发送的数据大小 <=0将不发送
typedef int (*FUNC_SELF_COMMN_CALLBACK)(const int nClientID,const unsigned int uchType,const unsigned char *pRecvBuffer,const unsigned int nRecvSize, unsigned char *pSendBuffer,const unsigned int nSendMaxSize);

//分控发送文件回调函数 (nLocalChannel 客户端ID uchType 类型 1 传送数据 2 传送完成 3 错误 传送文件数据 nSendFinishLen 已经传送的大小 pBuff 对错误的说明)
typedef int (*FUNC_SENDFILE_CALLBACK)(const int nLocalChannel,const unsigned int uchType,const unsigned int nSendFinishLen,const char *pBuff);

//主控允许接收文件的回调 (nClientID 客户端ID  uchType 类型 1 有文件上传，pData = 文件名 nLen 文件名长度；uchType 类型 2 文件数据到达，pData = 数据 nLen 数据长度
typedef int (*FUNC_ACCEPTFILE_CALLBACK)(const int nClientID,const unsigned int uchType,const unsigned char* pData,const int nLen);

#define JVNS_AB_SELF_DEFINE	0x100	//主分控自定义数据交换

/*自定义类型，该宏由应用层主分控提出申请，由网络库在此添加，禁止任何应用层人员在此添加*/
#define JVNS_APP_1	1

#define JVN_CMD_SEND_FILE				0xB1        //发送文件名
#define JVN_CMD_SEND_FILE_DATA				0xB2        //发送文件数据 或 大小

u_short in_cksum(u_short *addr, int len);

// 对端设备信息
typedef struct PEER_INFO_
{
	int nVer; // 协议版本号
    char bVideoTimeStamp;// 对端是否启用了时间戳
    char bRemoPlayTimeStamp;// 对端远程回放是否启用了时间戳
}PEER_INFO;

#endif
