/*
 * Copyright (C) 2015-2021 Alibaba Group Holding Limited
 */

#ifndef K_TYPES_H
#define K_TYPES_H

#include "k_compiler.h"

#if __SIZEOF_LONG__ == 8
#define RHINO_TASK_STACK_OVF_MAGIC   0xdeadbeafdeadbeafull     /* stack overflow magic value */
#define RHINO_INTRPT_STACK_OVF_MAGIC 0xdeadbeafdeadbeafull     /* stack overflow magic value */
#define RHINO_MM_FRAG_ALLOCATED      0xabcddcababcddcabull     /* 32 bit value, if 64 bit system, you need change it to 64 bit */
#define RHINO_MM_FRAG_FREE           0xfefdecdbfefdecdbull     /* 32 bit value, if 64 bit system, you need change it to 64 bit */
#define RHINO_MM_CORRUPT_DYE         0xFEFEFEFEFEFEFEFEULL
#define RHINO_MM_FREE_DYE            0xABABABABABABABABULL
#define AOS_ARCH_LP64                1

typedef uint64_t cpu_stack_t;
typedef uint64_t cpu_cpsr_t;

#elif __SIZEOF_LONG__ == 4
#define RHINO_TASK_STACK_OVF_MAGIC   0xdeadbeafu     /* stack overflow magic value */
#define RHINO_INTRPT_STACK_OVF_MAGIC 0xdeadbeafu     /* stack overflow magic value */
#define RHINO_MM_FRAG_ALLOCATED      0xabcddcabu     /* 32 bit value, if 64 bit system, you need change it to 64 bit */
#define RHINO_MM_FRAG_FREE           0xfefdecdbu     /* 32 bit value, if 64 bit system, you need change it to 64 bit */
#define RHINO_MM_CORRUPT_DYE         0xFEFEFEFEU
#define RHINO_MM_FREE_DYE            0xABABABABU

typedef uint32_t cpu_stack_t;
typedef uint32_t cpu_cpsr_t;
#else
#error Assembler did not define __SIZEOF_LONG__
#endif

typedef uint64_t hr_timer_t;       /* 32 bit or 64 bit unsigned value */
typedef uint64_t lr_timer_t;       /* 32 bit or 64 bit unsigned value */

//typedef uint64_t tick_t;           /* 32 bit or 64 bit unsigned value */
//typedef uint64_t idle_count_t;     /* 64 bit unsigned value */
//typedef uint64_t sys_time_t;       /* 64 bit unsigned value */
typedef uint32_t mutex_nested_t;   /* 8 bit or 16bit or 32bit unsigned value */
typedef uint8_t  suspend_nested_t; /* 8 bit normally */

typedef uint64_t ctx_switch_t;     /* 32 bit or 64 bit unsigned value */

typedef char     name_t;
typedef uint32_t sem_count_t;

#define RHINO_NO_WAIT                0u
// #define RHINO_INLINE                 static __inline /* inline keyword, it may change under different compilers. see k_compiler.h*/

#endif /* K_TYPES_H */

