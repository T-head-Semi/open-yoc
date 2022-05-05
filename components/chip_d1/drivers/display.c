/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */
#include <drv/display.h>

extern int disp_probe(void);
extern int disp_ioctl(int cmd, void *arg);

int csi_display_init(void)
{
    return disp_probe();
}

int csi_display_ioctl(disp_cmd_e cmd, void *arg)
{
    return disp_ioctl((int)cmd, arg);
}