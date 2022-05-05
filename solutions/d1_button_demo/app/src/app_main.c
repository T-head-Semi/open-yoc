/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "board.h"
#include <aos/kernel.h>
#include <stdio.h>
#include <ulog/ulog.h>
#include <drv/lradc.h>

#define TAG "app"

extern void  cxx_system_init(void);
extern void board_yoc_init(void);

static void lradc_irq_callback(uint32_t irq_status, uint32_t data)
{
    // Here, we only catpure key button's up IRQ to confirm user press key one times
    if (irq_status & LRADC_ADC0_UPPEND) {
        printf("user key is pressed one times\n");
    }
}

static int test_lradc(void)
{
    int ret = -1;

    printf("Run lradc test\n");

    ret = csi_lradc_init();
    if (ret) {
        printf("lradc init failed!\n");
        return -1;
    }

    csi_lradc_register_callback(lradc_irq_callback);

    return 0;
}

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    test_lradc();
    while (1) {
        aos_msleep(3000);
    };
}