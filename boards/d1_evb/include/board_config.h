/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     board_config.h
 * @brief    header File for pin definition
 * @version  V1.0
 * @date     02. June 2020
 ******************************************************************************/

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <stdint.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_GETTIME_USE_TIMER_ID     0
#define CONSOLE_UART_IDX               0

#define WLAN_ENABLE_PIN     PG12
#define WLAN_POWER_PIN      0xFFFFFFFF

#define LED_PIN             PC1     // LED RGB

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_CONFIG_H_ */

