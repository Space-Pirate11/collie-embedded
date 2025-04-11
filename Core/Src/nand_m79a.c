#include "nand_m79a.h"
#include <string.h>

/**
    @brief __map_logical_addr: Converts a 32-bit logical address to a physical NAND address.
    @param address: Pointer to the logical address.
    @param addr_struct: Pointer to the PhysicalAddrs structure that is filled out.
    @return Ret_Success.
*/
NAND_ReturnType __map_logical_addr(NAND_Addr *address, PhysicalAddrs *addr_struct) {
    addr_struct->plane   = ADDRESS_2_PLANE(*address);
    addr_struct->block   = ADDRESS_2_BLOCK(*address);
    addr_struct->page    = ADDRESS_2_PAGE(*address);
    addr_struct->rowAddr = ((ADDRESS_2_BLOCK(*address) << ROW_ADDRESS_PAGE_BITS) | ADDRESS_2_PAGE(*address));
    addr_struct->colAddr = ADDRESS_2_COL(*address);
    return Ret_Success;
}

/**
    @brief NAND_Init: Initializes the NAND flash device.
    @note  Resets the device and verifies the device ID.
*/
NAND_HL_ReturnType NAND_Init(SPI_HandleTypeDef *hspi) {
    NAND_ID dev_ID;
    NAND_Wait(T_POR);  // Wait for power-on reset period.
    if (NAND_Reset(hspi) != Ret_Success) {
        return Ret_ResetFailed;
    }
    NAND_Read_ID(hspi, &dev_ID);
    if (dev_ID.manufacturer_ID != NAND_ID_MANUFACTURER || dev_ID.device_ID != NAND_ID_DEVICE) {
        return Ret_WrongID;
    }
    return Ret_Success;
}

/**
    @brief NAND_Write: Writes a buffer to NAND flash at the specified logical address.
    @note  This simple implementation assumes the write fits within one page and does not span pages.
*/
NAND_HL_ReturnType NAND_Write(NAND_Addr start_addr, const uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phys_addr;
    __map_logical_addr(&start_addr, &phys_addr);
    // For simplicity, we assume length does not exceed PAGE_DATA_SIZE.
    if (NAND_Page_Program(hspi, &phys_addr, (uint8_t*)buffer, length) != Ret_Success) {
        return Ret_ProgramFailed;
    }
    return Ret_Success;
}

/**
    @brief NAND_Read: Reads data from NAND flash starting at the given logical address.
    @note  This function maps the logical address and then reads the data.
*/
NAND_HL_ReturnType NAND_Read(NAND_Addr start_addr, uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phys_addr;
    __map_logical_addr(&start_addr, &phys_addr);
    if (NAND_Page_Read(hspi, &phys_addr, buffer, length) != Ret_Success) {
        return Ret_ReadFailed;
    }
    return Ret_Success;
}

/**
    @brief NAND_Erase_Block: Erases a block given its block index.
    @note  The block index is the physical block number (0 to NUM_BLOCKS-1).
*/
NAND_HL_ReturnType NAND_Erase_Block(uint32_t block_index, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phys_addr;
    // Set the block and set page and column to 0.
    phys_addr.block = block_index;
    phys_addr.page = 0;
    phys_addr.rowAddr = (block_index << ROW_ADDRESS_PAGE_BITS) | 0;
    phys_addr.colAddr = 0;
    if (NAND_Block_Erase(hspi, &phys_addr) != Ret_Success) {
        return Ret_EraseFailed;
    }
    return Ret_Success;
}
