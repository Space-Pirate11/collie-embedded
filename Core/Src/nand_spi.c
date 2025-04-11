/************************** NAND SPI Functions ***********************************
   Filename:    nand_spi.c
   Description: Implements SPI wrapper functions for the NAND flash driver using STM32 HAL.
                These functions perform complete SPI transactions (transmit, receive, command+data).
   Version:     0.1
   Author:      Tharun Suresh
********************************************************************************/

#include "nand_spi.h"

/*
 * Internal helper function to pull the NAND chip-select low.
 * This starts an SPI transaction.
 */
void __nand_spi_cs_low(void) {
    HAL_GPIO_WritePin(NAND_NCS_PORT, NAND_NCS_PIN, GPIO_PIN_RESET);
}

/*
 * Internal helper function to pull the NAND chip-select high.
 * This ends an SPI transaction.
 */
void __nand_spi_cs_high(void) {
    HAL_GPIO_WritePin(NAND_NCS_PORT, NAND_NCS_PIN, GPIO_PIN_SET);
}

/*
 * NAND_Wait: A simple wrapper for HAL_Delay() to wait a specified number of milliseconds.
 */
void NAND_Wait(uint8_t milliseconds) {
    HAL_Delay(milliseconds);
}

/*
 * NAND_SPI_Send: Transmits a given buffer over SPI.
 */
NAND_SPI_ReturnType NAND_SPI_Send(SPI_HandleTypeDef *hspi, SPI_Params *data_send) {
    HAL_StatusTypeDef send_status;
    __nand_spi_cs_low();
    send_status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (send_status == HAL_OK) ? SPI_OK : SPI_Fail;
}

/*
 * NAND_SPI_SendReceive: Transmits data and then receives data in the same SPI session.
 */
NAND_SPI_ReturnType NAND_SPI_SendReceive(SPI_HandleTypeDef *hspi, SPI_Params *data_send, SPI_Params *data_recv) {
    HAL_StatusTypeDef transmit_status;
    __nand_spi_cs_low();
    HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    transmit_status = HAL_SPI_Receive(hspi, data_recv->buffer, data_recv->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (transmit_status == HAL_OK) ? SPI_OK : SPI_Fail;
}

/*
 * NAND_SPI_Receive: Receives data over SPI.
 */
NAND_SPI_ReturnType NAND_SPI_Receive(SPI_HandleTypeDef *hspi, SPI_Params *data_recv) {
    HAL_StatusTypeDef receive_status;
    __nand_spi_cs_low();
    receive_status = HAL_SPI_Receive(hspi, data_recv->buffer, data_recv->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (receive_status == HAL_OK) ? SPI_OK : SPI_Fail;
}

/*
 * NAND_SPI_Send_Command_Data: Sends a command then additional data within one transaction.
 */
NAND_SPI_ReturnType NAND_SPI_Send_Command_Data(SPI_HandleTypeDef *hspi, SPI_Params *cmd_send, SPI_Params *data_send) {
    HAL_StatusTypeDef send_status;
    __nand_spi_cs_low();
    HAL_SPI_Transmit(hspi, cmd_send->buffer, cmd_send->length, NAND_SPI_TIMEOUT);
    send_status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    __nand_spi_cs_high();
    return (send_status == HAL_OK) ? SPI_OK : SPI_Fail;
}
