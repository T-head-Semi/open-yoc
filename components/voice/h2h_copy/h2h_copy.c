#include <stdio.h>
#include <string.h>
#include <ipc.h>

#ifdef CONFIG_CSI_V1
#include <drv/dmac.h>
#include <drv/timer.h>
#include <drv/irq.h>
#else
#include <drv/dma.h>
#include <drv/timer.h>
#include <drv/irq.h>
#endif

#include <soc.h>
#include "voice_def.h"

typedef struct {
    void         *src;
    void         *dst;
    int           size;
    volatile int  flag;
} h2h_mem_t;

#ifdef CONFIG_CSI_V1
#define DMAC1_CFG(addr)  (*(volatile uint32_t *)((CSKY_DMAC1_BASE + (addr))))
#else
#define DMAC1_CFG(addr)  (*(volatile uint32_t *)((DW_DMAC1_BASE + (addr))))
#endif

#define DMA_REG_SARx            0x0
#define DMA_REG_DARx            0x8
#define DMA_REG_LLPX            0x10
#define DMA_REG_CTRLax          0x18
#define DMA_REG_CTRLbx          0x1c
#define DMA_REG_CFGax           0x40
#define DMA_REG_CFGbx           0x44

#define DMA_REG_RawTfr          0x2c0
#define DMA_REG_RawBlock        0x2c8
#define DMA_REG_RawSrcTran      0x2d0
#define DMA_REG_RawDstTran      0x2d8
#define DMA_REG_RawErr          0x2e0

#define DMA_REG_StatusTfr       0x2e8
#define DMA_REG_StatusBlock     0x2f0
#define DMA_REG_StatusSrcTran   0x2f8
#define DMA_REG_StatusDstTran   0x300
#define DMA_REG_StatusErr       0x308

#define DMA_REG_MaskTfr         0x310
#define DMA_REG_MaskBlock       0x318
#define DMA_REG_MaskSrcTran     0x320
#define DMA_REG_MaskDstTran     0x328
#define DMA_REG_MaskErr         0x330

#define DMA_REG_ClearTfr        0x338
#define DMA_REG_ClearBlock      0x340
#define DMA_REG_ClearSrcTran    0x348
#define DMA_REG_ClearDstTran    0x350
#define DMA_REG_ClearErr        0x358
#define DMA_REG_StatusInt       0x360

#define DMA_REG_ReqSrc          0x368
#define DMA_REG_ReqDst          0x370
#define DMA_REG_SglReqSrc       0x378
#define DMA_REG_SglReqDst       0x380
#define DMA_REG_LstReqSrc       0x388
#define DMA_REG_LstReqDst       0x390

#define DMA_REG_Cfg             0x398
#define DMA_REG_ChEn            0x3a0

static h2h_mem_t g_shm __attribute__((section(".tcm1")));
static int32_t g_h2h_dma_channel;

static void dma1_config(int dma_ch)
{
    uint32_t val = DMAC1_CFG(DMA_REG_CTRLax + (dma_ch * 0x58));
    /*set inc add*/
    val &= ~((3 << 7) | (3 << 9) | (7 << 11) | (7 << 14) | (3 << 23) | (3 << 25));
    /*set mem to mem*/
    val &= ~(7 << 20);
    /*set msize = 4*/
    val |= 1 << 11 | 1 << 14;
    /*set hb*/
    val |= 1 << 23;
    /*int disable*/
    val &= ~1;
    DMAC1_CFG(DMA_REG_CTRLax + (dma_ch * 0x58)) = val;
    val = DMAC1_CFG(DMA_REG_CFGax + (dma_ch * 0x58));
    /*set hs softwoare*/
    val |= 3 << 10;
    DMAC1_CFG(DMA_REG_CFGax + (dma_ch * 0x58)) = val;
    val = DMAC1_CFG(DMA_REG_Cfg);
    if (val == 0) {
        DMAC1_CFG(DMA_REG_Cfg) = 1;
    }
}

