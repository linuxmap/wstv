/**
 *@file rcmodule.h 分控与各模块通讯的扩展类型的部分
 *@author Liu Fengxiang
 */
#ifndef _RCMODULE_H_
#define _RCMODULE_H_

//扩展类型，用于指定哪个模块去处理,lck20120206
#define RC_EX_FIRMUP		0x01
#define RC_EX_NETWORK		0x02
#define RC_EX_STORAGE		0x03
#define RC_EX_ACCOUNT		0x04
#define RC_EX_PRIVACY		0x05
#define RC_EX_MD		    0x06
#define RC_EX_ALARM		    0x07
#define RC_EX_SENSOR		0x08
#define RC_EX_PTZ			0x09
#define RC_EX_AUDIO			0x0a
#define RC_EX_ALARMIN		0x0b
#define RC_EX_REGISTER 		0x0c
#define RC_EX_EXPOSURE		0x0d
#define RC_EX_QRCODE		0x0e
#define RC_EX_IVP			0x0f
#define RC_EX_DOORALARM		0x10
#define RC_EX_PTZUPDATE		0x11
#define RC_EX_COMTRANS		0x12
#define RC_EX_CHAT			0x13
#define RC_EX_ALARMOUT		0x14
#define RC_EX_NICKNAME		0x15
#define RC_EX_CLOUD			0x16
#define RC_EX_STREAM		0x17
#define RC_EX_PLAY_REMOTE   0x18
#define RC_EX_HEARTBEAT		0x19
#define RC_EX_RESTRICTION	0x20
#define RC_EX_REQ_IDR		0x21

#define RC_EX_DEBUG			0x24


typedef struct tagEXTEND
{
	U32	nType;
	U32	nParam1;
	U32	nParam2;
	U32	nParam3;
	U8	acData[RC_DATA_SIZE-16];
} EXTEND, *PEXTEND;

//
#define   COMM_NO_PERMISSION	0X0405

//------------------网络设置-------------------------------------
#define EX_ADSL_ON		0x01	//连接ADSL消息
#define EX_ADSL_OFF		0x02	//断开ADSL消息
#define EX_WIFI_AP		0x03	//获取AP消息：单独给手机使用以兼容手机版本
#define EX_WIFI_ON		0x04	//连接wifi
#define EX_WIFI_OFF		0x05	//断开wifi
#define EX_NETWORK_OK	0x06	//设置成功
#define EX_UPDATE_ETH	0x07	//修改eth网络信息
#define EX_NW_REFRESH	0x08	//刷新当前网络信息
#define EX_NW_SUBMIT	0x09	//刷新当前网络信息
#define EX_WIFI_CON 	0x0a
#define EX_WIFI_AP_CONFIG 0x0b
#define EX_START_AP		0x0c	//开启AP
#define EX_START_STA	0x0d	//开启STA

#define EX_WIFI_AP_DLL	0x0e	//DLL和手机客户端有冲突单独搞一个命令出来

#define EX_WIFI_SHOW_RSSID 0x0f //工厂检测设备wifi信号质量时，发送这个命令

typedef struct 
{
    char eth_acip[16];
    char eth_acdns[16];
    char eth_acgw[16];
    char eth_acnm[16];

    BOOL dir_link;
    BOOL dhcp;
	BOOL bReboot;
	BOOL bModify;

	U32 nWIFI_mode;
	U32 nAP;
	char	acID[32];
	char	acPW[64];
    U32 wifiAuth;
    U32 wifiEncryp;

	U32	nActived;
}NETWORKINFO;
extern NETWORKINFO  stNetworkInfo;

typedef struct
{
    char wifiId[32];
    char wifiPwd[64];
    int wifiAuth;
    int wifiEncryp;
    U8 wifiIndex;
    U8 wifiChannel;
    U16 wifiRate;
}WIFI_INFO;


U32 build_network_param(char *pData);

//远程设置消息处理函数
VOID NetworkProc(REMOTECFG *remotecfg);

//添加视频配置参数,lck20120221
U32 build_stream_param(char *pData);


//------------------移动检测-------------------------------------
#define EX_MD_REFRESH			0x01
#define EX_MD_SUBMIT			0x02
#define EX_MD_UPDATE			0x03

