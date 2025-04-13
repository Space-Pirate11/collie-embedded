/************************** Flash Memory Driver ***********************************
    Filename:    nand_m79a_lld.h
    Description: Low-level driver functions for reading and writing to M79a NAND Flash via SPI.
                 This file is based on the Micron MT29F2G01ABAGDWB-IT datasheet and the original
                 development by Tharun Suresh.
    Version:     0.1
    Author:      Tharun Suresh
********************************************************************************
    Version History.
    Ver.    Date            Comments
    0.1     Jan 2022        In Development
********************************************************************************/

#ifndef INC_NAND_M79A_LLD_H_
#define INC_NAND_M79A_LLD_H_

#include "nand_spi.h"  // Our SPI wrapper header
#include <stdint.h>    // Include for uint types

/* Functions Return Codes for NAND operations */
typedef enum {
    Ret_Success,
    Ret_Failed,
    Ret_ResetFailed,
    Ret_WrongID,
    Ret_NANDBusy,
    /* Ret_AddressInvalid, */
    Ret_RegAddressInvalid,
    /* Ret_MemoryOverflow, Ret_BlockEraseFailed, Ret_PageNrInvalid, Ret_SubSectorNrInvalid,
       Ret_SectorNrInvalid, Ret_FunctionNotSupported, Ret_NoInformationAvailable,
       Ret_OperationOngoing, Ret_OperationTimeOut, */
    Ret_ReadFailed,
    Ret_ProgramFailed,
    Ret_EraseFailed,
    /* Ret_SectorProtected, Ret_SectorUnprotected, Ret_SectorProtectFailed, Ret_SectorUnprotectFailed,
       Ret_SectorLocked, Ret_SectorUnlocked, Ret_SectorLockDownFailed, */
    Ret_WrongType
} NAND_ReturnType;

/* Define supported device model */
#define MT29F2G01ABAGD

