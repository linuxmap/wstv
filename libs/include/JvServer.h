#ifndef JVSERVER_H
#define JVSERVER_H
#include "JVNSDKDef.h"
#ifndef WIN32
	//#include "JVNSDKDef.h"
	#ifdef __cplusplus
		#define JOVISION_API extern "C"
	#else
		#define JOVISION_API
	#endif
#else
	#define JOVISION_API extern "C" __declspec(dllexport)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                     //
//                                      主控端接口                                                     //
//                                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/***************************************接口清单********************************************************
JVN_InitSDK -----------------01 初始化SDK资源
JVN_ReleaseSDK---------------02 释放SDK资源
JVN_RegisterCallBack --------03 设置主控端回调函数
JVN_ActiveYSTNO -------------04 激活云视通号码
JVN_InitYST -----------------05 初始化云视通服务
JVN_StartChannel ------------06 开启某通道网络服务
JVN_StopChannel -------------07 停止某通道所有服务
JVN_SendData ----------------08 发送数据(通道群发)
JVN_SendDataTo --------------09 发送数据(定向发送)
JVN_SendChatData ------------10 发送聊天信息(语音聊天和文本聊天)
JVN_EnableLog ---------------11 设置写出错日志是否有效
JVN_RegCard -----------------12 注册产品
JVN_SetLanguage -------------13 设置日志/提示信息语言(英文/中文)
JVN_GetLanguage -------------14 获取日志/提示信息语言(英文/中文)
JVN_SendAndRecvS ------------15 与最快服务器交互
JVN_StartWebServer ----------16 开启web服务
JVN_StartSelfWebServer ------17 开启自定义web服务(OEM)
JVN_StopWebServer -----------18 停止web服务
JVN_SetClientLimit ----------19 设置分控数目上限
JVN_GetClientLimit ----------20 获取分控数目上限设置值
JVN_EnablePCTCPServer -------21 开启关闭PC客户TCP服务(保留暂未使用)
JVN_EnableMOServer ----------22 开启关闭某通道的手机服务
JVN_SendMOData --------------23 发送手机数据，对TYPE_MO_TCP/TYPE_MO_UDP连接有效
JVN_Command -----------------24 运行特定指令，要求SDK执行特定操作
JVN_StartLANSerchServer -----25 开启局域网搜索服务
JVN_StopLANSerchServer ------26 停止局域网搜索服务
JVN_SetLocalNetCard ---------27 设置哪一张网卡(IPC)
JVN_SetDeviceName -----------28 设置本地设备别名
JVN_SetDomainName -----------29 设置新的域名，系统将从其获取服务器列表
JVN_SetLocalFilePath --------30 自定义本地文件存储路径，包括日志，生成的其他关键文件等
JVN_StartStreamService ------31 开启流媒体服务
JVN_StopStreamService -------32 停止流媒体服务
JVN_GetStreamServiceStatus --33 查询流媒体服务状态
JVN_StartBroadcastServer ----34 开启自定义广播服务(IPC)
JVN_StopBroadcastServer -----35 关闭自定义广播服务(IPC)
JVN_BroadcastRSP ------------36 发送自定义广播应答(IPC)
JVN_SendPlay-----------------37 向指定目标发送远程回放数据,暂用于MP4文件远程回放
JVN_EnableLANToolServer------38 开启关闭局域网生产工具服务
JVN_RegDownLoadFileName------39 注册回调函数，用于调用者特殊处理远程下载文件名
JVN_SetDeviceInfo------------40 设置设备信息
JVN_GetDeviceInfo------------41 获取设备信息
JVN_SetIPCWANMode------------42 启用IPC外网特殊处理模式
JVN_GetNetSpeedNow-----------43 获取当前网络状况
JVN_ClearBuffer-------------44 清空通道发送缓存
JVN_SetLSPrivateInfo---------45 设置本地自定义信息，用于设备搜索
JVN_SetWANClientLimit--------46 设置外网连接数目上限
JVN_GetExamItem -------------47	查询可以检测的项目
JVN_ExamItem ----------------48 检测项目
JVN_SendChannelInfo----------49 设置每一个通道参数
JVN_SendJvnInfo--------------50 设置每一个通道参数
JVN_RegNickName--------------51 注册别名
JVN_DeleteNickName-----------52 删除别名
JVN_GetNickName--------------53 获取别名
JVN_SetNickName--------------54 设置别名
JVN_RegClientMsgCallBack-----55 注册分控消息回调
JVN_GetPositionID-----------56 查询地区
JVN_SetChannelInfo----------57 设置主控通道视频信息
JVN_RTMP_Callback------------58 注册流媒体连接状态回调
JVN_RTMP_EnableSelf----------59 自定义流媒体服务
JVN_RTMP_SendData------------60 发送流媒体数据
JVN_SendDownFile-----------------61 向指定目标发送远程下载数据,暂用于sv6文件远程下载
JVN_SetSelfSerListFileName---62 定制服务器列表
*******************************************************************************************************/                                                                                                     //


