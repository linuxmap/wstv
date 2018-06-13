#include "jv_common.h"
#include <sys/types.h>  
#include <termios.h>
#include "MRemoteCfg.h"
#include "rcmodule.h"
#include "maccount.h"
#include "mtransmit.h"
#include "JvServer.h"
#include "mptz.h"
#include "muartcomm.h"
#include "SYSFuncs.h"

static int urtfd = -1; 
static ptz_thread_group_t mcomtrans_group;
/**
*@brief  打开设备
*@param  fd     类型 int  打开串口的文件句柄
*@return  文件句柄或者错误码
*/
int OpenDev(char *Dev)
{
    if(NULL == Dev || 0 == Dev[0])
    {
        return -1;
    }
    else
    {
        return open(Dev, O_SYNC | O_RDWR | O_NOCTTY);
    }
}

void ComtransThrd(void* ptr)
{
	fd_set readfds;
	struct timeval timeout;
	int ret = 0;
	int rlen = 0;
	int i = 0;

	U8 rbuf[32] = {0};
	U8 *p = &rbuf[0];

	pthreadinfo_add((char *)__func__);

	mcomtrans_group.running = TRUE;

	char buffer[128] = {0};
	PACKET *stFPacket = (PACKET *)buffer;
	stFPacket->nPacketType	= RC_EXTEND;
	stFPacket->nPacketCount	= RC_EX_COMTRANS;
	EXTEND *pstFExt = (EXTEND*)stFPacket->acData;
	pstFExt->nType = EX_COMTRANS_RESV;
	
	while (mcomtrans_group.running)
	{
		FD_ZERO(&readfds);
		FD_SET(urtfd, &readfds);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		ret = select(urtfd+1, &readfds, NULL, NULL, &timeout);
		if(ret < 0)
		{
			perror("select");
			break;
		}
		else if(ret == 0)
		{
			Printf("select timeout\n");
			continue;
		}
		else
		{
			if(FD_ISSET(urtfd, &readfds))
			{
				/*串口调试功能，收发消息*/
				while(read(urtfd, p, 1) > 0)
				{
					rlen++;
					p++;
				}
				Printf("ComtransThrd   rlen=%d[<=10]  Hex: \n", rlen);
			}
		}
		usleep(100*1000);
	}
	//close(urtfd);
	mcomtrans_group.running = FALSE;
}

void muartcomm_init()
{
	return;

	urtfd = OpenDev("/dev/ttyAMA1");

	pthread_mutex_init(&mcomtrans_group.mutex, NULL);

	if(urtfd > 0)
	{
		PTZ pPtz;
		pPtz.nHwParams.nBaudRate = 9600;
		pPtz.nHwParams.nCharSize = 8;
		pPtz.nHwParams.nStopBit = 1;
		pPtz.nHwParams.nParityBit = PAR_NONE;
		DecoderSetCom(urtfd, &pPtz.nHwParams);

		pthread_t pid;
		pthread_create_detached(&pid,NULL,(void*)ComtransThrd,NULL);
	}
}

VOID ComTransProc(REMOTECFG *cfg)
{
	return;

	EXTEND *pstEx = (EXTEND*)(cfg->stPacket.acData);

	char wbuf[64] = {0};
	if(urtfd <= 0)
	{
		printf("An error occurred while trying to handle ComTrans.\n");
		return;
	}
	
	Printf("ComTransProc: ");
	pthread_mutex_lock(&mcomtrans_group.mutex);
	switch(pstEx->nType)
	{
	case EX_COMTRANS_OPEN:
		{
			if(mcomtrans_group.running == TRUE)
			{
				printf("Comsend	already opened yeat!\n");
				break;
			}
			printf("Comsend open	");
			mcomtrans_group.running = TRUE;
			pthread_t pidComTrans;
			pthread_create_detached(&pidComTrans,NULL,(void*)ComtransThrd,NULL);
		}
		break;
	case EX_COMTRANS_CLOSE:
		{
			printf("Comsend close	");
			mcomtrans_group.running = FALSE;
		}
		break;
	case EX_COMTRANS_SEND:
		{
		}
		break;
	case EX_COMTRANS_SET:
		printf("com set: %s\n", (char *)pstEx->acData);
		snprintf(wbuf, sizeof(wbuf), "%s", (char *)pstEx->acData);

		int nBaudRate;
		char cParityBit;
		int nCharSize;
		int nStopBit;
		sscanf(wbuf, "%d,%c,%d,%d", &nBaudRate, &cParityBit, &nCharSize, &nStopBit);  // 参数格式"9600,n,8,1"
		PTZ *pPtz = PTZ_GetInfo();
		pPtz[0].nBaudRate = nBaudRate;
		pPtz[0].nHwParams.nBaudRate = nBaudRate;
		pPtz[0].nHwParams.nCharSize = nCharSize;
		pPtz[0].nHwParams.nStopBit = nStopBit;
		switch(cParityBit)
		{
			case 'n':
			case 'N':
				pPtz[0].nHwParams.nParityBit = PAR_NONE;
				break;
			case 'e':
			case 'E':
				pPtz[0].nHwParams.nParityBit = PAR_EVEN;
				break;
			case 'o':
			case 'O':
				pPtz[0].nHwParams.nParityBit = PAR_ODD;
				break;
			default:
				break;
		}
		DecoderSetCom(urtfd, &pPtz->nHwParams);
		WriteConfigInfo();
		break;
	default:
		Printf("default:%d", pstEx->nType);
		break;
	}
	pthread_mutex_unlock(&mcomtrans_group.mutex);
}

