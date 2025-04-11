/************************** Flash Memory Driver ***********************************
    Filename:    nand_m79a_lld.c
    Description: Low-level driver functions for reading/writing to M79a NAND Flash via SPI.
                 The functions follow the command sequences in the datasheet.
    Version:     0.1
    Author:      Tharun Suresh
********************************************************************************
    Version History.
    Ver.    Date            Comments
    0.1     Jan 2022        In Development
********************************************************************************/

#include "nand_m79a_lld.h"

/******************************************************************************
 *                              Status Operations
 *****************************************************************************/

/**
    @brief NAND_Reset: Sends a reset command to the NAND flash chip.
    @note  This issues SPI_NAND_RESET command and waits T_POR then polls the status until the OIP bit clears.
    @return NAND_ReturnType:
            Ret_ResetFailed if SPI transaction fails,
            Ret_NANDBusy if the device remains busy,
            Ret_Success otherwise.
*/
NAND_ReturnType NAND_Reset(SPI_HandleTypeDef *hspi) {
    uint8_t command = SPI_NAND_RESET;
    SPI_Params transmit = { .buffer = &command, .length = 1 };

    NAND_SPI_ReturnType SPI_Status = NAND_SPI_Send(hspi, &transmit);
    NAND_Wait(T_POR);  // Wait for power-on reset time (approx. 2 ms)
    if (SPI_Status != SPI_OK) {
        return Ret_ResetFailed;
    } else {
        // Wait until the device indicates it is ready (OIP bit cleared)
        return NAND_Wait_Until_Ready(hspi);
    }
}

/**
    @brief NAND_Wait_Until_Ready: Polls the NAND status until it is ready.
    @note  This function reads the status register using NAND_Get_Features.
    @return Ret_Success if ready, or Ret_NANDBusy if timeout occurs.
*/
NAND_ReturnType NAND_Wait_Until_Ready(SPI_HandleTypeDef *hspi) {
    uint8_t timeout_counter = 0;
    const uint8_t max_attempts = 2;
    uint8_t status_reg;
    SPI_Params rx = { .buffer = &status_reg, .length = 1 };

    /* Check if device is busy using NAND_Check_Busy */
    NAND_ReturnType status = NAND_Check_Busy(hspi);
    if (status == Ret_NANDBusy) {
        while (CHECK_OIP(status_reg)) {
            if (timeout_counter < max_attempts) {
                NAND_SPI_Receive(hspi, &rx);
                NAND_Wait(1);
                timeout_counter++;
            } else {
                return Ret_NANDBusy;
            }
        }
    }
    return Ret_Success;
}

/******************************************************************************
 *                      Identification Operations
 *****************************************************************************/

/**
    @brief NAND_Read_ID: Reads the manufacturer and device ID of the NAND flash chip.
    @note  Sends SPI_NAND_READ_ID command with one dummy byte.
    @return Ret_Success if the read is successful.
*/
NAND_ReturnType NAND_Read_ID(SPI_HandleTypeDef *hspi, NAND_ID *nand_ID) {
    uint8_t data_tx[] = { SPI_NAND_READ_ID, 0 }; // Dummy second byte
    uint8_t data_rx[2];                         // Buffer for two bytes of data

    SPI_Params tx = { .buffer = data_tx, .length = 2 };
    SPI_Params rx = { .buffer = data_rx, .length = 2 };

    NAND_SPI_SendReceive(hspi, &tx, &rx);
    nand_ID->manufacturer_ID = data_rx[0];  // First byte read
    nand_ID->device_ID = data_rx[1];          // Second byte read
    return Ret_Success;
}

/******************************************************************************
 *                              Feature Operations
 *****************************************************************************/

/**
    @brief NAND_Check_Busy: Checks if the device is busy by reading the status register’s OIP bit.
    @return Ret_NANDBusy if the OIP bit is set; otherwise, Ret_Success.
*/
NAND_ReturnType NAND_Check_Busy(SPI_HandleTypeDef *hspi) {
    uint8_t status_reg;
    NAND_Get_Features(hspi, SPI_NAND_STATUS_REG_ADDR, &status_reg);
    return (CHECK_OIP(status_reg)) ? Ret_NANDBusy : Ret_Success;
}

