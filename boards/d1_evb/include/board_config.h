/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     board_config.h
 * @brief    header File for pin definition
 * @version  V1.0
 * @date     02. June 2020
 ******************************************************************************/

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <stdint.h>
#include "soc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_GETTIME_USE_TIMER_ID     0
#define CONSOLE_UART_IDX                    0
#define CONSOLE_TXD                    PA23
#define CONSOLE_RXD                    PA24
#define CONSOLE_TXD_FUNC               PA23_UART0_TX
#define CONSOLE_RXD_FUNC               PA24_UART0_RX

/* example pin manager */
#define WAKEUP_PIN                      PA4
#define WAKEUP_PIN_FUNC                 PIN_FUNC_GPIO
#define EXAMPLE_WAKEUP_NUM              WJ_IOCTL_Wakeupn
#define EXAMPLE_BOARD_WAKEUP_PIN_NAME   "A4"


#define EXAMPLE_UART_IDX                0
#define EXAMPLE_PIN_UART_TX             PA23
#define EXAMPLE_PIN_UART_RX             PA24
#define EXAMPLE_PIN_UART_TX_FUNC        PA23_UART0_TX
#define EXAMPLE_PIN_UART_RX_FUNC        PA24_UART0_RX

/* example uart_dmac_etb */
#define EXAMPLE_DMAC_ETB_UART_IDX       0
#define EXAMPLE_DMAC_ETB_UART_TX        PA23
#define EXAMPLE_DMAC_ETB_UART_RX        PA24
#define EXAMPLE_DMAC_ETB_UART_TX_FUNC   PA23_UART0_TX
#define EXAMPLE_DMAC_ETB_UART_RX_FUNC   PA24_UART0_RX

#define EXAMPLE_SPI_IDX                 0
#define EXAMPLE_PIN_SPI_MOSI            PA17
#define EXAMPLE_PIN_SPI_MISO            PA18
#define EXAMPLE_PIN_SPI_CS              PA15
#define EXAMPLE_PIN_SPI_SCK             PA16
#define EXAMPLE_PIN_SPI_MOSI_FUNC       PA17_SPI0_MOSI
#define EXAMPLE_PIN_SPI_MISO_FUNC       PA18_SPI0_MISO
#define EXAMPLE_PIN_SPI_CS_FUNC         PA15_SPI0_CS
#define EXAMPLE_PIN_SPI_SOFT_CS_FUNC    PIN_FUNC_GPIO
#define EXAMPLE_PIN_SPI_SCK_FUNC        PA16_SPI0_SCK

#define EXAMPLE_PIN_SPI_CS_GPIO_IDX     0
#define EXAMPLE_PIN_SPI_CS_MSK          ( 1 << EXAMPLE_PIN_SPI_CS )

#define EXAMPLE_SPI_SLAVE_IDX           1
#define EXAMPLE_PIN_SPI_SLAVE_MOSI      PA29
#define EXAMPLE_PIN_SPI_SLAVE_MISO      PA30
#define EXAMPLE_PIN_SPI_SLAVE_CS        PA27
#define EXAMPLE_PIN_SPI_SLAVE_SCK       PA28
#define EXAMPLE_PIN_SPI_SLAVE_MOSI_FUNC PA29_SPI1_MOSI
#define EXAMPLE_PIN_SPI_SLAVE_MISO_FUNC PA30_SPI1_MISO
#define EXAMPLE_PIN_SPI_SLAVE_CS_FUNC   PA27_SPI1_CS
#define EXAMPLE_PIN_SPI_SLAVE_SCK_FUNC  PA28_SPI1_SCK

#define EXAMPLE_IIC_IDX                 0
#define EXAMPLE_PIN_IIC_SDA             PA9
#define EXAMPLE_PIN_IIC_SCL             PA8
#define EXAMPLE_PIN_IIC_SDA_FUNC        PA9_IIC0_SDA
#define EXAMPLE_PIN_IIC_SCL_FUNC        PA8_IIC0_SCL

#define EXAMPLE_GPIO_PIN                PA11
#define EXAMPLE_BOARD_GPIO_PIN_NAME     "PA11"
#define EXAMPLE_GPIO_PIN_FUNC           PIN_FUNC_GPIO

