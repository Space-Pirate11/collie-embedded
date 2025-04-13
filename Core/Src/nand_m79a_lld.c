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
#include "nand_spi.h" // Include SPI HAL wrapper

/******************************************************************************
 * Status Operations
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

    NAND_SPI_Select(); // Select chip
    NAND_SPI_ReturnType SPI_Status = NAND_SPI_Send(hspi, &transmit);
    NAND_SPI_Deselect(); // Deselect chip

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
           Includes a simple timeout mechanism.
    @return Ret_Success if ready, or Ret_NANDBusy if timeout occurs.
*/
NAND_ReturnType NAND_Wait_Until_Ready(SPI_HandleTypeDef *hspi) {
    uint8_t timeout_counter = 0;
    const uint8_t max_attempts = 100; // Increased attempts for safety (e.g., 100ms total wait)
    uint8_t status_reg = 0xFF;
    NAND_ReturnType status;

    do {
        status = NAND_Get_Features(hspi, SPI_NAND_STATUS_REG_ADDR, &status_reg);
        if (status != Ret_Success) {
            return Ret_Failed; // Error reading status register
        }

        if (!CHECK_OIP(status_reg)) {
            return Ret_Success; // Device is ready
        }

        NAND_Wait(1); // Wait 1ms before polling again
        timeout_counter++;

    } while (timeout_counter < max_attempts);

    return Ret_NANDBusy; // Timeout occurred
}

/******************************************************************************
 * Identification Operations
 *****************************************************************************/

/**
    @brief NAND_Read_ID: Reads the manufacturer and device ID of the NAND flash chip.
    @note  Sends SPI_NAND_READ_ID command with one dummy byte.
    @return Ret_Success if the read is successful, Ret_Failed otherwise.
*/
NAND_ReturnType NAND_Read_ID(SPI_HandleTypeDef *hspi, NAND_ID *nand_ID) {
    uint8_t command[] = { SPI_NAND_READ_ID, 0x00 }; // Command + 1 dummy byte for address/clock cycles
    uint8_t data_rx[2];                           // Buffer for JEDEC ID bytes read

    SPI_Params tx = { .buffer = command, .length = sizeof(command) }; // Send command and dummy
    SPI_Params rx = { .buffer = data_rx, .length = sizeof(data_rx) }; // Receive 2 bytes ID

    NAND_SPI_Select();
    // Send command, then receive data
    NAND_SPI_ReturnType spi_status = NAND_SPI_SendReceive(hspi, &tx, &rx);
    NAND_SPI_Deselect();

    if (spi_status == SPI_OK) {
        nand_ID->manufacturer_ID = data_rx[0]; // First byte received is Manufacturer ID
        nand_ID->device_ID       = data_rx[1]; // Second byte received is Device ID
        return Ret_Success;
    } else {
        return Ret_Failed;
    }
}


/******************************************************************************
 * Feature Operations
 *****************************************************************************/

/**
    @brief NAND_Check_Busy: Checks if the device is busy by reading the status register’s OIP bit.
    @return Ret_NANDBusy if the OIP bit is set; otherwise, Ret_Success. Ret_Failed on SPI error.
*/
NAND_ReturnType NAND_Check_Busy(SPI_HandleTypeDef *hspi) {
    uint8_t status_reg;
    NAND_ReturnType status = NAND_Get_Features(hspi, SPI_NAND_STATUS_REG_ADDR, &status_reg);
    if (status != Ret_Success) {
        return Ret_Failed; // Failed to read status
    }
    return (CHECK_OIP(status_reg)) ? Ret_NANDBusy : Ret_Success;
}

/**
    @brief NAND_Get_Features: Reads a feature register from the NAND flash using the GET FEATURE command.
    @param reg_addr: The register address to read (one of the defined ones).
    @param reg: Pointer where the register value will be stored.
    @return Ret_Success on success; Ret_Failed on SPI error.
*/
NAND_ReturnType NAND_Get_Features(SPI_HandleTypeDef *hspi, RegisterAddr reg_addr, uint8_t *reg) {
    uint8_t command[] = { SPI_NAND_GET_FEATURES, (uint8_t)reg_addr };
    SPI_Params tx = { .buffer = command, .length = sizeof(command) };
    SPI_Params rx = { .buffer = reg, .length = 1 };

    NAND_SPI_Select();
    NAND_SPI_ReturnType status = NAND_SPI_SendReceive(hspi, &tx, &rx);
    NAND_SPI_Deselect();

    return (status == SPI_OK) ? Ret_Success : Ret_Failed;
}

