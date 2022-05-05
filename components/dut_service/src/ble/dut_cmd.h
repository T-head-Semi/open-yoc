/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#ifndef __DUT_CMD_BLE_H_
#define __DUT_CMD_BLE_H_

#include "dut_service.h"

typedef enum {
    /* gpio mode config */
    AUTO_MODE = 0,
    WRITE_MODE = 1,
    READ_MODE = 2,
} dut_gpio_cfg_e;
int dut_cmd_tx_single_tone(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_tx_mod_burst(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_rx_demod_burst(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_sleep(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_opt_mac(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_xtal_cap(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_transmit_stop(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_rx_current_test(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_freqoff(dut_cmd_type_e type, int argc, char *argv[]);

int dut_cmd_gpio_test(dut_cmd_type_e type, int argc, char *argv[]);
#endif
