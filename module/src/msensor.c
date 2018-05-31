#include <jv_common.h>

#include "mrecord.h"
#include "mosd.h"
#include "msnapshot.h"
#include "mdetect.h"
#include "msensor.h"
#include "mstream.h"
#include "mwdt.h"
#include <jv_ai.h>
#include <utl_cmd.h>
#include "mlog.h"
#include "jv_stream.h"
#include <utl_filecfg.h>
#include "mivp.h"
#include "mptz.h"
#include "mipcinfo.h"
#include "malarmout.h"

static msensor_attr_t _stSensorAttr;
static JVRotate_e sRotate = JVSENSOR_ROTATE_MAX;

static MLE_t	mlocalexposure;

static BOOL s_bNightModeNow = FALSE;

BOOL bWhiteLight =FALSE;//TRUE表示白光灯补光全彩模式，FALSE表示常规红外灯


static int fdAMA2 = -1;
static pthread_mutex_t mutexAMA2;  	//串口2
typedef enum
{
	ISP_CMD_BRIGHT = 0,
	ISP_CMD_BRIGHT_MOD,
	ISP_CMD_CONTRAST,
	ISP_CMD_DAYNIGHT,
	ISP_CMD_DAYNIGHT_MOD,
	ISP_CMD_DEFOG,
	ISP_CMD_WDR,
	ISP_CMD_BLC,
	ISP_CMD_SATURATION,
		
}ISP_CMD_TYPE_E;

typedef enum
{
	PTZ_CMD_MENU_ON=0,
	PTZ_CMD_MENU_OFF,
	PTZ_CMD_MENU_OK,
	PTZ_CMD_MENU_RETURN,
	PTZ_CMD_MENU_UP,
	PTZ_CMD_MENU_DOWN,
	PTZ_CMD_MENU_LEFT,
	PTZ_CMD_MENU_RIGHT,
		
}PTZ_CMD_TYPE_E;

/**
 *@brief 提取sensor effect_flag的bit 
 *
 *@param pstAttr 传入的参数
 *
 */
static int JudgeBit(unsigned int effect,int bit)
{
	return (effect>>bit) & 0x0001;
}

/**
 *@brief 通过#rotate 计算旋转后的位置
 */
int msensor_rotate_calc(JVRotate_e rotate, int viw, int vih, RECT *rect)
{
	CPrintf("rotate: %d  xywh: %d,%d,%d,%d\n", rotate, rect->x, rect->y, rect->w, rect->h);
	int temp;
	int top, left, right, bottom;
	left = rect->x;
	top = rect->y;
	right = viw - rect->w - left;
	bottom = vih - rect->h - top;
	if (rotate == JVSENSOR_ROTATE_90)
	{
		left = left * vih / viw;
		right = right * vih / viw;
		top = top * viw / vih;
		bottom = bottom * viw / vih;
		temp = left;
		left = top;
		top = right;
		right = bottom;
		bottom = temp;
	}
	else if (rotate == JVSENSOR_ROTATE_180)
	{
		temp = left;
		left = right;
		right = temp;
		temp = top;
		top = bottom;
		bottom = temp;
	}
	else if (rotate == JVSENSOR_ROTATE_270)
	{
		left = left * vih / viw;
		right = right * vih / viw;
		top = top * viw / vih;
		bottom = bottom * viw / vih;
		temp = left;
		left = bottom;
		bottom = right;
		right = top;
		top = temp;
	}
	rect->x = left;
	rect->y = top;
	rect->w = viw - right - left;
	rect->h = vih - bottom - top;
	CPrintf("xywh: %d,%d,%d,%d\n", rect->x, rect->y, rect->w, rect->h);
	return 0;
}


/**
 *@brief 设置sensor模块的参数
 *
 *@param pstAttr 传入的参数
 *
 */
void msensor_setparam(msensor_attr_t *pstAttr)
{
	if(!pstAttr)
		return;

	if(!hwinfo.wdrBsupport)
		pstAttr->bEnableWdr = FALSE;

	if (!hwinfo.rotateBSupport)
		pstAttr->rotate = JVSENSOR_ROTATE_NONE;

	sRotate = pstAttr->rotate;
	memcpy(&_stSensorAttr, pstAttr, sizeof(msensor_attr_t));

	return;
}

