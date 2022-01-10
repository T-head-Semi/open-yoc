/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if (AOS_COMP_DEBUG > 0)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "k_api.h"
//#include "aos/debug.h"
#include "debug/debug_api.h"
#include "csi_core.h"

#define BACKTRACE_FUNC_LIMIT   0x2000
#define BACKTRACE_SP_LIMIT     0x100
#define BACKTRACE_LVL_LIMIT    64

typedef enum {
    /* subi      	r14, r14, ...;
       ...;
       addi         r14, r14, ...;
       jmp      	r15;
    */
    /* or */
    /* ...;
       jmp      	r15;
    */
    CALL_TYPE_NOPUSH,
    /* nie;
       ipush;
       ...;
       ipop;
       nir;
    */
    CALL_TYPE_ISR,
    /* push      	..., r15;
       ...;
       pop      	..., r15;
    */
    /* or */
    /* push      	..., r15;
       subi      	r14, r14, ...;
       ...;
       addi      	r14, r14, ...;
       pop      	..., r15;
    */
    CALL_TYPE_NORMAL,
    CALL_TYPE_ERROR
} bt_type;

/* reference to cpu_task_stack_init */
typedef struct {
    /* VR0~VR15: float registers */
    uint32_t FloatRegs[16];
    /* R16~R31: high registers */
    uint32_t High_Regs[16];
    /* R0~R13: regular registers */
    uint32_t Regs[14];
    /* R15 */
    uint32_t LR;
    uint32_t PSR;
    uint32_t PC;
} context_t;

//#define OS_PRINT_DBG           printf

extern void krhino_task_deathbed(void);

/* convert int to ascii(HEX)
   while using format % in libc, malloc/free is involved.
   this function avoid using malloc/free. so it works when heap corrupt. */
static char *u_int2str(int num, char *str)
{
    char     index[] = "0123456789ABCDEF";
    uint32_t usnum   = (uint32_t)num;

    str[7] = index[usnum % 16];
    usnum /= 16;
    str[6] = index[usnum % 16];
    usnum /= 16;
    str[5] = index[usnum % 16];
    usnum /= 16;
    str[4] = index[usnum % 16];
    usnum /= 16;
    str[3] = index[usnum % 16];
    usnum /= 16;
    str[2] = index[usnum % 16];
    usnum /= 16;
    str[1] = index[usnum % 16];
    usnum /= 16;
    str[0] = index[usnum % 16];
    usnum /= 16;

    return str;
}
static int backtraceChkPC(uintptr_t pc)
{
    extern char *__stext;
    extern char *__etext;

    uintptr_t addr_start = (uintptr_t)(&__stext);
    uintptr_t addr_end   = (uintptr_t)(&__etext);

    return (pc < addr_end && pc > addr_start);
}

static int backtraceChkSP(uintptr_t sp)
{
    /* need modify linkscript */
    return 1;
}

__attribute__((noinline)) uintptr_t backtraceGetPC()
{
    //To keep such calls from being optimized away
    asm ("");
    return (uintptr_t)__builtin_return_address(0);
}

inline __attribute__ ((__always_inline__)) uintptr_t backtraceGetSP()
{
    uintptr_t SP;
    //To keep such calls from being optimized away
    asm ("");
    __asm__ volatile("mov %0, sp\n" : "=r"(SP));
    return SP;
}

/* find current function caller, update PC and SP
   non-leaf(A non-leaf procedure is one that does call another procedure), return addr must be saved in stack.
   returns: 0  success
            1  success and find buttom
            -1 fail */
