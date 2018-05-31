/**@file 负责远程升级
 *
 *具体参见doc/firmup目录的内容
 *
 * 远程升级的版本文件内容如下：
 * 
 
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
 *
 */

#include "jv_common.h"

#include "JvServer.h"
#include "MRemoteCfg.h"
#include "mfirmup.h"
#include "rcmodule.h"
#include "maccount.h"
#include "mtransmit.h"
#include "utl_common.h"
#include "mlog.h"
#include "SYSFuncs.h"
#include "sctrl.h"

#include "yst_firmup.h"


#ifdef YST_SVR_SUPPORT

//EX_FIRMUP_UPDINFO_REQ
typedef struct{
	char product[32];		//产品类别
	char url[4][128];	//升级服务器地址
	int urlCnt;
	char binName[32];		//升级文件
	char verName[32];		//升级版本文件
}UpdateInfo_t;

static struct
{
	char	binpath[256];
	char	verpath[256];
	FirmVersion_t	newver;
	int		binfd;			// bin文件句柄，文件升级使用
}s_FirmupYstInfo;


static void _Yst_FirmupEvent(int nEvent, void* arg, int param1, int param2)
{
	int* param = (int*)arg;
	int nCh = param[0];
	int nClientID = param[1];
	char data[128] = {0};
	PACKET *stFPacket = (PACKET *)data;
	stFPacket->nPacketType	= RC_EXTEND;
	stFPacket->nPacketCount	= RC_EX_FIRMUP;
	EXTEND *pstExt = (EXTEND*)stFPacket->acData;

	switch (nEvent)
	{
	case FIRMUP_DOWNLOAD_START:
	case FIRMUP_DOWNLOAD_PROGRESS:
	case FIRMUP_DOWNLOAD_FINISH:
		return;
	case FIRMUP_DOWNLOAD_TIMEOUT:
	case FIRMUP_DOWNLOAD_FAILED:
		pstExt->nType = EX_FIRMUP_RET;
		pstExt->nParam1 = param1;
		break;
	case FIRMUP_UPDATE_START:
		pstExt->nType = EX_FIRMUP_START;
		pstExt->nParam1 = 0;
		pstExt->nParam2 = param1;
		break;
	case FIRMUP_UPDATE_FINISH:
		pstExt->nType = EX_FIRMUP_OK;
		pstExt->nParam1 = param1;
		pstExt->nParam2 = param2;
		break;
	case FIRMUP_UPDATE_FAILED:
		pstExt->nType = EX_FIRMUP_RET;
		pstExt->nParam1 = param1;
		break;
	default:
		printf("%s, unknown event type: %d\n", __func__, nEvent);
		return;
	}

	MT_TRY_SendChatData(nCh, nClientID, JVN_RSP_TEXTDATA, (unsigned char*)data, 20);
}

