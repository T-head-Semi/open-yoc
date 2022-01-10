CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(../../cmake/toolchain.cmake)

# include
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./include/amrnb)
INCLUDE_DIRECTORIES(./src)
INCLUDE_DIRECTORIES(./src/common/include)
INCLUDE_DIRECTORIES(./src/dec/src)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes ")

ADD_DEFINITIONS(
	-DWMOPS=0
)

SET(LIBSOURCE 
	src/dec/src/amrdecode.cpp
	src/dec/src/a_refl.cpp
	src/dec/src/wmf_to_ets.cpp
	src/dec/src/d2_11pf.cpp
	src/dec/src/d_plsf.cpp
	src/dec/src/c_g_aver.cpp
	src/dec/src/d4_17pf.cpp
	src/dec/src/dec_amr.cpp
	src/dec/src/ec_gains.cpp
	src/dec/src/pstfilt.cpp
	src/dec/src/d2_9pf.cpp
	src/dec/src/if2_to_ets.cpp
	src/dec/src/agc.cpp
	src/dec/src/int_lsf.cpp
	src/dec/src/ex_ctrl.cpp
	src/dec/src/post_pro.cpp
	src/dec/src/lsp_avg.cpp
	src/dec/src/d3_14pf.cpp
	src/dec/src/bgnscd.cpp
	src/dec/src/dec_lag6.cpp
	src/dec/src/dec_lag3.cpp
	src/dec/src/d_plsf_3.cpp
	src/dec/src/dec_gain.cpp
	src/dec/src/d_plsf_5.cpp
	src/dec/src/d_gain_c.cpp
	src/dec/src/d1035pf.cpp
	src/dec/src/d8_31pf.cpp
	src/dec/src/sp_dec.cpp
	src/dec/src/dec_input_format_tab.cpp
	src/dec/src/b_cn_cod.cpp
	src/dec/src/preemph.cpp
	src/dec/src/qgain475_tab.cpp
	src/dec/src/d_gain_p.cpp
	src/dec/src/dtx_dec.cpp
	src/dec/src/ph_disp.cpp
	src/common/src/l_shr_r.cpp
	src/common/src/l_abs.cpp
	src/common/src/div_s.cpp
	src/common/src/negate.cpp
	src/common/src/mult_r.cpp
	src/common/src/grid_tbl.cpp
	src/common/src/copy.cpp
	src/common/src/ph_disp_tab.cpp
	src/common/src/q_plsf_3.cpp
	src/common/src/lsp_tab.cpp
	src/common/src/int_lpc.cpp
	src/common/src/sub.cpp
	src/common/src/lsp_lsf_tbl.cpp
	src/common/src/vad1.cpp
	src/common/src/round.cpp
	src/common/src/log2_norm.cpp
	src/common/src/inv_sqrt_tbl.cpp
	src/common/src/log2_tbl.cpp
	src/common/src/syn_filt.cpp
	src/common/src/q_plsf_5.cpp
	src/common/src/add.cpp
	src/common/src/gray_tbl.cpp
	src/common/src/norm_s.cpp
	src/common/src/az_lsp.cpp
	src/common/src/inv_sqrt.cpp
	src/common/src/gmed_n.cpp
	src/common/src/lsfwt.cpp
	src/common/src/sqrt_l_tbl.cpp
	src/common/src/log2.cpp
	src/common/src/q_plsf_3_tbl.cpp
	src/common/src/q_plsf_5_tbl.cpp
	src/common/src/bitreorder_tab.cpp
	src/common/src/gains_tbl.cpp
	src/common/src/shr_r.cpp
	src/common/src/l_deposit_l.cpp
	src/common/src/window_tab.cpp
	src/common/src/norm_l.cpp
	src/common/src/lsp_az.cpp
	src/common/src/weight_a.cpp
	src/common/src/pow2_tbl.cpp
	src/common/src/extract_l.cpp
	src/common/src/reorder.cpp
	src/common/src/pred_lt.cpp
	src/common/src/sqrt_l.cpp
	src/common/src/pow2.cpp
	src/common/src/q_plsf.cpp
	src/common/src/gc_pred.cpp
	src/common/src/div_32.cpp
	src/common/src/lsp_lsf.cpp
	src/common/src/residu.cpp
	src/common/src/lsp.cpp
	src/common/src/qua_gain_tbl.cpp
	src/common/src/bitno_tab.cpp
	src/common/src/bits2prm.cpp
	src/common/src/extract_h.cpp
	src/common/src/set_zero.cpp
	src/common/src/c2_9pf_tab.cpp
	src/common/src/shr.cpp
	src/common/src/l_deposit_h.cpp
)

ADD_LIBRARY(amrnb ${LIBSOURCE})

INSTALL(TARGETS amrnb DESTINATION ${INSTALL_PATH}/lib)
INSTALL(DIRECTORY ./include DESTINATION ${INSTALL_PATH}/include)


