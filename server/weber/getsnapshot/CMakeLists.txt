
PROJECT(GETSNAPSHOT)

SET(GETSNAPSHOTSRC 
					getsnapshot.c)

ADD_EXECUTABLE(getsnapshot.cgi ${GETSNAPSHOTSRC})

INSTALL (TARGETS getsnapshot.cgi
		RUNTIME DESTINATION html/cgi-bin/)
