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

int flash_write_lock_fd = -1;
static HI_U16 ispFramerate = 25;	//0时默认初始化30帧，30帧重启会变成25帧。

int MAX_FRAME_RATE = 30;
int VI_WIDTH = 2592;
int VI_HEIGHT = 1944;

BOOL VI_CROP_ENABLE= FALSE;
int VI_CROP_X  =0;
int VI_CROP_Y  =0;
int VI_CROP_W  =1920;
int VI_CROP_H = 1080;

int jv_sensor_set_wdr_mode(BOOL  bWdrEnable);

static JVCommonParam_t stJvParam;

#define SAMPLE_PIXEL_FORMAT	PIXEL_FORMAT_YUV_SEMIPLANAR_420

#define sns_type hwinfo.sensor

static pthread_t gs_IspPid;
#if ENABLE_0130_960
#define AR0130_ATTR	DEV_ATTR_AR0130_DC_960P
#else
#define AR0130_ATTR	DEV_ATTR_AR0130_DC_720P
#endif
//目前平台上对接了SENSOR_OV9712 SENSOR_AR0130 MT9M034 三款sensor 其中AR0130与MT9M034使用同一个VI_DEV_ATTR_S

BOOL ipcinfo_b_supportAWAF();
BOOL ipcinfo_b_supportAWMF();

#include <termios.h>

hwinfo_t hwinfo;

#include "jv_gpio.h"

static void __gpio_get_value(const char *name, GpioValue_t *value, int defGroup, int defBit)
{
	__get_gpio_group_bit(name, &value->group, &value->bit, defGroup, defBit);
}

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
	__gpio_get_value("cut-check", &higpios.cutcheck, 0, 3);
	__gpio_get_value("cut-day", &higpios.cutday, 5, 2);
	__gpio_get_value("cut-night", &higpios.cutnight, 5, 3);
	__gpio_get_value("redlight", &higpios.redlight, -1, -1);
	__gpio_get_value("alarmin1", &higpios.alarmin1, 10, 0);
	__gpio_get_value("alarmin2", &higpios.alarmin2, 10, 1);
	__gpio_get_value("alarmout1", &higpios.alarmout1, 10, 2);
	__gpio_get_value("sensorreset", &higpios.sensorreset, -1, -1);
	__gpio_get_value("pir", &higpios.pir, -1, -1);
	 {
		//audio out mute
		higpios.audioOutMute.group = 6;
		higpios.audioOutMute.bit = 1;
		jv_gpio_muxctrl(0x200F0084, 0);
		jv_gpio_dir_set_bit(higpios.audioOutMute.group, higpios.audioOutMute.bit, 1);
	 }
	if (higpios.cutcheck.group != -1)
	{
		//set redlightGroup input
		dir = jv_gpio_dir_get(higpios.cutcheck.group);
		dir &= ~(1 << higpios.cutcheck.bit);
		jv_gpio_dir_set(higpios.cutcheck.group, dir);
	}

	if (higpios.redlight.group != -1)
	{
		//set redlightGroup output
		dir = jv_gpio_dir_get(higpios.redlight.group);
		dir |= 1 << higpios.redlight.bit;
		jv_gpio_dir_set(higpios.redlight.group, dir);
	}
	if (higpios.cutday.group != -1)
	{
		dir = jv_gpio_dir_get(higpios.cutday.group);
		dir |= 1 << higpios.cutday.bit;
		jv_gpio_dir_set(higpios.cutday.group, dir);
	}
	if (higpios.cutnight.group != -1)
	{
		dir = jv_gpio_dir_get(higpios.cutnight.group);
		dir |= 1 << higpios.cutnight.bit;
		jv_gpio_dir_set(higpios.cutnight.group, dir);
	}

	if (higpios.alarmin1.group != -1)
	{
		dir = jv_gpio_dir_get(higpios.alarmin1.group);
		dir &= (~(1 << higpios.alarmin1.bit));
		jv_gpio_dir_set(higpios.alarmin1.group, dir);
	}
	if (higpios.alarmin2.group != -1)
	{
		dir = jv_gpio_dir_get(higpios.alarmin2.group);
		dir &= (~(1 << higpios.alarmin2.bit));
		jv_gpio_dir_set(higpios.alarmin2.group, dir);
	}
	if (higpios.alarmout1.group != -1)
	{
		dir = jv_gpio_dir_get(higpios.alarmout1.group);
		dir |= 1 << higpios.alarmout1.bit;
		jv_gpio_dir_set(higpios.alarmout1.group, dir);
	}
	//higpios.sensorreset.group =0;
	//higpios.sensorreset.bit =0;
	if (higpios.sensorreset.group != -1)
	{
		dir = jv_gpio_dir_get(higpios.sensorreset.group);
		dir |= 1 << higpios.sensorreset.bit;
		jv_gpio_dir_set(higpios.sensorreset.group, dir);
		jv_gpio_write(higpios.sensorreset.group, higpios.sensorreset.bit, 0);
		usleep(100*1000);
		jv_gpio_write(higpios.sensorreset.group, higpios.sensorreset.bit, 1);
		usleep(100*1000);
	}

	if (hwinfo.bHomeIPC)
	{
		//io key
		higpios.resetkey.group = 9;
		higpios.resetkey.bit = 2;
		jv_gpio_muxctrl(0x200F00D0, 1);
		jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);

		//status led
		higpios.statusled.group = 0;
		higpios.statusled.bit = 6;
		jv_gpio_muxctrl(0x200F0138, 0);
		jv_gpio_dir_set_bit(higpios.statusled.group, higpios.statusled.bit, 1);

	}

	else
	{
		//非家用机，0-6与9-1都接到了CUT检测口
		//jv_gpio_dir_set_bit(0, 6, 0);

		higpios.resetkey.group = -1;
		higpios.statusled.group = -1;
	}
}