/**
 *@brief 把结构体转化为字符串
 *@param pData 保存转化后字符串的内存地址
 *@return 转化后字符串的长度
 *
 */
U32 build_mdetect_param(int channelid, char *pData);

//远程设置处理
VOID MDProc(REMOTECFG *cfg);


//------------------区域遮挡-------------------------------------
#define EX_COVERRGN_REFRESH		0x01
#define EX_COVERRGN_SUBMIT		0x02
#define EX_COVERRGN_UPDATE		0x03

//添加区域遮挡参数
U32 build_privacy_param(char *pData);

//远程设置处理
VOID PrivacyProc(REMOTECFG *cfg);//xian U8 *pData


//------------------sensor设置-----------------------------------
#define EX_SENSOR_REFRESH		0x01
#define EX_SENSOR_SUBMIT		0x02
#define EX_SENSOR_CANCEL		0x03
#define EX_SENSOR_SAVE			0x04
#define EX_SENSOR_GETPARAM	0x05

//添加sensor参数
U32 build_sensor_param(char *pData);

//远程设置处理
VOID SensorProc(REMOTECFG *cfg);

#define EX_PTZ_PRESET_ADD	0x01	//添加预置点
#define EX_PTZ_PRESET_DEL	0x02	//删除
#define EX_PTZ_PRESET_CALL	0x03	//调用预置点

#define EX_PTZ_PATROL_ADD	0x04
#define EX_PTZ_PATROL_DEL	0x05
#define EX_PTZ_PATROL_START	0x06	//开始巡航
#define EX_PTZ_PATROL_STOP		0x07	//结束巡航

#define EX_PTZ_TRAIL_REC_START	0x08
#define EX_PTZ_TRAIL_REC_STOP	0x09
#define EX_PTZ_TRAIL_START		0x0A	//开始轨迹
#define EX_PTZ_TRAIL_STOP		0x0B	//停止轨迹

#define EX_PTZ_GUARD_START	0x0C	//启动守望
#define EX_PTZ_GUARD_STOP		0x0D	//停止守望

#define EX_PTZ_SCAN_LEFT		0x0E	//线扫左边界
#define EX_PTZ_SCAN_RIGHT		0x0F	//线扫右边界
#define EX_PTZ_SCAN_START		0x10	//线扫开始
#define EX_PTZ_SCAN_STOP		0x11	//结束扫描
#define EX_PTZ_WAVE_SCAN_START	0x12	//花样扫描
#define EX_PTZ_COM_SETUP			0x13	//云台协议波特率等参数设置
#define EX_PTZ_PRESETS_PATROL_START			0x14	//一键批量添加预置点并开始巡航
#define EX_PTZ_PRESETS_PATROL_STOP			0x15	//一键批量删除预置点并停止巡航
#define EX_PTZ_BOOT_ITEM				0x16	//IPC重启后执行的动作：无，巡航，线扫
#define EX_AF_CALIBRATION				0x17	//一体机autofocus校正
#define EX_PTZ_PRESET_SET		0x18	//直接设置预制点，可以不必添加
#define EX_PTZ232_COM_SETUP			0x19	//机芯协议波特率等参数设置
#define EX_PTZ_PRESET_CLEAR		0x1A	//直接清除预置点
#define EX_PTZ_SCHEDULE_SAVE	0x1B	//保存定时计划
#define EX_PTZ_PATROL_SETSPEED	0x1C	//设置巡航速度
#define EX_PTZ_GUARD_SAVE		0x1D	//保存守望信息
#define EX_PTZ_ZOOM_ZONE		0x1E	//3D定位
#define EX_AF_CALIBRATION_INFAUTO				0x20	//一体机autofocus以INF为基准执行自动校准
#define EX_AF_CALIBRATION_FILTERSET				0x21	//hisi的AF滤波器参数设置
#define EX_AF_CALIBRATION_CHECKLENS				0x22			//镜头料检,2m
#define EX_AF_CALIBRATION_CHECKRESULT				0x23		//校准后的检验
#define EX_AF_CALIBRATION_CHECKINFRESULT			0x24	 //校准后INF的检验
#define EX_AF_CALIBRATION_SEL_CURVE					0x25	 //选择物距曲线