/**
 *@brief 获取当前的Rotate状态。
 *@note 这个状态，是刚开机时的状态。中途即使通过#msensor_setparam 修改，本函数获取的仍然是原来的值，只有重启才生效
 */
JVRotate_e msensor_get_rotate()
{
	return sRotate;
}

/**
 *@brief 获得ir_cut 的状态
 *
 *@param sensorid sensor号，目前摄像机为单sensor，传入0即可
 *@param brightness 亮度值
 *
 */
void msensor_get_ir_cut(int sensorid, int brightness)
{
//    printf("get ir_cut\n");
	_stSensorAttr.ir_cut=s_bNightModeNow;
}
/**
 *@brief 获取sensor模块的参数
 *
 *@param pstAttr 传入的参数
 *
 */
 
void msensor_getparam(msensor_attr_t *pstAttr)
{
    
	if(pstAttr)
	{
	    msensor_get_ir_cut(0,0);
		memcpy(pstAttr, &_stSensorAttr, sizeof(msensor_attr_t));
	}
}
/**
 *@brief 刷新sensor使用已保存的参数
 *
 *@param sensorid sensor号，目前摄像机为单sensor，传入0即可
 *
 */
void msensor_flush(int sensorid)
{
//    printf("msensor_flush\n");

//printf("sence:%d brightness: %d saturation: %dcontrast: %d sharpness:%d\n",_stSensorAttr.sence,_stSensorAttr.brightness,_stSensorAttr.saturation,_stSensorAttr.saturation,_stSensorAttr.contrast,_stSensorAttr.sharpness);
    DRC_t drc;
    jv_sensor_drc_get(&drc);
    drc.bDRCEnable = _stSensorAttr.drc.bDRCEnable;
    jv_sensor_drc_set(&drc);

    if(hwinfo.wdrBsupport == TRUE)
    {
        jv_sensor_wdr(sensorid, _stSensorAttr.bEnableWdr);
    }

	JVRotate_e curRotate;
	if(jv_sensor_rotate_get(&curRotate) == 0)
	{
		if(curRotate != _stSensorAttr.rotate)
		{
			int i;
			mdetect_stop(0);
			for(i=HWINFO_STREAM_CNT-1; i>=0; i--)
			{
				mstream_stop(i);
			}

			jv_sensor_rotate_set(_stSensorAttr.rotate);

			for(i=0; i<HWINFO_STREAM_CNT; i++)
			{
				mstream_attr_t attr;
				mstream_get_param(i,&attr);
				mstream_set_param(i,&attr);
				mstream_flush(i);
			}
			mivp_restart(0);	

		}
	}

    jv_sensor_set_sence(sensorid, _stSensorAttr.sence);
	jv_sensor_set_definition(sensorid,_stSensorAttr.imageD);

	jv_sensor_set_exp_mode(sensorid,_stSensorAttr.exp_mode);
	jv_sensor_set_exp_params(sensorid,_stSensorAttr.exposure);

	jv_sensor_brightness(sensorid, _stSensorAttr.brightness);
	Printf("Flush saturation: %d\n", _stSensorAttr.saturation);
    jv_sensor_contrast(sensorid,_stSensorAttr.contrast);
	jv_sensor_sharpness(sensorid, _stSensorAttr.sharpness);
	jv_sensor_antifog(sensorid, _stSensorAttr.antifog);
	jv_sensor_light(sensorid, _stSensorAttr.light);
	jv_sensor_auto_lowframe_enable(sensorid,_stSensorAttr.AutoLowFrameEn);
	BOOL turn = JudgeBit(_stSensorAttr.effect_flag, EFFECT_TURN);
	BOOL mirror = JudgeBit(_stSensorAttr.effect_flag, EFFECT_MIRROR);

	//check mirror turn need change
	{
		static int sensorTurn = -1;
		if (sensorTurn == -1)
		{
			char *vstr;
			vstr = utl_fcfg_get_value(CONFIG_HWCONFIG_FILE, "sensor-turn");
			if (vstr)
			{
				sensorTurn = atoi(vstr);
			}
			else
				sensorTurn = 0;
		}

		//sensorTurn发生在，有个相机硬件设计时，Sensor放反了，所以要整个转过来
		if (sensorTurn)
		{
			turn = !turn;
			mirror = !mirror;
		}
	}
    jv_sensor_mirror_turn(sensorid,mirror , turn );
    if(JudgeBit(_stSensorAttr.effect_flag,EFFECT_AWB))
        jv_sensor_awb(sensorid,0);
    else
        jv_sensor_awb(sensorid,1);
    
    if(JudgeBit(_stSensorAttr.effect_flag,EFFECT_AGC))
        ;//printf("open AGC!\n");
    else
        ;//printf("close AGC!\n"); 
    if(JudgeBit(_stSensorAttr.effect_flag,EFFECT_ABLC))
        ;//printf("open ABLC!\n");
    else
        ;//printf("close ABLC!\n"); 
    
    //只有白天模式，才允许切换黑白模式
    if (!s_bNightModeNow)
    {
    	jv_sensor_nocolour(sensorid,(JudgeBit(_stSensorAttr.effect_flag,EFFECT_NOCOLOUR)));

    	//黑白模式时，不设置色饱和度
    	if (!JudgeBit(_stSensorAttr.effect_flag,EFFECT_NOCOLOUR))
    	{
    		jv_sensor_saturation(sensorid, _stSensorAttr.saturation);
    	}
    }
    if(s_bNightModeNow && JudgeBit(_stSensorAttr.effect_flag,EFFECT_LOW_FRAME))
    {
          jv_sensor_low_frame( sensorid,TRUE);
    }
    else if(s_bNightModeNow)
    {
         jv_sensor_low_frame( sensorid,FALSE);
    }

    AutoExposure_t ae;
    jv_sensor_ae_get(&ae);
    ae.bAEME = _stSensorAttr.ae.bAEME;
	if(ae.bAEME)
	{
		ae.bByPassAE = _stSensorAttr.ae.bByPassAE;
		ae.exposureMax = _stSensorAttr.ae.exposureMax;
		ae.exposureMin = _stSensorAttr.ae.exposureMin;
	    ae.u16DGainMax = _stSensorAttr.ae.u16DGainMax;
	    ae.u16DGainMin = _stSensorAttr.ae.u16DGainMin;
	    ae.u16AGainMax = _stSensorAttr.ae.u16AGainMax;
	    ae.u16AGainMin = _stSensorAttr.ae.u16AGainMin;
	    ae.u32ISPDGainMax = _stSensorAttr.ae.u32ISPDGainMax;
	    ae.u32SystemGainMax = _stSensorAttr.ae.u32SystemGainMax;
	}
	else
	{
	    ae.s32AGain = _stSensorAttr.ae.s32AGain;
	    ae.s32DGain = _stSensorAttr.ae.s32DGain;
	    ae.u32ISPDGain = _stSensorAttr.ae.u32ISPDGain;
	    ae.u32ExpLine = _stSensorAttr.ae.u32ExpLine;
	    ae.bManualExpLineEnable = _stSensorAttr.ae.bManualExpLineEnable;
	    ae.bManualAGainEnable = _stSensorAttr.ae.bManualAGainEnable;
	    ae.bManualDGainEnable = _stSensorAttr.ae.bManualDGainEnable;
	    ae.bManualISPGainEnable = _stSensorAttr.ae.bManualISPGainEnable;
	}
    jv_sensor_ae_set(&ae);
}

