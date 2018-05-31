
PROJECT(transfer)


ADD_EXECUTABLE(device_service transfer.c)

INSTALL (TARGETS device_service
         RUNTIME DESTINATION html/onvif/)

INSTALL (CODE "execute_process(COMMAND ln -sf device_service ${CMAKE_INSTALL_PREFIX}/html/onvif/Media)")
INSTALL (CODE "execute_process(COMMAND cp ${CMAKE_INSTALL_PREFIX}/html/onvif/device_service ${CMAKE_INSTALL_PREFIX}/html/cgi-bin/transfer_8127)")
