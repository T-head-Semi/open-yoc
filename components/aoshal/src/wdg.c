/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 */

#include "soc.h"
#include "drv/wdt.h"
#include "aos/hal/wdg.h"

static csi_wdt_t wdt_handle;

/**
 * This function will initialize the on board CPU hardware watch dog
 *
 * @param[in]  wdg  the watch dog device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_wdg_init(wdg_dev_t *wdg)
{
    int32_t ret;

    if (wdg == NULL) {
        return -1;
    }

    ret = csi_wdt_init(&wdt_handle, wdg->port);

    if (ret < 0) {
        return -1;
    }

    ret = csi_wdt_set_timeout(&wdt_handle, wdg->config.timeout);

    if (ret < 0) {
        return -1;
    }

    ret = csi_wdt_start(&wdt_handle);

    if (ret < 0) {
        return -1;
    }

    return 0;
}
/**
 * Reload watchdog counter.
 *
 * @param[in]  wdg  the watch dog device
 */
void hal_wdg_reload(wdg_dev_t *wdg)
{
    int32_t ret;

    if (wdg == NULL) {
        return;
    }

    ret = csi_wdt_feed(&wdt_handle);

    if (ret < 0) {
        return;
    }
}
/**
 * This function performs any platform-specific cleanup needed for hardware watch dog.
 *
 * @param[in]  wdg  the watch dog device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_wdg_finalize(wdg_dev_t *wdg)
{
    if (wdg == NULL) {
        return -1;
    }

    csi_wdt_uninit(&wdt_handle);

    return 0;
}