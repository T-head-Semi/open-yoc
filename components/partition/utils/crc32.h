#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>
#include <stddef.h>

uint32_t pcrc32 (uint32_t crc, unsigned char *buf, size_t len);

#endif
