
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/module/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/sport)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/Tencent)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/sctrl)
PROJECT(UTILITY)

SET(UTILITYSRC
	#md5.c
	utl_audio
	utl_cmd.c
	utl_common.c
	utl_timer.c
	utl_ifconfig.c
	utl_iconv.c
	utl_filecfg.c
	utl_ipscan.c
	utl_mutex.c
	utl_list.c
	webproxy.c
	utl_system
	utl_scale
	utl_queue
	utl_crc
	httpclient.c
	utl_inifile.c
	ftp_client.c
	utl_jpg2bmp.c
	utl_splice.c
	utl_thread.c
	utl_algorithm.c
	utl_aes.c
	utl_base64.cpp
	utl_map.cpp
	)

ADD_LIBRARY(utility STATIC ${UTILITYSRC})
TARGET_LINK_LIBRARIES(utility
	WIFI
	)

ADD_EXECUTABLE(mySystem mySystem.c)
TARGET_LINK_LIBRARIES(mySystem
	-Wl,-wrap,printf -Wl,-wrap,puts
	rt
	pthread
	)

ADD_EXECUTABLE(webproxy webproxy_test.c)
TARGET_LINK_LIBRARIES(webproxy
	rt
	pthread
	utility
	)
IF(BOARD STREQUAL gk710xs)
ADD_EXECUTABLE(writeflash writeflash.c)
TARGET_LINK_LIBRARIES(writeflash
	utility
	rt
	pthread
	)
INSTALL (TARGETS writeflash
	RUNTIME DESTINATION bin)
endif()

INSTALL (TARGETS webproxy mySystem
	RUNTIME DESTINATION bin)
