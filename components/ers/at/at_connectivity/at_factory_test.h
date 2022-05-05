/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _AT_FACTORY_H
#define _AT_FACTORY_H

#include <uservice/eventid.h>

typedef int (*at_cmd_gpio_test)(void);

void user_at_wscan_handler(char *cmd, int type, char *data);
void user_at_wjoin_handler(char *cmd, int type, char *data);
void user_at_gpio_handler(char *cmd, int type, char *data);
void user_at_z_handler(char *cmd, int type, char *data);
void at_cmd_gpio_test_cb_reg(int (*cb)(void));

#endif