#ifdef MT29F2G01ABAGD

    /* Device ID structure */
    typedef struct {
        uint8_t manufacturer_ID;
        uint8_t device_ID;
    } NAND_ID;
    #define NAND_ID_MANUFACTURER    0x2C
    #define NAND_ID_DEVICE          0x24

    /* Device geometry based on datasheet: */
    #define FLASH_WIDTH             8               /* Data width in bits */
    #define FLASH_SIZE_BYTES        0x10000000      /* 256 MB flash size (2Gb -> 256MB) */
    #define NUM_BLOCKS              2048            /* Number of blocks */
    #define NUM_PAGES_PER_BLOCK     64              /* Pages per block */
    #define PAGE_TOTAL_SIZE         2176            /* Total bytes per page (data+spare) */
    #define PAGE_DATA_SIZE          2048            /* Data bytes per page */
    #define PAGE_SPARE_SIZE         128             /* Spare bytes per page (Actual spare = 2176-2048 = 128 bytes)*/

    #define BAD_BLOCK_MARKER_POS    PAGE_DATA_SIZE  /* Position of bad block marker in spare area (e.g., first byte) */
    #define BAD_BLOCK_MARKER_VALUE  0x00            /* Value indicating a bad block */

    /* Addressing definitions: */
    typedef uint32_t NAND_Addr; // Logical address (byte offset)

    /* Bit counts per address field according to datasheet page 11 */
    // Row Address = Page Address + Block Address
    // Block Address = 11 bits (B10..B0)
    // Page Address = 6 bits (P5..P0)
    // Total Row Address bits = 17 bits. Datasheet says 24 bits - ensure this matches device variant if different
    #define ROW_ADDRESS_BLOCK_BITS   11
    #define ROW_ADDRESS_PAGE_BITS    6
    // Column Address = 12 bits (C11..C0) - selects byte within the 2176-byte page
    #define COL_ADDRESS_BITS         12

    /* Structure for physical addressing within NAND flash */
    typedef struct {
        // Note: Using bit-fields can have implementation-defined padding/alignment.
        // Using standard types might be safer if portability is critical.
        uint16_t block       : ROW_ADDRESS_BLOCK_BITS;  // Block number (0-2047)
        uint16_t page        : ROW_ADDRESS_PAGE_BITS;   // Page number within block (0-63)
        uint16_t colAddr     : COL_ADDRESS_BITS;        // Column (offset) address within page (0-2175)
    } PhysicalAddrs;

    /* Macros to extract parts of the logical address (if needed in nand_m79a.c) */
    /* Block Address = 11 bits (B10..B0) -> Logical Address >> (6 + 11) = LA >> 17 ? No, >> (6+12) for byte address? Check datasheet.
       Based on Page Size 2048 (2^11) and Pages/Block 64 (2^6): Block size = 2^17 bytes.
       LogicalAddr / BlockSize = Block Number */
    #define ADDRESS_2_BLOCK(Address)    ((uint16_t)((Address) / (PAGE_DATA_SIZE * NUM_PAGES_PER_BLOCK)))
    /* Page Address = 6 bits (P5..P0) -> (LogicalAddr % BlockSize) / PageSize */
    #define ADDRESS_2_PAGE(Address)     ((uint16_t)(((Address) % (PAGE_DATA_SIZE * NUM_PAGES_PER_BLOCK)) / PAGE_DATA_SIZE))
    /* Column Address = 12 bits (C11..C0) -> LogicalAddr % PageSize */
    #define ADDRESS_2_COL(Address)      ((uint16_t)((Address) % PAGE_DATA_SIZE)) // Within the data area

    /* Macro to calculate the 17-bit Row Address (Block + Page) needed for commands */
    /* Row Address = {BlockAddr[10:0], PageAddr[5:0]} */
    #define CALC_ROW_ADDRESS(addr_struct) (((uint32_t)(addr_struct)->block << ROW_ADDRESS_PAGE_BITS) | (uint32_t)(addr_struct)->page)

    /* Status Register Bit Definitions (Datasheet page 39, Register C0h) */
    #define SPI_NAND_OIP            (1 << 0) // Bit 0: Operation In Progress (1=Busy, 0=Ready)
    #define SPI_NAND_WEL            (1 << 1) // Bit 1: Write Enable Latch (1=Enabled, 0=Disabled)
    #define SPI_NAND_E_FAIL         (1 << 2) // Bit 2: Erase Fail (1=Failed, 0=Passed)
    #define SPI_NAND_P_FAIL         (1 << 3) // Bit 3: Program Fail (1=Failed, 0=Passed)
    #define SPI_NAND_ECCS0          (1 << 4) // Bit 4: ECC Status 0
    #define SPI_NAND_ECCS1          (1 << 5) // Bit 5: ECC Status 1
    // Bit 6: Reserved
    // Bit 7: Cache Read Busy (Internal use)

    /* Macro to check the Operation In Progress (OIP) bit in Status Register */
    #define CHECK_OIP(status_reg)       ((status_reg) & SPI_NAND_OIP)

    /* Command Code Definitions per datasheet page 13 */
    typedef enum {
        SPI_NAND_RESET                  = 0xFF,
        SPI_NAND_GET_FEATURES           = 0x0F,
        SPI_NAND_SET_FEATURES           = 0x1F,
        SPI_NAND_READ_ID                = 0x9F,
        SPI_NAND_PAGE_READ              = 0x13, // Read page data into cache
        SPI_NAND_READ_PAGE_CACHE_RANDOM = 0x30, // Read page data into cache (random) - Not typically used for sequential read
        SPI_NAND_READ_PAGE_CACHE_LAST   = 0x3F, // Read page data into cache (last) - Not typically used
        SPI_NAND_READ_CACHE_X1          = 0x03, // Read data from cache (1x I/O)
        SPI_NAND_READ_CACHE_X2          = 0x3B, // Read data from cache (2x I/O)
        SPI_NAND_READ_CACHE_X4          = 0x6B, // Read data from cache (4x I/O)
        SPI_NAND_READ_CACHE_DUAL_IO     = 0xBB, // Read data from cache (Dual I/O)
        SPI_NAND_READ_CACHE_QUAD_IO     = 0xEB, // Read data from cache (Quad I/O)
        SPI_NAND_WRITE_ENABLE           = 0x06,
        SPI_NAND_WRITE_DISABLE          = 0x04,
        SPI_NAND_BLOCK_ERASE            = 0xD8,
        SPI_NAND_PROGRAM_EXEC           = 0x10, // Execute program (write cache to array)
        SPI_NAND_PROGRAM_LOAD_X1        = 0x02, // Load program data into cache (1x I/O)
        SPI_NAND_PROGRAM_LOAD_X4        = 0x32, // Load program data into cache (4x I/O)
        SPI_NAND_PROGRAM_LOAD_RANDOM_X1 = 0x84, // Load random program data (1x I/O)
        SPI_NAND_PROGRAM_LOAD_RANDOM_X4 = 0x34, // Load random program data (4x I/O)
        // Block Lock commands not fully implemented here
        // SPI_NAND_PERMANENT_BLK_LOCK     = 0x2C
    } CommandCodes;

    /* Register address definitions for Get/Set Feature commands (datasheet page 37) */
    typedef enum {
        SPI_NAND_BLKLOCK_REG_ADDR = 0xA0, // Block Lock Register
        SPI_NAND_CFG_REG_ADDR     = 0xB0, // Configuration Register
        SPI_NAND_STATUS_REG_ADDR  = 0xC0, // Status Register
        SPI_NAND_DIE_SEL_REG_ADDR = 0xD0, // Die Select Register (Not applicable for single die)
    } RegisterAddr;

    /* Internal time constants (in ms) used in NAND commands */
    #define T_POR           2  /* Power on reset wait time: minimum 1.25 ms, rounded up */
    #define T_BERS          10 /* Max Block Erase time: 10ms */
    #define T_PROG          1  /* Max Page Program time: 700us, rounded up */
    #define T_RD            1  /* Max Page Read time: 120us, rounded up */

