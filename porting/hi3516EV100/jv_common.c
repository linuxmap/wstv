#include "jv_common.h"
#include "hicommon.h"

#include <hi_sns_ctrl.h>
#include <jv_sensor.h>
#include <utl_filecfg.h>
#include "utl_ifconfig.h"
#include "3518_isp.h"
#include <mpi_ae.h>
#include <mpi_awb.h>
#include <mpi_af.h>
#include "hi_comm_isp.h"
#include "hi_comm_3a.h"
#include "hi_awb_comm.h"
#include "hi_mipi.h"
#include "hi_i2c.h"
#include "jv_spi_flash.h"
#include <termios.h>
#include "jv_gpio.h"
#include "jv_ai.h"
#include "jv_mdetect.h"

int flash_write_lock_fd = -1;

int MAX_FRAME_RATE = 30;
int VI_WIDTH = 1920;
int VI_HEIGHT = 1080;

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
	higpios.cutday.group = 5;
	higpios.cutday.bit = 2;
	jv_gpio_muxctrl(0x00D0, 0);//5_2
	jv_gpio_dir_set_bit(higpios.cutday.group, higpios.cutday.bit, 1);
	jv_gpio_write(higpios.cutday.group, higpios.cutday.bit, 0);

	higpios.cutnight.group = 5;
	higpios.cutnight.bit = 3;
	jv_gpio_muxctrl(0x00C8, 0);//5_3
	jv_gpio_dir_set_bit(higpios.cutnight.group, higpios.cutnight.bit, 1);
	jv_gpio_write(higpios.cutnight.group, higpios.cutnight.bit, 0);

	//WIFI上电
	utl_system("sync;echo 3 > /proc/sys/vm/drop_caches");
	jv_gpio_muxctrl(0x00CC, 0);//5_1
	jv_gpio_dir_set_bit(5, 1, 1);
	jv_gpio_write(5, 1, 1);

	//Sensor Reset
	higpios.sensorreset.group = 0;
	higpios.sensorreset.bit = 7;
	jv_gpio_muxctrl(0x0038, 0);//0_7
	jv_gpio_dir_set_bit(higpios.sensorreset.group, higpios.sensorreset.bit, 1);
	jv_gpio_write(higpios.sensorreset.group, higpios.sensorreset.bit, 0);
	usleep(100*1000);
	jv_gpio_write(higpios.sensorreset.group, higpios.sensorreset.bit, 1);
	usleep(100*1000);

    //IR LED
	higpios.redlight.group = 6;
	higpios.redlight.bit = 5;
	jv_gpio_muxctrl(0x0004, 0);//6_5
	jv_gpio_dir_set_bit(higpios.redlight.group, higpios.redlight.bit, 1);

	//reset key
	higpios.resetkey.group = 5;
	higpios.resetkey.bit = 0;
	jv_gpio_muxctrl(0x00D4, 0);//5_0
	jv_gpio_dir_set_bit(higpios.resetkey.group, higpios.resetkey.bit, 0);
	
	if(HWTYPE_MATCH(HW_TYPE_C8H))
	{
		//white light
		higpios.whitelight.group = 6;
		higpios.whitelight.bit = 6;
		jv_gpio_muxctrl(0x0008, 0);//6_6
		jv_gpio_dir_set_bit(higpios.whitelight.group, higpios.whitelight.bit, 1);
		jv_gpio_write(higpios.whitelight.group, higpios.whitelight.bit, 0);
	}
	else if(HWTYPE_MATCH(HW_TYPE_C3W))
	{
		//set led R
		higpios.statusledR.group = 6;
		higpios.statusledR.bit = 1;
		jv_gpio_muxctrl(0x0018, 0);
		jv_gpio_dir_set_bit(higpios.statusledR.group, higpios.statusledR.bit, 1);
		jv_gpio_write(higpios.statusledR.group, higpios.statusledR.bit, 1);

		//connect led B
		higpios.statusledB.group = 1;
		higpios.statusledB.bit = 6;
		jv_gpio_muxctrl(0x0058, 0);
		jv_gpio_dir_set_bit(higpios.statusledB.group, higpios.statusledB.bit, 1);	
		jv_gpio_write(higpios.statusledB.group, higpios.statusledB.bit, 1);
	}
}

