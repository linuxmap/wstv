#include "jv_common.h"
#include "hicommon.h"

#include <hi_sns_ctrl.h>
#include <jv_sensor.h>
#include <utl_filecfg.h>
#include "3518_isp.h"
#include <mpi_ae.h>
#include <mpi_awb.h>
#include <mpi_af.h>
#include "hi_comm_isp.h"
#include "hi_comm_3a.h"
#include "hi_awb_comm.h"
#include "hi_mipi.h"
#include "jv_spi_flash.h"
#include <termios.h>
#include "jv_gpio.h"
#include "jv_ai.h"
#include "utl_ifconfig.h"

int flash_write_lock_fd = -1;
static HI_U16 ispFramerate = 25;	//0时默认初始化30帧，30帧重启会变成25帧。

int MAX_FRAME_RATE = 30;
int VI_WIDTH = 1280;
int VI_HEIGHT = 960;

BOOL VI_CROP_ENABLE= FALSE;
int VI_CROP_X  =0;
int VI_CROP_Y  =0;
int VI_CROP_W  =1920;
int VI_CROP_H = 1080;


int jv_sensor_wdr_mode_set(WDR_MODE_E mode);

static JVCommonParam_t stJvParam;

#define SAMPLE_PIXEL_FORMAT	PIXEL_FORMAT_YUV_SEMIPLANAR_420

#define sns_type hwinfo.sensor

static pthread_t gs_IspPid;

hwinfo_t hwinfo;
higpio_values_t higpios;

static void __gpio_clear()
{
	int cnt;
	int i;
	GpioValue_t value = {-1, -1};

	GpioValue_t *ptr = (GpioValue_t *)&higpios;
	cnt = sizeof(higpios) / sizeof(GpioValue_t);
	for(i=0; i<cnt; i++)
	{
		memcpy(ptr+i, &value, sizeof(GpioValue_t));
	}
}

static void __gpio_init()
{
	int dir = 0;
	__gpio_clear();

	//CUT
	higpios.cutday.group = 8;
	higpios.cutday.bit = 0;
	jv_gpio_muxctrl(0x200F0100, 1);//8_0
	jv_gpio_dir_set_bit(higpios.cutday.group, higpios.cutday.bit, 1);

	higpios.cutnight.group = 8;
	higpios.cutnight.bit = 1;
	jv_gpio_muxctrl(0x200F0104, 1);//8_1
	jv_gpio_dir_set_bit(higpios.cutnight.group, higpios.cutnight.bit, 1);

	//WIFI上电
	utl_system("sync;echo 3 > /proc/sys/vm/drop_caches");
	jv_gpio_muxctrl(0x200F00E0, 0);
	jv_gpio_dir_set_bit(7, 0, 1);
	jv_gpio_write(7,0,1);

	//Sensor Reset
	higpios.sensorreset.group = 0;
	higpios.sensorreset.bit = 5;
	jv_gpio_muxctrl(0x200F0004, 1);//0_5
	dir = jv_gpio_dir_get(higpios.sensorreset.group);
	dir |= 1 << higpios.sensorreset.bit;
	jv_gpio_dir_set(higpios.sensorreset.group, dir);
	jv_gpio_write(higpios.sensorreset.group, higpios.sensorreset.bit, 0);
	usleep(100*1000);
	jv_gpio_write(higpios.sensorreset.group, higpios.sensorreset.bit, 1);
	usleep(100*1000);

	if (HWTYPE_MATCH(HW_TYPE_V6)
		|| HWTYPE_MATCH(HW_TYPE_HA210)
		|| HWTYPE_MATCH(HW_TYPE_HA230))
	{
		//IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 7;
		jv_gpio_muxctrl(0x200F00FC, 1);//7_7
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);

		//one minute rec
		higpios.oneminrec.group = 6;
		higpios.oneminrec.bit = 7;
		jv_gpio_muxctrl(0x200F00DC, 0);
		jv_gpio_dir_set_bit(higpios.oneminrec.group, higpios.oneminrec.bit, 0);	

		//reset key
		higpios.resetkey.group = 7;
		higpios.resetkey.bit = 1;
		jv_gpio_muxctrl(0x200F00E4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//set led R
		higpios.statusledR.group = 0;
		higpios.statusledR.bit = 0;
		jv_gpio_muxctrl(0x200F0070, 0);
		jv_gpio_dir_set_bit(higpios.statusledR.group, higpios.statusledR.bit, 1);
		
		//set led G 
		higpios.statusledG.group = 0;
		higpios.statusledG.bit = 1;
		jv_gpio_muxctrl(0x200F0074, 0);
		jv_gpio_dir_set_bit(higpios.statusledG.group, higpios.statusledG.bit, 1);

		//connect led B
		higpios.statusledB.group = 0;
		higpios.statusledB.bit = 2;
		jv_gpio_muxctrl(0x200F0078, 0);
		jv_gpio_dir_set_bit(higpios.statusledB.group, higpios.statusledB.bit, 1);	
	}
	else if (HWTYPE_MATCH(HW_TYPE_A4))
	{
		//IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 7;
		jv_gpio_muxctrl(0x200F00FC, 1);//7_7
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);

		//reset key
		higpios.resetkey.group = 7;
		higpios.resetkey.bit = 1;
		jv_gpio_muxctrl(0x200F00E4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//set led R
		higpios.statusledR.group = 0;
		higpios.statusledR.bit = 0;
		higpios.statusledR.onlv = 1;
		jv_gpio_muxctrl(0x200F0070, 0);
		jv_gpio_dir_set_bit(higpios.statusledR.group, higpios.statusledR.bit, 1);

		//set led B
		higpios.statusledB.group = 0;
		higpios.statusledB.bit = 1;
		higpios.statusledB.onlv = 1;
		jv_gpio_muxctrl(0x200F0074, 0);
		jv_gpio_dir_set_bit(higpios.statusledB.group, higpios.statusledB.bit, 1);
	}
	else if (HWTYPE_MATCH(HW_TYPE_V3))
	{
		//IR LED
		jv_gpio_muxctrl(0x200F00EC, 1);
		jv_gpio_dir_set_bit(7, 3, 1);
		jv_gpio_write(7, 3, 1);
		higpios.redlight.group = 7;
		higpios.redlight.bit = 4;
		jv_gpio_muxctrl(0x200F00F0, 1);
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);

		//reset key
		higpios.resetkey.group = 7;
		higpios.resetkey.bit = 1;
		jv_gpio_muxctrl(0x200F00E4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//set led R
		higpios.statusledR.group = 0;
		higpios.statusledR.bit = 0;
		jv_gpio_muxctrl(0x200F0070, 0);
		jv_gpio_dir_set_bit(higpios.statusledR.group, higpios.statusledR.bit, 1);
		
		//connect led B
		higpios.statusledB.group = 0;
		higpios.statusledB.bit = 2;
		jv_gpio_muxctrl(0x200F0078, 0);
		jv_gpio_dir_set_bit(higpios.statusledB.group, higpios.statusledB.bit, 1);	
		jv_gpio_write(higpios.statusledR.group, higpios.statusledR.bit, 0);
		jv_gpio_write(higpios.statusledB.group, higpios.statusledB.bit, 1);
	}
	else if (
			HWTYPE_MATCH(HW_TYPE_C5)
			|| HWTYPE_MATCH(HW_TYPE_C9)
			)
	{
		//IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 7;
		jv_gpio_muxctrl(0x200F00FC, 1);//7_7
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);
	
		//reset key
		higpios.resetkey.group = 6;
		higpios.resetkey.bit = 5;
		jv_gpio_muxctrl(0x200F00D4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//set led G 
		higpios.statusledG.group = 0;
		higpios.statusledG.bit = 0;
		jv_gpio_muxctrl(0x200F0070, 0);
		jv_gpio_dir_set_bit(higpios.statusledG.group, higpios.statusledG.bit, 1);
	}
	else if(HWTYPE_MATCH(HW_TYPE_C3))
	{
	    //IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 7;
		jv_gpio_muxctrl(0x200F00FC, 1);//7_7
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);
	
		//reset key
		higpios.resetkey.group = 6;
		higpios.resetkey.bit = 5;
		jv_gpio_muxctrl(0x200F00D4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//set led R
		higpios.statusledR.group = 0;
		higpios.statusledR.bit = 0;
		jv_gpio_muxctrl(0x200F0070, 0);
		jv_gpio_dir_set_bit(higpios.statusledR.group, higpios.statusledR.bit, 1);
		jv_gpio_write(higpios.statusledR.group, higpios.statusledR.bit, 0);

		//connect led B
		higpios.statusledB.group = 0;
		higpios.statusledB.bit = 2;
		jv_gpio_muxctrl(0x200F0078, 0);
		jv_gpio_dir_set_bit(higpios.statusledB.group, higpios.statusledB.bit, 1);	
		jv_gpio_write(higpios.statusledB.group, higpios.statusledB.bit, 1);
	}
	else if (
			HWTYPE_MATCH(HW_TYPE_C8A)
			|| HWTYPE_MATCH(HW_TYPE_V8)
			)
	{
	    //IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 7;
		jv_gpio_muxctrl(0x200F00FC, 1);//7_7
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);

		//reset key
		higpios.resetkey.group = 6;
		higpios.resetkey.bit = 5;
		jv_gpio_muxctrl(0x200F00D4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);
	}
	else if(HWTYPE_MATCH(HW_TYPE_V7)
			|| HWTYPE_MATCH(HW_TYPE_C8)
			)
	{
	    //IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 3;
		jv_gpio_muxctrl(0x200F00EC, 1);//7_3
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);

		//reset key
		higpios.resetkey.group = 6;
		higpios.resetkey.bit = 5;
		jv_gpio_muxctrl(0x200F00D4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);
	}
	else if (PRODUCT_MATCH("HXBJRB"))
	{
		jv_gpio_muxctrl(0x200F00CC, 3);  //UART2_RXD
		jv_gpio_muxctrl(0x200F00D0, 3);  //UART2_TXD

		//audio out mute
		higpios.audioOutMute.group = 6;
		higpios.audioOutMute.bit = 7;
		jv_gpio_muxctrl(0x200F00DC, 0);
		jv_gpio_dir_set_bit(higpios.audioOutMute.group, higpios.audioOutMute.bit, 1);

		//reset
		higpios.resetkey.group = 6;
		higpios.resetkey.bit = 5;
		jv_gpio_muxctrl(0x200F00D4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//status led
		higpios.statusledG.group = 0;
		higpios.statusledG.bit = 1;
		jv_gpio_muxctrl(0x200F0074, 0);
		jv_gpio_dir_set_bit(higpios.statusledG.group, higpios.statusledG.bit, 1);
	}
	else
	{
		//IR LED
		higpios.redlight.group = 7;
		higpios.redlight.bit = 7;
		jv_gpio_muxctrl(0x200F00FC, 1);//7_7
		jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);
	
		//reset key
		higpios.resetkey.group = 6;
		higpios.resetkey.bit = 5;
		jv_gpio_muxctrl(0x200F00D4, 0);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//set led G 
		higpios.statusledG.group = 0;
		higpios.statusledG.bit = 0;
		jv_gpio_muxctrl(0x200F0070, 0);
		jv_gpio_dir_set_bit(higpios.statusledR.group, higpios.statusledR.bit, 1);
	}
}

