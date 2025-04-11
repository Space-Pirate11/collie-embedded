/*
 * nand_m79a.h
 *
 *  Created on: Apr 11, 2025
 *      Author: amoltandon
 */

#ifndef INC_NAND_M79A_H_
#define INC_NAND_M79A_H_

#include "nand_m79a_lld.h"

/* Logical address type */
typedef uint32_t NAND_Addr;

/* High-level API return type definition */
typedef NAND_ReturnType NAND_HL_ReturnType;

/* Function prototypes */
NAND_HL_ReturnType NAND_Init_HighLevel(SPI_HandleTypeDef *hspi);
NAND_HL_ReturnType NAND_Write(NAND_Addr start_addr, const uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi);
NAND_HL_ReturnType NAND_Read(NAND_Addr start_addr, uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi);
NAND_HL_ReturnType NAND_Erase_Block(uint32_t block_index, SPI_HandleTypeDef *hspi);


#endif /* INC_NAND_M79A_H_ */
