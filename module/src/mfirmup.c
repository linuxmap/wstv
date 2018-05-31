/**
 *@file 负责设备固件升级
 */

#include "jv_common.h"

#include "mfirmup.h"
#include "mlog.h"
#include <utl_filecfg.h>
#include <utl_ifconfig.h>
#include "mosd.h"
#include "mstream.h"
#include "mdetect.h"
#include "SYSFuncs.h"
#include "jv_spi_flash.h"
#include "mwdt.h"
#include "utl_system.h"
#include "utl_common.h"
#include "mstorage.h"


#ifdef FIRMUP_BY_MEDIAMEM
#define FIRMUP_RAMDISK_PATH		"/tmp/ramdisk"
#endif
#define FIRMUP_FILELEN			(8*1024*1024)		// 默认升级文件大小8M
#define BLOCK_BUF_SIZE			(1024*64)			// 每次写入的数据块大小64K


typedef struct
{
	U32	nStatus;	//0:空闲，1:使用中
	U32	nType;		//0:文件升级，
	U32	nBlocks;
	U32	nWrited;
	FirmupStream_Callback	Callback;
	void*	arg;
}FirmupState_t;

typedef struct
{
	BOOL	busy;
	char	url[512];
	char	verfile[256];
	char	binfile[256];
	int		binsize;
	int		timeout;
	BOOL	autoupdate;
	FirmupEvent_Callback	Callback;
	void*	arg;
}FirmupContext_t;

static FirmupCfg_t		s_FirmupCfg;		// JOVISION_CONFIG_FILE中存储的信息

static FirmupState_t	s_FirmupState;		// 升级状态
static FirmVersion_t	s_FirmupCurver;		// CONFIG_PATH中的当前版本信息
static FirmupContext_t	s_FirmupCtx;		// Firmup上下文信息


enum
{
	MEDIA_MEM_ERR_INIT		= -1,
	MEDIA_MEM_ERR_CANCEL	= -2,
	MEDIA_MEM_ERR_DOWNLOAD	= -3,
	MEDIA_MEM_ERR_OTHER		= -4,
};

#ifdef FIRMUP_BY_MEDIAMEM
//保留一帧图像，供文件升级时向分控持续发送
//避免文件升级时，因关闭了媒体业务
//导致分控收不到视频数据而超时自动关闭
#define SAVE_FRAME_CHANNEL	1			// 保存次码流数据，节省内存、带宽
#define MAX_I_FRAME_SIZE	64*1024

static struct
{
	char	videodata[MAX_I_FRAME_SIZE];
	int		videosize;
}s_FirmSavedIFrame;

static void _mfirmup_save_frame(int channelid, void *data, unsigned int size, jv_frame_type_e type, unsigned long long timestamp)
{
	if(channelid != SAVE_FRAME_CHANNEL
		|| type != JV_FRAME_TYPE_I
		|| size > MAX_I_FRAME_SIZE)
	{
		return;
	}

	s_FirmSavedIFrame.videosize = size;
	memcpy(s_FirmSavedIFrame.videodata, data, size);	
}

//临时向分控发送固定视频数据,防止JNVR收不到数据关闭连接
static void _mfirmup_sendframe()
{
	if (s_FirmupState.Callback)
	{
		s_FirmupState.Callback(s_FirmupState.arg, s_FirmSavedIFrame.videodata, s_FirmSavedIFrame.videosize);
	}
}

