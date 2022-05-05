/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef _THEAD_KWS_H
#define _THEAD_KWS_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


int thead_kws_init(int channels, int sample_rate, size_t *wav_len, size_t *len);

int thead_kws_run(int16_t *wav, size_t wav_len, void *output_data, size_t len);

void thead_kws_postprocess(void *input_data, size_t len, int *output_data[2], size_t chanels);

#ifdef __cplusplus
}
#endif

#endif