int msensor_callback(int *param)
{
	return _stSensorAttr.bEnableSl;
}

static void* __msensor_ircut_check(void *param)
{
	int		bNightNow[2] = {-1, -1};
	U32		nSensorDelay = 0;
    int		nhwdelay = 0;
    float	oldFramerate = -1;

	pthreadinfo_add((char *)__func__);

	jv_sensor_get_b_low_frame(0, &oldFramerate, NULL);

	while (1)
	{
		//检查并切换sensor状态,lck20120401
	    if (_stSensorAttr.daynightMode == MSENSOR_DAYNIGHT_AUTO)
	    {
	    	if(hwinfo.ir_sw_mode==IRCUT_SW_BY_UART)//UART控制客户端切黑白仍显示AUTO
	    	{
	      		if(_stSensorAttr.uartdaynightMode==MSENSOR_DAYNIGHT_UART_TODAY)
	    		{
	    		 	nSensorDelay = _stSensorAttr.cutDelay*2; //使立即生效
	    		    bNightNow[0] = 0;
	    		}
	    		else if(_stSensorAttr.uartdaynightMode==MSENSOR_DAYNIGHTUART_TONIGHT)
	    		{
	    		   	nSensorDelay = _stSensorAttr.cutDelay*2; //使立即生效
	    		   	bNightNow[0] = 1;
	    		}
	    		else
	    		{
	    			bNightNow[0] = jv_sensor_b_night();
	    		}
	    		_stSensorAttr.uartdaynightMode = MSENSOR_DAYNIGHT_AUTO;
	    	}
	    	else
	    	{
				if(jv_sensor_b_daynight_usingAE())
					nSensorDelay = _stSensorAttr.cutDelay*2; //使立即生效		
				bNightNow[0] = jv_sensor_b_night();
	    	}
	    }
	    else if (_stSensorAttr.daynightMode == MSENSOR_DAYNIGHT_ALWAYS_DAY)
	    {
	    	nSensorDelay = _stSensorAttr.cutDelay*2; //使立即生效
	    	bNightNow[0] = 0;
	    }
	    else if (_stSensorAttr.daynightMode == MSENSOR_DAYNIGHT_TIMER)
	    {
	//    	static int checkCnt = 0;
	//    	checkCnt++;
	//    	//1分钟之内检查一次即可
	//    	if (checkCnt > 100)
	//    	{
	//    		checkCnt = 0;
	//
	//    	}
			time_t tt;
			struct tm tm;
			tt = time(NULL);
			localtime_r(&tt, &tm);
			int now = tm.tm_hour*60+tm.tm_min;
			int s = _stSensorAttr.dayStart.hour*60+_stSensorAttr.dayStart.minute;
			int e = _stSensorAttr.dayEnd.hour*60+_stSensorAttr.dayEnd.minute;
	//    	printf("now%d:%d, : %d, s: %d, e: %d\n", tm.tm_hour, tm.tm_min, now, s, e);
			if (s < e)
			{
				if (now >= s && now < e)
					bNightNow[0] = FALSE;
				else
					bNightNow[0] = TRUE;
			}
			else //日夜颠倒
			{
				if (now >= e && now < s)
					bNightNow[0] = TRUE;
				else
					bNightNow[0] = FALSE;
			}

	    }
	    else
	    {
	    	nSensorDelay = _stSensorAttr.cutDelay*2; //使立即生效
	    	bNightNow[0] = 1;
	    }
		jv_sensor_set_daynight_mode(0, _stSensorAttr.daynightMode);

		if(bNightNow[0] != bNightNow[1])
		{
			int cutDelay = _stSensorAttr.cutDelay*2;
			//由于延时的不准确性，带ADC的机器本函数会阻塞500ms，实际情况是1s检查一次，真正的3s
			if(++nSensorDelay >= cutDelay)
			{
				if(bNightNow[0])
				{
					s_bNightModeNow = TRUE;
					//置状态后，延时一下，避免其它地方因状态判断出错
					usleep(10*1000);
					printf("Trun to NightMode...\n");

					jv_sensor_nocolour(0, TRUE);
					usleep(10*1000);
	                jv_sensor_set_nightmode(TRUE);
					jv_sensor_nocolour(0, TRUE);
					jv_sensor_set_ircut(TRUE);
					mivp_set_day_night_mode(TRUE);
					jv_sensor_sharpness(0, _stSensorAttr.sharpness);
				    if(JudgeBit(_stSensorAttr.effect_flag,EFFECT_LOW_FRAME))
				    {
				          jv_sensor_low_frame( 0,TRUE);
				    }
					else
					{
						jv_sensor_low_frame( 0,FALSE);

					}
				}
				else
				{
					printf("Trun to DayMode...\n");

					jv_sensor_set_ircut(FALSE);
					jv_sensor_set_nightmode(FALSE);
					mivp_set_day_night_mode(FALSE);
					jv_sensor_nocolour(0, FALSE);
					jv_sensor_saturation(0, _stSensorAttr.saturation);
					jv_sensor_sharpness(0, _stSensorAttr.sharpness);
					jv_sensor_low_frame( 0,FALSE);

					s_bNightModeNow = FALSE;
				}

				bNightNow[1] = bNightNow[0];
				nSensorDelay = 0;
				//msensor_flush(0); 暂时不刷新图像参数
			}
		}
		else
		{
			nSensorDelay = 0;
		}

		{
			float curFramerate = 0;
			jv_sensor_get_b_low_frame(0, &curFramerate, NULL);
			if (oldFramerate != curFramerate)
			{
				int i;
				jv_stream_attr attr;
				for (i=0;i<HWINFO_STREAM_CNT;i++)
				{
					mchnosd_stop(i);
					mchosd_region_stop();
					jv_stream_stop(i);
					jv_stream_start(i);
					ipcinfo_t ipcinfo;
					ipcinfo_get_param(&ipcinfo);
					multiosd_process(&ipcinfo);
					mchnosd_flush(i);
				}
				mrecord_stop(0);
				mrecord_flush(0);
				oldFramerate = curFramerate;
			}
		}

		usleep(1000 * 1000);
	}

	//Printf("sensor check running...%d == %d, nSensorDelay: %d\n", nSensorMode[0] ,nSensorMode[1], nSensorDelay);
	return NULL;
}

