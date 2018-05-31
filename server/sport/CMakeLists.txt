

INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include/ivp)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/mml/core)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include/bls)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/module/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sctrl)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/yst_svr)

PROJECT(SPORTLIB)

SET(SPORTSRC 
		sp_audio
		sp_ivp.c
		sp_alarm.c
		sp_devinfo.c
		sp_ptz.c
		sp_rec.c

		sp_privacy.c
		sp_storage.c
		sp_user.c
		sp_ifconfig.c
		sp_image.c
		sp_osd.c
		sp_snapshot
		sp_stream.c
		
		sp_mdetect
		sp_connect
		
		sp_init.c
		sp_log.c
		sp_firmup.c
        )

ADD_LIBRARY(sport STATIC ${SPORTSRC})
 
 
