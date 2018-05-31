#include <jv_common.h>
#include "sp_firmup.h"
#include "mfirmup.h"
#include "mlog.h"
#include "sctrl.h"
#include "mwdt.h"
#include "SYSFuncs.h"
#include "utl_common.h"

typedef enum{
	UPDATE_FREE,
	UPDATE_DOWNLOAD,
	UPDATE_ERASE,
	UPDATE_WRITE
}UpdateStatus_e;

static UpdateStatus_e updateStatus = UPDATE_FREE;
static int downloadProgress = 0;
static FirmVersion_t	new_version;
static char ver_filename[256] = {0};
static char bin_filename[256] = {0};

int sp_firmup_update_check(char *version)
{
	if (updateStatus != UPDATE_FREE)
	{
		printf("ERROR: already started\n");
		return 0;
	}
	const FirmupCfg_t*	pCfg = mfirmup_getconfig();
	const FirmVersion_t* nowver = mfirmup_getnowver();
	FirmVersion_t	newver;
	char verpath[256] = {0};
	int i = 0;
	int ret = 0;

	updateStatus = UPDATE_DOWNLOAD;

	mfirmup_download_file(pCfg->url[0], pCfg->vername, "/tmp", pCfg->vername, FIRMUP_VER_TIMEOUT, TRUE);

	snprintf(verpath, sizeof(verpath), "/tmp/%s", pCfg->vername);
	ret = mfirmup_check_verfile(verpath, &newver);
	//检查获取版本结果
	if (newver.cnt <= 0 || !ret)
	{
		printf("target.cnt: %d, need update: %d\n", newver.cnt, ret);
		for (i = 0; i < nowver->cnt; ++i)
		{
			if(strcmp(nowver->list[i].name, "appfs") == 0)
			{
				sprintf(version, "%s.%d", MAIN_VERSION, nowver->list[i].ver);
				break;
			}
		}
		updateStatus = UPDATE_FREE;
		return 0;
	}
	for (i = 0;i < newver.cnt; ++i)
	{
		if(strcmp(newver.list[i].name, "appfs") == 0)
		{
			sprintf(version, "%s.%d", MAIN_VERSION, newver.list[i].ver);
		}
	}
	updateStatus = UPDATE_FREE;
	return 1;
}

static void __sp_firmup_event(int nEvent, void* arg, int param1, int param2)
{
	switch (nEvent)
	{
	case FIRMUP_DOWNLOAD_START:
	case FIRMUP_DOWNLOAD_FINISH:
		break;
	case FIRMUP_DOWNLOAD_FAILED:
	case FIRMUP_DOWNLOAD_TIMEOUT:
		updateStatus = UPDATE_FREE;
		downloadProgress = -1;
		break;
	case FIRMUP_UPDATE_START:
		updateStatus = UPDATE_WRITE;
		break;
	case FIRMUP_UPDATE_FINISH:
		updateStatus = UPDATE_FREE;
		downloadProgress = 0;
		break;
	case FIRMUP_UPDATE_FAILED:
		updateStatus = UPDATE_FREE;
		downloadProgress = -1;
		break;
	default:
		printf("%s, unknown event type: %d\n", __func__, nEvent);
		return;
	}
}

int sp_firmup_update(const char *method, const char *url)
{
	if (updateStatus != UPDATE_FREE)
	{
		printf("ERROR: already started\n");
		return -1;
	}

	// 只处理http情况
	if(strcmp(method, "http") != 0)
	{
		return 0;
	}

	if(mfirmup_claim(FIRMUP_HTTP) != SUCCESS)
	{
		updateStatus = UPDATE_FREE;
		return 0;
	}

	const FirmupCfg_t*	pCfg = mfirmup_getconfig();
	const FirmVersion_t* nowver = mfirmup_getnowver();
	int ret;

	updateStatus = UPDATE_DOWNLOAD;
	ret = mfirmup_auto_update(pCfg->url[0], &ver_filename, &bin_filename, &new_version, 
									60, TRUE, __sp_firmup_event, NULL);
	if (ret != FIRMUP_SUCCESS)
	{
		goto update_failed;
	}

	return 0;

update_failed:
	mfirmup_release();
	if (ret == FIRMUP_LATEST_VERSION)
	{
		printf("Latest version, exit...");
		mlog_write("已是最新版本");
		return -2;
	}
	if (ret == FIRMUP_PRODUCT_ERROR)
	{
		printf("Product check error, exit...");
		mlog_write("升级文件错误");
		return -3;
	}
	if (ret == FIRMUP_DOWNLOAD_FAILED)
	{
		printf("Download update file failed, exit...");
		mlog_write("下载文件失败");
		return -3;
	}

	printf("update failed with %d, exit...", ret);
	mlog_write("升级失败");
	return -3;
}

int sp_firmup_update_get_progress(char *phase, int *progress)
{
	if(updateStatus == UPDATE_FREE)
	{
		*progress = 100;
		strcpy(phase, "free");
	}
	else
	{
		if(updateStatus == UPDATE_DOWNLOAD)
		{
			strcpy(phase, "download");
			*progress = utl_get_file_size(bin_filename) * 100 / ((new_version.fileSize == 0) ? 0x800000 : new_version.fileSize);
		}
		else if(updateStatus == UPDATE_ERASE)
		{
			strcpy(phase, "erase");
			*progress = 100;
		}
		else if(updateStatus == UPDATE_WRITE)
		{
			strcpy(phase, "write");
			if(downloadProgress != -1)
				*progress = mfirmup_get_writepercent(NULL, NULL);
			else
				*progress = downloadProgress;
		}
	}

	return 0;
}