BOOL __judgebEMMC()
{
	BOOL bEMMC = FALSE;
	char cmdline[1024] = {0};

	FILE* fp = fopen("/proc/cmdline", "r");
	if (fp == NULL)
	{
		return bEMMC;
	}

	if (fread(cmdline, 1, sizeof(cmdline), fp) <= 0)
	{
		goto Exit_File;
	}

	if (strstr(cmdline, "blkdevparts") != NULL)
	{
		bEMMC = TRUE;
	}

Exit_File:
	fclose(fp);
	return bEMMC;
}

static void __checkHwType()
{
	if (PRODUCT_MATCH(PRODUCT_HA210))
	{
		hwinfo.HwType = HW_TYPE_HA210;
	}
	else if (PRODUCT_MATCH(PRODUCT_HA230)
			|| PRODUCT_MATCH(PRODUCT_HA230C)
			|| PRODUCT_MATCH(PRODUCT_HA230E)
			|| PRODUCT_MATCH(PRODUCT_HA230_AP))
	{
		hwinfo.HwType = HW_TYPE_HA230;
	}
	else if (PRODUCT_MATCH(PRODUCT_HA410)
			|| PRODUCT_MATCH(PRODUCT_ZHDZ)
			|| PRODUCT_MATCH(PRODUCT_AB1))
	{
		hwinfo.HwType = HW_TYPE_A4;
	}
	else if (PRODUCT_MATCH(PRODUCT_C3H)
			|| PRODUCT_MATCH(PRODUCT_C4L)
			|| PRODUCT_MATCH(PRODUCT_C3HE)
			|| PRODUCT_MATCH(PRODUCT_C3H_AP)
			|| PRODUCT_MATCH(PRODUCT_C3C)
			|| PRODUCT_MATCH(PRODUCT_DQP_YTJ))
	{
		hwinfo.HwType = HW_TYPE_C3;
	}
	else if (PRODUCT_MATCH(PRODUCT_C5)
			|| PRODUCT_MATCH(PRODUCT_C5_R1)
			|| PRODUCT_MATCH(PRODUCT_C5S)
			|| PRODUCT_MATCH(PRODUCT_C5S_M)
			|| PRODUCT_MATCH(PRODUCT_C6S))
	{
		hwinfo.HwType = HW_TYPE_C5;
	}
	else if (PRODUCT_MATCH(PRODUCT_V8A)
			|| PRODUCT_MATCH(PRODUCT_PBS)
			|| PRODUCT_MATCH(PRODUCT_C8A)
			|| PRODUCT_MATCH(PRODUCT_C8E)
			|| PRODUCT_MATCH(PRODUCT_C8A_EN))
	{
		hwinfo.HwType = HW_TYPE_C8A;
	}
	else if (PRODUCT_MATCH(PRODUCT_C8))
	{
		hwinfo.HwType = HW_TYPE_C8;
	}
	else if (PRODUCT_MATCH(PRODUCT_C9))
	{
		hwinfo.HwType = HW_TYPE_C9;
	}
	else if (PRODUCT_MATCH(PRODUCT_V3)
			|| PRODUCT_MATCH(PRODUCT_V3H))
	{
		hwinfo.HwType = HW_TYPE_V3;
	}
	else if (PRODUCT_MATCH(PRODUCT_V6))
	{
		hwinfo.HwType = HW_TYPE_V6;
	}
	else if (PRODUCT_MATCH(PRODUCT_V7))
	{
		hwinfo.HwType = HW_TYPE_V7;
	}
	else if (PRODUCT_MATCH(PRODUCT_V8))
	{
		hwinfo.HwType = HW_TYPE_V8;
	}
	else
	{
		hwinfo.HwType = HW_TYPE_UNKNOWN;
		printf("%s, ===Warnning, Unknown HW Type, name: %s!!!\n", __func__, hwinfo.devName);
	}
}

void hwinfo_init()
{
	memset(&hwinfo, 0, sizeof(hwinfo));
    strcpy(hwinfo.type,"Hi3518EV200");
	hwinfo.ptzBsupport = FALSE;
	hwinfo.ptzbaudrate = 2400;
	hwinfo.ptzprotocol = 29; //标准 PELCO-D (BandRate = 2400)
    hwinfo.wdrBsupport=TRUE;
    hwinfo.rotateBSupport=TRUE;
    hwinfo.encryptCode = ENCRYPT_130W;
    hwinfo.streamCnt = 2;
    hwinfo.ipcType = stJvParam.ipcType;
    hwinfo.ir_sw_mode = IRCUT_SW_BY_ADC1;
	hwinfo.bHomeIPC = 1;
	hwinfo.bSupportVoiceConf = TRUE;	// 默认支持声波配置
    hwinfo.ir_power_holdtime = 300*1000;
    strcpy(hwinfo.product, "Unknown");

    utl_fcfg_get_value_ex(JOVISION_CONFIG_FILE, "product", hwinfo.product, sizeof(hwinfo.product));
	utl_fcfg_get_value_ex(CONFIG_HWCONFIG_FILE, "product", hwinfo.devName, sizeof(hwinfo.devName));

	// 检查硬件型号
	__checkHwType();

	// 光敏
	hwinfo.ir_sw_mode = IRCUT_SW_BY_AE;		// 目前均为软光敏

	// 云台
	if (HWTYPE_MATCH(HW_TYPE_C3)
		|| (HWTYPE_MATCH(HW_TYPE_C5))
		|| (HWTYPE_MATCH(HW_TYPE_C9))
		|| (HWTYPE_MATCH(HW_TYPE_V3))
		|| (HWTYPE_MATCH(HW_TYPE_V6)))
	{
		hwinfo.bSoftPTZ = 1;
	}

	if (PRODUCT_MATCH(PRODUCT_V3H)
		|| PRODUCT_MATCH(PRODUCT_C3H)
		|| PRODUCT_MATCH(PRODUCT_C3W)
		|| PRODUCT_MATCH(PRODUCT_C4L)
		|| PRODUCT_MATCH(PRODUCT_C3H_AP)
		|| PRODUCT_MATCH(PRODUCT_C5S)
		|| PRODUCT_MATCH(PRODUCT_C5S_M)
		|| PRODUCT_MATCH(PRODUCT_C3C)
		|| PRODUCT_MATCH(PRODUCT_V6)
		|| PRODUCT_MATCH(PRODUCT_C6S))
	{
		hwinfo.bSupport3DLocate = 1;
	}

	// 报警服务选择，默认使用小维报警服务
	if (PRODUCT_MATCH(PRODUCT_HA230C))
	{
		hwinfo.bXWNewServer = FALSE;
		hwinfo.bNewVoiceDec = FALSE;
		hwinfo.bCloudSee = TRUE;
	}
	else
	{
		hwinfo.bXWNewServer = TRUE;
		hwinfo.bNewVoiceDec = TRUE;
		hwinfo.bCloudSee = FALSE;
	}

	// 音频模式选择，默认为双向对讲
	if (PRODUCT_MATCH(PRODUCT_C8A)
		|| PRODUCT_MATCH(PRODUCT_C8A_EN)
		|| PRODUCT_MATCH(PRODUCT_C8E))
	{
		hwinfo.remoteAudioMode = AUDIO_MODE_NO_WAY;
	}
	else
	{
		hwinfo.remoteAudioMode = AUDIO_MODE_TWO_WAY;
	}

	// 使用AP的设备把声波关掉，以免影响AP下音频
	if (utl_ifconfig_bsupport_apmode(utl_ifconfig_wifi_get_model()))
	{
		hwinfo.bNewVoiceDec = FALSE;
		hwinfo.bSupportVoiceConf = FALSE;
	}

	// 判断是否是EMMC
	hwinfo.bEMMC = __judgebEMMC();

	// 视频协议选择，默认使用云视通
	if (PRODUCT_MATCH(PRODUCT_V8) ||
		PRODUCT_MATCH(PRODUCT_V8A) ||
		PRODUCT_MATCH(PRODUCT_HA410) ||
		PRODUCT_MATCH(PRODUCT_ZHDZ) ||
		PRODUCT_MATCH(PRODUCT_V3) ||
		PRODUCT_MATCH(PRODUCT_V3H) ||
		PRODUCT_MATCH(PRODUCT_V6) ||
		PRODUCT_MATCH(PRODUCT_V7))
	{
		gp.bNeedOnvif = TRUE;
		gp.bNeedZrtsp = TRUE;
		gp.bNeedWeb = TRUE;
	}
	else if (PRODUCT_MATCH(PRODUCT_C4L))		// 只开启流媒体协议
	{
	}
	else
	{
		gp.bNeedYST = TRUE;
	}

	// 小维国际版
	if (PRODUCT_MATCH(PRODUCT_C8E) ||
		PRODUCT_MATCH(PRODUCT_C8A_EN) ||
		PRODUCT_MATCH(PRODUCT_HA230E) ||
		PRODUCT_MATCH(PRODUCT_C3HE))
	{
		hwinfo.bInternational = TRUE;
	}

	// V6开启智能存储功能
	if (PRODUCT_MATCH(PRODUCT_V6) ||
		PRODUCT_MATCH(PRODUCT_V3H)||
		PRODUCT_MATCH(PRODUCT_C6S))
	{
		hwinfo.bSupportAVBR = TRUE;
		hwinfo.bSupportSmtVBR = TRUE;
	}

	// VGS画线功能
	if (PRODUCT_MATCH(PRODUCT_C6S))
	{
		hwinfo.bVgsDrawLine = TRUE;
	}
	// 小维智能分析功能
	if (PRODUCT_MATCH(PRODUCT_C6S))
	{
		hwinfo.bSupportMVA = TRUE;
		hwinfo.bSupportReginLine = TRUE;
		hwinfo.bSupportLineCount = TRUE;
		hwinfo.bSupportHideDetect = TRUE;
		hwinfo.bSupportClimbDetect =TRUE;
	}
	// 摄录一体机去掉Onvif支持,有需要单独开启
#if 0
	if (PRODUCT_MATCH(PRODUCT_C8A)
		|| PRODUCT_MATCH(PRODUCT_C8))
	{
		gp.bNeedOnvif = TRUE;
		gp.bNeedZrtsp = TRUE;
		gp.bNeedWeb = TRUE;
	}
#endif

	// 工厂检测模式，只用云视通，关闭流媒体和Onvif
	if (gp.bFactoryFlag)
	{
		gp.bNeedYST = TRUE;
		gp.bNeedOnvif = FALSE;
		gp.bNeedZrtsp = FALSE;
		gp.bNeedWeb = FALSE;
	}

	// 是否支持云存储，默认支持
	if (PRODUCT_MATCH(PRODUCT_PBS)
		|| PRODUCT_MATCH(PRODUCT_ZHDZ)
		|| hwinfo.bInternational
		|| hwinfo.bCloudSee)
	{
		hwinfo.bSupportXWCloud = FALSE;
	}
	else
	{
		hwinfo.bSupportXWCloud = TRUE;
	}

	// 支持预置点和巡航功能
	// hwinfo.bSupportPatrol = TRUE;

	__gpio_init();
}