static void dma1_copy(int dma_ch, void *src, void *dst, uint32_t num_bytes)
{
    uint32_t val = DMAC1_CFG(DMA_REG_CTRLax+ (dma_ch * 0x58));
    val &= ~((0x7 << 1) | (0x7 << 4));
    if(num_bytes % 4 == 0) {
        num_bytes /= 4;
        val |= ((0x2 << 1) | (0x2 << 4));
    } else if(num_bytes % 2 == 0) {
        num_bytes /= 2;
        val |= ((0x1 << 1) | (0x1 << 4));
    }
    /*set width*/
    DMAC1_CFG(DMA_REG_CTRLax+ (dma_ch * 0x58)) = val;
    DMAC1_CFG(DMA_REG_SARx + (dma_ch * 0x58)) = (int32_t)src;
    DMAC1_CFG(DMA_REG_DARx + (dma_ch * 0x58)) = (int32_t)dst;
    /*set block cnt*/
    DMAC1_CFG(DMA_REG_CTRLbx+ (dma_ch * 0x58)) = num_bytes;
    /*start dma*/
    DMAC1_CFG(DMA_REG_ChEn) |= ((1 << dma_ch) << 8) | (1 << dma_ch);
    while(!(DMAC1_CFG(DMA_REG_RawBlock) & (1 << dma_ch)));
    DMAC1_CFG(DMA_REG_ClearBlock) |=  (1 << dma_ch);

}

#ifdef CONFIG_CSI_V1
static timer_handle_t g_time_hdl;
static void timer_event(int32_t idx, timer_event_e event)   ///< Pointer to \ref timer_event_cb_t : TIMER Event call back.
{
    csi_timer_stop(g_time_hdl);
    dma1_copy(g_h2h_dma_channel, g_shm.src,  g_shm.dst,  g_shm.size);
    g_shm.flag = 1;
}

void h2h_copy_init(void)
{
    /*must used dma1*/
    for (int i = CONFIG_PER_DMAC0_CHANNEL_NUM; i < CONFIG_DMA_CHANNEL_NUM; i++) {
        g_h2h_dma_channel = csi_dma_alloc_channel_ex(i);
        if (g_h2h_dma_channel != -1) {
            break;
        }
    }

    g_h2h_dma_channel -= CONFIG_PER_DMAC0_CHANNEL_NUM;
    dma1_config(g_h2h_dma_channel);
    memset(&g_shm, 0x00, sizeof(h2h_mem_t));
    g_time_hdl  = csi_timer_initialize(H2H_TIMER_ID, timer_event);
    for (int i = 0; i < 64; i++) {
        csi_vic_set_prio(i, 3);
    }
    csi_vic_set_prio(H2H_TIMER_IRQ_NUM, 0);
}
#else
static csi_timer_t g_time_hdl;
static void timer_event(csi_timer_t *timer, void *arg)   ///< Pointer to \ref timer_event_cb_t : TIMER Event call back.
{
    csi_timer_stop(&g_time_hdl);
    dma1_copy(g_h2h_dma_channel, g_shm.src,  g_shm.dst,  g_shm.size);
    g_shm.flag = 1;
}

void h2h_copy_init(void)
{
    static csi_dma_ch_t dma_ch;
    memset(&dma_ch, 0, sizeof(dma_ch));
    /*must used dma1*/
    for (int i = 0; i < 8; i++) {
        int ret = csi_dma_ch_alloc(&dma_ch, i, 1);
        if (ret == CSI_OK) {
            g_h2h_dma_channel = dma_ch.ch_id;
            break;
        }
    }

    if (g_h2h_dma_channel == -1) {
        return;
    }
    dma1_config(g_h2h_dma_channel);
    memset(&g_shm, 0x00, sizeof(h2h_mem_t));
    csi_timer_init(&g_time_hdl, H2H_TIMER_ID);
    csi_timer_attach_callback(&g_time_hdl, timer_event, NULL);

    for (int i = 0; i < 64; i++) {
        csi_vic_set_prio(i, 3);
    }
    csi_vic_set_prio(H2H_TIMER_IRQ_NUM, 0);
}
#endif

void *h2h_copy_shm_addr_get(void)
{
    return &g_shm;
}