#define EXAMPLE_TOGGLE_GPIO_PIN         PA10
#define EXAMPLE_TOGGLE_GPIO_PIN1        PA11
#define EXAMPLE_TOGGLE_GPIO_PIN_FUNC    PIN_FUNC_GPIO
#define EXAMPLE_TOGGLE_GPIO_PIN1_FUNC   PIN_FUNC_GPIO
#define EXAMPLE_TOGGLE_GPIO_IDX         0
#define EXAMPLE_TOGGLE_GPIO_PIN_MASK    ( ( 1 << EXAMPLE_TOGGLE_GPIO_PIN ) | ( 1 << EXAMPLE_TOGGLE_GPIO_PIN1  ) )

#define EXAMPLE_INTR_GPIO_PIN           PA10
#define EXAMPLE_INTR_GPIO_PIN1          PA11
#define EXAMPLE_INTR_GPIO_PIN_FUNC      PIN_FUNC_GPIO
#define EXAMPLE_INTR_GPIO_PIN1_FUNC     PIN_FUNC_GPIO
#define EXAMPLE_INTR_GPIO_IDX           0
#define EXAMPLE_INTR_GPIO_PIN_NAME      "PA10"
#define EXAMPLE_INTR_GPIO_PIN1_NAME     "PA11"
#define EXAMPLE_INTR_GPIO_PIN_MASK    ( ( 1 << EXAMPLE_INTR_GPIO_PIN ) | ( 1 << EXAMPLE_INTR_GPIO_PIN1  ) )

#define EXAMPLE_PWM_IDX                 0
#define EXAMPLE_PWM_CH_IDX              0
#define EXAMPLE_PWM_CH                  PA6
#define EXAMPLE_PWM_CH_FUNC             PA6_PWM_CH6

#define EXAMPLE_PWM_TIMER_CH_IDX          0

#define EXAMPLE_PWM_CAPTURE_CH_IDX        0
#define EXAMPLE_PWM_CAPTURE_CH            PA6
#define EXAMPLE_PWM_CAPTURE_CH_FUNC       PA6_PWM_CH6
#define EXAMPLE_PWM_CAPTURE_GPIO_PIN      PA7
#define EXAMPLE_PWM_CAPTURE_GPIO_PIN_FUNC PIN_FUNC_GPIO
#define EXAMPLE_PWM_CAPTURE_GPIO_MASK     (1 << EXAMPLE_PWM_CAPTURE_GPIO_PIN)

#define QSPIFLASH_IDX                   0
#define EXAMPLE_QSPI_IDX                0

#define EXAMPLE_I2S_IDX                 0
#define EXAMPLE_I2S_MCLK                PA10            /* board X7A: CH5 */
#define EXAMPLE_I2S_SCLK                PA6             /* board X7A: A6 */
#define EXAMPLE_I2S_WSCLK               PA7             /* board X7A: A7 */
#define EXAMPLE_I2S_SDA                 PA8             /* board X7A: A8  */
#define EXAMPLE_I2S_MCLK_FUNC           PA10_I2S0_MCLK
#define EXAMPLE_I2S_SCLK_FUNC           PA6_I2S0_SCLK
#define EXAMPLE_I2S_WSCLK_FUNC          PA7_I2S0_WSCLK
#define EXAMPLE_I2S_SDA_FUNC            PA8_I2S0_SDA

#define EXAMPLE_I2S_SLAVE_IDX           1
#define EXAMPLE_I2S_SLAVE_MCLK          PA26
#define EXAMPLE_I2S_SLAVE_SCLK          PA2
#define EXAMPLE_I2S_SLAVE_WSCLK         PA3
#define EXAMPLE_I2S_SLAVE_SDA           PA5
#define EXAMPLE_I2S_SLAVE_MCLK_FUNC     PA26_I2S1_MCLK
#define EXAMPLE_I2S_SLAVE_SCLK_FUNC     PA2_I2S1_SCLK
#define EXAMPLE_I2S_SLAVE_WSCLK_FUNC    PA3_I2S1_WSCLK
#define EXAMPLE_I2S_SLAVE_SDA_FUNC      PA5_I2S1_SDA