VI_DEV_ATTR_S DEV_ATTR_OV9732_MIPI_720P =
{
    /* interface mode */
    VI_MODE_MIPI,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFC00000,    0x0},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

    /* synchronization information */
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,
   
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            720,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /* use interior ISP */
    VI_PATH_ISP,
    /* input data type */
    VI_DATA_TYPE_RGB,    
    /* bRever */
    HI_FALSE,    
    /* DEV CROP */
    {0, 0, 1280, 720}
};

VI_DEV_ATTR_S DEV_ATTR_OV9750_DC_960P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    /*VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,*/
	VI_VSYNC_FIELD, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            960,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* bRever */
    HI_FALSE,    
    /* DEV CROP */
    {0, 0, 1280,960}
};

/*OV2710 DC 10bit输入*/
VI_DEV_ATTR_S DEV_ATTR_OV2710_DC_1080P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0x3ff0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    //VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
     VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
        
    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* bRevert */
    HI_FALSE,
    /* stDevRect */
    {0, 0, 1920, 1080} 
};
/*SC2045 DC 10bit输入*/
VI_DEV_ATTR_S DEV_ATTR_SC2045_DC_1080P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0x3ff0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    //VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
     VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
        
    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* bRevert */
    HI_FALSE,
    /* stDevRect */
    {0, 0, 1920, 1080} 
};


VI_DEV_ATTR_S DEV_ATTR_OV9750_MIPI_960p =
{
    /* interface mode */
    VI_MODE_MIPI,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF00000,    0x0},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

    /* synchronization information */
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,
   
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            960,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /* use interior ISP */
    VI_PATH_ISP,
    /* input data type */
    VI_DATA_TYPE_RGB,    
    /* bRever */
    HI_FALSE,    
    /* DEV CROP */
    {0, 0, 1280, 960}
};


VI_DEV_ATTR_S DEV_ATTR_AR0130_DC_960P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    /*VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,*/
	VI_VSYNC_FIELD, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            960,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* bRever */
    HI_FALSE,    
    /* DEV CROP */
    {0, 0, 1280,960}
};

VI_DEV_ATTR_S DEV_ATTR_SC2135_DC_1080P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFC0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    /*VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,*/
	VI_VSYNC_PULSE,VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,

    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* bRever */
    HI_FALSE,    
    /* DEV CROP */
    {0, 0, 1920,1080}
};


VI_DEV_ATTR_S DEV_ATTR_OV9732_DC_720P =
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0x3FF0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,

    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            720,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
        /* bRever */
    HI_FALSE,    
    /* DEV CROP */
    {0, 0, 1280, 720}
};
/*OV2735 DC 10bit输入*/
VI_DEV_ATTR_S DEV_ATTR_OV2735_DC_1080P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0x3FF0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
     VI_VSYNC_FIELD, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
        
    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* bRevert */
    HI_FALSE,
    /* stDevRect */
    {0, 0, 1920, 1080} 
};

combo_dev_attr_t DVP_2lane_SENSOR_OV9750_12BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};

combo_dev_attr_t DVP_SENSOR_OV9732_10BIT_NOWDR_ATTR = 
{
     .input_mode = INPUT_MODE_CMOS_33V,
    {

    }
};

combo_dev_attr_t DVP_SENSOR_SC2045_10BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};

combo_dev_attr_t MIPI_OV2710_10BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};

combo_dev_attr_t MIPI_2lane_SENSOR_OV9750_12BIT_NOWDR_ATTR = 
{
    .input_mode = INPUT_MODE_MIPI,
        
    .mipi_attr =    
    {
        RAW_DATA_12BIT,
        {0, 1, -1, -1, -1, -1, -1, -1}
    }
};

combo_dev_attr_t MIPI_1lane_SENSOR_OV9732_10BIT_NOWDR_ATTR = 
{
	.input_mode = INPUT_MODE_MIPI,
	{
	   .mipi_attr = 
		{
			RAW_DATA_10BIT,
			{0, -1, -1, -1, -1, -1, -1, -1}
		}
	}
};

combo_dev_attr_t MIPI_AR0130_12BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};
combo_dev_attr_t DVP_SC2135_12BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};

combo_dev_attr_t DVP_OV9732_10BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};

combo_dev_attr_t MIPI_OV2735_10BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};
VI_DEV_ATTR_S DEV_ATTR_MN34227_MIPI_1080P =
{
	/* interface mode */
	VI_MODE_MIPI,
	/* multiplex mode */
	VI_WORK_MODE_1Multiplex,
	/* r_mask	 g_mask    b_mask*/
	{0xFFF00000,	0x0},
	/* progessive or interleaving */
	VI_SCAN_PROGRESSIVE,
	/*AdChnId*/
	{ -1, -1, -1, -1},
	/*enDataSeq, only support yuv*/
	VI_INPUT_DATA_YUYV,

	/* synchronization information */
	{
		/*port_vsync   port_vsync_neg	  port_hsync		port_hsync_neg		  */
		VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

		/*hsync_hfb    hsync_act	hsync_hhb*/
		{
			0,			  1920, 	   0,
			/*vsync0_vhb vsync0_act vsync0_hhb*/
			0,			  1080,		  0,
			/*vsync1_vhb vsync1_act vsync1_hhb*/
			0,			  0,			0
		}
	},
	/* use interior ISP */
	VI_PATH_ISP,
	/* input data type */
	VI_DATA_TYPE_RGB,
	/* bRever */
	HI_FALSE,
	/* DEV CROP */
	{6, 8, 1920, 1080}
};
combo_dev_attr_t MIPI_2lane_SENSOR_MN34227_12BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_MIPI,
    {
        .mipi_attr =    
        {
            RAW_DATA_12BIT,
            {0, 1, -1, -1, -1, -1, -1, -1}
        }
    }
};
combo_dev_attr_t DVP_SC2235_12BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_CMOS_33V,
    {
    
    }
};




HI_S32 SAMPLE_COMM_VI_SetMipiAttr()
{
	HI_S32 fd;
	combo_dev_attr_t *pstcomboDevAttr;

	/* mipi reset unrest */
	fd = open("/dev/hi_mipi", O_RDWR);
	if (fd < 0)
	{
		printf("warning: open hi_mipi dev failed\n");
		return -1;
	}

	if (sns_type == SENSOR_OV9750)
	{
		pstcomboDevAttr = &DVP_2lane_SENSOR_OV9750_12BIT_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_OV9750m)
	{
		pstcomboDevAttr = &MIPI_2lane_SENSOR_OV9750_12BIT_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_OV9732)
	{
		//pstcomboDevAttr = &MIPI_1lane_SENSOR_OV9732_10BIT_NOWDR_ATTR;
		pstcomboDevAttr = &DVP_OV9732_10BIT_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_OV2710)
	{
		pstcomboDevAttr = &MIPI_OV2710_10BIT_NOWDR_ATTR;
	}
    else if (sns_type == SENSOR_AR0130)
	{
		pstcomboDevAttr = &MIPI_AR0130_12BIT_NOWDR_ATTR;
	}
	 else if (sns_type == SENSOR_SC2045)
	{
		pstcomboDevAttr = &DVP_SENSOR_SC2045_10BIT_NOWDR_ATTR;
	}
	 else if (sns_type == SENSOR_SC2135)
	{
		pstcomboDevAttr = &DVP_SC2135_12BIT_NOWDR_ATTR;
	}
	 else if (sns_type == SENSOR_OV2735)
	{
		pstcomboDevAttr = &MIPI_OV2735_10BIT_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_SC2235)
	{
		pstcomboDevAttr = &DVP_SC2235_12BIT_NOWDR_ATTR;
	}
	else if(sns_type == SENSOR_MN34227)
	{
		pstcomboDevAttr = &MIPI_2lane_SENSOR_MN34227_12BIT_NOWDR_ATTR;

	}
	else 
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		close(fd);
		return -1;
	}

	if (ioctl(fd, HI_MIPI_SET_DEV_ATTR, pstcomboDevAttr))
	{
		printf("%s: set mipi attr failed\n", __FUNCTION__);
		close(fd);
		return -1;
	}
	close(fd);
	return HI_SUCCESS;
}

/*****************************************************************************
* function : init mipi
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_StartMIPI()
{
	int s32Ret;

	s32Ret = SAMPLE_COMM_VI_SetMipiAttr();
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("%s: mipi init failed!\n", __FUNCTION__);
		/* disable videv */
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/******************************************************************************
* funciton : ISP init
******************************************************************************/
HI_S32 SAMPLE_COMM_ISP_Init(HI_BOOL WdrEnble)
{
	ISP_DEV IspDev = 0;
	HI_S32 s32Ret;
	ISP_PUB_ATTR_S stPubAttr;
	//ISP_INPUT_TIMING_S stInputTiming;
	HI_U16 frm;
	if (ispFramerate > 0)
		frm = ispFramerate;
	else
		frm = MAX_FRAME_RATE;

#if 0
	/* 1. sensor register callback */
	s32Ret = sensor_register_callback();
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: sensor_register_callback failed with %#x!\n", __FUNCTION__, s32Ret);
		return s32Ret;
	}
