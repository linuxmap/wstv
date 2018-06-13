#include "mioctrl.h"
#include "jv_common.h"
#include "jv_io.h"
#include "SYSFuncs.h"
#include "utl_timer.h"
#include "utl_ifconfig.h"
#include "maccount.h"
#include "mvoicedec.h"
#include "maudio.h"
#include "utl_ifconfig.h"
#include "mipcinfo.h"
#include "jv_gpio.h"
#include "malarmout.h"
#include "msensor.h"
#include "jv_sensor.h"
#include "mrecord.h"
#include "mbizclient.h"
#include <msnapshot.h>
#include "mvoicedec.h"
#include "mstorage.h"
#include "mdetect.h"
#include "utl_common.h"
#include "jv_ai.h"
#include "jv_ao.h"

static DevStatus_e devStatus;
static int timer_io_ctrl = -1;
static int g_resetFlag = 0;
static BOOL s_bResetting = FALSE;
static BOOL s_bOneMinRecState = FALSE;
static int g_ringTimer = -1;
static int g_ringTimes = 0;
static volatile DEV_ST_CHAT g_chatStatus = DEV_ST_CHAT_FREE;
#ifdef BIZ_CLIENT_SUPPORT
static JV_ALARM s_OneMinAlarmInfo;
#endif

static void _mio_led_ctrl(int type, int state)
{
	static int last_led_state[IO_LED_BUTT] = {0};

	switch (state)
	{
	case IO_ON:
	case IO_OFF:
		last_led_state[type] = state;
		break;
	case IO_BLINK:
		last_led_state[type] = (last_led_state[type] == IO_ON) ? IO_OFF : IO_ON;
		break;
	default:
		break;
	}

	jv_io_led_set(type, last_led_state[type]);
}

static void _mio_onemin_rec_finished()
{
	malarm_sound_stop();//alarm sound off
	while(!malarm_get_speakerFlag())
	{
		usleep(50*1000);
	}
	maudio_resetAIAO_mode(2);
	maudio_speaker(VOICE_STOP_RECORD, TRUE, TRUE, TRUE);
	if(speakerowerStatus == JV_SPEAKER_OWER_VOICE)
	{
		maudio_resetAIAO_mode(1);
	}
	mdetect_enable();
	_mio_led_ctrl(IO_LED_GREEN, IO_OFF);

	s_bOneMinRecState = FALSE;

#ifdef BIZ_CLIENT_SUPPORT
	if (hwinfo.bXWNewServer)
	{
		int ret = mbizclient_PushAlarm(s_OneMinAlarmInfo.alarmDevName, s_OneMinAlarmInfo.ystNo, s_OneMinAlarmInfo.nChannel, ALARM_TEXT,
								s_OneMinAlarmInfo.uid, s_OneMinAlarmInfo.alarmType, s_OneMinAlarmInfo.time, 
								s_OneMinAlarmInfo.PicName, s_OneMinAlarmInfo.VideoName);
		printf("One min rec mbizclient_PushAlarm one min %s!!!!\n\n", (ret == 0) ? "successfully" : "failed");
	}
#endif
}

static void __notify_user(int hz, int sec)
{
	int interval = 500 / hz;

	// C8/C8A/C8E设备无喇叭，使用红外灯提示用户设备重置
	if (HWTYPE_MATCH(HW_TYPE_C8)
		|| HWTYPE_MATCH(HW_TYPE_C8A)
		|| HWTYPE_MATCH(HW_TYPE_C8S)
		|| HWTYPE_MATCH(HW_TYPE_C8H))
	{
		while (sec--)
		{
			_mio_led_ctrl(IO_LED_INFRARED, IO_ON);
			usleep(interval * 1000);
			_mio_led_ctrl(IO_LED_INFRARED, IO_OFF);
			usleep(interval * 1000);
		}

		// 恢复红外灯状态
		if (msensor_mode_get())
		{
			_mio_led_ctrl(IO_LED_INFRARED, IO_ON);
		}
	}
}

