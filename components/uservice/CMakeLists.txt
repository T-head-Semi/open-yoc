CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(../aos_port/include)
INCLUDE_DIRECTORIES(../ulog/include)

ADD_DEFINITIONS(
	-DCONFIG_USERVICE_EVENT_TASK_STACK_SIZE=2048
)

SET(LIBSOURCE 
	src/event.c
	src/event_svr.c
	src/rpc.c
	src/uservice.c
	src/utask.c
)

ADD_LIBRARY(uservice ${LIBSOURCE})

INSTALL(TARGETS uservice DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


