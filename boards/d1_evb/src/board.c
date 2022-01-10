/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <board.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/dma.h>
#include <sys_clk.h>
#include <drv/codec.h>

static void board_pinmux_config(void)
{
}

void board_pre_init(void)
{
}

void board_sound_init(void)
{
    csi_codec_t codec;
    csi_codec_init(&codec, 0);
}

void board_gpio_init(void)
{
    csi_gpio_t gpio_handler;

    soc_clk_init();
    csi_gpio_init(&gpio_handler, 0);
}

void board_dma_init(void)
{
    csi_dma_t dma;
    csi_dma_init(&dma, 0);
}

void board_init(void)
{
    board_pinmux_config();
    board_gpio_init();
    board_dma_init();
}