//#define CUT_TEST
//#ifdef CUT_TEST
static void __cut_test(void *param)
{
	pthreadinfo_add((char *)__func__);

	while(1)
	{
		sleep(5);	//5s之后开始测试
		while(_stSensorAttr.stCutTest.bOpenCutTest && _stSensorAttr.stCutTest.nCountTimes!=0)
		{
			jv_sensor_set_ircut(TRUE);
			sleep(1);
			jv_sensor_set_ircut(FALSE);
			if(_stSensorAttr.stCutTest.nCountTimes>0)
				_stSensorAttr.stCutTest.nCountTimes-- ;
			if(_stSensorAttr.stCutTest.nInterval>0)
				sleep(_stSensorAttr.stCutTest.nInterval);
		}
	}
}
//#endif

/**
 *@brief 刷新sensor使用已保存的参数
 *
 *@param sensorid sensor号，目前摄像机为单sensor，传入0即可
 *
 */
void msensor_init(int sensorid)
{
    int id;
  //  jv_sensor_wdr(sensorid, _stSensorAttr.bEnableWdr);
    msensor_set_local_exposure(0,&mlocalexposure);

	pthread_t pid_check;
	pthread_create_detached(&pid_check, NULL, __msensor_ircut_check, NULL);

	jv_msensor_set_callback(msensor_callback);

//#ifdef CUT_TEST
	pthread_t pid;
//	_stSensorAttr.stCutTest.nCountTimes = -1;
//	_stSensorAttr.stCutTest.bOpenCutTest = 1;
//	_stSensorAttr.stCutTest.nInterval = 0;
//	pthread_create_detached(&pid, NULL, (void *) __cut_test, NULL);
//#endif
}