int backtrace_nonleaf(uintptr_t *pSP, uintptr_t *pPC,
                      int (*print_func)(const char *fmt, ...))
{
    uintptr_t funcaddr = 0;
    uintptr_t SP = *pSP;
    uintptr_t PC = *pPC;
    uintptr_t LR;
    int       i;
    bt_type   type = CALL_TYPE_ERROR;
    uint16_t  ins16;
    uint32_t  ins32;
    uint32_t  sub;
    uint32_t  framesize = 0; /* 1 for word */
    uint32_t  offset    = 0; /* LR offset in stack frame */

    /* func call ways:
       1. push ..., r15; ...; pop ..., r15;
       2. push ..., r15; subi r14,...; ...; addi r14,...; pop ..., r15;
       3. ...; jmp r15;
       */

    /* 1. scan code */
    for (i = 2; i < BACKTRACE_FUNC_LIMIT; i += 2) {
        if ( backtraceChkPC(PC - i) == 0) {
            if (print_func != NULL) {
                print_func("backtrace : invalid pc.\r\n");
            }
            return -1;
        }

        ins16 = *(uint16_t *)(PC - i);
        ins32 =
            ((uint32_t)ins16 << 16) | ((uint32_t)*(uint16_t *)(PC - i + 2));

        /* find "push16 ..., r15" */
        if ((ins16 & 0xfff0) == 0x14d0 && (ins16 & 0xf) < 9) {
            framesize = (ins16 & 0xf) + 1;
            offset    = 1;
            funcaddr  = PC - i;
            type      = CALL_TYPE_NORMAL;
            break;
        }

        /* check if "jmp r15" */
        if (ins16 == 0x783c) {
            /* no "push" in this func */
            funcaddr  = PC - i + 2;
            framesize = 0;
            offset    = 1;
            type      = CALL_TYPE_NOPUSH;
            break;
        }

        /* find "push32 ..., r15" */
        if ((ins32 & 0xfffffe10) == 0xebe00010
            && (ins32 & 0xf) < 9
            && ((ins32 >> 5) & 7) < 3 ) {
            framesize  = (ins32 & 0xf) + 1;
            framesize += (ins32 >> 5) & 7;
            framesize += (ins32 >> 8) & 1;
            offset     = framesize - (ins32 & 0xf);
            funcaddr   = PC - i;
            type       = CALL_TYPE_NORMAL;
            break;
        }

        /* check if isr enter: "nie" "ipush" */
        if (ins32 == 0x14601462) {
            type = CALL_TYPE_ISR;
            break;
        }

        /* check if isr exit: "nir" "ipop" */
        if (ins32 == 0x14631461) {
            funcaddr  = PC - i + 4;
            framesize = 0;
            offset    = 1;
            type      = CALL_TYPE_NOPUSH;
            break;
        }
    }
#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d type = %d func = %p frsz=%d ofst=%d\n\r",
                 __func__, __LINE__, type, (void *)funcaddr, framesize, offset);
#endif

    if (type == CALL_TYPE_ERROR) {
        /* error branch */
        if (print_func != NULL) {
            print_func("Backtrace fail.\r\n");
        }
        return -1;
    }
    if (type == CALL_TYPE_ISR) {
        /* isr */
        if (print_func != NULL) {
            print_func("backtrace : ^isr entry^\r\n");
        }
        return 1;
    }

    /* 2. scan code, check if in isr handler */
    if (type == CALL_TYPE_NORMAL) {
        for (i = 2; i < 20; i += 2) {
            if ( backtraceChkPC(funcaddr - i) == 0) {
                if (print_func != NULL) {
                    print_func("backtrace : invalid pc.\r\n");
                }
                return -1;
            }

            ins32 = *(uint16_t *)(funcaddr - i);
            ins32 <<= 16;
            ins32 |= *(uint16_t *)(funcaddr - i + 2);
            /* check if isr enter: "nie" + "ipush" */
            if (ins32 == 0x14601462) {
                //type == CALL_TYPE_ISR;
                if (print_func != NULL) {
                    print_func("backtrace : ^isr entry^\r\n");
                }
                return 1;
            }
            /* check if isr exit: "nir" + "ipop" */
            if (ins32 == 0x14631461) {
                break;
            }
        }
    }

    /* 3. scan code, find frame size from "subi r14", while CALL_TYPE_NORMAL */
    if (type == CALL_TYPE_NORMAL) {
        ins16 = *(uint16_t *)funcaddr;
        i = (ins16 & 0x8000) == 0 ? 2 : 4;
        while (i < 20) {
            if (funcaddr + i >= PC) {
                break;
            }

            ins16 = *(uint16_t *)(funcaddr + i);
            ins32 = ((uint32_t)ins16 << 16) | ((uint32_t)*(uint16_t *)(funcaddr + i + 2));
            i += (ins16 & 0x8000) == 0 ? 2 : 4;


            /* find "subi16 r14" */
            if ((ins16 & 0xfce0) == 0x1420) {
                sub = ((ins16 >> 3) & 0x60) + (ins16 & 0x1f);
                framesize += sub;
                continue;
            }

            /* find "subi32 r14" */
            if ((ins32 & 0xfffff000) == 0xe5ce1000) {
                sub = (ins32 & 0xfff) + 1;
                if (sub % 4 == 0) {
                    framesize += sub/4;
                    continue;
                }
            }

            /* none stack open instruction */
            // break;
        }
    }

    /* 4. scan code, find frame size from "subi r14", while CALL_TYPE_NOPUSH */
    if (type == CALL_TYPE_NOPUSH) {
        for (i = 2; i < BACKTRACE_FUNC_LIMIT; i += 2) {
            if (PC - i <= funcaddr) {
                break;
            }

            ins16 = *(uint16_t *)(PC - i);
            ins32 =
                ((uint32_t)ins16 << 16) | ((uint32_t)*(uint16_t *)(PC - i + 2));

            /* find "subi16 r14" */
            if ((ins16 & 0xfce0) == 0x1420) {
                sub = ((ins16 >> 3) & 0x60) + (ins16 & 0x1f);
                framesize += sub;
                /* "mov r8, r14" before subi */
                if(*(uint16_t *)(PC - i - 2) == 0x6e3b) {
                    continue;
                }
                break;
            }

            /* find "subi32 r14" */
            if ((ins32 & 0xfffff000) == 0xe5ce1000) {
                sub = (ins32 & 0xfff) + 1;
                if (sub % 4 == 0) {
                    framesize += sub/4;
                    continue;
                }
            }
        }
    }

