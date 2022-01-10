/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

extern int __dtor_end__;
extern int __ctor_end__;
extern int __ctor_start__;

typedef void (*func_ptr) (void);

__attribute__((weak)) void *__dso_handle = 0;

__attribute__((weak)) void cxx_system_init(void)
{
    func_ptr *p;
    for (p = (func_ptr *)&__ctor_end__ - 1; p >= (func_ptr *)&__ctor_start__; p--) {
        (*p) ();
    }
#ifdef HAVE_INIT_ARRAY_LD
    extern func_ptr __ctors_start__[];
    extern func_ptr __ctors_end__[];
    for (p = __ctors_start__; p < __ctors_end__; p++) {
        (*p)();
    }
#endif
}