static void* _notify_user(void* param)
{
	pthreadinfo_add((char *)__func__);

	// 最少闪烁8秒，确保用户可以看到
	__notify_user(1, 8);
	while (s_bResetting)
	{
		__notify_user(1, 2);
	}

	return NULL;
}

static void* reset_proc(void *param)
{
	pthreadinfo_add((char *)__func__);

	if (s_bOneMinRecState)
	{
		printf("========%s, stop one min rec\n", __func__);
		mrecord_request_stop_record(0, RECORD_REQ_ONEMIN);
		utl_WaitTimeout(!s_bOneMinRecState, 5000);
	}

	// C8/C8A/C8E设备无喇叭，使用红外灯提示用户设备重置
	pthread_create_detached(NULL, NULL, _notify_user, NULL);

	SYSFuncs_factory_default_without_reboot();
	printf("========%s, recovery finished at %d, during: %d!!!\n", 
				__func__, (int)time(NULL), (int)time(NULL) - (int)param);

	s_bResetting = FALSE;
	return NULL;
}

static BOOL __ring_timer(int tid, void* param)
{
	printf("__ring_timer\n");
#ifdef BIZ_CLIENT_SUPPORT
	if (hwinfo.bXWNewServer)
	{
		g_ringTimes++;
		if(g_ringTimes > 10)
		{
			g_ringTimes = 0;
			if(g_ringTimer >= 0)
				utl_timer_destroy(g_ringTimer);
			maudio_speaker(VOICE_CHAT_NO_ANSWER, TRUE, TRUE, TRUE);
			g_chatStatus = DEV_ST_CHAT_STOP;
			printf("g_chatStatus=%d\n", g_chatStatus);
		}
		else
		{
			maudio_speaker(VOICE_CHAT_RING, FALSE, TRUE, FALSE);
			utl_timer_reset(g_ringTimer, 4000, (utl_timer_callback_t)__ring_timer, NULL);	
		}
	}
#endif
	return TRUE;
}

