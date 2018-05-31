
PROJECT(WAGENT)

SET(WAGENTSRC 
					wagent.c)


ADD_EXECUTABLE(wagent ${WAGENTSRC})
 
#TARGET_LINK_LIBRARIES(wagent	)

INSTALL (TARGETS wagent
         RUNTIME DESTINATION html/cgi-bin/)