/**
    @brief NAND_Set_Features: Writes a value to a feature register using the SET FEATURE command.
    @note  The status register is read-only and cannot be written.
    @return Ret_Success on success; Ret_RegAddressInvalid if attempting to write to status; Ret_Failed on SPI error.
*/
NAND_ReturnType NAND_Set_Features(SPI_HandleTypeDef *hspi, RegisterAddr reg_addr, uint8_t reg) {
    // Status register (C0h) is read-only according to datasheet page 39
    if (reg_addr == SPI_NAND_STATUS_REG_ADDR) {
        return Ret_RegAddressInvalid;
    }
    uint8_t command[] = { SPI_NAND_SET_FEATURES, (uint8_t)reg_addr, reg };
    SPI_Params tx = { .buffer = command, .length = sizeof(command) };

    NAND_SPI_Select();
    NAND_SPI_ReturnType status = NAND_SPI_Send(hspi, &tx);
    NAND_SPI_Deselect();

    // It's good practice to wait until the device is ready after Set Features
    if (status == SPI_OK) {
        return NAND_Wait_Until_Ready(hspi);
    } else {
        return Ret_Failed;
    }
}


/******************************************************************************
 * Read Operations
 *****************************************************************************/

/**
    @brief NAND_Page_Read: Reads data from a page in NAND flash.
    @note  The read sequence:
           1) Send PAGE READ command (0x13) with 3-byte row address to load page into cache.
           2) Wait until device is ready (OIP bit clear in status register).
           3) Send READ FROM CACHE command (0x03) with 2-byte column address and 1 dummy byte.
           4) Receive 'length' bytes of data.
    @param addr: Pointer to the physical address structure (block, page, colAddr).
    @param buffer: Buffer to store the read data.
    @param length: Number of bytes to read (should be within PAGE_TOTAL_SIZE).
    @return Ret_Success on success; Ret_ReadFailed if any SPI call fails or length invalid.
*/
NAND_ReturnType NAND_Page_Read(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr, uint8_t *buffer, uint16_t length) {
    NAND_SPI_ReturnType status;

    // Validate length against total page size
    if (length == 0 || length > PAGE_TOTAL_SIZE) {
        return Ret_ReadFailed; // Invalid length
    }

    /* Command 1: Page Read (Loads page from array to cache) */
    uint32_t row_addr = CALC_ROW_ADDRESS(addr); // Calculate 17-bit row address
    uint8_t command_page_read[4] = {
        SPI_NAND_PAGE_READ,
        0x00, // Dummy byte (PA16 - always 0 as Row Address is 17 bits B10-B0, P5-P0)
        (uint8_t)(row_addr >> 8), // Row Address A15-A8 (B10-B3)
        (uint8_t)(row_addr & 0xFF) // Row Address A7-A0 (B2-B0, P5-P0)
    };
    SPI_Params tx_page_read = { .buffer = command_page_read, .length = sizeof(command_page_read) };

    NAND_SPI_Select();
    status = NAND_SPI_Send(hspi, &tx_page_read);
    NAND_SPI_Deselect();

    if (status != SPI_OK) {
        return Ret_ReadFailed;
    }

    /* Command 2: Wait until device is ready (data loaded into cache) */
    if (NAND_Wait_Until_Ready(hspi) != Ret_Success) {
        return Ret_ReadFailed; // Device failed to become ready
    }

    /* Command 3: Read From Cache (Reads data from cache to output) */
    uint16_t col_addr = addr->colAddr; // Get 12-bit column address
    uint8_t command_cache_read[4] = {
        SPI_NAND_READ_CACHE_X1,
        (uint8_t)(col_addr >> 8),   // Column Address A11-A8
        (uint8_t)(col_addr & 0xFF), // Column Address A7-A0
        DUMMY_BYTE                  // Dummy byte required after address
    };
    SPI_Params tx_cache_read = { .buffer = command_cache_read, .length = sizeof(command_cache_read) };
    SPI_Params rx_cache_read = { .buffer = buffer, .length = length };

    NAND_SPI_Select();
    status = NAND_SPI_SendReceive(hspi, &tx_cache_read, &rx_cache_read);
    NAND_SPI_Deselect();

    return (status == SPI_OK) ? Ret_Success : Ret_ReadFailed;
}


