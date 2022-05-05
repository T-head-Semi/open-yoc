/******************************************************************************
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <aos/aos.h>
#ifndef CONFIG_CHIP_D1
#include <sdmmc_host.h>
#include <sdmmc_spec.h>
#else
#include "sdio.h"
#include "sdmmc.h"
#include "card.h"
#include "hal_sdhost.h"
#include "sys/sys_debug.h"
#endif
#include "osdep_service.h"
#include "wifi_io.h"

#include <device_lock.h>

#define DEBUG_SDIO 0
#define CONFIG_SDC_ID 1

/* test wifi driver */
#define ADDR_MASK 0x10000
#define LOCAL_ADDR_MASK 0x00000

#define dbg_host printf

#ifndef CONFIG_CHIP_D1
static sdio_card_t SDIO_Card;
#else
static struct mmc_card * SDIO_Card;
#endif

struct rtl_sdio_func *wifi_sdio_func = NULL;
extern struct osdep_service_ops osdep_service;
static const char* TAG = "WIFI_IO";


void rtl8723ds_set_invalid();

typedef void(*rtl8723ds_irq_handler)(struct rtl_sdio_func *);
static void *func_data;
static rtl8723ds_irq_handler irq_handler;
static _mutex claim_host_mutex;
static int sdio_thread_running = 0;
int sdio_thread_exit = 1;
// _sema irq_sema;
aos_event_t  g_wifi_irq_event;
#if  !defined(CONFIG_CHIP_YUNVOICE_CPU0) && !defined(CONFIG_CHIP_D1)
static int g_idx = 0;
#endif
//#undef CONFIG_CHIP_YUNVOICE_CPU0

#ifdef CONFIG_CHIP_D1
static void SDIO_PowerON(struct mmc_card *card)
{
    SDC_InitTypeDef sdc_param = { 0 };

#ifdef CONFIG_DETECT_CARD
    sdc_param.cd_mode = CARD_ALWAYS_PRESENT;
#endif
    sdc_param.debug_mask = ROM_WRN_MASK|ROM_ERR_MASK;
#ifdef CONFIG_SDC_DMA_USED
    sdc_param.dma_use = 1;
#else
    sdc_param.dma_use = 0;
#endif
    card->host = hal_sdc_create(CONFIG_SDC_ID, &sdc_param);

    hal_sdc_init(card->host);
}

static void SDIO_PowerOFF(struct mmc_card *card)
{
    hal_sdc_deinit(CONFIG_SDC_ID);
    hal_sdc_destroy(card->host);
}

static int SDIO_Initialize_Cards(struct mmc_card *card)
{
    int ret;

    ret = mmc_rescan(card, CONFIG_SDC_ID);
    if (ret) {
        return ret;
    }

    return 0;
}

static int SDIO_DeInitialize_Cards(struct mmc_card *card)
{
    int ret;

    ret = mmc_card_deinit(card);
    if (ret) {
        return ret;
    }

    return 0;
}
#endif

void rtl8723ds_irq_callback(void *prim)
{
#ifndef CONFIG_CHIP_D1
    SDMMCHOST_Disable_Interrupt(g_idx);
#endif
    aos_event_set(&g_wifi_irq_event, 1, AOS_EVENT_OR);
}