/****************************************************************************
*名称  : JVN_InitSDK
*功能  : 初始化SDK资源，必须被第一个调用
*参数  : [IN] nYSTPort    用于与云视通服务通信的端口,0时默认9100
         [IN] nLocPort    用于提供主控服务通信的端口,0时默认9101
		 [IN] nSPort      用于与云视通服务器交互(激活号码，注册等)的端口,0时默认9102
		 [IN] nVersion    主控版本号，用于云视通服务器验证
		 [IN] lRSBuffer   主控端用于收发单帧数据的最小缓存，为0时默认150K，
		                  IPC高清数据单帧可能很大，可根据最大帧确定该值，若调大则对应分控端也需要相应调大.
						  该参数需求来自IPC；
		 [IN] bLANSpeedup 是否对内网连接优化提速(IPC局域网大码流传输特殊处理,其他普通码流产品建议置为FALSE)
		 [IN] stDeviceType 产品类型 详见JVNSDKDef.h 中 STDeviceType
		                   普通板卡样例：stDeviceType.nType = 1;stDeviceType.nMemLimit = 1;
						   普通IPC样例： stDeviceType.nType = 3;stDeviceType.nMemLimit = 1;
						   内存严重不足DVR样例：stDeviceType.nType = 2;stDeviceType.nMemLimit = 3;
		 [IN] nDSize       请填入sizeof(STDeviceType),用于校验传入结构合法性
		 [IN] pchPackOnline 加密数据
		 [IN] nLen			加密数据长度
		 [IN] nOEMID	OEM编号，0 是公司产品 1 是泰国定制，2为小维优化产品，以后再有OEM直接编号累加
*返回值: TRUE     成功
         FALSE    失败
*其他  : 主控端长期占用两个端口，一个用于与云视通服务器通信，一个用于与分控通信
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_InitSDK(int nYSTPort, int nLocPort, int nSPort, int nVersion, long lRSBuffer, int bLANSpeedup, STDeviceType stDeviceType, int nDSize,char *pchPackOnline, int nLen,int nOEMID);
#else
	JOVISION_API bool __stdcall	JVN_InitSDK(int nYSTPort, int nLocPort, int nSPort, int nVersion, long lRSBuffer, BOOL bLANSpeedup, STDeviceType stDeviceType, int nDSize,char *pchPackOnline, int nLen,int nOEMID);
#endif

/****************************************************************************
*名称  : JVN_ReleaseSDK
*功能  : 释放SDK资源，必须最后被调用 
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_ReleaseSDK();
#else
	JOVISION_API void __stdcall	JVN_ReleaseSDK();
#endif

/****************************************************************************
*名称  : JVN_RegisterSCallBack
*功能  : 设置主控端回调函数 
*参数  : [IN] ConnectCallBack   分控连接状况回调函数
         ...
*返回值: 无
*其他  : 主控端回调函数包括：
             分控身份验证函数            (身份验证)
             与云视通服务器通信状态函数；(上线状态)
		     与分控端通信状态函数；      (连接状态)
		     录像检索函数；              (录像检索请求)
			 远程云台控制函数；          (远程云台控制)
			 语音聊天/文本聊天函数       (远程语音和文本聊天)
			 回放控制       (目前的回放控制mp4文件)
			 别名相关返回消息+云存储开关状态通知消息
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_RegisterCallBack(FUNC_SCHECKPASS_CALLBACK CheckPassCallBack,
											FUNC_SONLINE_CALLBACK OnlineCallBack,
											FUNC_SCONNECT_CALLBACK ConnectCallBack,
											FUNC_SCHECKFILE_CALLBACK CheckFileCallBack,
											FUNC_SYTCTRL_CALLBACK YTCtrlCallBack,
											FUNC_SCHAT_CALLBACK ChatCallBack,
											FUNC_STEXT_CALLBACK TextCallBack,
											FUNC_SFPLAYCTRL_CALLBACK FPlayCtrlCallBack,
											FUNC_RECVSERVERMSG_CALLBACK FRecvMsgCallBack);
#else
	JOVISION_API void __stdcall	JVN_RegisterCallBack(FUNC_SCHECKPASS_CALLBACK CheckPassCallBack,
												  FUNC_SONLINE_CALLBACK OnlineCallBack,
												  FUNC_SCONNECT_CALLBACK ConnectCallBack,
												  FUNC_SCHECKFILE_CALLBACK CheckFileCallBack,
												  FUNC_SYTCTRL_CALLBACK YTCtrlCallBack,
												  FUNC_SCHAT_CALLBACK ChatCallBack,
												  FUNC_STEXT_CALLBACK TextCallBack,
												  FUNC_SFPLAYCTRL_CALLBACK FPlayCtrlCallBack,
												  FUNC_RECVSERVERMSG_CALLBACK FRecvMsgCallBack);
#endif

/****************************************************************************
*名称  : JVN_RegClientMsgCallBack
*功能  : 设置分控信息回调，
*参数  : [IN] FRecvMsgCallBack   分控消息回调函数，视频暂停，视频恢复
*返回值: 无

*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_RegClientMsgCallBack(FUNC_RECVCLIENTMSG_CALLBACK FRecvMsgCallBack);
#else
	JOVISION_API void __stdcall JVN_RegClientMsgCallBack(FUNC_RECVCLIENTMSG_CALLBACK FRecvMsgCallBack);
#endif

/****************************************************************************
*名称  : JVN_ActiveYSTNO
*功能  : 激活云视通号码
*参数  : [IN]  pchPackGetNum  卡信息(STGetNum结构)
         [IN]  nLen           信息长度(sizeof(STGetNum))
         [OUT] nYSTNO         成功激活的云视通号码
*返回值: TRUE  成功
         FALSE 失败
*其他  : 云视通服务器地址在内部获取；
         函数独立使用，只返回激活的YST号码，程序内不做任何记录。
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_ActiveYSTNO(char *pchPackGetNum, int nLen, int *nYSTNO);
#else
	JOVISION_API bool __stdcall	JVN_ActiveYSTNO(char *pchPackGetNum, int nLen, int &nYSTNO);
#endif

/****************************************************************************
*名称  : JVN_InitYST
*功能  : 初始化云视通服务
*参数  : [IN] 云视通号码等信息(STOnline结构)
         [IN] 信息长度
*返回值: 无
*其他  : 该函数需在启动通道云视通服务前调用，否则通道云视通服务将启动失败；
         
		 该函数只需调用一次，即，若所有通道中只要有需要启动云视通服务的，
		 在启动服务前调用一次该接口即可；
         
		 该函数记录云视通号码及服务，在上线时使用这些参数。
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_InitYST(char *pchPackOnline, int nLen);
#else
	JOVISION_API void __stdcall	JVN_InitYST(char *pchPackOnline, int nLen);
#endif

/****************************************************************************
*名称  : JVN_StartChannel
*功能  : 开启某通道网络服务
*参数  : [IN] nChannel  服务通道号 >=1  
                        特别:nChannel=-1时特殊通道，可用于避开视频通道，传输普通数据

         [IN] nType     服务启动类型，0内外网服务都启动；1只启动局域网服务；

		 [IN] bJVP2P    开启中维网络直播，内存不充足或是没有硬盘时该功能不能启用；
		                即为TRUE时为多播方式；为FALSE时为传统方式；
						建议给用户展现时对应的两个选项：常规模式(无延时)和大连结数模式(小量延时)；
						默认为常规模式即可；
						没有特殊需要,可将该功能置为FALSE,不必展现给用户；

		 [IN] lBufLen   通道视频帧缓冲区大小，单位字节, 
		                普通方式时缓存大小指两个帧序列数据量(需>400*1024,应按最大码流计算)；
						JVP2P方式时建议置为>=8M，最小为1024000；
						如果该值设置不足，可能产生每个帧序列末尾的帧丢失造成卡顿的现象；

		 [IN] chBufPath 缓存文件路径，不使用文件缓存时置为""; 

*返回值: TRUE  成功
         FALSE 失败
*其他  : 每个通道的视频帧尺寸不同，因此所需缓冲区也不同，由应用层设置；
         该函数可重复调用，参数有变化时才进行实际操作，重复的调用会被忽略；
		 应用层在更新了某些设置后，可重新调用该接口设置服务。

         若开启了jvp2p，则有两种方式进行缓存：内存方式和文件方式
		 即lbuflen 和 chBufPath必须有一个是有效的，如果同时有效则优先采用内存方式，两者都无效则失败

		 建议：1.内存方式时每个通道建议>=8M内存，效果最佳，内存若充足则建议采用内存方式；
		       2.DVR等内存非常紧张的设备可以安装硬盘并且采用文件存储方式使用jvp2p。
			     DVR内存较充足的设备建议至少每个通道分配>=1M的视频缓冲区，否则不建议使用；
				 DVR内存紧张设备无法使用jvp2p，不必展现给用户，仅默认提供'常规模式'即可；
			   3.每个通道都可以单独开启jvp2p功能，但出于应用层管理的简便以及多通道对整体带宽的竞争，
			     建议所有通道统一开启或关闭该功能；即若使用jvp2p，则所有通道都使用，否则都不用；

         bJVP2P=TRUE时，将以"大连结数和保证流畅"的模式运行，并且主控端只要提供了必要的基础上传带宽，比如2M，
		 就能满足几十上百人同时连接，若配合开通中维VIP转发，将能确保画面流畅。但实时性稍差，
		 远程画面与实际画面可能延时2s-7s；
		 bJVP2P=FALSE时，将以"尽可能无延时"的模式运行，但连接数完全取决于带宽和设备资源，是一种传统传输方式；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartChannel(int nChannel, int nType, int bJVP2P, long lBufLen, char chBufPath[256]);
#else
	JOVISION_API bool __stdcall	JVN_StartChannel(int nChannel, int nType, BOOL bJVP2P, long lBufLen, char chBufPath[MAX_PATH]);
#endif

/****************************************************************************
*名称  : JVN_StopChannel
*功能  : 停止某通道所有服务 
*参数  : [IN] nChannel 服务通道号 >=1
*返回值: 无
*其他  : 停止某个服务也可通过重复调用JVN_StartChannel实现；
         若想停止所有服务，只能通过该接口实现。
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_StopChannel(int nChannel);
#else
	JOVISION_API void __stdcall	JVN_StopChannel(int nChannel);
#endif

/****************************************************************************
*名称  : JVN_SendData
*功能  : 发送数据 
*参数  : [IN] nChannel   服务通道号 >=1
         [IN] uchType    数据类型：视频I;视频P;视频B;视频S;音频;尺寸;自定义类型;帧发送时间间隔
         [IN] pBuffer    待发数据内容,视频/音频/自定义数据时有效
		 [IN] nSize      待发数据长度,视频/音频/自定义数据时有效
		 [IN] nWidth     uchType=JVN_DATA_S时表示帧宽/uchType=JVN_CMD_FRAMETIME时表示帧间隔(单位ms)
		 [IN] nHeight    uchType=JVN_DATA_S时表示帧高/uchType=JVN_CMD_FRAMETIME时表示关键帧周期
*返回值: 无
*其他  : 以通道为单位，向通道连接的所有分控发送数据
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SendData(int nChannel,unsigned char uchType,unsigned char *pBuffer,int nSize,int nWidth,int nHeight);
#else
	JOVISION_API void __stdcall	JVN_SendData(int nChannel,unsigned char uchType,BYTE *pBuffer,int nSize,int nWidth,int nHeight);
#endif

/****************************************************************************
*名称  : JVN_SendDataTo
*功能  : 发送数据 
*参数  : [IN] nChannel   服务通道号 >=1
         [IN] uchType    数据类型：目前只用于尺寸发送尺寸;断开某连接;自定义类型
         [IN] pBuffer    待发数据内容
		 [IN] nSize      待发数据长度
		 [IN] nWidth     uchType=JVN_DATA_S时表示帧宽/uchType=JVN_CMD_FRAMETIME时表示帧间隔(单位ms)
		 [IN] nHeight    uchType=JVN_DATA_S时表示帧高/uchType=JVN_CMD_FRAMETIME时表示关键帧周期
*返回值: 无
*其他  : 向通道连接的某个具体分控发送数据
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SendDataTo(int nClientID,unsigned char uchType,unsigned char *pBuffer,int nSize,int nWidth,int nHeight);
#else
	JOVISION_API void __stdcall	JVN_SendDataTo(int nClientID,unsigned char uchType,BYTE *pBuffer,int nSize,int nWidth,int nHeight);
#endif

/****************************************************************************
*名称  : JVN_SendChatData
*功能  : 发送聊天信息(语音聊天和文本聊天)
*参数  : [IN] nChannel   服务通道号 >=1,广播式发送时有效;
         [IN] nClientID  分控ID,向指定分控发送,此时nChannel无效,优先级高于nChannel;
         [IN] uchType      数据类型：语音请求;
		                             文本请求;
		                             同意语音请求;
                                     同意文本请求;
								     语音数据;
								     文本数据;
								     语音关闭;
								     文本关闭;
         [IN] pBuffer    待发数据内容
         [IN] nSize      待发数据长度
*返回值: true   成功
         false  失败
*其他  : 调用者将聊天数据发送给请求语音服务的分控端;
         nChannel和nClientID不能同时<=0,即不能同时无效;
		 nChannel和nClientID同时>0时,nClientID优先级高,此时只向指定分控发送。
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SendChatData(int nChannel,int nClientID,unsigned char uchType,unsigned char *pBuffer,int nSize);
#else
	JOVISION_API bool __stdcall	JVN_SendChatData(int nChannel,int nClientID,unsigned char uchType,BYTE *pBuffer,int nSize);
#endif

/****************************************************************************
*名称  : JVN_EnableLog
*功能  : 设置写出错日志是否有效 
*参数  : [IN] bEnable  TRUE:出错时写日志；FALSE:不写任何日志
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_EnableLog(int bEnable);
#else
	JOVISION_API void __stdcall	JVN_EnableLog(bool bEnable);
#endif

/****************************************************************************
*名称  : JVN_SetLanguage
*功能  : 设置日志/提示信息语言(英文/中文) 
*参数  : [IN] nLgType  JVN_LANGUAGE_ENGLISH/JVN_LANGUAGE_CHINESE
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetLanguage(int nLgType);
#else
	JOVISION_API void __stdcall	JVN_SetLanguage(int nLgType);
#endif

/****************************************************************************
*名称  : JVN_GetLanguage
*功能  : 获取日志/提示信息语言(英文/中文) 
*参数  : 无
*返回值: JVN_LANGUAGE_ENGLISH/JVN_LANGUAGE_CHINESE
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetLanguage();
#else
	JOVISION_API int __stdcall	JVN_GetLanguage();
#endif

/****************************************************************************
*名称  : JVN_SetClientLimit
*功能  : 设置分控数目上限
*参数  : [IN] nChannel  通道号(>0;=0;<0)
         [IN] nMax      数目上限值 <0时表示无限制
		                nChannel<0 时表示分控总数目上限为nMax; 
						nChannel=0 时表示所有通道使用相同单通道分控数目上限为nMax; 
						nChannel>0 时表示单通道分控数目上限为nMax
*返回值: 无
*其他  : 可重复调用,以最后一次设置为准;
         总数上限和单通道上限可同时起作用;

         对普通产品，不严格区分内外网，只使用该接口就能达到限制连接数目的；
		 对于需要区分内外网连接数的产品，可配合调用JVN_SetWANClientLimit来
		 限定外网总连接数；
		 即
		 如果同时使用JVN_SetClientLimit和JVN_SetWANClinetLimit,则：
		             JVN_SetClientLimit限定的是基本连接数；
		             JVN_SetWANClientLimit单独限定的是外网连接数；
		 如果只使用JVN_SetClientLimit,限定的就是(不区分内外网)链接数目；
		 如果只使用JVN_SetWANClientLimit,限定的就是外网连接数目；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetClientLimit(int nChannel, int nMax);
#else
	JOVISION_API void __stdcall	JVN_SetClientLimit(int nChannel, int nMax);
#endif

/****************************************************************************
*名称  : JVN_GetClientLimit
*功能  : 获取分控数目上限设置值
*参数  : [IN] nChannel  通道号(>0;<0)
		                nChannel<0 时表示获取分控总数目上限; 
						nChannel>0 时表示获取单通道分控数目上限;
*返回值: 数目上限值 <=0表示无限制
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetClientLimit(int nChannel);
#else
	JOVISION_API int __stdcall	JVN_GetClientLimit(int nChannel);
#endif

/****************************************************************************
*名称  : JVN_SetWANClientLimit
*功能  : 设置外网分控数目上限
*参数  : [IN] nWANMax   数目上限值 <0时表示无限制
		                >=0表示外网分控总数目上限为nWANMax; 
		 
*返回值: 无
*其他  : 可重复调用,以最后一次设置为准;
         总数上限和单通道上限可同时起作用;

		 对普通产品，不严格区分内外网，只使用JVN_SetClientLimit接口就能达到限制连接数目的；
		 对于需要区分内外网连接数的产品，可配合调用JVN_SetWANClientLimit来
		 限定外网总连接数；
		 即
		 如果同时使用JVN_SetClientLimit和JVN_SetWANClinetLimit,则：
		 JVN_SetClientLimit限定的是基本连接数；
		 JVN_SetWANClientLimit限定的是外网连接数；

		 如果只使用JVN_SetClientLimit,限定的就是(不区分内外网)链接数目；
		 
		 如果只使用JVN_SetWANClientLimit,限定的就是外网连接数目；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetWANClientLimit(int nWANMax);
#else
	JOVISION_API void __stdcall	JVN_SetWANClientLimit(int nWANMax);
#endif

/****************************************************************************
*名称  : JVN_RegCard
*功能  : 注册产品
*参数  : [IN] chGroup    分组号，形如"A" "AAAA"
         [IN] pBuffer    待发数据内容(SOCKET_DATA_TRAN结构)
		 [IN] nSize      待发数据总长度
*返回值: TRUE  成功
         FALSE 失败
*其他  : 向最快服务发送数据
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_RegCard(char chGroup[4], unsigned char *pBuffer,int nSize);
#else
	JOVISION_API bool __stdcall	JVN_RegCard(char chGroup[4], BYTE *pBuffer,int nSize);
#endif

/****************************************************************************
*名称  : JVN_SendAndRecvS
*功能  : 与最快服务器交互
*参数  : [IN] chGroup       分组号，形如"A" "AAAA"
         [IN] pBuffer       待发数据内容
         [IN] nSize         待发数据总长度
         [OUT] pRecvBuffer  接收数据缓冲，由调用者分配
         [IN/OUT] &nRecvLen 传入接收缓冲长度，返回实际数据长度
         [IN] nTimeOut      超时时间(秒)
*返回值: TRUE  成功
FALSE 失败
*其他  : 向最快服务发送数据
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SendAndRecvS(char chGroup[4],unsigned char *pBuffer,int nSize, 
		                               unsigned char *pRecvBuffer,int *nRecvLen,int nTimeOut);
#else
	JOVISION_API bool __stdcall	JVN_SendAndRecvS(char chGroup[4], BYTE *pBuffer,int nSize, 
		                                         BYTE *pRecvBuffer,int &nRecvLen,int nTimeOut);
#endif

/****************************************************************************
*名称  : JVN_StartWebServer
*功能  : 开启web服务
*参数  : [IN] chHomeDir   目的文件所在本地路径 如"D:\\test"
         [IN] chDefIndex  目的文件名(本地)
		 [IN] chLocalIP   本地ip
         [IN] nPort       web服务端口
*返回值: TRUE  成功
		 FALSE 失败
*其他  : web服务功能即返回目的文件给客户端
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartWebServer(char chHomeDir[256], char chDefIndex[256], char chLocalIP[30], int nPort);
#else
	JOVISION_API bool __stdcall JVN_StartWebServer(char chHomeDir[256], char chDefIndex[256], char chLocalIP[30], int nPort);
#endif

/****************************************************************************
*名称  : JVN_StartSelfWebServer
*功能  : 开启自定义web服务(OEM)
*参数  : [IN] chHomeDir   目的文件所在本地路径 如"D:\\test"
         [IN] chDefIndex  目的文件名(本地)
		 [IN] chLocalIP   本地ip
         [IN] nPort       web服务端口
		 [IN] chSelfWebPos   自定义网站控件index文件位置 如"www.afdvr.com/cloudsee"
		 [IN] bOnlyUseLocal  仅使用本地web服务，不使用外网网站
*返回值: TRUE  成功
		 FALSE 失败
*其他  : web服务功能即返回目的文件给客户端
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartSelfWebServer(char chHomeDir[256], char chDefIndex[256], 
		                                     char chLocalIP[30], int nPort,
											 char chSelfWebPos[500], int bOnlyUseLocal);
#else
	JOVISION_API bool __stdcall JVN_StartSelfWebServer(char chHomeDir[256], char chDefIndex[256], 
		                                               char chLocalIP[30], int nPort,
													   char chSelfWebPos[500], BOOL bOnlyUseLocal);
#endif

/****************************************************************************
*名称  : JVN_StopWebServer
*功能  : 停止web服务
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_StopWebServer();
#else
	JOVISION_API void __stdcall JVN_StopWebServer();
#endif

/****************************************************************************
*名称  : JVN_Command
*功能  : 运行特定指令，要求SDK执行特定操作
*参数  : [IN] nChannel  本地通道 ==0时对所有音视频通道有效(不包括特殊通道)
         [IN] nCMD  指令类型
*返回值: 无
*其他  : 支持的指令参看类型定义
         目前仅支持：CMD_TYPE_CLEARBUFFER
		 主控端进行了某个操作，如果希望客户端能立即更新到当前最新的数据，可执行该指令；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_Command(int nChannel, int nCMD);
#else
	JOVISION_API void __stdcall JVN_Command(int nChannel, int nCMD);
#endif
	
/****************************************************************************
*名称  : JVN_StartLANSerchServer
*功能  : 开启局域网搜索服务
*参数  : [IN] nChannelCount 当前设备总的通道数
         [IN] nPort         服务端口号(<=0时为默认9103,建议使用默认值与分控端统一)
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartLANSerchServer(int nPort);
#else
	JOVISION_API bool __stdcall JVN_StartLANSerchServer(int nPort);
#endif

/****************************************************************************
*名称  : JVN_StopLANSerchServer
*功能  : 停止局域网搜索服务
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_StopLANSerchServer();
#else
	JOVISION_API void __stdcall JVN_StopLANSerchServer();
#endif
	
/****************************************************************************
*名称  : JVN_SetLocalNetCard
*功能  : 设置哪一张网卡 eth0,eth1,... for linux or 0, 1, 2,...for win  
*参数  : [IN] strCardName   网卡
*返回值: 成功 TRUE ,失败 FALSE
*日期  : 2012 5
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetLocalNetCard(char* strCardName);
#else
	JOVISION_API BOOL __stdcall JVN_SetLocalNetCard(char* strCardName);
#endif

#ifndef WIN32
	JOVISION_API int JVN_GetLocalNetCard(char* strCardName);
#else
	JOVISION_API BOOL __stdcall JVN_GetLocalNetCard(char* strCardName);
#endif
/****************************************************************************
*名称  : JVN_EnablePCTCPServer
*功能  : 开启或关闭PC用户的TCP服务
*参数  : [IN] bEnable 开启/关闭
*返回值: TRUE  成功
		 FALSE 失败
*其他  : TCP服务功能接收分控以TCP方式连接，以TCP方式向分控发送数据；
         目前中维分控都未使用该TCP服务,没有特殊需要可不使用该功能；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_EnablePCTCPServer(int bEnable);
#else
	JOVISION_API bool __stdcall JVN_EnablePCTCPServer(BOOL bEnable);
#endif

/****************************************************************************
*名称  : JVN_EnableMOServer
*功能  : 开启/关闭某通道的手机服务 
*参数  : [IN] nChannel   服务通道号 >=1 当=0时开启或关闭所有通道的手机服务
         [IN] bEnable    TRUE为开启；FALSE为关闭
		 [IN] nOpenType  服务开启类型：请置为0;bEnable为TRUE时有效；
		 [IN] bSendVSelf  是否单独发送手机视频数据，如果为TRUE，则手机数据和PC数据完全隔离，
		                 手机数据必须用JVN_SendMOData发送；否则手机视频数据将不需要单独发送；
						 如果把手机当分控用，给手机和给分控的是相同的码流，bSendVSelf=FALSE即可；
		 [IN] bProtocol  是否用自定义协议,当bSendVSelf时有效；
		                 TRUE时，JVN_SendMOData的数据打包需要单独添加头尾，使手机端能区别出该数据，
						         旧版主控发送的JPG数据和标准H264数据就是这种发送方式；
						 FALSE时，JVN_SendMOData的数据打包格式和分控数据一致，与分控数据的区别仅仅是数据内容，
						         如果把手机当分控用，但给手机的数据是单独的码流，可以使用该方式；
		[IN] nBufSize   手机发送缓存大小，不低于500000,传0时默认大小500000					 
*返回值: 无
*其他  : 该函数只对JVN_StartChannel开启了的通道起作用；JVN_StopChannel之后需要重新启用手机服务；
         没开启的通道将不能接受手机连接；
         重复调用将以最后一次调用为准；
		 由于旧版分控协议不同，是完全作为分控使用，该功能不能将其区分和禁用；
		 使用该版主控时公司产品已可以完全支持h264码流，数据和分控相同，不再支持JGP数据，
		 仅将手机服务开启即可；
		 <*****使用建议*****>：
		 1.如果给分控的数据和给手机的数据完全相同，则开启方式为JVN_EnableMOServer(0, TRUE, 0, FALSE, FALSE);
		 2.如果给分控的数据和给手机的数据格式相同，但是一个独立的码流，则开启方式为JVN_EnableMOServer(0, TRUE, 0, TRUE, FALSE);
		 3.如果给分控的数据和给手机的数据格式不同，需要让手机端单独处理，则开启方式为JVN_EnableMOServer(0, TRUE, 0, TRUE, TRUE);
		 理论上给手机的数据与分控数据分开，采用小帧率小码流，用第2种方式效果最佳；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_EnableMOServer(int nChannel, int bEnable, int nOpenType, int bSendVSelf, int bProtocol,int nBufSize);
#else
	JOVISION_API BOOL __stdcall	JVN_EnableMOServer(int nChannel, BOOL bEnable, int nOpenType, BOOL bSendVSelf, BOOL bProtocol,int nBufSize);
#endif

/****************************************************************************
*名称  : JVN_SendMOData
*功能  : 发送手机数据，对TYPE_MO_TCP/TYPE_MO_UDP连接有效 
*参数  : [IN] nChannel   服务通道号 >=1
         [IN] uchType    数据类型：视频:自定义类型;
         [IN] pBuffer    待发数据内容,视频/自定义数据时有效
		 [IN] nSize      待发数据长度,视频/自定义数据时有效
*返回值: 无
*其他  : 以通道为单位，向通道TYPE_MO_TCP/TYPE_MO_UDP连接的所有手机发送一些自定义数据；
         由于JGP数据的淘汰，手机数据和PC分控数据已完全相同，该接口不支持JPG数据；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SendMOData(int nChannel,unsigned char uchType,unsigned char *pBuffer,int nSize);
#else
	JOVISION_API void __stdcall	JVN_SendMOData(int nChannel,unsigned char uchType,BYTE *pBuffer,int nSize);
#endif

/****************************************************************************
*名称  : JVN_StartStreamService
*功能  : 开启流媒体服务
*参数  : [IN] nChannel   服务通道号 >=1
         [IN] pSServerIP    流媒体服务器IP;
         [IN] nPort   流媒体服务器端口
	     [IN] bAutoConnect 开启服务失败是否自动重连
*返回值: 成功
         失败
*其他  : 当通道以普通方式运行时，流媒体服务器有效；
         当通道以中维网络直播方式运行时，流媒体服务器无效；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartStreamService(int nChannel,char *pSServerIP,int nPort,int bAutoConnect);
#else
	JOVISION_API BOOL __stdcall JVN_StartStreamService(int nChannel,char *pSServerIP,int nPort,BOOL bAutoConnect);
#endif

/****************************************************************************
*名称  : JVN_StopStreamService
*功能  : 停止流媒体服务
*参数  : [IN] nChannel   服务通道号 >=1
*返回值: 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_StopStreamService(int nChannel);
#else
	JOVISION_API void __stdcall JVN_StopStreamService(int nChannel);
#endif

/****************************************************************************
*名称  : JVN_GetStreamServiceStatus
*功能  : 查询流媒体服务状态
*参数  : [IN] nChannel   服务通道号 >=1
         [OUT] pSServerIP    流媒体服务器IP;
         [OUT] nPort   流媒体服务器端口
		 [OUT] bAutoConnect 流媒体服务是否正在自动重连
*返回值: 流媒体服务是否成功开启
*日期  : 2012 2
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetStreamServiceStatus(int nChannel,char *pSServerIP,int *nPort,int *bAutoConnect);
#else
	JOVISION_API BOOL __stdcall JVN_GetStreamServiceStatus(int nChannel,char *pSServerIP,int *nPort,BOOL *bAutoConnect);
#endif

/****************************************************************************
*名称  : JVN_SetDomainName 
*功能  : 设置新的域名，系统将从其获取服务器列表
*参数  : [IN]  pchDomainName     域名
         [IN]  pchPathName       域名下的文件路径名 形如："/down/YSTOEM/yst0.txt"
*返回值: TRUE  成功
         FALSE 失败
*其他  : 系统初始化(JVN_InitSDK)完后设置
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetDomainName(char *pchDomainName,char *pchPathName);
#else
	JOVISION_API BOOL __stdcall	JVN_SetDomainName(char *pchDomainName,char *pchPathName);
#endif

/****************************************************************************
*名称  : JVN_SetDeviceName
*功能  : 设置本地设备别名 
*参数  : [IN] chDeviceName   设备别名
*返回值: 无
*其他  : 为设备起一个别名，局域网设备搜索中可作搜索和显示使用；
         重复调用以最后一次调用为有效；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetDeviceName(char chDeviceName[100]);
#else
	JOVISION_API void __stdcall	JVN_SetDeviceName(char chDeviceName[100]);
#endif

/****************************************************************************
*名称  : JVN_SetLocalFilePath
*功能  : 自定义本地文件存储路径，包括日志，生成的其他关键文件等 
*参数  : [IN] chLocalPath  路径 形如："C:\\jovision"  其中jovision是文件夹
*返回值: 无
*其他  : 参数使用内存拷贝时请注意初始化，字符串需以'\0'结束
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetLocalFilePath(char chLocalPath[256]);
#else
	JOVISION_API BOOL __stdcall	JVN_SetLocalFilePath(char chLocalPath[256]);
#endif

/****************************************************************************
*名称  : JVN_StartBroadcastServer
*功能  : 开启局域网广播服务
*参数  : [IN] nPort    服务端口号(<=0时为默认9106,建议使用默认值与分控端统一)
         [IN] BCData   广播数据回调函数
*返回值: 成功/失败
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartBroadcastServer(int nPort, FUNC_SBCDATA_CALLBACK BCData);
#else
	JOVISION_API BOOL __stdcall JVN_StartBroadcastServer(int nPort, FUNC_SBCDATA_CALLBACK BCData);
#endif

/****************************************************************************
*名称  : JVN_StopBroadcastServer
*功能  : 停止局域网广播服务
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_StopBroadcastServer();
#else
	JOVISION_API void __stdcall JVN_StopBroadcastServer();
#endif

/****************************************************************************
*名称  : JVN_BroadcastRSP
*功能  : 局域网广播响应
*参数  : [IN] nBCID  广播标识，取自回调函数
         [IN] pBuffer 广播净载数据
		 [IN] nSize   广播净载数据长度
		 [IN] nDesPort 广播目标端口，取自回调函数，或是与分控约定固定
*返回值: 成功/失败
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_BroadcastRSP(int nBCID, unsigned char *pBuffer, int nSize, int nDesPort);
#else
	JOVISION_API BOOL __stdcall JVN_BroadcastRSP(int nBCID, BYTE *pBuffer, int nSize, int nDesPort);
#endif

/****************************************************************************
*名称  : JVN_SendPlay
*功能  : 发送回放MP4数据 
*参数  : [IN] nClientID   连接号
         [IN] uchType     类型
		 [IN] nConnectionType     连接类型
		 [IN] ucFrameType    帧类型	JVN_DATA_I JVN_DATA_S...
         [IN] pBuffer    待发数据内容,保留
		 [IN] nSize      待发数据长度,保留
         [IN] nWidth    宽度
		 [IN] nHeight      高度
		 [IN] nTotalFram      总帧数
		 [IN] time  时间戳
		 [IN] frame 帧序列号
*返回值: 无
*其他  : 向通道连接的某个具体分控发送数据
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SendPlay(int nClientID,unsigned char uchType,int nConnectionType,unsigned char ucFrameType,unsigned char *pBuffer,int nSize,int nWidth,int nHeight,int nTotalFram,unsigned long long time,int frame);
#else
	JOVISION_API void __stdcall	JVN_SendPlay(int nClientID,BYTE uchType,int nConnectionType,BYTE ucFrameType,BYTE *pBuffer,int nSize,int nWidth,int nHeight,int nTotalFram,char*  time,int frame);
#endif


/****************************************************************************
*名称  : JVN_EnableLANToolServer
*功能  : 开启或关闭局域网生产工具服务
*参数  : [IN] bEnable         开启/关闭
         [IN] nPort           本地使用的端口，=0时默认为9104
         [IN] LanToolCallBack 工具回调函数
*返回值: TRUE  成功
		 FALSE 失败
*其他  : 局域网生产工具会向本地询问号码信息，并且反馈生产相关的附加信息
         信息详细说明请参考回调函数说明。
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_EnableLANToolServer(int bEnable, int nPort, FUNC_LANTOOL_CALLBACK LanToolCallBack);
#else
	JOVISION_API bool __stdcall JVN_EnableLANToolServer(BOOL bEnable, int nPort, FUNC_LANTOOL_CALLBACK LanToolCallBack);
#endif

/****************************************************************************
*名称  : JVN_RegDownLoadFileName
*功能  : 注册回调函数，用于调用者特殊处理远程下载文件名
*参数  : [IN] DLFNameCallBack 远程下载文件名处理回调函数
*返回值: 无
*其他  : 普通产品不必使用，有特殊要求的可对客户端的下载文件名做二次处理
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_RegDownLoadFileName(FUNC_DLFNAME_CALLBACK DLFNameCallBack);
#else
	JOVISION_API bool __stdcall JVN_RegDownLoadFileName(FUNC_DLFNAME_CALLBACK DLFNameCallBack);
#endif

/****************************************************************************
*名称  : JVN_SetIPCWANMode
*功能  : 启用IPC外网特殊处理模式
*参数  : 无
*返回值: 无
*其他  : 普通产品不必使用，有特殊要求的大码流产品可以使用；
         启用后，外网传输将采用特别处理来减少对内网连接的干扰；
		 在JVN_InitSDK后，JVN_StartChannel前调用即可；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetIPCWANMode();
#else
	JOVISION_API void __stdcall JVN_SetIPCWANMode();
#endif

/****************************************************************************
*名称  : JVN_SetDeviceInfo
*功能  : 设置设备信息
*参数  : [IN] pstInfo 设备信息结构体
      	 [IN] nSize   设备信息结构体大小,用来前后兼容
	     [IN] nflag   设置选项标志 
*返回值: 成功返回0，失败返回-1
*其他  : 如设置设备支持的网络模式为有线+wifi，当前正在使用的是wifi，则参数是: 
		 nflag = DEV_SET_NET;//设置设备支持的网络模式
		 pstInfo->nNetMod= NET_MOD_WIFI | NET_MOD_WIRED;
         pstInfo->nNetMod= NET_MOD_WIFI;//设备当前使用的是wifi
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetDeviceInfo(STDEVINFO *pstInfo, int nSize,int nflag);
#else
	JOVISION_API int JVN_SetDeviceInfo(STDEVINFO *pstInfo, int nSize,int nflag);
#endif
	
/****************************************************************************
*名称  : JVNS_GetDeviceInfo
*功能  : 获取设备信息
*参数  : [OUT] pstInfo 设备信息结构体
	     [IN]  nSize   设备信息结构体大小,用来前后兼容
*返回值: 成功返回0，失败返回-1
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetDeviceInfo(STDEVINFO *pstInfo,int nSize);
#else
	JOVISION_API int JVN_GetDeviceInfo(STDEVINFO *pstInfo,int nSize);
#endif

/****************************************************************************
*名称  : JVN_GetNetSpeedNow
*功能  : 获取当前网络状况
*参数  : [IN] nChannel	通道号
		 [IN] nClientID 连接号
		 [IN] nInterval	计算间隔, 该参数缺省时为默认时间间隔16秒
		 [OUT] pSpeed 返回当前网络状况，单位B/s
		 [OUT] pSendOK TRUE表示发送正常,FALSE表示有发送失败，通常是因为带宽不足
*返回值: 成功返回值>0，失败返回-1
*其他  : 返回当前网络状况值，失败时原因主要有视频传输没有开启，或者参数错误
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetNetSpeedNow(int nChannel, int nClientID, int nInterval, int* pSpeed, int* pSendOK, unsigned long *pdwFrameDelay);
#else
	JOVISION_API int __stdcall JVN_GetNetSpeedNow(int nChannel, int nClientID, int nInterval, int* pSpeed, BOOL* pSendOK, DWORD *pdwFrameDelay);
#endif

	/****************************************************************************
*名称  : JVN_SetLSPrivateInfo
*功能  : 设置本地自定义信息，用于设备搜索
*参数  : [IN]	chPrivateInfo  自定义信息
		 [IN]	nSize          自定义信息长度		 
*返回值: 无
*其他  : 可重复调用，以最后一次调用为准，之前的内容会被覆盖；
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetLSPrivateInfo(char chPrivateInfo[500], int nSize);
#else
	JOVISION_API void __stdcall JVN_SetLSPrivateInfo(char chPrivateInfo[500], int nSize);
#endif

/****************************************************************************
*名称  : JVNS_ClearBuffer
*功能  : 清空通道发送缓存
*参数  : [IN]	nChannel	通道号
*返回值: 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_ClearBuffer(int nChannel);
#else
	JOVISION_API void __stdcall	JVN_ClearBuffer(int nChannel);
#endif

/****************************************************************************
*名称  : JVN_RegNickName
*功能  : 注册别名
*参数  : [IN]	chNickName  别名，支持6-32字节UTF8字符串，禁止使用其他字符集格式的字符串,字符串中必须包含一个特殊字符，禁止使用@ 建议& * _
*返回值: 返回注册情况：0开始进行注册; -1别名无效; -2未上线; -3正在注册中
*其他  : 注册返回结果见回调函数FUNC_RECVSERVERMSG_CALLBACK 类型为0
*****************************************************************************/

