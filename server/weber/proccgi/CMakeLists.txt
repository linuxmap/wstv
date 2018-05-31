
PROJECT(PROCCGI)

SET(PROCCGISRC 
					proccgi.c)


ADD_EXECUTABLE(proccgi ${PROCCGISRC})
 
INSTALL (TARGETS proccgi
         RUNTIME DESTINATION html/cgi-bin/)

