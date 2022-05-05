#include "nsp_driver.h"
#include "stdio.h"
#include "drv/gpio.h"
// #include "pin_name.h"
// #include "pinmux.h"
#include "ulog/ulog.h"

//Use this definition if the input mode is QUASI
//#define QUASI_MODE //?蝜?怀?踽?宒粒蚚QUASIㄛ?輮墓繫侀例?
//===========================================================
//Systerm intial, please modify according to your MCU platform
/*#define SCLK_PORT		BP0R
#define SCLK_PORT_BIT	BIT0
#define SCLK_PORT_D		BP0D
#define SCLK_PORT_M		BP0M

#define SDA_PORT		BP0R
#define SDA_PORT_BIT	BIT1
#define SDA_PORT_D		BP0D
#define SDA_PORT_M		BP0M*/

UINT8 	u8HOST_FLAG=0; 
UINT8 	u8RX_ERROR_COUNT=0;
UINT16  u16CMD_TX_BYTE	= 0;
UINT16  u16CMD_RX_BYTE	= 0;
UINT8  	CMD_TX_DATA[8]={0};		
UINT8  	CMD_RX_DATA[5]={0};		

// static gpio_pin_handle_t     clk_handle = NULL; 

// static gpio_pin_handle_t     sda_handle = NULL;

#define TAG  "NSP_DRIVER"

extern void udelay(uint32_t us);


//===========================================================
//Systerm intial, please modify according to your MCU platform
void GPIO1_SCLK_OUTPUT(void) 
{
// //設置為輸出模式  Set to output mode
//     if(!clk_handle) {
//         //   drv_pinmux_config(NSP_CLK_PIN, PIN_FUNC_GPIO);
//     clk_handle = csi_gpio_pin_initialize(NSP_CLK_PIN, NULL);
//     if ((clk_handle == NULL)) {
//         LOGE(TAG, "clk gpio init fail");
//         return;
//     }
// 	}

//     csi_gpio_pin_config_mode(clk_handle, GPIO_MODE_PUSH_PULL);
//     csi_gpio_pin_config_direction(clk_handle, GPIO_DIRECTION_OUTPUT);
}

//Systerm intial, please modify according to your MCU platform
void GPIO1_SCLK_OUTPUT0(void) 
{
//設置為輸出低電平 Set to output low level
    // csi_gpio_pin_write(clk_handle, 0);
}

//Systerm intial, please modify according to your MCU platform
void GPIO1_SCLK_OUTPUT1(void) 
{
    //設置為輸出高電平 Set to output High level
    // csi_gpio_pin_write(clk_handle, 1);
}
//----------------------------------
//Systerm intial, please modify according to your MCU platform
void GPIO2_SDA_OUTPUT(void) 
{
//設置為輸出模式  Set to output mode
    // drv_pinmux_config(NSP_SDA_PIN, PIN_FUNC_GPIO);
    // sda_handle = csi_gpio_pin_initialize(NSP_SDA_PIN, NULL);
    // if ((sda_handle == NULL)) {
    //     LOGE(TAG, "clk gpio init fail");
    //     return;
    // }
    // csi_gpio_pin_config_mode(sda_handle, GPIO_MODE_PUSH_PULL);
    // csi_gpio_pin_config_direction(sda_handle, GPIO_DIRECTION_OUTPUT);
}
//Systerm intial, please modify according to your MCU platform
void GPIO2_SDA_OUTPUT0(void) 
{
//設置為輸出低電平 Set to output low level
    // csi_gpio_pin_write(sda_handle, 0);
}
//Systerm intial, please modify according to your MCU platform
void GPIO2_SDA_OUTPUT1(void) 
{
//設置為輸出高電平 Set to output High level
    // csi_gpio_pin_write(sda_handle, 1);
}
//----------------------------------
//Systerm intial, please modify according to your MCU platform
void GPIO2_SDA_INPUT(void) 
{
//設置為輸入模式  Set to input mode
    // if(!sda_handle) {
    //      drv_pinmux_config(NSP_SDA_PIN, PIN_FUNC_GPIO);
    // sda_handle = csi_gpio_pin_initialize(NSP_SDA_PIN, NULL);
    // if ((sda_handle == NULL)) {
    //     LOGE(TAG, "clk gpio init fail");
    //     return;
    // }
	// }

    // csi_gpio_pin_config_mode(sda_handle, GPIO_MODE_PULLDOWN);
    // csi_gpio_pin_config_direction(sda_handle, GPIO_DIRECTION_INPUT); 
}
//Systerm intial, please modify according to your MCU platform
UINT8 GPIO2_SDA_INPUT_READ() 
{
	UINT8 u8Res = 0;
//讀取輸入數值     Read input values
    // csi_gpio_pin_read(sda_handle, (bool *)&u8Res);
	return (u8Res);
}
//----------------------------------
//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_PIN_DURATION(void)
{
//設置延遲時間 100us;  Delay time 100 us
    udelay(100);
}

