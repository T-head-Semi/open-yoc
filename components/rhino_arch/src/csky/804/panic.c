/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <csi_core.h>
#include "aos/debug.h"
#include <debug/debug_api.h>

#if (AOS_COMP_DEBUG > 0)
extern volatile uint32_t g_crash_steps;
#define DEBUG_CONFIG_PANIC 1

#if (DEBUG_CONFIG_PANIC > 0)

typedef struct {
    /* R0~R31: regular registers */
    uint32_t Regs[32];
    /* FR0~FR15: float registers */
    uint32_t FloatRegs[16];
    /* EPSR: PSR when panic */
    uint32_t EPSR;
    /* EPC:  PC when panic */
    uint32_t EPC;
} fault_context_t;

fault_context_t *g_fcontext;

#define LR_IDX 15
#define SP_IDX 14

void panicGetCtx(void *context, char **pPC, char **pLR, int **pSP)
{
    if ( context != NULL ) {
        g_fcontext = (fault_context_t *)context;
    }

    *pSP = (int *)g_fcontext->Regs[SP_IDX];
    *pLR = (char *)g_fcontext->Regs[LR_IDX];
    *pPC = (char *)g_fcontext->EPC;
}


void panicShowRegs(void *context, int (*print_func)(const char *fmt, ...))
{
    int              idx;
    uint32_t         vec       = (__get_PSR() & PSR_VEC_Msk) >> PSR_VEC_Pos;
    fault_context_t *trap_info = (fault_context_t *)context;

    print_func = print_func == NULL ? printf : print_func;

    print_func("========== Regs info  ==========\r\n");

    print_func("CPU Exception: NO.%u\r\n", vec);
    print_func("\r\n");

    print_func("PC\t:0x%08X\r\n",   trap_info->EPC);
    print_func("LR\t:0x%08X\r\n",   trap_info->Regs[LR_IDX]);
    print_func("SP\t:0x%08X\r\n",   trap_info->Regs[SP_IDX]);
    print_func("EPSR\t:0x%08X\r\n", trap_info->EPSR);

    for (idx = 0; idx < 32; idx++) {
        print_func("R%d\t:0x%08X\t", idx, trap_info->Regs[idx]);

        if ((idx % 4) == 3) {
            print_func("\r\n");
        }
    }
}

void exceptionHandler(void *context)
{
    int *ptr = context;
    int  fpcnt = 0;
    static int  CPSR = 0;

    g_crash_steps++;
    if (g_crash_steps > 1) {
        context = NULL;
    } else {
#if defined(FPU_AVL) && (FPU_AVL)
        fpcnt += 2;
#if (NEON_AVL)
        fpcnt += 64;
#else
        fpcnt += 32;
#endif
#endif
        ptr += fpcnt;
        /* ptr point to context_t->CPSR now */
        CPSR = ptr[0];
        CPSR = CPSR;
    }

#if defined (RHINO_CONFIG_CPU_NUM) && (RHINO_CONFIG_CPU_NUM > 1)
    cpu_freeze_others();
#endif

#if (DEBUG_CONFIG_PANIC > 0)
    panicHandler(context);
#else
    printf("exception occur!\n");
#endif

    while (1)
        ;
}
#endif

#if 0
void debug_watchpoint_init(void)
{
    /* to be continued..*/
}

int debug_watch_on(struct watchpoint *watchpoint)
{
    /* to be continued..*/
    return 0;
}

int debug_watch_off(struct watchpoint *watchpoint)
{
    /* to be continued..*/
    return 0;
}
#endif

#endif
