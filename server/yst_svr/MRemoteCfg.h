
/*	MRemoteCfg.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织接收PC端搜索以及设置信息的相关函数
	更改历史详见svn版本库日志
*/

#ifndef __MREMOTECFG_H__
#define __MREMOTECFG_H__
#include "jv_common.h"

#define	RC_PORT				6666
#define RC_ITEM_NUM			MAX_STREAM*64		//每个通道最大64项设置
#define RC_ITEM_LEN			64					//设置项的最大长度64字节
#define RC_DATA_SIZE		RC_ITEM_NUM * RC_ITEM_LEN + 128*1024

//远程控制指令
#define RC_DISCOVER			0x01
#define RC_GETPARAM			0x02
#define RC_SETPARAM			0x03
#define RC_VERITY			0x04
#define RC_LOADDLG			0x05
#define RC_EXTEND			0x06
#define RC_USERLIST			0x07
#define RC_PRODUCTREG		0X08
#define RC_GETSYSTIME		0x09
#define RC_SETSYSTIME		0x0a
#define RC_DEVRESTORE		0x0b
#define RC_SETPARAMOK		0x0c
#define RC_DVRBUSY			0X0d
#define RC_GETDEVLOG		0x0e
#define RC_DISCOVER_CSST	0x0f
#define RC_WEB_PROXY		0x0f//请求WEB页面
//#define RC_JSPARAM			0x10	//json格式的设置模式
#define RC_GPIN_ADD 		0x10
#define RC_GPIN_SET 		0x11
#define RC_GPIN_SELECT		0x12
#define RC_GPIN_DEL 		0x13
#define RC_BOARDEXT_CHECK	0x14
#define RC_GET_WIFI_CFG_TYPE	0x17
#define RC_GPIN_BIND_PTZPRE 0x18

typedef struct tagPACKET
{
	U32	nPacketType:5;			//包的类型，支持32种类型
	U32	nPacketCount:8;			//包总数，最大支持256个包
	U32	nPacketID:8;			//包序号
	U32	nPacketLen:11;			//包的长度，包的最大长度不超过2048字节，年(不超过2048年)
	U8	acData[RC_DATA_SIZE];
} PACKET, *PPACKET; 

//此结构体为PACKET结构体和EXTEND结构体的合并,lck20121105
typedef struct tagPACKETEX
{
	//旧packet结构体的前四字节
	int	nType:5;
	int	nCount:8;
	int	nID:8;
	int	nLen:11;

	//extend
	int	nTypeEx;
	int	nParam1;
	int	nParam2;
	int	nParam3;
	char acData[RC_DATA_SIZE];
} PACKETEX, *PPACKETEX;

//和手机通信使用的数据包,lck20121107
#define PACKET_O_SIZE			1024
#define PACKET_O_STARTCODE		0xFFFFFFFF
typedef struct
{
	int nStartCode;		//固定为PACKET_O_STARTCODE
	int nDataType;		//类型
	int reserved;		//保留字段
	int nLength;		//data字符串长度
	char acData[PACKET_O_SIZE];
}PACKET_O;

//nDataType定义:
#define RC_GET_MOPIC	0x01		//向手机发送画面配置 data:"Enabled=0;nPictureType=0"
#define RC_SET_MOPIC	0x02		//手机设置主控画面配置 data:"nPictureType=0"

//画质:
typedef enum
{
	PIC_QUALITY_HIGH = 0,	//高
	PIC_QUALITY_MID,			//中
	PIC_QUALITY_LOW,		  //低
}E_PicQuality;


//远程设置所用的结构
typedef struct tagREMOTECFG
{
	U32			nSockID;
	BOOL		bRunning;
	BOOL		bInThread;
	pthread_mutex_t mutex;	//用于分控设置的互斥琐

	//远程设置状态
	BOOL		bSetting;
	S32			nClientID;
	U32			nCmd;
	S32			nCh;
	U32			nSize;

	//远程设置数据
	PACKET		stPacket;
}REMOTECFG;

// 计算REMOTECFG所需的字节数，nSize为stPacket变量所需字节数
// 保险起见，多加32字节
#define REMOTE_CFG_SIZE(nSize)		(sizeof(REMOTECFG) - sizeof(PACKET) + (nSize) + 32)


extern REMOTECFG stRemoteCfg;

//dll名称以及宏定义
enum
{
	IPCAM_MAIN		=0,
	IPCAM_SYSTEM	=1,
	IPCAM_STREAM	=2,
	IPCAM_STORAGE	=3,
	IPCAM_ACCOUNT	=4,
	IPCAM_NETWORK	=5,
	IPCAM_LOGINERR	=6,
	IPCAM_PTZ		=7,
	IPCAM_IVP		=8,
	IPCAM_ALARM    = 9,
	IPCAM_MAX
};

#include <mrecord.h>

void remotecfg_send_self_data(int channelid,char *data,int nSize);

//通知分控关闭或打开报警
void remotecfg_alarm_on(int channelid, BOOL bOn, alarm_type_e type);

/**
 *@brief 初始化
 *
 */
int remotecfg_init(void);

/**
 *@brief 结束
 *
 */
int remotecfg_deinit(void);

void remote_start_chat(int channelid, int clientid);

//channelid值为-1时，不发送STOP消息出去。因为那代表着连接断开
void remote_stop_chat(int channelid, int clientid);

//发送对讲数据
void remote_send_chatdata(int channelid, char *buffer, int len);

/**
 *@brief 发送消息
 *@param 要发送的消息的指针
 *@note cfg指向的内存，是动态分配的，用完后需要释放
 *
 */
int remotecfg_sendmsg(REMOTECFG *cfg);

VOID SetDVRParam(REMOTECFG *cfg, char *pData);

/**
 *@brief 处理param所指的远程设置数据
 *@param 要处理的数据指针
 *@note param指向的内存，是动态分配的，用完后需要释放
 *
 */
void* remotecfg_proc(void *param);

int remotecfg_sendmsg(REMOTECFG *cfg);

void remotecfg_alarmin_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mdetect_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mbabycry_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mpir_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mivp_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mivp_vr_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mivp_hide_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mivp_left_callback(int channelid, BOOL bAlarmOn);

void remotecfg_mivp_removed_callback(int channelid, BOOL bAlarmOn);

void DoorAlarmInsert(signed char result, signed char index);
void DoorAlarmSend(unsigned char* name, int arg);
void DoorAlarmStop();

#endif