//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_PIN_DURATION_SHORT(void)
{
//設置延遲時間 20us;  Delay time 20 us
    udelay(20);
}

//Systerm intial,0 please modify according to your MCU platform, during which you can perform other operations on host
void HOST_CMD_INTERVAL_SHORT(void)
{
//設置延遲時間 60us;  Delay time 60 us
    udelay(60);
}

//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_CMD_INTERVAL(void)
{
//設置延遲時間 300us;  Delay time 300 us
    udelay(300);
}

//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_Delay500uS(void)
{
//設置延遲時間 500us;  Delay time 500 us
    udelay(500);
}
//===========================================================
//protocol
void HOST_BUS_Init(void)
{
	GPIO1_SCLK_OUTPUT();
	GPIO2_SDA_OUTPUT();
	GPIO1_SCLK_OUTPUT1();
	GPIO2_SDA_OUTPUT1();	
}

void CMD_RX_DATA_INIT(void)
{
	CMD_RX_DATA[0] = 0;
	CMD_RX_DATA[1] = 0;
	CMD_RX_DATA[2] = 0;
	CMD_RX_DATA[3] = 0;
	CMD_RX_DATA[4] = 0;
}


void HOST_Init(UINT8* SP_VOL)
{
	*SP_VOL = 0x80;	
	HOST_BUS_Init();
}

void I2C_LIKE_START(void)
{
	//SCLK:1,SDA:1->0
	GPIO1_SCLK_OUTPUT1();
	HOST_PIN_DURATION();
	
	GPIO2_SDA_OUTPUT1();
	HOST_PIN_DURATION();
	GPIO2_SDA_OUTPUT0();
	HOST_PIN_DURATION();
}

void I2C_LIKE_STOP(void)
{
	//SCLK:1,SDA:0->1
	GPIO1_SCLK_OUTPUT0();
	GPIO2_SDA_OUTPUT0();
	//HOST_PIN_DURATION();
	HOST_PIN_DURATION();
	
	GPIO1_SCLK_OUTPUT1();
	HOST_PIN_DURATION();
	GPIO2_SDA_OUTPUT1();
	HOST_PIN_DURATION();	
}

void I2C_LIKE_START_SHORT(void)
{
	//SCLK:1,SDA:1->0
	GPIO1_SCLK_OUTPUT1();
	HOST_PIN_DURATION_SHORT();
	
	GPIO2_SDA_OUTPUT1();
	HOST_PIN_DURATION_SHORT();
	GPIO2_SDA_OUTPUT0();
	HOST_PIN_DURATION_SHORT();
}