static void __checkHwType()
{
	if (PRODUCT_MATCH(PRODUCT_C8S)
		|| PRODUCT_MATCH(PRODUCT_V8S))
	{
		hwinfo.HwType = HW_TYPE_C8S;
	}
	else if(PRODUCT_MATCH(PRODUCT_C8H))
	{
		hwinfo.HwType = HW_TYPE_C8H;
	}
	else if(PRODUCT_MATCH(PRODUCT_C3W))
	{
		hwinfo.HwType = HW_TYPE_C3W;
	}
}

void hwinfo_init()
{
	memset(&hwinfo, 0, sizeof(hwinfo));
    strcpy(hwinfo.type,"Hi3516EV100");
	hwinfo.ptzBsupport = FALSE;
	hwinfo.ptzbaudrate = 2400;
	hwinfo.ptzprotocol = 29; //标准 PELCO-D (BandRate = 2400)
    hwinfo.wdrBsupport=TRUE;
    hwinfo.rotateBSupport=TRUE;
    hwinfo.encryptCode = ENCRYPT_200W;
    hwinfo.streamCnt = 2;
    hwinfo.bHomeIPC = 1;
	hwinfo.bSupportVoiceConf = TRUE;	// 默认支持声波配置
	hwinfo.bNewVoiceDec = TRUE;
	hwinfo.bXWNewServer = TRUE;			// 默认走小维报警服务
	hwinfo.bSupportXWCloud = TRUE;
	hwinfo.bSupportAVBR = TRUE;
	hwinfo.bSupportSmtVBR = TRUE;

    hwinfo.ipcType = stJvParam.ipcType;
    hwinfo.ir_sw_mode = IRCUT_SW_BY_AE;
    hwinfo.ir_power_holdtime = 300*1000;

    strcpy(hwinfo.product, "Unknown");
    utl_fcfg_get_value_ex(JOVISION_CONFIG_FILE, "product", hwinfo.product, sizeof(hwinfo.product));
	utl_fcfg_get_value_ex(CONFIG_HWCONFIG_FILE, "product", hwinfo.devName, sizeof(hwinfo.devName));

	// 检查硬件型号
	__checkHwType();

	// 音频模式选择，默认为双向对讲
	if (HWTYPE_MATCH(HW_TYPE_C8S))
	{
		// 按测试部建议，默认保留音频和对讲
		// hwinfo.remoteAudioMode = AUDIO_MODE_NO_WAY;
		hwinfo.remoteAudioMode = AUDIO_MODE_TWO_WAY;
	}
	else
	{
		hwinfo.remoteAudioMode = AUDIO_MODE_TWO_WAY;
	}

	// 云台
	if (HWTYPE_MATCH(HW_TYPE_C3W))
	{
		hwinfo.bSoftPTZ = 1;
		hwinfo.bSupport3DLocate = 1;
	}

	if (PRODUCT_MATCH(PRODUCT_V8S))
	{
		gp.bNeedOnvif = TRUE;
		gp.bNeedZrtsp = TRUE;
		gp.bNeedWeb = TRUE;
	}
	else
	{
		gp.bNeedYST = TRUE;
	}

	if(PRODUCT_MATCH(PRODUCT_C8H))
	{
		hwinfo.bSupportRedWhiteCtrl = TRUE;
	}

	if(PRODUCT_MATCH(PRODUCT_C3W))
	{
		hwinfo.bSupportMCU433 = TRUE;
		hwinfo.bCloudSee = TRUE;
	}

	// 使用AP的设备把声波关掉，以免影响AP下音频
	if (utl_ifconfig_bsupport_apmode(utl_ifconfig_wifi_get_model()))
	{
		hwinfo.bNewVoiceDec = FALSE;
		hwinfo.bSupportVoiceConf = FALSE;
	}
#if 0
	if (PRODUCT_MATCH(PRODUCT_C8H))
	{
		hwinfo.bSupportMVA = TRUE;
		hwinfo.bSupportReginLine = TRUE;
		hwinfo.bSupportLineCount = TRUE;
		hwinfo.bSupportHideDetect = TRUE;
		hwinfo.bSupportClimbDetect =TRUE;
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

    __gpio_init();
}

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
	    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,
		//VI_VSYNC_FIELD, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,

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
	VI_DATA_TYPE_RGB,
	/* bRever */
	HI_FALSE,
	/* DEV CROP */
	{0, 0, 1920, 1080}
	
};