// 向客户端重复发送最后一个I帧，保证视频不停，连接不断
static VOID* _mfirmup_keepstream(VOID *pArgs)
{
#define INTERVAL_MS 500

	// int finish_time = 0;
	BOOL bUpdateFailed = FALSE;
	int retry_times = 0;

	pthreadinfo_add((char *)__func__);

	while(1)
	{
		if ((s_FirmupState.nBlocks > 0) && (s_FirmupState.nWrited == s_FirmupState.nBlocks))
		{
			// 升级完成后会自动重启
			#if 0
			finish_time += INTERVAL_MS;
			if(finish_time >= 5000)
			{
				SYSFuncs_reboot();
				break;
			}
			#endif
		}
		else if ((S32)s_FirmupState.nWrited < 0)		// 升级失败
		{
			/*printf("########reboot by %s(%d)!\n", (s_FirmupState.nWrited == MEDIA_MEM_ERR_INIT) ? "mount ramdisk failed" : (s_FirmupState.nWrited == MEDIA_MEM_ERR_CANCEL) ? "user cancel" : "failed in thread", s_FirmupState.nWrited);*/
			sleep(5);
			bUpdateFailed = TRUE;
		}
		else if (s_FirmupState.nWrited == 0 && s_FirmupCtx.binfile[0])			// 还在下载
		{
			int binsize = utl_get_file_size(s_FirmupCtx.binfile);

			if (binsize > 0 && binsize < s_FirmupCtx.binsize)
			{
				int ret = utl_system("pidof wget > /dev/null");

				// 命令已执行，但执行结果为失败，即没有wget进程
				// 此时wget已超时退出，但没有下载完，认为超时
				if (ret != -1 && WIFEXITED(ret) != 0 && WEXITSTATUS(ret) != 0)
				{
					// 这里再判断一次，避免再pidof wget时，wget正好下载完成退出
					binsize = utl_get_file_size(s_FirmupCtx.binfile);
					if (binsize > 0 && binsize < s_FirmupCtx.binsize)
					{
						bUpdateFailed = TRUE;
					}
				}
			}
		}

		if (bUpdateFailed)
		{
			mlog_write("Update Failed, Now reboot...");
			SYSFuncs_reboot();
			break;
		}

		_mfirmup_sendframe();
		usleep(INTERVAL_MS * 1000);
	}

	return NULL;
}

// 停止业务，释放内存后，将媒体内存挂载为文件系统，用以下载升级文件
// 成功返回0，失败返回-1
static int __mfirmup_prepare_ramdisk()
{
	int ret;

	if (access(FIRMUP_RAMDISK_PATH, F_OK) == 0)
	{
		return 0;
	}

	mstream_request_idr(1);
	// usleep(200 * 1000);
    mchnosd_deinit();
	mdetect_deinit();
	mstream_deinit();
	jv_common_deinit_platform();	// 退出媒体业务平台，释放媒体内存

	pthread_create_detached(NULL, NULL, (void *)_mfirmup_keepstream, NULL);

	utl_system("insmod /home/ipc_drv/extdrv/ramdisk.ko");
	utl_WaitTimeout(!access("/dev/ramdisk", F_OK), 1000);
	ret = utl_system("mkdosfs /dev/ramdisk && mkdir -p %s && mount /dev/ramdisk %s", FIRMUP_RAMDISK_PATH, FIRMUP_RAMDISK_PATH);

	return IsSystemFail(ret) ? -1 : 0;
}
#endif

/*
文件内容：
product=JVS-HI3518ESIV200-7601
firmup-url=http://updatewt.afdvr.com/homeipc/3518es/HC301
firmup-url=http://updatedx.afdvr.com/homeipc/3518es/HC301
firmup-url=http://updatehw.afdvr.com/homeipc/3518es/HC301
firmup-binName=jvs3518esiv200-7601.bin
firmup-verName=jvs3518esiv200-7601-ver.bin
*/
/**
 *@从配置文件中初始化升级所需的参数
 */
static void _mfirmup_parse_config(FirmupCfg_t *cfg)
{
	keyvalue_t *kv;
	int cur = 0;
	int cnt = 0;

	memset(cfg, 0, sizeof(*cfg));
	utl_fcfg_start_getnext(JOVISION_CONFIG_FILE);

	while(1)
	{
		kv = utl_fcfg_get_next(JOVISION_CONFIG_FILE, &cur);
		if (kv == NULL)
			break;

		Printf("%s=%s\n", kv->key, kv->value);
		if (strcmp("firmup-url", kv->key) == 0)
		{
			if (cnt < 4)
			{
				strcpy(cfg->url[cnt++], kv->value);
			}
		}
		else if (strcmp("firmup-binName", kv->key) == 0)
		{
			strcpy(cfg->binname, kv->value);
		}
		else if (strcmp("firmup-verName", kv->key) == 0)
		{
			strcpy(cfg->vername, kv->value);
		}
		else if (strcmp("product", kv->key) == 0)
		{
			strcpy(cfg->product, kv->value);
		}
	}
	utl_fcfg_end_getnext(JOVISION_CONFIG_FILE);
}

