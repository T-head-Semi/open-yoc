/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

__BEGIN_DECLS__

#ifndef CONFIG_JSE_TASK_STACK_SIZE
#define CONFIG_JSE_TASK_STACK_SIZE    (10*1024)
#endif

#ifndef CONFIG_TSL_DEVICER_MESH
#define CONFIG_TSL_DEVICER_MESH      (1)
#endif

#ifndef CONFIG_JSE_STATIC_STACK
#define CONFIG_JSE_STATIC_STACK       (1)
#endif

#ifndef CONFIG_SAVE_JS_TO_RAM
#define CONFIG_SAVE_JS_TO_RAM         (0)
#endif

#ifndef CONFIG_TSL_BYTECODE_ENABLE
#define CONFIG_TSL_BYTECODE_ENABLE   (0)
#endif

__END_DECLS__

#endif /* __CONFIG_H__ */

