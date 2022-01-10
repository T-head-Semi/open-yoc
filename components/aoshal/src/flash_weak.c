/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


#  define _weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

#  define weak_alias(name, aliasname) _weak_alias (name, aliasname)
static void weak_func(void)
{
}

weak_alias(weak_func,csi_spiflash_qspi_init)
weak_alias(weak_func,csi_spiflash_get_flash_info)
weak_alias(weak_func,csi_spiflash_erase)
weak_alias(weak_func,csi_spiflash_program)
weak_alias(weak_func,csi_eflash_read)
weak_alias(weak_func,csi_eflash_erase_sector)
weak_alias(weak_func,csi_eflash_program)
weak_alias(weak_func,csi_eflash_init)
weak_alias(weak_func,csi_eflash_erase)


