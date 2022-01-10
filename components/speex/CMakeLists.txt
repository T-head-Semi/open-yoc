CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./include/speex)
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(./src)

ADD_DEFINITIONS(
    -DDISABLE_FLOAT_API
    -DFIXED_POINT
    -DUSE_KISS_FFT
)

SET(LIBSOURCE 
	src/cb_search.c
	src/exc_10_32_table.c
	src/exc_8_128_table.c
	src/filters.c
	src/gain_table.c
	src/hexc_table.c
	src/high_lsp_tables.c
	src/lsp.c
	src/ltp.c
	src/speex.c
	src/stereo.c
	src/vbr.c
	src/vq.c
	src/bits.c
	src/exc_10_16_table.c
	src/exc_20_32_table.c
	src/exc_5_256_table.c
	src/exc_5_64_table.c
	src/gain_table_lbr.c
	src/hexc_10_32_table.c
	src/lpc.c
	src/lsp_tables_nb.c
	src/modes.c
	src/modes_wb.c
	src/nb_celp.c
	src/quant_lsp.c
	src/sb_celp.c
	src/speex_callbacks.c
	src/speex_header.c
	src/window.c
)

ADD_LIBRARY(speex ${LIBSOURCE})

INSTALL(TARGETS speex DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