/**
 *@brief 读取版本文件内容
#####################3
#file format:
#module=name, the name want to update. such as boot,kernel,fs,config ...
#ver=3, version of the module
#offset=0, offset in the file
#size=0x100000, size in byte
#dev=/dev/mtdblock/0, dev used to update
#
  
module=boot
ver=3
offset=0
size=0x100000
dev=/dev/mtdblock/0

module=kernel
ver=4
offset=0x100000
size=0x100000
dev=/dev/mtdblock/1

module=fs
ver=91
offset=0x200000
size=0x500000
dev=/dev/mtdblock/2

checksum=0x4d8c89b9
* 
 *
 */
static int _mfirmup_parse_verfile(const char *fname, FirmVersion_t *ver)
{
	keyvalue_t *kv;
	int cur = 0;

	memset(ver, 0, sizeof(*ver));
	ver->cnt = -1;

	ver->fileSize = FIRMUP_FILELEN;
	utl_fcfg_start_getnext(fname);

	while(1)
	{
		kv = utl_fcfg_get_next(fname, &cur);
		if (kv == NULL)
			break;
		Printf("%s=%s\n", kv->key, kv->value);
		if (strcmp("module", kv->key) == 0)
		{
			ver->cnt++;
			if (ver->cnt >= sizeof(ver->list) / sizeof(FirmModule_t))
			{
				Printf("ERROR: too many modules.\n");
				break;
			}
			strncpy(ver->list[ver->cnt].name, kv->value, sizeof(ver->list[ver->cnt].name));
		}
		else if (strcmp("checksum", kv->key) == 0)
		{
			ver->checksum = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("fileSize", kv->key) == 0)
		{
			ver->fileSize = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("product", kv->key) == 0)
		{
			strncpy(ver->product, kv->value, sizeof(ver->product));
		}
		if (ver->cnt == -1)
			continue;
		//ver=3offset=0size=0x100000dev=/dev/mtdblock/0

		if (strcmp("ver", kv->key) == 0)
		{
			ver->list[ver->cnt].ver = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("offset", kv->key) == 0)
		{
			ver->list[ver->cnt].offset = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("size", kv->key) == 0)
		{
			ver->list[ver->cnt].size = strtoul(kv->value, NULL, 0);
		}
		else if (strcmp("dev", kv->key) == 0)
		{
			strncpy(ver->list[ver->cnt].dev, kv->value, sizeof(ver->list[ver->cnt].dev));
		}
	}
	utl_fcfg_end_getnext(fname);
	ver->cnt++;
	utl_fcfg_close(fname);

	{
		int i;
		for (i = 0; i < ver->cnt; ++i)
		{
			Printf("module: %s, ver: %d, offset: 0x%x, size: 0x%x, dev: %s\n", 
				ver->list[i].name, ver->list[i].ver, ver->list[i].offset, ver->list[i].size, ver->list[i].dev);
		}
	}
	return 0;
}

static FirmModule_t* _mfirmup_getmodule(FirmVersion_t *firm, char *module)
{
	int i;
	for (i=0;i<firm->cnt;i++)
	{
		if (0 == strcmp(firm->list[i].name, module))
		{
			return &firm->list[i];
		}
	}

	return NULL;
}

static BOOL _mfirmup_need_update(FirmVersion_t *target, FirmVersion_t *mine)
{
	BOOL ret = FALSE;
	FirmModule_t *fm;
	int i;

	s_FirmupState.nBlocks = 0;
	for (i = 0; i < target->cnt; ++i)
	{
		fm = _mfirmup_getmodule(mine, target->list[i].name);
		if (fm == NULL)
		{
			Printf("Failed find module: %s\n", target->list[i].name);
			s_FirmupState.nBlocks += target->list[i].size / BLOCK_BUF_SIZE;
			target->list[i].needUpdate = TRUE;
			ret = TRUE;
		}
		else if (target->list[i].ver != fm->ver)
		{
			Printf("[%s] version diff..., ver: %d != %d, name: %s\n", target->list[i].name, target->list[i].ver, fm->ver, fm->name);
			s_FirmupState.nBlocks += target->list[i].size / BLOCK_BUF_SIZE;
			target->list[i].needUpdate = TRUE;
			ret = TRUE;
		}
	}
	return ret;
}

