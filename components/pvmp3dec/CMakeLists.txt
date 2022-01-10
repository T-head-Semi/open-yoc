CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./include/pvmp3dec)

ADD_DEFINITIONS(
	"-DOSCL_UNUSED_ARG\\(x\\)=\\(void\\)\\(x\\)"
)

SET(LIBSOURCE 
	src/pvmp3_normalize.cpp
	src/pvmp3_alias_reduction.cpp
	src/pvmp3_crc.cpp
	src/pvmp3_decode_header.cpp
	src/pvmp3_decode_huff_cw.cpp
	src/pvmp3_getbits.cpp
	src/pvmp3_dequantize_sample.cpp
	src/pvmp3_framedecoder.cpp
	src/pvmp3_get_main_data_size.cpp
	src/pvmp3_get_side_info.cpp
	src/pvmp3_get_scale_factors.cpp
	src/pvmp3_mpeg2_get_scale_data.cpp
	src/pvmp3_mpeg2_get_scale_factors.cpp
	src/pvmp3_mpeg2_stereo_proc.cpp
	src/pvmp3_huffman_decoding.cpp
	src/pvmp3_huffman_parsing.cpp
	src/pvmp3_tables.cpp
	src/pvmp3_imdct_synth.cpp
	src/pvmp3_mdct_6.cpp
	src/pvmp3_dct_6.cpp
	src/pvmp3_poly_phase_synthesis.cpp
	src/pvmp3_equalizer.cpp
	src/pvmp3_seek_synch.cpp
	src/pvmp3_stereo_proc.cpp
	src/pvmp3_reorder.cpp
	src/pvmp3_polyphase_filter_window.cpp
	src/pvmp3_mdct_18.cpp
	src/pvmp3_dct_9.cpp
	src/pvmp3_dct_16.cpp
)

ADD_LIBRARY(pvmp3dec ${LIBSOURCE})

INSTALL(TARGETS pvmp3dec DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include/pvmp3dec)


