#ifndef __LIBPTZ_H__
#define __LIBPTZ_H__
typedef enum PARITYBIT_ENUM
{
	PAR_NONE = 0,
	PAR_EVEN,
	PAR_ODD
}tParityBit_e;
typedef enum
{
	PTZ_DATAFLOW_NONE = 0,
	PTZ_DATAFLOW_HW,
	PTZ_DATAFLOW_SF

}tPtzDataFlow_e;
typedef struct tagPORTPARAMS
{
	int		nBaudRate;      // BAUDRATE_ENUM
	int		nCharSize;      // CHARSIZE_ENUM
	int		nStopBit;       // STOPBIT_ENUM
	tParityBit_e	nParityBit;     // PARITYBIT_ENUM
	tPtzDataFlow_e	nFlowCtl;       // Flow Control type(1/0)
} NC_PORTPARAMS, *PNC_PORTPARAMS;

typedef enum
{
	PTZ_PROTOCOL_HUIXUN = 1,
	PTZ_PROTOCOL_STD_PELCOP = 4,
	PTZ_PROTOCOL_YAAN = 28,
	PTZ_PROTOCOL_STD_PELCOD = 29,	
	PTZ_PROTOCOL_SHUER_PELCOD=30 ,
	PTZ_PROTOCOL_SELF=31 ,			//中维一体机自己控制自己的时侯

	PTZ_PROTOCOL_MAX
}PtzProtocol_e;

typedef struct tagPTZinfo
{
	PtzProtocol_e nProtocol;
	int			  naddress;
	NC_PORTPARAMS nHwParams;
}tPTZhwinfo;

//函数说明 : 打开串口
//参数     : char *cFileName:串口名
//返回值   : 文件描述符,如果返回-1表示打开失败
int DecoderOpenCom(const char *pFileName);

//函数说明 : 关闭串口
//参数     : int fd:串口的文件描述符
//返回值   : 0:成功; -1:失败
int DecoderCloseCom(int fd);

//函数说明 : 设置波特率
//参数     : int fd:串口的文件描述符
//           int nSpeed:波特率:B115200,B57600,B38400,B19200,B9600,B4800,B2400,B1200,B300
//返回值   : 0:成功; -1:失败
int DecoderSetComBaudrate(int fd, int nSpeed);

//函数说明 : 设置串口数据位
//参数     : int fd:串口的文件描述符
//           int nDatabits:数据位位数:5,6,7,8
//           int nStopbits:停止位位数:1,2
//           int nParity:校验位格式:PAR_NONE,PAR_ODD,PAR_EVEN
//返回值   : 0:成功; -1:失败
int DecoderSetComBits(int fd, int nDatabits, int nStopbits, int nParity);

//函数说明 : 设置串口
//参数     : int fd:串口的文件描述符
//           PNC_PORTPARAMS pParam:串口属性
//返回值   : 0:成功; -1:失败
int DecoderSetCom(int fd, PNC_PORTPARAMS pParam);


////////////////////////////串口发送接收数据////////////////////////////
void DecoderSendCommand(int fd, char *pCmd, int nLen, int nBaudRate);
int DecoderReceiveDate(int fd, char *pCmd, int nLen, int nBaudRate);
void DecoderSendCommand_Ex(int fd, unsigned char *pCmd, int nLen, int nBaudRate);


//==========================重置============================================================
void DecoderReset(int fd, int nAddress, int nProtocol, int nBaudRate);

//==========================上、下、左、右、自动==================================================
void DecoderLeftStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
void DecoderLeftStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderRightStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
void DecoderRightStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderUpStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
void DecoderUpStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderDownStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
void DecoderDownStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAutoStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
void DecoderAutoStop(int fd, int nAddress, int nProtocol, int nBaudRate);

//多方位移动
//bLeft 为真时左移，为假是右移，leftSpeed为0时不移动
//bUp 为真是上移，为假时下移，upSpeed为0时不移动
void DecoderPanTiltStart(int fd, int nAddress, int nProtocol, int nBaudRate, int bLeft, int bUp, int leftSpeed, int upSpeed);
void DecoderPanTiltStop(int fd, int nAddress, int nProtocol, int nBaudRate);

