#ifndef _MULTI_FLAG_H_
#define _MULTI_FLAG_H_

int set_all_flags(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number);
int clear_all_flags(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number);
int clear_flag(uint8_t *set_octets_flags, uint16_t set_octets_number, uint16_t set_bit);
int set_flag(uint8_t *set_octets_flags, uint16_t set_octets_number, uint16_t set_bit);
int get_and_clear_flag(uint8_t *set_octets_flags, uint16_t set_octets_number, uint16_t actual_number, uint16_t *set_bit);
bool is_flag_set(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t number);
bool is_all_flags_set(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number);
bool is_no_flag_set(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number);


#endif