void sdio_irq_thread(void *arg)
{
    sdio_thread_exit = 0;

#ifdef CONFIG_CHIP_YUNVOICE_CPU0
    LOGD(TAG, "sdio_irq_thread enter the LOOP routine");
    while (sdio_thread_running) {
        device_mutex_lock(RT_DEV_LOCK_WLAN);
        osdep_service.rtw_mutex_get(&claim_host_mutex);
        if (irq_handler)
            irq_handler(func_data);
        osdep_service.rtw_mutex_put(&claim_host_mutex);
        device_mutex_unlock(RT_DEV_LOCK_WLAN);
        aos_msleep(10);
    }
#else
    LOGD(TAG, "sdio_irq_thread enter IRQ routine");

#ifndef CONFIG_CHIP_D1
    SDMMCHOST_RegisterInterrupt(rtl8723ds_irq_callback);
#else
    sdio_claim_irq(SDIO_Card->sdio_func[0], (sdio_irq_handler_t *)rtl8723ds_irq_callback);
#endif
    while (sdio_thread_running) {
        uint32_t act_flags;
        // if (osdep_service.rtw_down_timeout_sema(&irq_sema, -1) == _SUCCESS) {
        if (aos_event_get(&g_wifi_irq_event, 1, AOS_EVENT_OR_CLEAR, &act_flags, -1) == 0) {

            if (sdio_thread_running) {
                device_mutex_lock(RT_DEV_LOCK_WLAN);
                osdep_service.rtw_mutex_get(&claim_host_mutex);
                if (irq_handler) {
                    irq_handler(func_data);
                }
#ifndef CONFIG_CHIP_D1
                SDMMCHOST_Enable_Interrupt(g_idx);
#endif
                osdep_service.rtw_mutex_put(&claim_host_mutex);
                device_mutex_unlock(RT_DEV_LOCK_WLAN);

            }
        } else {
            //printf("sdio_irq_thread down sema failed");
            //aos_msleep(10);
        }
    }
#endif
    sdio_thread_exit = 1;
    LOGD(TAG, "sdio_irq_thread exit\n");
}

void print_buf(char *buf, int len)
{
    int j = 0;

    for (j = 0; j < len; j++) {
        if (0 == (j & (16 - 1))) { //print 16 hex every line
            printf("\n%06x ", j);
        }

        printf(" %02x", buf[j] & 0xff);
    }

    printf("\n");

}


/**
    Register SDIO interrupt function
*/
int __sdio_claim_irq(struct rtl_sdio_func *func, void(*handler)(struct rtl_sdio_func *))
{
    //LOGD(TAG, "%s", __FUNCTION__);
    irq_handler = handler;
    func_data = func;

    aos_task_t task_handle;
    sdio_thread_running = 1;
    if (0 != aos_task_new_ext(&task_handle, "sdio_irq", sdio_irq_thread, NULL, 1024 * 6, AOS_DEFAULT_APP_PRI + 5)) {
        LOGE("SDIO", "Create sdio_irq_thread task failed.");
    }

    return 0;
}

/**
    Release SDIO interrupt function
*/
int __sdio_release_irq(struct rtl_sdio_func *func)
{
    LOGD(TAG, "%s", __FUNCTION__);

#ifdef CONFIG_CHIP_D1
    sdio_release_irq(SDIO_Card->sdio_func[0]);
#else
    SDMMCHOST_RegisterInterrupt(NULL);
#endif
    irq_handler = NULL;
    sdio_thread_running = 0;
    aos_event_set(&g_wifi_irq_event, 1, AOS_EVENT_OR);

    return 0;
}

#if 0
/*! @brief SDMMC host detect card configuration */
static const sdmmchost_detect_card_t sdio_CardDetect = {
    .cdType = kSDMMCHOST_DetectCardByHostCD,
    .cdTimeOut_ms = (~0U),
    .card_inserted = sdio_inserted_callback,
    .card_removed = sdio_removed_callback,
};
#endif

int __sdio_card_reset()
{
#ifdef CONFIG_CHIP_D1
    SDIO_PowerOFF(SDIO_Card);
    SDIO_PowerON(SDIO_Card);
#else
    SDIO_CardReset(SDIO_Card);
#endif
    return 0;
}

