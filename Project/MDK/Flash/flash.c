#include "flash.h"

uint8_t flash_rx_buffer[FLASH_REC_SIZE];
extern void Delay(uint16_t count); 

void spi1_init(void)
{
    GPIO_Config_T gpioConfig;
    SPI_Config_T spiConfig;
     
    /** Enable related clock*/
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SPI1);
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_GPIOA);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SYSCFG);
    
     /** Config alter function*/  
    GPIO_ConfigPinAF(GPIOA, GPIO_PIN_SOURCE_5, GPIO_AF_PIN0);
    GPIO_ConfigPinAF(GPIOA, GPIO_PIN_SOURCE_6, GPIO_AF_PIN0);
    GPIO_ConfigPinAF(GPIOA, GPIO_PIN_SOURCE_7, GPIO_AF_PIN0);
    
    /** config PIN_5->SCK , PIN_7->MOSI*/ 
    gpioConfig.pin =  GPIO_PIN_5 | GPIO_PIN_7;    
    gpioConfig.mode = GPIO_MODE_AF;
    gpioConfig.outtype = GPIO_OUT_TYPE_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pupd = GPIO_PUPD_PU;
    GPIO_Config(GPIOA, &gpioConfig);
    
     /** config PIN_4->NSS*/
    gpioConfig.pin =  GPIO_PIN_4;     
    gpioConfig.outtype = GPIO_OUT_TYPE_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pupd = GPIO_PUPD_PU;
    gpioConfig.mode = GPIO_MODE_OUT;
    GPIO_Config(GPIOA, &gpioConfig);
    GPIO_SetBit(GPIOA, GPIO_PIN_4);
    
    /** config PIN_6  MISO*/
    gpioConfig.pin = GPIO_PIN_6;     
    gpioConfig.mode = GPIO_MODE_AF;
    gpioConfig.pupd = GPIO_PUPD_PU;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &gpioConfig);
    
    /** SPI RESET*/
    SPI_Reset(SPI1);
    SPI_ConfigStructInit(&spiConfig);

    /** SPI configuration*/
    /** Set Clock polarity is Low, but Slave is High*/
    spiConfig.polarity = SPI_CLKPOL_LOW;
    
    /** select master mode*/
    spiConfig.mode = SPI_MODE_MASTER;
    
    /** SPI Clock Phase is 1EDGE, but Slave is 1EDGE*/
    spiConfig.phase = SPI_CLKPHA_1EDGE;
    
    /** Enable Software slave control */
    spiConfig.slaveSelect = SPI_SSC_ENABLE;
    
    /** Set SPI BaudRate divider*/
    spiConfig.baudrateDiv = SPI_BAUDRATE_DIV_64;
    
    /** SPI data length*/
    spiConfig.length = SPI_DATA_LENGTH_8B;
    
    /** Set internal slave*/
    SPI_EnableInternalSlave(SPI1);
    SPI_Config(SPI1, &spiConfig);
    SPI_ConfigFIFOThreshold(SPI1, SPI_RXFIFO_QUARTER);

    SPI_Enable(SPI1);
}

void Flash_WriteEnable(void)
{
	CS_L;
	Flash_Sendbyte(0x06);
	CS_H;
}

u8 Flash_Sendbyte(u8 byte)
{
	/*!< Loop while DR register in not emplty */
	while (SPI_ReadStatusFlag(SPI1, SPI_FLAG_TXBE) == RESET); 

	/*!< Send byte through the SPI1 peripheral */
	SPI_TxData8(SPI1, byte);

	/*!< Wait to receive a byte */
	while (SPI_ReadStatusFlag(SPI1, SPI_FLAG_RXBNE) == RESET); 

	/*!< Return the byte read from the SPI bus */
	return SPI_RxData8(SPI1);
}

u8 Flash_ReadStatus(void)
{
	u8 temp;
	CS_L ;
	temp = Flash_Sendbyte(0x05);  //读Flash状态寄存器
	temp = Flash_Sendbyte(0xFF); 
	CS_H;
	return temp;
}


/* NORMAL READ OPERATION (NORD, 03h) */
u8 Flash_NORD(u32 addr, u32 size,u8 *destaddr)
{
	u32 i;
	
	CS_L;
	Flash_Sendbyte(0x03);
	Flash_Sendbyte((u8)((addr&0x00FF0000)>>16));	//A[23:16]
	Flash_Sendbyte((u8)((addr&0x0000FF00)>>8));	//A[15:8]
	Flash_Sendbyte((u8)(addr&0x000000FF));		//A[7:0]
	for(i=0;i<size;i++)
	{
		*destaddr = Flash_Sendbyte(0xFF);
		destaddr++;
	}
	CS_H;
	return 0;
}


/* PAGE PROGRAM OPERATION (PP, 02h)  一次最多256字节*/
u8 Flash_PageProgram(u32 addr,u32 size,u8 *source)
{
	u32 i;
	
	Flash_WriteEnable();
	
	CS_L;
	Flash_Sendbyte(0x02);
	Flash_Sendbyte((u8)((addr&0x00FF0000)>>16));	//A[23:16]
	Flash_Sendbyte((u8)((addr&0x0000FF00)>>8));	//A[15:8]
	Flash_Sendbyte((u8)(addr&0x000000FF));		//A[7:0]
	for(i=0;i<size;i++)
	{
		Flash_Sendbyte(*source);
		source++;
	}
	CS_H;
	
	Flash_WaitForWipEnd();
	return 0;
}

