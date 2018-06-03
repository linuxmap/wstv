#include <termios.h>
#include <pthread.h>
#include <jv_common.h>
#include <utl_filecfg.h>
#include "mptz.h"
#include "libPTZ.h"
#include <utl_cmd.h>
#include <jv_rs485.h>
#include "jv_gpio.h"
#include <utl_timer.h>
#include <SYSFuncs.h>
#include "msensor.h"
#include <mwdt.h>
#include <msoftptz.h>
#include <jv_sensor.h>
#include <mosd.h>
#include <mstream.h>
#include "jv_uartcomm.h"

#define PTZ_CNT	1
#define PATROL_CNT	2

static BOOL g_PtzRunning = FALSE;
static pthread_t 	patrolThreadId;
static void *OnPtzPatrolThread(void *param);

static pthread_mutex_t guardmutex; 
//static PTZ_SCHGUARD_STATUS schGuard_status;	//定时任务守护状态
static PTZ_SCHEDULE_STATUS sch_status;		//定时任务状态
//预置点信息
typedef struct
{
	int presetno;
    int zoom;
	int focus;
	int left;
	int up;
}PTZPresetPos_t;

static struct
{
		PTZ stPtz[PTZ_CNT];//每个通道的云台结构
		PTZ_PRESET_INFO stPreInf[PTZ_CNT]; 	//预置点信息
		PTZ_PATROL_INFO stPatrolInf[PTZ_CNT][PATROL_CNT];	//巡航信息
		PTZ_GUARD_T	stGuardInf[PTZ_CNT];

		PTZPresetPos_t presetPos[PTZ_CNT][MAX_PRESET_CT];// 自己管理电机时，用于保存预置点的位置
		PTZ_SCHEDULE_INFO stScheduleInf[PTZ_CNT];
}s_PTZInfo;

//云台控制状态
static struct
{
    PTZ				*pPtz;			//云台结构
    PTZ_PRESET_INFO 	*pPreInf;		//预置点信息
    PTZ_PATROL_INFO 	*pPatrolInf;	//巡航信息
    PTZ_GUARD_T		pGuardInf;

	BOOL		    	bPtzAuto;                   //是否处于自动状态
	BOOL		    	bStartPatrol;               //是否处于软巡航状态
	S32		    		nCurPatrolNod;              //当前软巡航点
	time_t          	tmCurPatrolNod;             //当前巡航时间

	time_t		guardLastTime;			//守望计时时间
	U32 		bootItem;				//开机启动项
	S32		    nPatrolPath;          	//当前软巡线
	PTZ_SCHEDULE_INFO *pScheduleInf;
	BOOL		    	bStartTrail;               //是否处于轨迹状态
}g_PtzStatus[PTZ_CNT];

#define PTZ_CTRL_UDLR 	0	//上下左右偏移量
#define PTZ_CTRL_AUTO	1	//自动
#define PTZ_CTRL_FOUCS 	2	//变焦
#define PTZ_CTRL_SCAN	3	//扫描
#define PTZ_CTRL_GUARD 	4	//守望
#define PTZ_CTRL_TRAIL	5	//轨迹


static U16 ptz_ctrl = 0; // [0]:上下左右，[1]: 自动，[2]:变焦，[3]:线扫花样，[4]:
#if 0
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

#define PRESETPOS_FILE CONFIG_PATH"presetpos.cfg"

static void __presetpos_save()
{
	int i,ch;
	FILE *fp;
	fp = fopen(PRESETPOS_FILE, "wb");
	if (!fp)
	{
		printf("Failed open %s, because: %s\n", PRESETPOS_FILE, strerror(errno));
		return ;
	}
	ch = 0;
	for (i=0;i<MAX_PRESET_CT;i++)
	{
		if (s_PTZInfo.presetPos[ch][i].presetno > 0)
		{
			fwrite(&s_PTZInfo.presetPos[ch][i], 1, sizeof(PTZPresetPos_t), fp);
		}
	}
	fclose(fp);
}

static void __presetpos_read()
{
	int i,ch = 0;
	FILE *fp;
	fp = fopen(PRESETPOS_FILE, "rb");
	if (!fp)
	{
		printf("Failed open %s, because: %s\n", PRESETPOS_FILE, strerror(errno));
		return ;
	}
	PTZPresetPos_t pos;
	int len;
	while(1)
	{
		len = fread(&pos, 1, sizeof(pos), fp);
		if (len != sizeof(pos))
			break;

		if (pos.presetno > 0 && pos.presetno < MAX_PRESET_CT)
		{
			s_PTZInfo.presetPos[ch][pos.presetno] = pos;
		}
		else
			break;
	}
	fclose(fp);
}

PTZ *PTZ_GetInfo(void)
{
	return s_PTZInfo.stPtz;
}

PTZ_PRESET_INFO *PTZ_GetPreset(void)
{
	return s_PTZInfo.stPreInf;
}

PTZ_PATROL_INFO *PTZ_GetPatrol(void)
{
	return (s_PTZInfo.stPatrolInf[0]);
}

PTZ_GUARD_T *PTZ_GetGuard(void)
{
	return s_PTZInfo.stGuardInf;
}

PTZ_SCHEDULE_INFO *PTZ_GetSchedule(void)
{
	return s_PTZInfo.stScheduleInf;
}

//PTZ_SCHGUARD_STATUS *PTZ_GetGuardStatus(void)
//{
//	return &schGuard_status;
//}

/**/
void PtzReSetup(NC_PORTPARAMS param)
{

	 jv_rs485_lock();
	 DecoderSetCom(jv_rs485_get_fd(), &param);
	  //释放互斥量
	 jv_rs485_unlock();

}
/*设置红外模式*/
void PTZ_SetIrMod(int mode)
{
	if(mode>MSENSOR_DAYNIGHT_ALWAYS_NIGHT || mode<MSENSOR_DAYNIGHT_AUTO)
	{
		return;
	}
	if(jv_rs485_get_fd() > 0)
	{
		printf("[%s]:%d mode:%d\n", __FUNCTION__, __LINE__,mode);
		switch(mode)
		{
			case MSENSOR_DAYNIGHT_AUTO:
			{
				PtzLocatePreset(0, 91);
				usleep(200000);
				PtzLocatePreset(0, 3);	
				break;
			}
			case MSENSOR_DAYNIGHT_ALWAYS_DAY:
			{
				PtzLocatePreset(0, 91);
				usleep(200000);
				PtzLocatePreset(0, 1);	
				break;
			}
			case MSENSOR_DAYNIGHT_ALWAYS_NIGHT:
			{
				PtzLocatePreset(0, 91);
				usleep(200000);
				PtzLocatePreset(0, 2);	
				break;
			}
			default:
				printf("Not supporter mode\n");
				break;
		}
	}
}
/*
 * 云台控制初始化
 */
S32 PTZ_Init()
{
	S32 i;
	int ret;
	//static NC_PORTPARAMS	s_serial_param = { 2400, 8,  1, PAR_NONE, PTZ_DATAFLOW_NONE };
	PTZ *mpPtz = PTZ_GetInfo();
	jv_rs485_init();
	ret = DecoderSetCom(jv_rs485_get_fd(), &mpPtz->nHwParams);
	for (i = 0; i < PTZ_CNT; ++i)
	{
		g_PtzStatus[i].pPtz = &s_PTZInfo.stPtz[i];
		g_PtzStatus[i].pPreInf = &s_PTZInfo.stPreInf[i];
		g_PtzStatus[i].pPatrolInf = s_PTZInfo.stPatrolInf[i];
		//g_PtzStatus[i].pGuardInf = &s_PTZInfo.stGuardInf[i];
		memcpy(&g_PtzStatus[i].pGuardInf, &s_PTZInfo.stGuardInf[i], sizeof(PTZ_GUARD_T));
		g_PtzStatus[i].bPtzAuto = FALSE;
		g_PtzStatus[i].bStartPatrol = FALSE;
		g_PtzStatus[i].nCurPatrolNod = -1;
		g_PtzStatus[i].tmCurPatrolNod = 0;
		g_PtzStatus[i].nPatrolPath = -1;
		g_PtzStatus[i].pScheduleInf = &s_PTZInfo.stScheduleInf[i];
		g_PtzStatus[i].bStartTrail = FALSE;
	}
	

	g_PtzRunning = TRUE;
	pthread_create(&patrolThreadId, NULL, OnPtzPatrolThread, NULL);
	__presetpos_read();
	msoftptz_init();

    return JVERR_NO;
}

S32 PTZ_Release()
{
	jv_rs485_deinit();

	if (TRUE == g_PtzRunning)
	{
		g_PtzRunning = FALSE;
		pthread_join(patrolThreadId, NULL);
	}
	msoftptz_deinit();

	return JVERR_NO;
}

/**
 * 设置预置点
 * @param nCh	通道号，默认 0
 * @param nPreset 预置点号
 */