#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d frsz=%d ofst=%d\n\r", __func__, __LINE__, framesize, offset);
#endif

    /* 4. output */
    LR = *(uintptr_t *)(SP + (framesize - offset)*4);
    if ( backtraceChkPC(LR) == 0) {
        if (print_func != NULL) {
            print_func("backtrace : invalid lr.\r\n");
        }
        return -1;
    }
    if (LR == (uintptr_t)&krhino_task_deathbed) {
        if (print_func != NULL) {
            print_func("backtrace : ^task entry^\r\n");
        }
        return 1;
    }
    if ( backtraceChkSP(SP + framesize*4) == 0 ) {
        if (print_func != NULL) {
            print_func("backtrace : invalid sp.\r\n");
        }
        return -1;
    }
    *pSP   = SP + framesize*4;
    /* LR after "bsr" */
    *pPC   = LR - 4;

#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d SP=%p, PC=%p\n\r", __func__, __LINE__, (void *)*pSP, (void *)*pPC);
#endif
    return 0;
}

/* find current function caller, update PC and SP
   non-leaf(A leaf procedure is one that doesn't all any other procedures.), return addr hold in registers.
   returns: 0  success
            1  success and find buttom
            -1 fail */
int backtrace_leaf(uintptr_t *pSP, uintptr_t *pPC, uintptr_t LR,
                   int (*print_func)(const char *fmt, ...))
{
    uintptr_t SP       = *pSP;
    uintptr_t PC       = *pPC;
    int       i;
    uint16_t  ins16;
    uint32_t  ins32;
    uint32_t  sub;
    uint32_t  framesize = 0; /* 1 for word */

    if (LR == (uintptr_t)&krhino_task_deathbed) {
        if (print_func != NULL) {
            print_func("backtrace : ^task entry^\r\n");
        }
        return 1;
    }

    /* 1. scan code, find stack open */
    for (i = 2; i < BACKTRACE_FUNC_LIMIT; i += 2) {
        ins16 = *(uint16_t *)(PC - i);
        ins32 = ((uint32_t)ins16 << 16) | ((uint32_t)*(uint16_t *)(PC - i + 2));

        /* find "subi16 r14" */
        if ((ins16 & 0xfce0) == 0x1420) {
            sub = ((ins16 >> 3) & 0x60) + (ins16 & 0x1f);
            framesize += sub;
            /* "mov r8, r14" before subi */
            if(*(uint16_t *)(PC - i - 2) == 0x6e3b) {
                continue;
            }
            break;
        }

        /* check if "jmp r15" */
        if (ins16 == 0x783c) {
            /* no stack open in this func */
            framesize = 0;
            break;
        }

        /* check if "pop16 r15" */
        if ((ins16 & 0xfff0) == 0x1490 && (ins16 & 0xf) < 9) {
            /* no stack open in this func */
            framesize = 0;
            break;
        }

        /* find "subi32 r14" */
        if ((ins32 & 0xfffff000) == 0xe5ce1000) {
            sub = (ins32 & 0xfff) + 1;
            if (sub % 4 == 0) {
                framesize += sub/4;
                continue;
            }
        }

        /* check if "pop32 r15" */
        if ((ins32 & 0xfffffe10) == 0xebc00010
            && (ins32 & 0xf) < 9
            && ((ins32 >> 5) & 7) < 3 ) {
            /* no stack open in this func */
            framesize = 0;
            break;
        }
    }

#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d frsz=%d\n\r", __func__, __LINE__, framesize);
#endif

    if ( i == BACKTRACE_FUNC_LIMIT ) {
        if (print_func != NULL) {
            print_func("Backtrace fail\r\n");
        }
        return -1;
    }

    /* 4. output */
    if ( backtraceChkSP(SP + framesize*4) == 0 ) {
        if (print_func != NULL) {
            print_func("backtrace : invalid sp\r\n");
        }
        return -1;
    }
    *pSP = SP + framesize*4;
    /* LR after "bsr" */
    *pPC = LR;

#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d SP=%p, PC=%p\n\r", __func__, __LINE__, (void *)*pSP, (void *)*pPC);
#endif
    return 0;

}

