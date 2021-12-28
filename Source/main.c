/*!
 * @file        main.c
 *
 * @brief       Main program body          
 *
 * @version     V1.0.1
 *
 * @date        2021-07-01
 */

#include "Board.h"
#include "stdio.h"
#include "apm32f0xx_gpio.h"
#include "apm32f0xx_misc.h"
#include "apm32f0xx_eint.h"
#include "apm32f0xx_spi.h"
#include "flash.h"

/** printf function configs to USART2*/
#define DEBUG_USART  USART2

/** Buffsize*/
#define BuffSize 56
/** SPI TX Buffer*/
uint8_t SPI_Buffer_TX[BuffSize] = 
{
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
    0x01,0x02,0x03,0x04,0x05,0x06,0X07,
};

/* Delay */
void Delay(uint16_t count); 
/* SPI Init */


/*!
 * @brief       Main program
 *
 * @param       None
 *
 * @retval      None
 *
 * @note       
 */
int main(void)
{
    uint16_t i;
    /** SPI Receive Buffer*/
    uint8_t SPI_Buffer_RX[BuffSize] = {0x00};
    
    APM_MINI_LEDInit(LED2);
    APM_MINI_LEDInit(LED3);
    APM_MINI_COMInit(COM2);
	spi1_init();

    printf("I am Master\r\n");

    Flash_WriteEnable();
    Flash_SetReadPara(0XE3);

	Flash_BlockErase(0x400,BLOCK_32K);
	Flash_NORD(0x400,sizeof(SPI_Buffer_RX),SPI_Buffer_RX);

    for(i=0;i<BuffSize;i++)
	{
		if(i%8==0)
		{
			printf("\r\n");
		}
		printf("0x%02x  ",SPI_Buffer_RX[i]);
	}
	printf("\r\n");

	Flash_Program(0x400, sizeof(SPI_Buffer_TX),SPI_Buffer_TX);
    Flash_NORD(0x400,sizeof(SPI_Buffer_RX),SPI_Buffer_RX);

    for(i=0;i<BuffSize;i++)
	{
		if(i%8==0)
		{
			printf("\r\n");
		}
		printf("0x%02x  ",SPI_Buffer_RX[i]);
	}
	printf("\r\n");
   
    for(;;)
    {
       Delay(0xffff);
       APM_MINI_LEDToggle(LED3);
    }
}

/*!
 * @brief       SPI Init
 *
 * @param       None
 *
 * @retval      None
 *
 * @note       
 */

/*!
 * @brief       Delay
 *
 * @param       None
 *
 * @retval      None
 *
 * @note       
 */
void Delay(uint16_t count)
{
    volatile uint32_t delay = count;
    
    while(delay--);
}
 /*!
 * @brief       Redirect C Library function printf to serial port.
 *              After Redirection, you can use printf function.
 *
 * @param       ch:  The characters that need to be send.
 *
 * @param       *f:  pointer to a FILE that can recording all information 
 *              needed to control a stream
 *     
 * @retval      The characters that need to be send.
 *
 * @note
 */
int fputc(int ch, FILE *f)
{
        /** send a byte of data to the serial port */
        USART_TxData(DEBUG_USART,(uint8_t)ch);
        
        /** wait for the data to be send  */
        while (USART_ReadStatusFlag(DEBUG_USART, USART_FLAG_TXBE) == RESET);        
       
        return (ch);
}