#define EXAMPLE_ADC_CHANNEL0            0
#define EXAMPLE_ADC_CHANNEL0_PIN        EXAMPLE_ADC_CH0
#define EXAMPLE_ADC_CHANNEL0_PIN_FUNC   EXAMPLE_ADC_CH0_FUNC
#define EXAMPLE_ADC_CHANNEL1            1
#define EXAMPLE_ADC_CHANNEL1_PIN        EXAMPLE_ADC_CH1
#define EXAMPLE_ADC_CHANNEL1_PIN_FUNC   EXAMPLE_ADC_CH1_FUNC
#define EXAMPLE_ADC_CHANNEL2            2
#define EXAMPLE_ADC_CHANNEL2_PIN        EXAMPLE_ADC_CH2
#define EXAMPLE_ADC_CHANNEL2_PIN_FUNC   EXAMPLE_ADC_CH2_FUNC
#define EXAMPLE_ADC_CHANNEL3            3
#define EXAMPLE_ADC_CHANNEL3_PIN        EXAMPLE_ADC_CH3
#define EXAMPLE_ADC_CHANNEL3_PIN_FUNC   EXAMPLE_ADC_CH3_FUNC

#define EXAMPLE_ADC_CH0     PA3
#define EXAMPLE_ADC_CH0_FUNC    PA3_ADC_A1
#define EXAMPLE_ADC_CH1     PA4
#define EXAMPLE_ADC_CH1_FUNC    PA4_ADC_A2
#define EXAMPLE_ADC_CH2     PA5
#define EXAMPLE_ADC_CH2_FUNC    PA5_ADC_A3
#define EXAMPLE_ADC_CH3     PA6
#define EXAMPLE_ADC_CH3_FUNC    PA6_ADC_A4
#define EXAMPLE_ADC_CH4     PA7
#define EXAMPLE_ADC_CH4_FUNC    PA7_ADC_A5
#define EXAMPLE_ADC_CH5     PA8
#define EXAMPLE_ADC_CH5_FUNC    PA8_ADC_A6
#define EXAMPLE_ADC_CH6     PA9
#define EXAMPLE_ADC_CH6_FUNC    PA9_ADC_A7
#define EXAMPLE_ADC_CH7     PA10
#define EXAMPLE_ADC_CH7_FUNC    PA10_ADC_A8
#define EXAMPLE_ADC_CH8     PA15
#define EXAMPLE_ADC_CH8_FUNC    PA15_ADC_A9
#define EXAMPLE_ADC_CH9     PA16
#define EXAMPLE_ADC_CH9_FUNC    PA16_ADC_A10
#define EXAMPLE_ADC_CH10    PA17
#define EXAMPLE_ADC_CH10_FUNC   PA17_ADC_A11
#define EXAMPLE_ADC_CH11    PA18
#define EXAMPLE_ADC_CH11_FUNC   PA18_ADC_A12
#define EXAMPLE_ADC_CH12    PA19
#define EXAMPLE_ADC_CH12_FUNC   PA19_ADC_A13
#define EXAMPLE_ADC_CH13    PA20
#define EXAMPLE_ADC_CH13_FUNC   PA20_ADC_A14
#define EXAMPLE_ADC_CH14    PA21
#define EXAMPLE_ADC_CH14_FUNC   PA21_ADC_A15

/* tests pin manager */
#define TEST_USART_IDX                  1
#define TEST_PIN_USART_TX               PA28         /* board X7A: A15*/
#define TEST_PIN_USART_RX               PA27         /* board X7A: A16*/
#define TEST_PIN_USART_TX_FUNC          PA28_UART1_TX
#define TEST_PIN_USART_RX_FUNC          PA27_UART1_RX

#define TEST_SPI_IDX                    1
#define TEST_PIN_SPI_MOSI               PA25            /* board X7A: MOSI */
#define TEST_PIN_SPI_MISO               PA26            /* board X7A: MISO */
#define TEST_PIN_SPI_CS                 PA0             /* board X7A: CS */
#define TEST_PIN_SPI_SCK                PA1             /* board X7A: SCK */
#define TEST_PIN_SPI_MOSI_FUNC          PA25_SPI0_MOSI
#define TEST_PIN_SPI_MISO_FUNC          PA26_SPI0_MISO
#define TEST_PIN_SPI_CS_FUNC            PA0_SPI0_CS
#define TEST_PIN_SPI_SCK_FUNC           PA1_SPI0_SCK

