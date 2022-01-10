/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 */

#include <sys/random.h>
#include <errno.h>
#if defined(CONFIG_TEE_CA)
#include <drv/tee.h>
#endif

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    (void)flags;
    if (buf == NULL) {
        errno = EFAULT;
        return -1;
    }
#if defined(CONFIG_TEE_CA)
    csi_tee_rand_generate(buf, buflen);
#else
    int i;
    uint32_t random;
    int mod = buflen % 4;
    int count = 0;
    unsigned char *output = buf;
    static uint32_t rnd = 0x12345;
    for (i = 0; i < buflen / 4; i++) {
        random = rnd * 0xFFFF777;
        rnd = random;
        output[count++] = (random >> 24) & 0xFF;
        output[count++] = (random >> 16) & 0xFF;
        output[count++] = (random >> 8) & 0xFF;
        output[count++] = (random) & 0xFF;
    }
    random = rnd * 0xFFFF777;
    rnd = random;
    for (i = 0; i < mod; i++) {
        output[i + count] = (random >> 8 * i) & 0xFF;
    }
#endif
    return buflen;
}