/**
 Get call stack, return levels of call stack
 trace[] output buffer
 size    buffer size
 offset  which lvl start
 */
int backtrace_dump(void *trace[], int size, int offset)
{
    int   lvl;
    int   ret;
    uintptr_t PC;
    uintptr_t SP;

    PC = backtraceGetPC();
    SP = backtraceGetSP();

#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d PC=%p SP=%p\n\r", __func__, __LINE__, (void *)PC, (void *)SP);
#endif

    memset(trace, 0, size*sizeof(void *));

    for (lvl = 0; lvl < BACKTRACE_LVL_LIMIT; lvl++) {
        ret = backtrace_nonleaf(&SP, &PC, NULL);
        if (ret != 0) {
            break;
        }
        if (lvl >= offset && lvl - offset < size) {
            trace[lvl - offset] = (void *)PC;
        }
        if (lvl - offset >= size) {
            break;
        }
    }

    return lvl - offset < 0 ? 0 : lvl - offset;
}

/* printf call stack
   return levels of call stack */
int backtrace_now(int (*print_func)(const char *fmt, ...))
{
    int   lvl;
    int   ret;
    uintptr_t PC;
    uintptr_t SP;
    char  s_panic_call[] = "backtrace : 0x         \r\n";

    if (print_func == NULL) {
        print_func = printf;
    }

    PC = backtraceGetPC();
    SP = backtraceGetSP();

    print_func("========== Call stack ==========\r\n");
    for (lvl = 0; lvl < BACKTRACE_LVL_LIMIT; lvl++) {
        u_int2str(PC, &s_panic_call[14]);
        print_func(s_panic_call);
#ifdef OS_PRINT_DBG
        OS_PRINT_DBG("[DEBUG]%s:%d PC=%p SP=%p\r\n", __func__, __LINE__, (void *)PC, (void *)SP);
#endif
        ret = backtrace_nonleaf(&SP, &PC, print_func);
        if (ret != 0) {
            break;
        }
    }
    print_func("==========    End     ==========\r\n");

    return lvl;
}

