/*
 * nand_m79a.c
 *
 *  Created on: Apr 11, 2025
 *      Author: amoltandon
 */

#include "nand_m79a.h"
#include <string.h>

static NAND_HL_ReturnType __map_logical_addr(NAND_Addr *address, PhysicalAddrs *addr_struct)
{
    addr_struct->plane   = ADDRESS_2_PLANE(*address);
    addr_struct->block   = ADDRESS_2_BLOCK(*address);
    addr_struct->page    = ADDRESS_2_PAGE(*address);
    addr_struct->rowAddr = ((ADDRESS_2_BLOCK(*address) << ROW_ADDRESS_PAGE_BITS) | ADDRESS_2_PAGE(*address));
    addr_struct->colAddr = ADDRESS_2_COL(*address);
    return Ret_Success;
}

NAND_HL_ReturnType NAND_Init_HighLevel(SPI_HandleTypeDef *hspi) {
    NAND_ID dev_ID;
    NAND_Wait(T_POR);
    if (NAND_Reset(hspi) != Ret_Success) {
        return Ret_ResetFailed;
    }
    NAND_Read_ID(hspi, &dev_ID);
    if (dev_ID.manufacturer_ID != NAND_ID_MANUFACTURER || dev_ID.device_ID != NAND_ID_DEVICE) {
        return Ret_WrongID;
    }
    return Ret_Success;
}

NAND_HL_ReturnType NAND_Write(NAND_Addr start_addr, const uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phy_addr;
    __map_logical_addr(&start_addr, &phy_addr);
    if(NAND_Page_Program(hspi, &phy_addr, (uint8_t*)buffer, length) != Ret_Success)
    {
        return Ret_ProgramFailed;
    }
    return Ret_Success;
}

NAND_HL_ReturnType NAND_Read(NAND_Addr start_addr, uint8_t *buffer, uint32_t length, SPI_HandleTypeDef *hspi) {
    PhysicalAddrs phy_addr;
    __map_logical_addr(&start_addr, &phy_addr);
    NAND_Page_Read(hspi, &phy_addr, buffer, length);
    return Ret_Success;
}

NAND_HL_ReturnType NAND_Erase_Block(uint32_t block_index, SPI_HandleTypeDef *hspi) {
    return NAND_Block_Erase(hspi, block_index);
}
