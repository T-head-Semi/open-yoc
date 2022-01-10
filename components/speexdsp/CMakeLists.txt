CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./include/speex)

ADD_DEFINITIONS(
	-DDB_DOWN_441_PATCH
    -DDISABLE_FLOAT_API
    -DFIXED_POINT
    -DUSE_KISS_FFT
)

SET(LIBSOURCE 
	src/preprocess.c
	src/jitter.c
	src/mdf.c
	src/fftwrap.c
	src/filterbank.c
	src/resample.c
	src/buffer.c
	src/scal.c
	src/kiss_fft.c
	src/kiss_fftr.c
)

ADD_LIBRARY(speexdsp ${LIBSOURCE})

INSTALL(TARGETS speexdsp DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


