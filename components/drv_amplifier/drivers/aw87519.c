
#include <aos/kernel.h>

#include <drv/gpio.h>
#include <pinmux.h>

#include "drv_amp.h"
#include "drv_amp_ops.h"

#define AMP_CHIPID  0x59
#define AW_ID_REG   0x00
#define AW_MUTE_REG 0x01

static unsigned char g_aw_bin[] = {0x69, 0xB7, 0x69, 0xB7, 0x02, 0x09, 0x03, 0xE4, 0x04, 0x01, 0x05,
                                   0x0D, 0x06, 0x53, 0x07, 0x4E, 0x08, 0x0B, 0x09, 0x08, 0x0A, 0x4B,
                                   0x60, 0x66, 0x61, 0xA0, 0x62, 0x01, 0x63, 0x0B, 0x64, 0xD5, 0x65,
                                   0xA4, 0x66, 0x78, 0x67, 0xC4, 0x68, 0x90, 0x01, 0x00};

static int aw_cfgbin_read(uint8_t *byte, uint16_t count)
{
    uint16_t i;

    if (count < sizeof(g_aw_bin)) {
        return -1;
    }

    uint8_t data_byte = 0;
    for (i = 0; i < sizeof(g_aw_bin) / 2; i++) {
        int ret = -1; //iic_bus_read_byte(PA_IIC_IDX, PA_IIC_ADDR, g_aw_bin[i * 2], &data_byte);
        if (ret < 0) {
            return -1;
        }

        byte[i * 2]     = g_aw_bin[i * 2];
        byte[i * 2 + 1] = data_byte;
    }

    return sizeof(g_aw_bin);
}

static int cfgbin_write(uint8_t *byte, uint16_t count)
{
    uint16_t i;
    int      ret = 0;

    for (i = 0; i < count / 2; i++) {
        ret = -1; //iic_bus_write_byte(PA_IIC_IDX, PA_IIC_ADDR, byte[i * 2], byte[i * 2 + 1]);
        if (ret < 0) {
            break;
        }
    }

    return ret > 0 ? count : -1;
}

static int aw_cfgbin_write(uint8_t *byte, uint16_t count)
{
    if (byte == NULL || count == 0) {
        return aw_cfgbin_write(g_aw_bin, sizeof(g_aw_bin)); /* 写默认配置 */
    } else {
        return cfgbin_write(byte, count);
    }

    return -1;
}

static int aw_amplifier_init(amplifier_pa_priv_t *priv)
{
    return aw_cfgbin_write(NULL, 0);
}

static int aw_amplifier_uninit()
{
    return 0;
}

static int aw_amplifier_onoff(amplifier_pa_priv_t *priv, int onoff, int amp_mode)
{
    return -1; //iic_bus_write_byte(PA_IIC_IDX, PA_IIC_ADDR, AW_MUTE_REG, onoff ? 0xF0 : 0x00);
}

static int aw_amplifier_reset(amplifier_pa_priv_t *priv)
{
    if (priv->pa_mute_hdl) {
        csi_gpio_pin_write(priv->pa_mute_hdl, 0);
        aos_msleep(10);
        csi_gpio_pin_write(priv->pa_mute_hdl, 1);
        aos_msleep(10);
    }

    return 0;
}

static int aw_amplifier_probe(amplifier_pa_priv_t *priv)
{
    int     ret;
    uint8_t byte_data;

    aw_amplifier_reset(priv);

    ret = -1; //iic_bus_read_byte(PA_IIC_IDX, PA_IIC_ADDR, AW_ID_REG, &byte_data);
    if (ret == 0) {
        if (byte_data == AMP_CHIPID) {
            return 0;
        }
    }
    return -1;
}

static int aw_amplifier_getid(void)
{
    uint8_t data_byte = 0;
    int     ret;

    ret = -1; //iic_bus_read_byte(PA_IIC_IDX, PA_IIC_ADDR, 0x00, &data_byte);
    if (ret < 0) {
        return -1;
    }
    return data_byte;
}

const struct amplifier_pa_ops g_aw87519_ops = {
    .name         = "aw87519",
    .init         = aw_amplifier_init,
    .uninit       = aw_amplifier_uninit,
    .onoff        = aw_amplifier_onoff,
    .probe        = aw_amplifier_probe,
    .cfgbin_read  = aw_cfgbin_read,
    .cfgbin_write = aw_cfgbin_write,
    .getid        = aw_amplifier_getid,
    .reset        = aw_amplifier_reset,
};
