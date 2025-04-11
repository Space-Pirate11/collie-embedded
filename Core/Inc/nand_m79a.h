#ifndef INC_NAND_M79A_H_
#define INC_NAND_M79A_H_

#include "nand_m79a_lld.h"

/* Define a logical address type for the high-level interface */
typedef uint32_t NAND_Addr;

/* High-level NAND flash interface return type (same as low-level) */
typedef NAND_ReturnType NAND_HL_ReturnType;

/******************************************************************************
 *                              Internal Function Prototypes
 *****************************************************************************/
/* This function maps a logical address to a physical address structure */
NAND_ReturnType __map_logical_addr(NAND_Addr *address, PhysicalAddrs *addr_struct);

/******************************************************************************
 *                              List of APIs
 *****************************************************************************/

/* NAND_Init: Initializes the NAND flash by resetting the device and verifying the device ID */
NAND_HL_ReturnType NAND_Init(SPI_HandleTypeDef *hspi);

/* NAND_Write: High-level API to write data to NAND flash at the given logical address.
   (Note: In this simple implementation, the write is assumed to fit within one page.)
*/
NAND_HL_ReturnType NAND_Write(NAND_Addr start_addr, const uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi);

/* NAND_Read: High-level API to read data from NAND flash from the given logical address.
   (This function maps the logical address to the physical address and calls the low-level page read.)
*/
NAND_HL_ReturnType NAND_Read(NAND_Addr start_addr, uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi);

/* NAND_Erase_Block: High-level API to erase one block in NAND flash.
   This function erases the block that contains the given physical address.
*/
NAND_HL_ReturnType NAND_Erase_Block(uint32_t block_index, SPI_HandleTypeDef *hspi);

#endif /* INC_NAND_M79A_H_ */
