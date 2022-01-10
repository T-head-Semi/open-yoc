/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drv/adc.h>

#include "soc.h"

#include <aos/kernel.h>
#include <aos/hal/adc.h>
#include <drv/pin.h>
#include <drv/tick.h>
#define ADC_CON_DATA_NUM 1
#define ADC_READ_SIZE 1
#ifdef ADC_MODE_DMA
static csi_dma_ch_t dma_ch;
#endif
extern const csi_pinmap_t adc_pinmap[];
static csi_adc_t g_adc_handle;
static aos_mutex_t g_adc_mutex = {NULL};
static uint16_t g_ref_count = 0;
static uint32_t recv_data[ADC_READ_SIZE];

static uint32_t get_adc_channl_pin(uint32_t adc_channl, const csi_pinmap_t *pinmap)
{
    const csi_pinmap_t *map = pinmap;
    uint32_t ret = 0xFFFFFFFFU;

    while ((uint32_t)map->pin_name != 0xFFFFFFFFU) {
        if (map->channel == adc_channl) {
            ret = (uint32_t)map->pin_name;
            break;
        }
        map++;
    }

    return ret;
}

static uint32_t get_adc_channl_pin_func(uint32_t adc_channl, const csi_pinmap_t *pinmap)
{
    const csi_pinmap_t *map = pinmap;
    uint32_t ret = 0xFFFFFFFFU;

    while ((uint32_t)map->pin_name != 0xFFFFFFFFU) {
        if (map->channel == adc_channl) {
            ret = (uint32_t)map->pin_func;
            break;
        }
        map++;
    }

    return ret;
}

#ifndef ADC_MODE_SYNC
static int adc_event = -1;
static void adc_event_cb(csi_adc_t *adc, csi_adc_event_t event, void *arg)
{
    adc_event = event;

    if (adc_event == ADC_EVENT_CONVERT_COMPLETE){
        *((uint32_t*)arg) = recv_data[0];
    }
}

#endif

int hal_adc_init(adc_dev_t *adc)
{
    int32_t ret = CSI_OK;
    uint32_t pin_name;
    uint32_t pin_func;
    pin_name = get_adc_channl_pin(adc->port,adc_pinmap);
    pin_func = get_adc_channl_pin_func(adc->port,adc_pinmap);
    csi_pin_set_mux(pin_name, pin_func);

    if(g_adc_handle.dev.reg_base == 0 && g_adc_mutex.hdl == NULL){
        aos_mutex_new(&g_adc_mutex);
    }

    aos_mutex_lock(&g_adc_mutex,AOS_WAIT_FOREVER);

    if(g_adc_handle.dev.reg_base == 0){
        ret = csi_adc_init(&g_adc_handle,0);

        if(ret < 0){
            return CSI_ERROR;
        }

        ret = csi_adc_channel_enable(&g_adc_handle, adc->port, true);

        if(ret<0){
        return CSI_ERROR;
        }
    }

    if(g_adc_handle.dev.reg_base == 0){
        aos_mutex_unlock(&g_adc_mutex);
        aos_mutex_free(&g_adc_mutex);
        return CSI_ERROR;
    }

    adc->priv = &g_adc_handle;
    g_ref_count++;
    aos_mutex_unlock(&g_adc_mutex);

    return CSI_OK;
}

int32_t hal_adc_value_get(adc_dev_t *adc,uint32_t *output,uint32_t timeout)
{
    #ifndef ADC_MODE_SYNC
    uint32_t timestart = 0;
    #endif
    int ret = CSI_OK;

    if(adc == NULL){
        return CSI_ERROR;
    }

    if(adc->priv == NULL){
        return CSI_ERROR;
    }

    csi_adc_t *handle = adc->priv;
    aos_mutex_lock(&g_adc_mutex,AOS_WAIT_FOREVER);
    ret = csi_adc_freq_div(handle,128);

    if(ret < 0){
        return CSI_ERROR;
    }

    ret = csi_adc_sampling_time(handle,adc->config.sampling_cycle);

    if(ret < 0){
        return CSI_ERROR;
    }

    ret = csi_adc_continue_mode(handle,true);

    if(ret < 0){
        return CSI_ERROR;
    }

    #ifndef ADC_MODE_SYNC
    ret = csi_adc_attach_callback(handle, adc_event_cb, output);

    if (ret < 0) {
        return CSI_ERROR;
    }

    ret = csi_adc_set_buffer(handle, recv_data, ADC_READ_SIZE);

    if (ret < 0) {
        return CSI_ERROR;
    }

    #endif
    #ifdef ADC_MODE_DMA
    ret = csi_adc_link_dma(handle, &dma_ch);

    if (ret < 0) {
        return CSI_ERROR;
    }

    #endif
    #ifndef ADC_MODE_SYNC
    ret = csi_adc_start_async(handle);

    if (ret < 0) {
        return CSI_ERROR;
    }

    timestart = csi_tick_get_ms();

    while (adc_event != ADC_EVENT_CONVERT_COMPLETE) {
        if ((csi_tick_get_ms() - timestart) > timeout) {
            return CSI_ERROR;
        }
    }

    ret = csi_adc_stop_async(handle);

    if (ret < 0) {
        return CSI_ERROR;
    }

    #ifdef ADC_MODE_DMA
    ret = csi_adc_link_dma(handle, NULL);

    if (ret < 0) {
        return -1;
    }

    #endif
    aos_mutex_unlock(&g_adc_mutex);

    #endif
    #ifdef ADC_MODE_SYNC
    ret = csi_adc_start(handle);

    if(ret<0){
        aos_mutex_unlock(&g_adc_mutex);
        return CSI_ERROR;
    }

    for(int i = 0;i<ADC_CON_DATA_NUM;i++){
        recv_data[i] = csi_adc_read(handle);
    }

    *((uint32_t*)output) = recv_data[0];
    ret = csi_adc_stop(handle);
    aos_mutex_unlock(&g_adc_mutex);

    if(ret<0){
        return CSI_ERROR;
    }
    #endif

    return CSI_OK;
}

int32_t hal_adc_finalize(adc_dev_t *adc)
{
    if(adc == NULL){
        return CSI_ERROR;
    }

    if(adc->priv == NULL){
        return CSI_ERROR;
    }

    aos_mutex_lock(&g_adc_mutex, AOS_WAIT_FOREVER);

    if (g_ref_count > 0) {
        g_ref_count--;
    }
    if (adc->priv && g_ref_count == 0) {
        csi_adc_uninit(adc->priv);
    }

    adc->priv = NULL;
    aos_mutex_unlock(&g_adc_mutex);

    if (adc->priv && g_ref_count == 0) {
        aos_mutex_free(&g_adc_mutex);
        adc->priv = NULL;
    }

    return CSI_OK;
}
