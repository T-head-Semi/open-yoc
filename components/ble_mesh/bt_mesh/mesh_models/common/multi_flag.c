#include "stdint.h"
#include "ulog/ulog.h"

#ifndef bool
#define bool unsigned char
#endif

#ifndef false
#define false  (0)
#endif

#ifndef true
#define true   (1)
#endif

#define  TAG  "MULTI_FLAG"

int set_all_flags(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number)
{
    uint16_t  total_index = (actual_number + 7) / 8;
    if(total_index > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return -1;
    }
    memset(set_octets_flags,0xff,total_index);
    uint8_t the_last_flag_bits = actual_number % 8;
    if(the_last_flag_bits) {
        set_octets_flags[total_index - 1] = set_octets_flags[total_index - 1] >> (8 - the_last_flag_bits);
    }
    return 0;
}

int clear_all_flags(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number)
{
    uint16_t  total_index = (actual_number + 7) / 8;
    if( total_index > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return -1;
    }
    memset(set_octets_flags,0x00,total_index);
    return 0;
}

int clear_flag(uint8_t *set_octets_flags, uint16_t set_octets_number, uint16_t set_bit)
{
    uint16_t index = set_bit / 8;

    if(index + 1 > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return -1;
    }

    uint8_t the_last_flag_bits = set_bit % 8;
    set_octets_flags[index] &= ~(1 << the_last_flag_bits);
    return 0;
}

int set_flag(uint8_t *set_octets_flags, uint16_t set_octets_number, uint16_t set_bit)
{
    uint16_t index = set_bit / 8;

    if(index + 1 > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return -1;
    }

    uint8_t the_last_flag_bits = set_bit % 8;
    set_octets_flags[index] |= (1 << the_last_flag_bits);
    return 0;
}


int get_and_clear_flag(uint8_t *set_octets_flags, uint16_t set_octets_number, uint16_t actual_number, uint16_t *set_bit)
{
    uint16_t  total_index =  (actual_number + 7) / 8;
    if(total_index > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return -1;
    }

    uint16_t index  = 0;
    uint8_t bit_flag = 0;

    for(index = 0; index < total_index; index++) {
        if(set_octets_flags[index] != 0x00) {
            break;
        }
    }

    if(index == total_index) {
        return -1;
    }


    while((set_octets_flags[index] & (0x01 << bit_flag)) == 0x0) {
        bit_flag++;
    }

    *set_bit = 8 * index + bit_flag;
    set_octets_flags[index] &= (~(1 << bit_flag));
    return 0;

}


bool is_flag_set(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t number)
{
    uint16_t  index =  number / 8;
    if(index + 1 > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return false;
    }
    uint8_t the_last_flag_bits = number % 8;

    return set_octets_flags[index] & (0x1 << the_last_flag_bits);
}

bool is_all_flags_set(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number)
{
    uint16_t  total_index =  (actual_number + 7) / 8;
    uint8_t   equal_flag = 0;
    if(total_index > set_octets_number) {
        LOGE(TAG,"Invalid flags size");
        return false;
    }

    uint8_t the_last_flag_bits = actual_number % 8;
    uint8_t the_last_actual = 0xff;

    if(the_last_flag_bits) {
        the_last_actual = the_last_actual >> (8 - the_last_flag_bits);
    }

    for(uint16_t index = 0; index < total_index; index++) {
        equal_flag =  (index == total_index - 1) ? (set_octets_flags[index] == the_last_actual)  : (set_octets_flags[index]  == 0xff);
        if(!equal_flag) {
            return false;
        }
    }
    return true;
}



bool is_no_flag_set(uint8_t *set_octets_flags,uint16_t set_octets_number,  uint16_t actual_number)
{
    uint16_t  total_index =  (actual_number + 7) / 8;
    if(total_index > set_octets_number || !total_index) {
        LOGE(TAG,"Invalid flags size");
        return false;
    }

    uint8_t the_last_flag_bits = actual_number % 8;
    uint8_t the_last_actual = 0xff;

    if(the_last_flag_bits) {
        the_last_actual = (the_last_actual >> (8 - the_last_flag_bits)) & set_octets_flags[total_index -1];
    } else {
        the_last_actual =  set_octets_flags[total_index -1];
    }

    if(the_last_actual) {
        return false;
    }

    for(uint16_t index = 0; index < total_index - 1; index++) {
        if((set_octets_flags[index] & 0xff) != 0x00) {
            return false;
        }
    }
    return true;
}