static void *_io_ctrl_thread(void *param)
{
	IOKey_e key;
	int reset_detect_times = 0;
	int wifi_set_detect_times = 0;
	int wps_detect_times = 0;
	int one_min_rec_times = 0;
	int light_alarm_rec_times = 0;
	int free_times = 0;
	int factory_led_state = 0;
	unsigned int TickCount = 0;
	const ipcinfo_t* pIpcInfo = ipcinfo_get_param(NULL);

	while(1)
	{
		int bReset = FALSE;
		int bWifiSet = 0;
		int bWps = FALSE;
		int night = 0;
		char b_one_min_rec = FALSE;

		//struct timeval tv;
		//gettimeofday(&tv, NULL);
		//printf("time:%ld.%ld\n", tv.tv_sec, tv.tv_usec/1000);
		
		usleep(100 * 1000);
		++TickCount;

		// 500ms处理一次
		if (TickCount % 5 == 0)
		{
			if (gp.bFactoryFlag)		//检测到工厂测试标识文件，指示灯循环点亮
			{
				_mio_led_ctrl(IO_LED_RED, IO_OFF);
				_mio_led_ctrl(IO_LED_GREEN, IO_OFF);
				_mio_led_ctrl(IO_LED_BLUE, IO_OFF); 

				if(factory_led_state % 3 == 0)
				{
					_mio_led_ctrl(IO_LED_RED, IO_ON);
				}
				else if(factory_led_state % 3 == 1)
				{
					_mio_led_ctrl(IO_LED_GREEN, IO_ON);
				}
				else
				{
					_mio_led_ctrl(IO_LED_BLUE, IO_ON); 
				}

				factory_led_state += 1;
			}
			else
			{
				night = jv_sensor_b_night();
				if (((LED_CONTROL_AUTO == pIpcInfo->LedControl) && (night == 1))	// 自动且夜视
					|| (LED_CONTROL_OFF == pIpcInfo->LedControl))					// 常灭
				{
					_mio_led_ctrl(IO_LED_RED, IO_OFF);
					_mio_led_ctrl(IO_LED_GREEN, IO_OFF);
					_mio_led_ctrl(IO_LED_BLUE, IO_OFF);
				}
				else
				{
					if (!s_bOneMinRecState)
					{
						switch(devStatus.stNet)
						{
						case DEV_ST_ETH_OK:
						case DEV_ST_WIFI_OK:
							if (
								HWTYPE_MATCH(HW_TYPE_A4) ||
								HWTYPE_MATCH(HW_TYPE_C3) ||
								HWTYPE_MATCH(HW_TYPE_C3W) ||
								HWTYPE_MATCH(HW_TYPE_V3) ||
								HWTYPE_MATCH(HW_TYPE_V6) ||
								PRODUCT_MATCH(PRODUCT_C3A))
							{
								_mio_led_ctrl(IO_LED_RED, IO_OFF);
								_mio_led_ctrl(IO_LED_BLUE, IO_ON);
							}
							else
							{
								_mio_led_ctrl(IO_LED_GREEN, IO_ON);
							}
							break;
						case DEV_ST_WIFI_SETTING:
							if(
								HWTYPE_MATCH(HW_TYPE_A4) ||
								HWTYPE_MATCH(HW_TYPE_C3) ||
								HWTYPE_MATCH(HW_TYPE_C3W) ||
								HWTYPE_MATCH(HW_TYPE_V3) ||
								HWTYPE_MATCH(HW_TYPE_V6) ||
								PRODUCT_MATCH(PRODUCT_C3A))
							{
								_mio_led_ctrl(IO_LED_BLUE, IO_OFF);
								_mio_led_ctrl(IO_LED_RED, IO_BLINK);
							}
							else
							{
								_mio_led_ctrl(IO_LED_GREEN, IO_BLINK);
							}
							break;
						case DEV_ST_WIFI_CONNECTED:
							if(
								HWTYPE_MATCH(HW_TYPE_A4) ||
								HWTYPE_MATCH(HW_TYPE_C3) ||
								HWTYPE_MATCH(HW_TYPE_C3W) ||
								HWTYPE_MATCH(HW_TYPE_V3) ||
								HWTYPE_MATCH(HW_TYPE_V6) ||
								PRODUCT_MATCH(PRODUCT_C3A))
							{
								_mio_led_ctrl(IO_LED_RED, IO_OFF);
								_mio_led_ctrl(IO_LED_BLUE, IO_ON);
							}
							else
							{
								_mio_led_ctrl(IO_LED_GREEN, IO_BLINK);
							}
							break;
						case DEV_ST_WIFI_DISCONNECT:
							break;
						case DEV_ST_WIFI_CONNECTING:
							if(
								HWTYPE_MATCH(HW_TYPE_A4) ||
								HWTYPE_MATCH(HW_TYPE_C3) ||
								HWTYPE_MATCH(HW_TYPE_C3W) ||
								HWTYPE_MATCH(HW_TYPE_V3) ||
								HWTYPE_MATCH(HW_TYPE_V6) ||
								PRODUCT_MATCH(PRODUCT_C3A))
							{
								_mio_led_ctrl(IO_LED_RED, IO_OFF);
								_mio_led_ctrl(IO_LED_BLUE, IO_BLINK);
							}
							else if(
								HWTYPE_MATCH(HW_TYPE_HA210) ||
								HWTYPE_MATCH(HW_TYPE_HA230) ||
								HWTYPE_MATCH(HW_TYPE_C5)
								)
							{
								_mio_led_ctrl(IO_LED_GREEN, IO_BLINK);
							}
							else
							{
								_mio_led_ctrl(IO_LED_RED, IO_BLINK);
							}
							break;
						case DEV_ST_NET_NONE:
							if (
								HWTYPE_MATCH(HW_TYPE_A4) ||
								HWTYPE_MATCH(HW_TYPE_C3) ||
								HWTYPE_MATCH(HW_TYPE_C3W) ||
								HWTYPE_MATCH(HW_TYPE_V3) ||
								HWTYPE_MATCH(HW_TYPE_V6) ||
								PRODUCT_MATCH(PRODUCT_C3A))
							{
								_mio_led_ctrl(IO_LED_RED, IO_ON);
								_mio_led_ctrl(IO_LED_BLUE, IO_OFF);
							}
							break;
						default:
							break;
						}
					}
					else
					{
					    _mio_led_ctrl(IO_LED_RED, IO_OFF);
						_mio_led_ctrl(IO_LED_BLUE, IO_OFF);
						_mio_led_ctrl(IO_LED_GREEN, IO_BLINK);
					}
				}	

				switch(devStatus.stLightAlarm)
				{
					case DEV_ST_LIGHT_ALARM_ON:
						light_alarm_rec_times++;
						if(light_alarm_rec_times >= 60) //30s
							devStatus.stLightAlarm = DEV_ST_LIGHT_ALARM_OFF;
						else
							_mio_led_ctrl(IO_LED_WHITE, IO_BLINK);
						break;
					case DEV_ST_LIGHT_ALARM_OFF:
						/* 手动关闭或者超时关闭 */
						if(light_alarm_rec_times > 0)
						{
							light_alarm_rec_times = 0;
							msensor_set_alarm_light(FALSE);
							_mio_led_ctrl(IO_LED_WHITE, msensor_get_whitelight_status() == TRUE ? IO_ON : IO_OFF);
						}
						break;
					default:
						break;
				}
			}
		}

		//系统还没有稳定时，不进行复位检测
		if (utl_get_sec() <= 25)
		{
			continue;
		}
		key = jv_io_key_get();
		switch(key)
		{
			case IO_KEY_RESET:
				reset_detect_times++;
				Printf("reset_detect_times=%d\n",reset_detect_times);
				if(strcmp(hwinfo.devName,"H211") == 0)
				{
					if(reset_detect_times == 50)
					{
						printf("reset for 5 seconds....\n");
						bReset = TRUE;
					}
					break;
				}
				else
				{
					// 工厂模式下立即响应
					if (gp.bFactoryFlag)
					{
						Printf("reset for factory test\n");
						bReset = TRUE;
					}
					else if(reset_detect_times == 15)
					{
						Printf("reset for 1.5 seconds....\n");
						bReset = TRUE;
					}
				}
				break;
			case IO_KEY_WPS:
				Printf("WPS....\n");
				if(wps_detect_times++ > 15)
					bWps = TRUE;
				break;
			case IO_KEY_WIFI_SET:
				Printf("IO_KEY_WIFI_SET....\n");
				wifi_set_detect_times++;
				if(wifi_set_detect_times == 15)
				{
					printf("wifi set for 1.5 seconds....\n");
					bWifiSet = TRUE;
				}
				break;
			case IO_KEY_ONE_MIN_REC:
				one_min_rec_times++;
				if(g_chatStatus == DEV_ST_CHAT_ONGOING
					|| g_chatStatus == DEV_ST_CHAT_CHATING
					|| (one_min_rec_times > 5 && !s_bOneMinRecState))
				{
					if(g_chatStatus == DEV_ST_CHAT_ONGOING)
					{
						//开启对讲后，按键空闲时间大于1s，才允许关闭对讲
						if(free_times > 10)
						{
							g_chatStatus = DEV_ST_CHAT_STOP;
							printf("g_chatStatus=%d\n", g_chatStatus);
						}
					}
					else if(g_chatStatus == DEV_ST_CHAT_CHATING)
					{
						//开启对讲后，按键空闲时间大于1s，才允许关闭对讲
						if(free_times > 10)
						{
							g_chatStatus = DEV_ST_CHAT_STOP;
							printf("g_chatStatus=%d\n", g_chatStatus);
						}
					}
					else
					{
						g_chatStatus = DEV_ST_CHAT_START;
						printf("g_chatStatus=%d\n", g_chatStatus);
					}
				}
				else
				{
					if(g_chatStatus == DEV_ST_CHAT_FREE)
					{
						if(s_bOneMinRecState)
						{
							b_one_min_rec = TRUE;
						}
						else
						{
							g_chatStatus = DEV_ST_CHAT_READY_1;
							printf("g_chatStatus=%d\n", g_chatStatus);
						}
					}
					else if((g_chatStatus == DEV_ST_CHAT_READY_2 && one_min_rec_times == 1)
							|| (g_chatStatus == DEV_ST_CHAT_FREE && s_bOneMinRecState))
					{
						b_one_min_rec = TRUE;
					}
				}


#if 0
				if (s_bOneMinRecState || (one_min_rec_times > 5 && g_chatStatus == DEV_ST_CHAT_READY_1))
				{
					b_one_min_rec = TRUE;
				}
				else if(one_min_rec_times >= 1 && !s_bOneMinRecState)
				{
					if(g_chatStatus == DEV_ST_CHAT_FREE)
					{
						g_chatStatus = DEV_ST_CHAT_READY_1;
						printf("g_chatStatus=%d\n", g_chatStatus);
					}
					else if(g_chatStatus == DEV_ST_CHAT_READY_2)
					{
						g_chatStatus = DEV_ST_CHAT_START;
						printf("g_chatStatus=%d\n", g_chatStatus);
					}
					else if(g_chatStatus == DEV_ST_CHAT_ONGOING)
					{
						//开启对讲后，按键空闲时间大于1s，才允许关闭对讲
						if(free_times > 10)
						{
							g_chatStatus = DEV_ST_CHAT_STOP;
							printf("g_chatStatus=%d\n", g_chatStatus);
						}
					}
					else if(g_chatStatus == DEV_ST_CHAT_CHATING)
					{
						//开启对讲后，按键空闲时间大于1s，才允许关闭对讲
						if(free_times > 10)
						{
							g_chatStatus = DEV_ST_CHAT_STOP;
							printf("g_chatStatus=%d\n", g_chatStatus);
						}
					}
				}
#endif
				break;
			
			default:
				break;
		}
		
		if(key >= IO_KEY_BUTT)
			key = IO_KEY_BUTT;
		
		if(bReset && (!s_bResetting))
		{
			s_bResetting = TRUE;
			reset_detect_times = 0;				//防止两次reset
			printf("recovery, time=%ld\n", time(NULL));
			pthread_create_detached(NULL, NULL, reset_proc, (void*)time(NULL));

			while(jv_io_key_get() == IO_KEY_RESET)
			{
				usleep(500*1000);	//消键检测，防止一直按reset键，重复reset
			}
		}
		
		if (b_one_min_rec)
		{
	    	//printf("IO_KEY_ONE_MIN_REC clear\n");
			REC_REQ_PARAM req = {.ReqType = RECORD_REQ_ONEMIN, .nDuration = 60, .pCallback = _mio_onemin_rec_finished};

			one_min_rec_times = 0;

			if (!s_bOneMinRecState)
			{
				mdetect_disable();
				malarm_sound_stop();//alarm sound off
				while (!malarm_get_speakerFlag())
				{
					usleep(50*1000);
				}
				maudio_resetAIAO_mode(2);
				maudio_speaker(VOICE_START_RECORD, TRUE, TRUE, TRUE);

				if (mrecord_request_start_record(0, &req) != 0)
				{
					maudio_speaker(VOICE_STOP_RECORD, TRUE, TRUE, TRUE);
					mdetect_enable();
				}
				else
				{
#ifdef BIZ_CLIENT_SUPPORT
					if (hwinfo.bXWNewServer)
					{
						memset(&s_OneMinAlarmInfo, 0, sizeof(s_OneMinAlarmInfo));
						malarm_build_info(&s_OneMinAlarmInfo, ALARM_ONE_MIN);
						mrecord_alarm_get_attachfile(REC_ONE_MIN, &s_OneMinAlarmInfo);
						if (s_OneMinAlarmInfo.PicName[0])
						{
							printf("alarmJpgFile: %s\n", s_OneMinAlarmInfo.PicName);
							if (msnapshot_get_file(0,  s_OneMinAlarmInfo.PicName) != 0)
							{
								s_OneMinAlarmInfo.PicName[0] = '\0';
							}
						}
						else
						{
							printf("One min rec cloudPicName and PicName are none\n");
						}
					}
#endif
					s_bOneMinRecState = TRUE;
				}

				if (speakerowerStatus == JV_SPEAKER_OWER_VOICE)
				{
				 	maudio_resetAIAO_mode(1);
				}
			}
			else
			{
				mrecord_request_stop_record(0, RECORD_REQ_ONEMIN);
			}

	   		while(jv_io_key_get() == IO_KEY_ONE_MIN_REC)
			{
				usleep(100*1000);	//消键检测，防止一直按键，重复响应
			}
			printf("*** b_one_min_rec exit \n");
		}
		
#if 0
		if(bWps)
		{
			utl_ifconfig_wifi_WPS_start();
			wps_detect_times = 0;
		}
		
		if (bWifiSet)
		{
			malarm_sound_stop();
			maudio_speaker(VOICE_ENABLE, TRUE,TRUE, TRUE);
			mvoicedec_enable();
			bWifiSet = FALSE;
			wifi_set_detect_times = 0;
		}
#endif
		if(g_chatStatus == DEV_ST_CHAT_START && !bReset)
		{
			printf("start video chat\n");
			g_chatStatus = DEV_ST_CHAT_ONGOING;
			printf("g_chatStatus=%d\n", g_chatStatus);
#ifdef BIZ_CLIENT_SUPPORT
			JV_ALARM alarm;
			malarm_build_info(&alarm, ALARM_CHAT_START);
			if (hwinfo.bXWNewServer)
			{
				mbizclient_PushAlarm(alarm.alarmDevName,alarm.ystNo,alarm.nChannel,ALARM_TEXT,alarm.uid,alarm.alarmType,alarm.time, "", "");
				printf("start video chat mbizclient_PushAlarm ok\n");
				g_ringTimes = 0;
				g_ringTimer = utl_timer_create("ring timer", 0, (utl_timer_callback_t)__ring_timer, NULL);
				if(g_ringTimer < 0)
				{
					printf("start ring timer failed\n");
				}
			}
#endif
		}
		else if(g_chatStatus == DEV_ST_CHAT_STOP 
			|| (bReset && (g_chatStatus == DEV_ST_CHAT_ONGOING || g_chatStatus == DEV_ST_CHAT_CHATING)))
		{
			printf("stop video chat\n");
			g_chatStatus = DEV_ST_CHAT_STOPPING;
			printf("g_chatStatus=%d\n", g_chatStatus);
			
#ifdef BIZ_CLIENT_SUPPORT

			//如果正在呼叫中设备主动挂断，停止呼叫
			g_ringTimes = 0;
			if(g_ringTimer >= 0)
				utl_timer_destroy(g_ringTimer);
			
			//播放挂断提示音，提示用户正在挂断
			maudio_speaker(VOICE_CHAT_HANGUP, FALSE, TRUE, TRUE);

			//发送设备主动挂断提示到手机端
			JV_ALARM alarm;
			malarm_build_info(&alarm, ALARM_CHAT_STOP);
			if (hwinfo.bXWNewServer)
			{
				mbizclient_PushAlarm(alarm.alarmDevName,alarm.ystNo,alarm.nChannel,ALARM_TEXT,alarm.uid,alarm.alarmType,alarm.time, "", "");
				printf("stop video chat mbizclient_PushAlarm ok\n");
			}
			
			//不管手机端是否收到挂断提示，设备停止对讲
			jv_ai_setChatStatus(FALSE); 
			if(speakerowerStatus == JV_SPEAKER_OWER_CHAT)
				speakerowerStatus = JV_SPEAKER_OWER_NONE;
			usleep(1000*1000);
			jv_ao_mute(1);
#endif
		}
		
		if (key == IO_KEY_BUTT)
		{
			if(free_times < 1000)
				free_times++;
			reset_detect_times = 0;
			one_min_rec_times = 0;
			wifi_set_detect_times = 0;
			wps_detect_times = 0;
			if(g_chatStatus == DEV_ST_CHAT_READY_1 || g_chatStatus == DEV_ST_CHAT_READY_2)
			{
				if(free_times > 10)
				{
					g_chatStatus = DEV_ST_CHAT_FREE;
					printf("g_chatStatus=%d\n", g_chatStatus);
				}
				else if(free_times >= 1)
				{
					g_chatStatus = DEV_ST_CHAT_READY_2;
					printf("g_chatStatus=%d\n", g_chatStatus);
				}
				
			}
			else if(g_chatStatus == DEV_ST_CHAT_STOPPING)
			{
				//关闭对讲后，按键空闲时间大于1s，才允许再次开启对讲
				if(free_times > 10)
				{
					g_chatStatus = DEV_ST_CHAT_FREE;
					printf("g_chatStatus=%d\n", g_chatStatus);
				}			
			}
		}
		else
		{
			free_times = 0;
		}
	}

	return NULL;
}