#ifndef WIN32
	JOVISION_API int JVN_RegNickName(char chNickName[36]);
#else
	JOVISION_API int __stdcall JVN_RegNickName(char chNickName[36]);
#endif

/****************************************************************************
*名称  : JVN_DeleteNickName
*功能  : 删除别名
*参数  : [IN]	chNickName  别名，支持6-32字节UTF8字符串，禁止使用其他字符集格式的字符串,字符串中必须包含一个特殊字符，禁止使用@ 建议 & * _
*返回值: 返回删除情况：0开始进行删除; -1别名无效; -2未上线; -3正在删除中
*其他  : 删除是否成功见回调函数FUNC_RECVSERVERMSG_CALLBACK 类型为2
*****************************************************************************/

#ifndef WIN32
	JOVISION_API int JVN_DeleteNickName(char chNickName[36]);
#else
	JOVISION_API int __stdcall JVN_DeleteNickName(char chNickName[36]);
#endif

/****************************************************************************
*名称  : JVN_GetNickName
*功能  : 获取别名
*参数  : 无
*返回值: 返回获取情况：0开始进行获取; -1号码无效;-2正在获取中，稍后再试
*其他  : 获取结果见回调函数FUNC_RECVSERVERMSG_CALLBACK 类型为3
*****************************************************************************/