#endif

	ALG_LIB_S stLib;
    
	/* 2. register hisi ae lib */
	stLib.s32Id = 0;
	strcpy(stLib.acLibName, HI_AE_LIB_NAME);
	s32Ret = HI_MPI_AE_Register(IspDev, &stLib);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_AE_Register failed!\n", __FUNCTION__);
		return s32Ret;
	}

	/* 3. register hisi awb lib */
	stLib.s32Id = 0;
	strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
	s32Ret = HI_MPI_AWB_Register(IspDev, &stLib);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_AWB_Register failed!\n", __FUNCTION__);
		return s32Ret;
	}

	/* 4. register hisi af lib */
	stLib.s32Id = 0;
	strcpy(stLib.acLibName, HI_AF_LIB_NAME);
	s32Ret = HI_MPI_AF_Register(IspDev, &stLib);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_AF_Register failed!\n", __FUNCTION__);
		return s32Ret;
	}

	/* 5. isp mem init */
	s32Ret = HI_MPI_ISP_MemInit(IspDev);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_ISP_Init failed!\n", __FUNCTION__);
		return s32Ret;
	}

	/* 6. isp set WDR mode */
	ISP_WDR_MODE_S stWdrMode;
	stWdrMode.enWDRMode  = WDR_MODE_NONE;
	s32Ret = HI_MPI_ISP_SetWDRMode(0, &stWdrMode);
	if (HI_SUCCESS != s32Ret)
	{
		printf("start ISP WDR failed!\n");
		return s32Ret;
	}

	/* 7. isp set pub attributes */
	if(sns_type == SENSOR_OV9750 || sns_type == SENSOR_OV9750m)
	{
        stPubAttr.enBayer               = BAYER_BGGR;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = 1280;
        stPubAttr.stWndRect.u32Height   = 960;
	}
	else if (sns_type == SENSOR_OV9732)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = 1280;
        stPubAttr.stWndRect.u32Height   = 720;
	}
	else if(sns_type == SENSOR_MN34227)
	{
        stPubAttr.enBayer               = BAYER_GRBG;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = 1920;
        stPubAttr.stWndRect.u32Height   = 1080;
	}
	else if(sns_type == SENSOR_OV2710)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
		stPubAttr.f32FrameRate          = frm;
		stPubAttr.stWndRect.s32X        = 0;
		stPubAttr.stWndRect.s32Y        = 0;
		stPubAttr.stWndRect.u32Width    = 1920;
		stPubAttr.stWndRect.u32Height   = 1080;
	}
    else if(sns_type == SENSOR_AR0130)
	{
        stPubAttr.enBayer               = BAYER_GRBG;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = 1280;
        stPubAttr.stWndRect.u32Height   = 960;
	}
	else if(sns_type == SENSOR_SC2045)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
		stPubAttr.f32FrameRate          = frm;
		stPubAttr.stWndRect.s32X        = 0;
		stPubAttr.stWndRect.s32Y        = 0;
		stPubAttr.stWndRect.u32Width    = 1920;
		stPubAttr.stWndRect.u32Height   = 1080;
	 }
	else if(sns_type == SENSOR_SC2135 || sns_type == SENSOR_SC2235)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
		stPubAttr.f32FrameRate          = 20;
		stPubAttr.stWndRect.s32X        = 0;
		stPubAttr.stWndRect.s32Y        = 0;
		stPubAttr.stWndRect.u32Width    = 1920;
		stPubAttr.stWndRect.u32Height   = 1080;
	 }
	else if(sns_type == SENSOR_OV2735)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
		stPubAttr.f32FrameRate          = frm;
		stPubAttr.stWndRect.s32X        = 0;
		stPubAttr.stWndRect.s32Y        = 0;
		stPubAttr.stWndRect.u32Width    = 1920;
		stPubAttr.stWndRect.u32Height   = 1080;
	}
	else
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_ISP_SetPubAttr(IspDev, &stPubAttr);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_ISP_SetPubAttr failed with %#x!\n", __FUNCTION__, s32Ret);
		return s32Ret;
	}

	jv_sensor_set_fps((int)stPubAttr.f32FrameRate);

	/* 8. isp init */
	s32Ret = HI_MPI_ISP_Init(IspDev);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_ISP_Init failed!\n", __FUNCTION__);
		return s32Ret;
	}

	return HI_SUCCESS;
}

/*****************************************************************************
* function : star vi dev (cfg vi_dev_attr; set_dev_cfg; enable dev)
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_StartDev()
{
	ISP_DEV IspDev = 0;
	HI_S32 s32Ret;
	int ViDev = 0;
	VI_DEV_ATTR_S *devAttr;
	ISP_WDR_MODE_S stWDRMode;

	if(sns_type == SENSOR_OV9750)
	{
		Printf("ov9750 running\n");
		devAttr = &DEV_ATTR_OV9750_DC_960P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV9750m)
	{
		Printf("ov9750 mipi running\n");
		devAttr = &DEV_ATTR_OV9750_MIPI_960p; 
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_MN34227)
	{
		devAttr = &DEV_ATTR_MN34227_MIPI_1080P;
		devAttr->stDevRect.s32X = 6;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV9732)
	{
		Printf("ov9732 running\n");
		//devAttr = &DEV_ATTR_OV9732_MIPI_720P;
		devAttr = &DEV_ATTR_OV9732_DC_720P;
		
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV2710)
	{
		Printf("ov2710 running\n");
		devAttr = &DEV_ATTR_OV2710_DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
    else if(sns_type == SENSOR_AR0130)
	{
		Printf("ar0130 running\n");
		devAttr = &DEV_ATTR_AR0130_DC_960P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
    
	else if(sns_type == SENSOR_SC2045)
	{
		Printf("BG0803 running\n");
		devAttr = &DEV_ATTR_SC2045_DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_SC2135 || sns_type == SENSOR_SC2235)
	{
		Printf("SC2235 running\n");
		devAttr = &DEV_ATTR_SC2135_DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV2735)
	{
		Printf("ov2735 running\n");
		devAttr = &DEV_ATTR_OV2735_DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		return HI_FAILURE;
	}

	if (VI_CROP_ENABLE)
	{
		devAttr->stDevRect.u32Width  = VI_CROP_W + VI_CROP_X * 2;
		devAttr->stDevRect.u32Height = VI_CROP_H + VI_CROP_Y * 2;		
	}

	s32Ret = HI_MPI_VI_SetDevAttr(ViDev, devAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_ISP_GetWDRMode(IspDev, &stWDRMode);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetWDRMode failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	if (stWDRMode.enWDRMode)  //wdr mode
	{
		VI_WDR_ATTR_S stWdrAttr;
		stWdrAttr.enWDRMode = stWDRMode.enWDRMode;
		stWdrAttr.bCompress = HI_TRUE;
		s32Ret = HI_MPI_VI_SetWDRAttr(ViDev, &stWdrAttr);
		if (s32Ret)
		{
			SAMPLE_PRT("HI_MPI_VI_SetWDRAttr failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}

	s32Ret = HI_MPI_VI_EnableDev(ViDev);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_EnableDev failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/******************************************************************************
* funciton : ISP Run
******************************************************************************/


HI_VOID* Test_ISP_Run(HI_VOID *param)
{
	ISP_DEV IspDev = 0;

	pthreadinfo_add((char *)__func__);

	HI_MPI_ISP_Run(IspDev);

	return HI_NULL;
}

HI_S32 SAMPLE_COMM_ISP_Run(HI_BOOL WdrEnble)
{
	static BOOL bInited = 0;
	HI_S32 s32Ret;

	//if (bInited)
		//return 0;
	//bInited = 1;
	s32Ret = SAMPLE_COMM_ISP_Init(WdrEnble);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s: ISP init failed!\n", __FUNCTION__);
		return HI_FAILURE;
	}

#if 1
	
	if (0 != pthread_create(&gs_IspPid, 0, (void* (*)(void*))Test_ISP_Run, NULL))
	{
		printf("%s: create isp running thread failed!\n", __FUNCTION__);
		return HI_FAILURE;
	}
#else
	/* configure thread priority */
	if (1)
	{
#include <sched.h>

		pthread_attr_t attr;
		struct sched_param param;
		int newprio = 50;

		pthread_attr_init(&attr);

		if (1)
		{
			int policy = 0;
			int min, max;

			pthread_attr_getschedpolicy(&attr, &policy);
			printf("-->default thread use policy is %d --<\n", policy);

			pthread_attr_setschedpolicy(&attr, SCHED_RR);
			pthread_attr_getschedpolicy(&attr, &policy);
			printf("-->current thread use policy is %d --<\n", policy);

			switch (policy)
			{
			case SCHED_FIFO:
				printf("-->current thread use policy is SCHED_FIFO --<\n");
				break;

			case SCHED_RR:
				printf("-->current thread use policy is SCHED_RR --<\n");
				break;

			case SCHED_OTHER:
				printf("-->current thread use policy is SCHED_OTHER --<\n");
				break;

			default:
				printf("-->current thread use policy is UNKNOW --<\n");
				break;
			}

			min = sched_get_priority_min(policy);
			max = sched_get_priority_max(policy);

			printf("-->current thread policy priority range (%d ~ %d) --<\n", min, max);
		}

		pthread_attr_getschedparam(&attr, &param);

		printf("-->default isp thread priority is %d , next be %d --<\n", param.sched_priority, newprio);
		param.sched_priority = newprio;
		pthread_attr_setschedparam(&attr, &param);

		if (0 != pthread_create(&gs_IspPid, &attr, (void* (*)(void*))HI_MPI_ISP_Run, NULL))
		{
			printf("%s: create isp running thread failed!\n", __FUNCTION__);
			return HI_FAILURE;
		}

		pthread_attr_destroy(&attr);
	}
#endif

	/* load configure file if there is */
#ifdef SAMPLE_LOAD_ISPREGFILE
	s32Ret = SAMPLE_COMM_ISP_LoadRegFile(CFG_OPT_LOAD, SAMPLE_ISPCFG_FILE);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s: load isp cfg file failed![%s]\n", __FUNCTION__, SAMPLE_ISPCFG_FILE);
		return HI_FAILURE;
	}
#endif

	return HI_SUCCESS;
}
/*****************************************************************************
* function : stop isp thread
*****************************************************************************/
HI_VOID SAMPLE_COMM_ISP_Stop()

{
	ISP_DEV IspDev = 0;

    HI_MPI_ISP_Exit(IspDev);

    pthread_join(gs_IspPid, 0);

    return;

}
/*****************************************************************************
* function : stop VI and stop ISP
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_StopIsp(void)
{
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_S32 i;
    HI_S32 s32Ret;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;

    
    /*** Stop VI Chn ***/
    for(i=0;i < u32ChnNum; i++)
    {
        /* Stop vi phy-chn */
        ViChn = i;
        s32Ret = HI_MPI_VI_DisableChn(ViChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_DisableChn failed with %#x\n",s32Ret);
            return HI_FAILURE;
        }
    }

    /*** Stop VI Dev ***/
    for(i=0; i < u32DevNum; i++)
    {
        ViDev = i;
        s32Ret = HI_MPI_VI_DisableDev(ViDev);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_DisableDev failed with %#x\n", s32Ret);
            return HI_FAILURE;
        }
    }

    SAMPLE_COMM_ISP_Stop();
    return HI_SUCCESS;
}
/*****************************************************************************
* function : star vi chn
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_StartChn(VI_CHN ViChn, RECT_S *pstCapRect, SIZE_S *pstTarSize, BOOL bMirror, BOOL bFlip)
{
	HI_S32 s32Ret;
	VI_CHN_ATTR_S stChnAttr;

	/* step  5: config & start vicap dev */
	memcpy(&stChnAttr.stCapRect, pstCapRect, sizeof(RECT_S));
	stChnAttr.enCapSel = VI_CAPSEL_BOTH;
	/* to show scale. this is a sample only, we want to show dist_size = D1 only */
	stChnAttr.stDestSize.u32Width = pstTarSize->u32Width;
	stChnAttr.stDestSize.u32Height = pstTarSize->u32Height;
	stChnAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;   /* sp420 or sp422 */

	stChnAttr.bMirror = bMirror;
	stChnAttr.bFlip = bFlip;

	stChnAttr.enCompressMode = COMPRESS_MODE_NONE;
	stChnAttr.s32SrcFrameRate = -1;
	stChnAttr.s32DstFrameRate = -1;

	s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VI_EnableChn(ViChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}
