

INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/tutk)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/mml/core)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/ivp)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/bizclient)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/bls)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/cloud)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/module/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/${BOARD}/include)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/porting/${BOARD})
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/gb28181)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/sctrl)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/osd_server)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/sport)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/sgrpc)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/telestore)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/bls)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/Tencent)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/tutk)
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/server/yst_svr)

PROJECT(MODULELIB)
set(MRTMP
	src/m_rtmp.c
	)

if(SD_RECORD_SUPPORT)
	set(MPLAY
		src/m_complayer.c
	)
endif()
if(XW_MMVA_SUPPORT)
	set(MMVASRC
		src/mmva.c
	)
endif()
SET(MODULESRC 
				src/maccount.c
				src/malarmin.c
				src/malarmout.c
				src/maudio.c
				src/mdetect.c
				src/mlog.c
				src/mosd.c
				src/mprivacy.c
				src/mrecord.c
				src/mstorage.c
				src/msensor.c
				src/msnapshot.c
				src/mstream.c
				src/mipcinfo.c
				src/mwdt.c
				src/mptz.c
				src/misp.c
				src/mplay_remote.c
				src/msoftptz
				src/mvoicedec
				src/mioctrl
				src/mivp.c
				src/mcloud.c
				src/m_mp4.c
				${MRTMP}
				src/mbizclient.c
				src/muartcomm.c
				src/mdebug.c
				src/mdevclient.c
				src/mfirmup.c
				src/mdooralarm.c
				${MPLAY}
				${MMVASRC}
        )

ADD_LIBRARY(module STATIC ${MODULESRC})
 
 