#ifndef WIN32
	JOVISION_API int JVN_GetNickName();
#else
	JOVISION_API int __stdcall JVN_GetNickName();
#endif

/****************************************************************************
*名称  : JVN_SendJvnInfo
*功能  : 设置每一个通道参数
*参数  : [IN]	chnn_info	通道参数
		 [IN]	svrAddr		服务器地址，通过回调函数FUNC_RECVSERVERMSG_CALLBACK获取		 
*返回值: 无
*其他  : 写入请求的信息，收到回调请求后再进行参数信息的发送
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SendJvnInfo(JVSERVER_INFO *jvsvr_info, struct sockaddr_in *svrAddr);
#else
	JOVISION_API void __stdcall JVN_SendJvnInfo(JVSERVER_INFO *jvsvr_info, SOCKADDR_IN *svrAddr);
#endif

/****************************************************************************
*名称  : JVN_SendChannelInfo
*功能  : 设置每一个通道参数
*参数  : [IN]	nMsgLen		信息长度
		 [IN]	chChnInfo	通道信息CHANNEL_INFO(多个), 	
		 [IN]	nConnCount  每一个号码对应一个连接数，用int表示，低2字节表示p2p连接数，最高1字节表示转发连接数，次高1字节表示手机连接数
*返回值: 无
*其他  : 首次启动时所有通道的信息，以后CHANNEL_INFO中的参数发生变化时再次调用
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SendChannelInfo(int nMsgLen, char *chChnInfo, int nConnCount);
#else
	JOVISION_API void __stdcall	JVN_SendChannelInfo(int nMsgLen, char *chChnInfo, int nConnCount);
#endif


/****************************************************************************
*名称  : JVN_GetExamItem
*功能  : 获取诊断项目
*参数  : [IN,OUT]	pExamItem  需要诊断的项目列表4BYTE 长度 +文本 [+ 4BYTE + 文本...]
[IN]	nSize          结果回调函数		 
*返回值: int 可以检测项的数目 ==0 没有检测项 <0 项目列表缓存太小
	*****************************************************************************/
	
