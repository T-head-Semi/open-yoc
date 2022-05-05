/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <vfs.h>
#include <vfs_inode.h>
#include <vfs_file.h>
#include <vfs_register.h>
#include <yoc/partition.h>

#include "lfs.h"
#include "littlefs_vfs.h"
#include "lfs_util.h"

/* lookahead size should be 8 bytes aligned */
#define _ALIGN8(n) (((n)+7) & (~7))
#define GET_LOOKAHEAD_SIZE(blks) (_ALIGN8((blks)>>4) > 16 ? _ALIGN8((blks)>>4) : 16)

#define WAIT_FOREVER 0xFFFFFFFF

typedef aos_mutex_t lfs_lock_t;

typedef struct {
    struct lfs_config *config;
    lfs_t             *lfs;
    lfs_lock_t        *lock;
} lfs_manager_t;

typedef struct _lfsvfs_dir_t
{
    vfs_dir_t    dir;
    lfs_dir_t    lfsdir;
    vfs_dirent_t cur_dirent;
} lfsvfs_dir_t;

partition_t lfs_hdl;
static char *lfs_mount_path;
static lfs_manager_t g_lfs_manager = {0};

static int32_t littlefs_block_read(const struct lfs_config *cfg, lfs_block_t block,
                             lfs_off_t off, void *dst, lfs_size_t size)
{
    uint32_t off_set = off + cfg->block_size * block;
    return partition_read(lfs_hdl, off_set, dst, size);
}

static int32_t littlefs_block_write(const struct lfs_config *cfg, lfs_block_t block,
                              lfs_off_t off, const void *dst, lfs_size_t size)
{
    uint32_t off_set = off + cfg->block_size * block;
    return partition_write(lfs_hdl, off_set, (void *)dst, size);
}

static int32_t littlefs_block_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    partition_info_t *part_info = hal_flash_get_info(lfs_hdl);
    uint32_t off_set = cfg->block_size * block;

    return partition_erase(lfs_hdl, off_set, cfg->block_size / part_info->sector_size);
}

static int32_t littlefs_block_sync(const struct lfs_config *cfg)
{
    return 0;
}

struct lfs_config default_cfg = {
    // block device operations
    .read  = littlefs_block_read,
    .prog  = littlefs_block_write,
    .erase = littlefs_block_erase,
    .sync  = littlefs_block_sync,
};

/* Global FS lock init */
static void lfs_lock_create(lfs_lock_t *lock)
{
    aos_mutex_new(lock);
}

/* Global FS lock destroy */
static void lfs_lock_destory(lfs_lock_t *lock)
{
    if (lock) {
        aos_mutex_free(lock);
        LFS_TRACE("%s: lock %p destroyed", aos_task_name(), lock);
    }
}

static void lfs_lock(lfs_lock_t *lock)
{
    if (lock) {
        aos_mutex_lock(lock, WAIT_FOREVER);
        LFS_TRACE("%s: lock %p locked", aos_task_name(), lock);
    }
}

static void lfs_unlock(lfs_lock_t *lock)
{
    if (lock) {
        LFS_TRACE("%s: lock %p to be unlocked", aos_task_name(), lock);
        aos_mutex_unlock(lock);
    }
}

/* Relative path convert */
static char *path_convert(const char *path)
{
    int32_t len, prefix;
    char *target_path, *p;

    if (path == NULL) {
        return NULL;
    }

    len = strlen(path);
    prefix = strlen(lfs_mount_path);
    if (strncmp(lfs_mount_path, path, prefix) != 0) {
        return NULL;
    }

    len = len - prefix;
    target_path =(char *)aos_malloc(len + 1);
    if (target_path == NULL) {
        return NULL;
    }

    memset(target_path, 0, len + 1);
    if (len > 0) {
        if (strcmp(lfs_mount_path, "/") == 0) {
            p = (char *)(path + prefix);
            memcpy(target_path, p, len);
        } else {
            p = (char *)(path + prefix + 1);
            memcpy(target_path, p, len - 1);
        }
    }

    target_path[len] = '\0';
    return target_path;
}

