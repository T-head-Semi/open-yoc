#include <stdbool.h>
#include <aos/kv.h>
#include <uservice/uservice.h>
#include <yoc/partition.h>
#include <yoc/init.h>
#include "board.h"
#include "app_main.h"
#ifdef AOS_COMP_DEBUG
#include <debug/dbg.h>
#endif

#define TAG "init"

void board_yoc_init(void)
{
    board_init();
    console_init(CONSOLE_UART_IDX, 115200, 512);
    board_cli_init();
    printf("###YoC###[%s,%s]\n", __DATE__, __TIME__);
    printf("cpu clock is %dHz\n", soc_get_cpu_freq(0));
    event_service_init(NULL);
#ifdef AOS_COMP_DEBUG
    aos_debug_init();
#endif
    ulog_init();
    aos_set_log_level(AOS_LL_DEBUG);
}