void hwinfo_init()
{
	memset(&hwinfo, 0, sizeof(hwinfo));
    strcpy(hwinfo.type,"Hi3516D");
	hwinfo.ptzBsupport = FALSE;
	hwinfo.ptzbaudrate = 9600;
	hwinfo.ptzprotocol = 29; //标准 PELCO-D (BandRate = 2400)
    hwinfo.wdrBsupport=TRUE;
    hwinfo.rotateBSupport=TRUE;
    hwinfo.encryptCode = ENCRYPT_500W;
    hwinfo.streamCnt = 3;
    hwinfo.ipcType = stJvParam.ipcType;
    hwinfo.ir_sw_mode = IRCUT_SW_BY_GPIO;
    hwinfo.ir_power_holdtime = 500*1000;
	
    strcpy(hwinfo.product, "Unknown");
    utl_fcfg_get_value_ex(JOVISION_CONFIG_FILE, "product", hwinfo.product, sizeof(hwinfo.product));
	utl_fcfg_get_value_ex(CONFIG_HWCONFIG_FILE, "product", hwinfo.devName, sizeof(hwinfo.devName));
    int ipdome = 0;
	if(hwinfo.ipcType == IPCTYPE_JOVISION && !hwinfo.bHomeIPC)
	{
		hwinfo.bSupportMultiOsd = TRUE;
	}
    __gpio_init();
}

VI_DEV_ATTR_EX_S DEV_ATTR_YC1080P_BASE_POEWRVISION_EX =
{
    /* interface mode */
    VI_MODE_BT601,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
	
	VI_COMBINE_SEPARATE,

	VI_COMP_MODE_DOUBLE,

	VI_CLK_EDGE_SINGLE_UP,
    /* r_mask     g_mask    b_mask*/
    {0xFF000000,    0xFF0000},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    { -1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_UVUV,

    /* synchronization information */
    {
        /*port_vsync   port_vsync_neg      port_hsync        port_hsync_neg          */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_PULSE, VI_HSYNC_NEG_HIGH, VI_VSYNC_NORM_PULSE, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            192,              1920,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            42,              1080,          2,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,              0,            0
        }
    },

	{BT656_FIXCODE_1,BT656_FIELD_POLAR_STD},

    /* use interior ISP */
    VI_PATH_BYPASS,
    /* input data type */
    VI_DATA_TYPE_YUV,
    /* bRever */
    HI_FALSE,
    /* DEV CROP */
    {0, 0, 1920, 1080}
};

VI_DEV_ATTR_S DEV_ATTR_IMX178_DC_5M =
{
 /* interface mode */
    VI_MODE_LVDS,
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
    {0, 0, 1920, 1080}
	
};
VI_DEV_ATTR_S DEV_ATTR_IMX290 =
{
 /* interface mode */
    VI_MODE_LVDS,
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
    {0, 0, 1920, 1080}
};

VI_DEV_ATTR_S DEV_ATTR_IMX123 =
{
 /* interface mode */
    VI_MODE_LVDS,
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
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,
   
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            2048,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1536,        0,
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
    {0, 20, 2048, 1536}
};
VI_DEV_ATTR_S DEV_ATTR_OV2710_DC_1080P =
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
    VI_DATA_TYPE_RGB
};


VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE_OV4689 =
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
    {0,            2592,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1520,        0,
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
    {0, 0, 1920, 1080}
};
VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE_OV4689_3M =
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
    {0,            2048,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1520,        0,
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
    {0, 0, 1920, 1080}
};

VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE_IMX185 =
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
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,
   
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
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
    {0, 0, 1920, 1080}
};


VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE_AR0330=
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
			0,			  2048, 	   0,
			/*vsync0_vhb vsync0_act vsync0_hhb*/
			0,			  1536, 	  0,
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
	{0, 0, 1920, 1080}
};


	VI_DEV_ATTR_S DEV_ATTR_LVDS_BASE_AR0230 =
	{
		/* interface mode */
		VI_MODE_LVDS,
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
			VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,
	
			/*hsync_hfb    hsync_act	hsync_hhb*/
			{
				0,			  1280, 	   0,
				/*vsync0_vhb vsync0_act vsync0_hhb*/
				0,			  720,		  0,
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
		{0, 0, 1920, 1080}
	};
	
	VI_DEV_ATTR_S DEV_ATTR_LVDS_BASE_AR0237 =
	{
		/* interface mode */
		VI_MODE_LVDS,
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
			VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,
	
			/*hsync_hfb    hsync_act	hsync_hhb*/
			{
				0,			  1280, 	   0,
				/*vsync0_vhb vsync0_act vsync0_hhb*/
				0,			  720,		  0,
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
		{0, 0, 1920, 1080}
	};

VI_DEV_ATTR_S DEV_ATTR_AR0237DC_1080P =
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
		VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

		/*timing信息，对应reg手册的如下配置*/
		/*hsync_hfb    hsync_act    hsync_hhb*/
		{
			0,            1920,        0,
			/*vsync0_vhb vsync0_act vsync0_hhb*/
			0,            1080,        0,
			/*vsync1_vhb vsync1_act vsync1_hhb*/
			0,            0,            0
		}
	},
	/*使用内部ISP*/
	VI_PATH_ISP,
	/*输入数据类型*/
	VI_DATA_TYPE_RGB
};


combo_dev_attr_t LVDS_4lane_SENSOR_IMX178_12BIT_5M_NOWDR_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_LVDS,
    {
        .lvds_attr = {
            .img_size = {2592, 1944},
            HI_WDR_MODE_NONE,
            LVDS_SYNC_MODE_SAV,
            RAW_DATA_12BIT,
            LVDS_ENDIAN_BIG,
            LVDS_ENDIAN_BIG,
            .lane_id = {0, 1, 2, 3, -1, -1, -1, -1},
            .sync_code = { 
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},

                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                        
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},

                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}} 
                }
        }
    }
};
combo_dev_attr_t LVDS_4lane_SENSOR_IMX290_12BIT_2M_NOWDR_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_LVDS,
    {
        .lvds_attr = {
            .img_size = {1920, 1080},
            HI_WDR_MODE_NONE,
            LVDS_SYNC_MODE_SAV,
            RAW_DATA_12BIT,
            LVDS_ENDIAN_BIG,
            LVDS_ENDIAN_BIG,
            .lane_id = {0, 1, 2, 3, -1, -1, -1, -1},
            .sync_code = { 
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},

                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                        
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},

                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}} 
                }
        }
    }
};

combo_dev_attr_t SENSOR_BT601_12BIT_2M_NOWDR_ATTR = 
{
    .input_mode = INPUT_MODE_BT1120,  
    {

    }    
};
combo_dev_attr_t DVP_SENSOR_OV2710_10BIT_2M_NOWDR_ATTR = 
{
    .input_mode = INPUT_MODE_CMOS_33V,  
    {

    }    
};


combo_dev_attr_t LVDS_4lane_SENSOR_IMX123_12BIT_3M_NOWDR_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_SUBLVDS,
    {
        .lvds_attr = {
            .img_size = {2048, 1536},
            HI_WDR_MODE_NONE,
            LVDS_SYNC_MODE_SAV,
            RAW_DATA_12BIT,
            LVDS_ENDIAN_BIG,
            LVDS_ENDIAN_BIG,
            .lane_id = {0, 1, 2, 3, 4, 5, 6, 7},
            .sync_code = { 
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},

                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                        
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},

                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}},
                    
                    {{0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}, 
                    {0xab0, 0xb60, 0x800, 0x9d0}} 
                }
        }
    }
};


combo_dev_attr_t MIPI_4lane_SENSOR_OV4689_12BIT_ATTR = 
{
    .input_mode = INPUT_MODE_MIPI,  
    {

        .mipi_attr =    
        {
            RAW_DATA_12BIT,
            {0, 1, 2, 3, -1, -1, -1, -1}
        }
    }    
};

combo_dev_attr_t MIPI_4lane_SENSOR_AR0330_12BIT_ATTR = 
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

combo_dev_attr_t HISPI_4lane_SENSOR_AR0230_12BIT_1080p_NOWDR_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_HISPI,
        
    {
        .lvds_attr = {
            .img_size = {1920, 1080},
            HI_WDR_MODE_NONE,            
            LVDS_SYNC_MODE_SOL,
            RAW_DATA_12BIT,                     
            LVDS_ENDIAN_LITTLE,
            LVDS_ENDIAN_LITTLE, 
            .lane_id = {0, 1, 2, 3, -1, -1, -1, -1},
            .sync_code = {                               
                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane1
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane2
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane3
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY1_lane0
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY1_lane1
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY1_lane2
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY1_lane3
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5},
                   {0x3, 0x7, 0x1, 0x5}},

            }        

        }
    }
};
combo_dev_attr_t HISPI_4lane_SENSOR_AR0237_12BIT_1080p_NOWDR_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_HISPI,
        
    {
        .lvds_attr = {
            .img_size = {1920, 1080},
            HI_WDR_MODE_NONE,            
            LVDS_SYNC_MODE_SOL,
            RAW_DATA_12BIT,                     
            LVDS_ENDIAN_LITTLE,
            LVDS_ENDIAN_LITTLE, 
            .lane_id = {0, 1, 2, 3, -1, -1, -1, -1},
            .sync_code = {                               
                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},


                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},


                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

            }        

        }
    }
};
combo_dev_attr_t HISPI_4lane_SENSOR_AR0237_12BIT_1080p_WDR_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_HISPI,
        
    {
        .lvds_attr = {
            .img_size = {1920, 1080},
            HI_WDR_MODE_2F,            
            LVDS_SYNC_MODE_SOL,
            RAW_DATA_12BIT,                     
            LVDS_ENDIAN_LITTLE,
            LVDS_ENDIAN_LITTLE, 
            .lane_id = {0, 1, 2, 3, -1, -1, -1, -1},
            .sync_code = {                               
                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},


                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},


                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

                   {{0x3, 0x7, 0x1, 0x5}, //PHY0_lane0
                   {0x43, 0x47, 0x41, 0x45},
                   {0x23, 0x27, 0x21, 0x25},
                   {0x83, 0x87, 0x81, 0x85}},

            }        

        }
    }
};

