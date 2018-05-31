
#ifndef __HICOMMON_H__
#define __HICOMMON_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_aio.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_isp.h"

#define	VI_DEVID			0		//预览
#define	VI_CHNID			0

extern int MAX_FRAME_RATE;	//某些sensor 30帧有问题，根据sensor修改。默认30帧

extern int VI_WIDTH;//			1280
extern int VI_HEIGHT;//			720

#define MAX_OSD_WINDOW		20

#define SAMPLE_PRT	Printf

#define JV_AIO_PAYLOAD_TYPE	PT_LPCM

#endif