int __sdio_bus_probe(int idx)
{
    LOGD(TAG, "%s", __FUNCTION__);
#ifndef CONFIG_CHIP_D1
    memset(&SDIO_Card, 0, sizeof(SDIO_Card));
    SDIO_Card.usrParam.cd = NULL;
#ifdef CONFIG_CSI_V2 
    if(idx == 0)
        SDIO_Card.host.base  = (sdif_handle_t)DW_SDIO0_BASE;
    else if(idx == 1)
        SDIO_Card.host.base  = (sdif_handle_t)DW_SDIO1_BASE;
#else
    if(idx == 0)
        SDIO_Card.host.base  = (sdif_handle_t)CSKY_SDIO0_BASE;
    else if(idx == 1)
        SDIO_Card.host.base  = (sdif_handle_t)CSKY_SDIO1_BASE;
#endif

    int ret = 0;
    if (kStatus_Success != (ret= SDIO_Init(&SDIO_Card))) {
        LOGE(TAG, "SDIO Init failed ret=%d", ret);
        rtl8723ds_set_invalid();
        return -1;
    } else {
        LOGD(TAG, "SDIO Init success");
        aos_msleep(100); /** wait card reset */
    }

    // set force to 512
    if (sdio_set_block_size(SDIO_Card, FN1, 512) != 0) {
        LOGE(TAG, "SDIO_SetBlockSize 512 error");
        rtl8723ds_set_invalid();
        return  1;
    } else {
        LOGD(TAG, "SDIO_SetBlockSize 512 success");
    }
#else
    struct mmc_card *card;
    SDCard_InitTypeDef card_param = { 0 };
    card_param.type = MMC_TYPE_SDIO;
    card_param.debug_mask = ROM_INF_MASK | ROM_WRN_MASK | ROM_ERR_MASK;

    mmc_card_create(CONFIG_SDC_ID, &card_param);
    card = mmc_card_open(CONFIG_SDC_ID);
    SDIO_PowerON(card);

    SDIO_Initialize_Cards(card);

    sdio_enable_func(card, FN1);

    aos_msleep(1);
    mmc_card_close(CONFIG_SDC_ID);

    SDIO_Card = card;

    sdio_set_block_size(card, FN1, 512);
#endif

    wifi_sdio_func = aos_zalloc(sizeof(struct rtl_sdio_func)); //TBD here
    wifi_sdio_func->num = 1;
    aos_event_new(&g_wifi_irq_event, 0);
    // osdep_service.rtw_init_sema(&irq_sema, 0);
    osdep_service.rtw_mutex_init(&claim_host_mutex);

    return 0;
}


int __sdio_bus_remove()
{
    LOGD(TAG, "%s", __FUNCTION__);
    aos_free(wifi_sdio_func);

#ifdef CONFIG_CHIP_D1
    if (!SDIO_Card)
        return -1;
    sdio_disable_func(SDIO_Card, FN1);

    SDIO_DeInitialize_Cards(SDIO_Card);

    SDIO_PowerOFF(SDIO_Card);

    mmc_card_delete(CONFIG_SDC_ID);
#else
    SDIO_Deinit(&SDIO_Card);
#endif

    return 0;
}


/**
    Get the SDIO privilege before IO
*/
void __sdio_claim_host(struct rtl_sdio_func *func)
{
    osdep_service.rtw_mutex_get(&claim_host_mutex);
    return;
}

/**
    Release the SDIO privilege after IO
*/
void __sdio_release_host(struct rtl_sdio_func *func)
{
    osdep_service.rtw_mutex_put(&claim_host_mutex);
    return;
}



static int __sdio_enable_func(struct rtl_sdio_func *func)
{
    LOGD(TAG, "Enable SDIO Func");

#ifndef CONFIG_CHIP_D1
    if (func->num == 1) {
        SDIO_SelectIO(&SDIO_Card, kSDIO_FunctionNum1);
    } else {
        SDIO_SelectIO(&SDIO_Card, kSDIO_FunctionNum0);
    }

    SDIO_EnableIO(&SDIO_Card, func->num, 1);
#else
    sdio_enable_func(SDIO_Card, 1);
#endif

    return 0;
}


int __sdio_disable_func(struct rtl_sdio_func *func)
{
    LOGD(TAG, "Disable SDIO Func ");
    sdio_disable_func(SDIO_Card, 1);
    return 0;
}

//size < 512, using byte mode, others using block mode
int __sdio_memcpy_fromio(struct rtl_sdio_func *func, void *dst, unsigned int addr, int count)
{
#ifndef CONFIG_CHIP_D1
    if (count >= 512) {
        if (count % 512 != 0)
            LOGE(TAG, "warning sdio rx need byte mode transfer %d bytes", count % 512);
        return sdio_io_rw_extended_block(func, 0, addr, 1, dst, count);
    } else {
        return sdio_io_rw_extended_byte(func, 0, addr, 1, dst, count);
    }
#else
    return sdio_memcpy_fromio(SDIO_Card, 1, dst, addr, count);
#endif
}

