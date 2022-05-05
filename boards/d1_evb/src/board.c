/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <board.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/dma.h>
#include <sys_clk.h>
#include <drv/codec.h>
#include <drv/rtc.h>
#include <soc.h>
#include <aos/cli.h>


csi_gpio_t chip_gpio_handler;
csi_rtc_t rtc_hdl;
csi_dma_t dma_hdl;

extern void hal_rtc_set_fel_flag();
static void efex_cmd(char *buf, int len, int argc, char **argv)
{
    hal_rtc_set_fel_flag();
}

static const struct cli_command cmd_xfex_specific = {
    /*cli self*/
    "xfex", "Enter xfex mode to burn image", efex_cmd
};

void  cli_reg_cmd_board_specific(void)
{
    aos_cli_register_command(&cmd_xfex_specific);
}

static void board_pinmux_config(void)
{
    // BT
    csi_pin_set_mux(PG6, PG6_UART1_TX);
    csi_pin_set_mux(PG7, PG7_UART1_RX);

    csi_pin_set_mux(PG8, PG8_UART1_RTS);
    csi_pin_set_mux(PG9, PG9_UART1_CTS);

    csi_pin_set_mux(PG18, PIN_FUNC_GPIO);
}

void board_sound_init(void)
{
    csi_codec_t codec;
    csi_codec_init(&codec, 0);
}

void board_clk_init(void)
{
    soc_clk_init();
    soc_clk_enable(BUS_UART1_CLK);
}

void board_init(void)
{
    board_clk_init();
    csi_rtc_init(&rtc_hdl, 0);
    csi_gpio_init(&chip_gpio_handler, 0);
    board_pinmux_config();
    csi_dma_init(&dma_hdl, 0);
}