#define TEST_SPI_SLAVE_IDX              0
#define TEST_PIN_SPI_SLAVE_MOSI         PA29            /* board X7A: A17 */
#define TEST_PIN_SPI_SLAVE_MISO         PA30            /* board X7A: CH0 */
#define TEST_PIN_SPI_SLAVE_CS           PA27            /* board X7A: A15 */
#define TEST_PIN_SPI_SLAVE_SCK          PA28            /* board X7A: A16 */
#define TEST_PIN_SPI_SLAVE_MOSI_FUNC    PA29_SPI1_MOSI
#define TEST_PIN_SPI_SLAVE_MISO_FUNC    PA30_SPI1_MISO
#define TEST_PIN_SPI_SLAVE_CS_FUNC      PA27_SPI1_CS
#define TEST_PIN_SPI_SLAVE_SOFT_CS_FUNC PIN_FUNC_GPIO
#define TEST_PIN_SPI_SLAVE_SCK_FUNC     PA28_SPI1_SCK
#define TEST_PIN_SPI_SLAVE_CS_MSK       ( 1 << TEST_PIN_SPI_SLAVE_CS )

#define TEST_IIC_IDX                    0
#define TEST_PIN_IIC_SDA                PA16
#define TEST_PIN_IIC_SCL                PA15
#define TEST_PIN_IIC_SDA_FUNC           PA16_I2C0_SDA
#define TEST_PIN_IIC_SCL_FUNC           PA15_I2C0_SCL

#define TEST_USI_IIC_IDX                1
#define TEST_PIN_USI_IIC_SDA            PA17
#define TEST_PIN_USI_IIC_SCL            PA16
#define TEST_PIN_USI_IIC_SDA_FUNC       PA17_USI1_SD0
#define TEST_PIN_USI_IIC_SCL_FUNC       PA16_USI1_SCLK

#define TEST_GPIO_PIN                   PA25
#define TEST_BOARD_GPIO_PIN_NAME        "MOSI"
#define TEST_GPIO_PIN_FUNC              PIN_FUNC_GPIO

#define TEST_QSPI0_IDX                  0

#define TEST_PWM_IDX                    0
#define TEST_PWM_CH                     0
#define TEST_PWM_CH0                    PA0
#define TEST_PWM_CH0_FUNC               PA0_PWM_CH0
#define TEST_PWM_GPIO_PIN               PA11
#define TEST_PWM_GPIO_PORT_FUNC         PIN_FUNC_GPIO


#define TEST_I2S_IDX                    0
#define TEST_I2S_MCLK                   PA9
#define TEST_I2S_SCLK                   PA8
#define TEST_I2S_WSCLK                  PA7
#define TEST_I2S_SDA                    PA6
#define TEST_I2S_MCLK_FUNC              PA9_I2S_MCLK
#define TEST_I2S_SCLK_FUNC              PA8_I2S_SCLK
#define TEST_I2S_WSCLK_FUNC             PA7_I2S_WSCLK
#define TEST_I2S_SDA_FUNC               PA6_I2S_SDA

#define TEST_ADC_IDX                    0
#define TEST_ADC_CHANNEL0               0
#define TEST_ADC_CHANNEL0_PIN           PA3
#define TEST_ADC_CHANNEL0_PIN_FUNC      PA3_ADC_A1
#define TEST_ADC_CHANNEL1               1
#define TEST_ADC_CHANNEL1_PIN           PA4
#define TEST_ADC_CHANNEL1_PIN_FUNC      PA4_ADC_A2
#define TEST_ADC_CHANNEL2               2
#define TEST_ADC_CHANNEL2_PIN           PA5
#define TEST_ADC_CHANNEL2_PIN_FUNC      PA5_ADC_A3
#define TEST_ADC_CHANNEL3               3
#define TEST_ADC_CHANNEL3_PIN           PA6
#define TEST_ADC_CHANNEL3_PIN_FUNC      PA6_ADC_A4

#define TEST_GPIO_PIN_INPUT             PA11
#define TEST_BOARD_GPIO_PIN_INPUT_NAME  "PA11"
#define TEST_GPIO_PIN_INPUT_FUNC        PIN_FUNC_GPIO
#define TEST_GPIO_PIN_INPUT_MSK         ( 1U << TEST_GPIO_PIN_INPUT )
#define TEST_GPIO_PIN_OUTPUT            PA18
#define TEST_BOARD_GPIO_PIN_OUPUT_NAME "PA18"
#define TEST_GPIO_PIN_OUTPUT_FUNC       PIN_FUNC_GPIO
#define TEST_GPIO_PIN_OUTPUT_MSK       ( 1U << TEST_GPIO_PIN_OUTPUT )

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_CONFIG_H_ */

