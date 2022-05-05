/******************************************************************************
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#ifndef _MMC_SDIO_CARD_H
#define _MMC_SDIO_CARD_H
#ifdef __cplusplus
extern "C" {
#endif


#ifdef CONFIG_READ_CIS
/*
 * SDIO function CIS tuple (unknown to the core)
 */
struct sdio_func_tuple {
	struct sdio_func_tuple *next;
	unsigned char code;
	unsigned char size;
	unsigned char data[0];
};
#endif


/*
 * SDIO function devices
 */
struct rtl_sdio_func {
	struct rtl_mmc_card		*card;		/* the card this device belongs to */
	void	(*irq_handler)(struct rtl_sdio_func *); /* IRQ callback */

	unsigned	int	max_blksize;	/* maximum block size */ //add
	unsigned	int	cur_blksize;	/* current block size */	 //add
	unsigned	int	enable_timeout;	/* max enable timeout in msec */ //add
	unsigned int	num;		/* function number *///add
	unsigned short		vendor;		/* vendor id */ //add
	unsigned short		device;		/* device id */ //add
	unsigned		num_info;	/* number of info strings */ //add
	const char		**info;		/* info strings */ //add
	unsigned char		class;		/* standard interface class *///add

	unsigned int			tmpbuf_reserved; //for tmpbuf 4 byte alignment
	unsigned char			tmpbuf[4];	/* DMA:able scratch buffer */

#ifdef CONFIG_READ_CIS
	struct sdio_func_tuple *tuples;
#endif
	void *drv_priv;
};

struct rtl_sdio_cccr {
	unsigned int		sdio_vsn;
	unsigned int		sd_vsn;
	unsigned int		multi_block:1;
	unsigned int		low_speed:1;
	unsigned int		wide_bus:1;
	unsigned int		high_power:1;
	unsigned int		high_speed:1;
	unsigned int		disable_cd:1;
};

struct rtl_sdio_cis {
	unsigned short		vendor;
	unsigned short		device;
	unsigned short		blksize;
	unsigned int		max_dtr;
};

struct rtl_mmc_card {
	struct rtl_mmc_card		*host;		/* the host this device belongs to */
	struct rtl_sdio_cccr		cccr;		/* common card info */
	struct rtl_sdio_cis		cis;		/* common tuple info */ //add
	struct rtl_sdio_func	*rtl_sdio_func[7]; /* SDIO functions (devices) *///add
	unsigned int		sdio_funcs;	/* number of SDIO functions *///add

	unsigned int			rca;			/* relative card address of device */
	unsigned int			type;		/* card type */

	unsigned		num_info;	/* number of info strings *///add
	const char		**info;		/* info strings *///add
#ifdef CONFIG_READ_CIS
	struct sdio_func_tuple	*tuples;	/* unknown common tuples *///add
#endif
};
#ifdef __cplusplus
}
#endif

#endif
