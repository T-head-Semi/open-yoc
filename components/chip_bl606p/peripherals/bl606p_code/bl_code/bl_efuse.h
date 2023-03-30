#ifndef __BL_EFUSE_H__
#define __BL_EFUSE_H__
#include <stdint.h>

int bl_efuse_get_mac(uint8_t mac[6]);

// 0 success, other invalid
int bl_efuse_get_mac_byslot(uint8_t slot, uint8_t mac[6]);
int bl_efuse_read_mac_smart(uint8_t smart, uint8_t mac[6], uint8_t slot);
int bl_efuse_write_mac_smart(uint8_t smart, uint8_t mac[6], uint8_t slot);
#endif