/******************************************************************************
 * Write Operations
 *****************************************************************************/

/**
    @brief NAND_Page_Program: Programs (writes) data to a NAND flash page.
    @note  Write sequence:
           1) Write Enable command (0x06).
           2) Program Load command (0x02) with 2-byte column address and data.
           3) Program Execute command (0x10) with 3-byte row address.
           4) Wait until device is ready (OIP bit clear).
           5) Optional: Check status register for Program Fail bit (P_FAIL).
           6) Write Disable command (0x04) - usually done implicitly by CS high, but explicit is safer.
    @param addr: Pointer to the physical address structure (block, page, colAddr).
    @param buffer: Buffer containing data to program.
    @param length: Number of data bytes to program (must be <= PAGE_TOTAL_SIZE).
    @return Ret_Success on success; Ret_ProgramFailed on any error or if length invalid.
*/
NAND_ReturnType NAND_Page_Program(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr, uint8_t *buffer, uint16_t length) {
    NAND_SPI_ReturnType status;

    // Validate length against total page size
    if (length == 0 || length > PAGE_TOTAL_SIZE) {
        return Ret_ProgramFailed; // Invalid length
    }

    /* Command 1: Write Enable */
    status = __write_enable(hspi);
    if (status != SPI_OK) {
        return Ret_ProgramFailed;
    }

    /* Command 2: Program Load (Loads data into cache) */
    uint16_t col_addr = addr->colAddr; // Get 12-bit column address
    uint8_t command_load[3] = {
        SPI_NAND_PROGRAM_LOAD_X1,
        (uint8_t)(col_addr >> 8),   // Column Address A11-A8
        (uint8_t)(col_addr & 0xFF)  // Column Address A7-A0
    };
    SPI_Params tx_cmd = { .buffer = command_load, .length = sizeof(command_load) };
    SPI_Params tx_data = { .buffer = buffer, .length = length };

    NAND_SPI_Select();
    status = NAND_SPI_Send_Command_Data(hspi, &tx_cmd, &tx_data);
    NAND_SPI_Deselect();

    if (status != SPI_OK) {
        // Attempt to Write Disable before returning error
        __write_disable(hspi);
        return Ret_ProgramFailed;
    }

    /* Command 3: Program Execute (Writes cache data to memory array) */
    uint32_t row_addr = CALC_ROW_ADDRESS(addr); // Calculate 17-bit row address
    uint8_t command_exec[4] = {
        SPI_NAND_PROGRAM_EXEC,
        0x00, // Dummy byte
        (uint8_t)(row_addr >> 8), // Row Address A15-A8
        (uint8_t)(row_addr & 0xFF) // Row Address A7-A0
    };
    SPI_Params exec_cmd = { .buffer = command_exec, .length = sizeof(command_exec) };

    NAND_SPI_Select();
    status = NAND_SPI_Send(hspi, &exec_cmd);
    NAND_SPI_Deselect();

    if (status != SPI_OK) {
        // Attempt to Write Disable if possible, though WEL might already be cleared by error
        // __write_disable(hspi); // WEL state uncertain on error
        return Ret_ProgramFailed;
    }

    /* Command 4: Wait until the device is ready (program operation complete) */
    if (NAND_Wait_Until_Ready(hspi) != Ret_Success) {
        // WEL might be cleared implicitly by internal operations after timeout,
        // but attempting disable is harmless.
        __write_disable(hspi);
        return Ret_ProgramFailed; // Device failed to become ready
    }

    /* Command 5: Optional - Check Status Register for Program Failure */
    uint8_t status_reg;
    if (NAND_Get_Features(hspi, SPI_NAND_STATUS_REG_ADDR, &status_reg) == Ret_Success) {
        if (status_reg & SPI_NAND_P_FAIL) {
            // Program operation failed according to status bit
             __write_disable(hspi); // Ensure WEL is disabled
             return Ret_ProgramFailed;
        }
    } else {
        // Failed to read status register after program
         __write_disable(hspi); // Ensure WEL is disabled
        return Ret_ProgramFailed;
    }


    /* Command 6: Write Disable (Good practice, though CS high usually suffices) */
    // WEL is automatically cleared after Program Execute completes (successfully or not) - Datasheet Fig 13.
    // So, calling __write_disable(hspi) here is redundant but harmless.

    return Ret_Success;
}


