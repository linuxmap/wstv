INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include/tuya)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/module/include)

PROJECT(TUYALIB)

SET(TUYASRC
	tuya_ipc_dp_handler.c
	tuya_ipc_dp_utils.c
	tuya_ipc_media_handler.c
	tuya_ipc_media_utils.c
	tuya_ipc_mgr_handler.c
	tuya_ipc_mgr_utils.c
	tuya_linux_wifi.c
	cJSON_Direct.c
	tuya.c
	)

ADD_LIBRARY(tuya STATIC ${TUYASRC})
 
 