void I2C_LIKE_STOP_SHORT(void)
{
	//SCLK:1,SDA:0->1
	GPIO1_SCLK_OUTPUT0();
	GPIO2_SDA_OUTPUT0();
	//HOST_PIN_DURATION_SHORT();
	HOST_PIN_DURATION_SHORT();
	
	GPIO1_SCLK_OUTPUT1();
	HOST_PIN_DURATION_SHORT();
	GPIO2_SDA_OUTPUT1();
	HOST_PIN_DURATION_SHORT();	
}
//------------------------------------------------
UINT8 I2C_LIKE_TX_RX_PROCESS()
{	
	UINT8 i = 0;
	UINT8 j = 0;
	UINT8 n = 0;
	UINT8 u8BitMap[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	
	if ((CMD_TX_DATA[0] == CMD_IO_TYPE) || (CMD_TX_DATA[0] == CMD_GET_INOUT))
	{
		n = 1;
	}
	
	CMD_RX_DATA_INIT();
	u8HOST_FLAG |= FLAG_TX_DOING;
	
	I2C_LIKE_START();
	
	//TX process
	for(i = 0; i < u16CMD_TX_BYTE; i++)
	{
		for(j = 0; j < 8; j++)
		{
			GPIO1_SCLK_OUTPUT0();
			//if( ((CMD_TX_DATA[i] << j) & 0x80) > 0 )
			if ((CMD_TX_DATA[i] & u8BitMap[j]) !=0 )
			{
				GPIO2_SDA_OUTPUT1();
			}
			else
			{
				GPIO2_SDA_OUTPUT0();
			}
			HOST_PIN_DURATION();
			
			GPIO1_SCLK_OUTPUT1();
			HOST_PIN_DURATION();
		}
	}
	
	u8HOST_FLAG &= ~FLAG_TX_DOING;
	u8HOST_FLAG |= FLAG_RX_DOING;
#ifdef QUASI_MODE
    	GPIO2_SDA_OUTPUT1();
#endif
	GPIO2_SDA_INPUT();
	
	//RX process
	for(i = 0; i < u16CMD_RX_BYTE; i++)
	{
		for(j = 0; j < 8; j++)
		{
			GPIO1_SCLK_OUTPUT0();
			HOST_PIN_DURATION();
			
			GPIO1_SCLK_OUTPUT1();
			HOST_PIN_DURATION();
			
			CMD_RX_DATA[i]=CMD_RX_DATA[i]<<1;
			if ( GPIO2_SDA_INPUT_READ() != 0 )
			{
				CMD_RX_DATA[i] = CMD_RX_DATA[i] | 0x1;
			}
		}		
	}			
	
	GPIO2_SDA_OUTPUT();		//SDA:input->output
	I2C_LIKE_STOP();
	u8HOST_FLAG &= ~FLAG_RX_DOING;
	
	if (u16CMD_RX_BYTE == 1)
	{
		if (CMD_RX_DATA[0] == RIGHT_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		if (CMD_RX_DATA[0] == ERROR_RTN)
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}	
		if (CMD_RX_DATA[0] == UNSUPPORTED_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 0xFF;
		}
	}
	else if (u16CMD_RX_BYTE == 2)
	{
		if (((UINT8)(CMD_RX_DATA[0] ^ CMD_RX_DATA[1]) & 0xFF)== 0xFF) 
		{
			u8RX_ERROR_COUNT = 0;
			if ((n == 1) && (CMD_RX_DATA[0] == UNSUPPORTED_RTN))
			{
				return 0xFF;	
			}
			else
			{
				return 1;
			}
		}
		else 
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}
	}
	else
	{
		j = 0;
		for(i = 0; i <= (u16CMD_RX_BYTE-2); i++)
		{
			j = j + CMD_RX_DATA[i];
		}
		j = j ^ CMD_RX_DATA[u16CMD_RX_BYTE-1];
		
		if (j == 0xFF)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		else
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}
	}
	return 0;
}
//------------------------------------------------
UINT8 I2C_LIKE_TX_RX_BUFFER_PROCESS(UINT8 *DATA_BUFFER)
{	
	UINT16 i = 0;
	UINT8 j = 0;
	UINT8 n = 0;
	UINT8 m = 0;
	UINT8 CMD_CHECKSUM = 0;
	//UINT8 CMD_CHECKSUM_CHECK = 0;
	UINT8 u8BitMap[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	
	if (CMD_TX_DATA[0] == CMD_MULTI_PLAY)
	{
		n = 1;
	}
	/*else if (CMD_TX_DATA[0] == )
	{
		n = 2;
	}*/
	
	CMD_RX_DATA_INIT();
	u8HOST_FLAG |= FLAG_TX_DOING;
	
	I2C_LIKE_START();
	
	//TX process
	for(i = 0; i < u16CMD_TX_BYTE; i++)
	{
		if (n == 1)
		{
			if (i <= 1) 
			{
				m = i;
				CMD_CHECKSUM = ((CMD_CHECKSUM + CMD_TX_DATA[m]) & 0xFF);
			}
			else if ((i >= 2) && (i < (u16CMD_TX_BYTE - 1)))
			{
				m = 2;
				CMD_TX_DATA[m] = *DATA_BUFFER;
				CMD_CHECKSUM = ((CMD_CHECKSUM + CMD_TX_DATA[m]) & 0xFF);
				DATA_BUFFER++;
			}
			else if (i == (u16CMD_TX_BYTE - 1))
			{
				CMD_CHECKSUM  = CMD_CHECKSUM  ^ 0xFF;
				m = 3;
				CMD_TX_DATA[m] = CMD_CHECKSUM;
			}
		}
		
		for(j = 0; j < 8; j++)
		{
			GPIO1_SCLK_OUTPUT0();
			if ((CMD_TX_DATA[m] & u8BitMap[j]) !=0 )
			{
				GPIO2_SDA_OUTPUT1();
			}
			else
			{
				GPIO2_SDA_OUTPUT0();
			}
			HOST_PIN_DURATION();
			
			GPIO1_SCLK_OUTPUT1();
			HOST_PIN_DURATION();
		}
	}
	
	u8HOST_FLAG &= ~FLAG_TX_DOING;
	u8HOST_FLAG |= FLAG_RX_DOING;
#ifdef QUASI_MODE
    	GPIO2_SDA_OUTPUT1();
#endif
	GPIO2_SDA_INPUT();
	
	//RX process
	for(i = 0; i < u16CMD_RX_BYTE; i++)
	{
		for(j = 0; j < 8; j++)
		{
			GPIO1_SCLK_OUTPUT0();
			HOST_PIN_DURATION();
			
			GPIO1_SCLK_OUTPUT1();
			HOST_PIN_DURATION();
			if ( n == 1)
			{
				CMD_RX_DATA[i]=CMD_RX_DATA[i]<<1;
				if ( GPIO2_SDA_INPUT_READ() != 0 )
				{
					CMD_RX_DATA[i] = CMD_RX_DATA[i] | 0x1;
				}
			}
			/*else if ( n == 2) 
			{
				if (i < (u16CMD_RX_BYTE-1))
				{
					*DATA_BUFFER = *DATA_BUFFER<<1 ;
					if ( GPIO2_SDA_INPUT_READ() != 0 )
					{
						*DATA_BUFFER = *DATA_BUFFER | 0x1;
					}
				}
				else if ( i == (u16CMD_RX_BYTE-1))
				{
					CMD_CHECKSUM = CMD_CHECKSUM<<1 ;
					if ( GPIO2_SDA_INPUT_READ() != 0 )
					{
						CMD_CHECKSUM = CMD_CHECKSUM | 0x1;
					}
				}
				
			}*/
			
		}
		/*if ( n == 2) 
		{
			if (i < (u16CMD_RX_BYTE-1))
			{
				CMD_CHECKSUM_CHECK = *DATA_BUFFER + CMD_CHECKSUM_CHECK;
				DATA_BUFFER++; 
			}
		}*/
			
	}			
	
	GPIO2_SDA_OUTPUT();		//SDA:input->output
	I2C_LIKE_STOP();
	u8HOST_FLAG &= ~FLAG_RX_DOING;
	
	if (u16CMD_RX_BYTE == 1)
	{
		if (CMD_RX_DATA[0] == RIGHT_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		if (CMD_RX_DATA[0] == ERROR_RTN)
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}	
		if (CMD_RX_DATA[0] == UNSUPPORTED_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 0xFF;
		}
	}
	/*if (n == 2)
	{
		if ((CMD_CHECKSUM ^ CMD_CHECKSUM_CHECK) == 0xFF)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		else
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}
	}*/
	
	return 0;
}
//------------------------------------------------
UINT8 I2C_LIKE_TX_RX_ISP_PROCESS(UINT8 *ISP_BUFFER)
{	
	UINT16 i = 0;
	UINT8 j = 0;
	UINT8 n = 0;
	UINT8 m = 0;
	UINT8 ISP_CHECKSUM = 0;
	UINT8 ISP_CHECKSUM_CHECK = 0;
	UINT8 u8BitMap[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	
	if (CMD_TX_DATA[0] == CMD_ISP_WRITE_PAGE)
	{
		n = 1;
	}
	else if (CMD_TX_DATA[0] == CMD_ISP_READ_PAGE)
	{
		n = 2;
	}
	
	CMD_RX_DATA_INIT();
	u8HOST_FLAG |= FLAG_TX_DOING;
	
	I2C_LIKE_START_SHORT();
	
	//TX process
	for(i = 0; i < u16CMD_TX_BYTE; i++)
	{
		if (n == 1)
		{
			if (i <= 3) 
			{
				m = i;
				ISP_CHECKSUM = ((ISP_CHECKSUM + CMD_TX_DATA[m]) & 0xFF);
			}
			else if ((i >= 4) && (i < (u16CMD_TX_BYTE - 1)))
			{
				m = 4;
				CMD_TX_DATA[m] = *ISP_BUFFER;
				ISP_CHECKSUM = ((ISP_CHECKSUM + CMD_TX_DATA[m]) & 0xFF);
				ISP_BUFFER++;
			}
			else if (i == (u16CMD_TX_BYTE - 1))
			{
				ISP_CHECKSUM = ISP_CHECKSUM ^ 0xFF;
				m = 4;
				CMD_TX_DATA[m] = ISP_CHECKSUM;
			}
		}
		else if (n == 2)
		{ 
			m = i;
		}
		
		for(j = 0; j < 8; j++)
		{
			GPIO1_SCLK_OUTPUT0();
			if ((CMD_TX_DATA[m] & u8BitMap[j]) !=0 )
			{
				GPIO2_SDA_OUTPUT1();
			}
			else
			{
				GPIO2_SDA_OUTPUT0();
			}
			HOST_PIN_DURATION_SHORT();
			
			GPIO1_SCLK_OUTPUT1();
			HOST_PIN_DURATION_SHORT();
		}
	}
	
	u8HOST_FLAG &= ~FLAG_TX_DOING;
	u8HOST_FLAG |= FLAG_RX_DOING;
#ifdef QUASI_MODE
    	GPIO2_SDA_OUTPUT1();
#endif
	GPIO2_SDA_INPUT();
	
	//RX process
	for(i = 0; i < u16CMD_RX_BYTE; i++)
	{
		if ( n == 2) 
		{
			*ISP_BUFFER = 0;
		}
		for(j = 0; j < 8; j++)
		{
			GPIO1_SCLK_OUTPUT0();
			HOST_PIN_DURATION_SHORT();
			HOST_PIN_DURATION_SHORT();
			
			GPIO1_SCLK_OUTPUT1();
			HOST_PIN_DURATION_SHORT();
			HOST_PIN_DURATION_SHORT();
			
			if ( n == 1)
			{
				CMD_RX_DATA[i]=CMD_RX_DATA[i]<<1;
				if ( GPIO2_SDA_INPUT_READ() != 0 )
				{
					CMD_RX_DATA[i] = CMD_RX_DATA[i] | 0x1;
				}
			}
			else if ( n == 2) 
			{
				if (i < (u16CMD_RX_BYTE-1))
				{
					*ISP_BUFFER = *ISP_BUFFER<<1 ;
					if ( GPIO2_SDA_INPUT_READ() != 0 )
					{
						*ISP_BUFFER = *ISP_BUFFER | 0x1;
					}
				}
				else if ( i == (u16CMD_RX_BYTE-1))
				{
					ISP_CHECKSUM = ISP_CHECKSUM<<1 ;
					if ( GPIO2_SDA_INPUT_READ() != 0 )
					{
						ISP_CHECKSUM = ISP_CHECKSUM | 0x1;
					}
				}
				
			}
			
		}
		if ( n == 2) 
		{
			if (i < (u16CMD_RX_BYTE-1))
			{
				ISP_CHECKSUM_CHECK = *ISP_BUFFER + ISP_CHECKSUM_CHECK;
				ISP_BUFFER++; 
			}
		}
			
	}			
	
	GPIO2_SDA_OUTPUT();		//SDA:input->output
	I2C_LIKE_STOP_SHORT();
	u8HOST_FLAG &= ~FLAG_RX_DOING;
	
	if (u16CMD_RX_BYTE == 1)
	{
		if (CMD_RX_DATA[0] == RIGHT_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		if (CMD_RX_DATA[0] == ERROR_RTN)
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}	
		if (CMD_RX_DATA[0] == UNSUPPORTED_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 0xFF;
		}
	}
	if (n == 2)
	{
		if ((ISP_CHECKSUM ^ ISP_CHECKSUM_CHECK) == 0xFF)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		else
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}
	}
	
	return 0;
}
//----------------------------------
void N_SLAVE_RESET(void)
{
	if(u8RX_ERROR_COUNT >=5) 
	{
		u8RX_ERROR_COUNT = 0;
		N_RESET();	
	}
}
//===========================================================
// function command
UINT8 N_READ_ID(UINT32* PID)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_READ_ID;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_READ_ID_TX_BYTE;
	u16CMD_RX_BYTE = CMD_READ_ID_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		 *PID = (CMD_RX_DATA[0]<<24) | (CMD_RX_DATA[1]<<16) | (CMD_RX_DATA[2]<<8) | CMD_RX_DATA[3];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_READ_STATUS(UINT8* COMMAND_STATUS)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_READ_STATUS;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_READ_STATUS_TX_BYTE;
	u16CMD_RX_BYTE = CMD_READ_STATUS_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*COMMAND_STATUS = CMD_RX_DATA[0];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_DO_LVD(void)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_DO_LVD;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_DO_LVD_TX_BYTE;
	u16CMD_RX_BYTE = CMD_DO_LVD_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_GET_LVD(UINT8* LVD_STATUS)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_GET_LVD;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_LVD_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_LVD_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*LVD_STATUS = CMD_RX_DATA[0];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_CHECKSUM_RIGHT(UINT8* CHECKSUM_BIT)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_CHECKSUM_RIGHT;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_CHECKSUM_RIGHT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_CHECKSUM_RIGHT_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*CHECKSUM_BIT = CMD_RX_DATA[0];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_GET_CHECKSUM(UINT16* CHECKSUM_BYTES)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_GET_CHECKSUM;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_CHECKSUM_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_CHECKSUM_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*CHECKSUM_BYTES = (CMD_RX_DATA[0]<<8) | CMD_RX_DATA[1];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_DO_CHECKSUM(void)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_DO_CHECKSUM;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_DO_CHECKSUM_TX_BYTE;
	u16CMD_RX_BYTE = CMD_DO_CHECKSUM_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_GET_FW_VERSION(UINT32* PFW_VERSION)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_GET_FW_VERSION;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_FW_VERSION_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_FW_VERSION_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		 *PFW_VERSION = (CMD_RX_DATA[0]<<16) | (CMD_RX_DATA[1]<<8) | CMD_RX_DATA[2];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_PLAY(UINT16 PlayListIndex)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_PLAY;
	CMD_TX_DATA[1] = PlayListIndex >>8;
	CMD_TX_DATA[2] = PlayListIndex & 0xFF;
	CMD_TX_DATA[3] = ((CMD_TX_DATA[0] + CMD_TX_DATA[1]+ CMD_TX_DATA[2])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_PLAY_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PLAY_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
UINT8 N_PLAY_SLEEP(UINT16 PlayListIndex)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_PLAY_SLEEP;
	CMD_TX_DATA[1] = PlayListIndex >>8;
	CMD_TX_DATA[2] = PlayListIndex & 0xFF;
	CMD_TX_DATA[3] = ((CMD_TX_DATA[0] + CMD_TX_DATA[1]+ CMD_TX_DATA[2])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_PLAY_SLEEP_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PLAY_SLEEP_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
UINT8 N_IO_CONFIG(UINT8 IO_FLAG)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_IO_CONFIG;
	CMD_TX_DATA[1] = IO_FLAG;
	CMD_TX_DATA[2] = ((CMD_TX_DATA[0] + CMD_TX_DATA[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_IO_CONFIG_TX_BYTE;
	u16CMD_RX_BYTE = CMD_IO_CONFIG_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_IO_TYPE(UINT8* READ_IO_FLAG)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_IO_TYPE;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_IO_TYPE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_IO_TYPE_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*READ_IO_FLAG = CMD_RX_DATA[0];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_SET_OUT(UINT8 IO_VALUE)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_SET_OUT;
	CMD_TX_DATA[1] = IO_VALUE;
	CMD_TX_DATA[2] = ((CMD_TX_DATA[0] + CMD_TX_DATA[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_SET_OUT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_SET_OUT_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_GET_INOUT(UINT8* READ_IO_VALUE)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_GET_INOUT;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_INOUT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_INOUT_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*READ_IO_VALUE = CMD_RX_DATA[0];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
UINT8 N_BZPIN_EN()
{
	UINT8 RTN = 0;
	u8HOST_FLAG |=FLAG_SPBZOUT_EN;
	CMD_TX_DATA[0] = CMD_BZPIN_EN;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_BZPIN_EN_TX_BYTE;
	u16CMD_RX_BYTE = CMD_BZPIN_EN_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_BZPIN_DIS()
{
	UINT8 RTN = 0;
	u8HOST_FLAG &= (0xFF^FLAG_SPBZOUT_EN);
	CMD_TX_DATA[0] = CMD_BZPIN_DIS;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_BZPIN_DIS_TX_BYTE;
	u16CMD_RX_BYTE = CMD_BZPIN_DIS_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_SET_VOL(UINT8 SP_VOL)
{
	UINT8 RTN = 0;
	
	CMD_TX_DATA[0] = CMD_SET_VOL_NSP;
	CMD_TX_DATA[1] = SP_VOL;
	CMD_TX_DATA[2] = ((CMD_TX_DATA[0] + CMD_TX_DATA[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_SET_VOL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_SET_VOL_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_GET_VOL(UINT8* SP_VOL)
{
	UINT8 RTN = 0;
	CMD_TX_DATA[0] = CMD_GET_VOL;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_VOL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_VOL_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	if (RTN == 1)
	{
		*SP_VOL = CMD_RX_DATA[0];
	}
	HOST_CMD_INTERVAL_SHORT();
	return RTN;
}
//----------------------------------
//After the host sends STOP cmd,
//it will send READ_STATUS cmd every interval (HOST_CMD_INTERVAL), 
//and the loop will execute until the READ_STATUS cmd gets the correct result, 
//so as to ensure that the NSP STOP operation has been completed.
UINT8 N_STOP()
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	
	CMD_TX_DATA[0] = CMD_STOP;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_STOP_TX_BYTE;
	u16CMD_RX_BYTE = CMD_STOP_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
//After the host sends RESET cmd,
//it will send READ_STATUS cmd every interval (HOST_CMD_INTERVAL), 
//and the loop will execute until the READ_STATUS cmd gets the correct result, 
//so as to ensure that the NSP stop operation has been completed.
UINT8 N_RESET()
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	
	CMD_TX_DATA[0] = CMD_RESET;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_RESET_TX_BYTE;
	u16CMD_RX_BYTE = CMD_RESET_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
UINT8 N_PWR_DOWN()
{
	UINT8 RTN = 0;
	UINT8 n = 0;
	
	CMD_TX_DATA[0] = CMD_PWR_DOWN;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_PWR_DOWN_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PWR_DOWN_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	for (n= 0; n <= 40; n++)
	{
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
UINT8 N_ISP_WRITE_START(UINT32 PROC_ID,UINT32 CHIP_PDID)
{
	UINT8 RTN = 0;
	UINT8 i = 0;
	UINT8 u8NSP_STATUS1 = 0;
	
	CMD_TX_DATA[0] = CMD_ISP_WRITE_START;
	CMD_TX_DATA[1] = (PROC_ID & 0xff000000) >> 24;
	CMD_TX_DATA[2] = (PROC_ID & 0x00ff0000) >> 16;
	CMD_TX_DATA[3] = (CHIP_PDID & 0xff000000) >> 24;
	CMD_TX_DATA[4] = (CHIP_PDID & 0xff0000) >> 16;
	CMD_TX_DATA[5] = (CHIP_PDID & 0xff00) >> 8;
	CMD_TX_DATA[6] = (CHIP_PDID & 0xff) ;
	CMD_TX_DATA[7] = 0;
	for(i = 0; i <(CMD_ISP_WRITE_START_TX_BYTE-1); i++)
		{
			CMD_TX_DATA[7] = CMD_TX_DATA[7] + CMD_TX_DATA[i];
		}
	CMD_TX_DATA[7] = (CMD_TX_DATA[7] ^ 0xff);
	u16CMD_TX_BYTE = CMD_ISP_WRITE_START_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_START_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
UINT8 N_ISP_WRITE_END(void)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	
	CMD_TX_DATA[0] = CMD_ISP_WRITE_END;
	CMD_TX_DATA[1] = CMD_TX_DATA[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_WRITE_END_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_END_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_PROCESS();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
UINT8 N_ISP_WRITE_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	
	
	CMD_TX_DATA[0] = CMD_ISP_WRITE_PAGE;
	CMD_TX_DATA[1] = (ISP_ADDR & 0xFF0000)>>16;
	CMD_TX_DATA[2] = (ISP_ADDR & 0xFF00)>>8;
	CMD_TX_DATA[3] = (ISP_ADDR & 0xFF);
	u16CMD_TX_BYTE = CMD_ISP_WRITE_PAGE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_PAGE_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_ISP_PROCESS(ISP_BUFFER);
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
UINT8 N_ISP_READ_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER)
{
	UINT8 RTN = 0;
	
	CMD_TX_DATA[0] = CMD_ISP_READ_PAGE;
	CMD_TX_DATA[1] = (ISP_ADDR & 0xFF0000)>>16;
	CMD_TX_DATA[2] = (ISP_ADDR & 0xFF00)>>8;
	CMD_TX_DATA[3] = (ISP_ADDR & 0xFF);
	CMD_TX_DATA[4] = ((CMD_TX_DATA[0] + CMD_TX_DATA[1]+ CMD_TX_DATA[2]+ CMD_TX_DATA[3])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_READ_PAGE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_READ_PAGE_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_ISP_PROCESS(ISP_BUFFER);
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_MULTI_PLAY(UINT8 PlayListNum,PUINT8 DATA_BUFFER)
{
	UINT8 RTN = 0;
	
	CMD_TX_DATA[0] = CMD_MULTI_PLAY;
	CMD_TX_DATA[1] = PlayListNum;
	u16CMD_TX_BYTE = CMD_MULTI_PLAY_TX_BYTE+PlayListNum-1;
	u16CMD_RX_BYTE = CMD_MULTI_PLAY_RX_BYTE;
	
	RTN = I2C_LIKE_TX_RX_BUFFER_PROCESS(DATA_BUFFER);
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
void N_WAKUP(void)
{
	GPIO2_SDA_OUTPUT0();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	GPIO2_SDA_OUTPUT1();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
}


