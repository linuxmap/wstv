
if (TINY_ONVIF_SUPPORT)

	SET(SMTP
		no-ssl-smtp/smtp)
else()
	SET(SMTP
		smtp)
endif()

INSTALL(PROGRAMS 
		#${SMTP}
		#smtp
		thttpd
		DESTINATION bin)
INSTALL(FILES 
		DESTINATION lib)

INSTALL(PROGRAMS
		startup.sh
		networkcfg.sh
		DESTINATION .)
