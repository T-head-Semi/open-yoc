/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     dw_uart_ll.c
 * @brief    dw uart ll driver
 * @version  V1.0
 * @date     18. December 2019
 ******************************************************************************/
#include <dw_uart_ll.h>

int32_t dw_uart_wait_idle(dw_uart_regs_t *uart_base)
{
    uint32_t timecount = 0U;
    int32_t ret = 0;

    while ((uart_base->USR & DW_UART_USR_BUSY_SET) && (timecount < UART_BUSY_TIMEOUT)) {
        timecount++;
    }

    if (timecount >= UART_BUSY_TIMEOUT) {
        ret = -1;
    }

    return ret;
}

int32_t  dw_uart_wait_timeout(dw_uart_regs_t *uart_base)
{
    uint32_t timecount = 0U;
    int32_t ret = 0;

    while ((uart_base->LSR & 0x81U) || (uart_base->USR & 0x1U)) {
        uart_base->LSR;
        uart_base->RBR;
        timecount++;

        if (timecount >= UART_BUSY_TIMEOUT) {
            ret = -1;
            break;
        }
    }

    if (ret == 0) {
        ret = dw_uart_wait_idle(uart_base);
    }

    return ret;
}

ATTRIBUTE_DATA int32_t dw_uart_config_baudrate(dw_uart_regs_t *uart_base, uint32_t baud, uint32_t uart_freq)
{
    uint32_t divisor;
    int32_t ret = 0;
    ret = dw_uart_wait_timeout(uart_base);

    if (ret == 0) {

        if (baud != 0U) {
            divisor = ((uart_freq * 10U) / baud) >> 4U;
        } else {
            divisor = 0x1U;
        }

        if (dw_uart_get_ip_id(uart_base) == DW_UART_SUPPORT_RATE) {
            divisor = divisor / 10U;
            uint32_t uart_rate = (((uart_freq * 10U) / (baud * divisor)));

            if ((uart_rate % 10U) >= 5U) {
                uart_rate = (uart_rate / 10U) + 1U;
            } else {
                uart_rate = uart_rate / 10U;
            }

            dw_uart_config_uart_baud(uart_base, (uint8_t)uart_rate);
        } else if (dw_uart_get_ip_id(uart_base) == DW_UART_SUPPORT_FRACTIONAL_BAUD_RATE) {
            divisor = (uart_freq * 10U) / (baud << 4U);
            uint32_t fractional = divisor % 10U;
            divisor = divisor / 10U;

            dw_uart_config_uart_baud(uart_base, 16U * fractional / 10U);
        } else {
            if ((divisor % 10U) >= 5U) {
                divisor = (divisor / 10U) + 1U;
            } else {
                divisor = divisor / 10U;
            }

            if (divisor == 0U) {
                divisor = 0x1U;
            }
        }

        uart_base->LCR |= DW_UART_LCR_DLAB_EN;
        /* DLL and DLH is lower 8-bits and higher 8-bits of divisor.*/
        uart_base->DLH = (divisor >> 8U) & 0xFFU;
        uart_base->DLL = divisor & 0xFFU;
        /*
         * The DLAB must be cleared after the baudrate is setted
         * to access other registers.
         */
        uart_base->LCR &= (~DW_UART_LCR_DLAB_EN);
    }

    return ret;
}

int32_t  dw_uart_config_stop_bits(dw_uart_regs_t *uart_base, uint32_t stop_bits)
{
    int32_t ret;
    ret = dw_uart_wait_timeout(uart_base);

    if (ret == 0) {

        //when data length is 5 bits, use dw_uart_config_stop_bits_2 will be 1.5 stop bits
        if (stop_bits == 1U) {
            dw_uart_config_stop_bits_1(uart_base);
        } else if (stop_bits == 2U) {
            dw_uart_config_stop_bits_2(uart_base);
        }
    }

    return ret;
}

int32_t dw_uart_config_parity_none(dw_uart_regs_t *uart_base)
{
    int32_t ret;
    ret = dw_uart_wait_timeout(uart_base);

    if (ret == 0) {
        uart_base->LCR &= (~DW_UART_LCR_PEN_EN);
    }

    return ret;
}

int32_t dw_uart_config_parity_odd(dw_uart_regs_t *uart_base)
{
    int32_t ret;

    ret = dw_uart_wait_timeout(uart_base);

    if (ret == 0) {
        uart_base->LCR |= DW_UART_LCR_PEN_EN;
        uart_base->LCR &= ~(DW_UART_LCR_EPS_EN);
    }

    return ret;
}

int32_t dw_uart_config_parity_even(dw_uart_regs_t *uart_base)
{
    int32_t ret;

    ret = dw_uart_wait_timeout(uart_base);

    if (ret == 0) {
        uart_base->LCR |= DW_UART_LCR_PEN_EN;
        uart_base->LCR |= DW_UART_LCR_EPS_EN;
    }

    return ret;
}

int32_t dw_uart_config_data_bits(dw_uart_regs_t *uart_base, uint32_t data_bits)
{
    int32_t ret;

    ret = dw_uart_wait_timeout(uart_base);

    uart_base->LCR &= 0xFCU;
    uart_base->LCR |= (data_bits - 5U);

    return ret;
}
