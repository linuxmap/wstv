/*
 * mivp.c
 *
 *  Created on: 2014-11-7
 *      Author: Administrator
 */
#include "jv_common.h"
#include "mdetect.h"
#include "mstream.h"
#include "msnapshot.h"
#include "malarmout.h"
#include "msensor.h"

#include "mrecord.h"
#include "mlog.h"
#include "mgb28181.h"
#include "SYSFuncs.h"
#include "mtransmit.h"
#include "mipcinfo.h"

#include "utl_timer.h"
#include "mlog.h"
#include "mcloud.h"

#include "mivp.h"
#include "utl_iconv.h"
#include "utl_filecfg.h"
#include "utl_splice.h"
#include "mosd.h"
#include "ks_api.h"
#include "mptz.h"
#include "ivp.h"
#include "ocl.h"
#include "mfirmup.h"
#include <utl_cmd.h>
#include "mmva.h"
#include "MRemoteCfg.h"
#include "mbizclient.h"
#include "mdevclient.h"

#define IVP_COUNT_FILE "/etc/conf.d/jovision/ivpCount.cfg"
MIVP_t mivplist[MAX_STREAM];
static alarming_ivp_callback_t callback_ptr = NULL;
static alarming_ivp_callback_t callback_ptr_vr = NULL;
static alarming_ivp_callback_t callback_ptr_hide = NULL;
static alarming_ivp_callback_t callback_ptr_left = NULL;
static alarming_ivp_callback_t callback_ptr_removed = NULL;
static alarming_ivp_callback_t callback_ptr_cde = NULL;
static alarming_ivp_callback_t callback_ptr_ocl = NULL;
static alarming_ivp_callback_t callback_ptr_fm = NULL;
static alarming_ivp_callback_t callback_ptr_hover = NULL;
static alarming_ivp_callback_t callback_ptr_fire = NULL;
static alarming_ivp_callback_t callback_ptr_vf = NULL;
static alarming_ivp_callback_t callback_ptr_sc = NULL;

IVPHandle mivphandle=NULL;
IVPHandle mivphandle_cde=NULL;
IVPHandle mivphandle_vf=NULL;
IVPHandle mivphandle_hm=NULL;
IVPHandle mivphandle_lpr=NULL;
IVPHandle mivphandle_sc=NULL;

#ifdef  XW_MMVA_SUPPORT
//MIVP_t *mivplist_count = &mivplist[0];//明明可以共用，想不通为啥要弄两个不同的结构体,
#define mivplist_count mivplist
#else
MIVP_t mivplist_count[MAX_STREAM];
#endif
IVPHandle mivphandle_count=NULL;

	
static int IVP_Count_In ,IVP_Count_Out;
static int mivp_count_timer = -1;
static int IVP_Release_State;			//关闭智能分析时报警状态
int bIVP_COUNT_SUPPORT = 1;

static void __build_point_vi(int xi,int yi,int *xo, int *yo,const int VI_W,const int VI_H,const int viw, const int vih)
{
	int expw, exph; //锁定比例时，期望的宽度和高度，以压缩的宽高为基准
	expw = VI_H * viw / vih;
	exph = VI_W * vih / viw;
	int X1,Y1,X2,Y2;//取值范围，超出按照最小计算


	if (expw - VI_W >= 32)
	{
		X1 = 0;
		Y1 = (VI_H - exph)/2;
		X2 = X1 + expw;
		Y2 = Y1 + exph;
		xi = xi<X1?X1:(xi>X2?X2:xi);
		yi = yi<Y1?Y1:(yi>Y2?Y2:yi);
		int h = abs(yi-VI_H/2)*VI_H/exph;
		if(	yi < (VI_H/2))
			*yo = VI_H/2-h;
		else
			*yo = VI_H/2+h;
		*xo = xi;
	}
	else if (exph - VI_H > 10)
	{
		X1 = (VI_W-expw)/2;
		Y1 = 0;
		X2 = X1 + expw;
		Y2 = Y1 + exph;
		xi = xi < X1 ? X1 : (xi > X2 ? X2 : xi);
		yi = yi < Y1 ? Y1 : (yi > Y2 ? Y2 : yi);
		int w = abs(xi - VI_W / 2) * VI_W / expw;
		if (xi < (VI_W / 2))
			*xo = VI_W / 2 - w;
		else
			*xo = VI_W / 2 + w;
		*yo = yi;
	}
	else
	{
		*yo = yi;
		*xo = xi;
	}
//	printf(".........................VIW:%d,VIH:%d\n",VI_W,VI_H);
//	printf(".........................viW:%d,viH:%d\n",viw,vih);
//	printf(".........................xi:%d,yi:%d\n",xi,yi);
//	printf(".........................xo:%d,yo:%d\n",*xo,*yo);

}

/*
 * 不发送时间的时候可以time=NULL
 */
static int __ivp_send_to_nvr(char *time,unsigned int in,unsigned int out)
{
	char Data[64] ={ 0 };
	Data[0] = 'I';
	Data[1] = 'V';
	Data[2] = 'P';
	Data[3] = 'C';
	Data[4] = 'O';
	Data[5] = 'U';
	Data[6] = 'N';
	Data[7] = 'T';
	char sendstr[64];
	if (time)
		sprintf(sendstr, "%s=count_in:%d,count_out:%u", time, in, out);
	else
		sprintf(sendstr, "count_in:%d,count_out:%u", in, out);
	memcpy(Data + 8, sendstr, 64 - 8);
	printf("send to nvr:%s\n", Data);

#ifdef YST_SVR_SUPPORT
	remotecfg_send_self_data(0, Data, 64);
#endif
	return 0;
}