/*OV2735 MIPI 10bit输入*/
VI_DEV_ATTR_S DEV_ATTR_OV2735_MIPI_1080P =
/* 典型时序3:7441 BT1120 1080P@30fps典型时序 (对接时序: 时序)*/
{
   /*接口模式*/
	VI_MODE_MIPI,
	/*1、2、4路工作模式*/
	VI_WORK_MODE_1Multiplex,
	/* r_mask    g_mask    b_mask*/
	{0xFFC00000,    0x0},
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



VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE =
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
    { -1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

    /* synchronization information */
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
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

combo_dev_attr_t MIPI_2lane_SENSOR_MN34227_12BIT_NOWDR_ATTR =
{
    .input_mode = INPUT_MODE_MIPI,
    {
        .mipi_attr =    
        {
            RAW_DATA_12BIT,
			HI_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1}
        }
    }
};

combo_dev_attr_t MIPI_4lane_SENSOR_IMX290_12BIT_1080_NOWDR_ATTR = 
{
	.devno = 0,
	.input_mode = INPUT_MODE_MIPI, 
	{
		.mipi_attr = 
		{
			RAW_DATA_12BIT,
			HI_MIPI_WDR_MODE_NONE,
			{0, 1, 2, 3}
		}
	}
};

combo_dev_attr_t DVP_SENSOR_OV2735_10BIT_2M_NOWDR_ATTR = 
{
    .input_mode = INPUT_MODE_CMOS,  
    {

    }    
};

combo_dev_attr_t MIPI_2lane_SENSOR_OV2735_10BIT_NOWDR_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    {
        .mipi_attr =    
        {
            RAW_DATA_10BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1}
        }
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

	if(sns_type == SENSOR_MN34227)
	{
		pstcomboDevAttr = &MIPI_2lane_SENSOR_MN34227_12BIT_NOWDR_ATTR;
	}
	else if (sns_type == SENSOR_IMX290)
	{
		pstcomboDevAttr = &MIPI_4lane_SENSOR_IMX290_12BIT_1080_NOWDR_ATTR;//根据sample修改
	}
	else if(sns_type == SENSOR_OV2735)
	{
		pstcomboDevAttr = &DVP_SENSOR_OV2735_10BIT_2M_NOWDR_ATTR;//DVP_SENSOR_OV2735_10BIT_2M_NOWDR_ATTR;
	}
	else 
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		close(fd);
		return -1;
	}
	
    /* 1. reset mipi */
    ioctl(fd, HI_MIPI_RESET_MIPI, &pstcomboDevAttr->devno);

    /* 2. reset sensor */
    ioctl(fd, HI_MIPI_RESET_SENSOR, &pstcomboDevAttr->devno);

    /* 3. set mipi attr */
    if (ioctl(fd, HI_MIPI_SET_DEV_ATTR, pstcomboDevAttr))
    {
        printf("set mipi attr failed\n");
        close(fd);
        return HI_FAILURE;
    }

	usleep(10000);

    /* 4. unreset mipi */
    ioctl(fd, HI_MIPI_UNRESET_MIPI, &pstcomboDevAttr->devno);

    /* 5. unreset sensor */
    ioctl(fd, HI_MIPI_UNRESET_SENSOR, &pstcomboDevAttr->devno);
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
	HI_U16 frm = 20;

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
	if(WdrEnble&&(sns_type == SENSOR_AR0230))
		stWdrMode.enWDRMode  = WDR_MODE_BUILT_IN;
	s32Ret = HI_MPI_ISP_SetWDRMode(0, &stWdrMode);
	if (HI_SUCCESS != s32Ret)
	{
		printf("start ISP WDR failed!\n");
		return s32Ret;
	}

	/* 7. isp set pub attributes */
	if(sns_type == SENSOR_MN34227)
	{
		stPubAttr.enBayer               = BAYER_GRBG;
		stPubAttr.f32FrameRate          = frm;
		stPubAttr.stWndRect.s32X        = 0;
		stPubAttr.stWndRect.s32Y        = 0;
		stPubAttr.stWndRect.u32Width    = VI_WIDTH;
		stPubAttr.stWndRect.u32Height   = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV2735)
	{
		stPubAttr.enBayer             = BAYER_BGGR;
		stPubAttr.f32FrameRate        = frm;
		if(VI_CROP_ENABLE)
		{
			stPubAttr.stWndRect.s32X        = VI_CROP_X;
			stPubAttr.stWndRect.s32Y        = VI_CROP_Y;
			stPubAttr.stWndRect.u32Width    = VI_CROP_W;
			stPubAttr.stWndRect.u32Height   = VI_CROP_H;
		}
		else
		{
			stPubAttr.stWndRect.s32X        = 0;
			stPubAttr.stWndRect.s32Y        = 0;
			stPubAttr.stWndRect.u32Width    = VI_WIDTH;
			stPubAttr.stWndRect.u32Height   = VI_HEIGHT;
		}
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
	JV_ISP_COMM_Set_LowFps(frm);

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

    if(sns_type == SENSOR_MN34227)
	{
		devAttr = &DEV_ATTR_MN34227_MIPI_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_IMX290)
	{
		devAttr = &DEV_ATTR_MIPI_BASE;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;
	}
	else if(sns_type == SENSOR_OV2735)
	{
		devAttr = &DEV_ATTR_OV2735_DC_1080P;//DEV_ATTR_OV2735_DC_1080P;
		devAttr->stDevRect.s32X = 0;
		devAttr->stDevRect.s32Y = 0;
		devAttr->stDevRect.u32Width  = VI_WIDTH;//VI_WIDTH;
		devAttr->stDevRect.u32Height = VI_HEIGHT;//VI_HEIGHT;
	}
	else
	{
		printf("%s: sensor type not supported\n", __FUNCTION__);
		return HI_FAILURE;
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

	for(i=0; i<HWINFO_STREAM_CNT; i++)
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

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM && VpssChn != 2)
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

#if 1
	if (VpssChn == 0)
	{
		VPSS_LOW_DELAY_INFO_S	LowDelay;
		HI_MPI_VPSS_GetLowDelayAttr(VpssGrp, VpssChn, &LowDelay);
		LowDelay.bEnable = HI_TRUE;
		LowDelay.u32LineCnt = 16;
		s32Ret = HI_MPI_VPSS_SetLowDelayAttr(VpssGrp, VpssChn, &LowDelay);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VPSS_SetLowDelayAttr failed with %#x!\n", s32Ret);
	        return HI_FAILURE;
	    }

		printf("=====Enable low delay on vpss channel %d=====\n", VpssChn);
	}
