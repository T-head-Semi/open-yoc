/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include <tsl_engine/tsl_common.h>

__BEGIN_DECLS__

/**
 * @brief  get the file-path content
 * @param  [in] path
 * @param  [in] osize
 * @return NULL on error
 */
char *get_file_content(const char *path, size_t *osize);
#if defined (CONFIG_SAVE_JS_TO_RAM) && (CONFIG_SAVE_JS_TO_RAM == 1)
int init_js_to_ram(void);
char *get_js_from_ram(const char *js_name, size_t *osize);
int save_js_to_ram(const char *js_name, const char *data, size_t size);
int is_js_already_existed(const char *js_name);
#endif


__END_DECLS__

#endif /* __FILE_UTILS_H__ */

