
#include <stdio.h>
#include <stdlib.h>
#include <drv/gpio.h>
#include <pinmux.h>
#include <aos/kernel.h>

#include "drv_amp.h"
#include "drv_amp_ops.h"

/******************************************
 * Defalt PA PIN Control
 *****************************************/
static int def_amplifier_init(amplifier_pa_priv_t *priv)
{
    return 0;
}

static int def_amplifier_uninit(amplifier_pa_priv_t *priv)
{
    return 0;
}

static int def_amplifier_onoff(amplifier_pa_priv_t *priv, int onoff, int amp_mode)
{
    int ret = csi_gpio_pin_write(priv->pa_mute_hdl, onoff);
    return ret;
}

const struct amplifier_pa_ops g_padef_ops = {.name         = "gpioamp",
                                             .init         = def_amplifier_init,
                                             .uninit       = def_amplifier_uninit,
                                             .onoff        = def_amplifier_onoff,
                                             .probe        = NULL,
                                             .cfgbin_read  = NULL,
                                             .cfgbin_write = NULL,
                                             .getid        = NULL,
                                             .reset        = NULL};
