
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/module/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sctrl)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include/bls)

PROJECT(BLSLIB)

SET(BLSSRC 
		bls.c
		pcs.c
		#AmfSerializer.cpp
        )

ADD_LIBRARY(bls STATIC ${BLSSRC})
 
 
