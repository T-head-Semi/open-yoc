/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef __LEDC_H__
#define __LEDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sunxi_hal_ledc.h"

void csi_ledc_init(void);
void csi_ledc_deinit(void);
void csi_ledc_trans_data(struct ledc_config *ledc);
void csi_ledc_clear_all_irq(void);
unsigned int csi_ledc_get_irq_status(void);
void csi_ledc_dma_callback(void *para);
void csi_ledc_reset(void);

#ifdef __cplusplus
}
#endif

#endif