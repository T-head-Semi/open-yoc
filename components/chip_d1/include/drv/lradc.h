/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef __DRV_LRADC_H__
#define __DRV_LRADC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*lradc_callback_t)(uint32_t irq_status, uint32_t data);

#define LRADC_ADC0_UPPEND       (1<<4)
#define LRADC_ADC0_DOWNPEND     (1<<1)
#define LRADC_ADC0_DATAPEND     (1<<0)

/*****************************************************************************
 * Enums
 *****************************************************************************/
typedef enum{
	LRADC_STATUS_ERROR_PARAMETER = -3,
	LRADC_STATUS_ERROR_CHANNEL = -2,
	LRADC_STATUS_ERROR = -1,
	LRADC_STATUS_OK = 0
} lradc_status_t;

/*****************************************************************************
 * Functions
 *****************************************************************************/
lradc_status_t csi_lradc_init(void);

lradc_status_t csi_lradc_deinit(void);

lradc_status_t csi_lradc_register_callback(lradc_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /*__DRV_LRADC_H__*/
