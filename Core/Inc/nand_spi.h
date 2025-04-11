/*
 * nand_spi.h
 *
 *  Created on: Apr 11, 2025
 *      Author: amoltandon
 */

#ifndef INC_NAND_SPI_H_
#define INC_NAND_SPI_H_

#include "stm32u3xx_hal.h"

#define NAND_NCS_PIN    GPIO_PIN_12
#define NAND_SCK_PIN    GPIO_PIN_13
#define NAND_MISO_PIN   GPIO_PIN_14
#define NAND_MOSI_PIN   GPIO_PIN_15

#define NAND_MOSI_PORT  GPIOB
#define NAND_MISO_PORT  GPIOB
#define NAND_SCK_PORT   GPIOB
#define NAND_NCS_PORT   GPIOB

#define DUMMY_BYTE         0x00
#define NAND_SPI_TIMEOUT   100

typedef enum {
    SPI_OK,
    SPI_Fail
} NAND_SPI_ReturnType;

typedef struct {
    uint8_t *buffer;
    uint16_t length;
} SPI_Params;

void __nand_spi_cs_low(void);
void __nand_spi_cs_high(void);

void NAND_Wait(uint8_t milliseconds);
NAND_SPI_ReturnType NAND_SPI_Send(SPI_HandleTypeDef *hspi, SPI_Params *data_send);
NAND_SPI_ReturnType NAND_SPI_SendReceive(SPI_HandleTypeDef *hspi, SPI_Params *data_send, SPI_Params *data_recv);
NAND_SPI_ReturnType NAND_SPI_Receive(SPI_HandleTypeDef *hspi, SPI_Params *data_recv);
NAND_SPI_ReturnType NAND_SPI_Send_Command_Data(SPI_HandleTypeDef *hspi, SPI_Params *cmd_send, SPI_Params *data_send);


#endif /* INC_NAND_SPI_H_ */