void inline PtzSetPreset(U32 nCh, S32 nPreset)
{
    //取得互斥量
     jv_rs485_lock();
     PPTZ pPtz = g_PtzStatus[nCh].pPtz;
     DecoderSetPreset(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nPreset);
     //释放互斥量
     jv_rs485_unlock();
}
/**
 * 设置预置点
 * @param nCh	通道号，默认 0
 * @param nPreset 预置点号
 */
void inline PtzClearPreset(U32 nCh, S32 nPreset)
{
    //取得互斥量
     jv_rs485_lock();
     PPTZ pPtz = g_PtzStatus[nCh].pPtz;
     DecoderClearPreset(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nPreset);
     //释放互斥量
     jv_rs485_unlock();
}

/*
 * 添加预置点
 * @param nCh: 通道
 * @param nPreset: 预置点
 * @return: 0 成功
 * 			-1 预置点非法
 * 			-2 预置点重复
 * 			-3 预置点数量到达上限
 */
S32 PTZ_AddPreset(U32 nCh, U32 nPreset, char *name)
{
	U32 i;
	int argv[2];
	if (0 >= nPreset || 255 < nPreset)
	{
		return -1;
	}

	if (MAX_PRESET_CT <= g_PtzStatus[nCh].pPreInf->nPresetCt)
		return -3;

	//检查预置点是否重复
	for (i = 0; i < g_PtzStatus[nCh].pPreInf->nPresetCt; ++i)
	{
		if (nPreset == g_PtzStatus[nCh].pPreInf->pPreset[i])
		{
			return -2;
		}
	}

	//添加预置点，清空预置点CTEXT，更新预置点列表
	++g_PtzStatus[nCh].pPreInf->nPresetCt;
	g_PtzStatus[nCh].pPreInf->pPreset[i] = nPreset;
	strncpy(g_PtzStatus[nCh].pPreInf->name[i], name, sizeof(g_PtzStatus[nCh].pPreInf->name[0]));
	s_PTZInfo.presetPos[nCh][nPreset].presetno = -1;
	if( g_PtzStatus[nCh].pPtz->nProtocol== PTZ_PROTOCOL_SELF)
	{
		s_PTZInfo.presetPos[nCh][nPreset].presetno = nPreset;
		s_PTZInfo.presetPos[nCh][nPreset].focus = argv[1];
		s_PTZInfo.presetPos[nCh][nPreset].zoom = argv[0];
		printf("%s: Get lenpos %x, %x,i=%d,presetnum=%d\n",__func__,argv[0],argv[1],i,g_PtzStatus[nCh].pPreInf->pPreset[i]);
	}
	if (msoftptz_b_support(nCh))
	{
		JVPTZ_Pos_t pos;
		msoftptz_pos_get(nCh, &pos);
		s_PTZInfo.presetPos[nCh][nPreset].presetno = nPreset;
		s_PTZInfo.presetPos[nCh][nPreset].left = pos.left;
		s_PTZInfo.presetPos[nCh][nPreset].up = pos.up;
	}
	//需要保存这东西
	if (s_PTZInfo.presetPos[nCh][nPreset].presetno)
	{
		__presetpos_save();
	}
    PtzSetPreset(nCh, nPreset);

	return JVERR_NO;
}

S32 PTZ_AddPreset_From_Dooralarm(U32 nCh, char *name)
{
	int nPreset = g_PtzStatus[nCh].pPreInf->nPresetCt+1;
	if(0 == PTZ_AddPreset(nCh,nPreset,name))
	{
		return nPreset;
	}
	else
		return -1;
}


/*
 * 删除预置点
 * @param nCh: 通道
 * @param nPreset: 预置点
 * @return: 0 成功
 * 			-1 删除的预置点不存在
 */
S32 PTZ_DelPreset(U32 nCh, U32 nPreset)
{
	S32 i, times, nIndex = -1;
	//删除预置点会影响到巡航，所以停止巡航
	PTZ_StopPatrol(nCh);

	//检查删除的预置点是否存在
	for (i = 0; i < g_PtzStatus[nCh].pPreInf->nPresetCt; ++i)
	{
		if (nPreset == g_PtzStatus[nCh].pPreInf->pPreset[i])
		{
			nIndex = i;
			break;
		}
	}

	if (0 > nIndex)
	{
		return -1;
	}

	//删除的点后面依次前移
	times = g_PtzStatus[nCh].pPreInf->nPresetCt - 1;
	for (i = nIndex; i < times; ++i)
	{
		g_PtzStatus[nCh].pPreInf->pPreset[i] = g_PtzStatus[nCh].pPreInf->pPreset[i + 1];
		strcpy(g_PtzStatus[nCh].pPreInf->name[i], g_PtzStatus[nCh].pPreInf->name[i + 1]);
	}
	g_PtzStatus[nCh].pPreInf->pPreset[i] = 0;
	--g_PtzStatus[nCh].pPreInf->nPresetCt;

	//遍历巡航数组，拷贝不是要删除的预置点
	int nPatrol;
	PATROL_NOD  aPatrol[MAX_PATROL_NOD];
	U32         nPatrolSize;
	for(nPatrol=0; nPatrol<PATROL_CNT; nPatrol++)
	{
		nPatrolSize = 0;
		memset(aPatrol, 0, sizeof(aPatrol));
		for (i = 0; i < g_PtzStatus[nCh].pPatrolInf[nPatrol].nPatrolSize; ++i)
		{
			if (nPreset != g_PtzStatus[nCh].pPatrolInf[nPatrol].aPatrol[i].nPreset)
			{
				aPatrol[nPatrolSize].nPreset = g_PtzStatus[nCh].pPatrolInf[nPatrol].aPatrol[i].nPreset;
				aPatrol[nPatrolSize].nStayTime = g_PtzStatus[nCh].pPatrolInf[nPatrol].aPatrol[i].nStayTime;
				++nPatrolSize;
			}
		}
		g_PtzStatus[nCh].pPatrolInf[nPatrol].nPatrolSize = nPatrolSize;
		memcpy(g_PtzStatus[nCh].pPatrolInf[nPatrol].aPatrol, aPatrol, sizeof(aPatrol));
	}
	PtzClearPreset(nCh, nPreset);

	return JVERR_NO;
}

static void _Ptz_patrolinfo_save(int chan,int patrol_path)
{
	FILE *fp;
	fp = fopen(PATROL_FLAG_FLAG, "wb");
	if (!fp)
	{
		printf("In softptz position save function,Failed open %s, because: %s\n", PATROL_FLAG_FLAG, strerror(errno));
		return ;
	}

	PATROL_INFO painfo;
	painfo.chan = chan;
	painfo.patrol_path = patrol_path;

	fwrite(&painfo, 1, sizeof(PATROL_INFO), fp);

	fclose(fp);
}


/*
 * 开始巡航
 * @param nCh: 通道	nPatrol：巡航线
 */
S32 PTZ_StartPatrol(U32 nCh, U32 nPatrol)
{
	if(g_PtzStatus[nCh].pPatrolInf[nPatrol].nPatrolSize >= 1)
	{
		g_PtzStatus[nCh].bStartPatrol = TRUE;
		g_PtzStatus[nCh].nCurPatrolNod = 0;
		g_PtzStatus[nCh].tmCurPatrolNod = time(NULL);
		g_PtzStatus[nCh].nPatrolPath = nPatrol;
		PtzLocatePreset(nCh, g_PtzStatus[nCh].pPatrolInf[nPatrol].aPatrol[0].nPreset);
		_Ptz_patrolinfo_save(nCh,nPatrol);
	}
	return JVERR_NO;
}

/*
 * 停止巡航
 * @param nCh: 通道
 */
S32 PTZ_StopPatrol(U32 nCh)
{
#if 0
    if(ptzHandle >= 0)
	{
		mchnosd_region_destroy(ptzHandle);
		printf(">>>4    Patrol stop,mchnosd_region_destroy!\n");
		ptzHandle = -1;
        PresetId = -1;
	}
#endif
	g_PtzStatus[nCh].bStartPatrol = FALSE;
	g_PtzStatus[nCh].nPatrolPath = -1;;
	if(0 == access(PATROL_FLAG_FLAG, F_OK))
		utl_system("rm "PATROL_FLAG_FLAG);
	return JVERR_NO;
}
//暂停守望
S32 Ptz_Guard_Pause(U32 nCh)
{
	g_PtzStatus[nCh].guardLastTime = 0;
	return JVERR_NO;
}

S32 inline PTZ_Auto(U32 nCh, U32 nSpeed)
{
	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (TRUE == g_PtzStatus[nCh].bPtzAuto)
    {
    	g_PtzStatus[nCh].bPtzAuto = FALSE;

        //取得互斥量
         jv_rs485_lock();

         DecoderAutoStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

         //释放互斥量
         jv_rs485_unlock();
    }
    else
    {
    	g_PtzStatus[nCh].bPtzAuto = TRUE;
        //取得互斥量
        jv_rs485_lock();

        DecoderAutoStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);

        //释放互斥量
        jv_rs485_unlock();
    }
    return JVERR_NO;
}

