/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#ifndef __DUT_HAL_COMMON_H_
#define __DUT_HAL_COMMON_H_

int dut_hal_sleep(uint8_t mode);

int dut_hal_mac_get(uint8_t addr[6]);

int dut_hal_mac_store(uint8_t addr[6]);

/* API for factory data read/write */
#ifndef CONFIG_USER_FACTORY_DATA_SIZE
#define CONFIG_USER_FACTORY_DATA_SIZE 256
#endif

/**************************************************************************************
 * @fn          dut_hal_factorydata_read
 *
 * @brief       This function process for read data from user's factory data
 *
 * input parameters
 *
 * @param
 *              offset    :   offset
 *              buffer    :   data
 *              length    :   data length
 *
 * output parameters
 *
 * @param       none
 *
 * @return      Success:0 ; Fail: < 0
 */
int dut_hal_factorydata_read(uint32_t offset, uint8_t *buffer, uint32_t length);

/**************************************************************************************
 * @fn          dut_hal_factorydata_store
 *
 * @brief       This function process for store user's factory data
 *
 * input parameters
 *
 * @param
 *              offset    :   offset
 *              buffer    :   data
 *              length    :   data length
 *
 * output parameters
 *
 * @param       none
 *
 * @return      Success:0 ; Fail: < 0
 */
int dut_hal_factorydata_store(uint32_t offset, uint8_t *buffer, uint32_t length);

int dut_hal_test_gpio_read(uint8_t wrgpio, uint8_t *value);

int dut_hal_test_gpio_write(uint8_t wrgpio, uint8_t value);

int dut_hal_rx_current_test(uint32_t sleep_time);
#endif
