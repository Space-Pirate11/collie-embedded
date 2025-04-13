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
#include <stdint.h>         // Include for uint types

/* Definition of NAND flash SPI pins and ports (Ensure these match your hardware) */
/* Example pins, adjust as necessary */
#define NAND_NCS_PIN    GPIO_PIN_4  // Example: Using PA4 as CS
#define NAND_NCS_PORT   GPIOA       // Example: Using GPIOA

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
 * Internal Functions
 *****************************************************************************/
/* These are likely implemented in nand_spi.c */
// void __nand_spi_cs_low(void);  // Make static if only used in nand_spi.c
// void __nand_spi_cs_high(void); // Make static if only used in nand_spi.c

/******************************************************************************
 * List of APIs
 *****************************************************************************/
/* Wait function for delays (wrapper around HAL_Delay) */
void NAND_Wait(uint8_t milliseconds);

/* SPI Chip Select Control Functions (Prototypes added) */
void NAND_SPI_Select(void);   // Added Prototype
void NAND_SPI_Deselect(void); // Added Prototype

/* SPI transaction wrappers for the NAND driver */
NAND_SPI_ReturnType NAND_SPI_Send(SPI_HandleTypeDef *hspi, SPI_Params *data_send);
NAND_SPI_ReturnType NAND_SPI_SendReceive(SPI_HandleTypeDef *hspi, SPI_Params *data_send, SPI_Params *data_recv);
NAND_SPI_ReturnType NAND_SPI_Receive(SPI_HandleTypeDef *hspi, SPI_Params *data_recv);
NAND_SPI_ReturnType NAND_SPI_Send_Command_Data(SPI_HandleTypeDef *hspi, SPI_Params *cmd_send, SPI_Params *data_send);

#endif /* INC_NAND_SPI_H_ */
