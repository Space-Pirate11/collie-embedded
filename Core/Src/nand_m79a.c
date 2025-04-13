#include "nand_m79a.h"
#include "nand_m79a_lld.h" // Include LLD for definitions like PAGE_DATA_SIZE etc.
#include <string.h>

// Define mapping macros locally using definitions from LLD header
#define ADDRESS_2_BLOCK(Address)    ((uint16_t)((Address) / (PAGE_DATA_SIZE * NUM_PAGES_PER_BLOCK)))
#define ADDRESS_2_PAGE(Address)     ((uint16_t)(((Address) % (PAGE_DATA_SIZE * NUM_PAGES_PER_BLOCK)) / PAGE_DATA_SIZE))
#define ADDRESS_2_COL(Address)      ((uint16_t)((Address) % PAGE_DATA_SIZE))

/**
    @brief __map_logical_addr: Converts a 32-bit logical address to a physical NAND address.
    @note  Logical address refers to the byte offset within the main data area (ignoring spare).
    @param address: Pointer to the logical address.
    @param addr_struct: Pointer to the PhysicalAddrs structure that is filled out.
    @return Ret_Success.
*/
NAND_ReturnType __map_logical_addr(NAND_Addr *address, PhysicalAddrs *addr_struct) {
    // Calculate physical address components based on logical address
    addr_struct->block   = ADDRESS_2_BLOCK(*address);
    addr_struct->page    = ADDRESS_2_PAGE(*address);
    addr_struct->colAddr = ADDRESS_2_COL(*address);

    // Removed assignments to non-existent members:
    // addr_struct->plane   = ADDRESS_2_PLANE(*address); // REMOVED
    // addr_struct->rowAddr = ((ADDRESS_2_BLOCK(*address) << ROW_ADDRESS_PAGE_BITS) | ADDRESS_2_PAGE(*address)); // REMOVED

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
    if (NAND_Read_ID(hspi, &dev_ID) != Ret_Success) {
        return Ret_Failed; // Failed to read ID
    }
    if (dev_ID.manufacturer_ID != NAND_ID_MANUFACTURER || dev_ID.device_ID != NAND_ID_DEVICE) {
        return Ret_WrongID;
    }
    return Ret_Success;
}

/**
    @brief NAND_Write: Writes a buffer to NAND flash at the specified logical address.
    @note  This simple implementation assumes the write fits within one page and does not span pages.
           Logical address maps only to the data area.
*/
NAND_HL_ReturnType NAND_Write(NAND_Addr start_addr, const uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phys_addr;
    if (length == 0 || length > PAGE_DATA_SIZE) {
        return Ret_ProgramFailed;
    }
    if (__map_logical_addr(&start_addr, &phys_addr) != Ret_Success) {
        return Ret_ProgramFailed;
    }
    if ((phys_addr.colAddr + length) > PAGE_DATA_SIZE) {
         return Ret_ProgramFailed; // Write spans across page boundary
    }
    if (NAND_Page_Program(hspi, &phys_addr, (uint8_t*)buffer, length) != Ret_Success) {
        return Ret_ProgramFailed;
    }
    return Ret_Success;
}

/**
    @brief NAND_Read: Reads data from NAND flash starting at the given logical address.
    @note  This function maps the logical address and then reads the data.
           Reads only from the data area. Assumes read fits within one page.
*/
NAND_HL_ReturnType NAND_Read(NAND_Addr start_addr, uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phys_addr;
    if (length == 0 || length > PAGE_DATA_SIZE) {
        return Ret_ReadFailed;
    }
    if (__map_logical_addr(&start_addr, &phys_addr) != Ret_Success) {
        return Ret_ReadFailed;
    }
     if ((phys_addr.colAddr + length) > PAGE_DATA_SIZE) {
         return Ret_ReadFailed; // Read spans across page boundary
     }
    if (NAND_Page_Read(hspi, &phys_addr, buffer, length) != Ret_Success) {
        return Ret_ReadFailed;
    }
    return Ret_Success;
}

/**
    @brief NAND_Erase_Block: Erases a block given its physical block index.
    @note  The block index is the physical block number (0 to NUM_BLOCKS-1).
*/
NAND_HL_ReturnType NAND_Erase_Block(uint32_t block_index, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phys_addr;
    if (block_index >= NUM_BLOCKS) {
        return Ret_EraseFailed;
    }
    phys_addr.block = (uint16_t)block_index;
    phys_addr.page = 0;
    phys_addr.colAddr = 0;

    // Removed assignment to non-existent member:
    // phys_addr.rowAddr = (block_index << ROW_ADDRESS_PAGE_BITS) | 0; // REMOVED

    if (NAND_Block_Erase(hspi, &phys_addr) != Ret_Success) {
        return Ret_EraseFailed;
    }
    return Ret_Success;
}