int mio_getboardext_status(int ext_type)
{
	int ret = 0;
	switch(ext_type)
	{
	case 0:
		ret = jv_io_dev_status(DEV_REDBOARD);
		break;

	case 1:
		ret = jv_io_dev_status(DEV_LIGHTBOARD);
		break;
		
	default:
		break;
	}
	return ret;
}

int mio_init(void)
{
	jv_io_init();
	memset(&devStatus, 0, sizeof(DevStatus_e));
	if (hwinfo.bHomeIPC)
	{
		pthread_create_detached(NULL, NULL, (void*)_io_ctrl_thread, NULL);
	}

	return 0;
} 

int mio_deinit(void)
{
	if(timer_io_ctrl != -1)
	{
		utl_timer_destroy(timer_io_ctrl);
	}
	return 0;
}

int mio_set_net_st(DEV_ST_NET stNet)
{
	devStatus.stNet = stNet;
	return 0;
}

DEV_ST_NET mio_get_get_st()
{
	return devStatus.stNet;
}

int mio_chat_start()
{
#ifdef BIZ_CLIENT_SUPPORT
	if (hwinfo.bXWNewServer)
	{
		if(g_ringTimer >= 0)
			utl_timer_destroy(g_ringTimer);
		if(g_chatStatus == DEV_ST_CHAT_ONGOING)
			g_chatStatus = DEV_ST_CHAT_CHATING;
	}
#endif
	return 0;
}

