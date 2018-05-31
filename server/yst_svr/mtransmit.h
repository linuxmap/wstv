
/*	mtransmit.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织网传相关函数
	更改历史详见svn版本库日志
*/


#include "MRemoteCfg.h"
#include "maccount.h"

#ifndef __MTRANSMIT_H__
#define __MTRANSMIT_H__

//局域网连接模式lk20140222
typedef enum{
	LAN_MODEL_DEFAULT,  		//局域网常用模式，连接数量较少，无延时
	LAN_MODEL_MAXNUM,			//局域网大连接数模式，目前限制最大连接100个，有延时
}LANModel_e;

/*连接相关数据*/
typedef struct _PNETLINKINFO
{
	U32		nChannel;
	U32		nClientID;
	char	chClientIP[16];
	U32		nClientPort;
	U32		nType;
	BOOL	bChatting;
	ACCOUNT	stNetUser;
	struct _PNETLINKINFO *next;
} NETLINKINFO, *PNETLINKINFO;

typedef struct tagYST
{
	S8		strGroup[8];
	U32		nID;
	U32		nPort;
	U32		nStatus;
	U32		nYSTPeriod;

	BOOL	bTransmit[MAX_STREAM];

	LANModel_e	eLANModel;//局域网连接模式lk20140222

	pthread_mutex_t mutex;	//用于分控连接的互斥琐
}YST, *PYST;


/*********************************************************************************************/

NETLINKINFO *__NETLINK_Get(U32 nClientID, NETLINKINFO *info);
NETLINKINFO *__NETLINK_GetByIndex(int clientIndex, NETLINKINFO *info);//用于轮询。当返回NULL时表示已经没了.clientIndex从0开始



ACCOUNT *GetClientAccount(U32 nClientID, ACCOUNT *account);

//关掉除nClientID之外的所有连接。
//当nClientID为-1时，关闭所有连接
void mtransmit_disconnect_all_but(U32 nClientID);

VOID InitYSTID();
S32 InitYST();
S32 ReleaseYST();
S32 StartYSTCh(U32 nCh, BOOL bLanOnly, U32 nBufSize);
VOID StopYSTCh(U32 nCh);
void YstOnline();

BOOL StartMOServer(U32 nCh);
BOOL StopMOServer(U32 nCh);

VOID Transmit(S32 nChannel, VOID *pData, U32 nSize, U32 nType, unsigned long long timestamp);

VOID* SendInfo2Client(VOID *pArgs);

//获取云视通参数
YST* GetYSTParam(YST* pstYST);
//设置云视通参数
VOID SetYSTParam(YST* pstYST);

//多次重试的方式，发送数据。避免失败的情况
void MT_TRY_SendChatData(int nChannel,int nClientID,unsigned char uchType,unsigned char *pBuffer,int nSize);
//lck20121025
//NVR和IPC进行广播通信
//指令是用到了就添加一个，结构体也是用到了就向后追加
//在做嵌入式NVR时可以综合考虑一下，换成JSON方式
typedef enum
{
	BC_SEARCH = 0x01,
	BC_GET,
	BC_SET,
	BC_NOPOWER,
	BC_GET_ERRORCODE,
	BC_SET_WIFI,
	BC_MAX,
}BC_CMD_E;

typedef struct tagBCPACKET
{
	U32		nCmd;
	S8		acGroup[8];
	U32		nYSTNO;
	U32		nPort;
	S8		acName[100];
	U32		nType;
	U32		nChannel;

	//验证用的管理员帐户
	char	acID[32];
	char	acPW[32];

	BOOL	bDHCP;
	U32		nlIP;
	U32		nlNM;
	U32		nlGW;
	U32		nlDNS;
	char	acMAC[20];

	U32		nErrorCode;
	int nPrivateSize;//自定义数据实际长度
	char chPrivateInfo[500];//自定义数据内容
	char nickName[36];
}BCPACKET;

#endif

