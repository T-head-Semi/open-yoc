#ifndef _PA_COMMON_H_
#define _PA_COMMON_H_

#include <stdint.h>
#ifdef CONFIG_CSI_V1
#include <drv/gpio.h>
#endif

/*已经支持的型号*/
#define AMP_TYPE_GPIO     0 /*拉高使能*/
#define AMP_TYPE_MIX2910  1 /*支持D1\D2\D3\AB模式*/
#define AMP_TYPE_AW87519  2 /*I2C接口*/
#define AMP_TYPE_CS8122S  3
#define APM_SUPPORT_COUNT 4

#define AMP_MODE_DEF 0 /*默认，普通GPIO拉高使能，拉低关闭*/
#define AMP_MODE_D1  1 /*D1类，防破音*/
#define AMP_MODE_D2  2 /*D2类，防破音*/
#define AMP_MODE_D3  3 /*D类 ，防破音关闭*/
#define AMP_MODE_AB  4 /*AB类，防破音关闭*/

/* 调试扩展：高两字节非0，进入不关闭模式 */
#define AMP_MODE_LOCK_ON 0x00010000

/**
 * Amplifier init
 *
 * @param[in]  amp_type   Initialize the specified chip type.
 * @param[in]  mute_pin   Mute control gpio PIN number.
 * @param[in]  power_pin  Power control gpio PIN number.
 *                        >=0 pin number
 *                        <0  no power control
 * @param[in]  amp_mode   Working mode of power amplifier.
 *
 * @return  0: success.
 */
int amplifier_init(int amp_type, int mute_pin, int power_pin, int amp_mode);

/**
 * Amplifier deinit
 * 
 * @return  0: success.
 */
int amplifier_deinit(void);

/**
 * Amplifier control
 *
 * @param[in]  onoff 0:off 1:on
 * 
 * @return  0: success.
 */
int amplifier_onoff(int onoff);

/**
 * Amplifier working mode config
 *
 * @param[in]  amp_mode  Working mode of power amplifier.
 * @param[in]  lock_on   Disable amplifier off.
 * 
 * @return  0: success.
 */
int amplifier_config(int amp_mode, int lock_on);

/**
 * Get amplifier chip id
 * 
 * @return chip id
 */
int amplifier_getid(void);

/**
 * Get amplifier config bin
 * 
 * @return  0: success.
 */
int amplifier_cfgbin_read(uint8_t *byte, uint16_t count);

/**
 * Set amplifier config bin
 * 
 * @return  0: success.
 */
int amplifier_cfgbin_write(uint8_t *byte, uint16_t count);

#endif