//检查校验并顺便解密
static int _mfirmup_checkbinfile(const char* binfile, U32 *nChecksum)
{
    U32 i, j;
    U32 pBuf[1024] = {0};
	int arrsize = ARRAY_SIZE(pBuf);
	int filesize = utl_get_file_size(binfile);
    FILE *fp;

    *nChecksum = 0;
    fp = fopen(binfile, "rb+");
    if(NULL == fp)
    {
        return -1;
    }

    for (i = 0; i < (filesize / sizeof(pBuf)); i++)
    {
        fread(pBuf, 4, arrsize, fp);
        for (j = 0; j < arrsize; j++)
        {
            *nChecksum += pBuf[j];
            pBuf[j] = pBuf[j] ^ 0x1f441f44;
        }
        fseek(fp, -sizeof(pBuf), SEEK_CUR);
        fwrite(pBuf, 4, arrsize, fp);
    }

    fclose(fp);
    return 0;
}

//说明   : 写flash分区
//参数   : fd:文件描述符
//         dev:要写入的块设备
//         pBuf:缓冲区
//         nBufSize:缓存区大小
//         nStart:当前分区的第几块开始烧写
//         nCount:烧写块数
//返回值 : 无
static S32 _mfirmup_writepartition(S32 fd, char *dev, U8 *pBuf, S32 nBufSize, S32 nStart, S32 nCount)
{
	S32 dev_fd;
	char osdstr[32];
	S32 ret;
	S32 i;

	Printf("Write param:nBufSize=%d, nStart=%d, nCount=%d\n", nBufSize, nStart, nCount);

	dev_fd = open (dev, O_SYNC | O_WRONLY);
	if(dev_fd < 0)
	{
	    printf("open dev failed \n");
	    return -1;
	}

	lseek(dev_fd, 0, SEEK_SET);
    //定位
	lseek(fd, nStart * nBufSize, SEEK_SET);
    
	for (i = 0; i < nCount; ++i)
	{
		ret = read(fd, pBuf, nBufSize);
		if (ret != nBufSize)
		{
			if (ret >= 0)		// 返回值>=0，说明是文件读到结尾了
			{
				static BOOL bNeedSet = TRUE;
				if (bNeedSet)
				{
					memset(pBuf + ret, 0xff, nBufSize - ret);	// 补0xff
					if (ret == 0)	// 减少memset次数，提高效率
					{
						bNeedSet = FALSE;
					}
				}
			}
		}
		ret = write(dev_fd ,pBuf, nBufSize) ;
		if (ret != nBufSize)
		{
			printf("write failed: %d != %d, errno: %s\n", ret, nBufSize, strerror(errno));
        	close(dev_fd);
            return -1;
		}

		//累加已经写入成功的块数
		s_FirmupState.nWrited ++;

		sprintf(osdstr, "Written: %d/%d", s_FirmupState.nWrited, s_FirmupState.nBlocks);
		mchnosd_debug_mode(TRUE, osdstr);
		printf("%s\n", osdstr);
	}

	close(dev_fd);
    return 0;
}

//写flash
static S32 _mfirmup_writeflash(char *pFileName, FirmVersion_t *target, FirmVersion_t *mine)
{
	S32 fd = -1;
	U8 buf[BLOCK_BUF_SIZE] = {0};
	int i;
	int ret;
    
    if(NULL == pFileName)
    {
        return -1;
    }
    fd = open(pFileName, O_RDONLY);
    if(fd < 0)
    {
        Printf("open file failed\n");
        return -1;
    }

    jv_flash_write_unlock();	//开始升级，解锁

	for (i = 0; i < target->cnt; ++i)
	{
		if (target->list[i].needUpdate)
		{
			printf("\nNow update: %s\n", target->list[i].name);
			ret = _mfirmup_writepartition(fd, target->list[i].dev, buf, BLOCK_BUF_SIZE, 
											target->list[i].offset / BLOCK_BUF_SIZE, target->list[i].size / BLOCK_BUF_SIZE);
			if (ret)
			{
				close(fd);
				printf("Write Failed...\n");
				return ret;
			}
		}
	}
	s_FirmupState.nWrited = s_FirmupState.nBlocks;
	jv_flash_write_lock();		//升级完毕，重新上锁
	close(fd);
	Printf("Write success\n");
    return 0;
}

