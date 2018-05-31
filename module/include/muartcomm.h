#ifndef __MUART_COMM_H__
#define __MUART_COMM_H__

#define EX_PTZ_HDWARE_INFO_REQ			0x01	//机芯请求云台硬件版本信息
#define EX_PTZ_HDWARE_INFO_RESP			0x02	//云台回应机芯硬件版本信息
#define EX_PTZ_UPDATE_START_REQ			0x03	//机芯请求云台升级
#define EX_PTZ_SFWARE_INFO_REQ			0x04	//云台请求升级文件长度和MD5吗
#define EX_PTZ_SFWARE_INFO_RESP			0x05	//机芯回应云台升级文件长度和MD5码
#define EX_PTZ_UPLOAD_START				0x06	//云台请求开始发送数据
#define EX_PTZ_UPLOAD_DATA				0x07	//云台请求继续发送数据
#define EX_PTZ_UPLOAD_OK				0x08	//机芯回应云台数据发送完成
#define EX_PTZ_UPDATE_RET				0x09	//云台回应机芯升级结果
#define EX_PTZ_UPLOAD_CANCEL			0xa0	//取消升级，原因:设备忙、权限不足
#define EX_PTZ_UPLOAD_RETRY				0xa1	//云台请求机芯重发数据

#define EX_COMTRANS_SEND				0x26	 //串口发
#define EX_COMTRANS_RESV				0x27	 //串口收
#define EX_COMTRANS_OPEN				0x28	 //串口功能打开
#define EX_COMTRANS_CLOSE				0x29	 //串口功能关闭
#define EX_COMTRANS_SET					0x2A	 //设置串口属性

//升级结果定义
#define	PTZ_UPDATE_SUCCESS		0x01	//升级成功
#define	PTZ_UPDATE_FAILED		0x02	//升级失败
#define PTZ_UPDATE_LATEST		0x03	//最新版本
#define PTZ_UPDATE_INVALID		0x04	//无效文件
#define PTZ_UPDATE_ERROR		0x05	//升级错误
#define PTZ_UPDATE_NOTFIT		0x06	//文件不匹配

typedef struct{
	U16 softver;			//软件版本
	char product[16];		//产品类别，固化在云台里
}PtzHdwareInfo_t;

typedef struct{	
	int filesize;			//软件文件长度，ver文件 为10进制数
	U8 checksum[16];		//MD5校验码
}PtzSfwareInfo_t;

typedef struct 
{
	BOOL running;
	pthread_mutex_t mutex;
}ptz_thread_group_t;

VOID ComTransProc(REMOTECFG *cfg);	//串口调试

void muartcomm_init();
#endif
