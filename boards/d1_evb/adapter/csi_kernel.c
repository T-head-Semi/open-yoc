/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     csi_rhino.c
 * @brief    the adapter file for the rhino
 * @version  V1.0
 * @date     20. July 2016
 ******************************************************************************/
#ifndef CONFIG_KERNEL_NONE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <csi_kernel.h>
#include <aos/kernel.h>
#include <csi_config.h>
#include <soc.h>

int32_t csi_kernel_sched_lock(void)
{
    return -EOPNOTSUPP;
}

int32_t csi_kernel_sched_unlock(void)
{
    return -EOPNOTSUPP;
}

int32_t csi_kernel_sched_restore_lock(int32_t lock)
{
    return -EOPNOTSUPP;
}

uint32_t csi_kernel_sched_suspend(void)
{
    aos_kernel_sched_suspend();
    return 0;
}

void csi_kernel_sched_resume(uint32_t sleep_ticks)
{
    aos_kernel_sched_resume();
}

k_status_t csi_kernel_intrpt_enter(void)
{
    return aos_kernel_intrpt_enter();
}

k_status_t csi_kernel_intrpt_exit(void)
{
    aos_kernel_intrpt_exit();
    return 0;
}

#endif