#endif

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
	char *fname = CONFIG_PATH"/sensor.sh";
	//isp_ioctl(0,GET_ID,(unsigned long)&sensor);
	//printf("====>>. sensor: %d\n", sensor);
	if (access(fname, F_OK) == 0)
	{
		str = utl_fcfg_get_value(fname, "SENSOR");
		if (str != NULL)
		{
		    if (strcmp(str, "mn34227") == 0)
			{
				sns_type = SENSOR_MN34227;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
				hwinfo.ir_sw_mode=IRCUT_SW_BY_ISP;
				MAX_FRAME_RATE = 20;
			}
			else if (strcmp(str, "imx290") == 0)
			{
				sns_type = SENSOR_IMX290;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
				hwinfo.ir_sw_mode=IRCUT_SW_BY_ISP;
			}
			else if (strcmp(str, "ov2735") == 0)
			{
				sns_type = SENSOR_OV2735;
				VI_WIDTH = 1920;
				VI_HEIGHT = 1080;
				hwinfo.encryptCode = ENCRYPT_200W;
				hwinfo.ir_sw_mode=IRCUT_SW_BY_ISP;
				MAX_FRAME_RATE = 20;
				
				VI_CROP_ENABLE = FALSE;
				VI_CROP_W = 1920 -80 -80;
				VI_CROP_H = 1080;
				VI_CROP_X = 80;
				VI_CROP_Y =0;
			}
			else
			{
				Printf("Failed Find sensor\n");
				sns_type = (unsigned int) -1;
				unlink(fname);
			}
			if (sns_type != SENSOR_MAX)
			{
				sensor = sns_type; 
				isp_ioctl(0,GET_ID,(unsigned long)&sensor);
				if(sensor == sns_type)
				{
					utl_fcfg_close(fname);
					//hwinfo.type 统一格式，统一处理：芯片名称-主板类型-分辨率：H8E-P-10：HI3518E-38板-100W
					{
						memset(hwinfo.type, 0, sizeof(hwinfo.type));
						strncpy(hwinfo.type, "H6EV100-", strlen("H6EV100-"));
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
			sensor = sns_type = SENSOR_MAX;
		}
	}
	else
	{
		printf("check sensor open file[%s] failed!\n", fname);
		sensor = sns_type = SENSOR_MAX;
	}
	if(SENSOR_MAX == sns_type)
	{
		isp_ioctl(0,GET_ID,(unsigned long)&sensor);
		printf("====>>. sensor: %d\n", sensor);
	}
	{
		if (sensor == SENSOR_MN34227)
		{
			sns_type = SENSOR_MN34227;
			cmd = "SENSOR=mn34227";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_IMX290)
		{
			sns_type = SENSOR_IMX290;
			cmd = "SENSOR=imx290";
			hwinfo.encryptCode = ENCRYPT_200W;
		}
		else if (sensor == SENSOR_OV2735)
		{
			sns_type = SENSOR_OV2735;
			cmd = "SENSOR=ov2735";
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
	int (*sensor_unregister_callback)(void);
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
	sensor->sensor_unregister_callback = dlsym(handle,"sensor_unregister_callback");
	bGet =1;
	if (sensor->sensor_init && sensor->cmos_set_wdr_mode && sensor->sensor_register_callback && sensor->cmos_fps_set_temporary)
		return 0;
	printf("one or more func is null:\n %p, %p, %p, %p\n", sensor->sensor_init, sensor->cmos_set_wdr_mode, sensor->sensor_register_callback,sensor->sensor_unregister_callback);
	return -1;
}

#define SENSOR_SO_BASE_DIR "/home/sensor"

//////////////////////////////////视频输入//////////////////////////////////
static _SensorPtr_t sensor;

static VOID InitVI(int mode)
{
	S32 s32Ret;
	if(sns_type == SENSOR_MN34227)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_mn34227.so", &sensor);
	}
	else if(sns_type == SENSOR_IMX290)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_imx290.so", &sensor);
	}
	else if(sns_type == SENSOR_OV2735)
	{
		s32Ret = _get_sensor_ptr(SENSOR_SO_BASE_DIR"/libsns_ov2735.so", &sensor);
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
    		{160,      120     },
    		{400,      224     },
    };

	if(VI_CROP_ENABLE)
	{
		resList[0][0]= VI_CROP_W;
		resList[0][1]= VI_CROP_H;
    };

    int wi = 0;
    int hi = 1;

	VPSS_MOD_PARAM_S VPSS_MODPARA;
	HI_MPI_VPSS_GetModParam(&VPSS_MODPARA);
	VPSS_MODPARA.bOneBufForLowDelay = HI_TRUE;
	s32Ret = HI_MPI_VPSS_SetModParam(&VPSS_MODPARA);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetModParam failed with %#x!\n", s32Ret);
        return ;
    }

    VpssGrp = 0;
	stVpssGrpAttr.u32MaxW = resList[0][wi];
	stVpssGrpAttr.u32MaxH = resList[0][hi];
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
	stVpssGrpAttr.bSharpenEn = HI_FALSE;
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

	//IVE_MD
    VpssChn = 2;
	stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
	stVpssChnMode.u32Width = resList[2][wi];
	stVpssChnMode.u32Height = resList[2][hi];
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
	s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Enable vpss chn %d failed!\n", VpssChn);
		return;
	}

    //抓图通道
    VpssChn = 3;
    stVpssExtChnAttr.s32BindChn = 1;
    stVpssExtChnAttr.s32SrcFrameRate = MAX_FRAME_RATE;
    stVpssExtChnAttr.s32DstFrameRate = 5;
    stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
    stVpssExtChnAttr.enCompressMode  = COMPRESS_MODE_NONE;
    stVpssExtChnAttr.u32Width        = resList[3][wi];
	stVpssExtChnAttr.u32Height       = resList[3][hi];
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
		stVpssExtChnAttr.u32Width = VI_CROP_W;
		stVpssExtChnAttr.u32Height =VI_CROP_H;
		
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, HI_NULL, &stVpssExtChnAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			return;
		}
	}

}

