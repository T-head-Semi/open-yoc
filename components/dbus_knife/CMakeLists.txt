CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)

#extern include
INCLUDE_DIRECTORIES(../aos_port/include)
INCLUDE_DIRECTORIES(../ulog/include)

#ADD_DEFINITIONS(
#)

SET(LIBSOURCE 
	src/kdbus_buf.c
	src/kdbus_introspect.c
	src/kdbus_utils.c
)

ADD_LIBRARY(dbus_knife ${LIBSOURCE})

INSTALL(TARGETS dbus_knife DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


