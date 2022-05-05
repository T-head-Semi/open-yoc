/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     clk.c
 * @brief    CSI Source File for clk Driver
 * @version  V1.0
 * @date     9. April 2020
 ******************************************************************************/

#include <stdint.h>
#include <soc.h>
#include <csi_core.h>
#include <csi_config.h>
#include <drv/common.h>

extern csi_clkmap_t clk_map[];

void csi_clk_enable(csi_dev_t *dev)
{
    csi_clkmap_t *map = clk_map;

    while (map->module != 0xFFFFFFFFU) {
        if ((map->dev_tag == dev->dev_tag) &&
            (map->idx == dev->idx)) {
            soc_clk_enable((clk_module_t)map->module);
            break;
        }

        map++;
    }
}

void csi_clk_disable(csi_dev_t *dev)
{
    csi_clkmap_t *map = clk_map;

    while (map->module != 0xFFFFFFFFU) {
        if ((map->dev_tag == dev->dev_tag) &&
            (map->idx == dev->idx)) {
            soc_clk_disable((clk_module_t)map->module);
            break;
        }

        map++;
    }
}