//==========================变倍、变焦、光圈======================================================
//变倍
void DecoderZoomInStart(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderZoomInStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderZoomOutStart(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderZoomOutStop(int fd, int nAddress, int nProtocol, int nBaudRate);

//变焦
void DecoderFocusNearStart(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderFocusNearStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderFocusFarStart(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderFocusFarStop(int fd, int nAddress, int nProtocol, int nBaudRate);

//光圈
void DecoderIrisOpenStart(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderIrisOpenStop(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderIrisCloseStart(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderIrisCloseStop(int fd, int nAddress, int nProtocol, int nBaudRate);

//==========================辅助==================================================================
void DecoderAUX1On(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX1Off(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX2On(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX2Off(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX3On(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX3Off(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX4On(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUX4Off(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderAUXNOn(int fd, int nAddress, int nProtocol, int nBaudRate, int n);
void DecoderAUXNOff(int fd, int nAddress, int nProtocol, int nBaudRate, int n);

//==========================扩展==================================================================
void DecoderSetLeftLimitPosition(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderSetRightLimitPosition(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderSetUpLimitPosition(int fd, int nAddress, int nProtocol, int nBaudRate);
void DecoderSetDownLimitPosition(int fd, int nAddress, int nProtocol, int nBaudRate);

void DecoderSetLimitScanSpeed(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan, int nSpeed);

void DecoderLimitScanStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan);

//结束线扫
void DecoderLimitScanStop(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan);

//开始花样扫描
void DecoderWaveScanStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);

//结束花样扫描
void DecoderWaveScanStop(int fd, int nAddress, int nProtocol, int nBaudRate);

//开始垂直扫描
void DecoderVertScanStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
//开始随机扫描
void DecoderRandomScanStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);
//开始帧扫描
void DecoderFrameScanStart(int fd, int nAddress, int nProtocol, int nBaudRate, int nSpeed);

//设置预置位
void DecoderSetPreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nNumber);
void DecoderClearPreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nNumber);
void DecoderClearAllPreset(int fd, int nAddress, int nProtocol, int nBaudRate);
//调用预置位
void DecoderLocatePreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nNumber, int nPatrolSpeed);

//开始预置位巡航
void DecoderStartPatrol(int fd, int nAddress, int nProtocol, int nBaudRate);
//停止预置位巡航
void DecoderStopPatrol(int fd, int nAddress, int nProtocol, int nBaudRate);
//开始轨迹记录
void DecoderSetScanOnPreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan);
//结束轨迹记录
void DecoderSetScanOffPreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan);
//开始轨迹巡航
void DecoderLocateScanPreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan);
//停止轨迹巡航
void DecoderStopScanPreset(int fd, int nAddress, int nProtocol, int nBaudRate, int nScan);

//==========================同力DVR的巡航接口========================================
//开始设置巡航
void DecoderSetPatrolOn(int fd, int nAddress, int nProtocol, int nBaudRate);
//结束设置巡航
void DecoderSetPatrolOff(int fd, int nAddress, int nProtocol, int nBaudRate);
//添加一个巡航点
void DecoderAddPatrol(int fd, int nAddress, int nProtocol, int nBaudRate, int nPreset, int nSpeed, int nStayTime);
//开始巡航
void DecoderStartHWPatrol(int fd, int nAddress, int nProtocol, int nBaudRate);
//结束巡航
void DecoderStopHWPatrol(int fd, int nAddress, int nProtocol, int nBaudRate);
//波特率格式转换
int Ptz_nToBaudrate(int nBaud);
//将某一点在屏幕中间放大显示
//x,y  将屏幕分成64x64个区域。x,y分别代表着各自方向的数值
//zoom 放大的倍数。其值为实际倍数x16
void DecoderZoomPosition(int fd, int nAddress, int nProtocol, int nBaudRate, int x, int y, int zoom);
extern void jv_cam_focus_far();
extern void jv_cam_stop();
extern void jv_cam_zoomin();
extern void jv_cam_zoomout();
extern void jv_cam_focus_near();
extern void jv_cam_get_zoomfocuspos( int *argv);
extern void jv_cam_get_zoompos( int *argv);
extern void jv_cam_get_focuspos( int *argv);
extern void jv_cam_zoomfocus_direct(unsigned int *argv);
extern void jv_cam_zoom_direct(unsigned int *argv);
extern void jv_cam_focus_direct(unsigned int *argv);

//球机3D定位
void DecoderZoomZone(int fd, int nAddress, int nProtocol, int nBaudRate, int x, int y, 
	int w, int h, int width, int height,int zoom);

//移动跟踪功能
void DecoderTraceObj(int fd, int nAddress, int nProtocol, int nBaudRate,int x, int y, 
	int zoom, int focus, int cmd);


#endif

