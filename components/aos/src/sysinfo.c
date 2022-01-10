/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <drv/tee.h>
#include <yoc/nvram.h>
#include <yoc/sysinfo.h>
// #include <mtb.h>

#ifndef SYSINFO_WEAK
#define SYSINFO_WEAK __attribute__((weak))
#endif

#ifndef CONFIG_SDK_VERSION
#define CONFIG_SDK_VERSION "V7.2.2_20200319"
#endif



SYSINFO_WEAK const char *aos_get_product_model(void)
{
#ifdef CONFIG_PRODUCT_MODEL
    const char *ret = CONFIG_PRODUCT_MODEL;
#else
    static char ret[64];
    int         res;
    res = nvram_get_val("model", ret, sizeof(ret));

    if (res <= 0) {
        return NULL;
    }

#endif
    return (const char *)ret;
}

char *aos_get_os_version(void)
{
    char *ret = CONFIG_SDK_VERSION;

    return (char *)ret;
}

char *aos_get_app_version(void)
{
    int32_t     ret;
    static char str_version[65] = {
        0,
    };
    uint32_t len = sizeof(str_version) - 1;

    if (str_version[0] == 0) {
#ifdef CONFIG_TEE_CA
        ret = csi_tee_get_sys_os_version((uint8_t *)str_version, &len);
#else
        extern int get_app_version(uint8_t *out, uint32_t *out_len);
        ret = get_app_version((uint8_t *)str_version, &len);
#endif
        if (ret != 0 || len >= sizeof(str_version)) {
            str_version[0] = 0;
            return NULL;
        }

        str_version[len] = 0;
    }

    return str_version;
}

SYSINFO_WEAK const char *aos_get_kernel_version(void)
{
    const char *ret = "AOS-R-1.2.0";
    return ret;
}

const char *aos_get_device_name(void)
{
    int ret;
    static char namebuf[64] = {0};

    if (namebuf[0] == 0) {
        memset(namebuf, 0, sizeof(namebuf));
        ret = nvram_get_val("DEVICENAME", namebuf, sizeof(namebuf));
        if (ret < 0) {
            return NULL;
        }
    }
    return namebuf;
}

char *aos_get_device_id(void)
{
    int ret;
    static char cidbuf[64] = {0};

    if (cidbuf[0] == 0) {
        memset(cidbuf, 0, sizeof(cidbuf));
#ifdef CONFIG_DEVICEID_FROM_KV
        ret = nvram_get_val("device_id", cidbuf, sizeof(cidbuf));
#else
#ifdef CONFIG_TEE_CA
        {
            uint32_t len = sizeof(cidbuf) - 1;
            ret = csi_tee_get_cid((uint8_t *)cidbuf, (uint32_t *)&len);
        }
#else
        ret = nvram_get_val("device_id", cidbuf, sizeof(cidbuf));
#endif /* CONFIG_TEE_CA */
#endif /* CONFIG_DEVICEID_FROM_KV */
        if (ret < 0) {
            return NULL;
        }
    }
    return cidbuf;
}

SYSINFO_WEAK const char *aos_get_imsi(void)
{
    static char ret[32] = {0};
    int         res;
    res = nvram_get_val("imsi", ret, sizeof(ret));

    if (res <= 0) {
        return NULL;
    }

    return (const char *)ret;
}

SYSINFO_WEAK const char *aos_get_imei(void)
{
    static char ret[32] = {0};
    int         res;
    res = nvram_get_val("imei", ret, sizeof(ret));

    if (res <= 0) {
        return NULL;
    }

    return (const char *)ret;
}