int __sdio_memcpy_toio(struct rtl_sdio_func *func, unsigned int addr, void *src, int count)
{
#ifndef CONFIG_CHIP_D1
    int ret = 0;

    if (count >= 512) {
        if (count % 512 != 0)
            LOGE(TAG, "warning sdio tx need byte mode transfer %d bytes", count % 512);
        ret = sdio_io_rw_extended_block(func, 1, addr, 1, src, count);
    } else {
        ret = sdio_io_rw_extended_byte(func, 1, addr, 1, src, count);
    }

    return ret;
#else
    return sdio_memcpy_toio(SDIO_Card, 1, addr, src, count);
#endif
}

static uint8_t g_tmp_buf[64] __attribute__ ((aligned(64)));
u8 __sdio_readb(struct rtl_sdio_func *func, unsigned int addr, int *err_ret)
{
#ifndef CONFIG_CHIP_D1
    status_t ret;

    ret = SDIO_IO_Read_Direct(&SDIO_Card, func->num, addr, g_tmp_buf);

    if (ret != kStatus_Success) {
        LOGE(TAG, "sdio_io_readb error");

        if (err_ret) {
            *err_ret = -1;
        }

        return *(u8 *)g_tmp_buf;
    }

    if (err_ret) {
        *err_ret = 0;
    }
#else
    g_tmp_buf[0] = sdio_readb(SDIO_Card, wifi_sdio_func->num, addr, err_ret);
#endif

#if DEBUG_SDIO
    printf("[U8][R][%x=%02x]\n", addr, *(u8 *)g_tmp_buf);
#endif


    return g_tmp_buf[0];
}

u16 __sdio_readw(struct rtl_sdio_func *func, unsigned int addr, int *err_ret)
{
    int ret;

    if (err_ret) {
        *err_ret = 0;
    }

    ret = __sdio_memcpy_fromio(func, g_tmp_buf, addr, 2);

    if (ret) {
        if (err_ret) {
            *err_ret = ret;
        }
    }

#if DEBUG_SDIO
    printf("[U16][R][%x=%04x]\n", addr, *(u16 *)g_tmp_buf);
#endif
    uint16_t *res = (uint16_t*)g_tmp_buf;

    return *res;
}


u32 __sdio_readl(struct rtl_sdio_func *func, unsigned int addr, int *err_ret)
{
    int ret;

    if (err_ret) {
        *err_ret = 0;
    }

    ret = __sdio_memcpy_fromio(func, g_tmp_buf, addr, 4);

    if (ret) {
        if (err_ret) {
            *err_ret = ret;
        }
    }

#if DEBUG_SDIO
    printf("[U32][R][%x=%08x]\n", addr, *(u32 *)g_tmp_buf);
#endif
    uint32_t *res = (uint32_t*)g_tmp_buf;

    return *res;
}


void __sdio_writeb(struct rtl_sdio_func *func, u8 b, unsigned int addr, int *err_ret)
{
#ifndef CONFIG_CHIP_D1
    uint8_t data_tmp = b;
    status_t ret;

#if DEBUG_SDIO
    printf("[U8][W][%x=%02x]\n", addr, *(u8 *)g_tmp_buf);
#endif

    ret = SDIO_IO_Write_Direct(&SDIO_Card, func->num, addr, &data_tmp, 0);

    if (err_ret) {
        *err_ret = 0;
    }

    if (ret != kStatus_Success) {
        if (err_ret) {
            *err_ret = 1;
        }

        LOGE(TAG, "sdio_io_writeb error reg 0x%x", addr);
    }
#else
    int32_t err;
    sdio_writeb(SDIO_Card, 1, b,  addr, &err);
    if (err_ret) {
        *err_ret = err;
    }
#endif
}

void __sdio_writew(struct rtl_sdio_func *func, u16 b, unsigned int addr, int *err_ret)
{
    int ret;

    if (err_ret) {
        *err_ret = 0;
    }

#if DEBUG_SDIO
    printf("[U16][W][%x=%04x]\n", addr, *(u16 *)g_tmp_buf);
#endif
    uint16_t *res = (uint16_t*)g_tmp_buf;

    *res = b;

    ret = __sdio_memcpy_toio(func, addr, res, 2);

    if (ret) {
        if (err_ret) {
            *err_ret = ret;
        }

        return;
    }

    return;

}

