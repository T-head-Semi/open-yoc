/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __LITTLEFS_VFS_H__
#define __LITTLEFS_VFS_H__

#include <aos/aos.h>
#include <aos/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LFS_MOUNTPOINT
#define LFS_MOUNTPOINT CONFIG_LFS_MOUNTPOINT
#else
#define LFS_MOUNTPOINT "/"
#endif
#ifdef CONFIG_LFS_BLOCK_SIZE
#define LFS_BLOCK_SIZE CONFIG_LFS_BLOCK_SIZE
#endif
#ifdef CONFIG_LFS_BLOCK_COUNT
#define LFS_BLOCK_COUNT CONFIG_LFS_BLOCK_COUNT
#endif
#ifdef CONFIG_LFS_READ_SIZE
#define LFS_READ_SIZE CONFIG_LFS_READ_SIZE
#endif
#ifdef CONFIG_LFS_PROG_SIZE
#define LFS_PROG_SIZE CONFIG_LFS_PROG_SIZE
#endif
#ifdef CONFIG_LFS_CACHE_SIZE
#define LFS_CACHE_SIZE CONFIG_LFS_CACHE_SIZE
#endif
#ifdef CONFIG_LFS_LOOKAHEAD_SIZE
#define LFS_LOOKAHEAD_SIZE CONFIG_LFS_LOOKAHEAD_SIZE
#endif
#ifdef CONFIG_LFS_BLOCK_CYCLES
#define LFS_BLOCK_CYCLES CONFIG_LFS_BLOCK_CYCLES
#endif

/**
 * @brief  register little file system
 * @param  [in] partition_desc: partion table
 * @return 0 on success
 */
int32_t vfs_lfs_register(char *partition_desc);

/**
 * @brief  unregister little file system
 * @return 0 on success
 */
int32_t vfs_lfs_unregister(void);

#ifdef __cplusplus
}
#endif

#endif /* __LITTLEFS_VFS_H__ */