#ifndef WIN32
	JOVISION_API int  JVN_GetExamItem(char *pExamItem,int nSize);
#else
		JOVISION_API int __stdcall	JVN_GetExamItem(char *pExamItem,int nSize);
#endif
	
/****************************************************************************
*名称  : JVN_ExamItem
*功能  : 诊断某一项
*参数  : [IN]	nExamType  诊断类型 ：-1 全部诊断 其他根据返回的已知类型诊断
[IN]	pUserInfo          诊断时用户填写的信息		 
[IN]	callBackExam          结果回调函数		 
*返回值: 无
*****************************************************************************/
	
#ifndef WIN32
	JOVISION_API int  JVN_ExamItem(int nExamType,char* pUserInfo,FUNC_EXAM_CALLBACK callBackExam);
#else
	JOVISION_API int __stdcall	JVN_ExamItem(int nExamType,char* pUserInfo,FUNC_EXAM_CALLBACK callBackExam);
#endif

	/****************************************************************************
*名称  : JVN_GetPositionID
*功能  : 查询当前所在的区域
*参数  : nGetType 查询类型 1 先通过第三方查询 2先通过afdvr查询
*返回值: 0 国内  1 国外 -1 出错，未知
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetPositionID(int nGetType);
#else
	JOVISION_API int __stdcall JVN_GetPositionID(int nGetType);
#endif

/****************************************************************************
*名称  : JVN_SetNickName
*功能  : 设置昵称 
*参数  : [IN] chDeviceName   设备昵称
*返回值: 无
*其他  : 
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_SetNickName(char chNickName[32]);
#else
	JOVISION_API void __stdcall	JVN_SetNickName(char chNickName[32]);
#endif

/****************************************************************************
*名称  : JVN_SetChannelInfo
*功能  : 设置通道的数据信息 打开通道服务后可以调用，码流参数改变后调用
*参数  : nChannelID 需要设置的通道
		 pData 设置数据 JVRTMP_Metadata_t
		 nSize 数据大小
*返回值: -1 未初始化 0 未找到通道 1 成功
	*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetChannelInfo(int nChannelID,char* pData,int nSize);
#else
	JOVISION_API int __stdcall JVN_SetChannelInfo(int nChannelID,char* pData,int nSize);
#endif

	/****************************************************************************
*名称  : JVN_RTMP_Callback
*功能  : 设置流媒体服务器连接的回调
*参数  : callBack 回调函数
*返回值: -1 未初始化  1 成功
*****************************************************************************/
#ifndef WIN32
JOVISION_API int JVN_RTMP_Callback(FUNC_SRTMP_CONNECT_CALLBACK callBack);
#else
JOVISION_API int __stdcall JVN_RTMP_Callback(FUNC_SRTMP_CONNECT_CALLBACK callBack);
#endif


