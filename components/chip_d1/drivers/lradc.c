/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */
#include <drv/lradc.h>
#include <sunxi_hal_lradc.h>

lradc_status_t csi_lradc_init(void)
{
    return hal_lradc_init();
}

lradc_status_t csi_lradc_deinit(void)
{
    return hal_lradc_deinit();
}

lradc_status_t csi_lradc_register_callback(lradc_callback_t callback)
{
    return hal_lradc_register_callback(callback);
}