/******************************************************************************
 * Erase Operations
 *****************************************************************************/

/**
    @brief NAND_Block_Erase: Erases an entire block of NAND flash.
    @note  Erase sequence:
           1) Write Enable (0x06).
           2) Block Erase command (0xD8) with 3-byte row address (block + page 0).
           3) Wait until device is ready (OIP bit clear).
           4) Optional: Check status register for Erase Fail bit (E_FAIL).
           5) Write Disable (0x04) - often implicit.
    @param addr: Pointer to the physical address structure (only the block number is relevant).
    @return Ret_Success on success; Ret_EraseFailed on error.
*/
NAND_ReturnType NAND_Block_Erase(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr) {
    NAND_SPI_ReturnType status;

    /* Command 1: Write Enable */
    status = __write_enable(hspi);
    if (status != SPI_OK) {
        return Ret_EraseFailed;
    }

    /* Command 2: Block Erase */
    // Erase command requires the row address of the first page in the block.
    addr->page = 0; // Ensure page is 0 for block erase row address
    uint32_t row_addr = CALC_ROW_ADDRESS(addr); // Calculate 17-bit row address

    uint8_t command[4] = {
        SPI_NAND_BLOCK_ERASE,
        0x00, // Dummy byte
        (uint8_t)(row_addr >> 8), // Row Address A15-A8
        (uint8_t)(row_addr & 0xFF) // Row Address A7-A0
    };
    SPI_Params tx_cmd = { .buffer = command, .length = sizeof(command) };

    NAND_SPI_Select();
    status = NAND_SPI_Send(hspi, &tx_cmd);
    NAND_SPI_Deselect();

    if (status != SPI_OK) {
        // __write_disable(hspi); // WEL state uncertain
        return Ret_EraseFailed;
    }

    /* Command 3: Wait until operation completes */
    if (NAND_Wait_Until_Ready(hspi) != Ret_Success) {
         // __write_disable(hspi);
        return Ret_EraseFailed; // Device failed to become ready
    }

    /* Command 4: Optional - Check Status Register for Erase Failure */
     uint8_t status_reg;
    if (NAND_Get_Features(hspi, SPI_NAND_STATUS_REG_ADDR, &status_reg) == Ret_Success) {
        if (status_reg & SPI_NAND_E_FAIL) {
            // Erase operation failed according to status bit
             // __write_disable(hspi); // WEL automatically cleared
             return Ret_EraseFailed;
        }
    } else {
        // Failed to read status register after erase
         // __write_disable(hspi); // WEL automatically cleared
        return Ret_EraseFailed;
    }


    /* Command 5: Write Disable (WEL is cleared automatically after Block Erase) */
    // __write_disable(hspi); // Redundant

    return Ret_Success;
}

/******************************************************************************
 * Internal Helper Functions
 *****************************************************************************/

/**
    @brief __write_enable: Issues the WRITE ENABLE command (0x06).
*/
NAND_SPI_ReturnType __write_enable(SPI_HandleTypeDef *hspi) {
    uint8_t command = SPI_NAND_WRITE_ENABLE;
    SPI_Params transmit = { .buffer = &command, .length = 1 };
    NAND_SPI_Select();
    NAND_SPI_ReturnType status = NAND_SPI_Send(hspi, &transmit);
    NAND_SPI_Deselect();
    return status;
}

/**
    @brief __write_disable: Issues the WRITE DISABLE command (0x04).
*/
NAND_SPI_ReturnType __write_disable(SPI_HandleTypeDef *hspi) {
    uint8_t command = SPI_NAND_WRITE_DISABLE;
    SPI_Params transmit = { .buffer = &command, .length = 1 };
    NAND_SPI_Select();
    NAND_SPI_ReturnType status = NAND_SPI_Send(hspi, &transmit);
    NAND_SPI_Deselect();
    return status;
}