//升级线程
static VOID* _mfirmup_update(VOID *arg)
{
	FirmupContext_t* ctx = (FirmupContext_t*)arg;
	char* verfile = ctx->verfile;
	char* binfile = ctx->binfile;
	FirmVersion_t newver;
	int ret;
	U32 checksum = 0;

	pthreadinfo_add((char *)__func__);

	mfirmup_check_verfile(verfile, &newver);

	//检查校验并顺便解密
	ret = _mfirmup_checkbinfile(binfile, &checksum);
	if (0 != ret || (checksum != newver.checksum) )
	{
		printf("Check sum error, now: %#x, should: %#x\n", checksum, newver.checksum);
		ret = FIRMUP_INVALID_FILE;
		mchnosd_debug_mode(TRUE, "Checksum Error.");
		goto update_failed;
	}

	// 判断版本后将进度条范围发送给分控
	Printf("Start firmup, steps:%d...\n", s_FirmupState.nBlocks);
	if (ctx->Callback)
	{
		ctx->Callback(FIRMUP_UPDATE_START, ctx->arg, s_FirmupState.nBlocks, 0);
	}
	mlog_write("Updating... Write Started");

	Printf("Prepare firmup...\n");
	// 烧写flash前清缓存，腾出内存空间，避免烧写过程中内存不足
	utl_system("sync; echo 3 > /proc/sys/vm/drop_caches");

	//烧写flash
	if (_mfirmup_writeflash(binfile, &newver, &s_FirmupCurver))
	{
		ret = FIRMUP_OTHER_ERROR;
		goto update_failed;
	}

	char nowverfile[256] = {0};
	snprintf(nowverfile, sizeof(nowverfile), "%s/%s", CONFIG_PATH, s_FirmupCfg.vername);

	//升级完成，复制新的版本文件
	utl_copy_file(verfile, nowverfile);
	_mfirmup_parse_verfile(nowverfile, &s_FirmupCurver);
	if (ctx->Callback)
	{
		ctx->Callback(FIRMUP_UPDATE_FINISH, ctx->arg, s_FirmupState.nWrited, s_FirmupState.nBlocks);
	}

	mfirmup_release();
	// 升级完成后删除文件，避免重复升级
	utl_system("rm -rf %s %s", binfile, verfile);
	memset(ctx, 0, sizeof(*ctx));

	mlog_write("Updated Success");

	printf("Write flash success. reboot after 3 sec.\n");
	sleep(3);
	SYSFuncs_reboot();

	return NULL;

update_failed:
	//向分控发送升级结果
	Printf("Send tpacket to client...\n");
	if (ctx->Callback)
	{
		ctx->Callback(FIRMUP_UPDATE_FAILED, ctx->arg, ret, 0);
	}
	s_FirmupState.nWrited = MEDIA_MEM_ERR_OTHER;
	mfirmup_release();
	// 升级完成后删除文件，避免重复升级
	utl_system("rm -rf %s %s", binfile, verfile);
	memset(ctx, 0, sizeof(*ctx));

	return NULL;
}

