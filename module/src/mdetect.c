
#include "jv_common.h"
#include "jv_mdetect.h"
#include "mstream.h"
#include "msnapshot.h"
#include "malarmout.h"

#include "mdetect.h"
#include "mivp.h"
#include "utl_timer.h"
#include "mrecord.h"
#include "mlog.h"
#include "mgb28181.h"
#include "SYSFuncs.h"
#include "mtransmit.h"
#include "jv_gpio.h"
#include "mipcinfo.h"
#include "mcloud.h"
#include "utl_iconv.h"
#include "mfirmup.h"
#include "utl_jpg2bmp.h"
#include "utl_common.h"
#include "JVNSDKDef.h"
#include "JvServer.h"
#include "sctrl.h"
#include "mbizclient.h"
#include "mdevclient.h"

static MD mdlist[MAX_STREAM];

int md_timer[MAX_STREAM] = {-1, -1, -1};
static alarming_callback_t callback_ptr = NULL;

static jv_thread_group_t group;
static int chID = 0;
static BOOL s_MDStarted = FALSE;
static BOOL s_DisableMD = FALSE;		// 禁用移动侦测标志
static int	s_MDHappenTime = 0;

BOOL _mdetect_timer_callback(int tid, void *param)
{
	int i;
	int ignore_flag = 0;//用于标志是否超时。如果在较短时间内第二次触发报警，则不再响应
	int alarmOutType = (int)param;

	for (i = 0; i < HWINFO_STREAM_CNT; i++)
	{
		if (md_timer[i] == tid)
			break;
	}

	//在报警模块中获取报警延迟时间,lck20120806
	ALARMSET stAlarm;
	malarm_get_param(&stAlarm);
	mdlist[i].nDelay = stAlarm.delay;


	Printf("stop alarm...\n");
	if(mdlist[i].nStart != 0)
	{
		if(alarmOutType & ALARM_OUT_CLIENT)
		{
			if (callback_ptr != NULL)
			{
				callback_ptr(i, FALSE);
			}
		}
		if(alarmOutType & ALARM_OUT_BUZZ)
		{
			malarm_buzzing_close();
		}
		if(alarmOutType & ALARM_OUT_SOUND)
		{
			malarm_sound_stop();
		}
		if(alarmOutType & ALARM_OUT_LIGHT)
		{
			malarm_light_stop();
		}
		alarmOutType = 0;
		mdlist[i].nStart = 0;
		if (!ignore_flag)
			mlog_write("MDetect: Client Alarm Off");
	}

	return FALSE;
}

/**
 * @brief 移动侦测处理线程
 */