/****************************************************************************
*名称  : JVN_RTMP_EnableSelfDefine
*功能  : 自定义流媒体服务器，用户可以自行搭建流媒体服务器,StartChannel后即可调用
*参数  : nEnable ==1 设置流媒体服务器 == 0 不设置或取消已经设置的流媒体服务器
		nChannel 需要设置或取消的通道号
		pServerURL 需要设置的流媒体服务器地址 如 rtmp://192.168.100.10:1935/a381_1 字符区分大小写
nSize 数据大小
*返回值: -1 未初始化 0 未找到通道 1 成功
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_RTMP_EnableSelfDefine(int nEnable,int nChannel,char* pServerURL);
#else
	JOVISION_API int __stdcall JVN_RTMP_EnableSelfDefine(int nEnable,int nChannel,char* pServerURL);
#endif

/****************************************************************************
*名称  : JVN_RTMP_SendData
*功能  : 发送流媒体的数据
*参数  : nChannel 需要发送的通道号
		uchType 帧类型 I O P等
		pBuffer 数据
		nSize 数据大小
		nSpsCount 暂时写0
		nPpsCount 暂时写0
nSize 数据大小
*返回值: -1 未初始化 0 未找到通道 1 成功
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_RTMP_SendData(int nChannel,unsigned char uchType,unsigned char *pBuffer,int nSize,int nSpsCount, int nPpsCount);
#else
	JOVISION_API int __stdcall JVN_RTMP_SendData(int nChannel,unsigned char uchType,unsigned char *pBuffer,int nSize,int nSpsCount, int nPpsCount);
#endif

	
/****************************************************************************
*名称  : JVN_GetCloudStoreStatus
*功能  : 查询主控是否支持云存储
*参数  : chCloudBuff 获取云存储参数存储空间，申请内存建议不小于128字节 格式： bucket:host 
		 nBuffSize长度为sizeof(chCloudBuff) 建议不小于128字节
*返回值: -1 系统未初始化 0 未查询到 1 支持 2 不支持或已经到期
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetCloudStoreStatus(char *chCloudBuff, int nBuffSize);
#else
	JOVISION_API int __stdcall JVN_GetCloudStoreStatus(char *chCloudBuff, int nBuffSize);
#endif

	

	/**
	* 上传指定的文件到云存储
	* @param year			[in] 上传数据所在的年
	* @param month			[in] 上传数据所在的月
	* @param day			[in] 上传数据所在的日
	* @param name			[in] 上传到云存储后显示的名称
	* @param local_file		[in] 要上传的本地文件路径
	* @param nFlag			[in] 上传文件标识，在生成报警文件，报警文件未录完时，收到云存储停止命令，该标志置为2，其他为0
	* @param nType	    	[in] 类型 一般类型0 门磁报警1
	* @return 0 返回成功 <0 失败
	* @return 0 返回成功     其他值[错误码] 失败
	* @retval 0 表示成功
	* @retval 其他 表示失败 错误码400 403 404 411 -1网络库未初始化 -4文件和报警文本不匹配
	*/