/**
    @brief NAND_Get_Features: Reads a feature register from the NAND flash using the GET FEATURE command.
    @param reg_addr: The register address to read (one of the defined ones).
    @param reg: Pointer where the register value will be stored.
    @return Ret_Success on success; Ret_Failed on SPI error.
*/
NAND_ReturnType NAND_Get_Features(SPI_HandleTypeDef *hspi, RegisterAddr reg_addr, uint8_t *reg) {
    uint8_t command[] = { SPI_NAND_GET_FEATURES, reg_addr };
    SPI_Params tx = { .buffer = command, .length = 2 };
    SPI_Params rx = { .buffer = reg, .length = 1 };
    NAND_SPI_ReturnType status = NAND_SPI_SendReceive(hspi, &tx, &rx);
    return (status == SPI_OK) ? Ret_Success : Ret_Failed;
}

/**
    @brief NAND_Set_Features: Writes a value to a feature register using the SET FEATURE command.
    @note  The status register is read-only and cannot be written.
    @return Ret_Success on success; Ret_RegAddressInvalid if attempting to write to status; Ret_Failed on SPI error.
*/
NAND_ReturnType NAND_Set_Features(SPI_HandleTypeDef *hspi, RegisterAddr reg_addr, uint8_t reg) {
    if (reg_addr == SPI_NAND_STATUS_REG_ADDR) {
        return Ret_RegAddressInvalid;
    }
    uint8_t command[] = { SPI_NAND_SET_FEATURES, reg_addr, reg };
    SPI_Params tx = { .buffer = command, .length = 3 };
    NAND_SPI_ReturnType status = NAND_SPI_Send(hspi, &tx);
    return (status == SPI_OK) ? Ret_Success : Ret_Failed;
}

/******************************************************************************
 *                              Read Operations
 *****************************************************************************/

/**
    @brief NAND_Page_Read: Reads data from a page in NAND flash.
    @note  The read sequence:
           1) Send PAGE READ command to load page into cache.
           2) Wait until device is ready.
           3) Send READ FROM CACHE command (with a dummy byte) and receive data.
    @param addr: Pointer to the physical address structure.
    @param buffer: Buffer to store the read data.
    @param length: Number of bytes to read (should be within PAGE_SIZE).
    @return Ret_Success on success; Ret_ReadFailed if any SPI call fails.
*/
NAND_ReturnType NAND_Page_Read(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr, uint8_t *buffer, uint16_t length) {
    NAND_SPI_ReturnType status;
    if (length > PAGE_SIZE) {
        return Ret_ReadFailed;
    }

    /* Command 1: Page Read */
    uint32_t row = addr->rowAddr;
    uint8_t command_page_read[4] = {
        SPI_NAND_PAGE_READ,
        (uint8_t)(row >> 16),
        (uint8_t)(row >> 8),
        (uint8_t)(row & 0xFF)
    };
    SPI_Params tx_page_read = { .buffer = command_page_read, .length = 4 };
    status = NAND_SPI_Send(hspi, &tx_page_read);
    if (status != SPI_OK) {
        return Ret_ReadFailed;
    }

    /* Command 2: Wait until device is ready (data loaded into cache) */
    if (NAND_Wait_Until_Ready(hspi) != Ret_Success) {
        return Ret_ReadFailed;
    }

    /* Command 3: Read From Cache */
    uint32_t col = addr->colAddr;
    uint8_t command_cache_read[4] = {
        SPI_NAND_READ_CACHE_X1,
        (uint8_t)(col >> 8),
        (uint8_t)(col & 0xFF),
        DUMMY_BYTE  // dummy byte
    };
    SPI_Params tx_cache_read = { .buffer = command_cache_read, .length = 4 };
    SPI_Params rx_cache_read = { .buffer = buffer, .length = length };

    status = NAND_SPI_SendReceive(hspi, &tx_cache_read, &rx_cache_read);
    return (status == SPI_OK) ? Ret_Success : Ret_ReadFailed;
}

/******************************************************************************
 *                              Write Operations
 *****************************************************************************/