#define EX_PTZ_SCAN_UP				0x2A	//扫描上边界
#define EX_PTZ_SCAN_DOWN			0x2B	//扫描下边界
#define EX_PTZ_VERT_SCAN_START		0x2C	//垂直扫描
#define EX_PTZ_RANDOM_SCAN_START	0x2D	//随机扫描
#define EX_PTZ_FRAME_SCAN_START		0x2E	//帧扫描
#define EX_PTZ_GETSPEED			0x2F		//获取云台速度
//-----------------预置点管理-----------------------------------
void PTZProc(REMOTECFG *remotecfg);

//------------------存储管理-------------------------------------
//存储管理指令,lck20120306
#define EX_STORAGE_REFRESH	0x01
#define EX_STORAGE_REC		0x02
#define EX_STORAGE_OK		0x04
#define EX_STORAGE_ERR		0x05
#define EX_STORAGE_FORMAT	0x06
#define EX_STORAGE_SWITCH   0x07
#define EX_STORAGE_GETRECMODE 0x08

U32 build_storage_param(char *pData);

//MStorage模块，远程指令处理,lck20120306
VOID StorageProc(REMOTECFG *remotecfg);


//------------------账户管理-------------------------------------
/**
 *@brief 帐户管理指令
 *
 */
#define EX_ACCOUNT_OK		0x01
#define EX_ACCOUNT_ERR		0x02
#define EX_ACCOUNT_REFRESH	0x03
#define EX_ACCOUNT_ADD		0x04
#define EX_ACCOUNT_DEL		0x05
#define EX_ACCOUNT_MODIFY	0x06

/**
 *@brief 将用户信息按指定格式放到字符串中
 *@param pData 用来存储用户信息的BUFFER
 *@return 用户信息的长度
 *
 */
U32 build_account_param(char *pData);

/**
 *@brief 远程设置处理
 *@param pData 要解析的数据
 *@param nSize 要解析数据的长度
 */
VOID AccountProc(REMOTECFG *remotecfg);


//------------------告警设置-------------------------------------
#define EX_MAIL_TEST	0x01	//连接ADSL消息
#define EX_ALARM_SUBMIT	0x02
#define EX_SCHEDULE_SUBMIT	0x03
#define EX_ALARM_REFRESH  0X04

U32 build_alarm_param(char *pData);
VOID MailProc(REMOTECFG *remotecfg);

//------------------报警输入-------------------------------------
#define EX_MALARMIN_SUBMIT			0x01

VOID MAlarmInProc(REMOTECFG *cfg);

//------------------门磁报警-------------------------------------
#define EX_MDOORALARM_SUBMIT			0x01

VOID MDoorAlarmProc(REMOTECFG *cfg);

//------------------云视通注册-----------------------------------
#define EX_REGISTER_SUCCESS 0x01	//注册成功
#define EX_REGISTER_FAILE	0x02	//注册失败
#define EX_REGISTER_ONLINE	0x03	//在线无需注册
VOID MRegisterProc(REMOTECFG *remotecfg);

//------------------二维码--------------------------------------
#define MAX_SIZE 3*64 * 64 + 128*1024-16
#define EX_QRCODE_YST			0x04
#define EX_QRCODE_AND			0x05
#define EX_QRCODE_IOS			0x06
#define EX_QRCODE_EX			0x07
VOID QRCodeProc(REMOTECFG *remotecfg);

//-----------------云视通别名----------------------------------
#define EX_NICKNAME_GET				0x08
#define EX_NICKNAME_SET				0x09
#define EX_NICKNAME_DEL				0x0a
#define EX_NICKNAME_GETOK			0x0b
#define EX_NICKNAME_GETFAIL			0x0c
#define EX_NICKNAME_SETOK			0x0d
#define EX_NICKNAME_SETFAIL			0x0e
#define EX_NICKNAME_DELOK			0x0f
#define EX_NICKNAME_DELFAIL			0x10
#define EX_NICKNAME_GETFAIL_INVALID	0x11
#define EX_NICKNAME_GETFAIL_OFFLINE			0x12
#define EX_NICKNAME_GETFAIL_INPROGRESS			0x13
#define EX_NICKNAME_SETFAIL_INVALID	0x14
#define EX_NICKNAME_SETFAIL_OFFLINE			0x15
#define EX_NICKNAME_SETFAIL_INPROGRESS			0x16
#define EX_NICKNAME_DELFAIL_INVALID			0x17
#define EX_NICKNAME_DELFAIL_OFFLINE			0x18
#define EX_NICKNAME_DELFAIL_INPROGRESS			0x19

