/**************************************************************************
 * File Name                   : tls_efuse.c
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 : Use Flash Addr as virtual efuse
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wm_debug.h"
#include "wm_regs.h"
#include "wm_efuse.h"
#include "wm_config.h"
#include "wm_list.h"
#include "wm_internal_flash.h"
#include "wm_crypto_hard.h"
#include "wm_mem.h"

#define USE_OTA_FT_PARAM  0
#include "wm_flash_map.h"

extern void flashSRRW(unsigned long offset,unsigned char *buf,unsigned long sz, unsigned char *backbuf, unsigned int backlen, unsigned int rd);


#define FT_MAGICNUM_ADDR		(FLASH_BASE_ADDR)
#define MAGICNUM_LEN			(4)
#define MAC_ADDR_LEN			(8)
#define FT_GAIN_LEN				(84)

typedef struct FT_PARAM
{
	unsigned int		magic_no;
	unsigned int 		checksum;
	unsigned char		wifi_mac_addr[MAC_ADDR_LEN];
	unsigned char		bt_mac_addr[MAC_ADDR_LEN];
	unsigned int		tx_dcoffset;
	unsigned int		rx_dcoffset;	
	unsigned int		tx_iq_gain;
	unsigned int		rx_iq_gain;	
	unsigned int		tx_iq_phase;
	unsigned int		rx_iq_phase;	
	unsigned char		tx_gain[FT_GAIN_LEN];	
}FT_PARAM_ST;

static u8 default_mac[6] = {0x00,0x25,0x08,0x09,0x01,0x0F};

FT_PARAM_ST gftParam;
int tls_ft_param_init(void)
{
	u32 crcvalue = 0;
	psCrcContext_t ctx;	
	FT_PARAM_ST *pft = NULL;
	
	if (gftParam.magic_no == SIGNATURE_WORD)
	{
		return TRUE;
	}

	pft = tls_mem_alloc(sizeof(FT_PARAM_ST));
	if (pft == NULL)
	{
		return FALSE;
	}

	memset(pft, 0xFF, sizeof(FT_PARAM_ST));
	memset(&gftParam, 0xFF, sizeof(FT_PARAM_ST));

	tls_fls_read(FT_MAGICNUM_ADDR, (unsigned char *)pft, sizeof(FT_PARAM_ST));
	if (pft->magic_no == SIGNATURE_WORD)
	{
		tls_crypto_init();
		tls_crypto_crc_init(&ctx, 0xFFFFFFFF, CRYPTO_CRC_TYPE_32, INPUT_REFLECT | OUTPUT_REFLECT);
		tls_crypto_crc_update(&ctx, (unsigned char *)pft + 8, sizeof(FT_PARAM_ST) - 8);
		tls_crypto_crc_final(&ctx, &crcvalue);		
		if (pft->checksum != crcvalue)
		{
			tls_mem_free(pft);
			return FALSE;
		}

		if (gftParam.magic_no != SIGNATURE_WORD)
		{
			memcpy(&gftParam, pft, sizeof(FT_PARAM_ST));
		}
	}
	tls_mem_free(pft);

	return TRUE;
}

int tls_ft_param_get(unsigned int opnum, void *data, unsigned int rdlen)
{
	switch (opnum)
	{
		case CMD_WIFI_MAC:	/*MAC*/
			if ((gftParam.wifi_mac_addr[0]&0x1)
				||(0 == (gftParam.wifi_mac_addr[0]|gftParam.wifi_mac_addr[1]|gftParam.wifi_mac_addr[2]|gftParam.wifi_mac_addr[3]|gftParam.wifi_mac_addr[4]|gftParam.wifi_mac_addr[5])))		
			{
				memcpy(data, default_mac, rdlen);
			}
			else
			{
				memcpy(data, gftParam.wifi_mac_addr, rdlen);
			}
		break;
		case CMD_BT_MAC:	/*MAC*/
            {
                u8 invalid_bt_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                u8 invalid_bt_mac1[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			    if ((memcmp(gftParam.bt_mac_addr, invalid_bt_mac, 6) == 0)||(memcmp(gftParam.bt_mac_addr, invalid_bt_mac1, 6) == 0))     
			    {
				    memcpy(data, default_mac, rdlen);
                    *((u8*)data+5) +=1;      /*defalut plus 1*/
                    *((u8*)data) |= 0xC0;    /*defalut public static type*/
			    }
			    else
			    {
				    memcpy(data, gftParam.bt_mac_addr, rdlen);
			    }
           }
		break;

		
		case CMD_TX_DC: /*tx_dcoffset*/
			*(unsigned int *)data = gftParam.tx_dcoffset;
		break;	
		
		case CMD_RX_DC: /*rx_dcoffset*/
			*(unsigned int *)data = gftParam.rx_dcoffset;
		break;	
		
		case CMD_TX_IQ_GAIN:
			*(unsigned int *)data = gftParam.tx_iq_gain;
		break;	
		
		case CMD_RX_IQ_GAIN:
			*(unsigned int *)data = gftParam.rx_iq_gain;
		break;
		
		case CMD_TX_IQ_PHASE:
			*(unsigned int *)data = gftParam.tx_iq_phase;
		break;	
		
		case CMD_RX_IQ_PHASE:
			*(unsigned int *)data = gftParam.rx_iq_phase;
		break;
		
		case CMD_TX_GAIN: /*gain*/
			if (rdlen < FT_GAIN_LEN)
			{
				memcpy(data, gftParam.tx_gain, rdlen);
			}
			else
			{
				memcpy(data, gftParam.tx_gain, FT_GAIN_LEN);
			}
		break;
		
		default:
		return -1;
	}
	//printf("tls_ft_param_get: opnum=%d, val=%x\n", opnum, *(unsigned int *)data);
	return 0;
}