/**
    @brief NAND_Page_Program: Programs (writes) data to a NAND flash page.
    @note  Write sequence:
           1) Write Enable command.
           2) Program Load command with data to be written.
           3) Program Execute command to write data from cache to memory.
           4) Wait until device is ready.
           5) Write Disable command.
    @param addr: Pointer to the physical address structure.
    @param buffer: Buffer containing data to program.
    @param length: Number of data bytes to program (must be ≤ PAGE_DATA_SIZE).
    @return Ret_Success on success; Ret_ProgramFailed on any error.
*/
NAND_ReturnType NAND_Page_Program(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr, uint8_t *buffer, uint16_t length) {
    NAND_SPI_ReturnType status;
    if (length > PAGE_DATA_SIZE) {
        return Ret_ProgramFailed;
    }

    /* Command 1: Write Enable */
    __write_enable(hspi);

    /* Command 2: Program Load */
    uint32_t col = addr->colAddr;
    uint8_t command_load[3] = {
        SPI_NAND_PROGRAM_LOAD_X1,
        (uint8_t)(col >> 8),
        (uint8_t)(col & 0xFF)
    };
    SPI_Params tx_cmd = { .buffer = command_load, .length = 3 };
    SPI_Params tx_data = { .buffer = buffer, .length = length };
    status = NAND_SPI_Send_Command_Data(hspi, &tx_cmd, &tx_data);
    if (status != SPI_OK) {
        return Ret_ProgramFailed;
    }

    /* Command 3: Program Execute */
    uint32_t row = addr->rowAddr;
    uint8_t command_exec[4] = {
        SPI_NAND_PROGRAM_EXEC,
        (uint8_t)(row >> 16),
        (uint8_t)(row >> 8),
        (uint8_t)(row & 0xFF)
    };
    SPI_Params exec_cmd = { .buffer = command_exec, .length = 4 };
    status = NAND_SPI_Send(hspi, &exec_cmd);
    if (status != SPI_OK) {
        return Ret_ProgramFailed;
    }

    /* Wait until the device is ready (program operation complete) */
    if (NAND_Wait_Until_Ready(hspi) != Ret_Success) {
        return Ret_ProgramFailed;
    }

    /* Command 4: Write Disable */
    __write_disable(hspi);

    /* In a complete implementation, here you would read the status register
       and check the program fail bit. For now, we assume success if no SPI error occurred.
    */
    return Ret_Success;
}

/******************************************************************************
 *                              Erase Operations
 *****************************************************************************/

/**
    @brief NAND_Block_Erase: Erases an entire block of NAND flash.
    @note  Erase sequence:
           1) Write Enable.
           2) Block Erase command.
           3) Wait until device is ready.
           4) Write Disable.
    @param addr: Pointer to the physical address structure (at least the block number is used).
    @return Ret_Success on success; Ret_EraseFailed on error.
*/
NAND_ReturnType NAND_Block_Erase(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr) {
    /* Command 1: Write Enable */
    __write_enable(hspi);

    /* Command 2: Block Erase */
    uint32_t block = addr->block; // Use the 11-bit block address
    uint8_t command[4] = {
        SPI_NAND_BLOCK_ERASE,
        (uint8_t)(block >> 16),
        (uint8_t)(block >> 8),
        (uint8_t)(block & 0xFF)
    };
    SPI_Params tx_cmd = { .buffer = command, .length = 4 };
    if (NAND_SPI_Send(hspi, &tx_cmd) != SPI_OK) {
        return Ret_EraseFailed;
    }

    /* Command 3: Wait until operation completes */
    if (NAND_Wait_Until_Ready(hspi) != Ret_Success) {
        return Ret_EraseFailed;
    }

    /* Command 4: Write Disable */
    __write_disable(hspi);

    /* Optionally, read the status register to check for erase failure bits here */
    return Ret_Success;
}

/******************************************************************************
 *                              Internal Helper Functions
 *****************************************************************************/

/**
    @brief __write_enable: Issues the WRITE ENABLE command.
*/
NAND_SPI_ReturnType __write_enable(SPI_HandleTypeDef *hspi) {
    uint8_t command = SPI_NAND_WRITE_ENABLE;
    SPI_Params transmit = { .buffer = &command, .length = 1 };
    return NAND_SPI_Send(hspi, &transmit);
}

/**
    @brief __write_disable: Issues the WRITE DISABLE command.
*/
NAND_SPI_ReturnType __write_disable(SPI_HandleTypeDef *hspi) {
    uint8_t command = SPI_NAND_WRITE_DISABLE;
    SPI_Params transmit = { .buffer = &command, .length = 1 };
    return NAND_SPI_Send(hspi, &transmit);
}

/* Additional operations such as copy-back, block locking, etc. are not implemented.
   They can be added here if needed in the future.
*/