static void* _mfirmup_download(void* arg)
{
	FirmupContext_t* ctx = (FirmupContext_t*)arg;
	char* url = ctx->url;
	char* binfile = ctx->binfile;
	char* verfile = ctx->verfile;
	BOOL autoupdate = ctx->autoupdate;
	int binsize = ctx->binsize;
	int timeout = ctx->timeout;
	int ret;

	pthreadinfo_add((char *)__func__);

	printf("Bin file size: %#x(%d) bytes\n", binsize, binsize);

	if (ctx->Callback)
	{
		ctx->Callback(FIRMUP_DOWNLOAD_START, ctx->arg, 0, 0);
	}

	while (--timeout >= 0)
	{
		ret = utl_get_file_size(binfile);
		if (ret >= binsize)
		{
			break;
		}
		if (ctx->Callback)
		{
			ctx->Callback(FIRMUP_DOWNLOAD_PROGRESS, ctx->arg, ret, binsize);
		}
		sleep(1);
	}

	if (utl_get_file_size(binfile) < binsize)
	{
		if (ctx->Callback)
		{
			ctx->Callback(FIRMUP_DOWNLOAD_TIMEOUT, ctx->arg, FIRMUP_DOWNLOAD_ERROR, 0);
		}
		goto download_failed_end;
	}
	else
	{
		if (ctx->Callback)
		{
			ctx->Callback(FIRMUP_DOWNLOAD_FINISH, ctx->arg, ret, binsize);
		}

		printf("%s, download finished, auto update: %d\n", __func__, autoupdate);
		// 下载完成后自动升级
		if (autoupdate)
		{
			_mfirmup_update(ctx);
			// _mfirmup_update函数中有完整逻辑，直接return即可
			return NULL;
		}
	}

	memset(ctx, 0, sizeof(*ctx));
	return NULL;

download_failed:
	if (ctx->Callback)
	{
		ctx->Callback(FIRMUP_DOWNLOAD_FAILED, ctx->arg, ret, 0);
	}
download_failed_end:
	mfirmup_release();
	utl_system("rm -rf %s", binfile, verfile);
	s_FirmupState.nWrited = MEDIA_MEM_ERR_DOWNLOAD;
	memset(ctx, 0, sizeof(*ctx));
	return NULL;
}

int mfirmup_init(FirmupStream_Callback cb, void* arg)
{
	char verfile[256] = {0};

	_mfirmup_parse_config(&s_FirmupCfg);

	snprintf(verfile, sizeof(verfile), "%s/%s", CONFIG_PATH, s_FirmupCfg.vername);
	_mfirmup_parse_verfile(verfile, &s_FirmupCurver);		// 不需要检查文件是否存在，不存在时，会清空curver

#ifdef FIRMUP_BY_MEDIAMEM
	mstream_set_transmit_callback(_mfirmup_save_frame);
#endif

	s_FirmupState.Callback = cb;
	s_FirmupState.arg = arg;

	return 0;
}

const FirmupCfg_t* mfirmup_getconfig()
{
	return &s_FirmupCfg;
}

const FirmVersion_t* mfirmup_getnowver()
{
	return &s_FirmupCurver;
}

int mfirmup_claim(int nType)
{
	if(1 == s_FirmupState.nStatus) return FAILURE;	//如果正在升级则直接返回失败

	s_FirmupState.nStatus = 1;
	s_FirmupState.nType = nType;
	s_FirmupState.nBlocks = 0;
	s_FirmupState.nWrited = 0;

	// 升级前清空缓存
	utl_system("sync; echo 3 > /proc/sys/vm/drop_caches");

	return SUCCESS;
}

VOID mfirmup_release()
{
	s_FirmupState.nStatus = 0;
}

//是否正在升级
BOOL mfirmup_b_updating()
{
	return s_FirmupState.nStatus;
}

int mfirmup_download_file(const char* url, const char* urlfile, const char* savepath, const char* savefile, int timeout_sec, BOOL bBlock)
{
	int ret = -1;

	if (!url || !url[0])
	{
		return ret;
	}
	if (!savepath || !savefile || !savefile[0])
	{
		return ret;
	}

	utl_system("killall wget; rm -f %s/%s", savepath, savefile);

	ret = utl_system("wget -T %d -O %s/%s %s%s%s %c", timeout_sec, savepath, savefile, 
						url, (urlfile && urlfile[0]) ? "/" : "", urlfile ? urlfile : "", 
						bBlock ? ' ' : '&');

	return IsSystemFail(ret) ? -1 : 0;
}