static void _mdetect_process(void *param)
{
	pthreadinfo_add((char *)__func__);

	int ret = FALSE;

	static time_t timeLastCloudAlarm = 0;
	static time_t timeEended = 0;
	time_t timenow;
	int ignore_flag;//用于标志是否超时。如果在较短时间内第二次触发报警，则不再响应
	int ignore_flag_cloud_alarm;
	static U32 alarmOutType;
	BOOL bAlarmReportEnable; // 安全防护开关
	BOOL bCloudRecEnable;    // 是否开通云存储功能
	BOOL bInValidTime;       // 是否在安全防护时间段内
	JV_ALARM alarm;
	struct timespec tv; 

	sleep(12); /* 第一次上电延时12秒  */

	while (group.running)
	{
		ignore_flag = 0;
		ignore_flag_cloud_alarm = 0;
		alarmOutType = 0;
		bAlarmReportEnable = FALSE;
		bCloudRecEnable = FALSE;
		bInValidTime = FALSE;

		clock_gettime(CLOCK_REALTIME, &tv);
		tv.tv_sec += 1;
		ret = sem_timedwait(&group.sem, &tv);
		if(-1 == ret)
		{
			if(errno == ETIMEDOUT) /* wait for alarm timeout */
			{
				// 静止一段时间后，降低码率
				if (hwinfo.bSupportSmtVBR)
				{
					if (utl_get_sec() - s_MDHappenTime > 5)
					{
						jv_stream_switch_lowbitrate(0, 0);
					}
				}
				continue;
			}
			else
				printf("wait for medetect error:%s\n", strerror(errno));
		}

		// 有移动时恢复码率
		if (hwinfo.bSupportSmtVBR)
		{
			jv_stream_revert_bitrate(0);
		}
		
		if (!mdlist[0].bEnable)
		{
			continue;
		}

		//在报警模块中获取报警延迟时间,lck20120806
		ALARMSET stAlarm;
		malarm_get_param(&stAlarm);
		mdlist[chID].nDelay = stAlarm.delay;

		clock_gettime(CLOCK_MONOTONIC, &tv);
		timenow = tv.tv_sec;

		bCloudRecEnable = mcloud_check_enable();
		if (bCloudRecEnable == TRUE)
		{
			if (timenow - timeLastCloudAlarm >= 30)
			{
				if (timenow - timeEended <= mdlist[chID].nDelay)
				{
					timeEended = timenow;
					ignore_flag = 1;
				}
			}
			else
			{
				ignore_flag = 1;
			}
		}
		else
		{
			if (timenow - timeEended <= mdlist[chID].nDelay)
			{
				ignore_flag = 1;
			}
			timeEended = timenow;
		}

		Printf("start alarm...%d\n",s_DisableMD);
		bInValidTime = malarm_check_validtime();
		if(!bInValidTime || s_DisableMD)
		{
			if(mdlist[chID].nStart != 0)
			{
				utl_timer_reset(md_timer[chID], 0, _mdetect_timer_callback, (void *)1);
			}
			continue;
		}

		if (!ignore_flag)
		{
		    if (bCloudRecEnable == TRUE)
			{
				timeEended = timenow + 30;
			    timeLastCloudAlarm = timenow;
			}
			mlog_write("MDetect: Detected Warning");
		}

		mdlist[chID].nStart = 1;
		bAlarmReportEnable = malarm_check_enable();
		bAlarmReportEnable = (mfirmup_b_updating()) ? FALSE : bAlarmReportEnable;
		
		malarm_build_info(&alarm, ALARM_MOTIONDETECT);
		if(mdlist[chID].bEnableRecord)
		{
			if(bAlarmReportEnable && bCloudRecEnable)
			{
				if (!ignore_flag)
				{
					alarm.cmd[1] = ALARM_VIDEO;
					mrecord_alarming(0, ALARM_TYPE_MOTION, &alarm);
				}
			}
			else
			{
				mrecord_alarming(0, ALARM_TYPE_MOTION, NULL);
			}
		}

		if (!ignore_flag)
		{
			if(bAlarmReportEnable)
			{
				mrecord_alarm_get_attachfile(REC_MOTION, &alarm);
				if (alarm.cloudPicName[0])
				{
					if (msnapshot_get_file(0, alarm.cloudPicName) != 0)
					{
						alarm.cloudPicName[0] = '\0';
						alarm.PicName[0] = '\0';
					}
				}
				else if (alarm.PicName[0])
				{
					if (msnapshot_get_file(0,  alarm.PicName) != 0)
					{
						alarm.PicName[0] = '\0';
					}
				}
				else
					printf("cloudPicName and picName all none!\n");
		
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
					ret = mbizclient_PushAlarm("MD",alarm.ystNo,alarm.nChannel,ALARM_TEXT,alarm.uid,alarm.alarmType,alarm.time,ppic,pvedio);
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
		
		if(mdlist[chID].bOutClient)
		{
			//if (mdlist[i].nStart == 0)
			{
				//这里每次都给分控发报警信号，便于刚连接的分控能收到
				if (callback_ptr != NULL)
				{
					callback_ptr(chID,TRUE);
				}
				if (!ignore_flag)
					mlog_write("MDetect: Client Alarm On");
				alarmOutType |= ALARM_OUT_CLIENT;
			}
		}

		if(!ignore_flag && mdlist[chID].bOutVMS)
		{
			ipcinfo_t info;
			ipcinfo_get_param(&info);
		}
		if(!ignore_flag)
		{
			if(mdlist[chID].bOutEMail)
			{
				Printf("Now malarm_sendmail\n");
				malarm_sendmail(0, mlog_translate("MDetect: Detected Warning"));//借用mlog的多语言管理
				mlog_write("MDetect: Mail Sended");
			}
		}
		if(mdlist[chID].bBuzzing)
		{
			malarm_buzzing_open();
			alarmOutType |= ALARM_OUT_BUZZ;
		}
#ifdef GB28181_SUPPORT
		if (!ignore_flag)
		{
			mgb_send_alarm(i, 2, 5);
		}
#endif

		if(hwinfo.bHomeIPC && stAlarm.bAlarmSoundEnable)
		{
			malarm_sound_start();
			alarmOutType |= ALARM_OUT_SOUND;
		}
		if(!ignore_flag && hwinfo.bHomeIPC && stAlarm.bAlarmLightEnable)
		{
			malarm_light_start();
			alarmOutType |= ALARM_OUT_LIGHT;
		}

		utl_timer_reset(md_timer[chID], mdlist[chID].nDelay*1000, _mdetect_timer_callback, (void *)alarmOutType);
	}/* while */
}/* _mdetect_process */


static void _mdetect_callback(int channelid, void *param)
{
	s_MDHappenTime = utl_get_sec();
	sem_post(&group.sem);
	chID = channelid;
	if (md_timer[channelid] == -1)
		md_timer[channelid] = utl_timer_create("mdetect", 0, _mdetect_timer_callback, NULL);
}

int mdetect_init(void)
{
	jv_mdetect_init();
	sem_init(&group.sem, 0, 0);
	group.running = TRUE;
	pthread_mutex_init(&group.mutex, NULL);
	pthread_create(&group.thread, NULL, (void *) _mdetect_process, NULL);

	return 0;
}

int mdetect_deinit(void)
{
	mdetect_stop(0);
	Printf("xian mdetect_stop over\n");
	if (md_timer[0] != -1)
		utl_timer_destroy(md_timer[0]);
	Printf("xian utl_timer_destroy over\n");
	jv_mdetect_deinit();

	sem_destroy(&group.sem);
	group.running = FALSE;
	pthread_join(group.thread, NULL);
	pthread_mutex_destroy(&group.mutex);

	Printf("mdetect_deinit over\n");
	return 0;
}

void mdetect_enable()
{
	s_DisableMD = FALSE;
}

void mdetect_disable()
{
	s_DisableMD = TRUE;
}

/**
 *@brief 设置报警回调函数
 *
 *
 */
int mdetect_set_callback(alarming_callback_t callback)
{
	callback_ptr = callback;
	return 0;
}


/**
 *@brief 开始移动检测
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mdetect_start(int channelid)
{
	jv_mdetect_attr_t attr;
//	RECT rect;
	int i;
	int ret;
	unsigned int w,h;

	mdlist[channelid].nRectNum = 0;
	for (i=0;i<MAX_MDRGN_NUM;i++)
	{
		if (mdlist[channelid].stRect[i].w != 0
			&& mdlist[channelid].stRect[i].h != 0)
		{
			mdlist[channelid].nRectNum++;
		}
	}

	attr.nSensitivity = mdlist[channelid].nSensitivity;
	attr.nThreshold = mdlist[channelid].nThreshold;
	attr.cnt = 0;
	//全屏捕捉
	if (mdlist[channelid].nRectNum == 0)
	{
		jv_stream_get_vi_resolution(0, &w, &h);
		attr.rect[attr.cnt].x = 0;
		attr.rect[attr.cnt].y= 0;
		attr.rect[attr.cnt].w = w;
		attr.rect[attr.cnt].h = h;
		attr.cnt = 1;
	}
	else
	{
		for (i=0;i<mdlist[channelid].nRectNum;i++)
		{
			attr.rect[attr.cnt++] = mdlist[channelid].stRect[i];
		}
	}

#ifdef MD_GRID_SET
	memcpy(attr.nRegion,mdlist[channelid].nRegion,4*MAX_REGION_ROW);
	attr.nRow = mdlist[channelid].nRow;
	attr.nColumn = mdlist[channelid].nColumn;
#endif

	ret = jv_mdetect_set_attr(channelid, &attr);
	if (ret != 0)
	{
		mlog_write("Mdetect Start: Failed");
		return -1;
	}
	jv_mdetect_stop(channelid);

	if(mdlist[channelid].bEnableRecord)
		mrecord_set(0, detecting, TRUE);//SD卡录像开启
	else
		mrecord_set(0, detecting, FALSE);
	WriteConfigInfo();

	ret = jv_mdetect_start(channelid, _mdetect_callback, NULL);
	if (0 == ret)
	{
		s_MDStarted = TRUE;
	}

	return ret;
}

/**
 *@brief 停止移动检测
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mdetect_stop(int channelid)
{
	int ret;

	ret = jv_mdetect_stop(channelid);
	if (ret != 0)
	{
		mlog_write("Mdetect Stop: Failed");
		return -1;
	}
	Printf("mdetect_stop over %d\n", channelid);
	if (0 == ret)
	{
		s_MDStarted = FALSE;
	}

	return 0;
}

int mdetect_set_param(int channelid, MD *md)
{
	int i;
	jvstream_ability_t ability;
	md->nStart = mdlist[channelid].nStart;
	md->nSensitivity  = VALIDVALUE (md->nSensitivity , 0, 100);
	md->nThreshold = VALIDVALUE (md->nThreshold , 0, 100);

	jv_stream_get_ability(channelid, &ability);

	for (i=0;i<MAX_MDRGN_NUM;i++)
	{
		md->stRect[i].x = VALIDVALUE (md->stRect[i].x, 0, ability.inputRes.width);
		md->stRect[i].y = VALIDVALUE (md->stRect[i].y, 0, ability.inputRes.height);
		md->stRect[i].w = VALIDVALUE (md->stRect[i].w, 0, ability.inputRes.width - md->stRect[i].x);
		md->stRect[i].h = VALIDVALUE (md->stRect[i].h, 0, ability.inputRes.height - md->stRect[i].y);
	}

	if (md->bEnable == TRUE )
	{
		if( mdlist[channelid].bEnable == FALSE)
		{
			mlog_write("Mdetect Start");
			// 鹏博士产品定制需求，移动侦测开启时，自动切换至报警录像模式
			if (PRODUCT_MATCH(PRODUCT_PBS))
			{
				mrecord_set_recmode(RECORD_MODE_ALARM, 0);
			}
		}
		if (mdlist[channelid].bOutEMail == 0  && md->bOutEMail == 1)
		{
			mlog_write("MD sendmail Enabled");
		} 
		else if(mdlist[channelid].bOutEMail == 1  && md->bOutEMail == 0)
		{
			mlog_write("MD sendmail Disabled");
		}
		//begin added by zhouwq 20150706 {
		//是否开启报警录像
		if(0 == mdlist[channelid].bEnableRecord && 1 == md->bEnableRecord)
		{
			mlog_write("MD alarmRecord Enabled");
		}
		else if(1 == mdlist[channelid].bEnableRecord && 0 == md->bEnableRecord)
		{
			mlog_write("MD alarmRecord Disabled");
		}
		
		//是否发送至VMS服务器
		if(0 == mdlist[channelid].bOutVMS && 1 == md->bOutVMS)
		{
			mlog_write("MD sendVMS Enabled");
		}
		else if(1 == mdlist[channelid].bOutVMS && 0 == md->bOutVMS)
		{
			mlog_write("MD sendVMS Disabled");
		}
		
		//是否蜂鸣器报警
		if(0 == mdlist[channelid].bBuzzing && 1 == md->bBuzzing)
		{
			mlog_write("MD alarmBuzzing Enabled");
		}
		else if(1 == mdlist[channelid].bBuzzing && 0 == md->bBuzzing)
		{
			mlog_write("MD alarmBuzzing Disabled");
		}
		//}end added
	}
	else 
	{
		if ( mdlist[channelid].bEnable == TRUE)
		{
			mlog_write("Mdetect Stop");
			// 鹏博士产品定制需求，移动侦测关闭后，自动切换至全天录像模式
			if (PRODUCT_MATCH(PRODUCT_PBS))
			{
				mrecord_set_recmode(RECORD_MODE_NORMAL, 0);
			}
		}
	} 

	memcpy(&mdlist[channelid],md,sizeof(MD));

	return 0;
}

int mdetect_get_param(int channelid, MD *md)
{
	memcpy(md, &(mdlist[channelid]), sizeof(MD));
	return 0;
}

int mdetect_flush(int channelid)
{
	if (!s_MDStarted)
	{
		mdetect_start(0);
	}

	if (mdlist[channelid].bEnable)
	{
		Printf("mdetect_start.\n");
#if  (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100)			//16CV200智能分析与移动侦测不能同时开
		mivp_stop(0);
#endif
		jv_mdetect_set_sensitivity(channelid, mdlist[channelid].nSensitivity);
	}
	else
	{
#if  (defined PLATFORM_hi3518EV200) || (defined PLATFORM_hi3516EV100)		//16CV200移动侦测关闭后需重启智能分析
		mivp_start(0);
#endif
	}
	if (!mdlist[channelid].bOutClient || !mdlist[channelid].bEnable)
	{
		if (callback_ptr)
		{
			callback_ptr(channelid, FALSE);
		}
	}
	return 0;
}

/**
 *@brief 是否发生移动检测报警
 *@param channelid 频道号
 *@return 为真，则有报警发生。否则没有。
 *
 *@note #MD::nDelay 的值，决定了报警后多长时间内，是处于报警中
 *
 */
BOOL mdetect_b_alarming(int channelid)
{
	if (mdlist[channelid].bEnable)
	{
		return mdlist[channelid].nStart;
	}
	return FALSE;
}
