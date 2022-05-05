/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <aos/kernel.h>
#include <yoc/netio.h>

#define TAG "fota"

#ifndef __linux__
#include <aos/aos.h>
#include <yoc/partition.h>
static int flash_open(netio_t *io, const char *path)
{
    partition_t handle = partition_open(path + sizeof("flash://") - 1);

    if (handle >= 0) {
        partition_info_t *lp = hal_flash_get_info(handle);
        aos_assert(lp);

        io->size = lp->length;
        io->block_size = lp->sector_size;

        io->private = (void *)((long)handle);

        return 0;
    }

    return -1;
}

static int flash_close(netio_t *io)
{
    partition_t handle = (partition_t)((long)io->private);
    partition_close(handle);

    return 0;
}

static int flash_read(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    partition_t handle = (partition_t)((long)io->private);

    if (io->size - io->offset < length)
        length = io->size - io->offset;

    if (partition_read(handle, io->offset, buffer, length) >= 0) {
        io->offset += length;
        return length;
    }

    return -1;
}

static int fota_flash_erase(partition_t partition, off_t off_set, int block_size, uint32_t block_count)
{
    if (off_set % block_size == 0) {
        if (partition_erase(partition, off_set, block_count) < 0) {
            LOGD(TAG, "0 erase addr:%x length:%x\n", off_set, block_count);
            return -1;
        }
    }

    return 0;
}

static int flash_write(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    partition_t handle = (partition_t)((long)io->private);
    // LOGD(TAG, "%d %d %d\n", io->size, io->offset, length);
    if (io->size - io->offset < length)
        length = io->size - io->offset;
    // LOGD(TAG, "length %d\n", length);
    if (fota_flash_erase(handle, io->offset + (io->block_size << 1), io->block_size, (length + io->block_size - 1) / io->block_size) < 0) {
        LOGE(TAG, "erase addr:%x length:%x\n", io->offset + (io->block_size << 1), (length + io->block_size - 1) / io->block_size);

        return -1;
    }

    if (partition_write(handle, io->offset + (io->block_size << 1), buffer, length) >= 0) {
        // LOGD(TAG, "write addr:%x length:%x\n", io->offset + (io->block_size << 1), length);
        io->offset += length;
        return length;
    }

    LOGD(TAG, "write fail addr:0x%x length:0x%x\n", io->offset + (io->block_size << 1), length);
    return -1;
}

static int flash_seek(netio_t *io, size_t offset, int whence)
{
    // partition_t handle = (partition_t)io->private;

    switch (whence) {
        case SEEK_SET:
            io->offset = offset;
            return 0;
        case SEEK_CUR:
            io->offset += offset;
            return 0;
        case SEEK_END:
            io->offset = io->size - offset;
            return 0;
    }

    return -1;
}
#else
typedef int partition_t;

static int flash_open(netio_t *io, const char *path)
{
    return 0;
}

static int flash_close(netio_t *io)
{
    return 0;
}

static int flash_read(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    return 0;
}

static int flash_write(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    return 0;
}

static int flash_seek(netio_t *io, size_t offset, int whence)
{
    // partition_t handle = (partition_t)io->private;

    switch (whence) {
        case SEEK_SET:
            io->offset = offset;
            return 0;
        case SEEK_CUR:
            io->offset += offset;
            return 0;
        case SEEK_END:
            io->offset = io->size - offset;
            return 0;
    }

    return -1;
}
#endif /* __linux__ */

const netio_cls_t flash = {
    .name = "flash",
    .open = flash_open,
    .close = flash_close,
    .write = flash_write,
    .read = flash_read,
    .seek = flash_seek,
};

int netio_register_flash(void)
{
    return netio_register(&flash);
}