BOOL mfirmup_check_verfile(const char* verfile, FirmVersion_t* verinfo)
{
	FirmVersion_t ver;
	int ret;

	// 为了调试、测试时删除版本文件后立即升级，这里再检测一次当前版本
	char curverfile[256] = "";
	snprintf(curverfile, sizeof(curverfile), "%s/%s", CONFIG_PATH, s_FirmupCfg.vername);
	if (access(curverfile, F_OK) != 0)		// 文件不存在时更新一下
	{
		_mfirmup_parse_verfile(curverfile, &s_FirmupCurver);
	}

	_mfirmup_parse_verfile(verfile, &ver);

	ret = _mfirmup_need_update(&ver, &s_FirmupCurver);
	if (verinfo)	*verinfo = ver;

	return ret;
}

int mfirmup_prepare_location(char* path, int maxlen)
{
	int location_type;

	if (hwinfo.bEMMC)
	{
		// 获取EMMC存储分区的路径
		mstorage_get_partpath(0, 0, path, maxlen);
		location_type = FIRMUP_LOCATION_EMMC;
		goto prepare_end;
	}

	char sd_path[256] = {0};
	mstorage_get_partpath(1, 0, sd_path, maxlen);
	if (access(sd_path, F_OK) == 0)
	{
		mstorage_allocate_space(200);
		snprintf(path, maxlen, "%s", sd_path);
		location_type = FIRMUP_LOCATION_CARD;
		goto prepare_end;
	}

#ifdef FIRMUP_BY_MEDIAMEM
	int ret = __mfirmup_prepare_ramdisk();
	if (ret == 0)
	{
		snprintf(path, maxlen, "%s", FIRMUP_RAMDISK_PATH);
		location_type = FIRMUP_LOCATION_MEDIA_MEM;
	}
	else
	{
		if (maxlen > 0)		path[0] = '\0';
		location_type = FIRMUP_LOCATION_NONE;
		s_FirmupState.nWrited = MEDIA_MEM_ERR_INIT;
	}
#else
	snprintf(path, maxlen, "%s", "/tmp");
	location_type = FIRMUP_LOCATION_MEM;
#endif

prepare_end:
	printf("%s, bin save path: %s\n", __func__, path);
	return location_type;
}

int mfirmup_auto_update(const char* url, char (*verpath)[256], char (*binpath)[256], FirmVersion_t* verinfo, 
							int timeout, BOOL autoupdate, FirmupEvent_Callback Callback, void* arg)
{
	if (s_FirmupCtx.busy)
	{
		printf("Already downloading!!!\n");
		return -1;
	}
	s_FirmupCtx.busy = TRUE;

	char binsavepath[256] = {0};
	int ret;
	FirmVersion_t ver;

	ret = mfirmup_download_file(url, s_FirmupCfg.vername, "/tmp", s_FirmupCfg.vername, FIRMUP_VER_TIMEOUT, TRUE);
	if (ret != 0)
	{
		ret = FIRMUP_DOWNLOAD_ERROR;
		goto download_end;
	}

	snprintf(*verpath, sizeof(*verpath), "/tmp/%s", s_FirmupCfg.vername);
	if (!mfirmup_check_verfile(*verpath, &ver))
	{
		ret = FIRMUP_LATEST_VERSION;
		goto download_end;
	}

	if (strcmp(ver.product, s_FirmupCfg.product) != 0)
	{
		ret = FIRMUP_PRODUCT_ERROR;
		goto download_end;
	}

	ret = mfirmup_prepare_location(binsavepath, sizeof(binsavepath));
	if (ret == FIRMUP_LOCATION_NONE)
	{
		ret = FIRMUP_OTHER_ERROR;
		goto download_end;
	}

	ret = mfirmup_download_file(url, s_FirmupCfg.binname, binsavepath, s_FirmupCfg.binname, timeout, FALSE);
	if (ret != 0)
	{
		ret = FIRMUP_DOWNLOAD_ERROR;
		goto download_end;
	}

	snprintf(*binpath, sizeof(*binpath), "%s/%s", binsavepath, s_FirmupCfg.binname);

	strcpy(s_FirmupCtx.binfile, *binpath);
	strcpy(s_FirmupCtx.verfile, *verpath);
	strcpy(s_FirmupCtx.url, url);
	s_FirmupCtx.timeout = timeout;
	s_FirmupCtx.binsize = ver.fileSize;
	s_FirmupCtx.autoupdate = autoupdate;
	s_FirmupCtx.Callback = Callback;
	s_FirmupCtx.arg = arg;
	pthread_create_detached(NULL, NULL, _mfirmup_download, &s_FirmupCtx);

	ret = FIRMUP_SUCCESS;

	if (verinfo)		*verinfo = ver;

download_end:
	s_FirmupCtx.busy = FALSE;
	return ret;
}

