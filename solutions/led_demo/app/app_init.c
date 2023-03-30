#include <stdbool.h>
#include <aos/kv.h>
#include <debug/dbg.h>
#include <uservice/uservice.h>
#include <yoc/partition.h>
#include <yoc/init.h>
#include "board.h"

#define TAG "init"

static void stduart_init(void)
{
    extern void console_init(int idx, uint32_t baud, uint16_t buf_size);
    console_init(CONSOLE_UART_IDX, CONFIG_CLI_USART_BAUD, CONFIG_CONSOLE_UART_BUFSIZE);
}

extern void board_cli_init();
void board_yoc_init(void)
{
    board_init();
    stduart_init();
    printf("###YoC###[%s,%s]\n", __DATE__, __TIME__);
    printf("cpu clock is %dHz\n", soc_get_cpu_freq(0));
    board_cli_init();
    event_service_init(NULL);
    ulog_init();
    aos_set_log_level(AOS_LL_INFO);
}
