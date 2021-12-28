#ifndef __FLASH_H
#define __FLASH_H

#endif

#include "apm32f0xx_gpio.h"
#include "apm32f0xx_misc.h"
#include "apm32f0xx_eint.h"
#include "apm32f0xx_spi.h"
#include "apm32f0xx_rcm.h"
#include "stdio.h"

#ifdef _SELF_DEFINE
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#endif

#if 1
#define CS_L	GPIOA->ODATA_B.ODATA4 = BIT_RESET
#define CS_H	GPIOA->ODATA_B.ODATA4 = BIT_SET
#else
#define CS_L	GPIOD->ODATA_B.ODATA2 = BIT_RESET
#define CS_H	GPIOD->ODATA_B.ODATA2 = BIT_SET
#endif

#define FLASH_SPI_PAGESIZE		0x100
#define SPI_WRITE	0x01
#define SPI_READ	0X05
#define FLASH_REC_SIZE 256

typedef enum
{
	BLOCK_32K		= 0,    ///Block Erase 32K
	BLOCK_64K		= 1     ///Block Erase 64K
} BLOCK_SIZE_T;


void spi1_init(void);
void Flash_WriteEnable(void);
void Flash_WaitForWipEnd(void);
u8 Flash_Sendbyte(u8 byte);
u8 Flash_ReadStatus(void);
u8 Flash_NORD(u32 addr,u32 size,u8 *destaddr);
u8 Flash_PageProgram(u32 addr,u32 size,u8 *source);
u8 Flash_Program(u32 addr,u32 size,void *source); //不定长SPI Flash写入
u8 Flash_SetReadPara(u8 para);
u8 Flash_SectorErase(u32 addr);
u8 Flash_BlockErase(u32 addr,BLOCK_SIZE_T block_size);
u8 Flash_ReadFunction(void);
void run_flash_operate(uint8_t rw_cmd, uint8_t *dst_addr, uint8_t *data);




