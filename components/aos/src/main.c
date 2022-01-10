/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     main.c
 * @brief    CSI Source File for main
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <yoc/sysinfo.h>
#include <yoc/yoc.h>

#ifdef CONFIG_OS_TRACE 
#include <trcTrig.h>
#include <trcKernelPort.h>
#endif


#ifndef CONFIG_INIT_TASK_STACK_SIZE
#define INIT_TASK_STACK_SIZE 8192
#else
#define INIT_TASK_STACK_SIZE CONFIG_INIT_TASK_STACK_SIZE
#endif
static aos_task_t app_task_handle;
extern int main(void);
static void application_task_entry(void *arg)
{
    main();

    aos_task_exit(0);
}

void base_init()
{

}

int pre_main(void)
{
    /* kernel init */
    aos_init();
#ifdef CONFIG_OS_TRACE 
    trace_init_data();
#endif

    /* init task */
    aos_task_new_ext(&app_task_handle, "app_task", application_task_entry,
                     NULL, INIT_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI);

    /* kernel start */
    aos_start();

    return 0;
}
