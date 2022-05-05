/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     sys_clk.c
 * @brief    source file for setting system frequency.
 * @version  V1.0
 * @date     14. Oct 2020
 ******************************************************************************/

#include <stdint.h>
#include <soc.h>
#include <sys_clk.h>
#include <drv/io.h>
#include <hal_clk.h>
#include <sunxi_hal_reset.h>
#include <ccmu/common_ccmu.h>

#define CLK_OSC12M 0
#define CLK_PLL_CPUX 1
#define CLK_PLL_DDR0 2
#define CLK_PLL_PERIPH0_PARENT 3
#define CLK_PLL_PERIPH0_2X 4
#define CLK_PLL_PERIPH0 5
#define CLK_PLL_PERIPH0_800M 6
#define CLK_PLL_PERIPH0_DIV3 7
#define CLK_PLL_VIDEO0 8
#define CLK_PLL_VIDEO0_2X 9
#define CLK_PLL_VIDEO0_4X 10
#define CLK_PLL_VIDEO1 11
#define CLK_PLL_VIDEO1_2X 12
#define CLK_PLL_VIDEO1_4X 13
#define CLK_PLL_VE 14
#define CLK_PLL_AUDIO0 15
#define CLK_PLL_AUDIO0_2X 16
#define CLK_PLL_AUDIO0_4X 17
#define CLK_PLL_AUDIO1 18
#define CLK_PLL_AUDIO1_DIV2 19
#define CLK_PLL_AUDIO1_DIV5 20
#define CLK_PLL_CPUX_DIV 21
#define CLK_CPUX 22
#define CLK_AXI 23
#define CLK_APB 24
#define CLK_PSI_AHB 25
#define CLK_APB0 26
#define CLK_APB1 27
#define CLK_MBUS 28
#define CLK_DE0 29
#define CLK_BUS_DE0 30
#define CLK_DI 31
#define CLK_BUS_DI 32
#define CLK_G2D 33
#define CLK_BUS_G2D 34
#define CLK_CE 35
#define CLK_BUS_CE 36
#define CLK_VE 37
#define CLK_BUS_VE 38
#define CLK_BUS_DMA 39
#define CLK_BUS_MSGBOX0 40
#define CLK_BUS_MSGBOX1 41
#define CLK_BUS_MSGBOX2 42
#define CLK_BUS_SPINLOCK 43
#define CLK_BUS_HSTIMER 44
#define CLK_AVS 45
#define CLK_BUS_DBG 46
#define CLK_BUS_PWM 47
#define CLK_BUS_IOMMU 48
#define CLK_DRAM 49
#define CLK_MBUS_DMA 50
#define CLK_MBUS_VE 51
#define CLK_MBUS_CE 52
#define CLK_MBUS_TVIN 53
#define CLK_MBUS_CSI 54
#define CLK_MBUS_G2D 55
#define CLK_BUS_DRAM 56
#define CLK_MMC0 57
#define CLK_MMC1 58
#define CLK_MMC2 59
#define CLK_BUS_MMC0 60
#define CLK_BUS_MMC1 61
#define CLK_BUS_MMC2 62
#define CLK_BUS_UART0 63
#define CLK_BUS_UART1 64
#define CLK_BUS_UART2 65
#define CLK_BUS_UART3 66
#define CLK_BUS_UART4 67
#define CLK_BUS_UART5 68
#define CLK_BUS_I2C0 69
#define CLK_BUS_I2C1 70
#define CLK_BUS_I2C2 71
#define CLK_BUS_I2C3 72
#define CLK_BUS_CAN0 73
#define CLK_BUS_CAN1 74
#define CLK_SPI0 75
#define CLK_SPI1 76
#define CLK_BUS_SPI0 77
#define CLK_BUS_SPI1 78
#define CLK_EMAC0_25M 79
#define CLK_BUS_EMAC0 80
#define CLK_IR_TX 81
#define CLK_BUS_IR_TX 82
#define CLK_BUS_GPADC 83
#define CLK_BUS_THS 84
#define CLK_I2S0 85
#define CLK_I2S1 86
#define CLK_I2S2 87
#define CLK_I2S2_ASRC 88
#define CLK_BUS_I2S0 89
#define CLK_BUS_I2S1 90
#define CLK_BUS_I2S2 91
#define CLK_SPDIF_TX 92
#define CLK_SPDIF_RX 93
#define CLK_BUS_SPDIF 94
#define CLK_DMIC 95
#define CLK_BUS_DMIC 96
#define CLK_AUDIO_DAC 97
#define CLK_AUDIO_ADC 98
#define CLK_BUS_AUDIO_CODEC 99
#define CLK_USB_OHCI0 100
#define CLK_USB_OHCI1 101
#define CLK_BUS_OHCI0 102
#define CLK_BUS_OHCI1 103
#define CLK_BUS_EHCI0 104
#define CLK_BUS_EHCI1 105
#define CLK_BUS_OTG 106
#define CLK_BUS_LRADC 107
#define CLK_BUS_DPSS_TOP0 108
#define CLK_HDMI_24M 109
#define CLK_HDMI_CEC 110
#define CLK_HDMI_CEC_32K 111
#define CLK_BUS_HDMI 112
#define CLK_MIPI_DSI 113
#define CLK_BUS_MIPI_DSI 114
#define CLK_TCON_LCD0 115
#define CLK_BUS_TCON_LCD0 116
#define CLK_TCON_TV 117
#define CLK_BUS_TCON_TV 118
#define CLK_TVE 119
#define CLK_BUS_TVE 120
#define CLK_BUS_TVE_TOP 121
#define CLK_TVD 122
#define CLK_BUS_TVD 123
#define CLK_BUS_TVD_TOP 124
#define CLK_LEDC 125
#define CLK_BUS_LEDC 126
#define CLK_CSI_TOP 127
#define CLK_CSI0_MCLK 128
#define CLK_BUS_CSI 129
#define CLK_TPADC 130
#define CLK_BUS_TPADC 131
#define CLK_BUS_TZMA 132
#define CLK_DSP 133
#define CLK_BUS_DSP_CFG 134
#define CLK_RISCV 135
#define CLK_RISCV_AXI 136
#define CLK_BUS_RISCV_CFG 137
#define CLK_FANOUT_24M 138
#define CLK_FANOUT_12M 139
#define CLK_FANOUT_16M 140
#define CLK_FANOUT_25M 141
#define CLK_FANOUT_32K 142
#define CLK_FANOUT_27M 143
#define CLK_FANOUT_PCLK 144
#define CLK_FANOUT0_OUT 145
#define CLK_FANOUT1_OUT 146
#define CLK_FANOUT2_OUT 147

