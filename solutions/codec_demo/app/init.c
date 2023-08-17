#include <stdbool.h>
#include <uservice/uservice.h>
#include <yoc/init.h>
#include <yoc/partition.h>
#include <aos/kv.h>
#include "board.h"
#include <yoc/partition.h>
#include <aos/kv.h>
#include <drv/uart.h>
#include "app_main.h"
#if defined(AOS_COMP_DEBUG) && (AOS_COMP_DEBUG > 0)
#include <debug/dbg.h>
#endif

#define TAG "init"

static void stduart_init(void)
{
    extern void console_init(int idx, uint32_t baud, uint16_t buf_size);
    console_init(CONSOLE_UART_IDX, CONFIG_CLI_USART_BAUD, CONFIG_CONSOLE_UART_BUFSIZE);
}

void board_yoc_init(void)
{
    board_init();
    stduart_init();
    board_cli_init();
    event_service_init(NULL);
    ulog_init();
    aos_set_log_level(AOS_LL_DEBUG);

#if defined(AOS_COMP_DEBUG) && (AOS_COMP_DEBUG > 0)
    aos_debug_init();
#endif
    
    int ret = partition_init();
    if (ret >= 0) {
        printf("partition_init success partition num %d\r\n", ret);
    }
    aos_kv_init("kv");
}
