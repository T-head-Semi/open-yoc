/*
 * Copyright (C) 2015-2019 Alibaba Group Holding Limited
 */

#include "k_api.h"

#if defined (AOS_COMP_DEBUG) && AOS_COMP_DEBUG
#include "debug/dbg.h"
#endif

void k_err_proc_debug(kstat_t err, char *file, int line)
{
#if defined (AOS_COMP_DEBUG) && AOS_COMP_DEBUG
    aos_debug_fatal_error(err, file, line);
#endif

    if (g_err_proc != NULL) {
        g_err_proc(err);
    }
}
