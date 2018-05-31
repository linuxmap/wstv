

set(SDK_DIR $ENV{HOME}/hisi/hi3507/Hi3511_VSSDK_V1.1.2.3_SPC003)

INCLUDE_DIRECTORIES(./include)

INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include)

PROJECT(PORTINGLIB)
if(XW_MMVA_SUPPORT)
	set(PMVA
		jv_mva.c 
	)
endif()
SET(PORTINGSRC 
				jv_common.c
				jv_privacy.c
				jv_stream.c
				jv_osddrv.c
				jv_mdetect.c
				jv_snapshot.c
				jv_wdt.c
				jv_sensor.c
				jv_ai.c
				jv_ao.c
				jv_rs485.c
				3518_isp.c
				jv_isp.c
				jv_alarmin.c
				jv_gpio.c
  				jv_ptz
  				jv_dooralarm.c
  				jv_io.c
				jv_rtc.c
				jv_uartcomm.c
				jv_gpiocomm.c
				${PMVA}  				
        )

#test add macro definition
ADD_DEFINITIONS(-DTEST1 -DTEST2)

ADD_LIBRARY(porting STATIC ${PORTINGSRC})


