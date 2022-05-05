#ifndef _PA_OPS_H_
#define _PA_OPS_H_

#include <stdint.h>
#include <drv/gpio.h>

#ifndef CONFIG_CSI_V1
    typedef void *gpio_pin_handle_t;
#endif

typedef struct _amplifier_pa_priv {
    int               pa_config;
    int               pa_onoff;
    int               pa_ctrl_pin;
    int               pa_power_pin;
    gpio_pin_handle_t pa_mute_hdl;
    gpio_pin_handle_t pa_power_hdl;
    int               active_pa_id;
} amplifier_pa_priv_t;

struct amplifier_pa_ops {
    const char *name;
    int (*probe)(amplifier_pa_priv_t *priv);
    int (*init)(amplifier_pa_priv_t *priv);
    int (*uninit)(amplifier_pa_priv_t *priv);
    int (*getid)(void);
    int (*onoff)(amplifier_pa_priv_t *priv, int onoff, int AMP_MODE);
    int (*reset)(amplifier_pa_priv_t *priv);
    int (*cfgbin_read)(uint8_t *byte, uint16_t count);
    int (*cfgbin_write)(uint8_t *byte, uint16_t count);
};

#endif
