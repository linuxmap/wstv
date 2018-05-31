
PROJECT(WIFILIB)

SET(WIFISRC 
				iwlib.c
				iwlist.c
	)

ADD_LIBRARY(WIFI STATIC ${WIFISRC})
 