void VI_Mirror_Flip(BOOL bMirror, BOOL bFlip)
{
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN_ATTR_S stVpssChnAttr;
	int i;
	VPSS_CHN VpssChn[3] = {0,1,2};
	if(hwinfo.encryptCode == ENCRYPT_200W)
	{
		VpssChn[0] = 1;
		VpssChn[1] = 2;
		VpssChn[2] = 3;
	}

	for(i=0; i<ARRAY_SIZE(VpssChn); i++)
	{
		HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn[i], &stVpssChnAttr);
		stVpssChnAttr.bFlip = bFlip;
		stVpssChnAttr.bMirror = bMirror;
		HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn[i], &stVpssChnAttr);
	}
}

HI_S32 SAMPLE_COMM_VPSS_StartGroup(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstVpssGrpAttr)
{
    HI_S32 s32Ret;
    VPSS_GRP_PARAM_S stVpssParam;
    
    if (VpssGrp < 0 || VpssGrp > VPSS_MAX_GRP_NUM)
    {
        printf("VpssGrp%d is out of rang. \n", VpssGrp);
        return HI_FAILURE;
    }

    if (HI_NULL == pstVpssGrpAttr)
    {
        printf("null ptr,line%d. \n", __LINE__);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

#if 0
    /*** set vpss param ***/
    s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp, &stVpssParam);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    
    //stVpssParam.u32MotionThresh = 0;
    
    s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp, &stVpssParam);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
#endif

    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


