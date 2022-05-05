/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

 /******************************************************************************
 * @file     startup.S
 * @brief    startup file. Should use with
 *           GCC for RISC-V Embedded Processors
 * @version  V1.0
 * @date     29. July 2019
 ******************************************************************************/

#include <csi_config.h>

.globl  Reset_Handler

.section .vectors
    .align  6
    .globl  __Vectors
    .type   __Vectors, @object
__Vectors:
    j   Default_Riscv_Handler				/* 0 */
    j   Stspend_Handler			/* 1 */
    j   Default_Riscv_Handler				/* 2 */
    j   Mtspend_Handler			/* 3 */
    j   Default_Riscv_Handler				/* 4 */
    j   Default_Riscv_Handler			/* 5 */
    j   Default_Riscv_Handler				/* 6 */
    j   Mcoret_Handler			/* 7 */
    j   Default_Riscv_Handler				/* 8 */
    j   Sirq_Handler			/* 9 */
    j   Default_Riscv_Handler				/* 10 */
    j   Mirq_Handler			/* 11 */
    j   Default_Riscv_Handler
    j   Default_Riscv_Handler
    j   Default_Riscv_Handler
    j   Default_Riscv_Handler

 .text
    .align  2
    j        Reset_Handler
    .align   2
    .long    0x594B5343 /* CSKY ASCII */
    .long    0x594B5343 /* CSKY ASCII */
    .align   2
    .long    Reset_Handler
_start:
    .type   Reset_Handler, %function
Reset_Handler:
.option push
.option norelax
    /* disable ie and clear all interrupts */
    csrw    mie, zero
    csrw    mip, zero

    /* Disable MIE to avoid triggering interrupts before the first task starts. */
    /* This bit is set when a task recovers context. */
    li      a0, 0x00000008UL        /*  Machine Interrupt Disable */
    csrc    mstatus, a0

    la      gp, __global_pointer$
.option pop
    /*
     * enable thead ISA extension:
     * THEADISAEE = 1, MM = 1,
     */
    csrr    t0, mxstatus
    li      t1, (1 << 22) | (1 << 15)
    or      t1, t1, t0
    csrw    mxstatus, t1

    /* FP: initial state */
    csrr    t0, mstatus
    li      t1, ~0x6000
    and     t0, t0, t1
    li      t1, 0x2000
    or      t0, t0, t1
    csrw    mstatus, t0
    /* csrwi   fcsr, 0 */

    /* RVV: initial state */
    li    t0, ~(3 << 23)
    csrr  t1, mstatus
    and   t2, t1, t0
    li    t0, 1 << 23
    or    t2, t2, t0
    csrw  mstatus, t2
    la      a0, __Vectors
    li	    a1, 0x1
    or	    a0, a0,a1
    csrw    mtvec, a0

    la      sp, g_top_irqstack

#ifndef __NO_SYSTEM_INIT
    la   a0, SystemInit
    jalr a0
#endif

#ifdef CONFIG_KERNEL_NONE
    /* enable interrupt */
    li      a0, SR_MIE
    csrs    mstatus, a0
#endif

    la   a0, pre_main
    jalr a0

    .size   Reset_Handler, . - Reset_Handler

__exit:
    j      __exit


.section .stack

    .align  3
    .global g_base_irqstack
    .global g_top_irqstack
g_base_irqstack:
    .space CONFIG_ARCH_INTERRUPTSTACK
g_top_irqstack: