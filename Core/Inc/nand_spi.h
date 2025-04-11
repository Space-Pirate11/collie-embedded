/************************** NAND SPI Functions ***********************************
   Filename:    nand_spi.h
   Description: Implements SPI wrapper functions for use by low-level NAND flash drivers.
                This layer uses the STM32 HAL (stm32u3xx_hal.h) to perform SPI transactions.
   Version:     0.1
   Author:      Tharun Suresh
********************************************************************************
    Version History.
    Ver.    Date            Comments
    0.1     Jan 2022        In Development
********************************************************************************/

#ifndef INC_NAND_SPI_H_
#define INC_NAND_SPI_H_

#include "stm32u3xx_hal.h"  // Updated HAL include for STM32U3xx

/* Definition of NAND flash SPI pins and ports */
#define NAND_NCS_PIN    GPIO_PIN_12
#define NAND_SCK_PIN    GPIO_PIN_13
#define NAND_MISO_PIN   GPIO_PIN_14
#define NAND_MOSI_PIN   GPIO_PIN_15

#define NAND_MOSI_PORT  GPIOB
#define NAND_MISO_PORT  GPIOB
#define NAND_SCK_PORT   GPIOB
#define NAND_NCS_PORT   GPIOB

/* Dummy byte used during SPI read operations */
#define DUMMY_BYTE         0x00
/* Timeout for SPI transactions (in ms) */
#define NAND_SPI_TIMEOUT   100

/* Return type for SPI wrapper functions */
typedef enum {
    SPI_OK,
    SPI_Fail
} NAND_SPI_ReturnType;

/* Structure for SPI transaction parameters */
typedef struct {
    uint8_t *buffer;  // Pointer to the data buffer
    uint16_t length;  // Number of bytes in the transaction
} SPI_Params;

/******************************************************************************
 *                              Internal Functions
 *****************************************************************************/
void __nand_spi_cs_low(void);
void __nand_spi_cs_high(void);

/******************************************************************************
 *                                  List of APIs
 *****************************************************************************/
/* Wait function for delays (wrapper around HAL_Delay) */
void NAND_Wait(uint8_t milliseconds);

/* SPI transaction wrappers for the NAND driver */
NAND_SPI_ReturnType NAND_SPI_Send(SPI_HandleTypeDef *hspi, SPI_Params *data_send);
NAND_SPI_ReturnType NAND_SPI_SendReceive(SPI_HandleTypeDef *hspi, SPI_Params *data_send, SPI_Params *data_recv);
NAND_SPI_ReturnType NAND_SPI_Receive(SPI_HandleTypeDef *hspi, SPI_Params *data_recv);
NAND_SPI_ReturnType NAND_SPI_Send_Command_Data(SPI_HandleTypeDef *hspi, SPI_Params *cmd_send, SPI_Params *data_send);

#endif /* INC_NAND_SPI_H_ */