HI_S32 SAMPLE_COMM_VPSS_StopGroup(VPSS_GRP VpssGrp)
{
    HI_S32 s32Ret;

    if (VpssGrp < 0 || VpssGrp > VPSS_MAX_GRP_NUM)
    {
        printf("VpssGrp%d is out of rang[0,%d]. \n", VpssGrp, VPSS_MAX_GRP_NUM);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/*****************************************************************************
* function : Vi chn bind vpss group
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_BindVpss()
{
    HI_S32 j, s32Ret;
    VPSS_GRP VpssGrp;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    VI_CHN ViChn;

    
    VpssGrp = 0;
    for (j=0; j<1; j++)
    {
        ViChn = j * 1;
        
        stSrcChn.enModId = HI_ID_VIU;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = ViChn;
    
        stDestChn.enModId = HI_ID_VPSS;
        stDestChn.s32DevId = VpssGrp;
        stDestChn.s32ChnId = 0;
    
        s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
        
        VpssGrp ++;
    }
    return HI_SUCCESS;
}
                                   

HI_S32 SAMPLE_COMM_VPSS_EnableChn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, 
                                                  VPSS_CHN_ATTR_S *pstVpssChnAttr,
                                                  VPSS_CHN_MODE_S *pstVpssChnMode,
                                                  VPSS_EXT_CHN_ATTR_S *pstVpssExtChnAttr)
{
    HI_S32 s32Ret;

    if (VpssGrp < 0 || VpssGrp > VPSS_MAX_GRP_NUM)
    {
        printf("VpssGrp%d is out of rang[0,%d]. \n", VpssGrp, VPSS_MAX_GRP_NUM);
        return HI_FAILURE;
    }

    if (VpssChn < 0 || VpssChn > VPSS_MAX_CHN_NUM)
    {
        printf("VpssChn%d is out of rang[0,%d]. \n", VpssChn, VPSS_MAX_CHN_NUM);
        return HI_FAILURE;
    }

    if (HI_NULL == pstVpssChnAttr && HI_NULL == pstVpssExtChnAttr)
    {
        printf("null ptr,line%d. \n", __LINE__);
        return HI_FAILURE;
    }

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, pstVpssChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
            return HI_FAILURE;
        }
    }
    else
    {
        s32Ret = HI_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn, pstVpssExtChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
            return HI_FAILURE;
        }
    }
    
    if (VpssChn < VPSS_MAX_PHY_CHN_NUM && HI_NULL != pstVpssChnMode)
    {
        s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn, pstVpssChnMode);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
            return HI_FAILURE;
        }     
    }

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM && VpssChn != 3)	//3是移动侦测通道，不再旋转
    {
        if(hwinfo.rotateBSupport && JVSENSOR_ROTATE_NONE != stJvParam.rotate)
        {
            s32Ret = HI_MPI_VPSS_SetRotate(VpssGrp, VpssChn, stJvParam.rotate);
            if(s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VPSS_SetRotate failed with %#x!\n", s32Ret);
                return s32Ret;
            }
        }
    }
/*	//深度的设置在其它地方
	s32Ret = HI_MPI_VPSS_SetDepth(VpssGrp, VpssChn, 1);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetDepth  failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }*/

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/*
 * 这里需要脚本配合，在/etc/init.d/rcS中
#default value of SENSOR
SENSOR=ar0130

#set the value of SENSOR
if [ -f /etc/conf.d/jovision/sensor.sh ] ; then
	. /etc/conf.d/jovision/sensor.sh
fi
sh ./load3518 -i ${SENSOR}

 */

/*

static void __check_sensor(void)
{
	int sensor;
	char *cmd;
	char *str;
	char *fname = CONFIG_PATH"/sensor.sh";
	isp_ioctl(0,GET_ID,(unsigned long)&sensor);
	printf("====>>. sensor: %d\n", sensor);
	if (access(fname, F_OK) == 0)
	{
		str = utl_fcfg_get_value(fname, "SENSOR");
		if (str != NULL)
		{
			
			
		    if (strcmp(str, "ov9750") == 0)
			{
				sns_type = SENSOR_OV9750;
				hwinfo.encryptCode = ENCRYPT_130W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 960;
			}
			else if (strcmp(str, "ov9750m") == 0)
			{
				sns_type = SENSOR_OV9750m;
				hwinfo.encryptCode = ENCRYPT_130W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 960;
			}
			else if (strcmp(str, "ov9732") == 0)
			{
				sns_type = SENSOR_OV9732;
				hwinfo.encryptCode = ENCRYPT_100W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 720;
			}
			else if (strcmp(str, "ov2710") == 0)
			{
				sns_type = SENSOR_OV2710;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;

				VI_CROP_ENABLE = FALSE;
				VI_CROP_W = 1920 -80 -80;
				VI_CROP_H = 1080;
				VI_CROP_X = 80;
				VI_CROP_Y =0;
				
			}
            else if (strcmp(str, "po1210") == 0)
			{
				sns_type = SENSOR_PO1210;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
            else if (strcmp(str, "gc2003") == 0)
			{
				sns_type = SENSOR_GC2003;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
            else if (strcmp(str, "ar0130") == 0)
			{
				sns_type = SENSOR_AR0130;
				hwinfo.encryptCode = ENCRYPT_130W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 960;
			}
            else if (strcmp(str, "ar0237") == 0)
			{
				sns_type = SENSOR_AR0237;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
                MAX_FRAME_RATE = 25;
			}
            else if (strcmp(str, "bg0803") == 0)
			{
				sns_type = SENSOR_BG0803;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
			else if (strcmp(str, "imx323") == 0)
			{
				sns_type = SENSOR_IMX323;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
			else if (strcmp(str, "sc2045") == 0)
			{
				sns_type = SENSOR_SC2045;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
			else if (strcmp(str, "sc2135") == 0)
			{
				sns_type = SENSOR_SC2135;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
			else if (strcmp(str, "ov2735") == 0)
			{
				sns_type = SENSOR_OV2735;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
			}
			else
			{
				Printf("Failed Find sensor\n");
				sns_type = (unsigned int) -1;
				unlink(fname);
			}
			//hwinfo.type 统一格式，统一处理：芯片名称-主板类型-分辨率：H8E-P-10：HI3518E-38板-100W
			{
				memset(hwinfo.type, 0, sizeof(hwinfo.type));
				strncpy(hwinfo.type, "H8EV200-", strlen("H8EV200-"));
				strcat(hwinfo.type, "S-");
				switch (hwinfo.encryptCode)
				{
				case ENCRYPT_100W:
					strcat(hwinfo.type, "10");
					break;
				case ENCRYPT_130W:
					strcat(hwinfo.type, "13");
					break;
				case ENCRYPT_200W:
					strcat(hwinfo.type, "20");
					break;
				case ENCRYPT_300W:
					strcat(hwinfo.type, "30");
					break;
				case ENCRYPT_500W:
					strcat(hwinfo.type, "50");
					break;
				}
			}

			if (strstr(hwinfo.product, "GC"))			//工程机
			{
				strcat(hwinfo.type, "GC");
			}
			if (sns_type == sensor)
			{
				utl_fcfg_close(fname);
				return;
			}
		}
	}

	{
		
		if (sensor == SENSOR_OV9750)
		{
			sns_type = SENSOR_OV9750;
			cmd = "SENSOR=ov9750";
			hwinfo.encryptCode = ENCRYPT_130W;
		}
		else if (sensor == SENSOR_OV9750m)
		{
			sns_type = SENSOR_OV9750m;
			cmd = "SENSOR=ov9750m";
			hwinfo.encryptCode = ENCRYPT_130W;
		}
		else if (sensor == SENSOR_OV2710)
		{
			sns_type = SENSOR_OV2710;
			cmd = "SENSOR=ov2710";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
        else if (sensor == SENSOR_PO1210)
		{
			sns_type = SENSOR_PO1210;
			cmd = "SENSOR=po1210";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
        else if (sensor == SENSOR_GC2003)
		{
			sns_type = SENSOR_GC2003;
			cmd = "SENSOR=gc2003";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
        else if (sensor == SENSOR_AR0130)
		{
			sns_type = SENSOR_AR0130;
			cmd = "SENSOR=ar0130";
			hwinfo.encryptCode = ENCRYPT_130W;
		}
        else if (sensor == SENSOR_AR0237)
		{
			sns_type = SENSOR_AR0237;
			cmd = "SENSOR=ar0237";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
        else if (sensor == SENSOR_BG0803)
		{
			sns_type = SENSOR_BG0803;
			cmd = "SENSOR=bg0803";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_IMX323)
		{
			sns_type = SENSOR_IMX323;
			cmd = "SENSOR=imx323";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_SC2045)
		{
			sns_type = SENSOR_SC2045;
			cmd = "SENSOR=sc2045";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_SC2135)
		{
			sns_type = SENSOR_SC2135;
			cmd = "SENSOR=sc2135";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_OV9732)
		{
			sns_type = SENSOR_OV9732;
			cmd = "SENSOR=ov9732";
			hwinfo.encryptCode = ENCRYPT_100W;
		}
		else if (sensor == SENSOR_OV2735)
		{
			sns_type = SENSOR_OV2735;
			cmd = "SENSOR=ov2735";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		FILE *fp;
		fp = fopen(fname, "wb");
		fputs(cmd, fp);
		fclose(fp);
		utl_system("reboot");
	}
}
*/
	
static void __check_sensor(void)
{
	int sensor;
	char *cmd;
	char *str;
	char *fname = CONFIG_PATH"/sensor.sh";
	//isp_ioctl(0,GET_ID,(unsigned long)&sensor);
	//printf("====>>. sensor: %d\n", sensor);
	if (access(fname, F_OK) == 0)
	{
		str = utl_fcfg_get_value(fname, "SENSOR");
		if (str != NULL)
		{
			
			
			if (strcmp(str, "ov9750") == 0)
			{
				sns_type = SENSOR_OV9750;
				hwinfo.encryptCode = ENCRYPT_130W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 960;
				jv_sensor_set_max_vi_20fps(FALSE);
			}
			else if (strcmp(str, "ov9750m") == 0)
			{
				sns_type = SENSOR_OV9750m;
				hwinfo.encryptCode = ENCRYPT_130W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 960;
				jv_sensor_set_max_vi_20fps(FALSE);
			}
			else if (strcmp(str, "mn34227") == 0)
			{
				sns_type = SENSOR_MN34227;
				//strcpy(hwinfo.type,"N91-HC-W");
				//hwinfo.bImageAD = TRUE;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
                VI_CROP_ENABLE = FALSE;
				VI_CROP_W =  1920-64-64;//1920-64-64;
				VI_CROP_H =   1080;
				VI_CROP_X  =64;
				VI_CROP_Y  =0;
				jv_sensor_set_max_vi_20fps(TRUE);
			}
			else if (strcmp(str, "ov9732") == 0)
			{
				sns_type = SENSOR_OV9732;
				hwinfo.encryptCode = ENCRYPT_100W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 720;
				jv_sensor_set_max_vi_20fps(FALSE);

				if (PRODUCT_MATCH(PRODUCT_C3C)
					|| PRODUCT_MATCH(PRODUCT_C5_R1))
				{
					hwinfo.encryptCode = ENCRYPT_130W;
					VI_WIDTH = 1440;
					VI_HEIGHT = 900;
					VI_CROP_ENABLE = TRUE;
					VI_CROP_X = 32;
					VI_CROP_Y = 0;
					VI_CROP_W = 1216;
					VI_CROP_H = 720;
				}
			}
			else if (strcmp(str, "ov2710") == 0)
			{
				sns_type = SENSOR_OV2710;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;

				VI_CROP_ENABLE = FALSE;
				VI_CROP_W = 1920 -80 -80;
				VI_CROP_H = 1080;
				VI_CROP_X = 80;
				VI_CROP_Y =0;
				jv_sensor_set_max_vi_20fps(TRUE);
				
			}
			else if (strcmp(str, "ar0130") == 0)
			{
				sns_type = SENSOR_AR0130;
				hwinfo.encryptCode = ENCRYPT_130W;
				VI_WIDTH = 1280;
				VI_HEIGHT = 960;
				jv_sensor_set_max_vi_20fps(FALSE);
			}
			else if (strcmp(str, "sc2045") == 0)
			{
				sns_type = SENSOR_SC2045;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
				jv_sensor_set_max_vi_20fps(TRUE);
			}
			else if (strcmp(str, "sc2135") == 0)
			{
				sns_type = SENSOR_SC2135;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
				jv_sensor_set_max_vi_20fps(TRUE);
			}
			else if (strcmp(str, "ov2735") == 0)
			{
				sns_type = SENSOR_OV2735;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 25;
				jv_sensor_set_max_vi_20fps(TRUE);
			}
			else if (strcmp(str, "sc2235") == 0)
			{
				sns_type = SENSOR_SC2235;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				MAX_FRAME_RATE = 20;

				VI_CROP_ENABLE = FALSE;
				VI_CROP_W = 1920;
				VI_CROP_H = 1080;
				VI_CROP_X = 0;
				VI_CROP_Y =0;
			}
			else
			{
				Printf("Failed Find sensor\n");
				sensor = sns_type = SENSOR_UNKNOWN;
				unlink(fname);
			}
			if( sns_type > SENSOR_UNKNOWN && sns_type < SENSOR_MAX)
			{
				sensor = sns_type; 
				isp_ioctl(0,GET_ID,(unsigned long)&sensor);
				if(sensor == sns_type)
				{
					utl_fcfg_close(fname);
					
					//hwinfo.type 统一格式，统一处理：芯片名称-主板类型-分辨率：H8E-P-10：HI3518E-38板-100W
					{
						memset(hwinfo.type, 0, sizeof(hwinfo.type));
						strncpy(hwinfo.type, "H8EV200-", strlen("H8EV200-"));
						strcat(hwinfo.type, "S-");
						switch (hwinfo.encryptCode)
						{
						case ENCRYPT_100W:
							strcat(hwinfo.type, "10");
							break;
						case ENCRYPT_130W:
							strcat(hwinfo.type, "13");
							break;
						case ENCRYPT_200W:
							strcat(hwinfo.type, "20");
							break;
						case ENCRYPT_300W:
							strcat(hwinfo.type, "30");
							break;
						case ENCRYPT_500W:
							strcat(hwinfo.type, "50");
							break;
						}
					}

					if (strstr(hwinfo.product, "GC"))			//工程机
					{
						strcat(hwinfo.type, "GC");
					}
					return;
				}
				else
				{
					printf("%s: sensor info is error\n",fname);
					sns_type = sensor;
					unlink(fname);
				}
			}
		}
		else
		{
			printf("check sensor file[%s] is NULL!\n", fname);
			sensor = sns_type = SENSOR_UNKNOWN;
		}
	}
	else
	{
		printf("check sensor file[%s] is NULL!\n", fname);
		sensor = sns_type = SENSOR_UNKNOWN;
	}
	if(SENSOR_UNKNOWN == sns_type)
	{
		isp_ioctl(0,GET_ID,(unsigned long)&sensor);
		printf("====>>. sensor: %d\n", sensor);
	}
	
	{
		
		if (sensor == SENSOR_OV9750)
		{
			sns_type = SENSOR_OV9750;
			cmd = "SENSOR=ov9750";
			hwinfo.encryptCode = ENCRYPT_130W;
		}
		else if (sensor == SENSOR_OV9750m)
		{
			sns_type = SENSOR_OV9750m;
			cmd = "SENSOR=ov9750m";
			hwinfo.encryptCode = ENCRYPT_130W;
		}
		else if (sensor == SENSOR_OV2710)
		{
			sns_type = SENSOR_OV2710;
			cmd = "SENSOR=ov2710";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_AR0130)
		{
			sns_type = SENSOR_AR0130;
			cmd = "SENSOR=ar0130";
			hwinfo.encryptCode = ENCRYPT_130W;
		}
		else if (sensor == SENSOR_MN34227)
		{
			sns_type = SENSOR_MN34227;
			cmd = "SENSOR=mn34227";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_AR0237)
		{
			sns_type = SENSOR_AR0237;
			cmd = "SENSOR=ar0237";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_SC2045)
		{
			sns_type = SENSOR_SC2045;
			cmd = "SENSOR=sc2045";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_SC2135)
		{
			sns_type = SENSOR_SC2135;
			cmd = "SENSOR=sc2135";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_OV9732)
		{
			sns_type = SENSOR_OV9732;
			cmd = "SENSOR=ov9732";
			hwinfo.encryptCode = ENCRYPT_100W;
		}
		else if (sensor == SENSOR_OV2735)
		{
			sns_type = SENSOR_OV2735;
			cmd = "SENSOR=ov2735";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_SC2235)
		{
			sns_type = SENSOR_SC2235;
			cmd = "SENSOR=sc2235";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else
		{
			printf("can not find right sensor !!! \n");
			cmd = NULL;
		}
		
		if(cmd)
		{
			FILE *fp;
			fp = fopen(fname, "wb");
			fputs(cmd, fp);
			fclose(fp);
		}
		utl_system("reboot");
	}
}

unsigned int ipcinfo_get_model_code();

typedef struct{
	void (*sensor_init)(void);
	void (*cmos_set_wdr_mode)(int mode);
	int (*sensor_register_callback)(void);
    void (*cmos_fps_set_temporary)(int fps);
}_SensorPtr_t;

#include <dlfcn.h>

int haveto = 1;
static int bGet=0;
static int _get_sensor_ptr(const char *fname, _SensorPtr_t *sensor)
{
	{//have to
		if (haveto == 0) //其实是不会成立的
		{
			HI_MPI_AE_SensorRegCallBack(0, NULL, 0, NULL);
		}
	}
	if(bGet)
		return 0;
	void *handle = dlopen(fname, RTLD_LAZY);
	if (!handle)
	{
		printf("ERROR: Failed dlopen: %s, because: %s\n", fname, dlerror());
		return -2;
	}
	sensor->sensor_init = dlsym(handle, "sensor_init");
	sensor->cmos_set_wdr_mode = dlsym(handle, "cmos_set_wdr_mode");
	sensor->sensor_register_callback = dlsym(handle, "sensor_register_callback");
    sensor->cmos_fps_set_temporary = dlsym(handle,"cmos_fps_set_temporary");
	bGet =1;
	if (sensor->sensor_init && sensor->cmos_set_wdr_mode && sensor->sensor_register_callback && sensor->cmos_fps_set_temporary)
		return 0;
	printf("one or more func is null:\n %p, %p, %p\n", sensor->sensor_init, sensor->cmos_set_wdr_mode, sensor->sensor_register_callback);
	return -1;
}

#define SENSOR_SO_BASE_DIR "/home/sensor"

//////////////////////////////////视频输入//////////////////////////////////
static _SensorPtr_t sensor;
BOOL __check_2735B();

static VOID InitVI(int mode)
{
	S32 s32Ret;
	if(sns_type == SENSOR_OV9750)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov9750.so", &sensor);
	}
	else if(sns_type == SENSOR_OV9750m)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov9750m.so", &sensor);
	}
	else if(sns_type == SENSOR_OV2710)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2710.so", &sensor);
	}
    else if(sns_type == SENSOR_AR0130)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ar0130.so", &sensor);
	}
	 else if(sns_type == SENSOR_SC2045)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_sc2045.so", &sensor);
	}
    else if(sns_type == SENSOR_SC2135)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_sc2135.so", &sensor);
	}
	else if(sns_type == SENSOR_MN34227)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_mn34227.so", &sensor);
	}
    else if(sns_type == SENSOR_OV9732)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov9732.so", &sensor);
	}
	else if(sns_type == SENSOR_SC2235)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_sc2235.so", &sensor);
	}
	else if(sns_type == SENSOR_OV2735)
	{
		if (HWTYPE_MATCH(HW_TYPE_HA230))
		{
			if(__check_2735B())
				s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2735b_230.so", &sensor);
			else
				s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2735_230.so", &sensor);
		}
		else
		{
			if(__check_2735B())
				s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2735b.so", &sensor);
			else
				s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2735.so", &sensor);
		}
	}
	else
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		return;
	}
	s32Ret = SAMPLE_COMM_VI_StartMIPI();
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: SAMPLE_COMM_VI_StartMIPI failed with %#x!\n", __FUNCTION__, s32Ret);
		return ;
	}

	s32Ret = sensor.sensor_register_callback();
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: sensor_register_callback failed with %#x!\n", \
			   __FUNCTION__, s32Ret);
		return ;
	}
	/******************************************
	 step 3: configure & run isp thread
	 note: you can jump over this step, if you do not use Hi3518 interal isp.
	 note: isp run must at this step -- after vi dev enable, before vi chn enable
	******************************************/
	s32Ret = SAMPLE_COMM_ISP_Run(mode);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("%s: ISP init failed!\n", __FUNCTION__);
		/* disable videv */
		return ;
	}
	SAMPLE_COMM_VI_StartDev();

	/******************************************************
	* Step 4: config & start vicap chn (max 1)
	******************************************************/
	{
		int ViChn = 0;
		RECT_S stCapRect;
		SIZE_S stTargetSize;

		if(VI_CROP_ENABLE)
		{
			stCapRect.s32X = VI_CROP_X;
			stCapRect.s32Y = VI_CROP_Y;
			stCapRect.u32Width = VI_CROP_W;
			stCapRect.u32Height = VI_CROP_H;
		}
		else
		{
			stCapRect.s32X = 0;
			stCapRect.s32Y = 0;
			stCapRect.u32Width = VI_WIDTH;
			stCapRect.u32Height = VI_HEIGHT;
		}

		stTargetSize.u32Width = stCapRect.u32Width;
		stTargetSize.u32Height = stCapRect.u32Height;

		s32Ret = SAMPLE_COMM_VI_StartChn(ViChn, &stCapRect, &stTargetSize, FALSE, FALSE);
		if (HI_SUCCESS != s32Ret)
		{
			//SAMPLE_COMM_ISP_Stop();
			return ;
		}
	}
	ISP_WDR_MODE_S pstWDRMode;
    s32Ret = HI_MPI_ISP_GetWDRMode(0, &pstWDRMode);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_ISP_GetWDRMode failed with %#x!\n", s32Ret);
		return ;
	}
	jv_sensor_wdr_mode_set(pstWDRMode.enWDRMode);
	
}
//停止视频输入
static VOID ReleaseVI()
{
}
#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))
typedef enum sample_vo_mode_e
{
    VO_MODE_1MUX = 0,
    VO_MODE_2MUX = 1,
    VO_MODE_BUTT
}SAMPLE_VO_MODE_E;