int backtrace_panic(uintptr_t SP, uintptr_t PC, uintptr_t LR, int (*print_func)(const char *fmt, ...))
{
    int   lvl;
    int   ret;
    uintptr_t SP_update, PC_update;
    char  s_panic_call[] = "backtrace : 0x         \r\n";

    SP_update = SP;
    PC_update = PC;

    print_func = print_func == NULL ? printf : print_func;

    /* 1. try, with no LR */
    for (lvl = 0; lvl < BACKTRACE_LVL_LIMIT; lvl++) {
#ifdef OS_PRINT_DBG
        OS_PRINT_DBG("[DEBUG]%s:%d PC=%p SP=%p\r\n", __func__, __LINE__, (void *)PC, (void *)SP);
#endif
        ret = backtrace_nonleaf(&SP_update, &PC_update, NULL);
        if (ret != 0) {
            break;
        }
    }

    if (ret == 1) {
        /* 1.1 try success, output */
        SP_update = SP;
        PC_update = PC;
        for (lvl = 0; lvl < BACKTRACE_LVL_LIMIT; lvl++) {
            u_int2str(PC_update, &s_panic_call[14]);
            print_func(s_panic_call);
#ifdef OS_PRINT_DBG
            OS_PRINT_DBG("[DEBUG]%s:%d PC=%p SP=%p\r\n", __func__, __LINE__, (void *)PC, (void *)SP);
#endif
            ret = backtrace_nonleaf(&SP_update, &PC_update, print_func);
            if (ret != 0) {
                return lvl + 1;
            }
        }
    }

    /* 2. try, with LR */
#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d PC=%p SP=%p\r\n", __func__, __LINE__, (void *)PC, (void *)SP);
#endif
    ret = backtrace_leaf(&SP_update, &PC_update, LR, NULL);
    if (ret >= 0) {
        /* 2.1 try success, output first level */
        SP_update = SP;
        PC_update = PC;
        u_int2str(PC_update, &s_panic_call[14]);
        print_func(s_panic_call);
        ret = backtrace_leaf(&SP_update, &PC_update, LR, print_func);
        if (ret == 1) {
            return 1;
        }
        /* 2.2 try success, output other levels  */
        for (lvl = 1; lvl < BACKTRACE_LVL_LIMIT; lvl++) {
            u_int2str(PC_update, &s_panic_call[14]);
            print_func(s_panic_call);
#ifdef OS_PRINT_DBG
            OS_PRINT_DBG("[DEBUG]%s:%d PC=%p SP=%p\r\n", __func__, __LINE__, (void *)PC, (void *)SP);
#endif
            ret = backtrace_nonleaf(&SP_update, &PC_update, print_func);
            if (ret != 0) {
                return lvl + 1;
            }
        }
    }

    return -1;
}

void backtrace_handle(char *PC, int *SP, char *LR, int (*print_func)(const char *fmt, ...))
{
    backtrace_panic(SP, PC, LR, print_func);
}

void *g_back_trace;
/**
 Get call stack, return levels of call stack
 trace[] output buffer
 size    buffer size
 offset  which lvl start
 */
int backtrace_now_get(void *trace[], int size, int offset)
{
    uintptr_t PC;
    uintptr_t SP;
    int   lvl;
    int   ret;

    /* compiler specific */
    PC = backtraceGetPC();
    SP = backtraceGetSP();

    memset(trace, 0, size*sizeof(void *));

    g_back_trace = trace;

    for (lvl = 0; lvl < BACKTRACE_LVL_LIMIT; lvl++) {
        ret = backtrace_nonleaf(&SP, &PC, NULL);
        if (ret != 0) {
            break;
        }
        if (lvl >= offset && lvl - offset < size) {
            trace[lvl - offset] = (void *)PC;
        }
        if (lvl - offset >= size) {
            break;
        }
    }
    return lvl - offset < 0 ? 0 : lvl - offset;
}

void backtrace_task(void *task, int (*print_func)(const char *fmt, ...))
{
    int       ret;
    uintptr_t PC, LR, SP;

    if (!task) {
        return;
    }

    context_t *cur_sp = (context_t *)(((ktask_t *)task)->task_stack);

    PC = cur_sp->PC;
    LR = cur_sp->LR;
    SP = (uintptr_t)(cur_sp + 1);

#ifdef OS_PRINT_DBG
    OS_PRINT_DBG("[DEBUG]%s:%d PC=%p LR=%p SP=%p\r\n", __func__, __LINE__, (void *)PC, (void *)LR, (void *)SP);
#endif

    ret = debug_task_is_running((ktask_t*)task);
    switch ( ret ) {
    case 0 :
    case 1 :
    case 2 :
        backtrace_panic((uintptr_t)SP, (uintptr_t)PC, (uintptr_t)LR, print_func);
        break;

    default:
        print_func("Status of task \"%s\" is 'Running', Can not backtrace!\n",
                   ((ktask_t *)task)->task_name ? ((ktask_t *)task)->task_name : "anonym");
        break;
    }

    print_func("==========    End     ==========\r\n");
}
#endif