//停止巡航等
static void __Ptz_StopPatrol(U32 nCh)
{
	PTZ_StopPatrol(nCh);
}

//暂停守望
static void __Ptz_Guard_Pause(U32 nCh)
{
	g_PtzStatus[nCh].guardLastTime = 0;
}

//重设守望
static void __Ptz_Guard_Reset(U32 nCh)
{
	g_PtzStatus[nCh].guardLastTime = time(NULL);
}

/*
 * 获取云台控制状态，若正在控制则为 1，否则为 0
 * @param nCh: 通道
 */
BOOL PTZ_Get_Status(U32 nCh)
{
	BOOL ptzctrl = FALSE;
	BOOL zoom = FALSE;
	ptzctrl = g_PtzStatus[nCh].bStartPatrol||g_PtzStatus[nCh].bStartTrail||zoom||(ptz_ctrl>0)
		||(g_PtzStatus[nCh].guardLastTime>0&&(g_PtzStatus[nCh].pGuardInf.guardType==GUARD_PRESET));
		
	Printf("pat%d  zoom%d  other%d  guard%d\n",g_PtzStatus[nCh].bStartPatrol,zoom,ptz_ctrl,
		((g_PtzStatus[nCh].guardLastTime>0)&&(g_PtzStatus[nCh].pGuardInf.guardType==GUARD_PRESET)));
	return	ptzctrl;
}

/*
 * 获取巡航状态
 * @param nCh: 通道
 */
BOOL PTZ_Get_PatrolStatus(U32 nCh)
{
	return	g_PtzStatus[nCh].bStartPatrol;
}
/*
 * 获取轨迹状态
 * @param nCh: 通道
 */
BOOL PTZ_Get_TrailStatus(U32 nCh)
{
	return	g_PtzStatus[nCh].bStartTrail;
}
/*
 * 巡航线程函数
 */