void __sdio_writel(struct rtl_sdio_func *func, u32 b, unsigned addr, int *err_ret)
{

    int ret;

    if (err_ret) {
        *err_ret = 0;
    }

#if DEBUG_SDIO
    printf("[U32][W][%x=%08x]\n", addr, *(u32 *)g_tmp_buf);
#endif
    uint32_t *res = (uint32_t*)g_tmp_buf;

    *res = b;

    ret = __sdio_memcpy_toio(func, addr, res, 4);

    if (ret) {
        if (err_ret) {
            *err_ret = ret;
        }

        return;
    }

    return;
}

/////////////////////////////// Porting code end //////////////////////////////////

int wifi_read(struct rtl_sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
    int err;

    __sdio_claim_host(func);
    err = __sdio_memcpy_fromio(func, pdata, addr, cnt);

    if (err) {
        LOGE(TAG, "%s: FAIL(%d)! ADDR=%#x Size=%d", __func__, err, addr, cnt);
    }

    __sdio_release_host(func);


    return err;
}

int wifi_write(struct rtl_sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
    int err;
    u32 size;

    __sdio_claim_host(func);
    size = cnt;
    err = __sdio_memcpy_toio(func, addr, pdata, size);

    if (err) {
        LOGE(TAG, "%s: FAIL(%d)! ADDR=%#x Size=%d(%d)", __func__, err, addr, cnt, size);
    }

    __sdio_release_host(func);

    return err;
}

u8 wifi_readb(struct rtl_sdio_func *func, u32 addr) //52
{
    int err;
    u8 ret = 0;

    __sdio_claim_host(func);
    ret = __sdio_readb(func, ADDR_MASK | addr, &err);
    __sdio_release_host(func);

    if (err) {
        LOGE(TAG, "%s: FAIL!(%d) addr=0x%05x", __func__, err, addr);
    }

    return ret;
}

u16 wifi_readw(struct rtl_sdio_func *func, u32 addr)
{
    int err;
    u16 v;

    __sdio_claim_host(func);
    v = __sdio_readw(func, ADDR_MASK | addr, &err);
    __sdio_release_host(func);

    if (err) {
        LOGE(TAG, "%s: FAIL!(%d) addr=0x%05x", __func__, err, addr);
    }

    return  v;
}

u32 wifi_readl(struct rtl_sdio_func *func, u32 addr)
{
    int err;
    u32 v;

    __sdio_claim_host(func);
    v = __sdio_readl(func, ADDR_MASK | addr, &err);
    __sdio_release_host(func);

    return  v;
}

void wifi_writeb(struct rtl_sdio_func *func, u32 addr, u8 val)
{
    int err;
    __sdio_claim_host(func);
    __sdio_writeb(func, val, ADDR_MASK | addr, &err);
    __sdio_release_host(func);
}

void wifi_writew(struct rtl_sdio_func *func, u32 addr, u16 v)
{
    int err;
    __sdio_claim_host(func);
    __sdio_writew(func, v, ADDR_MASK | addr, &err);
    __sdio_release_host(func);
}

void wifi_writel(struct rtl_sdio_func *func, u32 addr, u32 v)
{
    int err;

    __sdio_claim_host(func);
    __sdio_writel(func, v, ADDR_MASK | addr, &err);
    __sdio_release_host(func);
}

u8 wifi_readb_local(struct rtl_sdio_func *func, u32 addr)
{
    int err;
    u8 ret = 0;
    ret = __sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);

    return ret;
}

void wifi_writeb_local(struct rtl_sdio_func *func, u32 addr, u8 val)
{
    int err;
    __sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
}


SDIO_BUS_OPS rtw_sdio_bus_ops = {
    __sdio_bus_probe,
    __sdio_bus_remove,
    __sdio_enable_func,
    __sdio_disable_func,
    NULL,
    NULL,
    __sdio_claim_irq,
    __sdio_release_irq,
    __sdio_claim_host,
    __sdio_release_host,
    __sdio_readb,
    __sdio_readw,
    __sdio_readl,
    __sdio_writeb,
    __sdio_writew,
    __sdio_writel,
    __sdio_memcpy_fromio,
    __sdio_memcpy_toio
};