int tls_ft_param_set(unsigned int opnum, void *data, unsigned int len)
{
	psCrcContext_t ctx;
	unsigned int writelen = 0;

	if (!data || !len)
	{
		return -1;
	}
	//printf("tls_ft_param_set: opnum=%d, val=%x\n", opnum, *(unsigned int *)data);
	//tls_fls_read(FT_MAGICNUM_ADDR, (unsigned char *)&gftParam, sizeof(gftParam));
	switch (opnum)
	{
		case CMD_WIFI_MAC:	/*MAC*/
			memcpy(gftParam.wifi_mac_addr, (unsigned char *)data, len);
		break;

		case CMD_BT_MAC:	/*BT MAC*/
			memcpy(gftParam.bt_mac_addr, (unsigned char *)data, len);
		break;

		case CMD_TX_DC:	/*tx_dcoffset*/
			gftParam.tx_dcoffset = *(unsigned int *)data;
		break;	
		
		case CMD_RX_DC:	/*rx_dcoffset*/
			gftParam.rx_dcoffset = *(unsigned int *)data;
		break;	
		
		case CMD_TX_IQ_GAIN:
			gftParam.tx_iq_gain = *(unsigned int *)data;
		break;	
		
		case CMD_RX_IQ_GAIN:
			gftParam.rx_iq_gain = *(unsigned int *) data;			
		break;	
		
		case CMD_TX_IQ_PHASE:
			gftParam.tx_iq_phase = *(unsigned int *)data;
		break;	
		
		case CMD_RX_IQ_PHASE:
			gftParam.rx_iq_phase = *(unsigned int *) data;			
		break;	
		
		case CMD_TX_GAIN: /*gain*/
			if (len >= FT_GAIN_LEN)
			{
				writelen = FT_GAIN_LEN;
			}
			else
			{
				writelen = len;
			}
			memcpy(gftParam.tx_gain, data, writelen);
		break;
		
		default:
		return -1;
	}

	tls_crypto_init();
	tls_crypto_crc_init(&ctx, 0xFFFFFFFF, CRYPTO_CRC_TYPE_32, INPUT_REFLECT | OUTPUT_REFLECT);
	gftParam.magic_no = SIGNATURE_WORD;
	tls_crypto_crc_update(&ctx, (unsigned char *)&gftParam + 8, sizeof(gftParam) -8);
	tls_crypto_crc_final(&ctx, &gftParam.checksum);
	tls_flash_unlock();
	tls_fls_write(FT_MAGICNUM_ADDR, (unsigned char *)&gftParam, sizeof(gftParam));
	tls_flash_lock();
	return 0;
}


/**********************************************************************************************************
* Description: 	This function is used to get mac addr.
*
* Arguments  : 	mac		mac addr,6 byte
*
* Returns    : 	TLS_EFUSE_STATUS_OK			get success
* 			TLS_EFUSE_STATUS_EIO		get failed
**********************************************************************************************************/
int tls_get_mac_addr(u8 *mac)
{
	return tls_ft_param_get(CMD_WIFI_MAC, mac, 6);
}

/**********************************************************************************************************
* Description: 	This function is used to set mac addr.
*
* Arguments  : 	mac		mac addr,6 byte
*
* Returns    : 	TLS_EFUSE_STATUS_OK			get success
* 			TLS_EFUSE_STATUS_EIO		get failed
**********************************************************************************************************/
int tls_set_mac_addr(u8 *mac)
{
	return tls_ft_param_set(CMD_WIFI_MAC, mac, 6);
}