VOID YSTNickNameProc(REMOTECFG *remotecfg);
//------------------sensor感兴趣区域优化：包括感兴趣区域编码，和局部曝光-----------------------------
#define EX_ROI_REFRESH		0x01
#define EX_ROI_SUBMIT		0x02
#define EX_ROI_UPDATE		0x03
#define ROI_MAX_RECT_CNT		4
typedef struct
{
	BOOL bROIReverse;
//	BOOL bROIExposureEN;
//	BOOL bROIQPEN;
	U32  nWeightQP;
	U32  nWeightExposure;
	RECT roiRect[ROI_MAX_RECT_CNT];
}roiInf;

VOID ROIProc(REMOTECFG *cfg);

char *GetKeyValue(const char *pBuffer, const char *key, char *valueBuf, int maxValueBuf);
U32 GetIntValueWithDefault(char *pBuffer, char *pItem, U32 defVal);

//----------------智能分析----------------------------------------------------------------------
#define EX_IVP_REFRESH		0x01
#define EX_IVP_SUBMIT		0x02
#define EX_IVP_UPDATE		0x03
#define EX_IVP_COUNT		0x04
#define EX_IVP_COUNT_RESET	0x05
#define EX_IVP_VR_REFRESH	0x06
#define EX_IVP_VR_UPDATE	0x07
#define EX_IVP_VR_SUBMIT	0x08
#define EX_IVP_VR_REF		0x09
#define EX_IVP_VR_TRIGER	0x10

#define EX_IVP_HIDE_SUBMIT	0x11
#define EX_IVP_TL_SUBMIT	0x12
#define EX_IVP_DETECT_SUBMIT	0x13

VOID IVPProc(REMOTECFG *remotecfg);

//----------------对讲部分功能-------------------------------------------------------------------
#define EX_CHAT_REFRESH 0x01
#define EX_CHAT_SUBMIT  0x02

VOID ChatProc(REMOTECFG* cfg);

//----------------报警输出部分功能----------------------------------------------------------------
#define EX_ALARMOUT_REFRESH 0x01
#define EX_ALARMOUT_SUBMIT  0X02

VOID AlarmOutProc(REMOTECFG* cfg);

//---------------云存储部分功能----------------------------------------------------------------------
#define EX_CLOUD_REFRESH 0x01
#define EX_CLOUD_SUBMIT  0x02

VOID CloudProc(REMOTECFG* cfg);

//-------------视频信息--------------------------------------------------------------------------
#define EX_STREAM_REFRESH 0x01
#define EX_STREAM_SUBMIT  0x02

VOID StreamProc(REMOTECFG *cfg);

//-------------远程录像回放，时间轴格式实现新增加接口-------------------------
#define EX_REPLAY_GETLIST	0x01
#define EX_REPLAY_START   0x02

VOID PlayRemoteProc(REMOTECFG* cfg);

//----------------限制部分功能----------------------------------------------------------------------
#define EX_RESTRICTION_GET	0x01//获取激活信息命令
#define EX_RESTRICTION_SET	0x02//激活命令
#define EX_RESTRICTION_GETOK	0x03//未激活
#define EX_RESTRICTION_SETOK	0x04//激活成功
#define EX_RESTRICTION_GETFAILED	0x05//获取激活信息失败
#define EX_RESTRICTION_SETFAILED	0x06//激活失败
#define EX_RESTRICTION_REP	0x07//已经激活
#define EX_RESTRICTION_REV	0x08//恢复命令
#define EX_RESTRICTION_REVOK	0x09//恢复成功
#define EX_RESTRICTION_REVFAILED	0x0A//恢复失败
VOID RestrictionProc(REMOTECFG *remotecfg);

VOID ReqIDRProc(REMOTECFG *cfg);

//------------------DebugProc----------------------
#define EX_DEBUG_TELNET			0x01
#define EX_DEBUG_TELNET_OFF		0x02
#define EX_DEBUG_REDIRECTPRINT	0x20
#define EX_DEBUG_RECOVERPRINT	0x21
#define EX_DEBUG_GETLOG			0x22
VOID DebugProc(REMOTECFG *remotecfg);

#endif