static int32_t mode_convert(int32_t flags)
{
    int32_t mode, res = 0;

    mode = flags & O_ACCMODE;
    if (mode == O_RDONLY) {
        res |= LFS_O_RDONLY;
    } else if (mode == O_WRONLY) {
        res |= LFS_O_WRONLY;
    } else if (mode == O_RDWR) {
        res |= LFS_O_RDWR;
    }

    if (flags & O_CREAT) {
        res |= LFS_O_CREAT;
    }
    if (flags & O_EXCL) {
        res |= LFS_O_EXCL;
    }
    if (flags & O_TRUNC) {
        res |= LFS_O_TRUNC;
    }
    if (flags & O_APPEND) {
        res |= LFS_O_CREAT | LFS_O_APPEND;
    }
    return res;
}

static int lfs_ret_value_convert(int lfs_ret)
{
    int ret;

    if (lfs_ret > 0) {
        return lfs_ret;
    }

    switch(lfs_ret) {
        case LFS_ERR_OK: ret = 0; break;
        case LFS_ERR_IO: ret = -EIO; break;
        case LFS_ERR_CORRUPT: ret = -EIO; break;
        case LFS_ERR_NOENT: ret = -ENOENT; break;
        case LFS_ERR_EXIST: ret = -EEXIST; break;
        case LFS_ERR_NOTDIR: ret = -ENOTDIR; break;
        case LFS_ERR_ISDIR: ret = -EISDIR; break;
        case LFS_ERR_NOTEMPTY: ret = -ENOTEMPTY; break;
        case LFS_ERR_BADF: ret = -EBADF; break;
        case LFS_ERR_FBIG: ret = -EFBIG; break;
        case LFS_ERR_INVAL: ret = -EINVAL; break;
        case LFS_ERR_NOSPC: ret = -ENOSPC; break;
        case LFS_ERR_NOMEM: ret = -ENOMEM; break;
        case LFS_ERR_NOATTR: ret = -ENODATA; break;
        case LFS_ERR_NAMETOOLONG: ret = -ENAMETOOLONG; break;
        default: ret = lfs_ret; break;
    }

    return ret;
}

static int32_t _lfs_deinit(void)
{
    if (g_lfs_manager.lfs != NULL) {
        aos_free(g_lfs_manager.lfs);
    }

    if (g_lfs_manager.lock != NULL) {
        lfs_lock_destory(g_lfs_manager.lock);
        aos_free(g_lfs_manager.lock);
    }
    return 0;
}

static int32_t _lfs_init(void)
{
    /* Set LFS default config */
    g_lfs_manager.config = &default_cfg;

    /* Create LFS Global Lock */
    g_lfs_manager.lock = (lfs_lock_t *)aos_malloc(sizeof(lfs_lock_t));
    if (g_lfs_manager.lock == NULL) {
        goto ERROR;
    }
    lfs_lock_create(g_lfs_manager.lock);

    /* Create LFS struct */
    g_lfs_manager.lfs = (lfs_t *)aos_malloc(sizeof(lfs_t));
    if (g_lfs_manager.lfs == NULL) {
        goto ERROR;
    }

    return 0;

ERROR:
    _lfs_deinit();
    return -1;
}

