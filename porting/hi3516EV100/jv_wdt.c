
#include "jv_common.h"
#include "jv_wdt.h"
#include "watchdog.h"

/**
*@brief 软看门狗，为了补充没有硬看门狗,lck20120616
*
*/
static PJDOG pDog;

/**
 *@brief 打开看门狗
 *@note 驱动必须先安装
 *@return 成功，返回打开的设备句柄；失败返回 -1
 *
 */
HWDT jv_open_wdt()
{
	//初始化软看门狗
	S32 segment_id = shmget(0x80040001, sizeof(JDOG), S_IRUSR|S_IWUSR);
	if (segment_id != -1)
	{
		pDog = shmat(segment_id, 0, 0);
		if (pDog)
		{
			pDog->nInitOK = TRUE;
		}
	}
	else
	{
		Printf("No WatchDog Found!\n");
	}

	//初始化硬看门狗
	HWDT iDog = 0;
	iDog = open("/dev/watchdog", O_RDONLY);
	if (iDog == -1) 
	{
		static BOOL haveWDT = TRUE;
		if (haveWDT)
		{
			if (access("/home/ipc_drv/extdrv/wdt.ko", F_OK) == 0)
			{
				utl_system("insmod /home/ipc_drv/extdrv/wdt.ko default_margin=60 nodeamon=1");
				//这里不用再处理，下次再来，自然就打开了
			}
			int i;
			for (i=0;i<5;i++)
			{
				//有不成功的情况，多试几次
				iDog = open("/dev/watchdog", O_RDONLY);
				if (iDog != -1)
				{
					break;
				}
				Printf("Failed one time, Retry...\n");
				usleep(300*1000);
			}
			if (iDog == -1)
			{
				haveWDT = FALSE;
				perror("No KO, fail to open wdt!  /home/ipc_drv/extdrv/wdt.ko");
				return -1;
			}
		}
		else
		{
			perror("fail to open wdt!");
			return -1;
		}
	}

	S32 nOption = 30;
	ioctl(iDog, WDIOC_SETTIMEOUT, &nOption);
	ioctl(iDog, WDIOC_GETTIMEOUT, &nOption);
	Printf("Watchdog=%d enabled, timeout=%d\n", iDog, nOption);

	nOption = WDIOS_ENABLECARD;
	ioctl(iDog, WDIOC_SETOPTIONS, &nOption);

	return iDog;
}

/**
 *@brief 关闭看门狗
 *@param S32 iDog 已打开的设备句柄
 */
void jv_close_wdt(HWDT iDog)
{
	if (pDog)
	{
		pDog->nQuit = TRUE;
		shmdt(pDog);
	}
	if(iDog > 0)
	{
		Printf("--------------------------close watchdog \n");
		S32 nOption = WDIOS_DISABLECARD;
		ioctl(iDog, WDIOC_SETOPTIONS, &nOption);
		close(iDog);
	}
	return;
}

/**
 *@brief 喂狗
 *@param S32 iDog 已打开的设备句柄
 */
void jv_feed_wdt(HWDT iDog)
{
	//Printf("--------------------------feeding watchdog \n");
	if (pDog)
	{
		pDog->nCount = 0;
	}
	if (iDog > 0)
	{
		//Printf("feed watch dog...\n");
		ioctl(iDog, WDIOC_KEEPALIVE, NULL);
	}
	return;
}