unsigned char HW_ReadEncrypt(unsigned char addr)
{
	int fd = -1;
	int ret = 0;
	struct i2c_rdwr_ioctl_data rdwr;
	struct i2c_msg msg[2];
	unsigned int data = 0;
	unsigned char buf[4] = {0};

	memset(&rdwr, 0, sizeof(rdwr));
	memset(&msg, 0, sizeof(msg));
	
	fd = open("/dev/i2c-1", O_RDONLY);
	if(fd < 0)
	{
		printf("Open /dev/i2c-1 error!\n");
		return -1;
	}
	ret = ioctl(fd, I2C_SLAVE_FORCE, 0x50);
	if (ret < 0)
	{
		printf("CMD_SET_DEV error!\n");
		close(fd);
		return ret;
	}	
	
	msg[0].addr = 0x50;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;
	
	msg[1].addr = 0x50;
	msg[1].flags = 0;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;
	
	rdwr.msgs = &msg[0];
	rdwr.nmsgs = (__u32)2;
	buf[0] = addr & 0xff;
	
	ret = ioctl(fd, I2C_RDWR, &rdwr);
	
	if (ret != 2) {
		printf("CMD_I2C_READ error!,%d\n",ret);
		close(fd);
		return ret;
	}
	
	data = buf[0];
	
	//printf("0x%x 0x%x\n", addr, data);
	
	close(fd);
	return data;
}