int mfirmup_startupdate(const char* verfile, const char* binfile, BOOL bBlock, 
							FirmupEvent_Callback Callback, void* arg)
{
	//测试发现，HTTP升级时，有可能存在一种情况 ，网络较卡的情况下，与客户端交互时，会因为网络卡顿，导致被调用多次。
	//这就导致做了多次解密，从而把文件搞得一段解密过，一段没解密过。无法正常使用。并造成升级失败。
	if (s_FirmupCtx.busy)
	{
		printf("Already updating!!!\n");
		return -1;
	}
	s_FirmupCtx.busy = TRUE;

	//发送给分控的执行结果
	FirmVersion_t ver;
	int size = 0;
	int ret = FIRMUP_SUCCESS;

	if (access(binfile, F_OK) != 0 || access(verfile, F_OK) != 0)
	{
		printf("Bin %s or ver %s not exist!\n", binfile, verfile);
		goto update_failed;
	}

	printf("Start update with %s and %s\n", verfile, binfile);

	_mfirmup_parse_verfile(verfile, &ver);

	//检查获取版本结果
	if (ver.cnt <= 0)
	{
		printf("Firmup error, no version file %s.\n", verfile);
		mchnosd_debug_mode(TRUE, "Parse Version File Error.");
		ret = FIRMUP_DOWNLOAD_ERROR;
		goto update_failed;
	}
	if(!_mfirmup_need_update(&ver, &s_FirmupCurver))
	{
		printf("Latest version.\n");
		mchnosd_debug_mode(TRUE, "Latest version.");
		ret = FIRMUP_LATEST_VERSION;
		goto update_failed;
	}
	//判断文件是否可用
	size = utl_get_file_size(binfile);
	if(ver.fileSize != size)
	{
		printf("Firmware, size error, now=%#x, should=%#x\n", size, ver.fileSize);
		mchnosd_debug_mode(TRUE, "Download File Error.");
		ret = FIRMUP_INVALID_FILE;
		goto update_failed;
	}

	strcpy(s_FirmupCtx.binfile, binfile);
	strcpy(s_FirmupCtx.verfile, verfile);
	s_FirmupCtx.Callback = Callback;
	s_FirmupCtx.arg = arg;

	pthread_t pid;
	pthread_create_normal(&pid, NULL, _mfirmup_update, &s_FirmupCtx);

	if (bBlock)
	{
		pthread_join(pid, NULL);
	}
	else
	{
		pthread_detach(pid);
	}
	return ret;

update_failed:
	s_FirmupCtx.busy = FALSE;
	return ret;
}

int mfirmup_cancelupdate(const char* verfile, const char* binfile)
{
	FirmVersion_t ver;

	if (access(verfile, F_OK) != 0)
	{
		goto cancel_end;
	}

	_mfirmup_parse_verfile(verfile, &ver);
	if(ver.cnt <= 0 || ver.fileSize <= 0)
	{
		goto cancel_end;
	}

	// 保险起见，下载80%以后，不允许再cancel
	// 避免因为时间差问题，cancel时已经开始烧写，导致升级失败无法启动
	if (utl_get_file_size(binfile) * 100 / ver.fileSize <= 80)
	{
		goto cancel_end;
	}

	return -1;

cancel_end:
	s_FirmupState.nWrited = MEDIA_MEM_ERR_CANCEL;
	utl_system("killall wget; rm -rf %s %s", binfile, verfile);
	mfirmup_release();
	return 0;
}

int mfirmup_get_writepercent(int* blocks, int* writed)
{
	int ret = 0;
	if(s_FirmupState.nBlocks != 0)
	{
		ret = s_FirmupState.nWrited * 100 / s_FirmupState.nBlocks;
	}
	if (blocks)		*blocks = s_FirmupState.nBlocks;
	if (writed)		*writed = s_FirmupState.nWrited;
	return ret;
}

