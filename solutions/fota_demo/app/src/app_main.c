/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <stdlib.h>
#include <string.h>
#include <aos/kernel.h>
#include <aos/list.h>
#include <aos/debug.h>
#include <ulog/ulog.h>
#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include <yoc/sysinfo.h>
#include <board.h>
#include "app_main.h"

#define TAG "appmain"

int main(void)
{
    board_yoc_init();
    app_fota_init();
    while(1) {
        aos_msleep(5000);
    }
}