//人数统计单独使用一个回调函数，因为这个不需要报警
static void __IvpAlarmCount(void *ivp, int areaIndex, _UINT32 type, Z_LIST *areas)
{
	if (type == e_IPV_ALARM_TYPE_COUNT)
	{
		int count_in, count_out;
		count_out = areaIndex & 0xFFFF;
		count_in = (areaIndex >> 16) & 0xFFFF;
		IVP_Count_In += count_in;
		IVP_Count_Out += count_out;
		char tmp_in[8], tmp_out[8];
		sprintf(tmp_in, "%d", IVP_Count_In);
		sprintf(tmp_out, "%d", IVP_Count_Out);
		utl_fcfg_set_value(IVP_COUNT_FILE, "IVPCountIn", tmp_in);
		utl_fcfg_set_value(IVP_COUNT_FILE, "IVPCountOut", tmp_out);
		utl_fcfg_flush(IVP_COUNT_FILE);
		//		__ivp_send_to_nvr(NULL,count_in,count_out);
		//		printf(".....................count:%x,count_in:%s,count_out:%s\n",areaIndex,tmp_in,tmp_out);
	}
}
static int type_flag = -1; //主要用于记录智能分析停止日志是否添加的判断标志，在报警持续时间内只停止一次
static void __IvpAlarmOut(Alarm_Out_t *alarm_param, _UINT32 type)
{
	int ret = FALSE;
	static int timeEended = 0;
	static time_t timeLastCloudAlarm = 0;

	int timenow;
	struct timespec tv; 
	int ignore_flag = 0; //用于标志是否超时。如果在较短时间内第二次触发报警，则不再响应

	ALARMSET stAlarm;
	malarm_get_param(&stAlarm);
	alarm_param->nDelay = stAlarm.delay;

	static U32 alarmOutType = 0;
	BOOL bAlarmReportEnable = FALSE; // 安全防护开关
	BOOL bCloudRecEnable = FALSE; // 是否开通云存储功能
	BOOL bInValidTime = FALSE; // 是否在安全防护时间段内
	JV_ALARM alarm;


	clock_gettime(CLOCK_MONOTONIC, &tv);
	timenow = tv.tv_sec;
	bCloudRecEnable = mcloud_check_enable();

	if (bCloudRecEnable == TRUE)
	{
		if (timenow - timeLastCloudAlarm >= 30)
		{
			if (timenow - timeEended <= alarm_param->nDelay)
			{
				printf("MMVA nDelay no need to send again %d.\n", alarm_param->nDelay);
				timeEended = timenow;
				ignore_flag = 1;
			}
		}
		else
		{
			printf("MMVA no need to send again %d.\n", 30);
			ignore_flag = 1;
		}
		
	}
	else
	{
		//printf("time send = %d\n",timesended);
		if (timenow - timeEended < alarm_param->nDelay)
		{
			ignore_flag = 1;
		}
		else if(type > 0)
			timeEended = timenow;

	}

	

	if (type>0)
	{
		bInValidTime = malarm_check_validtime();
		if(!bInValidTime)
		{
			if (alarm_param->bStarting)
			{
				if(alarmOutType & ALARM_OUT_CLIENT)
				{
					if(type == e_IPV_ALARM_TYPE_HIDE)	//遮挡报警
					{
						if (callback_ptr_hide != NULL)
						{
							callback_ptr_hide(0, FALSE);
						}
					}
					else if(type==e_IPV_ALARM_TYPE_ABANDONED_OBJ)
					{
						if (callback_ptr_left != NULL)
							callback_ptr_left(0, FALSE);
					}
					else if(type==e_IPV_ALARM_TYPE_REMOVED_OBJ)
					{
						if (callback_ptr_removed != NULL)
							callback_ptr_removed(0, FALSE);
					}
					else if(type == e_IPV_ALARM_TYPE_CROWD_DENSITY)
					{
						if (callback_ptr_cde != NULL)
							callback_ptr_cde(0,FALSE);
					}
					else if(type == e_IPV_ALARM_TYPE_FAST_MOVE)
					{
						if (callback_ptr_fm != NULL)
							callback_ptr_fm(0,FALSE);
					}
					else if(type == e_IPV_ALARM_TYPE_HOVER)
					{
						if (callback_ptr_hover != NULL)
							callback_ptr_hover(0,FALSE);
					}
		                    else if(type == e_IPV_ALARM_TYPE_OCL)
		                    {
						if (callback_ptr_ocl != NULL)
							callback_ptr_ocl(0,FALSE);
		                    }
					else if(type == e_IPV_ALARM_TYPE_FIRE)
					{
						if (callback_ptr_fire != NULL)
							callback_ptr_fire(0,FALSE);
					}
					else if(type == e_IPV_ALARM_TYPE_VIRTUAL_FOCUS)
					{
						if (callback_ptr_vf != NULL)
							callback_ptr_vf(0,FALSE);
					}
					else if(type == e_IVP_ALARM_TYPE_SCENE_CHANGE)
					{
						if (callback_ptr_sc != NULL)
							callback_ptr_sc(0,FALSE);
					}
					else	//区域入侵和绊线检测报警
					{
						if (callback_ptr != NULL)
						{
			//				printf(">>>> LK test send to client stop!\n");
							callback_ptr(0, FALSE);
						}
					}
				}
				if(alarmOutType & ALARM_OUT_BUZZ)
					malarm_buzzing_close();
				if(alarmOutType & ALARM_OUT_SOUND)
					malarm_sound_stop();
				alarmOutType = 0;
	//			mlog_write("MAlarmIn: Client Alarm Off");
				//mrecord_set(0, alarming, FALSE);
				alarm_param->bStarting = FALSE;
			}
			return;
		}

		if(!ignore_flag)
		{
			if(bCloudRecEnable)
			{
				timeEended = timenow + 30;
				timeLastCloudAlarm = timenow;
			}
		}

		alarm_param->bStarting = TRUE;
		bAlarmReportEnable = malarm_check_enable();
	       bAlarmReportEnable = (mfirmup_b_updating()) ? FALSE : bAlarmReportEnable;
		if (!ignore_flag)
		{
			type_flag = type;
			int i;
			if(type == e_IPV_ALARM_TYPE_HIDE)	//遮挡报警
			{
				mlog_write("MIVA Alarm: IVA Video Covered Alarm Start");
			}
			else if(type==e_IPV_ALARM_TYPE_ABANDONED_OBJ)
			{
				mlog_write("MIVA Alarm: IVA Item Left Alarm Start");
			}
			else if(type==e_IPV_ALARM_TYPE_REMOVED_OBJ)
			{
				mlog_write("MIVA Alarm: IVA Item Removed Alarm Start");
			}
			else if(type==e_IPV_ALARM_TYPE_CROWD_DENSITY)
				mlog_write("MIVA Alarm: IVA Item Population density Alarm Start");
			else if(type==e_IPV_ALARM_TYPE_FAST_MOVE)
				mlog_write("MIVA Alarm: IVA Item Fast Moving Alarm Start");
			else if(type==e_IPV_ALARM_TYPE_HOVER)
				mlog_write("MIVA Alarm: IVA Item Hovering Detection alarm Alarm Start");
			else if(type==e_IPV_ALARM_TYPE_OCL)
				mlog_write("MIVA Alarm: IVA Item Overcrowding Detection alarm Alarm Start");
			else if(type==e_IPV_ALARM_TYPE_FIRE)
				mlog_write("MIVA Alarm: IVA Item Fire Detection alarm Alarm Start");
			else if(type==e_IPV_ALARM_TYPE_VIRTUAL_FOCUS)
			{
				mlog_write("MIVA Alarm: IVA Item Virtual Focus Detection alarm Alarm Start");
			}
			else if(type==e_IVP_ALARM_TYPE_SCENE_CHANGE)
				mlog_write("MIVA Alarm: IVA Item Scene Change alarm Alarm Start");
			else
			{
				mlog_write("MIVA Alarm: IVA Alarm Start");
			}
		}
//		printf("ignore_flag=%d alarm_param->bEnableRecord=%d,bAlarmReportEnable=%d,bCloudRecEnable=%d\n",ignore_flag,alarm_param->bEnableRecord,bAlarmReportEnable,bCloudRecEnable);


		if (!ignore_flag)
		{
			if(hwinfo.bHomeIPC && bAlarmReportEnable)
			{
				if(type == e_IPV_ALARM_TYPE_HIDE)	//遮挡报警
					malarm_build_info(&alarm, ALARM_CAT_HIDEDETECT);
				else
					malarm_build_info(&alarm, ALARM_IVP);
				if(bCloudRecEnable)
					alarm.pushType = ALARM_PUSH_YST_CLOUD;
			}
			if (alarm_param->bEnableRecord)
			{
				if(hwinfo.bHomeIPC  && bCloudRecEnable && bAlarmReportEnable)
				{
					alarm.cmd[1] = ALARM_VIDEO;
					mrecord_alarming(0, ALARM_TYPE_IVP, &alarm);
				}
				else
				{
					//printf("#########start alarm record#######################\n");
					mrecord_alarming(0, ALARM_TYPE_IVP,NULL);
				}
			}
		
			if(hwinfo.bHomeIPC && bAlarmReportEnable)
			{
				mrecord_alarm_get_attachfile(REC_IVP, &alarm);
				if (alarm.cloudPicName[0])
				{
					printf("alarmJpgFile: %s\n", alarm.cloudPicName);
					if (msnapshot_get_file(0, alarm.cloudPicName) != 0)
					{
						alarm.cloudPicName[0] = '\0';
						alarm.PicName[0] = '\0';
					}
				}
				else if (alarm.PicName[0])
				{
					printf("alarmJpgFile: %s\n", alarm.PicName);
					if (msnapshot_get_file(0,  alarm.PicName) != 0)
					{
						alarm.PicName[0] = '\0';
					}
				}
				else
					printf("MIVP cloudPicName and PicName are none\n");
		
#ifdef BIZ_CLIENT_SUPPORT
				if (hwinfo.bXWNewServer)
				{
					int ret = -1;
					char *ppic = "";
					char tmppic[64] = {0};
					char *pvedio = "";
					if (alarm.pushType == ALARM_PUSH_YST_CLOUD)
					{
						if (alarm.cloudPicName[0])
						{
							ppic = strrchr(alarm.cloudPicName, '/');
							struct tm tmDate;
							localtime_r(&alarm.time, &tmDate);
							sprintf(tmppic,"%s/%s/%.4d%.2d%.2d%s",obss_info.days,alarm.ystNo,tmDate.tm_year + 1900,
								tmDate.tm_mon + 1, tmDate.tm_mday, ppic);	
							ppic = tmppic;
						}
						if (alarm.cloudVideoName[0])
						{
							pvedio = alarm.cloudVideoName;
						}
					}
					else
					{
						if (alarm.PicName[0])
						{
							ppic = alarm.PicName;
						}
						if (alarm.VideoName[0])
						{
							pvedio = alarm.VideoName;
						}
					}
					ret = mbizclient_PushAlarm("MIVP",alarm.ystNo,alarm.nChannel,ALARM_TEXT,alarm.uid,alarm.alarmType,alarm.time,ppic,pvedio);
					printf("MIVP mbizclient_PushAlarm successful!!!! %d\n\n", ret);
				}
#endif
				if(bCloudRecEnable && alarm.cloudPicName[0])
				{
					alarm.cmd[1] = ALARM_PIC;
					mcloud_upload_alarm_file(&alarm);
				}

				if (hwinfo.bCloudSee)
				{
					alarm.cmd[1] = ALARM_TEXT;
					mdevSendAlarm2Server(&alarm);
				}
			}
		}		
		if (alarm_param->bOutClient)
		{
			{
				//这里每次都给分控发报警信号，便于刚连接的分控能收到
				if(type == e_IPV_ALARM_TYPE_HIDE)	//遮挡报警
				{
					if (callback_ptr_hide != NULL)
					{
						callback_ptr_hide(0, TRUE);
					}
				}
				else if(type==e_IPV_ALARM_TYPE_ABANDONED_OBJ)
				{
					if (callback_ptr_left != NULL)
						callback_ptr_left(0, TRUE);
				}
				else if(type==e_IPV_ALARM_TYPE_REMOVED_OBJ)
				{
					if (callback_ptr_removed != NULL)
						callback_ptr_removed(0, TRUE);
				}
				else if(type == e_IPV_ALARM_TYPE_CROWD_DENSITY)
				{
					if (callback_ptr_cde != NULL)
						callback_ptr_cde(0,TRUE);
				}
				else if(type == e_IPV_ALARM_TYPE_FAST_MOVE)
				{
					if (callback_ptr_fm != NULL)
						callback_ptr_fm(0,TRUE);
				}
				else if(type == e_IPV_ALARM_TYPE_HOVER)
				{
					if (callback_ptr_hover != NULL)
						callback_ptr_hover(0,TRUE);
				}
				else if(type == e_IPV_ALARM_TYPE_OCL)
				{
					if (callback_ptr_ocl != NULL)
						callback_ptr_ocl(0,TRUE);
				}
				else if(type == e_IPV_ALARM_TYPE_FIRE)
				{
					if (callback_ptr_fire != NULL)
					{
						callback_ptr_fire(0,TRUE);
					}
				}
				else if(type == e_IPV_ALARM_TYPE_VIRTUAL_FOCUS)
				{
					if (callback_ptr_vf != NULL)
					{
						callback_ptr_vf(0,TRUE);
					}
				}
				else if(type == e_IVP_ALARM_TYPE_SCENE_CHANGE)
				{
					if (callback_ptr_sc != NULL)
					{
						callback_ptr_sc(0,TRUE);
					}
				}
				else
				{
					if (callback_ptr != NULL)
					{
						callback_ptr(0, TRUE);
					}
				}

//				if (!ignore_flag)
//					mlog_write("MIVPAlarm: Client Alarm On");
				alarmOutType |= ALARM_OUT_CLIENT;
			}
		}
		if (!ignore_flag && alarm_param->bOutVMS)
		{
			//printf("send alarm to vms server\n");
			alarm_param->bOutVMS = FALSE;
			ipcinfo_t param;
			ipcinfo_get_param(&param);
		}
		if (!ignore_flag)
		{
			if (alarm_param->bOutEMail)
			{
				if(type == e_IPV_ALARM_TYPE_HIDE)	//遮挡报警
					malarm_sendmail(0, mlog_translate("MIVA Alarm: IVA Video Covered Alarm Start")); //借用mlog的多语言管理
				else if (type == e_IPV_ALARM_TYPE_ABANDONED_OBJ)
					malarm_sendmail(0, mlog_translate("MIVA Alarm: IVA Item Left Alarm Start"));
				else if (type == e_IPV_ALARM_TYPE_REMOVED_OBJ)
					malarm_sendmail(0, mlog_translate("MIVA Alarm: IVA Item Removed Alarm Start"));
				else if(type==e_IPV_ALARM_TYPE_CROWD_DENSITY)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Population density Alarm Start"));
				else if(type==e_IPV_ALARM_TYPE_FAST_MOVE)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Fast Moving Alarm Start"));
				else if(type==e_IPV_ALARM_TYPE_HOVER)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Hovering Detection alarm Alarm Start"));
				else if(type==e_IPV_ALARM_TYPE_OCL)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Overcrowding Detection alarm Alarm Start"));
				else if(type==e_IPV_ALARM_TYPE_FIRE)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Fire Detection alarm Alarm Start"));
				else if(type==e_IPV_ALARM_TYPE_VIRTUAL_FOCUS)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Virtual Focus Detection alarm Alarm Start"));
				else if(type==e_IVP_ALARM_TYPE_SCENE_CHANGE)
					malarm_sendmail(0,mlog_translate("MIVA Alarm: IVA Item Scene Change alarm Alarm Start"));
				else
					malarm_sendmail(0, mlog_translate("MIVA Alarm: IVA Alarm Start")); //借用mlog的多语言管理
//				mlog_write("MIVA Alarm: Mail Sended");
				//printf(">>>> LK test send to email alarm in alarming!\n");
			}

		}
		if (alarm_param->bOutAlarm1 && bAlarmReportEnable)
		{
			//蜂鸣器报警输出140415
			Printf("LK TEST BUZZING ALARMIVP\n");
			malarm_buzzing_open();
			alarmOutType |= ALARM_OUT_BUZZ;
		}
		if(alarm_param->bOutSound && hwinfo.bHomeIPC)
		{
			malarm_sound_start();
			alarmOutType |= ALARM_OUT_SOUND;

		}
	}
	else if (type==0)
	{
		Printf("stop alarm...\n");

		if (alarm_param->bStarting)
		{	
			MIVP_t mivp;
			mivp_get_param(0,&mivp);
			if(alarmOutType & ALARM_OUT_CLIENT)
			{
				if(callback_ptr_hide!=NULL)
					callback_ptr_hide(0, FALSE);
				
				if(callback_ptr_left != NULL)
					callback_ptr_left(0, FALSE);
				
				if(callback_ptr_removed != NULL)
					callback_ptr_removed(0, FALSE);
				
				if(callback_ptr_cde != NULL)
					callback_ptr_cde(0,FALSE);		

				if(callback_ptr_fm != NULL)
					callback_ptr_fm(0,FALSE);	

				if(callback_ptr_hover != NULL)
					callback_ptr_hover(0,FALSE);
				if(callback_ptr_ocl != NULL)
				    callback_ptr_ocl(0, FALSE);
				
				if(callback_ptr_fire != NULL)
					callback_ptr_fire(0,FALSE);	

				if(callback_ptr_vf != NULL)
					callback_ptr_vf(0,FALSE);	
				
				if(callback_ptr_sc != NULL)
					callback_ptr_sc(0,FALSE);	
				
				if(callback_ptr != NULL)
					callback_ptr(0, FALSE);
			}

			if (type_flag != type)
			{
				type_flag = type;
				if(mivp.st_hide_attr.bHideEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Video Covered Alarm Stop");
				else if(mivp.st_tl_attr.bTLEnable == TRUE)
				{
					if(mivp.st_tl_attr.nTLMode == 0)		//e_IPV_ALARM_TYPE_ABANDONED_OBJ)
						mlog_write("MIVA Alarm: IVA Item Left Alarm Stop");
					else if(mivp.st_tl_attr.nTLMode == 1)	//e_IPV_ALARM_TYPE_REMOVED_OBJ)
						mlog_write("MIVA Alarm: IVA Item Removed Alarm Stop");
				}
				else if(mivp.st_rl_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Alarm Stop");
				else if(mivp.st_cde_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Population density Alarm Stop");
				else if(mivp.st_fm_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Fast Moving Alarm Stop");
				else if(mivp.st_hover_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Hovering Detection alarm Alarm Stop");
				else if(mivp.st_ocl_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Overcrowding Detection alarm Alarm Stop");
				else if(mivp.st_fire_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Fire Detection alarm Alarm Stop");
				else if(mivp.st_vf_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Virtual Focus Detection alarm Alarm Stop");
				else if(mivp.st_sc_attr.bEnable == TRUE)
					mlog_write("MIVA Alarm: IVA Item Scene Change alarm Alarm Stop");
			}
			if(alarmOutType & ALARM_OUT_BUZZ)
				malarm_buzzing_close();
			if(alarmOutType & ALARM_OUT_SOUND)
				malarm_sound_stop();
	
			alarmOutType = 0;
//			mlog_write("MAlarmIn: Client Alarm Off");
			//mrecord_set(0, alarming, FALSE);
			alarm_param->bStarting = FALSE;
		}
	}
}

static void __IvpAlarm(void *ivp, int areaIndex, _UINT32 type, Z_LIST *areas)
{
	printf("##################=============>> IVP ALARM ING TYPE:%d  <<==============###################\n",type);
	int i = 0;
	IVP_Release_State = type;
	Alarm_Out_t *PAlarmOut;
	if(type==e_IPV_ALARM_TYPE_HIDE)	//视频遮挡报警
		__IvpAlarmOut(&mivplist[i].st_hide_attr.stHideAlarmOut, type);
	else if(type==e_IPV_ALARM_TYPE_ABANDONED_OBJ||type==e_IPV_ALARM_TYPE_REMOVED_OBJ)
		__IvpAlarmOut(&mivplist[i].st_tl_attr.stTLAlarmOut, type);
	else if(type==e_IPV_ALARM_TYPE_HOVER)
		__IvpAlarmOut(&mivplist[i].st_hover_attr.stAlarmOut, type);
	else if(type==e_IPV_ALARM_TYPE_FAST_MOVE)
		__IvpAlarmOut(&mivplist[i].st_fm_attr.stAlarmOutRL, type);
	else if(type==e_IPV_ALARM_TYPE_FIRE)
	{
		__IvpAlarmOut(&mivplist[i].st_fire_attr.stAlarmOut, type);
	}
	else if(type==e_IPV_ALARM_TYPE_VIRTUAL_FOCUS)
	{
		__IvpAlarmOut(&mivplist[i].st_vf_attr.stAlarmOut, type);
	}
	else if(type==e_IVP_ALARM_TYPE_SCENE_CHANGE)	//绊线检测和区域入侵报警
		__IvpAlarmOut(&mivplist[i].st_sc_attr.stAlarmOut, type);
	else if(type>0)	//绊线检测和区域入侵报警
	{

			__IvpAlarmOut(&mivplist[i].st_rl_attr.stAlarmOutRL, type);
	}
	else if(type==0)
	{	//停止所有报警
		__IvpAlarmOut(&mivplist[i].st_hide_attr.stHideAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_rl_attr.stAlarmOutRL, type);
		__IvpAlarmOut(&mivplist[i].st_tl_attr.stTLAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_cde_attr.stCDEAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_ocl_attr.stOCLAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_fm_attr.stAlarmOutRL, type);
		__IvpAlarmOut(&mivplist[i].st_hover_attr.stAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_fire_attr.stAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_vf_attr.stAlarmOut, type);
		__IvpAlarmOut(&mivplist[i].st_sc_attr.stAlarmOut, type);
	}
	return ;
}

static int __IvpGetPtzSt(int viChn)
{
	BOOL ptz_ctrl = PTZ_Get_Status(0);
	return ptz_ctrl;
}

//修复绊线画的太平的时候检测方向错误问题，改左右方向为上下方向
static int _IvpLineCheckModeCorrect2UpDown(IVP_POINT *points0,IVP_POINT *points1)	//绊线检测方向矫正
{
	int uw = points0->x > points1->x ? points0->x-points1->x : points1->x-points0->x;
	int uh = points0->y > points1->y ? points0->y-points1->y : points1->y-points0->y;
	int sw = points0->x-points1->x;
	int sh = points0->y-points1->y;
	if(sw==0)
		return 0;	//R And L
	if(sh==0)
		return 1;	//Down to Up
	if((sw>0 && sh>0)||(sw<0 && sh<0))
	{
		if(uw>uh)
			return 1;	////Down to Up
	}
	else
	{
		if(uw>uh)
			return 2;	////Up to Down
	}
	return 0;
}

/*
 *@brief 设置智能分析区域
 *@return 0成功，-1句柄错误，-2区域为空
 */
static int __IvpSetArea(void *ivp,int chn)
{
#ifdef IVP_SUPPORT
    int i,j;

    if(!ivp)
        return -1;
    if(mivplist[chn].st_rl_attr.nRgnCnt<=0)
    	return -2;

    ivpRegisterCallbk(ivp, __IvpAlarm);


    IVP_POINT points[MAX_POINT_NUM] = {{0}};
    jvstream_ability_t ability;
    jv_stream_get_ability(chn, &ability);

    IVP_CHECK_MODE mode;
    jv_stream_attr attr;
    jv_stream_get_attr(chn, &attr);
    unsigned int viWidth, viHeight;
    viWidth = ability.inputRes.width;
    viHeight = ability.inputRes.height;

    for(i=0;i<mivplist[chn].st_rl_attr.nRgnCnt;i++)
    {
		if(mivplist[chn].st_rl_attr.bHKEnable) //是海康nvr在设置,执行兼容海康nvr的参数
		{
			if(i == 0 && !mivplist[chn].st_rl_attr.bCLEnable)	//hk nvr绊线设置
				continue;
			else if(i == 1 && !mivplist[chn].st_rl_attr.bRInvEnable) //hk nvr 区域入侵
				continue;
			else if(i == 2 && !mivplist[chn].st_rl_attr.bRInEnable) //hk nvr 进入区域
				continue;
			else if(i == 3 && !mivplist[chn].st_rl_attr.bROutEnable) //hk nvr 离开区域
				continue;
		}
		
    	for(j=0;j<mivplist[chn].st_rl_attr.stRegion[i].nCnt;j++)
    	{
    		points[j].x = mivplist[chn].st_rl_attr.stRegion[i].stPoints[j].x;//* attr.width /  viWidth;;
    		points[j].y = mivplist[chn].st_rl_attr.stRegion[i].stPoints[j].y;// * attr.height / viHeight;
  //  		__build_point_vi(mivplist[chn].st_rl_attr.stRegion[i].stPoints[j].x,mivplist[chn].st_rl_attr.stRegion[i].stPoints[j].y,&points[j].x,&points[j].y,viWidth,viHeight,attr.width,attr.height);
			points[j].x = points[j].x>viWidth?viWidth:points[j].x;
			points[j].y = points[j].y>viHeight?viHeight:points[j].y;
    	}
    	if(mivplist[chn].st_rl_attr.stRegion[i].nCnt>2)
    	{
			mode = e_IVP_CHECK_MODE_AREA;
    	}
    	else if(mivplist[chn].st_rl_attr.stRegion[i].nCnt==2)
    	{
    		int b2UpDown = _IvpLineCheckModeCorrect2UpDown(&points[0],&points[1]);
    		switch(mivplist[chn].st_rl_attr.stRegion[i].nIvpCheckMode)
			{
				case 0:
				mode = b2UpDown?(b2UpDown==1?(e_IVP_CHECK_MODE_LINE_D2U):(e_IVP_CHECK_MODE_LINE_U2D)):e_IVP_CHECK_MODE_LINE_L2R;
				break;
				case 1:
				mode = b2UpDown?(b2UpDown==1?(e_IVP_CHECK_MODE_LINE_U2D):(e_IVP_CHECK_MODE_LINE_D2U)):e_IVP_CHECK_MODE_LINE_R2L;
				break;
				case 2:
				mode = e_IVP_CHECK_MODE_LINE_LR;
				break;
				default:
				mode = e_IVP_CHECK_MODE_LINE_LR;
				break;
			}
    	}
    	else
    		return -1;

#if 0	//test
    	int k = 0;
    	for(;k<mivplist[chn].st_rl_attr.stRegion[i].nCnt;k++)
    		printf("point:%d,%d\n",points[k].x,points[k].y);
    	printf("=================%d,%d\n",viWidth,viHeight);
#endif

    	ivpAddRule(ivp, i, mode, mivplist[chn].st_rl_attr.stRegion[i].nCnt, points, viWidth, viHeight);
    	memset(points,0,MAX_POINT_NUM);
    }
	printf("##############:Sen:%d,StayTime:%d,Threshold:%d\n",mivplist[chn].st_rl_attr.nSen,mivplist[chn].st_rl_attr.nStayTime,mivplist[chn].st_rl_attr.nThreshold);
	ivpSetSensitivity(mivphandle,100 - mivplist[chn].st_rl_attr.nSen);
	//    ivpSetStaytime(mivphandle,mivplist[chn].st_rl_attr.nStayTime);
	ivpSetThreshold(mivphandle,mivplist[chn].st_rl_attr.nThreshold);
    	ivpSetStaytime(mivphandle,mivplist[chn].st_rl_attr.nStayTime);
#endif
    return 0;
}

int mivp_start_count(int chn)
{
#ifdef IVP_COUNT_SUPPORT
	if(mivp_count_bsupport())
	{
		if(!mivplist_count[chn].st_count_attr.bOpenCount)
			return -1;
		if(!mivphandle_count)
		{
			U32 IvpMode=0;
			if(mivplist_count[chn].st_count_attr.bDrawFrame)
				IvpMode |= e_IVP_MODE_DRAW_FRAME;
			if (mivplist_count[chn].st_count_attr.bFlushFrame && mivplist_count[chn].st_count_attr.bDrawFrame)
				IvpMode |= e_IVP_MODE_FLUSH_FRAME;
			if(mivplist_count[chn].st_count_attr.bOpenCount)
				IvpMode |= e_IVP_MODE_COUNT_INOUT;
			mivphandle_count = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
			ivpRegisterCallbk(mivphandle_count, __IvpAlarmCount);
		}
		if (mivphandle_count)
		{
			int index = 0;
			int pointCnt = 2;
			IVP_POINT points[2];
			jvstream_ability_t ability;
			jv_stream_get_ability(chn, &ability);

			int nLinePosY = mivplist_count[chn].st_count_attr.nLinePosY;
			nLinePosY = nLinePosY>=100?99:nLinePosY;
			int minY = ability.inputRes.height*3/7;
			int maxY = ability.inputRes.height*3/4;
			int Y = minY + (maxY-minY)*nLinePosY/100;

			points[0].x = 0;
			points[0].y = Y;
			points[1].x = ability.inputRes.width;
			points[1].y = Y;
			ivpAddRule(mivphandle_count, index, e_IVP_CHECK_MODE_LINE_UD,pointCnt, points, ability.inputRes.width, ability.inputRes.height);
		}
		return 0;
	}
#endif
	return -1;
}

#define MAX_CNT 25
static char tmp_str[MAX_CNT][64];
static int count_cnt = 0;
static int reset_year = 0,reset_day = 0;
static int COUNT_IN_LAST,COUNT_OUT_LAST;

static BOOL __ivp_count_timer(int tid, void *param)
{
	struct tm tm;
	time_t tt = time(NULL);
	localtime_r(&tt, &tm);
	char tmp_time[32];
	static int old_hour = -1;
	static int bOpenCountOld=-1;
	int resettime=0;

	if(!mivplist_count[0].st_count_attr.bOpenCount)
		return TRUE;//没开启不传递数据

	if(bOpenCountOld!=mivplist_count[0].st_count_attr.bOpenCount)
	{
		if(mivplist_count[0].st_count_attr.bOpenCount==1)
			resettime = 1;
		bOpenCountOld = mivplist_count[0].st_count_attr.bOpenCount;
	}

	if(reset_year==2000 || resettime)//未对时	|| 第一次开启也要记录一下时间
	{
		if(reset_year != tm.tm_year+1900 || resettime)//对时成功
		{
			reset_year = tm.tm_year + 1900;
			reset_day = tm.tm_yday;
			char str[8];
			sprintf(str, "%d", reset_year);
			utl_fcfg_set_value(IVP_COUNT_FILE, "reset_year", str);
			sprintf(str, "%d", reset_day);
			utl_fcfg_set_value(IVP_COUNT_FILE, "reset_day", str);
			utl_fcfg_flush(IVP_COUNT_FILE);
		}
		return TRUE;
	}

	sprintf(tmp_time, "%d:%d", tm.tm_hour, 0);

	while(tm.tm_hour!=old_hour)
	{
		if(old_hour == -1)//第一次不发送
		{
			old_hour = tm.tm_hour;
			break;
		}
		if(tm.tm_hour<old_hour && tm.tm_hour!=0)//此时说明IPC往前对时了，比如从9点多对时到8点多，做累加处理防止覆盖数据
		{
			printf("set time\n");
			if (--count_cnt < 0)
				count_cnt = 23;
			int tmp_last_in,tmp_last_out;
			sscanf(tmp_str[count_cnt],"count_in:%d,count_out:%d",&tmp_last_in,&tmp_last_out);
			COUNT_IN_LAST -= tmp_last_in;
			COUNT_OUT_LAST -= tmp_last_out;
			old_hour = tm.tm_hour;
			break;
		}
		old_hour = tm.tm_hour;
		char Data[64]= {0};
		Data[0]	= 'I';
		Data[1]	= 'V';
		Data[2] = 'P';
		Data[3] = 'C';
		Data[4] = 'O';
		Data[5] = 'U';
		Data[6] = 'N';
		Data[7] = 'T';
		char tmpstr[64];
		sprintf(tmpstr,"%s=%s",tmp_time,tmp_str[count_cnt]);
		memcpy(Data+8,tmpstr,64-8);
//		printf("send to nvr:%s\n",Data);
#ifdef YST_SVR_SUPPORT
		remotecfg_send_self_data(0 ,Data ,64);
#endif

		COUNT_IN_LAST = IVP_Count_In;
		COUNT_OUT_LAST = IVP_Count_Out;
		if (++count_cnt >= 24)
			count_cnt = 0;

		break;
	}

#if 0
	if(tm.tm_min%10<=1)
	{
		int tmp_min = tm.tm_min;
		if(count_cnt>=144)
			count_cnt = 0;
		if(tmp_min%10 == 1)
			tmp_min--;
		sprintf(tmp_time,"%d:%d",tm.tm_hour,tmp_min);
		memset(tmp_str[count_cnt],0,64);
		sprintf(tmp_str[count_cnt],"count_in:%d,count_out:%d",IVP_Count_In,IVP_Count_Out);
		printf(tmp_time);
		utl_fcfg_set_value(IVP_COUNT_FILE, tmp_time,tmp_str[count_cnt]);
		utl_fcfg_flush(IVP_COUNT_FILE);
		printf("save file\n");
		count_cnt++;
	}
#else
	memset(tmp_str[count_cnt], 0, 64);
	sprintf(tmp_str[count_cnt], "count_in:%d,count_out:%d",
			IVP_Count_In - COUNT_IN_LAST, IVP_Count_Out - COUNT_OUT_LAST);
	utl_fcfg_set_value(IVP_COUNT_FILE, tmp_time, tmp_str[count_cnt]);
	utl_fcfg_flush(IVP_COUNT_FILE);
//	printf("save file:%s=%s\n",tmp_time,tmp_str[count_cnt]);
#endif
	//定时清空环节
	int year_offset = 0;
	static int cleard_hour = -1;
	if (reset_year != tm.tm_year + 1900)
		year_offset = 365;
	if ((tm.tm_yday + year_offset - reset_day)
			>= mivplist_count[0].st_count_attr.nCountSaveDays + 1 && tm.tm_hour == 0 &&cleard_hour == 23
			&& tm.tm_min <= 1)
	{
		reset_year = tm.tm_year;
		reset_day = tm.tm_yday + 1900;
		char str[8];
		sprintf(str, "%d", reset_year);
		utl_fcfg_set_value(IVP_COUNT_FILE, "reset_year", str);
		sprintf(str, "%d", reset_day);
		utl_fcfg_set_value(IVP_COUNT_FILE, "reset_day", str);
		mivp_count_reset(0);
	}
	if(tm.tm_hour != cleard_hour)
			cleard_hour = tm.tm_hour;
	return TRUE;
}

static BOOL __ivp_linecount_timer(int tid, void *param)
{
	struct tm tm;
	time_t tt = time(NULL);
	localtime_r(&tt, &tm);
	char tmp_time[32];
	static int old_hour = -1;
	static int bOpenCountOld=-1;
	int resettime=0;

	if(!mivplist_count[0].st_count_attr.bOpenCount)
		return TRUE;//没开启不传递数据

	if(bOpenCountOld!=mivplist_count[0].st_count_attr.bOpenCount)
	{
		if(mivplist_count[0].st_count_attr.bOpenCount==1)
			resettime = 1;
		bOpenCountOld = mivplist_count[0].st_count_attr.bOpenCount;
	}

	if(reset_year==2000 || resettime)//未对时	|| 第一次开启也要记录一下时间
	{
		if(reset_year != tm.tm_year+1900 || resettime)//对时成功
		{
			reset_year = tm.tm_year + 1900;
			reset_day = tm.tm_yday;
		}
		return TRUE;
	}

	//定时清空环节
	int year_offset = 0;
	static int cleard_hour = -1;
	if (reset_year != tm.tm_year + 1900)
		year_offset = 365;
	if ((tm.tm_yday + year_offset - reset_day)
			>= mivplist_count[0].st_count_attr.nCountSaveDays + 1 && tm.tm_hour == 0 &&cleard_hour == 23
			&& tm.tm_min <= 1)
	{
		reset_year = tm.tm_year + 1900;
		reset_day = tm.tm_yday;
		mivp_count_reset(0);
	}
	if(tm.tm_hour != cleard_hour)
			cleard_hour = tm.tm_hour;

	return TRUE;
}
static BOOL __proc_test()
{
	char Data[64] =
	{ 0 };
	Data[0] = 'I';
	Data[1] = 'V';
	Data[2] = 'P';
	Data[3] = 'V';
	Data[4] = 'R';
	char sendstr[64];
	sprintf(sendstr, "rate=%d", 22);
	memcpy(Data + 5, sendstr, 64 - 5);
	printf("ivp v.r. send to nvr:%s\n", Data);
#ifdef YST_SVR_SUPPORT
	remotecfg_send_self_data(0, Data, 64);
#endif
	if(callback_ptr_vr!=NULL)	//报警信息，改报警信息可以发送到所有客户端
	{
		printf("ivp v.r. send jpeg to nvr:%s\n", Data);
		mchnosd_snap_add_txt(0, sendstr);
		callback_ptr_vr(0, TRUE);
		mchnosd_snap_del_txt(0);
	}
	if (callback_ptr_vr != NULL)	//报警信息，改报警信息可以发送到所有客户端
	{
		callback_ptr_vr(0, FALSE);
	}
	return TRUE;
}
#if 1
static int ivp_debug_main(int argc, char *argv[])
{
	int time;
	 if(strcmp(argv[1], "re")==0)				
	{
		mivp_restart(0);
	
	}
	 else if(strcmp(argv[1], "atime")==0)		
	 {
	 	time=atoi(argv[2]);
		mivp_SetAlarmtime(0, time);
		printf("######Set alarm time=%d\n",time);
	 }
	return 0;
}
#endif
int mivp_init()
{
#if defined XW_MMVA_SUPPORT
	mmva_init();
	mivp_count_timer = utl_timer_create("line count timer", 30*1000, __ivp_linecount_timer, NULL);

	return 0;

#endif
#ifdef IVP_SUPPORT
#ifdef IVP_COUNT_SUPPORT
	bIVP_COUNT_SUPPORT = 1;	//做成单独软件不在从hwconfig中读取，默认为1，由IVP_COUNT_SUPPORT来控制是否支持
	if(mivp_count_bsupport())
	{
		struct tm tm;
		time_t tt = time(NULL);
		localtime_r(&tt, &tm);
		if (0 != access(IVP_COUNT_FILE, F_OK))
		{
			utl_system("touch "IVP_COUNT_FILE);
			reset_year = tm.tm_year+1900;
			reset_day = tm.tm_yday;
			char str[8];
			sprintf(str,"%d",reset_year);
			utl_fcfg_set_value(IVP_COUNT_FILE, "reset_year",str);
			sprintf(str,"%d",reset_day);
			utl_fcfg_set_value(IVP_COUNT_FILE, "reset_day",str);
			utl_fcfg_flush(IVP_COUNT_FILE);
		}
		else
		{
			COUNT_IN_LAST = IVP_Count_In = utl_fcfg_get_value_int(IVP_COUNT_FILE,"IVPCountIn",0);
			COUNT_OUT_LAST = IVP_Count_Out = utl_fcfg_get_value_int(IVP_COUNT_FILE,"IVPCountOut",0);
			reset_year = utl_fcfg_get_value_int(IVP_COUNT_FILE,"reset_year",0);
			reset_day = utl_fcfg_get_value_int(IVP_COUNT_FILE,"reset_day",0);
		}
		mivp_count_timer = utl_timer_create("ivp count timer", mivplist_count[0].st_count_attr.nTimeIntervalReport*1000, __ivp_count_timer, NULL);
	}
#endif
//	mivp_count_timer = utl_timer_create("ivp count timer", 1*5*1000, __proc_test, NULL);
#endif

	utl_cmd_insert("ivp", "ivp debug", "\nexp: ivp param1 \n",ivp_debug_main);

	return 0;
}

int mivp_rl_start(int chn)
{
#ifdef IVP_SUPPORT
	if(mivplist[chn].st_rl_attr.nRgnCnt<=0)
		return -1;
	if(!mivplist[chn].st_rl_attr.bEnable && !mivplist[chn].st_rl_attr.bHKEnable)
		return -1;
	if(mivplist[chn].st_rl_attr.nRgnCnt>MAX_IVP_REGION_NUM)
		mivplist[chn].st_rl_attr.nRgnCnt = MAX_IVP_REGION_NUM;	//多出的不画

	if(!mivphandle)
	{
		U32 IvpMode=0;
		if(mivplist[chn].st_rl_attr.bDrawFrame)
		IvpMode |= e_IVP_MODE_DRAW_FRAME;
		if (mivplist[chn].st_rl_attr.bFlushFrame && mivplist[chn].st_rl_attr.bDrawFrame)
		IvpMode |= e_IVP_MODE_FLUSH_FRAME;
		if (mivplist[chn].st_rl_attr.bMarkObject)
		IvpMode |= e_IVP_MODE_MARK_OBJECT;
		if (mivplist[chn].st_rl_attr.bMarkAll)
		IvpMode |= e_IVP_MODE_MARK_ALL;
		IvpMode |= e_IVP_MODE_MARK_SMOOTH;	//默认开启平滑模式
		//mivplist[chn].st_rl_attr.nAlpha = 0;
		mivphandle = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	else
		return -1;

	if(mivphandle)
	{
		printf("IVP start\n");
		return __IvpSetArea(mivphandle,chn);
	}
	else
		return -1;
#endif
	return 0;
}

int mivp_detect_start(int chn)
{
#ifdef IVP_SUPPORT
#endif	
	return 0;
}


/**
 *@brief 初始化
 *@return 成功0；失败-1
 */
int mivp_start(int chn)
{
	JVRotate_e rotate = msensor_get_rotate();
	if(rotate == JVSENSOR_ROTATE_90||rotate == JVSENSOR_ROTATE_270)
		return 0;
	if(mivp_bsupport() !=TRUE)
		return 0;
#ifdef  XW_MMVA_SUPPORT
	mmva_register_alarmcallbk(__IvpAlarm);
	mmva_start(&mivplist[0]);
	return 0;

#endif

#ifdef IVP_COUNT_SUPPORT
	if(mivp_count_bsupport())
	{
		if(mivplist_count[chn].st_count_attr.bOpenCount)
			if(mivp_start_count(chn)>=0)
				return 0;
	}
#endif
#ifdef IVP_SUPPORT
	if(mivp_detect_bsupport())
	{
		if(mivplist[chn].st_detect_attr.bDetectEn == TRUE)
		{
			mivp_detect_start(chn);
		}
	}
//	printf("%s handle=%d\n",__func__,mivphandle);

	if(mivp_bsupport())
	{

		if(mivplist[chn].st_rl_attr.bEnable || mivplist[chn].st_rl_attr.bHKEnable)
			if(mivp_rl_start(chn)>=0 && !mivp_hide_bsupport())
			{
//					printf("%s handle=%d\n",__func__,mivphandle);

				return 0;
			}
	}
#endif

#ifdef IVP_HIDE_SUPPORT
	if(mivp_hide_bsupport())
	{
//		mivplist[chn].st_hide_attr.bHideEnable = 1;
		if(mivplist[chn].st_hide_attr.bHideEnable)
			if(mivp_hide_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_TL_SUPPORT
	if(mivp_tl_bsupport())
	{
		if(mivplist[chn].st_tl_attr.bTLEnable || mivplist[chn].st_tl_attr.bHKEnable)
			if(mivp_tl_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_CDE_SUPPORT
	if(mivp_cde_bsupport())
	{
		if(mivplist[chn].st_cde_attr.bEnable)
			if(mivp_cde_start(chn)>=0)
				return 0;
	}

#endif

#ifdef IVP_OCL_SUPPORT
	if(mivp_ocl_bsupport())
	{
		if(mivplist[chn].st_ocl_attr.bEnable)
			if(mivp_ocl_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_FM_SUPPORT
	if(mivp_fm_bsupport())
	{
		if(mivplist[chn].st_fm_attr.bEnable)
			if(mivp_fm_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_HOVER_SUPPORT
	if(mivp_hover_bsupport())
	{
		if(mivplist[chn].st_hover_attr.bEnable)
			if(mivp_hover_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_FIRE_SUPPORT
	if(mivp_fire_bsupport())
	{
		if(mivplist[chn].st_fire_attr.bEnable)
			if(mivp_fire_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_VF_SUPPORT
	if(mivp_vf_bsupport())
	{
		if(mivplist[chn].st_vf_attr.bEnable)
			if(mivp_vf_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_SC_SUPPORT
	if(mivp_sc_bsupport())
	{
		if(mivplist[chn].st_sc_attr.bEnable)
			if(mivp_sc_start(chn)>=0)
				return 0;
	}
#endif

#ifdef IVP_LPR_SUPPORT
	if(mivp_lpr_bsupport())
	{
		if(mivplist[chn].st_lpr_attr.bEnable)
			if(mivp_lpr_start(chn)>=0)
				return 0;
	}
#endif

	return 0;
}

int mivp_mode_en(int chn,IVP_MODE value, int en)
{
#ifdef IVP_SUPPORT
	return ivpEnMode(mivphandle,value,en);
	return 0;
#endif
	return 0;
}
/**
 *@brief 结束
 *@return 0
 *
 */
int mivp_stop(int chn)
{
	int ret=0;
#if defined XW_MMVA_SUPPORT
	//TODO release
	mmva_stop();
	if(IVP_Release_State > 0)
		__IvpAlarmOut(&mivplist[0].st_rl_attr.stAlarmOutRL, 0);
	return 0;

#endif

	
	if(IVP_Release_State > 0)			//关闭智能分析时如果正在报警，则停止所有报警
	{
		__IvpAlarmOut(&mivplist[0].st_hide_attr.stHideAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_rl_attr.stAlarmOutRL, 0);
		__IvpAlarmOut(&mivplist[0].st_tl_attr.stTLAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_cde_attr.stCDEAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_ocl_attr.stOCLAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_fm_attr.stAlarmOutRL, 0);
		__IvpAlarmOut(&mivplist[0].st_hover_attr.stAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_fire_attr.stAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_vf_attr.stAlarmOut, 0);
		__IvpAlarmOut(&mivplist[0].st_sc_attr.stAlarmOut, 0);
	}
#ifdef IVP_SUPPORT
	if(mivp_rl_bsupport())
	{
		if(mivphandle)
		{
			ret = ivpRelease(mivphandle);
			printf("IVP rl stop : %d\n",ret);
		}
		mivphandle = NULL;
		

	}

#ifdef IVP_COUNT_SUPPORT
	if(mivp_count_bsupport())
	{
		if(mivphandle_count)
		{
			ret = ivpRelease(mivphandle_count);
			printf("IVP count stop: %d\n",ret);
		}
		mivphandle_count = NULL;
	}
#endif

#ifdef IVP_HIDE_SUPPORT
	if(mivp_hide_bsupport())
		mivp_hide_stop(0);
#endif

#ifdef IVP_TL_SUPPORT
	if(mivp_tl_bsupport())
		mivp_tl_stop(0);
#endif
#ifdef IVP_CDE_SUPPORT
	if(mivp_cde_bsupport())
		mivp_cde_stop(0);
#endif
#ifdef IVP_OCL_SUPPORT
	if(mivp_ocl_bsupport())
		mivp_ocl_stop(0);
#endif
#ifdef IVP_HOVER_SUPPORT
	if(mivp_hover_bsupport())
		mivp_hover_stop(0);
#endif
#ifdef IVP_FIRE_SUPPORT
	if(mivp_fire_bsupport())
		mivp_fire_stop(0);
#endif
#ifdef IVP_VF_SUPPORT
	if(mivp_vf_bsupport())
		mivp_vf_stop(0);
#endif
#ifdef IVP_SC_SUPPORT
	if(mivp_sc_bsupport())
		mivp_sc_stop(0);
#endif

#ifdef IVP_LPR_SUPPORT
	if(mivp_lpr_bsupport())
		mivp_lpr_stop(0);
#endif

#endif
	return ret;
}

int mivp_count_reset(int chn)
{
#ifdef IVP_COUNT_SUPPORT
	if(mivp_count_bsupport())
	{
		IVP_Count_In = 0;
		IVP_Count_Out = 0;
		COUNT_IN_LAST = 0;
		COUNT_OUT_LAST = 0;
		char tmp_in[8],tmp_out[8];
		sprintf(tmp_in,"%d",IVP_Count_In);
		sprintf(tmp_out,"%d",IVP_Count_Out);
		utl_fcfg_set_value(IVP_COUNT_FILE, "IVPCountIn",tmp_in);
		utl_fcfg_set_value(IVP_COUNT_FILE, "IVPCountOut",tmp_out);
		utl_fcfg_flush(IVP_COUNT_FILE);
		mlog_write("IVE count data cleared");
		return ivpReset(mivphandle_count);
	}
#endif

#if (defined XW_MMVA_SUPPORT)
	int i;
	mmva_count_reset();
	if(mivplist_count[0].st_count_attr.bOpenCount && mivplist_count[0].st_count_attr.bShowCount)
	for(i=0;i<HWINFO_STREAM_CNT;i++)		//刷新计数的OSD
		mchnosd_flush(i);
	
	mlog_write("IVE count data cleared");
#endif
	return 0;
}



int mivp_count_in_get()
{
#if (defined XW_MMVA_SUPPORT)
	return mmva_count_in_get();
#else
	return IVP_Count_In;

#endif
}
int mivp_count_out_get()
{
#if (defined XW_MMVA_SUPPORT)
	return mmva_count_out_get();
#else
	return IVP_Count_Out;
#endif

}

int mivp_count_set(int channelid, MIVP_t *mivp)
{
	int flag = mivp->st_count_attr.bOpenCount - mivplist_count[channelid].st_count_attr.bOpenCount;
	if(flag > 0)
		mlog_write("Start passenger counting.");
	else if(flag < 0)
		mlog_write("Stop passenger counting.");
	if(mivp->st_count_attr.nTimeIntervalReport != mivplist_count[channelid].st_count_attr.nTimeIntervalReport)
	{
		if(mivp_count_timer >= 0)
		{
			mivplist_count[channelid].st_count_attr.nTimeIntervalReport = mivp->st_count_attr.nTimeIntervalReport;
			utl_timer_destroy(mivp_count_timer);
			mivp_count_timer = -1;
#if defined XW_MMVA_SUPPORT
			mivp_count_timer = utl_timer_create("line count timer", 30*1000, __ivp_linecount_timer, NULL);
#else
			mivp_count_timer = utl_timer_create("ivp count timer", mivplist_count[0].st_count_attr.nTimeIntervalReport*1000, __ivp_count_timer, NULL);
#endif
		}
	}
	memcpy(&mivplist_count[channelid].st_count_attr,&mivp->st_count_attr,sizeof(MIVP_Count_t));
	printf(" %s line %d   rl enable %d count enable %d  cnt %d\n ", __func__,__LINE__,  mivplist[0].st_rl_attr.bEnable,mivplist[0].st_count_attr.bOpenCount,mivplist[0].st_rl_attr.nRgnCnt);

	return 0;
}

int mivp_count_get(int channelid, MIVP_t *mivp)
{

	memcpy(mivp, &(mivplist_count[channelid]), sizeof(MIVP_t));
	return 0;
}

unsigned short mivp_count_show_color_get(IVPCountColor_t *color)
{
	//黑字白边无底色
	color->clear = 0;
	color->text = 0x8000;
	color->border = 0xFFFF;
	switch(mivplist_count[0].st_count_attr.nCountOSDColor)
	{
	case 0:	//白字黑边
		color->text = 0xFFFF;
		color->border = 0x8000;
		return 0xFFFF;
		break;
	case 1:	//黑字白边
		return 0x8000;
		break;
	case 2:	//红字白边
		color->text = 0xFC00;
		return 0xFF00;
		break;
	case 3:	//蓝字白边
		color->text = 0x801F;
		return 0xF00F;
		break;
	case 4:	//绿字白边
		color->text = 0x83E0;
		return 0xF0F0;
		break;
	case 5:	//黄字黑边
		color->text = 0xFFE0;
		color->border = 0x8000;
		return 0xFFF0;
		break;
	default:
		return 0xFFFF;
		break;
	}
	return 0;
}

/**
 *@brief 设置报警回调函数,通知分控的
 *
 */
int mivp_set_callback(alarming_ivp_callback_t callback)
{
#if( (defined IVP_SUPPORT) ||(defined XW_MMVA_SUPPORT))
	callback_ptr = callback;
#endif
	return 0;
}

int mivp_vr_set_callback(alarming_ivp_callback_t callback)
{
	return 0;
}

int mivp_hide_set_callback(alarming_ivp_callback_t callback)
{
#if( (defined IVP_SUPPORT) ||(defined XW_MMVA_SUPPORT))
	callback_ptr_hide = callback;
#endif
	return 0;
}

int mivp_left_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_TL_SUPPORT
	callback_ptr_left = callback;
#endif
	return 0;
}
int mivp_removed_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_TL_SUPPORT
	callback_ptr_removed = callback;
#endif
	return 0;
}
int mivp_cde_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_CDE_SUPPORT
	callback_ptr_cde = callback;
#endif
	return 0;
}
int mivp_ocl_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_OCL_SUPPORT
	callback_ptr_ocl = callback;
#endif
	return 0;
}
int mivp_fm_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_FM_SUPPORT
	callback_ptr_fm = callback;
#endif
	return 0;
}
int mivp_hover_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_HOVER_SUPPORT
	callback_ptr_hover = callback;
#endif
	return 0;
}
int mivp_fire_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_FIRE_SUPPORT
	callback_ptr_fire = callback;
#endif
	return 0;
}
int mivp_vf_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_VF_SUPPORT
	callback_ptr_vf = callback;
#endif
	return 0;
}
int mivp_sc_set_callback(alarming_ivp_callback_t callback)
{
#ifdef IVP_SC_SUPPORT
	callback_ptr_sc = callback;
#endif
	return 0;
}


/**
 *@brief 设置参数
 *@param channelid 频道号
 *@param mivp 要设置的属性结构体
 *@note 如果不能确定所有属性的值，请先#mivp_get_param获取原本的值
 *@return 0 成功，-1 失败
 *
 */
int mivp_set_param(int channelid, MIVP_t *mivp)
{
	int i,j;
	jvstream_ability_t ability;
	mivp->st_rl_attr.stAlarmOutRL.bStarting = mivplist[channelid].st_rl_attr.stAlarmOutRL.bStarting;
	mivp->st_rl_attr.nAlpha  = VALIDVALUE (mivp->st_rl_attr.nAlpha , 0, 100);

	mivp->st_rl_attr.nSen = VALIDVALUE (mivp->st_rl_attr.nSen , 0, 100);

	jv_stream_get_ability(channelid, &ability);

	for (i=0;i<MAX_IVP_REGION_NUM;i++)
	{
		if(mivp->st_rl_attr.stRegion[i].nCnt > 1 && mivp->st_rl_attr.stRegion[i].nCnt < MAX_POINT_NUM)
		{
			for(j=0;j<mivp->st_rl_attr.stRegion[i].nCnt;j++)
			{
				mivp->st_rl_attr.stRegion[i].stPoints[j].x = VALIDVALUE (mivp->st_rl_attr.stRegion[i].stPoints[j].x, 0, ability.inputRes.width);
				mivp->st_rl_attr.stRegion[i].stPoints[j].y = VALIDVALUE (mivp->st_rl_attr.stRegion[i].stPoints[j].y, 0, ability.inputRes.height);
			}
		}
	}

	if (mivp->st_rl_attr.bEnable == TRUE )
	{
		if( mivplist[channelid].st_rl_attr.bEnable == FALSE)
		{
			printf("IVP start enable\n");
		}
		if (mivplist[channelid].st_rl_attr.stAlarmOutRL.bOutEMail == 0  && mivp->st_rl_attr.stAlarmOutRL.bOutEMail == 1)
		{
			printf("IVP SendMail Enable\n");
		}
		else if(mivplist[channelid].st_rl_attr.stAlarmOutRL.bOutEMail == 1  && mivp->st_rl_attr.stAlarmOutRL.bOutEMail == 0)
		{
			printf("IVP SendMail Disabled\n");
		}
	}
	else
	{
		if ( mivplist[channelid].st_rl_attr.bEnable == TRUE)
		{
			printf("IVP Stop\n");
		}
	}
	memcpy(&mivplist[channelid],mivp,sizeof(MIVP_t));
	printf(" %s line %d   rl enable %d count enable %d  cnt %d\n ", __func__,__LINE__,  mivplist[0].st_rl_attr.bEnable,mivplist[0].st_count_attr.bOpenCount,mivplist[0].st_rl_attr.nRgnCnt);
#ifdef  XW_MMVA_SUPPORT
	mmva_push_param(&mivplist[channelid]);
	for(i=0;i<HWINFO_STREAM_CNT;i++)		//刷新计数的OSD
		mchnosd_flush(i);
	printf("Line Count  FLUSH OSD \n ");
	if( mivplist[0].st_rl_attr.stAlarmOutRL.bEnableRecord)
		mrecord_set(0, ivping, TRUE);//SD卡录像开启
	else
		mrecord_set(0, ivping, FALSE);//SD卡录像开启
//	WriteConfigInfo();
	
#endif
	return 0;
}

/**
 *@brief 获取参数，指针方式省内存
 *@param channelid 频道号
 */
PMIVP_t  mivp_get_info(int channelid)
{

	return &mivplist[channelid];
}


/**
 *@brief 获取参数
 *@param channelid 频道号
 *@param mivp 要设置的属性结构体
 *@return 0 成功，-1 失败
 *
 */
int mivp_get_param(int channelid, MIVP_t *mivp)
{
	memcpy(mivp, &(mivplist[channelid]), sizeof(MIVP_t));
	return 0;
}

/*
 * @brief 设置黑天白天模式
 * bNightMode 1夜间模式，0白天模式
 */
int mivp_set_day_night_mode(int bNightMode)
{
#ifdef IVP_SUPPORT
	if(mivphandle)
		ivpSetDayNightMode(mivphandle,bNightMode);
#endif
#ifdef IVP_COUNT_SUPPORT
	if(mivp_count_bsupport())
	{
		if(mivphandle_count)
			ivpSetDayNightMode(mivphandle_count,bNightMode);
	}
#endif
	return 0;
}

/**
 *@brief 重启
 *	在#mivp_set_param之后，调用本函数
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mivp_restart(int chn)
{
	mivp_stop(chn);
	mivp_start(chn);
	return 0;
}

/*设置检测灵敏度0~100,值越大越灵敏,缺省为90*/
int mivp_SetSensitivity(int chn, int sensitivity)
{
#ifdef IVP_SUPPORT
	if(mivphandle)
	{
		sensitivity = VALIDVALUE (sensitivity , 0, 100);
		sensitivity = 100 - sensitivity;//大小转换
		printf("##############mivp_SetSensitivity:%d\n",sensitivity);
		ivpSetSensitivity(mivphandle,sensitivity);
	}
#else
	#ifdef  XW_MMVA_SUPPORT
	mmva_set_sensitivity(sensitivity);
	#endif

#endif
	return 0;
}

/*设置防区内停留多长时间报警,单位秒,缺省为0*/
int mivp_SetStaytime(int chn, int time)
{
#ifdef IVP_SUPPORT
	if(mivphandle)
	{
		printf("##############mivp_SetStaytime:%d\n",time);
		ivpSetStaytime(mivphandle,time);
	}
#endif
	return 0;
}

/*设置报警持续多长时间报警,单位秒,缺省为20*/
int mivp_SetAlarmtime(int chn, int time)
{
#ifdef IVP_SUPPORT
	if(mivphandle)
	{
		printf("##############mivp_SetAlarmtime:%d\n",time);
		ivpSetAlarmtime(mivphandle,time);
	}
#else
//	#ifdef  XW_MMVA_SUPPORT
//	mmva_set_alarmtime(time);
//	#endif

#endif

	
	return 0;
}


/*设置运动检测阈值*/
int mivp_SetThreshold(int chn, int value)
{
#ifdef IVP_SUPPORT
	if(mivphandle)
	{
		printf("##############mivp_SetThreshold:%d\n",value);
		ivpSetThreshold(mivphandle,value);
	}
#endif
	return 0;
}

/**
 *@brief 暂停/启动 智能分析
 *@param channelid 频道号
 *@param mode 0暂停，1启动
 *@return 0 成功，-1 失败
 */
int mivp_pause(int channelid, int mode)
{
#ifdef IVP_SUPPORT
	ivpPause(0,mode);
#endif
	return 0;
}

int mivp_bsupport()
{
#if( (defined IVP_SUPPORT) ||(defined XW_MMVA_SUPPORT))

	JVRotate_e rotate = msensor_get_rotate();
	if(rotate == JVSENSOR_ROTATE_90||rotate == JVSENSOR_ROTATE_270)
		return 0;

#if  (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100)
	MD md;
	mdetect_get_param(0, &md);
	if(md.bEnable)
		return 0;
#endif
	return hwinfo.bSupportMVA;


#endif
	return 0;
}

int mivp_rl_bsupport()
{
#if (defined XW_MMVA_SUPPORT)
	return hwinfo.bSupportReginLine;
#endif

#ifdef IVP_RL_SUPPORT
	if(strstr(hwinfo.product,"STC") || strstr(hwinfo.product,"VR"))
		return 0;
	return 1;
#endif
	return 0;
}

int mivp_count_bsupport()
{
#if (defined XW_MMVA_SUPPORT)
	return hwinfo.bSupportLineCount;

#endif

#ifdef IVP_COUNT_SUPPORT
	if(bIVP_COUNT_SUPPORT)
	{
		if(mivp_bsupport())
			//if(strstr(hwinfo.product,"STC"))
				return 1;
		return 0;
	}
#endif
	return 0;
}

int mivp_detect_bsupport()
{
	return 0;
}

/********************************************************遮挡报警**********************************************************/
int mivp_hide_bsupport()
{
#if( defined XW_MMVA_SUPPORT)
	return hwinfo.bSupportHideDetect;
#endif

#ifdef IVP_HIDE_SUPPORT
	return 1;
#endif
	return 0;
}

int mivp_hide_start(int chn)
{
#ifdef IVP_HIDE_SUPPORT
	if (!mivp_hide_bsupport())
		return 0;
	if (!mivplist[chn].st_hide_attr.bHideEnable)
		return 0;
	if (mivphandle == NULL)	//没有开启智能分析
	{
		U32 IvpMode = 0;
		mivphandle = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	if (mivphandle)
	{
		int index = 1;
		int pointCnt = 3;
		IVP_POINT points[3];
		points[0].x = 10;
		points[0].y = 10;
		points[1].x = 50;
		points[1].y = 50;
		points[2].x = 10;
		points[2].y = 50;

#if 0
        if(mivplist[chn].st_rl_attr.nRgnCnt<=0)	//智能分析没有框，从0开始
			index = 0;
		else
			index = mivplist[chn].st_rl_attr.nRgnCnt;
#endif//by zs 因功能互斥，规则号用0即可，防止越界
		ivpRegisterCallbk(mivphandle, __IvpAlarm);
		jvstream_ability_t ability;
		jv_stream_get_ability(chn, &ability);
		ivpAddRule(mivphandle, index, e_IVP_CHECK_MODE_HIDE,pointCnt, points, ability.inputRes.width, ability.inputRes.height);
		int threshold = 100 - mivplist[chn].st_hide_attr.nThreshold;
		printf("---------------------------------threshold:%d\n",threshold);
		ivpSetHideSensitivity(mivphandle, threshold);
	}
	printf("IVP Hide start Success\n");
#endif
	return 0;
}

int mivp_hide_flush(int chn)
{
#ifdef IVP_HIDE_SUPPORT
		if(mivphandle)
		{
			if(mivplist[chn].st_hide_attr.bHideEnable == TRUE)
			{
				int threshold = 100 - mivplist[chn].st_hide_attr.nThreshold;
				printf("---------------------------------threshold:%d\n",threshold);
				ivpSetHideSensitivity(mivphandle, threshold);
			}
		}
#endif
	return 0;

}


int mivp_hide_stop(int chn)
{
	int ret;
#ifdef IVP_HIDE_SUPPORT
	if(mivphandle)
	{
		ret = ivpRelease(mivphandle);
		printf("IVP Hide stop : %d\n",ret);
	}
	mivphandle = NULL;
#endif
	return 0;
}

int mivp_hide_get_param(int chn, MIVP_HIDE_t *attr)
{
	memcpy(attr,&mivplist[chn].st_hide_attr,sizeof(MIVP_HIDE_t));
	return 0;
}
int mivp_hide_set_param(int chn, MIVP_HIDE_t *attr)
{
	memcpy(&mivplist[chn].st_hide_attr,attr,sizeof(MIVP_HIDE_t));
	return 0;
}
/********************************************************遮挡报警**********************************************************/
/*********************************************************人群密度估计****************************************************************/

static void __IvpCdeFinishCalc(void *ivp, double rate, struct timeval tv)
{
#ifdef IVP_CDE_SUPPORT
	if(mivplist[0].st_cde_attr.nCDERate != (int)(rate*100))
	{
		mivplist[0].st_cde_attr.nCDERate= (int)(rate*100);
//		printf("FinishCalc	rate=%d\n", mivplist[0].st_cde_attr.nCDERate);
		mchnosd_ivp_cde_draw(mivplist[0].st_cde_attr.nCDERate);
	}
//	printf("FinishCalc	rate=%d\n", mivplist[0].st_cde_attr.nCDERate);
#endif
}

int mivp_cde_get_rate()
{
#ifdef IVP_CDE_SUPPORT
	//printf("Get	rate=%d\n", mivplist[0].st_pde_attr.nPDERate);
	return mivplist[0].st_cde_attr.nCDERate;
#else
	return 0;
#endif
}

static void __IvpCdeAlarm(void *ivp, double rate, struct timeval tv)
{
	if (!mivplist[0].st_cde_attr.bEnable)
		return ;
	if(mivplist[0].st_cde_attr.nCDERate != (int)(rate*100))
	{
//		printf("Alarm	rate=%d\n", mivplist[0].st_cde_attr.nCDERate);
		mivplist[0].st_cde_attr.nCDERate= (int)(rate*100);
//		printf("------------------------------------------------------->alarm:%d\n",mivplist[0].st_cde_attr.nCDERate);
		mchnosd_ivp_cde_draw(mivplist[0].st_cde_attr.nCDERate);			
	}	
	if(mivplist[0].st_cde_attr.nCDEThreshold < mivplist[0].st_cde_attr.nCDERate)
	{
		IVP_Release_State = e_IPV_ALARM_TYPE_CROWD_DENSITY;
		__IvpAlarmOut(&mivplist[0].st_cde_attr.stCDEAlarmOut, e_IPV_ALARM_TYPE_CROWD_DENSITY);	//报警
	}
	else if(mivplist[0].st_cde_attr.nCDEThreshold > mivplist[0].st_cde_attr.nCDERate)	
	{
		IVP_Release_State = 0;
		__IvpAlarmOut(&mivplist[0].st_cde_attr.stCDEAlarmOut, 0);
	}
/*	char Data[64] = { 0 };
	//char str[64];
	Data[0] = 'I';
	Data[1] = 'V';
	Data[2] = 'P';
	Data[3] = 'C';
	Data[4] = 'D';
	Data[5] = 'E';
	char sendstr[64];
	sprintf(sendstr, "CDErate=%d", (int)(rate*100));
	memcpy(Data + 6, sendstr, 64 - 6);
//	printf("ivp Pde send to nvr:%s\n", Data);
	remotecfg_send_self_data(0, Data, 64);
	//sprintf(str, "rate=%d %%", (int)(var_rate*100));
	//_alarm_send_delay(0, (void *)str);
*/	
	//printf("<<<<<<<<<<<<IVP_PDE_ALARM_START:pde rate:%f,check_time:%ld>>>>>>>>>>>>>\n",rate,tv.tv_sec/3600);
}


int mivp_cde_bsupport()
{
#ifdef IVP_CDE_SUPPORT
	return 1;
#endif
	return 0;
}

int mivp_ocl_bsupport()
{
#ifdef IVP_OCL_SUPPORT
	return 1;
#endif
	return 0;
}

#define MAX_RECT_NUM 3

int mivp_cde_correct(int chn)
{	
#ifdef IVP_CDE_SUPPORT
	//printf("[%s] line:%d	\n",__FUNCTION__,__LINE__);
	int i;
	IVP_RECT points[MAX_RECT_NUM] = {{0}};
	jvstream_ability_t ability;
	jv_stream_get_ability(chn, &ability);
	jv_stream_attr attr;
	jv_stream_get_attr(chn, &attr);
	unsigned int viWidth, viHeight;
	jv_stream_get_vi_resolution(chn, &viWidth, &viHeight);
	
	if(mivphandle_cde != NULL)
	{
		if(mivplist[chn].st_cde_attr.nRgnCnt>=3)
		{
			//printf("stRect.nCnt=%d	[", mivplist[chn].st_pde_attr.stRect.nCnt);
			for(i=1; i<mivplist[0].st_cde_attr.nRgnCnt; i++)
		    {
				__build_point_vi(mivplist[chn].st_cde_attr.stRegion[i].stPoints[0].x,mivplist[chn].st_cde_attr.stRegion[i].stPoints[0].y,&points[i-1].x,&points[i-1].y,viWidth,viHeight,attr.width,attr.height);
				__build_point_vi(mivplist[chn].st_cde_attr.stRegion[i].stPoints[2].x,mivplist[chn].st_cde_attr.stRegion[i].stPoints[2].y,&points[i-1].x2,&points[i-1].y2,viWidth,viHeight,attr.width,attr.height);
				points[i-1].x = points[i-1].x>viWidth?viWidth:points[i-1].x;
				points[i-1].y = points[i-1].y>viHeight?viHeight:points[i-1].y;
				points[i-1].x2 = points[i-1].x2>viWidth?viWidth:points[i-1].x2;
				points[i-1].y2 = points[i-1].y2>viHeight?viHeight:points[i-1].y2;
	
				printf(" i:%d: %d,%d,%d,%d  \n",i,points[i-1].x,points[i-1].y,points[i-1].x2,points[i-1].y2);
		    }
			//printf("]");
			ivpAddDensityEstimateRect(mivphandle_cde, &points[0], mivplist[chn].st_cde_attr.nRgnCnt-1, ability.inputRes.width, ability.inputRes.height);
		}
	}
	//printf("\n");
#endif
	return 0;
}


int mivp_cde_start(int chn)
{
#ifdef IVP_CDE_SUPPORT
	if(mivplist[chn].st_cde_attr.nRgnCnt<=0)
		return -1;
	if(!mivplist[chn].st_cde_attr.bEnable)
		return -1;
	if(mivplist[chn].st_cde_attr.nRgnCnt>MAX_IVP_REGION_NUM)
		mivplist[chn].st_cde_attr.nRgnCnt = MAX_IVP_REGION_NUM;	//多出的不画

	if(!mivphandle_cde)
	{
		U32 IvpMode=0;
		if(mivplist[chn].st_cde_attr.bDrawFrame)
			IvpMode |= e_IVP_MODE_DRAW_FRAME;	
		IvpMode |= e_IVP_MODE_CROWD_DENSITY_ESTIMATE;	//开启人员密度检测
		mivphandle_cde = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	else
		return -1;
	if(mivphandle_cde)
	{
		printf("CDE start\n");
		int i;
		IVP_POINT points[MAX_POINT_NUM] = {{0}};
		jvstream_ability_t ability;
		jv_stream_get_ability(chn, &ability);
		IVP_CHECK_MODE mode;
		jv_stream_attr attr;
		jv_stream_get_attr(chn, &attr);
		unsigned int viWidth, viHeight;
		jv_stream_get_vi_resolution(0, &viWidth, &viHeight);
		//printf("[%s] line:%d	nCnt:%d", __FUNCTION__, __LINE__, mivplist[chn].st_pde_attr.stRegion.nCnt);
    	for(i=0;i<mivplist[chn].st_cde_attr.stRegion[0].nCnt;i++)
    	{
	   		__build_point_vi(mivplist[chn].st_cde_attr.stRegion[0].stPoints[i].x,mivplist[chn].st_cde_attr.stRegion[0].stPoints[i].y,&points[i].x,&points[i].y,viWidth,viHeight,attr.width,attr.height);
			points[i].x = points[i].x>viWidth?viWidth:points[i].x;
			points[i].y = points[i].y>viHeight?viHeight:points[i].y;
    	}
    	if(mivplist[chn].st_cde_attr.stRegion[0].nCnt>2)
    	{
			mode = e_IVP_CHECK_MODE_CROWD_DENSITY;
    	}
    	else
    		return -1;
		//printf("ivpAddRule	pde mode:0x%x\n",mode);
	    ivpAddRule(mivphandle_cde, 0, mode, mivplist[chn].st_cde_attr.stRegion[0].nCnt, points, ability.inputRes.width, ability.inputRes.height);

//		ivpRegisterCallbk(mivphandle_cde, __IvpAlarm);		
		ivpRegDensityEstimateAlarmCB(mivphandle_cde, __IvpCdeAlarm);
		ivpRegDensityEstimateFinishCB(mivphandle_cde,__IvpCdeFinishCalc);
		double threshold = ((double)mivplist[chn].st_cde_attr.nCDEThreshold)/100;
		printf("++++++++++++++++++++++++++++++threshold:%f\n",threshold);
		ivpSetDensityEstimateAlarmBias(mivphandle_cde, threshold);
		mivp_cde_correct(chn);
		__chnosd_ivpcde_flush(0);
	}
	else
		return -1;
#endif
	return 0;
}
int mivp_cde_stop(int chn)
{
	int ret;
#ifdef IVP_CDE_SUPPORT
	if(mivphandle_cde)
	{
		ret = ivpRelease(mivphandle_cde);
		printf("IVP CDE stop : %d\n",ret);
		__chnosd_ivpcde_flush(0);
	}
	mivphandle_cde = NULL;
#endif
	return 0;

}

int mivp_cde_flash(int chn)
{
#ifdef IVP_CDE_SUPPORT
	if(mivphandle_cde)
	{
		if(mivplist[chn].st_cde_attr.bEnable == TRUE)
		{
			double threshold = ((double)mivplist[chn].st_cde_attr.nCDEThreshold)/100;
//			printf("---------------------------------threshold:%f\n",threshold);
			ivpSetDensityEstimateAlarmBias(mivphandle_cde, threshold);
		}
	}
#endif
	return 0;
}


/*********************************************************人群密度估计**************************************************************/

/*********************************************************拥挤检测******************************************************************/
static OCLCB __IvpOCLAlarm(void * parg)
{
    OCLAlarm * alarm_signal = (OCLAlarm *) parg;
    printf("-------------------------------------ocl alarm[%d]\n", *alarm_signal);

    if(0 == *alarm_signal)
    {
        IVP_Release_State = 0;
        __IvpAlarmOut(&mivplist[0].st_ocl_attr.stOCLAlarmOut, 0);	//消警
    }
    else
    {
        IVP_Release_State = e_IPV_ALARM_TYPE_OCL;
        __IvpAlarmOut(&mivplist[0].st_ocl_attr.stOCLAlarmOut, e_IPV_ALARM_TYPE_OCL);	//报警
    }

    return NULL;
}

void * gh_ocl = NULL;

MMLRect coor_transform(MMLRect rect, MMLSize src_res, MMLSize dst_res) 
{    
    MMLRect dst_rect;    
    float ratio_x = (float) dst_res.width / src_res.width;    
    float ratio_y = (float) dst_res.height / src_res.height;    
    dst_rect.x = rect.x * ratio_x;    
    dst_rect.y = rect.y * ratio_y;    
    dst_rect.width = rect.width * ratio_x;    
    dst_rect.height = rect.height * ratio_y;    
    return dst_rect;
}

int mivp_ocl_start(int chn)
{
#ifdef IVP_OCL_SUPPORT    
	if((mivplist[chn].st_ocl_attr.nRgnCnt<=0)
        ||(!mivplist[chn].st_ocl_attr.bEnable))
		return -1;

    if(NULL == gh_ocl)
    {
        OCLProp ocl_prop;
        MMLRect rect;

        jvstream_ability_t ability;
        jv_stream_get_ability(0, &ability);

        int nExist = mivplist[chn].st_ocl_attr.nExist;
        int nLimit = mivplist[chn].st_ocl_attr.nLimit;
        BOOL bDrawframe = mivplist[chn].st_ocl_attr.bDrawFrame;

        rect.x = mivplist[chn].st_ocl_attr.stRegion.stPoints[0].x;
        rect.y = mivplist[chn].st_ocl_attr.stRegion.stPoints[0].y;
        rect.width = mivplist[chn].st_ocl_attr.stRegion.stPoints[1].x - mivplist[chn].st_ocl_attr.stRegion.stPoints[0].x;
        rect.height = mivplist[chn].st_ocl_attr.stRegion.stPoints[1].y - mivplist[chn].st_ocl_attr.stRegion.stPoints[0].y;

        printf("OCL rect~~~~~~~x[%d],y[%d],width[%d],height[%d],exist[%d],limit[%d]\n",
            rect.x,
            rect.y,
            rect.width,
            rect.height,
            nExist,
            nLimit);
        

        MMLSize src_res = {ability.inputRes.width, ability.inputRes.height};
        MMLSize dst_res = {320, 180}; 
        //vi是16:9的话设置 320 * 180，vi是4:3的话设置成 320 * 240
        if(((float)src_res.width)/((float)src_res.height) < 1.7)
        {
            dst_res.width = 320;
            dst_res.height = 240;
        }

        ocl_prop.img_size.width = dst_res.width;
        ocl_prop.img_size.height = dst_res.height;
        
        gh_ocl = oclInit(ocl_prop);
        printf("gh_ocl: %#x\n", (unsigned int)gh_ocl);

        if(bDrawframe != oclIsDrawEnable(gh_ocl))
        {
            if(bDrawframe)
            {
                oclDrawEnable(gh_ocl);
            }
            else
            {
                oclDrawDisable(gh_ocl);
            }
        }
        oclRegCB(gh_ocl, (OCLCB)__IvpOCLAlarm);

        MMLRect dst_rect = coor_transform(rect, src_res, dst_res);
        oclSetRegion(gh_ocl, dst_rect);
        oclSetTargetNum(gh_ocl, nExist);
        oclSetTargetBias(gh_ocl, nLimit);
    }
#endif
	return 0;
}
int mivp_ocl_stop(int chn)
{
	int ret;
#ifdef IVP_OCL_SUPPORT
    if(NULL != gh_ocl)
    {
        oclDrawDisable(gh_ocl);
        oclFinal(&gh_ocl);
        gh_ocl = NULL;
    }
#endif
	return 0;
}
int mivp_ocl_flush(int chn)
{
#ifdef IVP_OCL_SUPPORT
    if(NULL != gh_ocl)
    {
        MMLRect rect;

        jvstream_ability_t ability;
        jv_stream_get_ability(0, &ability);

        int nExist = mivplist[chn].st_ocl_attr.nExist;
        int nLimit = mivplist[chn].st_ocl_attr.nLimit;
        BOOL bDrawframe = mivplist[chn].st_ocl_attr.bDrawFrame;

        rect.x = mivplist[chn].st_ocl_attr.stRegion.stPoints[0].x;
        rect.y = mivplist[chn].st_ocl_attr.stRegion.stPoints[0].y;
        rect.width = mivplist[chn].st_ocl_attr.stRegion.stPoints[1].x - mivplist[chn].st_ocl_attr.stRegion.stPoints[0].x;
        rect.height = mivplist[chn].st_ocl_attr.stRegion.stPoints[1].y - mivplist[chn].st_ocl_attr.stRegion.stPoints[0].y;

        printf("OCL rect~~~~~~~x[%d],y[%d],width[%d],height[%d],nexist[%d],nlimit[%d]\n",
            rect.x,
            rect.y,
            rect.width,
            rect.height,
            nExist,
            nLimit);
        

        MMLSize src_res = {ability.inputRes.width, ability.inputRes.height};
        MMLSize dst_res = {320, 180}; 
        //vi是16:9的话设置 320 * 180，vi是4:3的话设置成 320 * 240
        if(((float)src_res.width)/((float)src_res.height) < 1.7)
        {
            dst_res.width = 320;
            dst_res.height = 240;
        }

        if(bDrawframe != oclIsDrawEnable(gh_ocl))
        {
            if(bDrawframe)
            {
                oclDrawEnable(gh_ocl);
            }
            else
            {
                oclDrawDisable(gh_ocl);
            }
        }
        
        MMLRect dst_rect = coor_transform(rect, src_res, dst_res);
        oclSetRegion(gh_ocl, dst_rect);
        oclSetTargetNum(gh_ocl, nExist);
        oclSetTargetBias(gh_ocl, nLimit);
    }
#endif
	return 0;
}
/*********************************************************拥挤检测******************************************************************/
/*********************************************************快速移动报警**************************************************************/
int mivp_fm_bsupport()
{
#ifdef IVP_FM_SUPPORT
		return 1;
#endif
		return 0;

}
int mivp_fm_start(int chn)
{
#ifdef IVP_FM_SUPPORT
	if(mivplist[chn].st_fm_attr.nRgnCnt<=0)
		return -1;
	if(!mivplist[chn].st_fm_attr.bEnable)
		return -1;
	if(mivplist[chn].st_fm_attr.nRgnCnt>MAX_IVP_REGION_NUM)
		mivplist[chn].st_fm_attr.nRgnCnt = MAX_IVP_REGION_NUM;	//多出的不画

	if(!mivphandle)
	{
		U32 IvpMode=0;
		if(mivplist[chn].st_fm_attr.bDrawFrame)
		IvpMode |= e_IVP_MODE_DRAW_FRAME;
		if (mivplist[chn].st_fm_attr.bFlushFrame && mivplist[chn].st_fm_attr.bDrawFrame)
		IvpMode |= e_IVP_MODE_FLUSH_FRAME;
		if (mivplist[chn].st_fm_attr.bMarkObject)
		IvpMode |= e_IVP_MODE_MARK_OBJECT;
		if (mivplist[chn].st_fm_attr.bMarkAll)
		IvpMode |= e_IVP_MODE_MARK_ALL;
		IvpMode |= e_IVP_MODE_MARK_SMOOTH;	//默认开启平滑模式
		IvpMode |= e_IVP_MODE_FAST_MOVE;	//开启快速移动
		//mivplist[chn].st_rl_attr.nAlpha = 0;
		mivphandle = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	else
		return -1;
	if(mivphandle)
	{
		printf("IVP start\n");
	    int i,j;
	    if(mivplist[chn].st_fm_attr.nRgnCnt<=0)
	    	return -2;

	    ivpRegisterCallbk(mivphandle, __IvpAlarm);

	    IVP_POINT points[MAX_POINT_NUM] = {{0}};
	    jvstream_ability_t ability;
	    jv_stream_get_ability(chn, &ability);

	    IVP_CHECK_MODE mode;
	    jv_stream_attr attr;
	    jv_stream_get_attr(chn, &attr);
	    unsigned int viWidth, viHeight;
	    viWidth = ability.inputRes.width;
	    viHeight = ability.inputRes.height;

	    for(i=0;i<mivplist[chn].st_fm_attr.nRgnCnt;i++)
	    {
	    	for(j=0;j<mivplist[chn].st_fm_attr.stRegion[i].nCnt;j++)
	    	{
	    		__build_point_vi(mivplist[chn].st_fm_attr.stRegion[i].stPoints[j].x,mivplist[chn].st_fm_attr.stRegion[i].stPoints[j].y,&points[j].x,&points[j].y,viWidth,viHeight,attr.width,attr.height);
				points[j].x = points[j].x>viWidth?viWidth:points[j].x;
				points[j].y = points[j].y>viHeight?viHeight:points[j].y;
	    	}
	    	if(mivplist[chn].st_fm_attr.stRegion[i].nCnt>2)
	    	{
				mode = e_IVP_CHECK_MODE_FAST_MOVE;
	    	}
	    	else
	    		return -1;

	    	ivpAddRule(mivphandle, i, mode, mivplist[chn].st_fm_attr.stRegion[i].nCnt, points, viWidth, viHeight);
	    	memset(points,0,MAX_POINT_NUM);
	    }
	    printf("##############Fast Move:Sen:%d,StayTime:%d,Threshold:%d\n",mivplist[chn].st_fm_attr.nSen,mivplist[chn].st_fm_attr.nStayTime,mivplist[chn].st_fm_attr.nThreshold);
	    ivpSetSensitivity(mivphandle,100 - mivplist[chn].st_fm_attr.nSen);
	    ivpSetStaytime(mivphandle,mivplist[chn].st_fm_attr.nStayTime);
	    ivpSetThreshold(mivphandle,mivplist[chn].st_fm_attr.nThreshold);
		ivpFastMovectl(mivphandle, e_IVP_FAST_MOVE_CTL_SPEED_LEVEL, mivplist[chn].st_fm_attr.nSpeedLevel);
	}
	else
		return -1;
#endif
	return 0;
}
int mivp_fm_flash(int chn)
{
#ifdef IVP_FM_SUPPORT
	if(mivphandle)
	{
		if(mivplist[chn].st_fm_attr.bEnable == TRUE)
		{
			mivp_SetSensitivity(0, mivplist[chn].st_fm_attr.nSen);
			mivp_SetStaytime(0, mivplist[chn].st_fm_attr.nStayTime);
			mivp_SetThreshold(0, mivplist[chn].st_fm_attr.nThreshold);
			ivpFastMovectl(mivphandle, e_IVP_FAST_MOVE_CTL_SPEED_LEVEL, mivplist[chn].st_fm_attr.nSpeedLevel);
		}
	}
#endif
	return 0;
}

int mivp_fm_stop(int chn)
{
	int ret;
#ifdef IVP_FM_SUPPORT
	if(mivphandle)
	{
		ret = ivpRelease(mivphandle);
		printf("IVP FM stop : %d\n",ret);
	}
	mivphandle = NULL;
#endif
	return 0;

}

/*********************************************************快速移动报警**************************************************************/

/****************************************************场景变更报警*****************************************/
int mivp_sc_bsupport()
{
#ifdef IVP_SC_SUPPORT
	return 1;
#endif
	return 0;
}
int mivp_sc_start(int chn)
{
#ifdef IVP_SC_SUPPORT
	if (!mivp_sc_bsupport())
		return 0;
	if (!mivplist[chn].st_sc_attr.bEnable)
		return 0;
	if (mivphandle_sc == NULL)	//没有开启智能分析
	{
		U32 IvpMode = e_IVP_MODE_SCENE_CHANGE;
		mivphandle_sc = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	if (mivphandle_sc)
	{
		int index = 0;
		int pointCnt = 3;
		IVP_POINT points[3];
		points[0].x = 10;
		points[0].y = 10;
		points[1].x = 50;
		points[1].y = 50;
		points[2].x = 10;
		points[2].y = 50;
		
		ivpRegisterCallbk(mivphandle_sc, __IvpAlarm);
		jvstream_ability_t ability;
		jv_stream_get_ability(chn, &ability);
		ivpAddRule(mivphandle_sc, index, e_IVP_CHECK_MODE_SCENE_CHANGE,pointCnt, points, ability.inputRes.width, ability.inputRes.height);
		float threshold = 0.0625 - mivplist[chn].st_sc_attr.nThreshold * 0.0005;
		printf("---------->threshold:%f\n",threshold);
		ivpSceneChangeSetThreshold(mivphandle_sc, threshold);
		ivpSceneChangeSetDuration(mivphandle_sc, mivplist[chn].st_sc_attr.duration);

	}
	printf("IVP SCENE CHANGE start Success\n");

#endif
	return 0;
}
int mivp_sc_flush(int chn)
{
#ifdef IVP_SC_SUPPORT
	if(mivphandle_sc)
	{
		float threshold = 0.0625 - mivplist[chn].st_sc_attr.nThreshold * 0.0005;
		printf("---------->threshold:%f\n",threshold);
		ivpSceneChangeSetThreshold(mivphandle_sc, threshold);
		ivpSceneChangeSetDuration(mivphandle_sc, mivplist[chn].st_sc_attr.duration);
	}
#endif
	return 0;
}
int mivp_sc_stop(int chn)
{
	int ret;
#ifdef IVP_SC_SUPPORT
	if(mivphandle_sc)
	{
		ret = ivpRelease(mivphandle_sc);
		printf("IVP SC stop : %d\n",ret);
	}
	mivphandle_sc = NULL;
#endif
	return 0;
}


/****************************************************场景变更报警*****************************************/

/*********************************************************徘徊报警**************************************************************/
int mivp_hover_bsupport()
{
#ifdef IVP_HOVER_SUPPORT
	return 1;
#endif
	return 0;
}


int mivp_hover_start(int chn)
{
#ifdef IVP_HOVER_SUPPORT
	if (!mivp_hover_bsupport())
		return 0;
	if (!mivplist[chn].st_hover_attr.bEnable)
		return 0;
	if (mivphandle == NULL)	//没有开启智能分析
	{
		U32 IvpMode = e_IVP_MODE_HOVER_DETECTION;
		mivphandle = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	if (mivphandle)
	{
		int index = 0;
		int pointCnt = 3;
		IVP_POINT points[3];
		points[0].x = 10;
		points[0].y = 10;
		points[1].x = 50;
		points[1].y = 50;
		points[2].x = 10;
		points[2].y = 50;

		ivpRegisterCallbk(mivphandle, __IvpAlarm);
		jvstream_ability_t ability;
		jv_stream_get_ability(chn, &ability);
		ivpAddRule(mivphandle, index, e_IVP_CHECK_MODE_HOVER,pointCnt, points, ability.inputRes.width, ability.inputRes.height);
	}
	printf("IVP HOVER start Success\n");
#endif
	return 0;
}

int mivp_hover_stop(int chn)
{
	int ret;
#ifdef IVP_HOVER_SUPPORT
	if(mivphandle)
	{
		ret = ivpRelease(mivphandle);
		printf("IVP Hide stop : %d\n",ret);
	}
	mivphandle = NULL;
#endif
	return 0;
}

/*********************************************************徘徊报警**************************************************************/
/*********************************************************烟火报警**************************************************************/

int mivp_fire_bsupport()
{
#ifdef IVP_FIRE_SUPPORT
	return 1;
#endif
	return 0;
}
int mivp_fire_start(int chn)
{
#ifdef IVP_FIRE_SUPPORT
	if (!mivp_fire_bsupport())
		return 0;
	if (!mivplist[chn].st_fire_attr.bEnable)
		return 0;
	if (mivphandle == NULL)	//没有开启智能分析
	{
		U32 IvpMode = e_IVP_MODE_FIRE_DETECT;
		mivphandle = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	ivpPause(mivphandle, 0);
	ivpRegisterCallbk(mivphandle, __IvpAlarm);
	ivpFireSmokeSetSensitivity(mivphandle, 100 - mivplist[chn].st_fire_attr.sensitivity);	//越小越灵敏
	printf("IVP FIRE start Success\n");
#endif
	return 0;
}
int mivp_fire_flash(int chn)
{
#ifdef IVP_FIRE_SUPPORT
	ivpFireSmokeSetSensitivity(mivphandle, 100 - mivplist[chn].st_fire_attr.sensitivity);	//越小越灵敏
#endif
	return 0;
}

int mivp_fire_stop(int chn)
{
	int ret;
#ifdef IVP_FIRE_SUPPORT
	if(mivphandle)
	{
		ret = ivpRelease(mivphandle);
		printf("IVP Hide stop : %d\n",ret);
	}
	mivphandle = NULL;
#endif
	return 0;
}

/*********************************************************烟火报警**************************************************************/
/****************************************************虚焦检测*****************************************/
int mivp_vf_bsupport()
{
#ifdef IVP_VF_SUPPORT
	return 1;
#endif
	return 0;
}
int mivp_vf_start(int chn)
{
#ifdef IVP_VF_SUPPORT
	if (!mivp_vf_bsupport())
		return 0;
	if (!mivplist[chn].st_vf_attr.bEnable)
		return 0;
	if (mivphandle_vf == NULL)	//没有开启智能分析
	{
		U32 IvpMode = 0;
		mivphandle_vf = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	if (mivphandle_vf)
	{
		int index = 0;
		int pointCnt = 3;
		IVP_POINT points[3];
		points[0].x = 10;
		points[0].y = 10;
		points[1].x = 50;
		points[1].y = 50;
		points[2].x = 10;
		points[2].y = 50;
		
		ivpRegisterCallbk(mivphandle_vf, __IvpAlarm);
		jvstream_ability_t ability;
		jv_stream_get_ability(chn, &ability);
		ivpAddRule(mivphandle_vf, index, e_IVP_CHECK_MODE_VIRTUAL_FOCUS,pointCnt, points, ability.inputRes.width, ability.inputRes.height);
		int threshold = 100 - mivplist[chn].st_vf_attr.nThreshold;
		printf("---------------------------------threshold:%d\n",threshold);
		ivpSetFocusSensitivity(mivphandle_vf, threshold);
	}
	printf("IVP VF start Success\n");
#endif
	return 0;
}

int mivp_vf_flush(int chn)
{
#ifdef IVP_VF_SUPPORT
		if(mivphandle_vf)
		{
			if(mivplist[chn].st_vf_attr.bEnable == TRUE)
			{
				int threshold = 100 - mivplist[chn].st_vf_attr.nThreshold;
				printf("---------------------------------threshold:%d\n",threshold);
				ivpSetFocusSensitivity(mivphandle_vf, threshold);
			}
		}
#endif
	return 0;

}


int mivp_vf_stop(int chn)
{
	int ret;
#ifdef IVP_VF_SUPPORT
	if(mivphandle_vf)
	{
		ret = ivpRelease(mivphandle_vf);
		printf("IVP VF stop : %d\n",ret);
	}
	mivphandle_vf = NULL;
#endif
	return 0;
}
/****************************************************虚焦检测*****************************************/
/****************************************************热度图*****************************************/
int mivp_hm_bsupport()
{
#ifdef IVP_HM_SUPPORT
	return 1;
#endif
	return 0;
}
void hm_result(void *ivp, void * parg)
{
	 memcpy(&mivplist[0].st_hm_attr.image,(MMLImage*)parg,sizeof(MMLImage));
	 printf("---------------------------->>>hm call back\n");
}
int mivp_hm_start(int chn)
{
#ifdef IVP_HM_SUPPORT
	if (!mivp_hm_bsupport())
		return 0;
	if (!mivplist[chn].st_hm_attr.bEnable)
		return 0;
	if (mivphandle_hm == NULL)	//没有开启智能分析
	{
		U32 IvpMode = e_IVP_MODE_HEAT_MAP;
		mivphandle_hm = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	ivpHeatMapctl(mivphandle_hm, e_IVP_HEATMAP_CTL_TYPE_TIME, mivplist[chn].st_hm_attr.upCycle);
	ivpRegHeatMapCb(mivphandle_hm, (void*)hm_result);
	ivpPause(mivphandle_hm, 0);
	printf("IVP HM start Success\n");
#endif
	return 0;
}
int mivp_hm_stop(int chn)
{
	int ret;
#ifdef IVP_HM_SUPPORT
	if(mivphandle_hm)
	{
		ret = ivpRelease(mivphandle_hm);
		printf("IVP HM stop : %d\n",ret);
	}
	mivphandle_hm = NULL;
#endif
	return 0;
}


/****************************************************热度图*****************************************/
/****************************************************车牌识别*****************************************/

int mivp_lpr_bsupport()
{
#ifdef IVP_LPR_SUPPORT
	return 1;
#endif
	return 0;
}

void lpr_cb(void * ivp, const void* parg) 
{    
#ifdef IVP_LPR_SUPPORT
	printf("-----------------------------------lprcb\n");
	IVP_LPR_CB_DEFAULT_PARAM *param = (IVP_LPR_CB_DEFAULT_PARAM*) parg;   
	const char* lp = param->lp;
	printf("lp: %s\n", lp);    
	memcpy(mivplist[0].st_lpr_attr.lp_num,param->lp,sizeof(mivplist[0].st_lpr_attr.lp_num));
#endif
	return;
}

int mivp_lpr_start(int chn)
{
#ifdef IVP_LPR_SUPPORT
	if (!mivp_lpr_bsupport())
		return 0;
	if (!mivplist[chn].st_lpr_attr.bEnable)
		return 0;
	if (mivphandle_lpr == NULL)	//没有开启智能分析
	{
		U32 IvpMode = e_IVP_MODE_LPR;
		mivphandle_lpr = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	jvstream_ability_t ability;
	jv_stream_get_ability(chn, &ability);
	jv_stream_attr attr;
    jv_stream_get_attr(chn, &attr);
    unsigned int viWidth, viHeight;
    viWidth = ability.inputRes.width;
    viHeight = ability.inputRes.height;
	if (mivphandle_lpr)
	{	
		IVP_LPR_RECT r = {0, 210, 1680, 288};
		if(mivplist[chn].st_lpr_attr.stRegion.nCnt > 2)
		{
		//	r.x = mivplist[chn].st_lpr_attr.stRegion.stPoints[0].x;
		//	r.y = mivplist[chn].st_lpr_attr.stRegion.stPoints[0].y;
			__build_point_vi(mivplist[chn].st_lpr_attr.stRegion.stPoints[0].x,mivplist[chn].st_lpr_attr.stRegion.stPoints[0].y,&r.x,&r.y,viWidth,viHeight,attr.width,attr.height);
			r.x = r.x>viWidth?viWidth:r.x;
			r.y = r.y>viHeight?viHeight:r.y;
			r.width = 1680/1920 * viWidth;
			r.height = 288/1080 * viHeight;
		}
		
//		IVP_LPR_PARAM_WORK_MODE work_mode = e_IVP_LPR_WORK_MODE_INSIDE_TRIGGER;
//		IVP_LPR_PARAM_DIRECTION def_dir = e_IVP_LPR_DIR_CCW;
//		IVP_LPR_PARAM_DISPLAY_TYPE display = e_IVP_LPR_SHOW_ROI;
		int timeout = 300;			//目前没用
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_WORK_MODE, (void*) &mivplist[chn].st_lpr_attr.work_mode);
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_ROI, (void*) &r);
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_DIRECTION, (void*) &mivplist[chn].st_lpr_attr.def_dir);
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_DISPLAY_ROI, (void*) &mivplist[chn].st_lpr_attr.display);
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_TRIGGER_TIMEOUT, (void*) &timeout);
        ivplprregcb(mivphandle_lpr, e_IVP_LPR_CB_DEFAULT,(void*) lpr_cb);
        ivpPause(mivphandle_lpr, 0);
		printf("IVP LPR start Success\n");
	}

#endif
	return 0;
}
int mivp_lpr_flush(int chn)
{
#ifdef IVP_LPR_SUPPORT
	if (mivphandle_lpr)
	{
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_WORK_MODE, (void*) &mivplist[chn].st_lpr_attr.work_mode);
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_DIRECTION, (void*) &mivplist[chn].st_lpr_attr.def_dir);
		ivplprctl(mivphandle_lpr, e_IVP_LPR_PARAM_DISPLAY_ROI, (void*) &mivplist[chn].st_lpr_attr.display);
		printf("IVP LPR Flash Success\n");
	}
#endif
	return 0;
}

int mivp_lpr_stop(int chn)
{
#ifdef IVP_LPR_SUPPORT
	if(mivphandle_lpr)
	{
		ivpRelease(mivphandle_lpr);
	}
	mivphandle_lpr = NULL;
#endif
	return 0;
}
/****************************************************车牌识别*****************************************/

/********************************************************拿取遗留报警**********************************************************/
/*
 * take left 拿取遗留报警
 */
int mivp_tl_bsupport()
{
#ifdef IVP_TL_SUPPORT
	return 1;
#endif
	return 0;
}

int mivp_tl_start(int chn)
{
#ifdef IVP_TL_SUPPORT
	if(mivplist[chn].st_tl_attr.nTLRgnCnt<=0)
		return -1;
	if(!mivplist[chn].st_tl_attr.bTLEnable && !mivplist[chn].st_tl_attr.bHKEnable)
		return -1;
	if(mivplist[chn].st_tl_attr.nTLRgnCnt>MAX_IVP_REGION_NUM)
		mivplist[chn].st_tl_attr.nTLRgnCnt = MAX_IVP_REGION_NUM;	//多出的不画

	if(!mivphandle)
	{
		U32 IvpMode=0;
		if(mivplist[chn].st_tl_attr.bHKEnable)
		{
			if(mivplist[chn].st_tl_attr.bLEnable)
				mivplist[chn].st_tl_attr.nTLMode = 0;
			else if(mivplist[chn].st_tl_attr.bTEnable)
				mivplist[chn].st_tl_attr.nTLMode = 1;
		}
		if (mivplist[chn].st_tl_attr.nTLMode == 0)
			IvpMode |= e_IVP_MODE_ABANDONED_OBJ_DETECTION;
		else if(mivplist[chn].st_tl_attr.nTLMode == 1)
			IvpMode |= e_IVP_MODE_REMOVED_OBJ_DETECTION;

		IvpMode |= e_IVP_MODE_DRAW_FRAME;
		mivphandle = ivpStart(IvpMode, chn, 8);//像素格式PIXEL_FORMAT_RGB_1555
	}
	else
		return -1;
	if(mivphandle)
	{
		printf("IVP tl start\n");
		int i,j;
		IVP_POINT points[MAX_POINT_NUM] = {{0}};
		jvstream_ability_t ability;
		jv_stream_get_ability(chn, &ability);

		ivpRegisterCallbk(mivphandle, __IvpAlarm);

		IVP_CHECK_MODE mode;
		jv_stream_attr attr;
		jv_stream_get_attr(chn, &attr);
		unsigned int viWidth, viHeight;
		jv_stream_get_vi_resolution(0, &viWidth, &viHeight);

		for(i=0;i<mivplist[chn].st_tl_attr.nTLRgnCnt;i++)
		{
			if(mivplist[chn].st_tl_attr.bHKEnable)
			{
				if(((i == 0 || i == 1) && !mivplist[chn].st_tl_attr.bLEnable) || mivplist[chn].st_tl_attr.stTLRegion[i].nCnt < 3)
					continue;
				else if(((i == 2 || i == 3) && !mivplist[chn].st_tl_attr.bTEnable) || mivplist[chn].st_tl_attr.stTLRegion[i].nCnt < 3)
					continue;
			}
		    for(j=0;j<mivplist[chn].st_tl_attr.stTLRegion[i].nCnt;j++)
		    {
		//    	__build_point_vi(mivplist[chn].st_tl_attr.stTLRegion[i].stPoints[j].x,mivplist[chn].st_tl_attr.stTLRegion[i].stPoints[j].y,&points[j].x,&points[j].y,viWidth,viHeight,attr.width,attr.height);
				points[j].x = mivplist[chn].st_tl_attr.stTLRegion[i].stPoints[j].x;//* attr.width /  viWidth;;
	    		points[j].y = mivplist[chn].st_tl_attr.stTLRegion[i].stPoints[j].y;// * attr.height / viHeight;
				points[j].x = points[j].x>viWidth?viWidth:points[j].x;
				points[j].y = points[j].y>viHeight?viHeight:points[j].y;
			}
		    if(mivplist[chn].st_tl_attr.stTLRegion[i].nCnt > 2)
		    {
				if(!mivplist[chn].st_tl_attr.bHKEnable)		//海康单独设置每一个reg检测模式
		    		mivplist[chn].st_tl_attr.stTLRegion[i].nIvpCheckMode = mivplist[chn].st_tl_attr.nTLMode;
		    	switch(mivplist[chn].st_tl_attr.stTLRegion[i].nIvpCheckMode)
				{
				case 0:
					mode = e_IVP_MODE_ABANDONED_OBJ_DETECTION;
					break;
				case 1:
					mode = e_IVP_MODE_REMOVED_OBJ_DETECTION;
					break;
				default:
					mode = e_IVP_MODE_ABANDONED_OBJ_DETECTION;
					break;
				}
		    }
		    else
		    {
				printf("_________________________::::cnt:%d\n",mivplist[chn].st_tl_attr.stTLRegion[i].nCnt);
		    	return -1;
		    }
			printf("ivpAddRule	tlmode:0x%x\n",mode);
		    ivpAddRule(mivphandle, i, mode, mivplist[chn].st_tl_attr.stTLRegion[i].nCnt, points, ability.inputRes.width, ability.inputRes.height);
		    memset(points,0,MAX_POINT_NUM);
		}
		printf("================sss:%d\n",mivplist[chn].st_tl_attr.nTLAlarmDuration);
		ivpAbandonDctl(mivphandle, e_IVP_ABANDON_CTL_TYPE_ALARM_TIME, mivplist[chn].st_tl_attr.nTLAlarmDuration);
//		ivpAbandonDctl(mivphandle, e_IVP_ABANDON_CTL_TYPE_SUSPECT_TIME, mivplist[chn].st_tl_attr.nTLSuspectTime);
		ivpAbandonDctl(mivphandle, e_IVP_ABANDON_CTL_TYPE_SENSITIVITY, mivplist[chn].st_tl_attr.nTLSen/20);

	}
	else
		return -1;
#endif
	return 0;
}

int mivp_tl_stop(int chn)
{
	int ret;
#ifdef IVP_TL_SUPPORT
	if(mivphandle)
	{
		ret = ivpRelease(mivphandle);
		printf("IVP tl stop : %d\n",ret);
	}
	mivphandle = NULL;
#endif
	return 0;
}

int mivp_tl_get_param(int chn, MIVP_TL_t *attr)
{
	memcpy(attr,&mivplist[chn].st_tl_attr,sizeof(MIVP_TL_t));
	return 0;
}
int mivp_tl_set_param(int chn, MIVP_TL_t *attr)
{
	memcpy(&mivplist[chn].st_tl_attr,attr,sizeof(MIVP_TL_t));
	return 0;
}

int mivp_tl_flush(int chn)
{
#ifdef IVP_TL_SUPPORT
	if(mivphandle)
	{
		if(mivplist[chn].st_tl_attr.bTLEnable == TRUE)
		{
			int sen = mivplist[chn].st_tl_attr.nTLSen/20;
			printf("================Duration:%d		sen:%d\n",mivplist[chn].st_tl_attr.nTLAlarmDuration, sen);
			ivpAbandonDctl(mivphandle, e_IVP_ABANDON_CTL_TYPE_ALARM_TIME, mivplist[chn].st_tl_attr.nTLAlarmDuration);
//			ivpAbandonDctl(mivphandle, e_IVP_ABANDON_CTL_TYPE_SUSPECT_TIME, mivplist[chn].st_tl_attr.nTLSuspectTime);
			ivpAbandonDctl(mivphandle, e_IVP_ABANDON_CTL_TYPE_SENSITIVITY, sen);
		}
	}
#endif
	return 0;
}
int mivp_climb_bsupport(void)
{
	return hwinfo.bSupportClimbDetect;
}

/********************************************************拿取遗留报警**********************************************************/