#ifndef WIN32
	JOVISION_API int JVN_FileUpload(const char *local_file, int nType);
#else
	JOVISION_API int __stdcall JVN_FileUpload(const char *local_file, int nType);
#endif
	
	
	/****************************************************************************
	*名称  : JVN_PushAlarmMsg
	*功能  : 推送报警文本消息
	*参数  : alarm_para 报警消息参数结构体
	*返回值: 0 成功 -1 系统未初始化 -2 号码未初始化 -3 数据长度超长（最大800字节）
	-4图片路径无效 -5报警视频路径无效 -6参数指针无效
	*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_PushAlarmMsg(ALARM_TEXT_PARA *alarm_para);
#else
	JOVISION_API int __stdcall JVN_PushAlarmMsg(ALARM_TEXT_PARA *alarm_para);
#endif
	
	/****************************************************************************
	*名称  : JVN_InitIPCPara
	*功能  : 初始化家用IPC上报参数
	*参数  : ipcPara 上报参数结构体
	*返回值: 0 成功 -1 系统未初始化 -2参数指针无效
	*其他  : SDK初始化完成后JVN_InitYST初始化之前必须调用JVNS_InitIPCPara接口
	*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_InitIPCPara(IPC_INIT_PARA *ipcPara);
#else
	JOVISION_API int __stdcall JVN_InitIPCPara(IPC_INIT_PARA *ipcPara);
#endif
	


/****************************************************************************
*名称  : JVNS_InitAccount
*功能  : 初始化账号
*参数  : [IN] 账号信息(AccountOnline结构)
         [IN] 信息长度
*返回值: 无
*其他  : 该函数需在启动通道云视通服务前调用，否则通道云视通服务将启动失败；
         
		 该函数只需调用一次，即，若所有通道中只要有需要启动云视通服务的，
		 在启动服务前调用一次该接口即可；

		 该函数用来实现账号上线功能
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_InitAccount(char *pchPackOnline, int nLen);
#else
	JOVISION_API void __stdcall	JVN_InitAccount(char *pchPackOnline, int nLen);
#endif


	/****************************************************************************
*名称  : JVN_CloudStoreInit
*功能  : 准备上传函数
*参数  : [IN] year 时间 年
		[IN] month 时间 月
		[IN] day 时间 日
		[IN] name 名称
		[OUT] id 标志号 以后传入缓存和关闭时需要输入

*返回值: -1 未初始化或未开通 0 失败 1 成功
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_CloudStoreInit(int year,int month,int day,char* name,int len,int* id);
#else
	JOVISION_API int __stdcall	JVN_CloudStoreInit(int year,int month,int day,char* name,int len,int* id);
#endif


	
/****************************************************************************
*名称  : JVN_CloudStoreSendData
*功能  : 上传数据
*参数  : [IN] id 初始化时的ID编号
		[IN] buff 需要上传的数据
		[IN] len 数据大小
		[IN] nType 类型 0 是一般数据 1 门磁数据

*返回值: >0 成功 0 失败 -1 未初始化或未开通
*****************************************************************************/


#ifndef WIN32
	JOVISION_API int JVN_CloudStoreSendData(int id,char* buff,int len,int nType);
#else
	JOVISION_API int __stdcall	JVN_CloudStoreSendData(int id,char* buff,int len,int nType);
#endif


	/****************************************************************************
*名称  : JVN_CloudStoreFinish
*功能  : 上传完成
*参数  : [IN] id 初始化时的返回的编号

*返回值: 1 成功 其他为失败
*****************************************************************************/

#ifndef WIN32
	JOVISION_API int JVN_CloudStoreFinish(int id);
#else
	JOVISION_API int __stdcall	JVN_CloudStoreFinish(int id);
#endif

	/****************************************************************************
*名称  : JVN_GetTimeZone
*功能  : 获取当前的时区 函数阻塞调用 尽量启用单独线程后台处理
*参数  : [OUT] fTimeZone 需要返回的时区对应的数值

*返回值: -1 未初始化网络库 0 查询失败 1 成功
*****************************************************************************/

#ifndef WIN32
	JOVISION_API int JVN_GetTimeZone(float* fTimeZone);
#else
	JOVISION_API int __stdcall	JVN_GetTimeZone(float* fTimeZone);
#endif

/****************************************************************************
*名称  : JVN_SendDownFile
*功能  : 发送远程下载数据 
*参数  : [IN] nClientID   连接号
         [IN] uchType     类型 JVN_RSP_DOWNLOADSVHEAD，JVN_RSP_DOWNLOADSVDATA...
		 [IN] nConnectionType     连接类型
         [IN] pBuffer    待发数据内容,注意数据内容要从pBuff+5 开始
		 [IN] nSize      待发数据长度
		 [IN] nTotalFram      总帧数，保留
*返回值: true，发送成功，false发送失败
*其他  : 
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SendDownFile(int nClientID,unsigned char uchType,int nConnectionType,unsigned char *pBuffer,int nSize,int nTotalFram);
#else
	JOVISION_API int __stdcall	JVN_SendDownFile(int nClientID,unsigned char uchType,int nConnectionType,unsigned char *pBuffer,int nSize,int nTotalFram);
#endif

/****************************************************************************
*名称  : JVN_RegRemoteDownLoadCallBack
*功能  : 注册远程下载回调函数
*参数  : [IN] DLFNameCallBack 远程下载回调函数
         [IN] nType 0,mp4格式下载；1，sv7格式下载
*返回值: 无
*其他  : 
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_RegRemoteDownLoadCallBack(FUNC_REMOTEDL_CALLBACK DLFNameCallBack,int nType);
#else
	JOVISION_API void __stdcall JVN_RegRemoteDownLoadCallBack(FUNC_REMOTEDL_CALLBACK DLFNameCallBack,int nType);
#endif

/****************************************************************************
*名称  : JVN_ActiveHeartBeat
*功能  : 激活猫眼设备
*参数  : void
*返回值: 无
*其他  : 
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_ActiveHeartBeat(void);
#else
	JOVISION_API void __stdcall	JVN_ActiveHeartBeat(void);
#endif

/****************************************************************************
*名称  : JVN_SetSelfSerListFileName 
*功能  : 设置自定义云视通号码信息，系统将根据此信息获取服务器列表
*参数  : [IN]  pchSerHome  主力服务器列表
		 [IN]  pchSerAll	 全部服务器列表
*返回值: TRUE  成功
         FALSE 失败
*其他  : 系统初始化(JVN_InitSDK)完后设置，在JVN_InitYST之前调用
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetSelfSerListFileName(char *pchSerHome, char *pchSerAll);
#else
	JOVISION_API BOOL __stdcall	JVN_SetSelfSerListFileName(char *pchSerHome, char *pchSerAll);
#endif


/****************************************************************************
*名称  : JVN_SetSelfCallback
*功能  : 设置设置自定义回调函数
*参数  : call 需要设置的回调函数
*返回值: 无
*说明  : 初始化网络库后可以调用
*****************************************************************************/

#ifndef WIN32
	JOVISION_API void JVN_SetSelfCallback(FUNC_SELF_COMMN_CALLBACK call);
#else
	JOVISION_API void __stdcall JVN_SetSelfCallback(FUNC_SELF_COMMN_CALLBACK call);
#endif

	/****************************************************************************
*名称  : JVN_SendSelfData
*功能  : 发送自定义数据
*参数  : 
	nClientID	客户端ID
	uchType		消息类型
	pBuffer		需要发送的数据
	nSize		需要发送的数据大小
*返回值: 
*说明  : 链接建立后可以调用
*****************************************************************************/
	
#ifndef WIN32
	JOVISION_API void JVN_SendSelfData(const int nClientID,const unsigned int uchType,const unsigned char *pBuffer,const unsigned int nSize);
#else
	JOVISION_API void __stdcall JVN_SendSelfData(const int nClientID,const unsigned int uchType,const unsigned char *pBuffer,const unsigned int nSize);
#endif


