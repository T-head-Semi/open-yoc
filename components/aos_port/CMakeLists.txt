CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)

#ADD_DEFINITIONS(
#	"-DOSCL_UNUSED_ARG\\(x\\)=\\(void\\)\\(x\\)"
#)

SET(LIBSOURCE 
	src/list.c
	src/port.c
)

ADD_LIBRARY(aos_port ${LIBSOURCE})

INSTALL(TARGETS aos_port DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


