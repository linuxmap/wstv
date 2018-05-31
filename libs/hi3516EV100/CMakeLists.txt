
INSTALL(PROGRAMS 
		thttpd
		DESTINATION bin)
INSTALL(FILES 
		DESTINATION lib)

INSTALL(PROGRAMS
		startup.sh
		networkcfg.sh
		DESTINATION .)