/*
 *@brief DEBUG用的yst库
 */
unsigned int jv_yst(unsigned int *a,unsigned int b)
{
#if 0
	int i;
	unsigned int data[] = {0xbf57c2e6 ,0x490a7425 ,0x6fdcd49b ,0x2e1e71cc ,0x14c55 ,0x132df00 ,0x41 ,0xd800};
	for (i=0;i<8;i++)
	{
		a[i]=data[i];
	}
	return 1;
#endif
	return 0;
}

static void __exit_platform()
{
	jv_mdetect_deinit();
	sensor_ircut_deinit();
	SAMPLE_COMM_VI_StopIsp();
	
	SAMPLE_COMM_VPSS_StopGroup(0);

	jv_ai_stop(0);
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
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

	signal(SIGINT, SAMPLE_VIO_HandleSig);
	signal(SIGTERM, SAMPLE_VIO_HandleSig);
	signal(SIGKILL, SAMPLE_VIO_HandleSig);

	stVbConf.u32MaxPoolCnt = 32;

	stVbConf.astCommPool[0].u32BlkSize	= JV_ALIGN_CEILING(VI_WIDTH, 64) * JV_ALIGN_CEILING(VI_HEIGHT, 4) * 1.5 + 
		VB_HEADER_STRIDE * VI_HEIGHT * 3 / 2;
	stVbConf.astCommPool[0].u32BlkCnt	= 2;
 
	stVbConf.astCommPool[1].u32BlkSize	= JV_ALIGN_CEILING(512, 64) * JV_ALIGN_CEILING(288, 4) * 1.5 +
		VB_HEADER_STRIDE * 288 * 3 / 2;
	stVbConf.astCommPool[1].u32BlkCnt	= 3;

	stVbConf.astCommPool[2].u32BlkSize	= JV_ALIGN_CEILING(160, 64) * JV_ALIGN_CEILING(120, 4) * 1.5 +
		VB_HEADER_STRIDE * 120 * 3 / 2;
	stVbConf.astCommPool[2].u32BlkCnt	= 2;

	stVbConf.astCommPool[3].u32BlkSize	= JV_ALIGN_CEILING(400, 64) * JV_ALIGN_CEILING(224, 4) * 1.5 +
		VB_HEADER_STRIDE * 224 * 3 / 2;
	stVbConf.astCommPool[3].u32BlkCnt	= 2;

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

	sensor_ircut_deinit();
	return 0;
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