int msensor_get_focus_value(int sensorid)
{
	extern int focus_reference_value;
	int ret = jv_sensor_get_focus_value(0);
//	printf("ret: %d\n", ret);
	if (ret <= 0)
	{
//		printf("now get ret: %d\n", focus_reference_value);
		ret = focus_reference_value;
	}
	return ret;
}

#define LKDEBUG 1
/**
 * @brief 设置局部曝光参数
 *
 * @param
 */
int msensor_set_local_exposure(int sensorid,MLE_t *amle)
{
	MLE_t _smle;
	MLE_t *mle;
	mle = &_smle;
	memcpy(mle, amle, sizeof(MLE_t));
#if LKDEBUG//在调试的
	unsigned int viW,viH;//视频的宽度，高度。
	int rectW,rectH;//底层每个矩形的宽度，高度。
	int x,y,w,h;
	int n,i,j;
	JV_SENSOR_LE_t jv_sensor_le;
	U32 weight = mle->nLE_Weight*14/255;
	mle->nLE_RectNum = 0;
	for (i=0;i<RECT_MAX_CNT;i++)
			if (mle->stLE_Rect[i].w != 0 && mle->stLE_Rect[i].h != 0)
				mle->nLE_RectNum++;

	memset(&jv_sensor_le,0,sizeof(JV_SENSOR_LE_t));
#if 1//默认值非全1，中间为2是为了变化更加明显，设置局部曝光时全部改为1，防止中间默认为2进行干扰
	if (!mle->bLE_Reverse)
	{
		for (i = 0; i < WEIGHT_ZONE_ROW; i++)
			for (j = 0; j < WEIGHT_ZONE_COLUMN; j++)
				jv_sensor_le.u8Weight[i][j] = 1;
	}
	else
	{
		weight = -weight;
		for (i = 0; i < WEIGHT_ZONE_ROW; i++)
			for (j = 0; j < WEIGHT_ZONE_COLUMN; j++)
				jv_sensor_le.u8Weight[i][j] = 15;
	}

#else
	//jv_sensor_get_local_exposure(0,&jv_sensor_le);
#endif
	memcpy(&mlocalexposure,mle,sizeof(MLE_t));
	jv_stream_get_vi_resolution(0, &viW, &viH);
	rectW = viW/WEIGHT_ZONE_COLUMN;
	rectH = viH/WEIGHT_ZONE_ROW;
//	printf("viW:%d,viH:%d,w:%d,h:%d\n",viW,viH,mle->stLE_Rect[0].w,mle->stLE_Rect[0].h);
//	printf("********************SET***********************\n");
	if(mle->nLE_RectNum == 0 || weight == 0)
	{
#if 1//没有局部曝光时恢复默认值
		msensor_exposure_set_default(0);
		return 0;
#else//用户可以设置全局权重，利用全局与局部权值的翻转实际可以实现选中哪一块曝哪一块，或者是选中哪一块排除哪一块
		//根据需求现在只有选中哪一块曝哪一块
//		for(i=0;i<WEIGHT_ZONE_ROW;i++)
//		{
//			for(j=0;j<WEIGHT_ZONE_COLUMN;j++)
//			{
//				jv_sensor_le.u8Weight[i][j] = mle->nLE_Weight;
//				printf(" %2d",mle->nLE_Weight);
//			}
//			printf("\n");
//		}
#endif
	}
	else
	{
		for(n = 0;n<mle->nLE_RectNum;n++)
		{
			if (sRotate != JVSENSOR_ROTATE_NONE)
				msensor_rotate_calc(sRotate, viW, viH, &mle->stLE_Rect[n]);
			x = mle->stLE_Rect[n].x/rectW;
			w = (mle->stLE_Rect[n].w+rectW-1)/rectW;
			y = mle->stLE_Rect[n].y/rectH;
			h = (mle->stLE_Rect[n].h+rectH-1)/rectH;
			if(w>WEIGHT_ZONE_COLUMN)
				w = WEIGHT_ZONE_COLUMN;
			if(h>WEIGHT_ZONE_ROW)
				h = WEIGHT_ZONE_ROW;
			for(i=0;i<h;i++)
			{
				for(j=0;j<w;j++)
				{
					jv_sensor_le.u8Weight[i+y][j+x] += weight;
//					printf(" %2d",weight);
				}
//				printf("\n");
			}
		}
	}
	jv_sensor_set_local_exposure(0,&jv_sensor_le);
#endif
	return 0;
}
/*
 * @brief 获取局部曝光参数
 */
