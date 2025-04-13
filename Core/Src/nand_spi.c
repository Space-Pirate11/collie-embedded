/* Collie_V0/Core/Src/nand_spi.c */
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
static void __nand_spi_cs_low(void) { // Made static as it's internal
    HAL_GPIO_WritePin(NAND_NCS_PORT, NAND_NCS_PIN, GPIO_PIN_RESET);
}

/*
 * Internal helper function to pull the NAND chip-select high.
 * This ends an SPI transaction.
 */
static void __nand_spi_cs_high(void) { // Made static as it's internal
    HAL_GPIO_WritePin(NAND_NCS_PORT, NAND_NCS_PIN, GPIO_PIN_SET);
}

/*
 * NAND_SPI_Select: Pulls the NAND chip-select low. Public wrapper.
 */
void NAND_SPI_Select(void) {
    __nand_spi_cs_low();
}

/*
 * NAND_SPI_Deselect: Pulls the NAND chip-select high. Public wrapper.
 */
void NAND_SPI_Deselect(void) {
    __nand_spi_cs_high();
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
    // CS low/high handled by NAND_SPI_Select/Deselect in caller (nand_m79a_lld.c)
    send_status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    return (send_status == HAL_OK) ? SPI_OK : SPI_Fail;
}

/*
 * NAND_SPI_SendReceive: Transmits data and then receives data in the same SPI session.
 * Assumes CS is already low and will be pulled high by caller.
 */
NAND_SPI_ReturnType NAND_SPI_SendReceive(SPI_HandleTypeDef *hspi, SPI_Params *data_send, SPI_Params *data_recv) {
    HAL_StatusTypeDef status = HAL_OK;

    // Transmit command/address first
    if (data_send != NULL && data_send->length > 0) {
        status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    }

    // Then receive data
    if (status == HAL_OK && data_recv != NULL && data_recv->length > 0) {
        status = HAL_SPI_Receive(hspi, data_recv->buffer, data_recv->length, NAND_SPI_TIMEOUT);
    }

    return (status == HAL_OK) ? SPI_OK : SPI_Fail;
}


/*
 * NAND_SPI_Receive: Receives data over SPI.
 * Assumes CS is already low and will be pulled high by caller.
 */
NAND_SPI_ReturnType NAND_SPI_Receive(SPI_HandleTypeDef *hspi, SPI_Params *data_recv) {
    HAL_StatusTypeDef receive_status;
    // CS low/high handled by caller
    receive_status = HAL_SPI_Receive(hspi, data_recv->buffer, data_recv->length, NAND_SPI_TIMEOUT);
    return (receive_status == HAL_OK) ? SPI_OK : SPI_Fail;
}

/*
 * NAND_SPI_Send_Command_Data: Sends a command then additional data within one transaction.
 * Assumes CS is already low and will be pulled high by caller.
 */
NAND_SPI_ReturnType NAND_SPI_Send_Command_Data(SPI_HandleTypeDef *hspi, SPI_Params *cmd_send, SPI_Params *data_send) {
    HAL_StatusTypeDef status = HAL_OK;

    // Send command part
    if (cmd_send != NULL && cmd_send->length > 0) {
        status = HAL_SPI_Transmit(hspi, cmd_send->buffer, cmd_send->length, NAND_SPI_TIMEOUT);
    }

    // Send data part
    if (status == HAL_OK && data_send != NULL && data_send->length > 0) {
        status = HAL_SPI_Transmit(hspi, data_send->buffer, data_send->length, NAND_SPI_TIMEOUT);
    }

    return (status == HAL_OK) ? SPI_OK : SPI_Fail;
}