combo_dev_attr_t DVP_SENSOR_AR0237DC_12BIT_2M_NOWDR_ATTR = 
{
    .input_mode = INPUT_MODE_CMOS_33V,  
    {

    }    
};


combo_dev_attr_t LVDS_4lane_SENSOR_IMX123_12BIT_WDR_ATTR = 
{
    /* input mode */
    .input_mode = INPUT_MODE_LVDS,
    {
        .lvds_attr = {
        .img_size = {2048, 1536},
        HI_WDR_MODE_DOL_2F,
        LVDS_SYNC_MODE_SAV,
        RAW_DATA_12BIT,
        LVDS_ENDIAN_BIG,
        LVDS_ENDIAN_BIG,
        .lane_id = {0, 1, 2, 3, 4, 5, 6, 7},
        .sync_code = { 
                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},
                
                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},

                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},
                
                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},
                
                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},
                    
                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},

                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}},
                
                {{0x2b0, 0x360, 0x801, 0x9d1}, 
                {0x2b0, 0x360, 0x802, 0x9d2}, 
                {0x2b0, 0x360, 0x804, 0x9d4}, 
                {0x2b0, 0x360, 0x808, 0x9d8}} 
            }
        }
    }
};

combo_dev_attr_t MIPI_4lane_SENSOR_IMX185_12BIT_ATTR =
{   
	.input_mode = INPUT_MODE_MIPI,    
	 {        
	 	.mipi_attr =  
		{           
			RAW_DATA_12BIT,            
			{3, 0, 1, 2, -1, -1, -1, -1}        
		}   
	 }
};