#endif  // MT29F2G01ABAGD

/******************************************************************************
 * Internal Functions Prototypes
 *****************************************************************************/
NAND_SPI_ReturnType __write_enable(SPI_HandleTypeDef *hspi);
NAND_SPI_ReturnType __write_disable(SPI_HandleTypeDef *hspi);

/******************************************************************************
 * List of APIs
 *****************************************************************************/
/* Status operations */
NAND_ReturnType NAND_Reset(SPI_HandleTypeDef *hspi);
NAND_ReturnType NAND_Wait_Until_Ready(SPI_HandleTypeDef *hspi); // Timeout handled internally

/* Identification operations */
NAND_ReturnType NAND_Read_ID(SPI_HandleTypeDef *hspi, NAND_ID *nand_ID);

/* Feature operations */
NAND_ReturnType NAND_Check_Busy(SPI_HandleTypeDef *hspi);
NAND_ReturnType NAND_Get_Features(SPI_HandleTypeDef *hspi, RegisterAddr reg_addr, uint8_t *reg);
NAND_ReturnType NAND_Set_Features(SPI_HandleTypeDef *hspi, RegisterAddr reg_addr, uint8_t reg);

/* Read operations */
NAND_ReturnType NAND_Page_Read(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr, uint8_t *buffer, uint16_t length);

/* Write operations */
NAND_ReturnType NAND_Page_Program(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr, uint8_t *buffer, uint16_t length);

/* Erase operation */
NAND_ReturnType NAND_Block_Erase(SPI_HandleTypeDef *hspi, PhysicalAddrs *addr);

#endif /* INC_NAND_M79A_LLD_H_ */