#define CLK_R_AHB       0
#define CLK_R_APB0      1
#define CLK_R_APB0_TIMER    2
#define CLK_R_APB0_TWD      3
#define CLK_R_PPU       4
#define CLK_R_APB0_IRRX     5
#define CLK_R_APB0_BUS_IRRX 6
#define CLK_R_AHB_BUS_RTC   7
#define CLK_R_APB0_CPUCFG   8

uint32_t g_system_clock = IHS_VALUE;


uint32_t soc_get_cpu_clk(uint32_t idx)
{
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, CLK_PLL_CPUX_DIV);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_ahb_clk(uint32_t idx)
{   
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, CLK_PSI_AHB);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_apb_clk(uint32_t idx)
{
    hal_clk_id_t id;
    switch (idx)
    {
    case 0:
        id = CLK_APB0;
        break;
    case 1:
        id = CLK_APB1;
        break;
    default:
        return 0;
    }
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, id);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_uart_freq(uint32_t idx)
{
    hal_clk_id_t id;
    switch (idx)
    {
    case 0:
        id = CLK_BUS_UART0;
        break;
    case 1:
        id = CLK_BUS_UART1;
        break;
    case 2:
        id = CLK_BUS_UART2;
        break;
    case 3:
        id = CLK_BUS_UART3;
        break;
    case 4:
        id = CLK_BUS_UART4;
        break;
    case 5:
        id = CLK_BUS_UART5;
        break;
    default:
        return 0;
    }
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, id);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_iic_freq(uint32_t idx)
{
    hal_clk_type_t	clk_type = HAL_SUNXI_CCU;
    hal_clk_id_t	twi_clk_id;
    hal_clk_t		clk;

    switch (idx)
    {
	    case 0:
		    twi_clk_id = CLK_BUS_I2C0;
		    break;
	    case 1:
		    twi_clk_id = CLK_BUS_I2C1;
		    break;
	    case 2:
		    twi_clk_id = CLK_BUS_I2C2;
		    break;
	    case 3:
		    twi_clk_id = CLK_BUS_I2C3;
		    break;
	    default:
		    return 0;
    }
    clk = hal_clock_get(clk_type, twi_clk_id);
    if (!clk)
    {
	    return 0;
    }
    return hal_clk_get_rate(clk);
}