/**********************************************************************************************************
* Description: 	This function is used to get bluetooth mac addr.
*
* Arguments  : 	mac		mac addr,6 byte
*
* Returns    : 	TLS_EFUSE_STATUS_OK			get success
* 			TLS_EFUSE_STATUS_EIO		get failed
**********************************************************************************************************/
int tls_get_bt_mac_addr(u8 *mac)
{
	return tls_ft_param_get(CMD_BT_MAC, mac, 6);
}

/**********************************************************************************************************
* Description: 	This function is used to set bluetooth mac addr.
*
* Arguments  : 	mac		mac addr,6 byte
*
* Returns    : 	TLS_EFUSE_STATUS_OK			get success
* 			TLS_EFUSE_STATUS_EIO		get failed
**********************************************************************************************************/
int tls_set_bt_mac_addr(u8 *mac)
{
	return tls_ft_param_set(CMD_BT_MAC, mac, 6);
}


/**********************************************************************************************************
* Description: 	This function is used to get tx lod.
*
* Arguments  : 	*txlo
*
* Returns    : 	0		get success
* 				-1		get failed
**********************************************************************************************************/
int tls_get_tx_lo(u8 *txlo)
{
	return tls_ft_param_get(CMD_TX_DC, txlo, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to set tx lo.
*
* Arguments  : 	txlo
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_set_tx_lo(u8 *txlo)
{
	return tls_ft_param_set(CMD_TX_DC, txlo, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to get tx iq gain.
*
* Arguments  : 	txGain
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_get_tx_iq_gain(u8 *txGain)
{
	return tls_ft_param_get(CMD_TX_IQ_GAIN, txGain, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to set tx iq gain.
*
* Arguments  : 	txGain
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_set_tx_iq_gain(u8 *txGain)
{
	return tls_ft_param_set(CMD_TX_IQ_GAIN, txGain, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to get rx iq gain.
*
* Arguments  : 	rxGain
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_get_rx_iq_gain(u8 *rxGain)
{
	return tls_ft_param_get(CMD_RX_IQ_GAIN, rxGain, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to set rx iq gain.
*
* Arguments  : 	rxGain
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_set_rx_iq_gain(u8 *rxGain)
{
	return tls_ft_param_set(CMD_RX_IQ_GAIN, rxGain, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to get tx iq phase.
*
* Arguments  : 	txPhase
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_get_tx_iq_phase(u8 *txPhase)
{
	return tls_ft_param_get(CMD_TX_IQ_PHASE, txPhase, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to set tx iq phase.
*
* Arguments  : 	txPhase
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_set_tx_iq_phase(u8 *txPhase)
{
	return tls_ft_param_set(CMD_TX_IQ_PHASE, txPhase, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to get rx iq phase.
*
* Arguments  : 	rxPhase
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_get_rx_iq_phase(u8 *rxPhase)
{
	return tls_ft_param_get(CMD_RX_IQ_PHASE, rxPhase, 4);
}

/**********************************************************************************************************
* Description: 	This function is used to set tx iq phase.
*
* Arguments  : 	rxPhase
*
* Returns    : 	0		set success
* 				-1		set failed
**********************************************************************************************************/
int tls_set_rx_iq_phase(u8 *rxPhase)
{
	return tls_ft_param_set(CMD_RX_IQ_PHASE, rxPhase, 4);
}


int tls_freq_err_op(u8 *freqerr, u8 flag)
{
	tls_flash_unlock();
	if (flag){
		tls_fls_write(FT_MAGICNUM_ADDR + sizeof(FT_PARAM_ST), freqerr, 4);
	}
	else
	{
		tls_fls_read(FT_MAGICNUM_ADDR + sizeof(FT_PARAM_ST), freqerr, 4);
	}
	tls_flash_lock();
	return TLS_EFUSE_STATUS_OK;
}

int tls_rf_vcg_ctrl_op(u8 *vcg, u8 flag)
{
	tls_flash_unlock();
	if (flag){
		tls_fls_write(VCG_ADDR, vcg, VCG_LEN);
	}
	else
	{
		tls_fls_read(VCG_ADDR, vcg, VCG_LEN);
	}
	tls_flash_lock();	
	return TLS_EFUSE_STATUS_OK;
}

/**********************************************************************************************************
* Description: 	This function is used to get tx gain.
*
* Arguments  : 	txgain		tx gain
*
* Returns    : 	0		get success
* 				-1		get failed
**********************************************************************************************************/
int tls_get_tx_gain(u8 *txgain)
{
	return tls_ft_param_get(CMD_TX_GAIN, txgain, TX_GAIN_LEN);
}

/**********************************************************************************************************
* Description: 	This function is used to set tx gain.
*
* Arguments  : 	txgain		tx gain
*
* Returns    : 	0			set success
* 				-1			set failed
**********************************************************************************************************/
int tls_set_tx_gain(u8 *txgain)
{

	return tls_ft_param_set(CMD_TX_GAIN, txgain, TX_GAIN_LEN);
	

}