static void InitVpss()
{
	int s32Ret;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    VPSS_EXT_CHN_ATTR_S stVpssExtChnAttr;

    int resList[][2] = {
    		{VI_WIDTH, VI_HEIGHT},
    		{512,      288     },
    		{352,      288     },
    		{400,      224     },
    		{720,      576     },
    };

	if(VI_CROP_ENABLE)
	{
		resList[0][0]= VI_CROP_W;
		resList[0][1]= VI_CROP_H;
    };

    int wi = 0;
    int hi = 1;

    VpssGrp = 0;
	stVpssGrpAttr.u32MaxW = resList[0][wi];
	stVpssGrpAttr.u32MaxH = resList[0][hi];
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        return ;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        return ;
    }
    //第一码流
	if(hwinfo.encryptCode ==  ENCRYPT_200W)
   		VpssChn = 1;
	else
		VpssChn = 0;
	
    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
    stVpssChnMode.u32Width       = resList[0][wi];
    stVpssChnMode.u32Height      = resList[0][hi];
    stVpssChnMode.bDouble        = HI_FALSE;
    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.bBorderEn = HI_FALSE;
    stVpssChnAttr.bSpEn = HI_FALSE;
    stVpssChnAttr.bMirror = HI_FALSE;
    stVpssChnAttr.bFlip = HI_FALSE;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
        return ;
    }
#if 0
	if(hwinfo.encryptCode== ENCRYPT_200W)
	{
		// 开启VPSS 通道0的低延时
		VPSS_LOW_DELAY_INFO_S lowDelayInfo;
		HI_MPI_VPSS_GetLowDelayAttr(VpssGrp, VpssChn, &lowDelayInfo);
		lowDelayInfo.bEnable = HI_TRUE;
		lowDelayInfo.u32LineCnt = 256;
		s32Ret = HI_MPI_VPSS_SetLowDelayAttr(VpssGrp, VpssChn, &lowDelayInfo);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("HI_MPI_VPSS_SetLowDelayAttr  failed with 0x%x!\n", s32Ret);
			return;
		}
	}
#endif
    //第二码流
	if(hwinfo.encryptCode ==  ENCRYPT_200W)
		VpssChn = 2;
	else
		VpssChn = 1;
	
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.u32Width      = resList[1][wi];
    stVpssChnMode.u32Height     = resList[1][hi];
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.bBorderEn = HI_FALSE;
    stVpssChnAttr.bSpEn = HI_FALSE;
    stVpssChnAttr.bMirror = HI_FALSE;
    stVpssChnAttr.bFlip = HI_FALSE;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
        return ;
    }
    
    if (hwinfo.streamCnt >= 3 && hwinfo.encryptCode !=  ENCRYPT_200W)
    {
        //第三码流，手机码流
        VpssChn = 2;
        stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
        stVpssChnMode.u32Width      = resList[2][wi];
        stVpssChnMode.u32Height     = resList[2][hi];
        stVpssChnMode.bDouble       = HI_FALSE;
        stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
        stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
        memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
        stVpssChnAttr.bBorderEn = HI_FALSE;
        stVpssChnAttr.bSpEn = HI_FALSE;
        stVpssChnAttr.bMirror = HI_FALSE;
        stVpssChnAttr.bFlip = HI_FALSE;
        stVpssChnAttr.s32SrcFrameRate = -1;
        stVpssChnAttr.s32DstFrameRate = -1;
        s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
            return ;
        }
    }

    //IVE_MD
    VpssChn = 3;
	stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
	stVpssChnMode.u32Width = 160;
	stVpssChnMode.u32Height = 120;
	stVpssChnMode.bDouble = HI_FALSE;
	stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
	memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	stVpssChnAttr.bBorderEn = HI_FALSE;
	stVpssChnAttr.bSpEn = HI_FALSE;
	stVpssChnAttr.bMirror = HI_FALSE;
	stVpssChnAttr.bFlip = HI_FALSE;
	stVpssChnAttr.s32SrcFrameRate = -1;
	stVpssChnAttr.s32DstFrameRate = -1;
	s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr,
			&stVpssChnMode, HI_NULL);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
		return;
	}

	//抓图通道
	VpssChn = 4;
	if(hwinfo.encryptCode ==  ENCRYPT_200W)
		stVpssExtChnAttr.s32BindChn = 2;
	else
		stVpssExtChnAttr.s32BindChn = 0;
	stVpssExtChnAttr.s32SrcFrameRate = MAX_FRAME_RATE;
	stVpssExtChnAttr.s32DstFrameRate = 5;
	stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	stVpssExtChnAttr.enCompressMode  = COMPRESS_MODE_NONE;
	stVpssExtChnAttr.u32Width        = 400;//resList[3][wi];;
	stVpssExtChnAttr.u32Height       = 224;//resList[3][hi];;
	s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, HI_NULL, &stVpssExtChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
		return ;
	}

	if(VI_CROP_ENABLE)//一码流放大可以走这里
	{
		VpssChn = 6;
		stVpssExtChnAttr.s32BindChn = 0;
		stVpssExtChnAttr.s32SrcFrameRate = MAX_FRAME_RATE;
		stVpssExtChnAttr.s32DstFrameRate = MAX_FRAME_RATE;
		stVpssExtChnAttr.enPixelFormat = SAMPLE_PIXEL_FORMAT;
		stVpssExtChnAttr.enCompressMode  = COMPRESS_MODE_NONE;
		stVpssExtChnAttr.u32Width = VI_WIDTH;
		stVpssExtChnAttr.u32Height = VI_HEIGHT;
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, HI_NULL, &stVpssExtChnAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			return;
		}
	}

    printf("====================Start Vpss Success: %d\n",VpssGrp);
}

unsigned char HW_ReadEncrypt(unsigned char addr)
{
	static int encfd = -1;
	if (encfd < 0)
	{
		encfd = open("/dev/jipc", O_RDWR);
		if (encfd < 0)
		{
			Printf("Failed open file because: %s\n", strerror(errno));
			return 0;
		}
	}

	int a;
	a = addr;
	int ret;
	ret = ioctl(encfd,0x80040005,&a);
//	printf("addr: %d, info: %d, ret: %d\n", addr, a, ret);
	return a;
}

/*
 *@brief DEBUG用的yst库
 */
unsigned int jv_yst(unsigned int *a,unsigned int b)
{
//	int i;
//	unsigned int data[] = {0xbf57c2e6 ,0x490a7425 ,0x6fdcd49b ,0x2e1e71cc ,0x14c55 ,0x132df00 ,0x41 ,0xd800};
//	for (i=0;i<8;i++)
//	{
//		a[i]=data[i];
//	}
//	return 1;
	return 0;
}

static void __exit_platform()
{
	// SAMPLE_COMM_VI_UnBindVpss();
	sensor_ircut_deinit();
	SAMPLE_COMM_VI_StopIsp();
	
	SAMPLE_COMM_VPSS_StopGroup(0);

	jv_ai_stop(0);
	//if (SIGINT == signo || SIGTSTP == signo)
	{
		HI_MPI_SYS_Exit();
		HI_MPI_VB_Exit();
	}
}

/******************************************************************************
* 这里可以保证，程序退出后再运行时，能够正常运行
******************************************************************************/
void SAMPLE_VIO_HandleSig(HI_S32 signo)
{
	printf("catch signal %d\n", signo);
	//if (SIGINT == signo || SIGTSTP == signo)
	__exit_platform();
	
	printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
	exit(-1);
}

