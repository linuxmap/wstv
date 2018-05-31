
#include "jv_common.h"
#include "jv_wdt.h"
#include  "utl_cmd.h"
static HWDT iDog = 0;
/**
 * @brief 增加一个手动关闭看门狗的功能
 * @param argc
 * @param argv
 * @return
 */
static int  ManualCloseWatchDog(int argc, char *argv[])
{
	if(iDog>0)
		jv_close_wdt(iDog);
	return 0;
}
/**
 *@brief 打开看门狗
 *@note 驱动必须先安装
 *@return 成功，返回打开的设备句柄；失败返回 -1
 *
 */
HWDT OpenWatchDog()
{

	iDog = jv_open_wdt();
	if(iDog>0)
		utl_cmd_insert("wdtoff","Manual close watchdog","exp: wdtoff",ManualCloseWatchDog);
	return iDog;
}

/**
 *@brief 关闭看门狗
 *@param S32 iDog 已打开的设备句柄
 */
void CloseWatchDog()
{
	if(iDog>0)
	{
		jv_close_wdt(iDog);
		iDog = -1;
	}
	return;
}

/**
 *@brief 喂狗
 *@param S32 iDog 已打开的设备句柄
 */
void FeedWatchDog()
{
	if(iDog>0)
		jv_feed_wdt(iDog);
	return;
}