VOID FirmupProc(REMOTECFG *cfg)
{
	EXTEND *pstEx = (EXTEND*)(cfg->stPacket.acData);

	ACCOUNT stAccount;
	char acCmd[MAX_PATH]={0};
	int ret = 0;

	if (GetClientAccount(cfg->nClientID, &stAccount) == NULL)
	{
		Printf("ERROR: account is NULL, remotecfg->nClientID: %d\n", cfg->nClientID);
		return;
	}

	if (!(stAccount.nPower & POWER_ADMIN))
	{
		Printf("User have no permision to update the devices, pstEx->nType: %d\n", pstEx->nType);
		pstEx->nType = EX_UPLOAD_CANCEL;	//1.设备正忙
		pstEx->nParam2 = 4;// 4. permision denied
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
		return;
	}

	switch(pstEx->nType)
	{
	case EX_FIRMUP_UPDINFO_REQ:
		{
			const FirmupCfg_t* pCfg = mfirmup_getconfig();
			UpdateInfo_t info;
			int i;

			memset(&info, 0, sizeof(info));
			snprintf(info.product, sizeof(info.product), "%s", pCfg->product);
			for (i = 0; i < ARRAY_SIZE(info.url); ++i)
			{
				if (!pCfg->url[i])
				{
					break;
				}
				snprintf(info.url[i], sizeof(info.url[i]), "%s", pCfg->url[i]);
				++info.urlCnt;
			}
			snprintf(info.binName, sizeof(info.binName), "%s", pCfg->binname);
			snprintf(info.verName, sizeof(info.verName), "%s", pCfg->vername);

			memcpy(pstEx->acData, &info, sizeof(info));
			pstEx->nParam3 = sizeof(info);
			pstEx->nType = EX_FIRMUP_UPDINFO_RESP;
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20+pstEx->nParam3);
			printf("vername: %s\n"
					"binName: %s\n", info.verName, info.binName);
		}
		break;
	case EX_UPLOAD_START:
		{
			const FirmupCfg_t* pCfg = mfirmup_getconfig();
			char *updateType = "";
			int ret = 0;

			Printf("EX_UPLOAD_START\n");
			ret = mfirmup_claim(pstEx->nParam1);
			if (ret != SUCCESS)
			{
				Printf("InitFirmup failed...\n");
				pstEx->nType = EX_UPLOAD_CANCEL;	//1.设备正忙
				pstEx->nParam2 = 1;
				goto upload_start_failed;
			}

			switch(pstEx->nParam1)
			{
			case FIRMUP_FTP:
			case FIRMUP_HTTP:
				updateType = (pstEx->nParam1 == FIRMUP_FTP) ? "FTP" : "HTTP";
				static int context[2];
				//开启升级进程
				context[0] = cfg->nCh;
				context[1] = cfg->nClientID;
				ret = mfirmup_auto_update((pstEx->nParam1 == FIRMUP_FTP) ? (char*)pstEx->acData : pCfg->url[0], 
												&s_FirmupYstInfo.verpath, &s_FirmupYstInfo.binpath, &s_FirmupYstInfo.newver, 
												FIRMUP_BIN_TIMEOUT, FALSE, _Yst_FirmupEvent, &context);
				if (ret != FIRMUP_SUCCESS)
				{
					pstEx->nType = EX_FIRMUP_RET;
					pstEx->nParam1 = ret;
					goto upload_start_failed;
				}

				pstEx->nType	= EX_UPLOAD_DATA;
				break;
			case FIRMUP_FILE:
				//创建临时版本文件
				updateType = "FILE";
				snprintf(s_FirmupYstInfo.verpath, sizeof(s_FirmupYstInfo.verpath), "/tmp/%s", pCfg->vername);
				S32 fd = open(s_FirmupYstInfo.verpath, O_CREAT | O_RDWR | O_TRUNC, 0777);
				if(fd <= 0)
				{
					pstEx->nType = EX_UPLOAD_CANCEL;
					pstEx->nParam2 = 2;	//2.开始上传文件失败
					Printf("Open tmpver file failed...\n");
				}
				write(fd, pstEx->acData, pstEx->nParam3);
				close(fd);

				ret = mfirmup_check_verfile(s_FirmupYstInfo.verpath, &s_FirmupYstInfo.newver);
				printf("bin fileSize: 0x%x\n", s_FirmupYstInfo.newver.fileSize);
				if (!ret)
				{
					pstEx->nType = EX_FIRMUP_RET;
					pstEx->nParam1 = FIRMUP_LATEST_VERSION;
					goto upload_start_failed;
				}

				//检查产品区别，如果产品不一样则拒绝升级,lck20120815
				printf("local product=%s, target product=%s\n", pCfg->product, s_FirmupYstInfo.newver.product);
				if(s_FirmupYstInfo.newver.product[0] && strcmp(s_FirmupYstInfo.newver.product, pCfg->product) != 0)
				{
					Printf("Firmup not fit...\n");
					//向分控发送升级结果
					pstEx->nType = EX_FIRMUP_RET;
					pstEx->nParam1 = FIRMUP_PRODUCT_ERROR;
					goto upload_start_failed;
				}

				char binpath[256] = {0};
				ret = mfirmup_prepare_location(binpath, sizeof(binpath));
				if (ret == FIRMUP_LOCATION_NONE)
				{
					pstEx->nType = EX_FIRMUP_RET;
					pstEx->nParam1 = FIRMUP_OTHER_ERROR;
					goto upload_start_failed;
				}

				snprintf(s_FirmupYstInfo.binpath, sizeof(s_FirmupYstInfo.binpath), "%s/%s", binpath, pCfg->binname);
				s_FirmupYstInfo.binfd = open(s_FirmupYstInfo.binpath, O_CREAT | O_RDWR | O_TRUNC, 0777);
				if(s_FirmupYstInfo.binfd <= 0)
				{
					printf("open %s failed with %s(%d)\n", s_FirmupYstInfo.binpath, strerror(errno), errno);
					pstEx->nType = EX_UPLOAD_CANCEL;
					pstEx->nParam2 = 2;	//2.开始上传文件失败
					goto upload_start_failed;
				}
				pstEx->nType	= EX_UPLOAD_DATA;
				break;
			default:
				break;
			}
			mlog_write("Updating... Download Started with type: %s", updateType);
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,(U8*)&cfg->stPacket, 20);
			break;

		upload_start_failed:
			mfirmup_release();
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA,(U8*)&cfg->stPacket, 20);
		}
		break;
	case EX_UPLOAD_DATA:
		switch(pstEx->nParam1)
		{
		case FIRMUP_FTP:
		case FIRMUP_HTTP:
			{
				//获取下载文件大小，将下载比例发送给分控
				pstEx->nType = EX_UPLOAD_DATA;
				pstEx->nParam2 = utl_get_file_size(s_FirmupYstInfo.binpath) * 100 / s_FirmupYstInfo.newver.fileSize;
				Printf("##########EX_UPLOAD_DATA step: %d\n", pstEx->nParam2);
				MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
			}
			break;
		case FIRMUP_FILE:
			if(pstEx->nParam2 != pstEx->nParam3)
			{
				Printf("Recive data error...\n");
				pstEx->nType = EX_UPLOAD_CANCEL;
				pstEx->nParam2 = 3;	//3.传输中出现错误
				MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
			}
			write(s_FirmupYstInfo.binfd, pstEx->acData, pstEx->nParam3);

			cfg->stPacket.nPacketCount = RC_EX_FIRMUP;
			pstEx->nType = EX_UPLOAD_DATA;
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
			break;
		default:
			break;
		}
		break;
	case EX_UPLOAD_CANCEL:
		Printf("EX_UPLOAD_CANCEL\n");
		if (mfirmup_b_updating())
		{
			pstEx->nParam2 = 1;	//1.设备忙
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
			return;
		}

		int ret = mfirmup_cancelupdate(s_FirmupYstInfo.verpath, s_FirmupYstInfo.binpath);
		if (ret < 0)
		{
			pstEx->nParam2 = 1;	//1.设备忙
			MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
			return;
		}
		if(s_FirmupYstInfo.binfd > 0)
		{
			close(s_FirmupYstInfo.binfd);
		}
		memset(&s_FirmupYstInfo, 0, sizeof(s_FirmupYstInfo));
		pstEx->nParam2 = 0;	//0.用户取消升级
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
		break;
	case EX_UPLOAD_OK:
		Printf("EX_UPLOAD_OK\n");
		if(s_FirmupYstInfo.binfd > 0)
		{
			write(s_FirmupYstInfo.binfd, pstEx->acData, pstEx->nParam3);
			close(s_FirmupYstInfo.binfd);
			s_FirmupYstInfo.binfd = 0;
		}
		pstEx->nType = EX_UPLOAD_OK;
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
		break;
	case EX_FIRMUP_START:
		{
			int ret;
			static int context[2];
			//开启升级进程
			Printf("Start firmup\n");
			Printf("xian    nCh:%d, nClientID:%d   \n", cfg->nCh,         cfg->nClientID);
			mlog_write("Updating... Write Started");
			context[0] = cfg->nCh;
			context[1] = cfg->nClientID;

			ret = mfirmup_startupdate(s_FirmupYstInfo.verpath, s_FirmupYstInfo.binpath, FALSE, 
											_Yst_FirmupEvent, context);
			if (ret != FIRMUP_SUCCESS)
			{
				mfirmup_release();
				//发送升级完成消息
				pstEx->nType = EX_FIRMUP_RET;
				pstEx->nParam1 = ret;
				MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
			}
		}
		break;
	case EX_FIRMUP_OK:
		break;
	case EX_FIRMUP_STEP:
		mfirmup_get_writepercent((int*)&pstEx->nParam2, (int*)&pstEx->nParam1);
		if(pstEx->nParam1 < pstEx->nParam2)
		{
			pstEx->nType = EX_FIRMUP_STEP;
		}
		else
		{
			//发送升级完成消息
			pstEx->nType = EX_FIRMUP_OK;
		}
		MT_TRY_SendChatData(cfg->nCh, cfg->nClientID, JVN_RSP_TEXTDATA, (U8*)&cfg->stPacket, 20);
		break;
	case EX_FIRMUP_REBOOT:
		//mlog_write("Client Restart Device");
		SYSFuncs_reboot();
		break;
	case EX_FIRMUP_RESTORE:
		//mlog_write("Client Recover Device");
		SYSFuncs_factory_default(EXIT_RECOVERY);
		break;
	default:
		break;
	}
}
#endif

