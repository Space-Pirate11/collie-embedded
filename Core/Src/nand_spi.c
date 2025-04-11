/*
 * nand_spi.c
 *
 *  Created on: Apr 11, 2025
 *      Author: amoltandon
 */


#include "nand_spi.h"

void __nand_spi_cs_low(void) {
    HAL_GPIO_WritePin(NAND_NCS_PORT, NAND_NCS_PIN, GPIO_PIN_RESET);
}

void __nand_spi_cs_high(void) {
    HAL_GPIO_WritePin(NAND_NCS_PORT, NAND_NCS_PIN, GPIO_PIN_SET);
}

void NAND_Wait(uint8_t milliseconds){
    HAL_Delay(milliseconds);
}

NAND_SPI_ReturnType NAND_SPI_Send(SPI_HandleTypeDef *hspi, SPI_Params *data_send) {
    HAL_StatusTypeDef status;
    __nand_spi_cs_low();
    status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (status == HAL_OK) ? SPI_OK : SPI_Fail;
}

NAND_SPI_ReturnType NAND_SPI_SendReceive(SPI_HandleTypeDef *hspi, SPI_Params *data_send, SPI_Params *data_recv) {
    HAL_StatusTypeDef status;
    __nand_spi_cs_low();
    HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    status = HAL_SPI_Receive(hspi, data_recv->buffer, data_recv->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (status == HAL_OK) ? SPI_OK : SPI_Fail;
}

NAND_SPI_ReturnType NAND_SPI_Receive(SPI_HandleTypeDef *hspi, SPI_Params *data_recv) {
    HAL_StatusTypeDef status;
    __nand_spi_cs_low();
    status = HAL_SPI_Receive(hspi, data_recv->buffer, data_recv->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (status == HAL_OK) ? SPI_OK : SPI_Fail;
}

NAND_SPI_ReturnType NAND_SPI_Send_Command_Data(SPI_HandleTypeDef *hspi, SPI_Params *cmd_send, SPI_Params *data_send) {
    HAL_StatusTypeDef status;
    __nand_spi_cs_low();
    status = HAL_SPI_Transmit(hspi, cmd_send->buffer, cmd_send->length, NAND_SPI_TIMEOUT);
    if(status == HAL_OK)
        status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (status == HAL_OK) ? SPI_OK : SPI_Fail;
}
