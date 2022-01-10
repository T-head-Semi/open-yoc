#include <stdint.h>
#include <string.h>
#include <soc.h>
#include "aos/hal/uart.h"

// extern usart_handle_t console_uart;

#if (DEBUG_LAST_WORD_ENABLE > 0)

int clear_silent_reboot_flag(void)
{
    // do nothing
    return 0;
}

int set_silent_reboot_flag(void)
{
    // do nothing
    return 0;
}

void k_dcache_clean(uint32_t addr, uint32_t len)
{
    csi_dcache_clean_range((uint32_t *)addr, (int32_t)len);
}

/* set ram attributes for some mcu*/
void alios_debug_lastword_init_hook()
{

}
#endif

#if defined(AOS_COMP_DEBUG) && (AOS_COMP_DEBUG > 0)
int alios_debug_print(const char *buf, int size)
{
    const uint8_t *ptr;
    int32_t left;

    if (buf == NULL || size <= 0) {
        return -1;
    }

    ptr = (const uint8_t *)buf;
    left = size;

    if (buf == NULL) {
        return 0;
    }

    uart_dev_t uart_stdio;
    memset(&uart_stdio, 0, sizeof(uart_stdio));
    uart_stdio.port = STDIO_UART;

    /* 阻塞式发送 */
    while (left > 0) {
        hal_uart_send_poll(&uart_stdio, ptr, 1);
        ++ptr;
        --left;
    }

    return size;
}
#endif

#if defined(AOS_COMP_CLI) && (AOS_COMP_CLI > 0)
#ifndef STDIO_UART
#define CLI_UART 0
#else
#define CLI_UART STDIO_UART
#endif
int uart_input_read()
{
    int ret;
    uint8_t rx_byte;
    uart_dev_t uart_stdio;
    memset(&uart_stdio, 0, sizeof(uart_stdio));
    uart_stdio.port = CLI_UART;

    ret = hal_uart_recv_poll(&uart_stdio, &rx_byte, 1);

    return ret > 0 ? rx_byte : 0;
}
#endif

// int alios_debug_pc_check(char *pc)
// {
//     if ( (((uint32_t)pc > (uint32_t)&__stext) &&
//           ((uint32_t)pc < (uint32_t)__etext)) ||
//          (((uint32_t)pc > (uint32_t)__ram_code_start__) &&
//           ((uint32_t)pc < (uint32_t)__ram_code_end__)) ||
//          (((uint32_t)pc > (uint32_t)__itcm_code_start__) &&
//           ((uint32_t)pc < (uint32_t)__itcm_code_end__)) ) {
//         return 0;
//     } else {
//         return -1;
//     }
// }