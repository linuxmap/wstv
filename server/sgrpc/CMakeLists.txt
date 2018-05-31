

INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/sport)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/libs/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/module/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/porting/include)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sctrl)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sport)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sgrpc)

INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sgrpc/grpc/)
INCLUDE_DIRECTORIES(${TOP_SOURCE_DIR}/server/sgrpc/grpc/generate/)

PROJECT(SGRPCLIB)

SET(SGRPCSRC 
		sgrpc.c
		cgrpc.c
		grpc/grpc.c
		grpc/platform.c
		grpc/generate/ipc.c
		grpc/generate/ipc_s_userdef.c
		grpc/generate/ipc_c_userdef.c
        )

ADD_LIBRARY(sgrpc STATIC 
		sgrpc.c
		cgrpc.c
		grpc/grpc.c
		grpc/platform.c
		grpc/generate/ipc.c
		grpc/generate/ipc_s_userdef.c
		grpc/generate/ipc_c_userdef.c
        )
 
