/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


void dispatcher_interrupt(long irq)
{
    extern void do_irq(void);
    do_irq();
}