static void *OnPtzPatrolThread(void *param)
{
	U32 i;
	U32 nChnCnt = 1;
	int firstdone=0;//云台由移动变为停止的标志
	time_t	now;
	U32 *nCurNod;
	S32 nPatrol;
	PATROL_NOD * pPatrol;

	pthreadinfo_add((char *)__func__);

	Printf("Thread Func: %s pid: %d\n", __func__, utl_thread_getpid());
	for (i = 0; i < nChnCnt; ++i)
		g_PtzStatus[i].guardLastTime = time(NULL);
	while (TRUE == g_PtzRunning)
	{
		now = time(NULL);
		for (i = 0; i < nChnCnt; ++i)
		{
			nPatrol = g_PtzStatus[i].nPatrolPath;
			//没有在巡航中
			if (FALSE == g_PtzStatus[i].bStartPatrol || g_PtzStatus[i].pPatrolInf[nPatrol].nPatrolSize == 0)
			{
				//守望
				if (g_PtzStatus[i].pGuardInf.guardTime > 0 && g_PtzStatus[i].guardLastTime != 0)
				{
					if (g_PtzStatus[i].guardLastTime + g_PtzStatus[i].pGuardInf.guardTime < now)
					{
						switch(g_PtzStatus[i].pGuardInf.guardType)
						{
						default:
							break;
						case GUARD_PRESET:
							PtzLocatePreset(i, g_PtzStatus[i].pGuardInf.nRreset);
							break;
						case GUARD_PATROL:
							PTZ_StartPatrol(i, 0);
							break;
						case GUARD_TRAIL:
							PtzTrailStart(i, g_PtzStatus[i].pGuardInf.nTrail);
							break;
						case GUARD_SCAN:
							break;
						}
						g_PtzStatus[i].guardLastTime = 0;
					}
				}

				continue;
			}
			
			//当前巡航预置点到时间，转到下一个
			nCurNod = (U32 *)&g_PtzStatus[i].nCurPatrolNod;
			pPatrol = g_PtzStatus[i].pPatrolInf[nPatrol].aPatrol;
			//if(jv_ptz_move_done(0))
			{
				if(firstdone==0)//云台移动停止，开始计时，用于停留时间
				{
					g_PtzStatus[i].tmCurPatrolNod = now;
					firstdone=1;
				}
				if (now - g_PtzStatus[i].tmCurPatrolNod > pPatrol[*nCurNod].nStayTime)
				{
					//g_PtzStatus[i].tmCurPatrolNod = now;
					++g_PtzStatus[i].nCurPatrolNod;
					firstdone=0;
					if (g_PtzStatus[i].nCurPatrolNod >= g_PtzStatus[i].pPatrolInf[nPatrol].nPatrolSize)
					{
						g_PtzStatus[i].nCurPatrolNod = 0;
					}
					Printf("patrol ch%d= 	Patrol:%d	go preset%d		nPatrolSize:%d\n", i,nPatrol, pPatrol[*nCurNod].nPreset,
						g_PtzStatus[i].pPatrolInf[nPatrol].nPatrolSize);
					PtzLocatePreset(i, pPatrol[*nCurNod].nPreset);
					usleep(50000);	//必须睡眠一下才能再调用下个预置点
				}
			}
		}
		//睡一会，不然cpu一直居高不下
		usleep(200000);
	}
	printf("out of OnPtzPatrolThread\n");
	return NULL;
}
BOOL __ismirror()
{
	msensor_attr_t snAttr;
	msensor_getparam(&snAttr);
	if((snAttr.effect_flag>>EFFECT_MIRROR)&0x01)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
BOOL __isturn()
{
	msensor_attr_t snAttr;
	msensor_getparam(&snAttr);
	if((snAttr.effect_flag>>EFFECT_TURN)&0x01)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//开始向上
void inline PtzUpStart(U32 nCh, S32 nSpeed)
{
	if (msoftptz_b_support(nCh))
	{
		if(__isturn())
		{
			msoftptz_move_start(nCh, 0, -nSpeed, 0);
		}else
		{
			msoftptz_move_start(nCh, 0, nSpeed, 0);
		}
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉高引脚
		if(__isturn())
		{
			jv_gpio_write(0, 6, 1);
		}
		else
		{
			jv_gpio_write(0, 7, 1);
		}
		
		return;
	}
	
    //取得互斥量
    jv_rs485_lock();
    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bUpDownSwitch)
    {
        Printf("PTZ Control:DecoderUpStart,fd: %d nAddr: %d nProtocol: %d nBaudRate: %d nSpeed: %d\n",jv_rs485_get_fd(),pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
        DecoderUpStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }
    else
    {
        DecoderDownStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }
    //释放互斥量
	jv_rs485_unlock();
	__Ptz_StopPatrol(nCh);
	__Ptz_Guard_Pause(nCh);
}
//停止向上
void inline PtzUpStop(U32 nCh)
{
	if (msoftptz_b_support(nCh))
	{
		msoftptz_move_stop(nCh);
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉低引脚
		jv_gpio_write(0, 7, 0);
		jv_gpio_write(0, 6, 0);
		jv_gpio_write(2, 2, 0);
		jv_gpio_write(2, 4, 0);
		return;
	}

    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bUpDownSwitch)
    {
        DecoderUpStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderDownStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}
//开始向下
void inline PtzDownStart(U32 nCh, S32 nSpeed)
{
	if (msoftptz_b_support(nCh))
	{
		//画面的翻转只在上下运动时起作用
		if(__isturn())
		{
			msoftptz_move_start(nCh, 0, nSpeed, 0);
		}else
		{
			msoftptz_move_start(nCh, 0, -nSpeed, 0);
		}
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉高引脚
		if (__isturn())
		{
			jv_gpio_write(0, 7, 1);
		}
		else
		{
			jv_gpio_write(0, 6, 1);
		}
		
		return;
	}

    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bUpDownSwitch)
    {
        DecoderDownStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }
    else
    {
        DecoderUpStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止向下
void inline PtzDownStop(U32 nCh)
{
	if (msoftptz_b_support(nCh))
	{
		msoftptz_move_stop(nCh);
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉低引脚
		//拉低引脚
		jv_gpio_write(0, 7, 0);
		jv_gpio_write(0, 6, 0);
		jv_gpio_write(2, 2, 0);
		jv_gpio_write(2, 4, 0);
		return;
	}
	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bUpDownSwitch)
    {
        DecoderDownStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderUpStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}
//开始向左
void inline PtzLeftStart(U32 nCh, S32 nSpeed)
{
	if (msoftptz_b_support(nCh))
	{
		//画面的镜像只在左右运动时起作用
		if(__ismirror())
		{
			msoftptz_move_start(nCh, -nSpeed,0 , 0);
		}else
		{
			msoftptz_move_start(nCh, nSpeed,0 , 0);
		}
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉高引脚
		if (__ismirror())
		{
			jv_gpio_write(2, 4, 1);
		}
		else
		{
			jv_gpio_write(2, 2, 1);
		}
		
		return;
	}

    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bLeftRightSwitch)
    {
        DecoderLeftStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }
    else
    {
        DecoderRightStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止向左
void inline PtzLeftStop(U32 nCh)
{
	if (msoftptz_b_support(nCh))
	{
		msoftptz_move_stop(nCh);
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉低引脚
		//拉低引脚
		jv_gpio_write(0, 7, 0);
		jv_gpio_write(0, 6, 0);
		jv_gpio_write(2, 2, 0);
		jv_gpio_write(2, 4, 0);
		return;
	}
	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bLeftRightSwitch)
    {
        DecoderLeftStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderRightStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}
//开始向右
void inline PtzRightStart(U32 nCh, S32 nSpeed)
{
	if (msoftptz_b_support(nCh))
	{
		//画面的镜像只在左右运动时起作用
		if(__ismirror())
		{
			msoftptz_move_start(nCh, nSpeed,0 , 0);
		}else
		{
			msoftptz_move_start(nCh, -nSpeed,0 , 0);
		}
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉高引脚
		if (__ismirror())
		{
			jv_gpio_write(2, 2, 1);
		}
		else
		{
			jv_gpio_write(2, 4, 1);
		}
		
		return;
	}

    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bLeftRightSwitch)
    {
        DecoderRightStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }
    else
    {
        DecoderLeftStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止向右
void inline PtzRightStop(U32 nCh)
{
	if (msoftptz_b_support(nCh))
	{
		msoftptz_move_stop(nCh);
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉低引脚
		//拉低引脚
		jv_gpio_write(0, 7, 0);
		jv_gpio_write(0, 6, 0);
		jv_gpio_write(2, 2, 0);
		jv_gpio_write(2, 4, 0);
		return;
	}

    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bLeftRightSwitch)
    {
        DecoderRightStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderLeftStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}


//多方位移动
//bLeft 为真时左移，为假是右移，leftSpeed为0时不移动
//bUp 为真是上移，为假时下移，upSpeed为0时不移动
void PtzPanTiltStart(U32 nCh, BOOL bLeft, BOOL bUp, int leftSpeed, int upSpeed)
{
	if (msoftptz_b_support(nCh))
	{
		if (!bLeft)
			leftSpeed = -leftSpeed;
		if (!bUp)
			upSpeed = -upSpeed;
		msoftptz_move_start(nCh, leftSpeed, upSpeed, 0);
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		if (leftSpeed != 0)
		{
			if (bLeft)
			{
				jv_gpio_write(2, 2, 1);
			}
			else
			{
				jv_gpio_write(2, 4, 1);
			}
		}
		if (upSpeed != 0)
		{
			if (bUp)
			{
				jv_gpio_write(0, 7, 1);
			}
			else
			{
				jv_gpio_write(0, 6, 1);
			}
		}
			
		return;
	}
	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (pPtz->bLeftRightSwitch)
    {
    	bLeft = !bLeft;
    }
	DecoderPanTiltStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, bLeft, bUp, leftSpeed, upSpeed);

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}

void PtzPanTiltStop(U32 nCh)
{
	if (msoftptz_b_support(nCh))
	{
		msoftptz_move_stop(nCh);
		return ;
	}

	if (!strcmp(hwinfo.devName, "YL"))
	{
		//拉低引脚
		jv_gpio_write(0, 7, 0);
		jv_gpio_write(0, 6, 0);
		jv_gpio_write(2, 2, 0);
		jv_gpio_write(2, 4, 0);
		return;
	}
	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    DecoderPanTiltStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}


//开始光圈+
void inline PtzIrisOpenStart(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisOpenStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomInStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomOutStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
#if 1
    jv_iris_adjust(0, TRUE);
#endif
}
//停止光圈+
void inline PtzIrisOpenStop(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisOpenStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomInStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomOutStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
}
//开始光圈-
void inline PtzIrisCloseStart(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisCloseStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomOutStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomInStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
 #if 1
    jv_iris_adjust(0, FALSE);
#endif
}
//停止光圈-
void inline PtzIrisCloseStop(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisCloseStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomOutStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomInStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
}
//开始变焦+
void inline PtzFocusNearStart(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusNearStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomInStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomOutStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止变焦+
void inline PtzFocusNearStop(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusNearStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomInStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomOutStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}
//开始变焦-
void inline PtzFocusFarStart(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusFarStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomOutStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomInStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止变焦-
void inline PtzFocusFarStop(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (0 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusFarStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomOutStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomInStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}
//开始变倍-
void inline PtzZoomOutStart(U32 nCh)
{	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (1 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisOpenStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (1 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusNearStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomOutStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomInStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止变倍-
void inline PtzZoomOutStop(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (1 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisOpenStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (1 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusNearStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomOutStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomInStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}
//开始变倍+
void inline PtzZoomInStart(U32 nCh)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (1 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisCloseStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (1 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusFarStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomInStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomOutStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}
//停止变倍+
void inline PtzZoomInStop(U32 nCh)
{	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    if (1 == pPtz->bIrisZoomSwitch)
    {
        DecoderIrisCloseStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (1 == pPtz->bFocusZoomSwitch)
    {
        DecoderFocusFarStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else if (0 == pPtz->bZoomSwitch)
    {
        DecoderZoomInStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }
    else
    {
        DecoderZoomOutStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);
    }

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}

//辅助N 开始
//n 第n项辅助功能
void PtzAuxAutoOn(U32 nCh, int n)
{
	//取得互斥量
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderAUXNOn(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, n);

	//释放互斥量
	jv_rs485_unlock();
}

//辅助N 结束
//n 第n项辅助功能
void PtzAuxAutoOff(U32 nCh, int n)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
        DecoderAUXNOff(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, n);

    //释放互斥量
    jv_rs485_unlock();
}

//转到预置点
void inline PtzLocatePreset(U32 nCh, S32 nPreset)
{
    //取得互斥量
	int argv[2];
	int i;
	msensor_attr_t snAttr;
    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    //取得互斥量
    jv_rs485_lock();
    if((nPreset>0) && (nPreset< MAX_PRESET_CT))
    {
    	if ( pPtz->nProtocol==PTZ_PROTOCOL_SELF)
    	{
			argv[0] = s_PTZInfo.presetPos[nCh][nPreset].zoom;
			argv[1] = s_PTZInfo.presetPos[nCh][nPreset].focus;
    	}
    	if (msoftptz_b_support(nCh))
    	{
    		JVPTZ_Pos_t pos;
    		pos.left = s_PTZInfo.presetPos[nCh][nPreset].left;
    		pos.up = s_PTZInfo.presetPos[nCh][nPreset].up;
    		msoftptz_goto(nCh, &pos);
    	}
    }
    if(nPreset==300)
    {
    	msensor_getparam(&snAttr);
    	if((snAttr.effect_flag>>EFFECT_MIRROR)&0x01)
		{
    		snAttr.effect_flag&=(~(1<<EFFECT_MIRROR));
		}
    	else
    	{
      		snAttr.effect_flag|=(1<<EFFECT_MIRROR);
    	}
		msensor_setparam(&snAttr);
		 msensor_flush(0);
    }
    else  if(nPreset==301)
    {
    	msensor_getparam(&snAttr);
		if((snAttr.effect_flag>>EFFECT_TURN)&0x01)
		{
			snAttr.effect_flag&=(~(1<<EFFECT_TURN));
		}
		else
		{
			snAttr.effect_flag|=(1<<EFFECT_TURN);
		}
		 msensor_setparam(&snAttr);
		 msensor_flush(0);
    }
    else  if(nPreset==302)
    {
    	 CloseWatchDog();
    }
    else  if(nPreset==303)
    {
    	OpenWatchDog();
    }
	if(g_PtzStatus[nCh].bStartPatrol == TRUE)
	{
		PTZ_PATROL_INFO *patrol = PTZ_GetPatrol();
		DecoderLocatePreset(jv_rs485_get_fd(),
                pPtz->nAddr,
                pPtz->nProtocol,
                pPtz->nBaudRate,
                nPreset,
                (patrol[g_PtzStatus[nCh].nPatrolPath].nPatrolSpeed >> 2));
	}
	else
	{
		DecoderLocatePreset(jv_rs485_get_fd(),
                        pPtz->nAddr,
                        pPtz->nProtocol,
                        pPtz->nBaudRate,
                        nPreset,
                        0);
	}

    Printf("goto preset %d\n", nPreset);

    //释放互斥量
    jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);

}

//门磁报警转到预置点
void PtzAlarm_gotoPreset(U32 nCh,S32 nPreset)
{
	msoftptz_setalarmPreSpeed_flag();
	PtzLocatePreset(nCh,nPreset);
}

//自动
void inline PtzAutoStart(U32 nCh ,S32 nSpeed)
{
	if (msoftptz_b_support(nCh) && msoftptz_auto_support(nCh))
	{
		msoftptz_move_auto(nCh, nSpeed);
		return ;
	}

    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    DecoderAutoStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);

    //释放互斥量
    jv_rs485_unlock();

	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
}

//停止
void inline PtzAutoStop(U32 nCh)
{
	if (msoftptz_b_support(nCh) && msoftptz_auto_support(nCh))
	{
		msoftptz_move_stop(nCh);
		return ;
	}
	
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
    DecoderAutoStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

    //释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
}

/////////////////////////////////////巡航点链表操作////////////////////////////////////

//函数说明 : 把预置点添加到巡航队列中
//参数     : PPTZ pPtz:要添加到的云台结构
//           U32 nPreset:预置点
//返回值   : 0:成功; -1:到最大数量; -2:该预置点已经存在(可以添加重复预置点，去掉)
S32 AddPatrolNod(PTZ_PATROL_INFO *pPatrol, U32 nPreset, U32 nStayTime)
{
    if(pPatrol->nPatrolSize >= MAX_PATROL_NOD)
    {
        return -1;
    }

	pPatrol->aPatrol[pPatrol->nPatrolSize].nPreset = nPreset;
    if(nStayTime < 5)
        nStayTime = 5;
	pPatrol->aPatrol[pPatrol->nPatrolSize].nStayTime = nStayTime;
	pPatrol->nPatrolSize++;

    return 0;
}

//函数说明 : 把预置点从巡航队列中删除
//参数     : PPTZ pPtz:在该云台结构中查找预置点删除
//           U32 nIndex:删除的预置点在巡航数组中的下标
//返回值   : 0:成功; -1:失败
S32 DelPatrolNod(PTZ_PATROL_INFO *pPatrol, U32 nIndex)
{
    U32 i;

    if (0 >= pPatrol->nPatrolSize ||nIndex >= pPatrol->nPatrolSize)
    {
    	Printf("size(%d) or index(%d) error\n", pPatrol->nPatrolSize, nIndex);
    	return -1;
    }

    pPatrol->nPatrolSize--;
	//后面的预置点前移1
	for (i = nIndex; i<pPatrol->nPatrolSize; i++)
	{
		memcpy(&(pPatrol->aPatrol[i]), &(pPatrol->aPatrol[i+1]), sizeof(PATROL_NOD));
	}

	memset(&(pPatrol->aPatrol[i]), 0 , sizeof(PATROL_NOD));

    return 0;
}

//函数说明 : 把预置点添加到巡航队列中
//参数     : PPTZ pPtz:要修改的云台结构
//           U32 nOldPreset:要修改的预置点
//			U32 nNewPreset:修改成的值
//			U32 nNewStayTime:停留时间
//返回值   : 0:成功; -1:预置点不存在 -2：预置点重复
S32 ModifyPatrolNod(PTZ_PATROL_INFO *pPatrol, U32 nOldPreset, U32 nNewPreset, U32 nNewStayTime)
{
    U32 i;
    S32 index = -1;

    //修改值和待修改值一致，直接查找修改
    if (nOldPreset == nNewPreset)
    {
        for (i=0; i<pPatrol->nPatrolSize; i++)
        {
            if (pPatrol->aPatrol[i].nPreset == nOldPreset)
            {
            	pPatrol->aPatrol[i].nPreset = nNewPreset;
            	pPatrol->aPatrol[i].nStayTime = nNewStayTime;
        		return 0;
            }
        }
        return -1;
    }
    //在当前链表中找修改的点是否已经存在
	else
	{
		for (i = 0; i < pPatrol->nPatrolSize; i++)
		{
			if (pPatrol->aPatrol[i].nPreset == nNewPreset)
			{
				return -2;
			}
			else if (pPatrol->aPatrol[i].nPreset == nOldPreset)
			{
				index = i;
			}
		}
	    //修改
		if (0 <= index)
		{
			pPatrol->aPatrol[i].nPreset = nNewPreset;
			pPatrol->aPatrol[i].nStayTime = nNewStayTime;
		    return 0;
		}
	    return -1;
	}
	return 0;
}

//开始录制轨迹
S32 PtzTrailStartRec(U32 nCh, U32 nTrail)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderSetScanOnPreset(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nTrail);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//停止录制轨迹
S32 PtzTrailStopRec(U32 nCh, U32 nTrail)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderSetScanOffPreset(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nTrail);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
	return 0;
}

//开始轨迹
S32 PtzTrailStart(U32 nCh, U32 nTrail)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderLocateScanPreset(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nTrail);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//停止轨迹
S32 PtzTrailStop(U32 nCh, U32 nTrail)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderStopScanPreset(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nTrail);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
	return 0;
}

//启用或者停止守望
S32 PtzGuardSet(U32 nCh, PTZ_GUARD_T *guard)
{
	__Ptz_StopPatrol(nCh);
	memcpy(&g_PtzStatus[nCh].pGuardInf, guard, sizeof(PTZ_GUARD_T));
	DEBUG_printf("PtzGuardSet: time=%d, preset:%d, type:%d\n",g_PtzStatus[nCh].pGuardInf.guardTime,
		g_PtzStatus[nCh].pGuardInf.nRreset, g_PtzStatus[nCh].pGuardInf.guardType);
	__Ptz_Guard_Reset(nCh);
	return 0;
}

//设置线扫左边界
S32 PtzLimitScanLeft(U32 nCh)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderSetLeftLimitPosition(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

	//释放互斥量
	jv_rs485_unlock();
	
	__Ptz_StopPatrol(nCh);
	return 0;
}

//设置线扫右边界
S32 PtzLimitScanRight(U32 nCh)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderSetRightLimitPosition(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

	//释放互斥量
	jv_rs485_unlock();
	
	__Ptz_StopPatrol(nCh);
	return 0;
}

//设置线扫速度
S32 PtzLimitScanSpeed(U32 nCh, int nScan, int nSpeed)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	pPtz->scanSpeed = nSpeed;
	DecoderSetLimitScanSpeed(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nScan, nSpeed);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//开始线扫
S32 PtzLimitScanStart(U32 nCh, int nScan)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderLimitScanStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nScan);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//设置线扫上边界
S32 PtzLimitScanUp(U32 nCh)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderSetUpLimitPosition(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

	//释放互斥量
	jv_rs485_unlock();
	
	__Ptz_StopPatrol(nCh);
	return 0;
}

//设置线扫下边界
S32 PtzLimitScanDown(U32 nCh)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderSetDownLimitPosition(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

	//释放互斥量
	jv_rs485_unlock();
	
	__Ptz_StopPatrol(nCh);
	return 0;
}

//开始垂直扫描
S32 PtzVertScanStart(U32 nCh, int nScan)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderVertScanStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nScan);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//停止线扫
S32 PtzLimitScanStop(U32 nCh, int nScan)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderLimitScanStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nScan);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
	return 0;
}

//开始随机扫描
S32 PtzRandomScanStart(U32 nCh, int nSpeed)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	pPtz->scanSpeed = nSpeed;
	DecoderRandomScanStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//开始帧扫描
S32 PtzFrameScanStart(U32 nCh, int nSpeed)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	pPtz->scanSpeed = nSpeed;
	DecoderFrameScanStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//开始花样扫描
S32 PtzWaveScanStart(U32 nCh, int nSpeed)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	pPtz->scanSpeed = nSpeed;
	DecoderWaveScanStart(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, nSpeed);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Pause(nCh);
	__Ptz_StopPatrol(nCh);
	return 0;
}

//结束花样扫描
S32 PtzWaveScanStop(U32 nCh, int nSpeed)
{
	jv_rs485_lock();

	PPTZ pPtz = g_PtzStatus[nCh].pPtz;
	DecoderWaveScanStop(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate);

	//释放互斥量
	jv_rs485_unlock();
	__Ptz_Guard_Reset(nCh);
	return 0;
}

//函数说明 : 把波特率转换字符串
//参数     : HI_U32 nBaudrate:波特率
//返回值   : COMBO的索引 ; -1:失败
char *Ptz_BaudrateToStr(U32 nBaudrate)
{
    switch(nBaudrate)
    {
    case B1200:
        return "1200";

    case B4800:
        return "4800";

    case B9600:
        return "9600";

    case B2400:
        return "2400";
	case B19200 :
        return "19200";
	case B38400:
	     return "38400";
	case B57600:
	     return "57600";
	case B115200:
	     return "115200";
     default:
        return "2400";
    }
}

//函数说明 : 字符串转换成波特率
//参数     : HI_U32 nIndex :COMBO的索引
//返回值   : 波特率 ; -1:失败
U32 Ptz_StrToBaudrate(char *cStr)
{
    int nBaud = atoi(cStr);
    switch(nBaud)
    {
    case 1200:
        return B1200;

    case 2400:
        return B2400;

    case 4800:
        return B4800;

    case 9600:
        return B9600;
 	case 19200 :
        return B19200;
	case 38400:
	     return B38400;
	case 57600:
	     return B57600;
	case 115200:
	     return B115200;

    default:
        return B2400;
    }
}

//函数说明 : 设置波特率时,把波特率转换成COMBO的索引号
//参数     : U32 nBaudrate:波特率
//返回值   : COMBO的索引 ; -1:失败
S32 Ptz_BaudrateToIndex(U32 nBaudrate)
{
    switch(nBaudrate)
    {
    case B1200:
        return 0;

    case B2400:
        return 1;

    case B4800:
        return 2;

    case B9600:
        return 3;
    default:
        return 0;   //zwq20100531
    }
}

//函数说明 : 设置波特率时,把COMBO的索引号转换成波特率
//参数     : U32 nIndex :COMBO的索引
//返回值   : 波特率 ; -1:失败
S32 Ptz_IndexToBaudrate(U32 nIndex)
{
    switch(nIndex)
    {
    case 0:
        return B1200;

    case 1:
        return B2400;

    case 2:
        return B4800;

    case 3:
        return B9600;

    default:
        return -1;
    }
}

//函数说明 : 设置协议时,把协议转换成COMBO的索引号
//参数     : U32 nProtocol:libPTZ.a中的协议编号
//返回值   : COMBO的索引 ; -1:失败
S32 Ptz_ProtocolToIndex(U32 nProtocol)
{
    switch(nProtocol)
    {
    case 1: //PELCO-D
        return 0;

    case 4: //PELCO-P
        return 1;

    case 7: //PELCO-D 扩展
        return 2;

    case 5: //PELCO-P 扩展
        return 3;

    case 13: //PELCOD-3
        return 4;

    default:
        return 0;
    }
}

//函数说明 : 设置协议时,把COMBO的索引号转换成libPTZ.a中的协议编号
//参数     : U32 nIndex :COMBO的索引
//返回值   : 协议编号 ; -1:失败
S32 Ptz_IndexToProtocol(U32 nIndex)
{
    switch(nIndex)
    {
    case 0: //PELCO-D
        return 1;

    case 1: //PELCO-P
        return 4;

    case 2: //PELCO-D 扩展
        return 7;

    case 3: //PELCO-P 扩展
        return 5;

    case 4: //PELCOD-3
        return 13;

    default:
        return -1;
    }
}

//将某一点在屏幕中间放大显示
//x,y  将屏幕分成64x64个区域。x,y分别代表着各自方向的数值
//zoom 放大的倍数。其值为实际倍数x16 - 16。不变倍为0. 例如放大5倍：5 x 16 - 16
void PtzZoomPosition(U32 nCh, int x, int y, int zoom)
{
    //取得互斥量
    jv_rs485_lock();

    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
        DecoderZoomPosition(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, x, y, zoom);

    //释放互斥量
    jv_rs485_unlock();
}

//客户端圈定区域，进行3D定位，并放大缩小显示
//(x, y, w, h) 圈定区域中心坐标及宽高; (width, height)当前码流分辨率
//zoom 3D定位指令: 0xC0 放大，0xC1 缩小。
void PtzZoomZone(U32 nCh, ZONE_INFO *zone, int width, int height, int zoom)
{
	if(__isturn())
	{
		zone->y = 0 - zone->y;
	}
	if(__ismirror())
	{
		zone->x = 0 - zone->x;
	}

	if (msoftptz_b_support(nCh))
	{

		msoftptz_ZoomZone(zone->x, zone->y, zone->w, zone->h, width, height, zoom);
		return;
	}
	
	if(!hwinfo.bHomeIPC)
	{
		//取得互斥量
		jv_rs485_lock();
		PPTZ pPtz = g_PtzStatus[nCh].pPtz;
		DecoderZoomZone(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, 
			zone->x, zone->y, zone->w, zone->h, width, height, zoom);

		//释放互斥量
		jv_rs485_unlock();
	}
}

//移动跟踪指令
//(x, y, zoom, focus) 请求或设置云台移动坐标步长及参与计算的聚焦步长
//cmd 移动跟踪指令: 0x20 设置0点，0x21 设置位置移动步长，0x22 请求当前位置步长
void PtzTraceObj(U32 nCh, TRACE_INFO *trace, int cmd)
{
    //取得互斥量
    jv_rs485_lock();
	if(__isturn())
	{
		trace->y = 0 - trace->y;
	}
	if(__ismirror())
	{
		trace->x = 0 - trace->x;
	}
    PPTZ pPtz = g_PtzStatus[nCh].pPtz;
        DecoderTraceObj(jv_rs485_get_fd(), pPtz->nAddr, pPtz->nProtocol, pPtz->nBaudRate, 
			trace->x, trace->y, trace->zoom, trace->focus, cmd);

    //释放互斥量
    jv_rs485_unlock();
}

//lyf 2014.02.20

//注意:这些参数适用于N85-HD高速球;
//若为其它型号的高速球，需要重新修改宏定义。

#define STAY_TIME			5	//测试人员要求的是30s
#define LEFT_RIGHT_SPEED	100
#define UP_DOWN_SPEED		255
#define PRESET_COUNT		6

static int presets_patrol_timer = -1;
static int stop_flag = 0;	//以外终止批量设置、一键巡航标志位


//函数说明 :utl_timer_create 回调函数
//参数     : 
//返回值   : 

static BOOL _addpresets_startpatrol(int id, void* param)
{
	static int nCh = 0;
	static int nPreset = 1;
	static int in = 0;
	static int inSum = 0;
	static int upflag = 1;
	static PTZ_PATROL_INFO *infoPatrol;
	int ret = 0;
	JVPTZ_Pos_t pos;
	infoPatrol = PTZ_GetPatrol();
	
	//PRESET_COUNT个预置点没有添加完就按下了停止按钮时stop_flag = 1
	if(stop_flag == 1)
	{
		utl_timer_destroy(presets_patrol_timer);
		//重置变量
		in = 0;
		inSum = 0;
		nPreset = 1;
		upflag = 1;
		stop_flag = 0;
		return TRUE;
	}
	
	in++;
	inSum++;
	
	//共6个预置点
	if(inSum > PRESET_COUNT * 4)
	{
		PTZ_StartPatrol(nCh, 0);
		utl_timer_destroy(presets_patrol_timer);
		//重置变量
		in = 0;
		inSum = 0;
		nPreset = 1;
		upflag = 1;	
		return TRUE;
	}
		
	if(in == 1)
	{		
		if(inSum==1)
		{
		
		PtzPanTiltStart(nCh, 1, 1, LEFT_RIGHT_SPEED, UP_DOWN_SPEED);
		pos.left = 32767;
    	pos.up = 65535;
		
		msoftptz_goto(nCh, &pos);
		}
		else if(inSum==5)
		{
		pos.left = 65535;
    	pos.up = 65535;
		msoftptz_goto(nCh, &pos);
		}	
		else if(inSum==9)
		{
		pos.left = 32767;
    	pos.up = 65535;
		msoftptz_goto(nCh, &pos);
		}
		else if(inSum==13)
		{
		pos.left = 0;
    	pos.up = 65535;
		msoftptz_goto(nCh, &pos);
		}
		else if(inSum==17)
		{
		pos.left = 32767;
    	pos.up = 65535;
		msoftptz_goto(nCh, &pos);
		}
		else if(inSum==21)
		{
		pos.left =32767;
    	pos.up = 0;
		msoftptz_goto(nCh, &pos);
		}
		
	}
	else if(in == 3)
	{
		PtzPanTiltStop(nCh);
		if(upflag == 0)
		{
			PtzZoomInStart(nCh);
		}
		else
		{
			PtzZoomOutStart(nCh);
		}
	}
	else if(in == 4)
	{
		if(upflag == 0)
		{
			PtzZoomInStop(nCh);
		}
		else
		{
			PtzZoomOutStop(nCh);
		}
		upflag = !upflag;
		
		ret = PTZ_AddPreset(nCh, nPreset, "Preset");
		WriteConfigInfo();
		if (ret != 0)
		{
			printf("PTZ_AddPreset Failed: %d\n", ret);
			return FALSE;
		}
		
		ret = AddPatrolNod(&infoPatrol[0], nPreset, STAY_TIME);
		if(ret != 0)
		{
			printf("AddPatrolNod Failed: %d\n", ret);
			return FALSE;
		}
		WriteConfigInfo();
		nPreset++;
		
	}

	if(in == 4)
	{	
		in = 0;
	}
	utl_timer_reset(presets_patrol_timer, 6000, _addpresets_startpatrol, NULL);

	return TRUE;
	
}

static BOOL __addpresets_startpatrol_af(int id, void* param)
{
	static int nCh = 0;
	static int nPreset = 1;
	static int in = 0;
	static int inSum = 0;
	static int upflag = 1;
	static PTZ_PATROL_INFO *infoPatrol;
	int ret = 0;
	JVPTZ_Pos_t pos;
	infoPatrol = PTZ_GetPatrol();
	
	//PRESET_COUNT个预置点没有添加完就按下了停止按钮时stop_flag = 1
	if(stop_flag == 1)
	{
		utl_timer_destroy(presets_patrol_timer);
		//重置变量
		in = 0;
		inSum = 0;
		nPreset = 1;
		upflag = 1;
		stop_flag = 0;
		return TRUE;
	}
	
	in++;
	inSum++;
	
	//共6个预置点
	if(inSum > PRESET_COUNT * 4)
	{
		PTZ_StartPatrol(nCh, 0);
		utl_timer_destroy(presets_patrol_timer);
		//重置变量
		in = 0;
		inSum = 0;
		nPreset = 1;
		upflag = 1;	
		return TRUE;
	}

	if(in == 1)
	{		
		if(inSum==1)
		{
			PtzPanTiltStart(nCh, 0, 1, 0, 150);
		}
		else if(inSum==5)
		{
			PtzPanTiltStart(nCh, 1, 0, 150, 0);
		}	
		else if(inSum==9)
		{
			PtzPanTiltStart(nCh, 1, 0, 0, 150);
		}
		else if(inSum==13)
		{
			PtzPanTiltStart(nCh, 0, 1, 150, 0);
		}
		else if(inSum==17)
		{
			PtzPanTiltStart(nCh, 0, 1, 150, 150);
		}
		else if(inSum==21)
		{
			PtzPanTiltStart(nCh, 1, 0, 150, 150);
		}
		
	}
	else if(in == 3)
	{
		PtzPanTiltStop(nCh);
	}
	else if(in == 4)
	{
		upflag = !upflag;
		char name[16] = {0};
		sprintf(name, "Preset-%d", nPreset);
		ret = PTZ_AddPreset(nCh, nPreset, name);
		if (ret != 0)
		{
			printf("PTZ_AddPreset Failed: %d\n", ret);
			return FALSE;
		}
		WriteConfigInfo();
		
		ret = AddPatrolNod(&infoPatrol[0], nPreset, 12);
		if(ret != 0)
		{
			printf("AddPatrolNod Failed: %d\n", ret);
			return FALSE;
		}
		WriteConfigInfo();
		nPreset++;
		
	}

	if(in == 4)
	{	
		in = 0;
	}
	utl_timer_reset(presets_patrol_timer, 4000, __addpresets_startpatrol_af, NULL);

	return TRUE;
	
}

//函数说明 : 一键批量设置预置点、巡航函数
//参数     :
//返回值   : 

void PTZ_PresetsPatrolStart()
{
	int i = 0;
	int ret = 0;
	int nCh = 0;
	
	//批量设置预置点、巡航前，先清空巡航、预置点
	
	//删除巡航中的预置点
	PTZ_PATROL_INFO *infoPatrol = &g_PtzStatus[nCh].pPatrolInf[0];

	for(i = infoPatrol->nPatrolSize - 1; i >=0; i--)
	{
		DelPatrolNod(infoPatrol, i);
	}

	//删除预置点
	PTZ_PRESET_INFO *infoPreset = g_PtzStatus[nCh].pPreInf;

	for(i = infoPreset->nPresetCt - 1; i >= 0 ; i--)
	{
		ret = PTZ_DelPreset(nCh, infoPreset->pPreset[i]);
		if(ret != 0)
		{
			printf("delete preset error occured!\n");
			return ;		
		}
		WriteConfigInfo();			
	}
	{
		if (presets_patrol_timer == -1)
		{
			presets_patrol_timer = utl_timer_create("pp_timer", 2000, _addpresets_startpatrol, NULL);
		}
		else
		{
			utl_timer_reset(presets_patrol_timer, 1000, _addpresets_startpatrol, NULL);
		}
	}
		
}

//函数说明 : 一键批量删除预置点、退出巡航函数
//参数     :
//返回值   : 

void PTZ_PresetsPatrolStop()
{
	int i = 0;
	int ret = 0;
	int nCh = 0;
	
	stop_flag = 1;
	
	PTZ_StopPatrol(nCh);
	//PtzPanTiltStart(nCh, 1, 1, 3, 0);
	//删除巡航中的预置点
	PTZ_PATROL_INFO *infoPatrol = &g_PtzStatus[nCh].pPatrolInf[0];
	for(i = infoPatrol->nPatrolSize - 1; i >=0; i--)
	{
		DelPatrolNod(&infoPatrol[nCh], i);
		WriteConfigInfo();
	}
	
	//删除预置点
	PTZ_PRESET_INFO *infoPreset = g_PtzStatus[nCh].pPreInf;
	for(i = infoPreset->nPresetCt - 1; i >= 0 ; i--)
	{
		ret = PTZ_DelPreset(nCh, infoPreset->pPreset[i]);
		if(ret != 0)
		{
			printf("PTZ_AddPreset Failed: %d\n", ret);
			return;
		}	
		WriteConfigInfo();
	}

	sleep(2);
}

//函数说明 : 设定开机启动项
//参数     :
//返回值   : 


void PTZ_SetBootConfigItem(U32 nCh, U32 item)
{
	g_PtzStatus[nCh].bootItem = item;
}

//函数说明 : 获取开机启动项
//参数     :
//返回值   : 

U32 PTZ_GetBootConfigItem()
{
	int nCh = 0;
	return g_PtzStatus[nCh].bootItem;
}
/**
 * @brief   PTZ启动开机启动项,扫描轨迹要延迟到云台自检之后
 * @param pArgs
 * @return
 */
void PTZ_BootThrd(VOID *pArgs)
{
	pthreadinfo_add((char *)__func__);

	int nCh = 0;
	U32 item = PTZ_GetBootConfigItem();

	switch(item)
	{
		case 1:
			PTZ_StartPatrol(nCh, 0);
			break;
		case 2:
			usleep(30*1000*1000);
			PtzLimitScanStart(nCh, 0);
			break;
		case 3:
			usleep(30*1000*1000);
			PtzTrailStart(nCh, g_PtzStatus[0].pGuardInf.nTrail);
			break;
		case 4:
			PtzGuardSet(nCh, &g_PtzStatus[0].pGuardInf);
			break;
		case 5:
			usleep(30*1000*1000);
			PtzWaveScanStart(nCh, g_PtzStatus[0].pPtz->scanSpeed);
			break;
		default:
			break;
	}
}

//函数说明 : 启动开机启动项
//参数     :
//返回值   : 

void PTZ_StartBootConfigItem()
{
	pthread_t		pidptz;
	pthread_attr_t	attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	//设置为分离线程
	pthread_create(&pidptz, &attr, (void*)PTZ_BootThrd, NULL);
	pthread_attr_destroy (&attr);

}


void _start_Schedule(PTZ_SCHEDULE_INFO *schedule, int nSch)
{
	int nCh = 0;
	DEBUG_printf("Sch[%d]	", nSch);
	
	if((sch_status.sch_LastItem!=schedule->schedule[nSch]) || (sch_status.sch_LastSch!=nSch))
	{
		DEBUG_printf(":%d	start\n", schedule->schedule[nSch]);
		switch(schedule->schedule[nSch])
		{
			case 0:
			case 1:
				PTZ_StartPatrol(nCh, (schedule->schedule[nSch]));
				break;
			case 2:
			{
				PtzLimitScanSpeed(nCh, 0, 55);
				PtzLimitScanStart(nCh, 0);
			}
				break;
			case 3:
			case 4:
			case 5:
			case 6:
				PtzTrailStart(nCh, (schedule->schedule[nSch]-3));
				break;
			case 7:
			{
				PTZ_GUARD_T *guardinfo = PTZ_GetGuard();
				PTZ_GUARD_T guard;
				guard.guardTime = guardinfo->guardTime;
				guard.nRreset = guardinfo->nRreset;
				guard.guardType = GUARD_PRESET;
				DEBUG_printf(" time:%d	type:%d	 preset:%d\n",guard.guardTime,guard.guardType, guard.nRreset);
				PtzGuardSet(nCh, &guard);
			}
				break;
			default:
				break;
		}
		sch_status.sch_LastSch = nSch;
		sch_status.sch_LastItem = schedule->schedule[nSch];
		//sch_status.sch_runflag[nSch] = TRUE;
		//sch_status.sch_runflag[1-nSch] = FALSE;
	}
	else
		DEBUG_printf(":%d	runing\n", schedule->schedule[nSch]);	
}
void _stop_Schedule(void)
{
	int nCh = 0;
	PTZ_GUARD_T guard;
	if(sch_status.sch_LastItem < 0)
	{
		DEBUG_printf("idle\n");
	}
	else if(sch_status.sch_LastItem < 2)
	{
		DEBUG_printf("stop patrol\n");
		PTZ_StopPatrol(nCh);
	}
	else if(sch_status.sch_LastItem < 3)
	{
		DEBUG_printf("stop scan\n");
		PtzLimitScanStop(nCh, 0);
	}
	else if(sch_status.sch_LastItem < 7)
	{
		DEBUG_printf("stop trail\n");
		PtzTrailStop(nCh, (sch_status.sch_LastItem-3));
	}
	else if(sch_status.sch_LastItem < 8)
	{
		DEBUG_printf("stop guard\n");
		guard.guardTime = 0;
		guard.guardType = GUARD_NO;
		PtzGuardSet(nCh, &guard);
	}
	else
		printf("unknown plan\n");
	sch_status.sch_LastItem = -1;
	sch_status.sch_LastSch = -1;
	//sch_status.sch_runflag[0] = FALSE;
	//sch_status.sch_runflag[1] = FALSE;
}
void PTZ_ScheduleThrd(VOID *pArgs)
{	
	sleep(10);
	int nCh = 0;
	static BOOL running = TRUE;
	PTZ_SCHEDULE_INFO oldSch;
	PTZ_GUARD_T guard;
	pthreadinfo_add((char *)__func__);


	while(running)
	{
		if(memcmp(PTZ_GetSchedule(), &oldSch, sizeof(PTZ_SCHEDULE_INFO)) != 0)
		{
			printf("##Schange\n");
			memcpy(&oldSch, PTZ_GetSchedule(), sizeof(PTZ_SCHEDULE_INFO));
		}
		else
		{
			DEBUG_printf("Nochange:%d	",sch_status.sch_LastItem);
		}
		if((oldSch.bSchEn[0]==FALSE) && (oldSch.bSchEn[1]==FALSE))
		{
			DEBUG_printf("no plan	");
			_stop_Schedule();
			usleep(500*1000);
			continue;
		}
		time_t tt;
		struct tm tm;
		tt = time(NULL);
		localtime_r(&tt, &tm);
		int now = tm.tm_hour*60 + tm.tm_min;
		int start1 = oldSch.schTimeStart[0].hour*60 + oldSch.schTimeStart[0].minute;
		int end1 = oldSch.schTimeEnd[0].hour*60 + oldSch.schTimeEnd[0].minute;
		int start2 = oldSch.schTimeStart[1].hour*60 + oldSch.schTimeStart[1].minute;
		int end2 = oldSch.schTimeEnd[1].hour*60 + oldSch.schTimeEnd[1].minute;
		DEBUG_printf("now[%02d:%02d]   Plan1:[%02d:%02d-%02d:%02d]  Plan2:[%02d:%02d-%02d:%02d]	", tm.tm_hour, tm.tm_min,oldSch.schTimeStart[0].hour,
			oldSch.schTimeStart[0].minute, oldSch.schTimeEnd[0].hour,oldSch.schTimeEnd[0].minute,oldSch.schTimeStart[1].hour,oldSch.schTimeStart[1].minute, 
			oldSch.schTimeEnd[1].hour,oldSch.schTimeEnd[1].minute);
		if(oldSch.bSchEn[0] == TRUE)
		{
			if(oldSch.bSchEn[1] == TRUE)
			{
				DEBUG_printf("Two:");
				if(start1 < end1)
				{
					if(start2 < end2)
					{
						if((now>=start1) && (now<end1))
						{
							DEBUG_printf("1	Day	");	
							_start_Schedule(&oldSch, 0);
						}
						else if((now>=start2) && (now<end2))
						{
							DEBUG_printf("2	Day	");
							_start_Schedule(&oldSch, 1);
						}
						else
						{
							DEBUG_printf("OUT	");
							_stop_Schedule();
						}
					}
					else
					{
						if((now>=start1) && (now<end1))
						{
							DEBUG_printf("1	Day	");	
							_start_Schedule(&oldSch, 0);
						}
						else if((now>=start2) || (now<end2))
						{
							DEBUG_printf("2	Net	");	
							_start_Schedule(&oldSch, 1);
						}
						else
						{
							DEBUG_printf("OUT	");
							_stop_Schedule();
						}
					}
				}
				else
				{
					if((now>=start1) || (now<end1))
					{
						DEBUG_printf("1	Net	");	
						_start_Schedule(&oldSch, 0);
					}
					else if((now>=start2) && (now<end2))
					{
						DEBUG_printf("2	Day	");	
						_start_Schedule(&oldSch, 1);
					}
					else
					{
						DEBUG_printf("all OUT");
						_stop_Schedule();
					}					
				}
				
			}
			else
			{
				DEBUG_printf("One:1	");
				if(start1 < end1)
				{
					if((now>=start1) && (now<end1))
					{
						DEBUG_printf("Day	");	
						_start_Schedule(&oldSch, 0);
					}
					else
					{
						DEBUG_printf("OUT	");
						_stop_Schedule();
					}
				}
				else
				{
					if((now>=start1) || (now<end1))
					{
						DEBUG_printf("Net	");
						_start_Schedule(&oldSch, 0);
					}
					else
					{
						DEBUG_printf("OUT	");
						_stop_Schedule();
					}
				}
			}
		}
		else
		{
			DEBUG_printf("One:2	");
			if(start2 < end2)
			{	
				if((now>=start2) && (now<end2))
				{
					DEBUG_printf("Day	");	
					_start_Schedule(&oldSch, 1);
				}
				else
				{
					DEBUG_printf("OUT	");
					_stop_Schedule();
				}
			}
			else
			{
				if((now>=start2) || (now<end2))
				{
					DEBUG_printf("Net	");
					_start_Schedule(&oldSch, 1);
				}
				else
				{
					DEBUG_printf("OUT	");
					_stop_Schedule();
				}
			}
		}
		usleep(500*1000);
	}
}
//函数说明 : 启动云台定时计划
//参数     :
//返回值   : 
void PTZ_StartSchedule()
{
	sch_status.sch_LastItem = -1;
	sch_status.sch_LastSch = -1;
	pthread_mutex_init(&guardmutex, NULL);
	pthread_t pidSch;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&pidSch, &attr, (void *)PTZ_ScheduleThrd, NULL);
	pthread_attr_destroy(&attr);
}

#include <utl_cmd.h>



int ptz_main(int argc, char *argv[])
{
	printf("ptz cmd, argv1: %s\n", argv[1]);
	if (strcmp(argv[1], "left") == 0)
	{
		printf("PtzLeftStart\n");
		PtzLeftStart(0, 100);
	}
	else if (strcmp(argv[1], "right") == 0)
	{
		printf("PtzRightStart\n");
		PtzRightStart(0, 100);
	}
	else if (strcmp(argv[1], "up") == 0)
	{
		printf("PtzUpStart\n");
		PtzUpStart(0, 100);
	}
	else if (strcmp(argv[1], "down") == 0)
	{
		printf("PtzDownStart\n");
		PtzDownStart(0, 100);
	}
	else if (strcmp(argv[1], "stop") == 0)
	{
		printf("PtzLeftStop\n");
		PtzLeftStop(0);
	}
	else if (strcmp(argv[1], "preset") == 0)
	{
		unsigned int preset = atoi(argv[2]);
		printf("PTZ_AddPreset: %d\n", preset);
		PTZ_AddPreset(0, preset, "argv[2]");
	}
	else if (strcmp(argv[1], "locatePreset") == 0)
	{
		unsigned int preset = atoi(argv[2]);
		printf("PtzLocatePreset: %d\n", preset);
		PtzLocatePreset(0, preset);
	}
	else if (strcmp(argv[1], "zoom") == 0)
	{
		int x,y,z;
		x = atoi(argv[2]);
		y = atoi(argv[3]);
		z = atoi(argv[4]);
		//x = x * 64 / 1280;
		//y = y * 64 / 720;
		//z = z * 16 - 16;
		PtzZoomPosition(0, x, y, z);
	}
	else if (strcmp(argv[1], "aux") == 0)
	{
		int value = atoi(argv[2]);
		if (value > 0)
			PtzAuxAutoOn(0, value);
		else
			PtzAuxAutoOff(0, 0-value);
	}
	else if (strcmp(argv[1], "sensor") == 0)
	{
		int value = atoi(argv[2]);
		//jv_sensor_wdt_weight(value);
	}
	else if (strcmp(argv[1], "dropon") == 0)
	{
		PtzAuxAutoOn(0, 0x2F);
	}
	else if (strcmp(argv[1], "dropoff") == 0)
	{
		PtzAuxAutoOff(0, 0x2F);
	}
	else
	{
		PtzZoomInStart(0);
	}
	return 0;
}

int ptz_test_init() __attribute__((constructor)) ;

int ptz_test_init()
{
	char *helper = "\tcapture\n";
	utl_cmd_insert("mptz", "mptz zoom [X] [Y] [ZOOM]", helper, ptz_main);
	return 0;
}