/****************************************************************************
*名称  : JVN_EnableAcceptFile
*功能  : 允许接收来自分控发来的文件,如MP3
*参数  : call 需要设置的回调函数
*返回值: 无
*说明  : 初始化网络库后可以调用
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_EnableAcceptFile(FUNC_ACCEPTFILE_CALLBACK call);
#else
	JOVISION_API void __stdcall JVN_EnableAcceptFile(FUNC_ACCEPTFILE_CALLBACK call);
#endif

/****************************************************************************
*名称  : JVN_RequestFileData
*功能  : 允许接收来自分控发来的文件,如MP3
*参数  : 
	nClientID 客户端ID
	nMaxLen 当前请求的最大数
*返回值: 无
*说明  : 初始化网络库后可以调用
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_RequestFileData(const int nClientID,const int nMaxLen);
#else
	JOVISION_API void __stdcall JVN_RequestFileData(const int nClientID,const int nMaxLen);
#endif

/****************************************************************************
*名称  : JVN_StartBroadcastSelfServer
*功能  : 开启局域网广播服务
*参数  : [IN] nPort    服务端口号(<=0时为默认9108,建议使用默认值与分控端统一)
         [IN] BCData   广播数据回调函数
*返回值: 成功/失败
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_StartBroadcastSelfServer(int nPort, FUNC_SBCSELFDATA_CALLBACK BCSelfData);
#else
	JOVISION_API BOOL __stdcall JVN_StartBroadcastSelfServer(int nPort, FUNC_SBCSELFDATA_CALLBACK BCSelfData);
#endif

/****************************************************************************
*名称  : JVN_StopBroadcastSelfServer
*功能  : 停止局域网广播服务
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_StopBroadcastSelfServer();
#else
	JOVISION_API void __stdcall JVN_StopBroadcastSelfServer();
#endif

/****************************************************************************
*名称  : JVN_BroadcastSelfRSP
*功能  : 局域网广播响应
*参数  : [IN] nBCID  广播标识，取自回调函数
         [IN] pBuffer 广播净载数据
		 [IN] nSize   广播净载数据长度
		 [IN] nDesPort 广播目标端口，取自回调函数，或是与分控约定固定
*返回值: 成功/失败
*其他  : 无
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_BroadcastSelfRSP(unsigned char *pBuffer, int nSize, int nDesPort);
#else
	JOVISION_API int __stdcall JVN_BroadcastSelfRSP(BYTE *pBuffer, int nSize, int nDesPort);
#endif

/****************************************************************************
*名称  : JVN_RegAccpetClientCallBack
*功能  : 设置主控处理是否接收连接回调（主要用于猫眼，在播放录像时，手机不能进行连接，直接返回提示信息（主控应用层来决定））
*参数  : [IN] FAccpetClientCallBack   主控处理是否接收连接回调函数，比如正在录像回放，分控禁止连接
*返回值: 无

*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_RegAccpetClientCallBack(FUNC_ACCEPTCLIENT_CALLBACK FAccpetClientCallBack);
#else
	JOVISION_API void __stdcall JVN_RegAccpetClientCallBack(FUNC_ACCEPTCLIENT_CALLBACK FAccpetClientCallBack);
#endif

/****************************************************************************
*名称  : JVN_GetSDKVer
*功能  : 获取网络库版本号信息
*参数  : [OUT] pVer   主控版本号
		 [OUT] pDescriptionVer   主控版本号的字符串表示
*返回值: -1 未初始化 0 正确获取到

*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_GetSDKVer(unsigned int* pVer,char* pDescriptionVer);
#else
	JOVISION_API int __stdcall JVN_GetSDKVer(unsigned int* pVer,char* pDescriptionVer);
#endif

/****************************************************************************
*功能  : 设置视频帧的头
*参数  : 
		nChannelID	对应的通道
		pBuffer		需要的数据
		nSize		数据大小
		*返回值: 设置的数据大小
*说明  : 打开通道服务后可以调用，每次改变通道的码流、分辨率等信息(0帧)时调一次
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetFrameHead(const int nChannelID,const unsigned char *pBuffer,const unsigned int nSize);
#else
	JOVISION_API int __stdcall JVN_SetFrameHead(const int nChannelID,const unsigned char *pBuffer,const unsigned int nSize);
#endif

/****************************************************************************
*名称  : JVN_SetNewPosition 
*功能  : 设置设备所在的区域信息，防止串货用的
*参数  : [IN]  pchBuff  位置信息的数据
			[IN]  nLen	 数据大小
*返回值: 1  设置成功（需要在上线后调用）
			0 失败
*其他  : 上线之后才可以调用
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetNewPosition(unsigned char *pchBuff, int nLen,FUNC_RECV_FROM_SERVER call);
#else
	JOVISION_API int __stdcall	JVN_SetNewPosition(unsigned char *pchBuff, int nLen,FUNC_RECV_FROM_SERVER call);
#endif
	
/****************************************************************************
*名称  : JVN_EnableYST 
*功能  : 禁止或允许云视通上线
*参数  : [IN]  nEnable  > 0 可以上线 0 禁止上线
*返回值: 1  设置成功（需要在上线后调用）
			0 失败
*其他  : 上线之后才可以调用
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_EnableYST(int nEnable);
#else
	JOVISION_API int __stdcall	JVN_EnableYST(int nEnable);
#endif

/****************************************************************************
*名称  : JVN_ClearClientBuff 
*功能  : 清除独立码流的缓存，只可以清除独立码流的缓存，建议在主控收到连接是发送O帧数据前清除,确保先发送O帧再发送新的I帧
*参数  : [IN]  nClientID  分控ID
*返回值: 1  清理成功
			0 失败
*其他  : 必须是启动了独立码流的连接才可以
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_ClearClientBuff(int nClientID);
#else
	JOVISION_API int __stdcall	JVN_ClearClientBuff(int nClientID);
#endif

/****************************************************************************
*名称  : JVN_RegisterCheckFileCallBack 
*功能  : 注册远程检索回调，可以回调出文件长度
*参数  : [IN]  CheckFileCallBack  回调函数
*返回值: 无
*其他  : 
*****************************************************************************/
#ifndef WIN32
	JOVISION_API void JVN_RegisterCheckFileCallBack(FUNC_SCHECKFILENEW_CALLBACK CheckFileCallBack);
#else
	JOVISION_API void __stdcall	JVN_RegisterCheckFileCallBack(FUNC_SCHECKFILENEW_CALLBACK CheckFileCallBack);
#endif

/****************************************************************************
*名称  : JVN_SetViewTimeFrame 
*功能  :  设置是否启用时间戳
*参数  : [IN]  nVideoTime： 0 不启动音视频传输时间戳 1： 启动音视频传输时间戳， 默认不启动
         [IN]  nRemoePlay:  0 不启动远程回放时间戳 1： 启动远程回放时间戳  默认不启动
*返回值: 0: 失败，1 成功
*其他  : 
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetViewTimeFrame(char nVideoTime,char nRemoePlay);
#else
	JOVISION_API int __stdcall JVN_SetViewTimeFrame(char nVideoTime,char nRemoePlay);
#endif

/****************************************************************************
*名称  : JVN_EnableUPNP
*功能  : 设置是否开启upnp功能
*参数  : [IN]  bEnable  TRUE:打开upnp；FALSE:关闭upnp
*返回值:  0: 失败，1 成功
*其他  : 网络库默认是打开了upnp，如果想关闭upnp，请调用JVN_EnableUPNP(FALSE);
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_EnableUpnp(int nEnable);
#else
	JOVISION_API int __stdcall JVN_EnableUpnp(int nEnable);
#endif

/****************************************************************************
*名称  : JVN_SetConUserData
*功能  : 设置链接时用户数据。该数据会在链接成功后传给分控端
*参数  : [IN]  pUserData 用户数据 
       : [IN]  len       数据长度
*返回值:  0: 失败，1 成功
*其他  : 接口需在调用完初始化接口之后调，用户可以根据这个数据做兼容
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetConUserData(const char* pUserData,const int len);
#else
	JOVISION_API int __stdcall JVN_SetConUserData(const char* pUserData,const int len);
#endif

/****************************************************************************
*名称  : JVN_SetConMemLimit
*功能  : 设置主控剩余内存对连接的限制
*参数  : [IN]  nActualLimit  内存(free+cache)小于此值后，禁止实际连接，单位MB
         [IN]  nVirtualLimit 内存(free+cache)小于此值后，禁止虚拟(助手)连接 单位MB
*返回值: 1  成功
		 0 失败
*其他  : 根据设备内存的实际使用情况传入参数，在初始化之后调用
		如果不调用此接口，nActualLimit默认为3MB，nVirtualLimit默认为6MB
*****************************************************************************/
#ifndef WIN32
	JOVISION_API int JVN_SetConMemLimit(int nActualLimit, int nVirtualLimit);
#else
	JOVISION_API int __stdcall	JVN_SetConMemLimit(int nActualLimit, int nVirtualLimit);
#endif

/****************************************************************************
*名称  : JVN_SetWatchSocket
*功能  : 设置要检测的套接字
*参数  : [IN]  nSocketFd 传入套接字
*返回值:  0: 失败，1 成功
*其他  : 请在创建完此套接字后调用
*****************************************************************************/
//#ifndef WIN32
//	JOVISION_API int JVN_SetWatchSocket(const int nSocketFd);
//#else
//	JOVISION_API int __stdcall JVN_SetWatchSocket(const int nSocketFd);
//#endif

#endif
