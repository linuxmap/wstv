#ifndef _SCTRL_H_
#define _SCTRL_H_

#include <time.h>
#include <sys/time.h>
#include "jv_common.h"
#include "JvCDefines.h"

#define SUCCESS				0
#define FAILURE				(-1)

#define VER_IPCAMCFG		 "/progs/res/IPCamCfg.ver"

#define	MAX_CLIENT			32
#define MAX_CLIENT_HOME		6

#define CONFIG_FILE			CONFIG_PATH"SystemCfg.ini"
#define LOG_FILE			CONFIG_PATH"ipcam.log"
#define GB28181_CFG_FILE 	CONFIG_PATH"gb28181.cfg"
#define TUTK_FILE			CONFIG_PATH"tutk.cfg"

//日期显示格式,zwq20100506
#define TF_MMDDYYYY			0		//05/07/2010
#define TF_YYYYMMDD			1		//2010-05-07
#define TF_DDMMYYYY			2		//07/05/2010

//退出代码,lck20120329
#define EXIT_DEFAULT		0
#define EXIT_SHUTDOWN		1
#define EXIT_REBOOT			2
#define EXIT_RECOVERY		3
#define EXIT_FIRMUP			4
#define EXIT_KILL			5
#define EXIT_SOFT_RECOVERY	6 //软复位。将保留网络设置相关的内容


typedef enum 
{
	MOBILE_QUALITY_HIGH = 0x01,		// 高清
	MOBILE_QUALITY_MIDDLE = 0x02,	// 标清
	MOBILE_QUALITY_LOW = 0x03,		// 流畅
}MobileQuality_e;

typedef struct
{
	//手机监控配置
	MobileQuality_e nPictureType;		// 适配新版手机软件,默认标清
	
}SCTRLINFO;

typedef struct
{
	U32 width;
	U32 height;
	U32 framerate;
	int vencType;
	int aencType;
	int audioEn;
}SctrlStreamInfo;


void SctrlSetParam(SCTRLINFO *info);

SCTRLINFO *SctrlGetParam(SCTRLINFO *info);

U32 SctrlGetMOChannel();

void SctrlMakeHeader(U32 nChannel, JVS_FILE_HEADER_EX *exHeader);

int webserver_flush();

char *SctrlGetKeyValue(const char *pBuffer, const char *key, char *valueBuf, int maxValueBuf);

int SctrlGetStreamInfo(S32 nChannel, SctrlStreamInfo* Info);

BOOL SctrlCheckStreamInfoChanged(const SctrlStreamInfo* a, const SctrlStreamInfo* b);

#endif

