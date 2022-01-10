# dump the task info for rhino RTOS
# please file the elf before use this script
set height 0

define paddr
  # print current info
  i r
  p *g_active_task[0]
  bt

    #*(--stk) = (uint32_t)entry;                   /* PC            */
    #*(--stk) = (uint32_t)0x31313131L;             /* X31           */
    #*(--stk) = (uint32_t)0x30303030L;             /* X30           */
    #*(--stk) = (uint32_t)0x29292929L;             /* X29           */
    #*(--stk) = (uint32_t)0x28282828L;             /* X28           */
    #*(--stk) = (uint32_t)0x27272727L;             /* X27           */
    #*(--stk) = (uint32_t)0x26262626L;             /* X26           */
    #*(--stk) = (uint32_t)0x25252525L;             /* X25           */
    #*(--stk) = (uint32_t)0x24242424L;             /* X24           */
    #*(--stk) = (uint32_t)0x23232323L;             /* X23           */
    #*(--stk) = (uint32_t)0x22222222L;             /* X22           */
    #*(--stk) = (uint32_t)0x21212121L;             /* X21           */
    #*(--stk) = (uint32_t)0x20202020L;             /* X20           */
    #*(--stk) = (uint32_t)0x19191919L;             /* X19           */
    #*(--stk) = (uint32_t)0x18181818L;             /* X18           */
    #*(--stk) = (uint32_t)0x17171717L;             /* X17           */
    #*(--stk) = (uint32_t)0x16161616L;             /* X16           */
    #*(--stk) = (uint32_t)0x15151515L;             /* X15           */
    #*(--stk) = (uint32_t)0x14141414L;             /* X14           */
    #*(--stk) = (uint32_t)0x13131313L;             /* X13           */
    #*(--stk) = (uint32_t)0x12121212L;             /* X12           */
    #*(--stk) = (uint32_t)0x11111111L;             /* X11           */
    #*(--stk) = (uint32_t)arg;                     /* X10           */
    #*(--stk) = (uint32_t)0x09090909L;             /* X9            */
    #*(--stk) = (uint32_t)0x08080808L;             /* X8            */
    #*(--stk) = (uint32_t)0x07070707L;             /* X7            */
    #*(--stk) = (uint32_t)0x06060606L;             /* X6            */
    #*(--stk) = (uint32_t)0x05050505L;             /* X5            */
    #*(--stk) = (uint32_t)0x04040404L;             /* X4            */
    #*(--stk) = (uint32_t)gp;                      /* X3            */
    #*(--stk) = (uint32_t)krhino_task_deathbed;    /* X1            */



  # store the register
  set $gdb_x0  = $x0
  set $gdb_x1  = $x1
  set $gdb_x3  = $x3
  set $gdb_x4  = $x4
  set $gdb_x5  = $x5
  set $gdb_x6  = $x6
  set $gdb_x7  = $x7
  set $gdb_x8  = $x8
  set $gdb_x9  = $x9
  set $gdb_x10 = $x10
  set $gdb_x11 = $x11
  set $gdb_x12 = $x12
  set $gdb_x13 = $x13
  set $gdb_x14 = $x14
  set $gdb_x15 = $x15
  set $gdb_x16 = $x16
  set $gdb_x17 = $x17
  set $gdb_x18 = $x18
  set $gdb_x19 = $x19
  set $gdb_x20 = $x20
  set $gdb_x21 = $x21
  set $gdb_x22 = $x22
  set $gdb_x23 = $x23
  set $gdb_x24 = $x24
  set $gdb_x25 = $x25
  set $gdb_x26 = $x26
  set $gdb_x27 = $x27
  set $gdb_x28 = $x28
  set $gdb_x29 = $x29
  set $gdb_x30 = $x30
  set $gdb_x31 = $x31
  set $gdb_mcause = $mcause
  set $gdb_pc  = $pc

  # get list offset
  set $gdb_offset = (int)((int)&g_active_task[0]->task_stats_item - (int)g_active_task[0])

  # get tcb addr
  set $gdb_tcb_addr = (ktask_t *)((int)(klist_t *)(&g_kobj_list.task_head)->next - $gdb_offset)
  
  # get task name
  set $gdb_task_name = ((ktask_t *)($gdb_tcb_addr))->task_name
  set $gdb_i = 0

  while (&($gdb_tcb_addr->task_stats_item) != &g_kobj_list.task_head)

    # print the task info
    p $gdb_i
    p $gdb_task_name
    p *$gdb_tcb_addr

    # set register to restore the task 
    set $x1  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 0)
    set $x4  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 8)
    set $x5  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 16)
    set $x6  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 24)
    set $x7  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 32)
    set $x8  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 40)
    set $x9  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 48)
    set $x10 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 56)
    set $x11 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 64)
    set $x12 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 72)
    set $x13 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 80)
    set $x14 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 88)
    set $x15 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 96)
    set $x16 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 104)
    set $x17 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 112)
    set $x18 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 120)
    set $x19 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 128)
    set $x20 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 136)
    set $x21 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 144)
    set $x22 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 152)
    set $x23 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 160)
    set $x24 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 168)
    set $x25 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 176)
    set $x26 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 184)
    set $x27 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 192)
    set $x28 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 200)
    set $x29 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 208)
    set $x30 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 216)
    set $x31 = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 224)
    set $pc  = *(cpu_stack_t *)((cpu_stack_t)$gdb_tcb_addr->task_stack + 232)
    set $sp = (cpu_stack_t)(($gdb_tcb_addr->task_stack) + (256+248))

	
    # print the task stack backtrace
    bt
	
	
    # go to next task
    set $gdb_i = $gdb_i + 1
    set $gdb_tcb_addr = (ktask_t *) ((int)$gdb_tcb_addr->task_stats_item - $gdb_offset)
    set $gdb_task_name = ((ktask_t *)($gdb_tcb_addr))->task_name
  end
  p $gdb_task_name
  # restore the register
  set  $x0 = $gdb_x0 
  set  $x1 = $gdb_x1 
  set  $x3 = $gdb_x3 
  set  $x4 = $gdb_x4 
  set  $x5 = $gdb_x5 
  set  $x6 = $gdb_x6 
  set  $x7 = $gdb_x7 
  set  $x8 = $gdb_x8 
  set  $x9 = $gdb_x9 
  set  $x10 = $gdb_x10
  set  $x11 = $gdb_x11
  set  $x12 = $gdb_x12
  set  $x13 = $gdb_x13
  set  $x14 = $gdb_x14
  set  $x15 = $gdb_x15
  set  $x16 = $gdb_x16
  set  $x17 = $gdb_x17
  set  $x18 = $gdb_x18
  set  $x19 = $gdb_x19
  set  $x20 = $gdb_x20
  set  $x21 = $gdb_x21
  set  $x22 = $gdb_x22
  set  $x23 = $gdb_x23
  set  $x24 = $gdb_x24
  set  $x25 = $gdb_x25
  set  $x26 = $gdb_x26
  set  $x27 = $gdb_x27
  set  $x28 = $gdb_x28
  set  $x29 = $gdb_x29
  set  $x30 = $gdb_x30
  set  $x31 = $gdb_x31
  set  $mcause = $gdb_mcause
  set  $pc = $gdb_pc 
end

paddr