HI_S32 SAMPLE_COMM_VI_SetMipiAttr(BOOL bWDREnable)
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

	if (sns_type == SENSOR_IMX178)
	{
		pstcomboDevAttr = &LVDS_4lane_SENSOR_IMX178_12BIT_5M_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_BT601)
	{
		printf("Bt601\n");
		pstcomboDevAttr = &SENSOR_BT601_12BIT_2M_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_IMX290)
	{
		pstcomboDevAttr = &LVDS_4lane_SENSOR_IMX290_12BIT_2M_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_OV2710)
	{
		pstcomboDevAttr = &DVP_SENSOR_OV2710_10BIT_2M_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_IMX123)
	{
		pstcomboDevAttr = &LVDS_4lane_SENSOR_IMX123_12BIT_3M_NOWDR_ATTR;
		if(bWDREnable)
			pstcomboDevAttr = &LVDS_4lane_SENSOR_IMX123_12BIT_WDR_ATTR;
	}
	else if(sns_type == SENSOR_OV4689)
	{
		pstcomboDevAttr = &MIPI_4lane_SENSOR_OV4689_12BIT_ATTR;

	}
	else if(sns_type == SENSOR_AR0330)
	{
		pstcomboDevAttr =&MIPI_4lane_SENSOR_AR0330_12BIT_ATTR;
	}
	else if(sns_type == SENSOR_AR0230)
	{
		pstcomboDevAttr = &HISPI_4lane_SENSOR_AR0230_12BIT_1080p_NOWDR_ATTR;

	}
	else if(sns_type == SENSOR_AR0237)
	{
		pstcomboDevAttr = &HISPI_4lane_SENSOR_AR0237_12BIT_1080p_NOWDR_ATTR;
		if(bWDREnable)
			pstcomboDevAttr = &HISPI_4lane_SENSOR_AR0237_12BIT_1080p_WDR_ATTR;

	}
    else if(sns_type == SENSOR_AR0237DC)
	{
		pstcomboDevAttr = &DVP_SENSOR_AR0237DC_12BIT_2M_NOWDR_ATTR;
	}
	else if(sns_type == SENSOR_IMX185)
	{
		pstcomboDevAttr = &MIPI_4lane_SENSOR_IMX185_12BIT_ATTR;

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
HI_S32 SAMPLE_COMM_VI_StartMIPI(BOOL bWDREnable)
{
	int s32Ret;

	s32Ret = SAMPLE_COMM_VI_SetMipiAttr(bWDREnable);
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
	if(WdrEnble&&(sns_type == SENSOR_OV4689))
		stWdrMode.enWDRMode  = WDR_MODE_2To1_LINE;
	else if(WdrEnble&&(sns_type == SENSOR_AR0230))
		stWdrMode.enWDRMode  = WDR_MODE_BUILT_IN;
	else if(WdrEnble&&(sns_type == SENSOR_AR0237||sns_type== SENSOR_AR0237DC))
		stWdrMode.enWDRMode  = WDR_MODE_2To1_LINE;
	else if(WdrEnble&&(sns_type == SENSOR_BT601))
		stWdrMode.enWDRMode  = WDR_MODE_BUILT_IN;
	else if(WdrEnble&&(sns_type == SENSOR_IMX123))
		stWdrMode.enWDRMode  = WDR_MODE_2To1_LINE;
	else
		stWdrMode.enWDRMode  = WDR_MODE_NONE;
	s32Ret = HI_MPI_ISP_SetWDRMode(0, &stWdrMode);
	if (HI_SUCCESS != s32Ret)
	{
		printf("start ISP WDR failed!\n");
		return s32Ret;
	}

	/* 7. isp set pub attributes */
	if(sns_type == SENSOR_IMX178)
	{
		stPubAttr.enBayer             = BAYER_GBRG;
		stPubAttr.f32FrameRate        = frm;
		stPubAttr.stWndRect.s32X      = 0;
		stPubAttr.stWndRect.s32Y      = 0;
		stPubAttr.stWndRect.u32Width  = VI_WIDTH;
		stPubAttr.stWndRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_IMX290)
	{
		stPubAttr.enBayer             = BAYER_GBRG;
		stPubAttr.f32FrameRate        = frm;
		stPubAttr.stWndRect.s32X      = 0;
		stPubAttr.stWndRect.s32Y      = 0;
		stPubAttr.stWndRect.u32Width  = VI_WIDTH;
		stPubAttr.stWndRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_IMX123)
	{
		stPubAttr.enBayer             = BAYER_GBRG;
		stPubAttr.f32FrameRate        = frm;
		stPubAttr.stWndRect.s32X      = 0;
		stPubAttr.stWndRect.s32Y      = 0;
		stPubAttr.stWndRect.u32Width  = VI_WIDTH;
		stPubAttr.stWndRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV4689)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
		//帧频越低，低照度效果越好，但不影响最终输出 
		//if(stWdrMode.enWDRMode == WDR_MODE_2To1_LINE)
        	//stPubAttr.f32FrameRate          = 20;
		//else
		stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = VI_WIDTH;
        stPubAttr.stWndRect.u32Height   = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV2710)
	{
		stPubAttr.enBayer               = BAYER_BGGR;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = VI_WIDTH;
        stPubAttr.stWndRect.u32Height   = VI_HEIGHT;

	}
	else if(sns_type == SENSOR_AR0330)
	{
		stPubAttr.enBayer               = BAYER_GRBG;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = VI_WIDTH;
        stPubAttr.stWndRect.u32Height   = VI_HEIGHT;
	}
	
	else if(sns_type == SENSOR_AR0230)
	{
		stPubAttr.enBayer               = BAYER_GRBG;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = VI_WIDTH;
        stPubAttr.stWndRect.u32Height   = VI_HEIGHT;

	}
	else if(sns_type == SENSOR_AR0237||sns_type == SENSOR_AR0237DC)
	{
		stPubAttr.enBayer               = BAYER_GRBG;
        stPubAttr.f32FrameRate          = frm;
        stPubAttr.stWndRect.s32X        = 0;
        stPubAttr.stWndRect.s32Y        = 0;
        stPubAttr.stWndRect.u32Width    = VI_WIDTH;
        stPubAttr.stWndRect.u32Height   = VI_HEIGHT;

	}
	else if(sns_type == SENSOR_IMX185)
	{
		stPubAttr.enBayer             = BAYER_RGGB;
		stPubAttr.f32FrameRate        = frm;
		stPubAttr.stWndRect.s32X      = 0;
		stPubAttr.stWndRect.s32Y      = 0;
		stPubAttr.stWndRect.u32Width  = VI_WIDTH;
		stPubAttr.stWndRect.u32Height = VI_HEIGHT;
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
	VI_DEV_ATTR_EX_S *devAttrEx;
	ISP_WDR_MODE_S stWDRMode;

	if(sns_type == SENSOR_IMX178)
	{
		Printf("imx178 running\n");
		devAttr = &DEV_ATTR_IMX178_DC_5M;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 20;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_IMX290)
	{
		printf("imx290 running\n");
		devAttr = &DEV_ATTR_IMX290;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 30;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_IMX185)
	{
		Printf("imx185 running\n");
		devAttr = &DEV_ATTR_MIPI_BASE_IMX185;
		devAttr->stDevRect.s32X = 6;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_IMX123)
	{
		printf("imx123 running\n");
		devAttr = &DEV_ATTR_IMX123;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 20;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV4689)
	{
		devAttr = &DEV_ATTR_MIPI_BASE_OV4689;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV2710)
	{
		devAttr = &DEV_ATTR_OV2710_DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_AR0330)
	{
		devAttr = &DEV_ATTR_MIPI_BASE_AR0330;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_AR0230)
	{
		devAttr = &DEV_ATTR_LVDS_BASE_AR0230;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_AR0237)
	{
		devAttr = &DEV_ATTR_LVDS_BASE_AR0237;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
    	else if(sns_type == SENSOR_AR0237DC)
	{
		devAttr = &DEV_ATTR_AR0237DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_BT601)
	{
		devAttrEx = &DEV_ATTR_YC1080P_BASE_POEWRVISION_EX;
		devAttrEx->stDevRect.s32X = 0;
		devAttrEx->stDevRect.s32Y = 0;
		devAttrEx->stDevRect.u32Width  = VI_WIDTH;
		devAttrEx->stDevRect.u32Height = VI_HEIGHT;
	}
	else
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		return HI_FAILURE;
	}
	
	if(sns_type == SENSOR_BT601)
	{
		s32Ret = HI_MPI_VI_SetDevAttrEx(ViDev, devAttrEx);
	}
	else
	{
		s32Ret = HI_MPI_VI_SetDevAttr(ViDev, devAttr);
	}
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
		
		if(sns_type == SENSOR_IMX123)
			stWdrAttr.bCompress = HI_FALSE;
		
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
	VPSS_CHN VpssChn;
	VPSS_CHN_ATTR_S stVpssChnAttr;
	int i;

	for(i=0; i<HWINFO_STREAM_CNT; i++)
	{
		VpssChn = i;
		HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
		stVpssChnAttr.bFlip = bFlip;
		stVpssChnAttr.bMirror = bMirror;
		HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
	}
}

#define ___VPSS___
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
	
    if (VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        U32 u32CoverMask = 0xFF;
        s32Ret = HI_MPI_VPSS_SetChnCover(VpssGrp, VpssChn, u32CoverMask);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VPSS_SetChnCover fail! Grp: %d, Chn: %d! s32Ret: %#x\n", VpssGrp, VpssChn, s32Ret);
            return s32Ret;
        }

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
static void __check_sensor(void)
{
	int sensor;
	char *cmd;
	char *str;
	BOOL bRd = FALSE;
	char *fname = CONFIG_PATH"/sensor.sh";
	isp_ioctl(0,GET_ID,(unsigned long)&sensor);
	Printf("====>>. sensor: %d\n", sensor);
	if (access(fname, F_OK) == 0)
	{
		str = utl_fcfg_get_value(fname, "SENSOR");
		if (str != NULL)
		{
			if (strcmp(str, "imx178") == 0)
			{
				sns_type = SENSOR_IMX178;
				//strcpy(hwinfo.type,"N95-HC-A");
				VI_WIDTH = 2592;
				VI_HEIGHT = 1944;
				hwinfo.encryptCode = ENCRYPT_500W;
			}
			else if (strcmp(str, "bt601") == 0)
			{
				sns_type = SENSOR_BT601;
				//strcpy(hwinfo.type,"N95-HC-A");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
			}
			else if (strcmp(str, "ov4689") == 0)
			{
				sns_type = SENSOR_OV4689;
				//strcpy(hwinfo.type,"N91-HC-A");
				VI_WIDTH = 2560;
				VI_HEIGHT = 1440;
				char *rd = utl_fcfg_get_value(CONFIG_HWCONFIG_FILE,"400wto300w");	//400w降300w
				if(rd)
				{
					if (atoi(rd))
					{
						VI_WIDTH = 2048;
						VI_HEIGHT = 1520;
						bRd = TRUE;
					}
				}
				hwinfo.encryptCode = ENCRYPT_300W;
			}
			else if (strcmp(str, "imx185") == 0)
			{
				sns_type = SENSOR_IMX185;
				//strcpy(hwinfo.type,"N91-HC-A");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
			}
			else if(strcmp(str, "ar0330") == 0)
			{
				sns_type = SENSOR_AR0330;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 2048;
				VI_HEIGHT = 1536;
				hwinfo.encryptCode = ENCRYPT_300W;
			}	
			else if (strcmp(str, "ar0230") == 0)
			{
				sns_type = SENSOR_AR0230;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_CROP_ENABLE = TRUE;
				VI_CROP_W =  1920-64-64;
				VI_CROP_H =   1080;
				VI_CROP_X  =64;
				VI_CROP_Y  =0;
			}
			else if (strcmp(str, "ar0237") == 0)
			{
				sns_type = SENSOR_AR0237;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
			}
            else if (strcmp(str, "ar0237dc") == 0)
			{
				sns_type = SENSOR_AR0237DC;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
			}
			else if (strcmp(str, "imx290") == 0)
			{
				sns_type = SENSOR_IMX290;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
			}
			else if (strcmp(str, "ov2710") == 0)
			{
				sns_type = SENSOR_OV2710;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
				VI_CROP_ENABLE = TRUE;
				VI_CROP_W =  1920-64-96;//1920-144-64;
				VI_CROP_H =   1080;
				VI_CROP_X  =64;
				VI_CROP_Y  =0;
			}
			else if (strcmp(str, "imx123") == 0)
			{
				sns_type = SENSOR_IMX123;
				//strcpy(hwinfo.type,"N91-HC-W");
				VI_WIDTH = 2048;
				VI_HEIGHT = 1536;
				hwinfo.encryptCode = ENCRYPT_300W;
			}
			else
			{
				Printf("Failed Find sensor\n");
				sns_type = (unsigned int)-1;
				unlink(fname);
			}

			//hwinfo.type 统一格式，统一处理：芯片名称-主板类型-分辨率：H8E-P-10：HI3518E-38板-100W
			{
				memset(hwinfo.type,0,sizeof(hwinfo.type));
				strncpy(hwinfo.type, "H6D-", strlen("H6D-"));
				if (strstr(hwinfo.devName, "N52-HS"))
					strcat(hwinfo.type, "S-");
				else
					strcat(hwinfo.type, "P-");
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
					if(sns_type == SENSOR_OV4689 && !bRd)	//4689默认是400W
						strcat(hwinfo.type, "40");
					else
						strcat(hwinfo.type, "30");
					break;
				case ENCRYPT_500W:
					strcat(hwinfo.type, "50");
					break;
				}
			}

			if (strstr(hwinfo.product, "GC"))//工程机
			{
				strcat(hwinfo.type, "GC");
			}
			if (sns_type == sensor)
			{
				//success
				utl_fcfg_close(fname);
				return ;
			}
		}
	}
	{
		if(sensor == SENSOR_IMX178)
		{
			sns_type = SENSOR_IMX178;
			cmd = "SENSOR=imx178";
			hwinfo.encryptCode = ENCRYPT_500W;
		}
		else if(sensor == SENSOR_IMX185)
		{
			sns_type = SENSOR_IMX185;
			cmd = "SENSOR=imx185";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if(sensor == SENSOR_BT601)
		{
			printf("SBT\n");
			sns_type = SENSOR_BT601;
			cmd = "SENSOR=bt601";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if(sensor == SENSOR_OV4689)
		{
			sns_type = SENSOR_OV4689;
			cmd = "SENSOR=ov4689";
			hwinfo.encryptCode = ENCRYPT_300W;
		}
		else if(sensor == SENSOR_AR0330)
		{
			sns_type = SENSOR_AR0330;
			cmd = "SENSOR=ar0330";
			hwinfo.encryptCode = ENCRYPT_300W;
		}
		else if(sensor == SENSOR_AR0230)
		{
			sns_type = SENSOR_AR0230;
			cmd = "SENSOR=ar0230";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if(sensor == SENSOR_AR0237)
		{
			sns_type = SENSOR_AR0237;
			cmd = "SENSOR=ar0237";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
        else if(sensor == SENSOR_AR0237DC)
		{
			sns_type = SENSOR_AR0237DC;
			cmd = "SENSOR=ar0237dc";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if(sensor == SENSOR_OV2710)
		{
			sns_type = SENSOR_OV2710;
			cmd = "SENSOR=ov2710";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if(sensor == SENSOR_IMX290)
		{
			sns_type = SENSOR_IMX290;
			cmd = "SENSOR=imx290";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if(sensor == SENSOR_IMX123)
		{
			sns_type = SENSOR_IMX123;
			cmd = "SENSOR=imx123";
			hwinfo.encryptCode = ENCRYPT_300W;
		}
		FILE *fp;
		fp = fopen(fname, "wb");
		fputs(cmd, fp);
		fclose(fp);
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

static VOID InitVI(int mode)
{
	S32 s32Ret;
	BOOL bWDREnable =FALSE;
	if(sns_type == SENSOR_IMX178)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_imx178.so", &sensor);
	}
	else if(sns_type == SENSOR_AR0330)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ar0330.so", &sensor);
	}
	else if(sns_type == SENSOR_AR0230)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ar0230.so", &sensor);
	}
	else if(sns_type == SENSOR_AR0237)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ar0237.so", &sensor);
	}
    else if(sns_type == SENSOR_AR0237DC)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ar0237DC.so", &sensor);
	}
	else if(sns_type == SENSOR_OV2710)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2710.so", &sensor);
	}
	else if(sns_type == SENSOR_IMX185)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_imx185.so", &sensor);
	}
	else if(sns_type == SENSOR_IMX290)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_imx290.so", &sensor);
	}
	else if(sns_type == SENSOR_IMX123)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_imx123.so", &sensor);
		//if(!strstr(hwinfo.devName, "N52-HS"))
		{
			mode = 0; //暂时不允许开启sensor WDR,只允许数字WDR

		}
	}
	else
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		return;
	}
	bWDREnable = mode > 0 ? TRUE:FALSE;
	s32Ret = SAMPLE_COMM_VI_StartMIPI(bWDREnable);
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
	if(pstWDRMode.enWDRMode ==WDR_MODE_NONE)
		jv_sensor_set_wdr_mode(FALSE);
	else
		jv_sensor_set_wdr_mode(TRUE);
}
//停止视频输入
static VOID ReleaseVI()
{
}

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
    		{704,      576     },
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

    //第二码流
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

    if (hwinfo.streamCnt >= 3)
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

    //抓图通道
    VpssChn = 4;
    stVpssExtChnAttr.s32BindChn = 1;
    stVpssExtChnAttr.s32SrcFrameRate = MAX_FRAME_RATE;
    stVpssExtChnAttr.s32DstFrameRate = 10;
    stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
    stVpssExtChnAttr.enCompressMode  = COMPRESS_MODE_NONE;
    stVpssExtChnAttr.u32Width        = resList[0][wi];//720;//resList[3][wi];;
    stVpssExtChnAttr.u32Height       = resList[0][hi];//576;//resList[3][hi];;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, HI_NULL, &stVpssExtChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
        return ;
    }

    //移动检测
    VpssChn = 5;
    stVpssExtChnAttr.s32BindChn = 1;
    stVpssExtChnAttr.s32SrcFrameRate = MAX_FRAME_RATE;
    stVpssExtChnAttr.s32DstFrameRate = MAX_FRAME_RATE;
    stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
    stVpssExtChnAttr.enCompressMode  = COMPRESS_MODE_NONE;
    stVpssExtChnAttr.u32Width        = 720;//resList[4][wi];;
    stVpssExtChnAttr.u32Height       = 576;//resList[4][hi];;
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
}

