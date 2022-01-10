CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./src)
INCLUDE_DIRECTORIES(./src/libFLAC/include)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

ADD_DEFINITIONS(
    -DHAVE_CONFIG_H
    -DNDEBUG
    -DFLAC__USE_VISIBILITY_ATTR
    -D_FORTIFY_SOURCE=0
)

SET(LIBSOURCE 
	src/libFLAC/bitmath.c
	src/libFLAC/bitreader.c
	src/libFLAC/bitwriter.c
	src/libFLAC/cpu.c
	src/libFLAC/crc.c
	src/libFLAC/fixed.c
	src/libFLAC/float.c
	src/libFLAC/format.c
	src/libFLAC/lpc.c
	src/libFLAC/md5.c
	src/libFLAC/memory.c
	src/libFLAC/stream_decoder.c
	src/libFLAC/stream_encoder.c
	src/libFLAC/stream_encoder_framing.c
	src/libFLAC/window.c
	src/libFLAC++/metadata.cpp
	src/libFLAC++/stream_decoder.cpp
	src/libFLAC++/stream_encoder.cpp
)

ADD_LIBRARY(flac ${LIBSOURCE})

INSTALL(TARGETS flac DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


