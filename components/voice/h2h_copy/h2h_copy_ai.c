#include <stdio.h>
#include <string.h>
#include <ipc.h>
#include <drv/timer.h>
#include <drv/irq.h>
#include <soc.h>

#include "voice_def.h"

typedef struct {
    void         *src;
    void         *dst;
    int           size;
    volatile int  flag;
} h2h_mem_t;

#ifdef CONFIG_CSI_V1
static timer_handle_t g_time_hdl;
#else
static csi_timer_t g_time_hdl;
#endif

static h2h_mem_t *g_shm;

void start_h2h_copy(void *src, void *dst, size_t size)
{
    g_shm->src = src;
    g_shm->dst = dst;
    g_shm->size = size;
    g_shm->flag = 0;

#ifdef CONFIG_CSI_V1
    csi_timer_start(g_time_hdl);
#else
    csi_timer_start(&g_time_hdl, 1);
#endif

}
void wait_h2h_copy_done(void)
{
    //do not add timeout
    while (!g_shm->flag) {
    }
}

void h2h_copy_shm_addr_set(void *addr)
{
    g_shm = addr;
}

void h2h_copy_ai_init(void)
{
#ifdef CONFIG_CSI_V1
    g_time_hdl  = csi_timer_initialize(H2H_TIMER_ID, NULL);
    csi_timer_config(g_time_hdl, TIMER_MODE_RELOAD);

    csi_timer_set_timeout(g_time_hdl, 0);
    // csi_timer_start(g_time_hdl);
#else
    csi_timer_init(&g_time_hdl, H2H_TIMER_ID);
#endif

}