int mio_chat_refuse()
{
#ifdef BIZ_CLIENT_SUPPORT
	if (hwinfo.bXWNewServer)
	{
		if(g_chatStatus == DEV_ST_CHAT_ONGOING)
		{
			if(g_ringTimer >= 0)
				utl_timer_destroy(g_ringTimer);
			g_chatStatus = DEV_ST_CHAT_STOP;
			maudio_speaker(VOICE_CHAT_REFUSE, TRUE, TRUE, TRUE);
		}
	}
#endif
	return 0;
}

int mio_chat_interrupt()
{
#ifdef BIZ_CLIENT_SUPPORT
	if (hwinfo.bXWNewServer)
	{
		if(g_chatStatus == DEV_ST_CHAT_ONGOING || g_chatStatus == DEV_ST_CHAT_CHATING)
		{
			if(g_ringTimer >= 0)
				utl_timer_destroy(g_ringTimer);
			g_chatStatus = DEV_ST_CHAT_STOP;
		}
	}
#endif
	return 0;
}

int mio_chat_stop()
{
#ifdef BIZ_CLIENT_SUPPORT
	if (hwinfo.bXWNewServer)
	{
		if(g_ringTimer >= 0)
			utl_timer_destroy(g_ringTimer);
		if(g_chatStatus == DEV_ST_CHAT_CHATING)
			g_chatStatus = DEV_ST_CHAT_STOPPING;
	}
#endif
	return 0;
}


static void FM1188Write(int fd, unsigned short reg, unsigned short data)
{
	FM1188Type type;

	type.reg = reg;
	type.data = data;
	ioctl(fd, GPIO_I2C_FM1188_WRITE, (unsigned long)&type);
}

int mio_set_light_alarm_st(DEV_ST_LIGHT_ALARM stLightAlarm)
{
	devStatus.stLightAlarm = stLightAlarm;
	return 0;
}

DEV_ST_LIGHT_ALARM mio_get_light_alarm_st()
{
	return devStatus.stLightAlarm;
}

