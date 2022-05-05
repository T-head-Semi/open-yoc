/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */
#include <drv/g2d.h>

extern int g2d_probe(void);
extern int sunxi_g2d_control(int cmd, void *arg);
extern int sunxi_g2d_close(void);
extern int sunxi_g2d_open(void);

int csi_g2d_init(void)
{
    return g2d_probe();
}

int csi_g2d_open(void)
{
    return sunxi_g2d_open();
}

int csi_g2d_close(void)
{
    return sunxi_g2d_close();
}

int csi_g2d_ioctl(g2d_cmd cmd, void *arg)
{
    return sunxi_g2d_control(cmd, arg);
}