/* 不定长SPI Flash数据写入 */
u8 Flash_Program(u32 addr,u32 size,void *source)
{
	u8 count = 0, NumofPage = 0, NumofSingle, Addr = 0, temp = 0;
	u8 *Source = (u8*)source; //通过void指针，传入任意类型数据。在函数里需要把指针类型定义成具体类型
	Addr =  addr % FLASH_SPI_PAGESIZE;
	NumofPage = size / FLASH_SPI_PAGESIZE;
	NumofSingle = size % FLASH_SPI_PAGESIZE;
	count = FLASH_SPI_PAGESIZE - Addr;
	
	if(Addr==0)		/*WriteAddr is Flash_PAGESIZE aligned*/
	{
		if(NumofPage==0)	/* Numofpage < Flash_PAGESIZE*/
		{
			Flash_PageProgram(addr,size,Source);
		}
		else	/* Numofpage > Flash_PAGESIZE*/
		{
			while(NumofPage--)
			{
				Flash_PageProgram(addr,FLASH_SPI_PAGESIZE,Source);
				addr += FLASH_SPI_PAGESIZE;
				Source += FLASH_SPI_PAGESIZE;
			}
			Flash_PageProgram(addr,NumofSingle,Source);
		}
	}
	else	/*WriteAddr is not Flash_PAGESIZE aligned*/
	{
		if(NumofPage==0)
		{
			if(NumofSingle > count) /*(Size + addr) > Flash_PAGESIZE, page buffer 越界*/
			{
				temp = NumofSingle - count;
				
				Flash_PageProgram(addr,count,Source);
				addr += count;
				Source += count;
				
				Flash_PageProgram(addr,temp,Source);
			}
			else
			{
				Flash_PageProgram(addr,size,Source);
			}
		}
		else /* Size > Flash_PAGESIZE */
		{
			size -= count;
			NumofPage = size / FLASH_SPI_PAGESIZE;
			NumofSingle = size % FLASH_SPI_PAGESIZE;
			
			Flash_PageProgram(addr,count,Source);
			addr += count;
			Source += count;
			
			while(NumofPage--)
			{
				Flash_PageProgram(addr,FLASH_SPI_PAGESIZE,Source);
				addr += FLASH_SPI_PAGESIZE;
				Source += FLASH_SPI_PAGESIZE;
			}
			if(NumofSingle != 0)
			{
				Flash_PageProgram(addr,NumofSingle,Source);
			}
		}
	}
	return 0;
}

u8 Flash_SetReadPara(u8 para)
{
	CS_L;
	Flash_Sendbyte(0xC0);  //Set Read Parameters (Volatile)
	Flash_Sendbyte(para);
	CS_H;
	return 0;
}

u8 Flash_SectorErase(u32 addr)
{
	CS_L;
	Flash_Sendbyte(0xD7);
	Flash_Sendbyte((u8)((addr&0x00FF0000)>>16));	//A[23:16]
	Flash_Sendbyte((u8)((addr&0x0000FF00)>>8));	//A[15:8]
	Flash_Sendbyte((u8)(addr&0x000000FF));		//A[7:0]
	CS_H;
	
	Flash_WaitForWipEnd();
	return 0;
}

u8 Flash_BlockErase(u32 addr,BLOCK_SIZE_T block_size)
{
	CS_L;
	
	if(block_size==BLOCK_32K)
	{
		Flash_Sendbyte(0x52);
	}
	else if(block_size==BLOCK_64K)
	{
		Flash_Sendbyte(0xD8);
	}
	Flash_Sendbyte((u8)((addr&0x00FF0000)>>16));	//A[23:16]
	Flash_Sendbyte((u8)((addr&0x0000FF00)>>8));	//A[15:8]
	Flash_Sendbyte((u8)(addr&0x000000FF));		//A[7:0]
	
	CS_H;
	
	Flash_WaitForWipEnd();
	return 0;
}

u8 Flash_ReadFunction(void)
{
	u8 temp;
	CS_L;
	temp = Flash_Sendbyte(0x48);  //Read Function Register
	temp = Flash_Sendbyte(0xFF); 
	CS_H;
	return temp;
}

void Flash_WaitForWipEnd(void)
{
	u8 temp;
	CS_L;
	temp = Flash_Sendbyte(0x05);  //读Flash状态寄存器
	temp = Flash_Sendbyte(0xFF);
	while((temp&0x01)==1)
	{
		temp = Flash_Sendbyte(0xFF);
	}
	CS_H;
}

void run_flash_operate(uint8_t rw_cmd, uint8_t *dst_addr, uint8_t *data)
{
	uint8_t i = 0;
	uint32_t ADDR = 0;
	
	ADDR = ((*dst_addr)<<16) | ((*(dst_addr+1))<<8) | (*(dst_addr+2));
	
	if(rw_cmd==SPI_READ)
	{
		Flash_NORD(ADDR, (*(dst_addr+3)), flash_rx_buffer);
		for(i=0;i<(*(dst_addr+3));i++)
		{
			if(i%16==0)
			{
				printf("\r\n");
			}
			printf("0x%02x  ",flash_rx_buffer[i]);
		}
		printf("\r\n");
//		APM_MINI_LEDToggle(LED3);
	}
	if(rw_cmd==SPI_WRITE)
	{
		Flash_WriteEnable();
		Flash_SectorErase(ADDR);
		Flash_Program(ADDR,(*(dst_addr+3)),dst_addr+4);
//		APM_MINI_LEDToggle(LED3);
	}
}