/*
读0x04地址处的数据：
int a;
a=0x04
ioctl(fd,0x80040005,&a)

写0x04地址处的数据：
int x;(高16位是要写的地址，低8位是要写的数据)
ioctl(fd,0x80040006,x)

*/
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
//498 bf57c2e6 490a7425 6fdcd49b 2e1e71cc 14c55 132df00 41 d800

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

/******************************************************************************
* 这里可以保证，程序退出后再运行时，能够正常运行
******************************************************************************/
void SAMPLE_VIO_HandleSig(HI_S32 signo)
{
	//if (SIGINT == signo || SIGTSTP == signo)
	{
		HI_MPI_SYS_Exit();
		HI_MPI_VB_Exit();
		printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
	}
	exit(-1);
}

void jv_common_wdr_switch(BOOL  bWDREnale) //1//  switch to wdr ,0-switch to linear
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
    s32Ret = SAMPLE_COMM_VI_StartMIPI(bWDREnale);
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
	if(bWDREnale)
	{
		if(sns_type == SENSOR_OV4689)
			stWDRAttr.enWDRMode=stWDRMode.enWDRMode = WDR_MODE_2To1_LINE;
		else if(sns_type == SENSOR_IMX123)
			stWDRAttr.enWDRMode=stWDRMode.enWDRMode = WDR_MODE_2To1_LINE;
		else if(sns_type == SENSOR_AR0237||sns_type == SENSOR_AR0237DC)
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
	 if(sns_type == SENSOR_IMX123)
	 	stWDRAttr.bCompress = HI_FALSE;

	 
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
	 return ;		
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

	signal(SIGINT, SAMPLE_VIO_HandleSig);
	signal(SIGTERM, SAMPLE_VIO_HandleSig);
	signal(SIGKILL, SAMPLE_VIO_HandleSig);
	stVbConf.u32MaxPoolCnt = 128;
	//74.5MB
	stVbConf.astCommPool[0].u32BlkSize	= JV_ALIGN_CEILING(VI_WIDTH, 64) * JV_ALIGN_CEILING(VI_HEIGHT, 64) * 1.5;
	stVbConf.astCommPool[0].u32BlkCnt	= 10;
	//6.4MB
	stVbConf.astCommPool[1].u32BlkSize	= JV_ALIGN_CEILING(1056, 64) * JV_ALIGN_CEILING(576, 64) * 1.5;
	stVbConf.astCommPool[1].u32BlkCnt	= 10;
	//1.8MB
	stVbConf.astCommPool[2].u32BlkSize	= JV_ALIGN_CEILING(512, 64) * JV_ALIGN_CEILING(288, 64) * 1.5;
	stVbConf.astCommPool[2].u32BlkCnt	= 10;

	if(hwinfo.encryptCode==ENCRYPT_200W)
	{
		stVbConf.astCommPool[0].u32BlkCnt	= 6;
		stVbConf.astCommPool[1].u32BlkCnt	= 5;
		stVbConf.astCommPool[2].u32BlkCnt	= 10;
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
	XDEBUG(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
	XDEBUG(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");

	ReleaseVI();

	sensor_ircut_deinit();
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

BOOL Sys_CheckSysVer()
{
    MPP_VERSION_S stMppVer;
    char FSMPPVER[VERSION_NAME_MAXLEN] = {"Hi3518_MPP_V1.0.9.0"};
    char LIBSMPPVER[VERSION_NAME_MAXLEN] = {"Hi3518_MPP_V1.0.9.0"};
    char *vstr;
    vstr = utl_fcfg_get_value("/home/ipc_drv/SDK.ver", "HI_VERSION");
    if (vstr)
    {
    	strncpy(FSMPPVER,vstr,strlen(vstr));
    }
    CHECK_RET(HI_MPI_SYS_GetVersion(&stMppVer));
    sscanf(stMppVer.aVersion, "HI_VERSION=%s", LIBSMPPVER);
    if (strcmp(FSMPPVER, LIBSMPPVER))
    {
    	printf("Invalid MPP version,please check your FileSystem.【FileSystem.MPP.Ver】:\"%s\",【Libs.MPP.Ver】:\"%s\"\n", FSMPPVER, LIBSMPPVER);
        return FALSE;
    }
    return TRUE;
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
