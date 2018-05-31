

#ifndef __MFIRMUP_H__
#define __MFIRMUP_H__

//升级结果定义
enum
{
	FIRMUP_SUCCESS			= 1,		// 升级成功
	FIRMUP_DOWNLOAD_ERROR,				// 下载固件失败
	FIRMUP_LATEST_VERSION,				// 已是最新版本
	FIRMUP_INVALID_FILE,				// 固件校验失败
	FIRMUP_OTHER_ERROR,					// 其他错误（升级初始化失败、烧写失败等）
	FIRMUP_PRODUCT_ERROR,				// 产品型号不匹配
	FIRMUP_CANNOT_DEMOTION,				// 不支持降级处理（ISSI Flash，目前未用到）
};

//升级方法
enum
{
	FIRMUP_HTTP,
	FIRMUP_FILE,
	FIRMUP_FTP,
};

enum
{
	FIRMUP_LOCATION_NONE	= -1,		// 获取失败
	FIRMUP_LOCATION_MEM,				// 保存在系统内存中（/tmp）
	FIRMUP_LOCATION_MEDIA_MEM,			// 保存在媒体内存中（/tmp/ramdisk，需要加载ramdisk驱动）
	FIRMUP_LOCATION_CARD,				// 保存在TF卡中
	FIRMUP_LOCATION_EMMC,				// 保存在EMMC中
};

// 配置参数，从JOVISION_CONFIG_FILE文件中读取
typedef struct
{
	char product[32];		//产品类别
	char url[4][256];		//升级服务器地址
	char vername[64];		//升级版本文件名
	char binname[64];		//升级文件名
}FirmupCfg_t;

// ver文件中的模块信息
typedef struct
{
	char name[32];//模块名称：boot,kernel,fs,config
	int ver;//版本号
	unsigned int offset;//该模块在文件中的偏移地址
	unsigned int size;//该模块的长度
	char dev[32];//该模块要烧到的设备
	BOOL needUpdate;
}FirmModule_t;

// ver文件信息
typedef struct
{
	char product[32];		//产品类别
	FirmModule_t list[20];	//模块列表
	int cnt;				//模块数量
	unsigned int checksum;
	unsigned int fileSize;
}FirmVersion_t;

enum
{
	FIRMUP_DOWNLOAD_START,
	FIRMUP_DOWNLOAD_PROGRESS,
	FIRMUP_DOWNLOAD_FINISH,
	FIRMUP_DOWNLOAD_TIMEOUT,
	FIRMUP_DOWNLOAD_FAILED,
	FIRMUP_UPDATE_START,		// 文件检查完成，开始烧写，param1为总共需要烧写的block数量
	FIRMUP_UPDATE_FINISH,		// 烧写完成，param1为已烧写块数量，param2为总块数量
	FIRMUP_UPDATE_FAILED,		// 升级失败，param1为失败原因，取值为FIRMUP_FAILED等
};

// 下载超时
#define FIRMUP_VER_TIMEOUT		10
#define FIRMUP_BIN_TIMEOUT		120

typedef void (*FirmupStream_Callback)(void* arg, char* data, int len);
typedef void (*FirmupEvent_Callback)(int nEvent, void* arg, int param1, int param2);


int mfirmup_init(FirmupStream_Callback cb, void* arg);

const FirmupCfg_t* mfirmup_getconfig();

const FirmVersion_t* mfirmup_getnowver();

// 占有升级程序
int mfirmup_claim(int nType);

// 是否在升级中
BOOL mfirmup_b_updating();

// 释放升级程序
VOID mfirmup_release();

// 文件下载，将url/urlfile保存到savepath/savefile中，filename可以为NULL
int mfirmup_download_file(const char* url, const char* urlfile, const char* savepath, const char* savefile, int timeout_sec, BOOL bBlock);

// 分析并检查版本文件
// 返回是否需要升级，TRUE需要升级，FALSE无需升级
BOOL mfirmup_check_verfile(const char* verfile, FirmVersion_t* verinfo);

// 获取并准备bin文件下载位置
// 返回下载位置类型FIRMUP_LOCATION_XXX，FIRMUP_LOCATION_NONE为获取失败
int mfirmup_prepare_location(char* path, int maxlen);

// 自动下载url/ver.bin、url/bin到默认路径，并自动升级（可选），适用于http/ftp等方式
// 下载ver.bin为阻塞方式，下载后会判断是否需要升级
// bin文件下载为非阻塞方式，开始下载后立即返回
// Timeout为bin文件下载超时时间
// autoupdate非0时，下载完成自动开始烧写
// Callback为事件回调函数，事件定义为FIRMUP_DOWNLOAD_XXX/FIRMUP_UPDATE_XXX
int mfirmup_auto_update(const char* url, char (*verpath)[256], char (*binpath)[256], FirmVersion_t* verinfo, 
							int timeout, BOOL autoupdate, FirmupEvent_Callback Callback, void* arg);

// 以binfile和verfile为升级文件开始升级
// Callback为事件回调函数，事件定义为FIRMUP_UPDATE_XXX
int mfirmup_startupdate(const char* binfile, const char* verfile, BOOL bBlock, 
							FirmupEvent_Callback Callback, void* arg);

// 取消升级，只能在下载过程中取消，开始烧写后无法取消
int mfirmup_cancelupdate(const char* verfile, const char* binfile);

// 获取烧写进度，返回0~100
int mfirmup_get_writepercent(int* blocks, int* writed);

#endif

