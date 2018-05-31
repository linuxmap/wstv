
INCLUDE_DIRECTORIES(${IPCAM_SOURCE_DIR}/libs/include/netlink)

PROJECT(NL80211LIB)

SET(NL80211SRC 
				event.c
				genl.c
				reason.c
				reg.c
				status.c
				util.c
				iw_80211_scan.c
	)

ADD_LIBRARY(nl80211 STATIC ${NL80211SRC})
 

