INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/ivp)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/mml/core)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/bizclient)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/cloud)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/module/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/weber)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/osd_server)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/gb28181)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/sport)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/media_svr)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/yst_svr)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/oct_svr)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/tuya)

PROJECT(SCTRL)

set (SYSLOG_LDFLAGS -Wl,-wrap,printf -Wl,-wrap,puts)

set (MEM_DEBUG	OFF)

if (MEM_DEBUG)
	set (MEMORYTEST_SRC 
		mem_hook.cpp
		mem_debug_queue.c
		linux_wrapper.c)
	set (MEMORYTEST_LDFLAGS -Wl,-wrap,malloc -Wl,-wrap,free -Wl,-wrap,realloc -Wl,-wrap,calloc)
	add_definitions(-DMEM_DEBUG=1)
endif()


SET(SCTRLSRC 
	main.c
	SYSFuncs.c
	sctrl.c
	${MEMORYTEST_SRC}
	${SYSLOG_SRC}
	)


#the libs.txt defined BOARD_LINK_DIRECTORIES and BOARD_LIBRARY for board.
INCLUDE (${IPCAM_SOURCE_DIR}/porting/${BOARD}/libs.txt)

LINK_DIRECTORIES(
	${IPCAM_SOURCE_DIR}/libs/${BOARD}
	${IPCAM_SOURCE_DIR}/libs/${CROSS_COMPILE}
	${BOARD_LINK_DIRECTORIES}
	)
LINK_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/Tencent)

ADD_EXECUTABLE(sctrl ${SCTRLSRC})

if (TINY_ONVIF_SUPPORT)
	set(TINY_ONVIF_LIBS tinyonvif tinyxml)
endif()

if (LIVE555_SUPPORT)
	set(JVLIVE_LIBS jvlive liveMedia groupsock BasicUsageEnvironment UsageEnvironment)
endif()

#音视实验室声波配置库支持
if (SOUND_WAVE_DEC_SUPPORT)
	SET(SOUND_WAVE_DEC_LIBS 
		sound_wave_dec
		)
endif()

if (GB28181_SUPPORT)
	SET(GB28181_LIBS 
		mgb28181
		gb28181
		eXosip2
		osip2
		osipparser2
		)
endif()

if (SD_RECORD_SUPPORT)
	SET(MP4LIBS
		Jmp4pkg
		)
			
endif()

if (ZRTSP_SUPPORT)
	set(ZRTSPLIBS
		zrtsp 
		parse
		cmv4 
		)
endif()

if (OPENSSL_ENABLE)
	set(SSLLIBS
		curl
		wolfssl
		crypto
		)
endif()

if(IVP_SUPPORT)
	set(IVPLIBS
		ivp
		)
endif()

if (TUYA_SUPPORT)
	set(TUYA_LIBS tuya_ipc AVAPIs IOTCAPIs)
	set(TUYA_SVR_LIB tuya)
endif()

set(SMARTCONFIG9083
	smartconfig
	pcap
	nl
	)

if(NOT ${BOARD} STREQUAL gk710xs)
set(SMARTCONFIG_MONT
	montconfig
	omnicfg
	)
endif()

if(DDNS_SUPPORT)
	set(DDNS
		Jvnddns
		)
endif()

if(BIZ_CLIENT_SUPPORT)
	set(BIZ_CLIENT
		bizdev
		)
endif()

set(JMC
	jmc
	)

if(OBSS_CLOUDSTORAGE)
	set(OBSS_CLOUD_STORAGE
		jhlsup_obss
		obss
		)
endif()

if(WEB_SUPPORT)
	set(WEBER
	    weber
	    )
endif()

if (YST_SVR_SUPPORT)
	set(YST_PROTOCOL_LIBS jvserver)
	set(YST_SVR_LIBS yst_svr)
endif()

TARGET_LINK_LIBRARIES(sctrl 
					${YST_SVR_LIBS}
					${WEBER}
					osd_server
					${GB28181_LIBS}
					${TINY_ONVIF_LIBS}
					sport 
					module 
					smtp
					${BLS}
					${BIZ_CLIENT}
					${ZRTSPLIBS}
					${IVPLIBS}
					${JVLIVE_LIBS}
					${SOUND_WAVE_DEC_LIBS}
					porting 
					WIFI
					#cJSON
					${TUYA_SVR_LIB}
					${TUYA_LIBS}
					${MP4LIBS}
					${SSLLIBS}
					utility
					nl80211
					${JMC}
					${YST_PROTOCOL_LIBS}
					${OBSS_CLOUD_STORAGE}
					#pool
					PTZ
					yst
					${DDNS}
					rt
					dl
					${SMARTCONFIG9083}
					${SMARTCONFIG_MONT}
					pthread
					stdc++
					${BOARD_LIBRARY}
					${MEMORYTEST_LDFLAGS}
					${SYSLOG_LDFLAGS}
					)

INSTALL (TARGETS sctrl
	RUNTIME DESTINATION bin)