int msensor_get_local_exposure(int sensorid,MLE_t *mle)
{
#if LKDEBUG//在调试
	int i,j;
	memcpy(mle,&mlocalexposure,sizeof(MLE_t));
	JV_SENSOR_LE_t jv_sensor_le;
	jv_sensor_get_local_exposure(0,&jv_sensor_le);
//	printf("********************GET***********************\n");
//	for(i=0;i<15;i++)
//	{
//		for(j=0;j<17;j++)
//			printf(" %2d",jv_sensor_le.u8Weight[i][j]);
//		printf("\n");
//	}
#endif
	return 0;
}
/*
 * @brief 设置默认值
 */
int msensor_exposure_set_default(int sensorid)
{
#if LKDEBUG//在调试
	JV_SENSOR_LE_t jv_sensor_le;
	U8 exposure_default_weight[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN] =
	{
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
			{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
			{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
			{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
			{1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,1},
			{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
			{1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

	};
	memcpy(&(jv_sensor_le.u8Weight),&(exposure_default_weight),WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN);
	jv_sensor_set_local_exposure(0,&jv_sensor_le);
#endif
	return 0;
}

/**
 * 获取数字防抖状态
 * 返回值：返回数字防抖开启关闭状态，1为开启0为关闭
 */
int msensor_dis_get(int sensorid)
{
	if(_stSensorAttr.bDISOpen)
		return 1;
	return 0;
}
/*
 *设置数字防抖状态
 *bDISOpen 数字防抖开启状态0为关闭，1为开启
 *返回值：0成功-1失败
 */
int msensor_dis_set(int snesorid,BOOL bDISOpen)
{
#if LKDEBUG
	_stSensorAttr.bDISOpen = bDISOpen;
	jv_sensor_set_dis(0,bDISOpen);
#endif
	return 0;
}


BOOL msensor_mode_get()
{
	return s_bNightModeNow;
}

/* 设置白光灯是否使能 */
void  msensor_set_whitelight_function(BOOL bEnable)
{
#ifdef PLATFORM_hi3516EV100 
	jv_sensor_set_whitelight_function(bEnable);
#endif
}

BOOL  msensor_get_whitelight_function()
{
#ifdef PLATFORM_hi3516EV100 
	return jv_sensor_get_whitelight_function();
#else
	return FALSE;
#endif
}

/* 白光灯报警闪烁时告知 */
void msensor_set_alarm_light(BOOL bEnable)
{
#ifdef PLATFORM_hi3516EV100 
	jv_sensor_set_alarm_light(bEnable);
#endif
}

/* 获取当前全彩模式下的白光灯状态 */
BOOL msensor_get_whitelight_status()
{
#ifdef PLATFORM_hi3516EV100 
	return jv_sensor_get_whitelight_status();
#else
	return FALSE;
#endif
}

#if 0
#include <utl_cmd.h>

int sensor_main(int argc, char *argv[])
{
	if (argc < 2)
	{
		return -1;
	}
	if (strcmp(argv[1], "night") == 0)
	{
		static BOOL bNight = FALSE;
//		jv_sensor_set_test_night_mode(bNight);
		printf("Now mode: %s\n", bNight ? "Night": "Day");
		bNight = !bNight;
	}
	else if (strcmp(argv[1], "wdr") == 0)
	{
		if (strcmp(argv[2], "on") == 0)
		{
			jv_sensor_wdr(0, TRUE);
		}
		else
			jv_sensor_wdr(0, FALSE);
	}
//	else if (strcmp(argv[1], "sensor") == 0)
//	{
//	}
	else
	{
	}
	return 0;
}

int sensor_test_init() __attribute__((constructor)) ;

int sensor_test_init()
{
	char *helper = "\tnight night mode switch\n"
			"\twdr [on/off]\n";
	utl_cmd_insert("sensor", "sensor [cmd]", helper, sensor_main);
	return 0;
}
#endif