void WDR_Switch(int mode) //1//  switch to wdr ,0-switch to linear
{
	ISP_DEV IspDev = 0;
	VI_DEV_ATTR_S devAttr;
	HI_S32 s32Ret = HI_SUCCESS;
	ISP_INNER_STATE_INFO_S stInnerStateInfo = {0};
	VI_WDR_ATTR_S stWDRAttr;
	
	s32Ret = HI_MPI_VI_GetDevAttr(0,&devAttr);
	if(s32Ret != HI_SUCCESS)        
	{            
		printf("get DevAttr failed!code:%#x\n",s32Ret);        
	}        
	
	s32Ret = HI_MPI_ISP_SetFMWState(IspDev, ISP_FMW_STATE_FREEZE);
    if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);	
    }
    s32Ret = HI_MPI_VI_DisableChn(0);
	if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);	
    }
	s32Ret = HI_MPI_VI_DisableDev(0);
	if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);
		
    }
    s32Ret = SAMPLE_COMM_VI_StartMIPI();
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: SAMPLE_COMM_VI_StartMIPI failed with %#x!\n", __FUNCTION__, s32Ret);
		return ;
	}

    s32Ret = HI_MPI_ISP_SetFMWState(IspDev, ISP_FMW_STATE_RUN);
	if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);
		
    }
	
	ISP_WDR_MODE_S stWDRMode;
	if(mode)
	{
		if(sns_type == SENSOR_OV4689)
			stWDRAttr.enWDRMode=stWDRMode.enWDRMode = WDR_MODE_2To1_LINE;
		else if(sns_type == SENSOR_AR0230)
			stWDRAttr.enWDRMode=stWDRMode.enWDRMode = WDR_MODE_BUILT_IN;
		else
		{
			stWDRAttr.enWDRMode =stWDRMode.enWDRMode = WDR_MODE_NONE;
			printf("sensor wdr mode switch failed\n");
		}
	}
	else
	{
		stWDRAttr.enWDRMode =stWDRMode.enWDRMode = WDR_MODE_NONE;
	}
	s32Ret = HI_MPI_ISP_SetWDRMode(IspDev, &stWDRMode);
	if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);
    }

	while (1)
    {
        HI_MPI_ISP_QueryInnerStateInfo(IspDev, &stInnerStateInfo);
        if (HI_TRUE == stInnerStateInfo.bWDRSwitchFinish)
        {
           
			//HI_MPI_ISP_GetWDRMode(IspDev,&stWDRMode);
			//printf("wdr mode %d switch finish!!!!!!!!!!!!\n",stWDRMode.enWDRMode);
            break;
        }
	
        usleep(10000);
    }	
   
     s32Ret = HI_MPI_VI_SetDevAttr(0, &devAttr);/* h).vi config */        
	 if(s32Ret != HI_SUCCESS)        
	 {    
	 	printf("HI_MPI_VI_SetDevAttr  failed!\n");       
	 }
	 
	 
	 
	 stWDRAttr.bCompress = HI_TRUE;

	 
	 s32Ret = HI_MPI_VI_SetWDRAttr(0, &stWDRAttr);        
	 if(s32Ret != HI_SUCCESS)        
	 {            
	 	printf("HI_MPI_VI_SetWDRAttr  failed!\n");        
	 }

	 s32Ret = HI_MPI_VI_EnableDev(0);
	 if (s32Ret != HI_SUCCESS)
	 {
	 	printf("vi enable failed with %#x!\n", s32Ret);
	 }
	 s32Ret = HI_MPI_VI_EnableChn(0);
	 if (s32Ret != HI_SUCCESS)
     {
        printf("vi enable failed with %#x!\n", s32Ret);
     }
		
}

// 完全退出平台
void jv_common_deinit_platform(void)
{
	__exit_platform();
	printf("\033[0;33mhisi bussiness termination fully!\033[0;39m\n");
}

/**
 *@brief 平台相关的总初始化
 *
 */
int jv_common_init(JVCommonParam_t *param)
{
	MPP_SYS_CONF_S stSysConf = {64};
	VB_CONF_S stVbConf = {0};

	memcpy(&stJvParam, param, sizeof(stJvParam));
	hwinfo_init();
	__check_sensor();

	// 软光敏灵敏度调节
	if (hwinfo.ir_sw_mode == IRCUT_SW_BY_AE)
	{
		switch (sns_type)
		{
		case SENSOR_OV9732:
		case SENSOR_OV2710:
			hwinfo.bSupportAESens = TRUE;
			break;
		default:
			hwinfo.bSupportAESens = FALSE;
			break;
		}
	}

	signal(SIGINT, SAMPLE_VIO_HandleSig);
	signal(SIGTERM, SAMPLE_VIO_HandleSig);
	signal(SIGKILL, SAMPLE_VIO_HandleSig);

	stVbConf.u32MaxPoolCnt = 32;
	
	if(VI_HEIGHT <= 960)
	{
		//1800K * 5 = 9000K
		stVbConf.astCommPool[0].u32BlkSize	= JV_ALIGN_CEILING(VI_WIDTH, 64) * JV_ALIGN_CEILING(VI_HEIGHT, 4) * 1.5 +
			VB_HEADER_STRIDE * VI_HEIGHT * 3 / 2;
		stVbConf.astCommPool[0].u32BlkCnt	= 5;
		//662K * 5 = 3.2M
		stVbConf.astCommPool[1].u32BlkSize	= JV_ALIGN_CEILING(512, 64) * JV_ALIGN_CEILING(384, 4) * 1.5 +
			VB_HEADER_STRIDE * 384 * 3 / 2;
		stVbConf.astCommPool[1].u32BlkCnt	= 5;
		//149K * 3 = 447K
		stVbConf.astCommPool[2].u32BlkSize	= JV_ALIGN_CEILING(160, 64) * JV_ALIGN_CEILING(120, 4) * 1.5 +
			VB_HEADER_STRIDE * 120 * 3 / 2;
		stVbConf.astCommPool[2].u32BlkCnt	= 4;
	}
	else if(VI_HEIGHT == 1080)
	{
		//3037.5K * 2 = 6076K
		stVbConf.astCommPool[0].u32BlkSize	= JV_ALIGN_CEILING(VI_WIDTH, 64) * JV_ALIGN_CEILING(VI_HEIGHT, 4) * 1.5 + 
			VB_HEADER_STRIDE * VI_HEIGHT * 3 / 2;
		stVbConf.astCommPool[0].u32BlkCnt	= hwinfo.bVgsDrawLine ? 4 : 3;
 
		//648K * 2 = 1296K
		stVbConf.astCommPool[1].u32BlkSize	= JV_ALIGN_CEILING(512, 64) * JV_ALIGN_CEILING(288, 4) * 1.5 +
			VB_HEADER_STRIDE * 288 * 3 / 2;
		stVbConf.astCommPool[1].u32BlkCnt	= 5;

		//162K * 6 = 972K
		stVbConf.astCommPool[2].u32BlkSize	= JV_ALIGN_CEILING(160, 64) * JV_ALIGN_CEILING(120, 4) * 1.5 +
			VB_HEADER_STRIDE * 120 * 3 / 2;
		stVbConf.astCommPool[2].u32BlkCnt	= 4;
	}
	XDEBUG(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");
	XDEBUG(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");

	XDEBUG(HI_MPI_VB_SetConf(&stVbConf),"HI_MPI_VB_SetConf");
	XDEBUG(HI_MPI_VB_Init(),"HI_MPI_VB_Init");
	XDEBUG(HI_MPI_SYS_SetConf(&stSysConf),"HI_MPI_SYS_SetConf");
	XDEBUG(HI_MPI_SYS_Init(),"HI_MPI_SYS_Init");

	if(stJvParam.bEnableWdr)
		InitVI(1);
	else
		InitVI(0);
	InitVpss();
	sensor_ircut_init();
	return 0;
}

/**
 *@brief 平台相关的结束
 *
 */
int jv_common_deinit(void)
{
	sensor_ircut_deinit();
	XDEBUG(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
	XDEBUG(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");

	ReleaseVI();

	
	return 0;
}

int jv_common_mkfifo(char *fname, mode_t mode)
{
	char tn[MAX_PATH];
	char *p;
	int ret;

	if (access(fname, F_OK) == 0)
		return 0;
	strncpy(tn, fname, MAX_PATH);
	p = tn;
	while(1)
	{
		p = strchr(p+1, '/');
		if (p != NULL)
		{
			*p = '\0';
			if (access(tn, F_OK) == -1)
			{
				ret = mkdir(tn, mode);
				if (ret == -1)
				{
					printf("mkdir %s err: %s\n",tn, strerror(errno));
					return -1;
				}
			}
			*p = '/';
		}
		else
		{
			break;
		}
	}
	ret = mkfifo(fname, mode);
	return ret;
}

void jv_flash_write_lock_init()
{
//	flash_write_lock_fd = open(SPI_FLASH_DEVNAME, O_RDWR);
//	if (flash_write_lock_fd < 0)
//	{
//		printf("open dev[%s] failed!,not support!\n", SPI_FLASH_DEVNAME);
//	}
}
void jv_flash_write_lock()
{
//	if(flash_write_lock_fd>=0)
//	{
//		if(ioctl(flash_write_lock_fd, SPI_WRITE_PROTECT, (int)flash_4M_8M)<0)
//			printf("=====================================flash write lock failed!\n");
//		else
//			printf("=====================================flash write lock success!\n");
//	}
}
void jv_flash_write_unlock()
{
//	if(flash_write_lock_fd>=0)
//	{
//		if(ioctl(flash_write_lock_fd, SPI_WRITE_PROTECT, (int)flash_0M_8M)<0)
//			printf("=====================================flash write unlock failed!\n");
//		else
//			printf("=====================================flash write unlock success!\n");
//	}
}
void jv_flash_write_lock_deinit()
{
//	if(flash_write_lock_fd>=0)
//	{
//		close(flash_write_lock_fd);
//	}
}

#if 0
#define __IDLE_FUNC__(x)	int x(void){return 0;}

__IDLE_FUNC__(JVN_SendData)
	__IDLE_FUNC__(JVN_StartLANSerchServer)
	__IDLE_FUNC__(JVN_SendChatData)
	__IDLE_FUNC__(JVN_SendDataTo)
	__IDLE_FUNC__(JVN_BroadcastRSP)
	__IDLE_FUNC__(JVN_SendMOData)

	__IDLE_FUNC__(JVN_ActiveYSTNO)
	__IDLE_FUNC__(JVN_InitYST)
	__IDLE_FUNC__(JVN_InitSDK)
	__IDLE_FUNC__(JVN_RegisterCallBack)
	__IDLE_FUNC__(JVN_EnableLog)
	__IDLE_FUNC__(JVN_SetLanguage)
	__IDLE_FUNC__(JVN_SetClientLimit)
	__IDLE_FUNC__(JVN_SetDeviceName)
	__IDLE_FUNC__(JVN_StartBroadcastServer)
	__IDLE_FUNC__(JVN_StopBroadcastServer)
	__IDLE_FUNC__(JVN_ReleaseSDK)
	__IDLE_FUNC__(JVN_StartChannel)
	__IDLE_FUNC__(JVN_StopChannel)
	__IDLE_FUNC__(JVN_EnableMOServer)
#endif