uint32_t soc_get_spi_freq(uint32_t idx)
{
    hal_clk_id_t id;
    switch (idx)
    {
    case 0:
        id = CLK_SPI0;
        break;
    case 1:
        id = CLK_SPI1;
        break;
    default:
        return 0;
    }
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, id);
    if (!clk) {
        return 0;
    }
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_pwm_freq(uint32_t idx)
{
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, CLK_BUS_PWM);
    if (!clk) {
        return 0;
    }
    return hal_clk_get_rate(clk);
}

uint32_t soc_get_i2s_freq(uint32_t idx)
{
    hal_clk_id_t id;
    switch (idx)
    {
    case 0:
        id = CLK_I2S0;
        break;
    case 1:
        id = CLK_I2S1;
        break;
    case 2:
        id = CLK_I2S2;
        break;
    default:
        return 0;
    }
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, id);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_wdt_freq(uint32_t idx)
{
    return IHS_VALUE / 750;
}

uint32_t soc_get_timer_freq(uint32_t idx)
{
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, CLK_AVS);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_rtc_freq(uint32_t idx)
{
    hal_clk_type_t clk_r_type = HAL_SUNXI_R_CCU;
    hal_clk_id_t rtc_clk_r_id = CLK_R_AHB_BUS_RTC;

    hal_clk_t clk = hal_clock_get(clk_r_type, rtc_clk_r_id);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

uint32_t soc_get_cpu_freq(uint32_t idx)
{
    return soc_get_cpu_clk(idx);
}

uint32_t soc_get_sys_freq(void)
{
    return g_system_clock;
}

uint32_t soc_get_ahb_freq(uint32_t idx)
{
    return soc_get_ahb_clk(idx);
}

uint32_t soc_get_apb_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_cur_cpu_freq(void)
{
    return soc_get_cpu_clk(0);
}

uint32_t soc_get_coretim_freq(void)
{
    return g_system_clock;
}

uint32_t soc_get_sdio_freq(uint32_t idx)
{
    hal_clk_id_t id;
    switch (idx)
    {
    case 0:
        id = CLK_MMC0;
        break;
    case 1:
        id = CLK_MMC1;
        break;
    case 2:
        id = CLK_MMC2;
        break;
    default:
        return 0;
    }
    hal_clk_t clk = hal_clock_get(HAL_SUNXI_CCU, id);
    uint32_t rate = hal_clk_get_rate(clk);
    return rate;
}

void soc_set_sys_freq(uint32_t val)
{
    g_system_clock = val;
}

void soc_clk_init(void)
{
    hal_clock_init();
}

#define RST_MBUS 0
#define RST_BUS_DE0 1
#define RST_BUS_DI 2
#define RST_BUS_G2D 3
#define RST_BUS_CE 4
#define RST_BUS_VE 5
#define RST_BUS_DMA 6
#define RST_BUS_MSGBOX0 7
#define RST_BUS_MSGBOX1 8
#define RST_BUS_MSGBOX2 9
#define RST_BUS_SPINLOCK 10
#define RST_BUS_HSTIMER 11
#define RST_BUS_DBG 12
#define RST_BUS_PWM 13
#define RST_BUS_DRAM 14
#define RST_BUS_MMC0 15
#define RST_BUS_MMC1 16
#define RST_BUS_MMC2 17
#define RST_BUS_UART0 18
#define RST_BUS_UART1 19
#define RST_BUS_UART2 20
#define RST_BUS_UART3 21
#define RST_BUS_UART4 22
#define RST_BUS_UART5 23
#define RST_BUS_I2C0 24
#define RST_BUS_I2C1 25
#define RST_BUS_I2C2 26
#define RST_BUS_I2C3 27
#define RST_BUS_CAN0 28
#define RST_BUS_CAN1 29
#define RST_BUS_SPI0 30
#define RST_BUS_SPI1 31
#define RST_BUS_EMAC0 32
#define RST_BUS_IR_TX 33
#define RST_BUS_GPADC 34
#define RST_BUS_THS 35
#define RST_BUS_I2S0 36
#define RST_BUS_I2S1 37
#define RST_BUS_I2S2 38
#define RST_BUS_SPDIF 39
#define RST_BUS_DMIC 40
#define RST_BUS_AUDIO_CODEC 41
#define RST_USB_PHY0 42
#define RST_USB_PHY1 43
#define RST_BUS_OHCI0 44
#define RST_BUS_OHCI1 45
#define RST_BUS_EHCI0 46
#define RST_BUS_EHCI1 47
#define RST_BUS_OTG 48
#define RST_BUS_LRADC 49
#define RST_BUS_DPSS_TOP0 50
#define RST_BUS_HDMI_SUB 51
#define RST_BUS_HDMI_MAIN 52
#define RST_BUS_MIPI_DSI 53
#define RST_BUS_TCON_LCD0 54
#define RST_BUS_TCON_TV 55
#define RST_BUS_LVDS0 56
#define RST_BUS_TVE 57
#define RST_BUS_TVE_TOP 58
#define RST_BUS_TVD 59
#define RST_BUS_TVD_TOP 60
#define RST_BUS_LEDC 61
#define RST_BUS_CSI 62
#define RST_BUS_TPADC 63
#define RST_BUS_DSP 64
#define RST_BUS_DSP_CFG 65
#define RST_BUS_DSP_DBG 66
#define RST_BUS_RISCV_CFG 67
#define RST_BUS_RISCV_SOFT 69
#define RST_BUS_RISCV_CPU_SOFT 70

extern void *hal_reset_control_get(hal_reset_type_t type, hal_reset_id_t id);
extern int hal_reset_control_put(void *reset);
extern int hal_reset_control_set(void *reset); //for other module
extern int hal_reset_control_deassert(void *reset); //for other module
extern int hal_reset_control_assert(void *reset); //for other_module
extern int hal_reset_control_reset(void *reset);  //for other_module
extern int hal_reset_control_status(void *reset); //for other_module

typedef struct {
    int clk_id;
    int rst_id;
} clk_rst_map_t;

static clk_rst_map_t g_clk_rst_map[] = {
    {DE0_CLK               ,  RST_BUS_DE0  },
    {BUS_DE0_CLK           ,  RST_BUS_DE0  },
    {DI_CLK                ,  RST_BUS_DI  },
    {BUS_DI_CLK            ,  RST_BUS_DI  },
    {G2D_CLK               ,  RST_BUS_G2D  },
    {BUS_G2D_CLK           ,  RST_BUS_G2D  },
    {CE_CLK                ,  RST_BUS_CE  },
    {BUS_CE_CLK            ,  RST_BUS_CE  },
    {BUS_DMA_CLK           ,  RST_BUS_DMA  },
    {BUS_PWM_CLK           ,  RST_BUS_PWM  },
    {MMC0_CLK              ,  RST_BUS_MMC0  },
    {MMC1_CLK              ,  RST_BUS_MMC1  },
    {MMC2_CLK              ,  RST_BUS_MMC2  },
    {BUS_MMC0_CLK          ,  RST_BUS_MMC0  },
    {BUS_MMC1_CLK          ,  RST_BUS_MMC1  },
    {BUS_MMC2_CLK          ,  RST_BUS_MMC2  },
    {BUS_UART0_CLK         ,  RST_BUS_UART0  },
    {BUS_UART1_CLK         ,  RST_BUS_UART1  },
    {BUS_UART2_CLK         ,  RST_BUS_UART2  },
    {BUS_UART3_CLK         ,  RST_BUS_UART3  },
    {BUS_UART4_CLK         ,  RST_BUS_UART4  },
    {BUS_UART5_CLK         ,  RST_BUS_UART5  },
    {BUS_I2C0_CLK          ,  RST_BUS_I2C0  },
    {BUS_I2C1_CLK          ,  RST_BUS_I2C1  },
    {BUS_I2C2_CLK          ,  RST_BUS_I2C2  },
    {BUS_I2C3_CLK          ,  RST_BUS_I2C3  },
    {SPI0_CLK              ,  RST_BUS_SPI0  },
    {SPI1_CLK              ,  RST_BUS_SPI1  },
    {BUS_SPI0_CLK          ,  RST_BUS_SPI0  },
    {BUS_SPI1_CLK          ,  RST_BUS_SPI1  },
    {I2S0_CLK              ,  RST_BUS_I2S0  },
    {I2S1_CLK              ,  RST_BUS_I2S1  },
    {I2S2_CLK              ,  RST_BUS_I2S2  },
    {BUS_I2S0_CLK          ,  RST_BUS_I2S0  },
    {BUS_I2S1_CLK          ,  RST_BUS_I2S1  },
    {BUS_I2S2_CLK          ,  RST_BUS_I2S2  },
    {SPDIF_TX_CLK          ,  RST_BUS_SPDIF  },
    {SPDIF_RX_CLK          ,  RST_BUS_SPDIF  },
    {BUS_SPDIF_CLK         ,  RST_BUS_SPDIF  },
    {DMIC_CLK              ,  RST_BUS_DMIC  },
    {BUS_DMIC_CLK          ,  RST_BUS_DMIC  },
    {BUS_AUDIO_CODEC_CLK   ,  RST_BUS_AUDIO_CODEC  },
    {BUS_OTG_CLK           ,  RST_BUS_OTG  },
    {BUS_LRADC_CLK         ,  RST_BUS_LRADC  },
    {MIPI_DSI_CLK          ,  RST_BUS_MIPI_DSI  },
    {BUS_MIPI_DSI_CLK      ,  RST_BUS_MIPI_DSI  },
    {0xFF, 0xFF}
};

static int reset_id_get(int clk_id)
{
    int i;

    i = 0;
    while(i < 0xFF) {
        clk_rst_map_t *clk_rst = &g_clk_rst_map[i];
        if (clk_rst->clk_id == 0xFF) {
            break;
        }
        if (clk_rst->clk_id == clk_id) {
            return clk_rst->rst_id;
        }
        i++;
    }
    return -1;
}

void soc_clk_enable(int32_t module)
{
    hal_clk_t clk;
    hal_clk_type_t	clk_type = HAL_SUNXI_CCU;
    hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	int	reset_id;

    clk = hal_clock_get(clk_type, module);
    if (clk) {
        hal_clock_enable(clk);
        reset_id = reset_id_get(module);
        if (reset_id != -1) {
            void *reset = hal_reset_control_get(reset_type, reset_id);
            if (!hal_reset_control_deassert(reset)) {
                hal_reset_control_put(reset);
            }
        }
    }
}

void soc_clk_disable(int32_t module)
{
    hal_clk_t clk;
    hal_clk_type_t	clk_type = HAL_SUNXI_CCU;
    hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	int	reset_id;

    clk = hal_clock_get(clk_type, module);
    if (clk) {
        hal_clock_disable(clk);
        reset_id = reset_id_get(module);
        if (reset_id != -1) {
            void *reset = hal_reset_control_get(reset_type, reset_id);
            if (!hal_reset_control_assert(reset)) {
                hal_reset_control_put(reset);
            }
        }
    }
}