static int32_t lfs_vfs_open(vfs_file_t *fp, const char *path, int flags)
{
    int res;
    char *target_path = NULL;
    lfs_file_t *file = NULL;

    target_path = path_convert(path);
    if (target_path == NULL) {
        return -EINVAL;
    }

    if (!strlen(target_path)) {
        return -EINVAL;
    }

    file = (lfs_file_t *)aos_malloc(sizeof(lfs_file_t));
    if (file == NULL) {
        aos_free(target_path);
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    res = lfs_file_open(g_lfs_manager.lfs, file,  target_path, mode_convert(flags));
    lfs_unlock(g_lfs_manager.lock);

    if (res != LFS_ERR_OK) {
        aos_free(target_path);
        aos_free(file);
        return res;
    } else {
        fp->f_arg = (void *)file;
    }


    aos_free(target_path);
    return res;
}

static int32_t lfs_vfs_close(vfs_file_t *fp)
{
    int res = LFS_ERR_INVAL;
    lfs_file_t *file = (lfs_file_t *)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    res = lfs_file_close(g_lfs_manager.lfs, file);
    lfs_unlock(g_lfs_manager.lock);

    if (res == LFS_ERR_OK) {
        aos_free(file);
    }

    return res;
}

static int32_t lfs_vfs_read(vfs_file_t *fp, char *buf, uint32_t len)
{
    int nbytes;

    lfs_file_t *file = (lfs_file_t *)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    nbytes = lfs_file_read(g_lfs_manager.lfs, file, buf, len);
    lfs_unlock(g_lfs_manager.lock);

    return nbytes;
}

static int32_t lfs_vfs_write(vfs_file_t *fp, const char *buf, uint32_t len)
{
    int nbytes;

    lfs_file_t *file = (lfs_file_t *)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    nbytes = lfs_file_write(g_lfs_manager.lfs, file, buf, len);
    lfs_unlock(g_lfs_manager.lock);

    return nbytes;
}

static uint32_t lfs_vfs_lseek(vfs_file_t *fp, int64_t off, int32_t whence)
{
    off_t res;

    lfs_file_t *file = (lfs_file_t *)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    res = lfs_file_seek(g_lfs_manager.lfs, file, off, whence);
    lfs_unlock(g_lfs_manager.lock);

    return res;
}

static int32_t lfs_vfs_sync(vfs_file_t *fp)
{
    int res;

    lfs_file_t *file = (lfs_file_t *)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    res = lfs_file_sync(g_lfs_manager.lfs, file);
    lfs_unlock(g_lfs_manager.lock);

    return res;
}

static int32_t lfs_vfs_fstat(vfs_file_t *fp, vfs_stat_t *st)
{
    struct lfs_info s;
    int32_t ret;

    lfs_file_t *file = (lfs_file_t*)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_fstat(g_lfs_manager.lfs, file, &s);
    lfs_unlock(g_lfs_manager.lock);

    ret = lfs_ret_value_convert(ret);
    if (ret == 0){
        st->st_size = s.size;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO |
                      ((s.type == LFS_TYPE_DIR) ? S_IFDIR : S_IFREG);
#ifdef FS_TIMESTAMP_WORKAROUND
        st->st_actime = time(NULL);
        st->st_modtime = time(NULL);
#endif
    }

    return ret;
}

static int32_t lfs_vfs_stat(vfs_file_t *fp, const char *path, vfs_stat_t *st)
{
    struct lfs_info s;
    int res;
    char *target_path = NULL;

    target_path = path_convert(path);
    if (target_path == NULL) {
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    res = lfs_stat(g_lfs_manager.lfs, target_path, &s);
    lfs_unlock(g_lfs_manager.lock);

    if (res == LFS_ERR_OK) {
        st->st_size = s.size;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO |
                      ((s.type == LFS_TYPE_DIR ? S_IFDIR : S_IFREG));
    }

    aos_free(target_path);
    return res;
}

static int32_t lfs_vfs_access(vfs_file_t *fp, const char *path, int mode)
{
    vfs_stat_t s;
    int ret;

    ret = lfs_vfs_stat(fp, path, &s);
    ret = lfs_ret_value_convert(ret);
    if (ret == 0) {
        switch(mode) {
            case F_OK: return 0;
            case R_OK: return s.st_mode & S_IRUSR ? 0 : 1;
            case W_OK: return s.st_mode & S_IWUSR ? 0 : 1;
            case X_OK: return s.st_mode & S_IXUSR ? 0 : 1;
        }
    }

    return ret;
}

static int32_t lfs_vfs_statfs(vfs_file_t *fp, const char *path, vfs_statfs_t *sfs)
{
    int32_t block_used;
    int ret;

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_fs_size(g_lfs_manager.lfs);
    lfs_unlock(g_lfs_manager.lock);
    ret = lfs_ret_value_convert(ret);
    if (ret >= 0) {
        block_used = ret;
        memset(sfs, 0, sizeof(vfs_statfs_t));
        sfs->f_type = 0xd3fc;
        sfs->f_bsize = g_lfs_manager.config->block_size;
        sfs->f_blocks = g_lfs_manager.config->block_count;
        sfs->f_bfree = g_lfs_manager.config->block_count - block_used;
        sfs->f_bavail = sfs->f_bfree;
        sfs->f_files = 1024;
        ret = 0;
    }

    return ret;
}

static int32_t lfs_vfs_remove(vfs_file_t *fp, const char *path)
{
    int res;
    char *target_path = NULL;

    target_path = path_convert(path);
    if (target_path == NULL) {
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    res = lfs_remove(g_lfs_manager.lfs, target_path);
    lfs_unlock(g_lfs_manager.lock);

    aos_free(target_path);
    return res;
}

static int32_t lfs_vfs_rename(vfs_file_t *fp, const char *oldpath, const char *newpath)
{
    int32_t ret;

    char *oldname = NULL;
    char *newname = NULL;

    oldname = path_convert(oldpath);
    if (!oldname) {
        return -EINVAL;
    }

    newname = path_convert(newpath);
    if (!newname) {
        aos_free(oldname);
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_rename(g_lfs_manager.lfs, oldname, newname);
    lfs_unlock(g_lfs_manager.lock);

    aos_free(oldname);
    aos_free(newname);

    return ret;
}

static vfs_dir_t *lfs_vfs_opendir(vfs_file_t *fp, const char *path)
{
    lfsvfs_dir_t *lfsvfs_dir = NULL;
    char *relpath = NULL;
    int32_t ret;

    relpath = path_convert(path);
    if (!relpath) {
        return NULL;
    }

    lfsvfs_dir = (lfsvfs_dir_t *)aos_malloc(sizeof(lfsvfs_dir_t) + LFS_NAME_MAX + 1);
    if (!lfsvfs_dir) {
        aos_free(relpath);
        return NULL;
    }

    memset(lfsvfs_dir, 0, sizeof(lfsvfs_dir_t) + LFS_NAME_MAX + 1);

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_dir_open(g_lfs_manager.lfs, &lfsvfs_dir->lfsdir, relpath);
    lfs_unlock(g_lfs_manager.lock);

    if (ret != LFS_ERR_OK) {
        aos_free(relpath);
        aos_free(lfsvfs_dir);
        return NULL;
    }

    aos_free(relpath);
    return (vfs_dir_t *)lfsvfs_dir;
}

static vfs_dirent_t *lfs_vfs_readdir(vfs_file_t *fp, vfs_dir_t *dir)
{
    lfsvfs_dir_t *lfsvfs_dir = (lfsvfs_dir_t*)dir;
    struct lfs_info info;
    int32_t ret;

    if (!lfsvfs_dir) {
        return NULL;
    }

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_dir_read(g_lfs_manager.lfs, &lfsvfs_dir->lfsdir, &info);
    lfs_unlock(g_lfs_manager.lock);

    if (ret != true) {
        return NULL;
    }

    if (info.name[0] == 0) {
        return NULL;
    }

    lfsvfs_dir->cur_dirent.d_ino = 0;
    lfsvfs_dir->cur_dirent.d_type = info.type;
    strncpy(lfsvfs_dir->cur_dirent.d_name, info.name, LFS_NAME_MAX);
    lfsvfs_dir->cur_dirent.d_name[LFS_NAME_MAX] = '\0';

    return &lfsvfs_dir->cur_dirent;
}

static int32_t lfs_vfs_closedir(vfs_file_t *fp, vfs_dir_t *dir)
{
    lfsvfs_dir_t *lfsvfs_dir = (lfsvfs_dir_t *)dir;
    int32_t ret;

    if (!lfsvfs_dir) {
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_dir_close(g_lfs_manager.lfs, &lfsvfs_dir->lfsdir);
    lfs_unlock(g_lfs_manager.lock);
    if (ret != 0) {
        return ret;
    }

    aos_free(lfsvfs_dir);

    return ret;
}

static int32_t lfs_vfs_mkdir(vfs_file_t *fp, const char *path)
{
    int32_t ret;
    char *pathname = NULL;

    pathname = path_convert(path);
    if (!pathname) {
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_mkdir(g_lfs_manager.lfs, pathname);
    lfs_unlock(g_lfs_manager.lock);

    aos_free(pathname);

    return ret;
}

static int32_t lfs_vfs_rmdir (vfs_file_t *fp, const char *path)
{
    int32_t ret;
    char *pathname = NULL;

    pathname = path_convert(path);
    if (!pathname) {
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_remove(g_lfs_manager.lfs, pathname);
    lfs_unlock(g_lfs_manager.lock);

    aos_free(pathname);

    return ret;
}

static void lfs_vfs_rewinddir(vfs_file_t *fp, vfs_dir_t *dir)
{
    lfsvfs_dir_t *lfsvfs_dir = (lfsvfs_dir_t *)dir;

    if (!lfsvfs_dir) {
        return;
    }

    lfs_lock(g_lfs_manager.lock);
    lfs_dir_rewind(g_lfs_manager.lfs, &lfsvfs_dir->lfsdir);
    lfs_unlock(g_lfs_manager.lock);
}

static int32_t lfs_vfs_telldir(vfs_file_t *fp, vfs_dir_t *dir)
{
    lfsvfs_dir_t *lfsvfs_dir = (lfsvfs_dir_t *)dir;
    int32_t ret;

    if (!lfsvfs_dir) {
        return -EINVAL;
    }

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_dir_tell(g_lfs_manager.lfs, &lfsvfs_dir->lfsdir);
    lfs_unlock(g_lfs_manager.lock);

    return lfs_ret_value_convert(ret);
}

static void lfs_vfs_seekdir(vfs_file_t *fp, vfs_dir_t *dir, int32_t loc)
{
    lfsvfs_dir_t *lfsvfs_dir = (lfsvfs_dir_t*)dir;

    if (!lfsvfs_dir) {
        return;
    }

    lfs_lock(g_lfs_manager.lock);
    lfs_dir_seek(g_lfs_manager.lfs, &lfsvfs_dir->lfsdir, (lfs_off_t)loc);
    lfs_unlock(g_lfs_manager.lock);
}

static int32_t lfs_vfs_utime(vfs_file_t *fp, const char *path, const vfs_utimbuf_t *times)
{
    return lfs_ret_value_convert(LFS_ERR_OK);
}

static int32_t lfs_vfs_truncate(vfs_file_t *fp, int64_t size)
{
    int32_t ret;

    lfs_file_t *file = (lfs_file_t *)(fp->f_arg);

    lfs_lock(g_lfs_manager.lock);
    ret = lfs_file_truncate(g_lfs_manager.lfs, file, size);
    lfs_unlock(g_lfs_manager.lock);

    return lfs_ret_value_convert(ret);
}


static vfs_filesystem_ops_t littlefs_ops = {
    .open       = &lfs_vfs_open,
    .close      = &lfs_vfs_close,
    .read       = &lfs_vfs_read,
    .write      = &lfs_vfs_write,
    .lseek      = &lfs_vfs_lseek,
    .sync       = &lfs_vfs_sync,
    .stat       = &lfs_vfs_stat,
    .unlink     = &lfs_vfs_remove,
    .remove     = &lfs_vfs_remove,
    .rename     = &lfs_vfs_rename,
    .access     = &lfs_vfs_access,
    .fstat      = &lfs_vfs_fstat,
    .statfs     = &lfs_vfs_statfs,
    .opendir    = &lfs_vfs_opendir,
    .readdir    = &lfs_vfs_readdir,
    .closedir   = &lfs_vfs_closedir,
    .mkdir      = &lfs_vfs_mkdir,
    .rmdir      = &lfs_vfs_rmdir,
    .rewinddir  = &lfs_vfs_rewinddir,
    .telldir    = &lfs_vfs_telldir,
    .seekdir    = &lfs_vfs_seekdir,
    .ioctl      = NULL,
    .utime      = &lfs_vfs_utime,
    .truncate   = &lfs_vfs_truncate,
};

int lfs_vfs_mount(void)
{
    int res;

    res = _lfs_init();
    if (res != LFS_ERR_OK) {
        return res;
    }

    lfs_lock(g_lfs_manager.lock);

    res = lfs_mount(g_lfs_manager.lfs, g_lfs_manager.config);
#ifdef LITTLEFS_FORMAT
    if (res != LFS_ERR_OK) {
        res = lfs_format(g_lfs_manager.lfs, g_lfs_manager.config);
        if (res != LFS_ERR_OK) {
            goto ERROR;
        }

        res = lfs_mount(g_lfs_manager.lfs, g_lfs_manager.config);
        if (res != LFS_ERR_OK) {
            goto ERROR;
        }
    }
#endif

    lfs_unlock(g_lfs_manager.lock);
    return res;

#ifdef LITTLEFS_FORMAT
ERROR:
    lfs_unlock(g_lfs_manager.lock);
    _lfs_deinit();
    return res;
#endif
}

int lfs_vfs_unmount(void)
{
    lfs_unmount(g_lfs_manager.lfs);
    _lfs_deinit();
    return LFS_ERR_OK;
}

int32_t vfs_lfs_register(char *partition_desc)
{
    lfs_hdl = partition_open(partition_desc);

    aos_check(lfs_hdl > 0, EIO);

    lfs_mount_path = (char *)aos_malloc(sizeof(LFS_MOUNTPOINT) + 1);
    sprintf(lfs_mount_path, "%s", LFS_MOUNTPOINT);

    // block device configuration
    partition_info_t *part_info = hal_flash_get_info(lfs_hdl);
#ifdef CONFIG_LFS_BLOCK_SIZE
    default_cfg.block_size     = LFS_BLOCK_SIZE;
#else
    default_cfg.block_size     = part_info->sector_size;
#endif
#ifdef CONFIG_LFS_BLOCK_COUNT
    default_cfg.block_count    = LFS_BLOCK_COUNT;
#else
    default_cfg.block_count    = part_info->length / part_info->sector_size;
#endif
#ifdef CONFIG_LFS_READ_SIZE
    default_cfg.read_size      = LFS_READ_SIZE;
#else
    default_cfg.read_size      = 256;
#endif
#ifdef CONFIG_LFS_PROG_SIZE
    default_cfg.prog_size      = LFS_PROG_SIZE;
#else
    default_cfg.prog_size      = 256;
#endif
#ifdef CONFIG_LFS_CACHE_SIZE
    default_cfg.cache_size     = LFS_CACHE_SIZE;
#else
    default_cfg.cache_size     = default_cfg.prog_size;
#endif
#ifdef CONFIG_LFS_LOOKAHEAD_SIZE
    default_cfg.lookahead_size = LFS_LOOKAHEAD_SIZE;
#else
    default_cfg.lookahead_size = GET_LOOKAHEAD_SIZE(default_cfg.block_count);
#endif
#ifdef CONFIG_LFS_BLOCK_CYCLES
    default_cfg.block_cycles   = LFS_BLOCK_CYCLES;
#else
    default_cfg.block_cycles   = 1000;
#endif

    // printf("default_cfg.block_size: %d\n", default_cfg.block_size);
    // printf("default_cfg.block_count: %d\n", default_cfg.block_count);
    // printf("default_cfg.read_size: %d\n", default_cfg.read_size);
    // printf("default_cfg.prog_size: %d\n", default_cfg.prog_size);
    // printf("default_cfg.cache_size: %d\n", default_cfg.cache_size);
    // printf("default_cfg.lookahead_size: %d\n", default_cfg.lookahead_size);
    // printf("default_cfg.block_cycles: %d\n", default_cfg.block_cycles);

    int res = lfs_vfs_mount();
    if (res == LFS_ERR_OK) {
        return vfs_register_fs(lfs_mount_path, &littlefs_ops, NULL);
    }
    return res;
}

int32_t vfs_lfs_unregister(void)
{
    lfs_vfs_unmount();
    partition_close(lfs_hdl);
    vfs_unregister_fs(lfs_mount_path);
    free(lfs_mount_path);
    return 0;
}
