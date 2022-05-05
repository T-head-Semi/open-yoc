/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "board.h"
#include <aos/kernel.h>
#include <stdio.h>
#include <ulog/ulog.h>
#include <drv/display.h>
#include <drv/g2d.h>

#define TAG "app"

enum {
    COLOR_RED = 0,
    COLOR_BLUE,
    COLOR_GREEN
};

extern void  cxx_system_init(void);
extern void board_yoc_init(void);

static void disp_set_bk_color(int color)
{
    struct disp_color bk;
    unsigned long arg[3];

    bk.red = 0x00;
    bk.green = 0x00;
    bk.blue = 0x00;
    arg[0] = 0;
    arg[1] = (unsigned long)&bk;
    switch (color)
    {
    case COLOR_RED:
        bk.red = 0xFF;
        break;
    case COLOR_BLUE:
        bk.blue = 0xFF;
        break;
    case COLOR_GREEN:
        bk.green = 0xFF;
        break;
    default:
        break;
    }
    csi_display_ioctl(DISP_SET_BKCOLOR, (void *)arg);
}

int main(int argc, char *argv[])
{
    static int g_color_index = 0;

    cxx_system_init();
    board_yoc_init();
    LOGI(TAG, "app start........\n");
    LOGI(TAG, "Display screen background color testing \n");
    while (1) {
        disp_set_bk_color(g_color_index++ % 3);
        aos_msleep(3000);
    };
}