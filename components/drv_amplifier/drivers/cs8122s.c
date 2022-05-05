
#include <stdio.h>
#include <stdlib.h>
#include <drv/gpio.h>
#include <pinmux.h>
#include <aos/kernel.h>

#include "drv_amp.h"
#include "drv_amp_ops.h"

static int cs8122s_amplifier_init(amplifier_pa_priv_t *priv)
{
    return 0;
}

static int cs8122s_amplifier_uninit(amplifier_pa_priv_t *priv)
{
    return 0;
}

static int cs8122s_amplifier_onoff(amplifier_pa_priv_t *priv, int onoff, int amp_mode)
{
    int ret = csi_gpio_pin_write(priv->pa_mute_hdl, onoff);
    return ret;
}

const struct amplifier_pa_ops g_cs8122s_ops = {.name         = "cs8122s",
                                               .init         = cs8122s_amplifier_init,
                                               .uninit       = cs8122s_amplifier_uninit,
                                               .onoff        = cs8122s_amplifier_onoff,
                                               .probe        = NULL,
                                               .cfgbin_read  = NULL,
                                               .cfgbin_write = NULL,
                                               .getid        = NULL,
                                               .reset        = NULL};
