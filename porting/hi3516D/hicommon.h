
#ifndef __HICOMMON_H__
#define __HICOMMON_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vpss.h"
#include "hi_comm_aio.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_isp.h"
#include "hifb.h"

#define	VI_DEVID			0		//预览
#define	VI_CHNID			0

extern int MAX_FRAME_RATE;	//某些sensor 30帧有问题，根据sensor修改。默认30帧

extern int VI_WIDTH;//			1280
extern int VI_HEIGHT;//			720

#define MAX_OSD_WINDOW		10

#define SAMPLE_PRT	Printf

#define JV_AIO_PAYLOAD_TYPE	PT_LPCM

typedef struct{
	int group;
	int bit;
}GpioValue_t;

typedef struct{
	GpioValue_t resetkey; //Reset按键
	GpioValue_t statusled; //状态指示灯
	GpioValue_t cutcheck; //夜视检测
	GpioValue_t cutday; //切白天
	GpioValue_t cutnight; //切晚上
	GpioValue_t redlight; //红外灯开关
	GpioValue_t alarmin1; //报警输入1
	GpioValue_t alarmin2; //报警输入2
	GpioValue_t alarmout1; //报警输出1
	GpioValue_t sensorreset; //sensor复位
	GpioValue_t audioOutMute;
	GpioValue_t pir; // PIR检测
}higpio_values_t;

extern higpio_values_t higpios;
#define GPIO 1

#endif

