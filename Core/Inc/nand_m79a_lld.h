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
    #define FLASH_SIZE_BYTES        0x10000000      /* 256 MB flash size */
    #define NUM_BLOCKS              2048            /* Number of blocks */
    #define NUM_PAGES_PER_BLOCK     64              /* Pages per block */
    #define PAGE_SIZE               2176            /* Total bytes per page (data+spare) */
    #define PAGE_DATA_SIZE          2048            /* Data bytes per page */
    #define PAGE_SPARE_SIZE         128             /* Spare bytes per page */

    #define BAD_BLOCK_BYTE          PAGE_DATA_SIZE
    #define BAD_BLOCK_VALUE         0x00

    /*
     * Explanation:
     * Without spare areas:
     *   1 page = 2048 bytes; 1 block = 2048*64 = 131072 bytes; device = 131072*2048 = 268435456 bytes (256 MB)
     * With spare areas:
     *   1 page = 2176 bytes; 1 block = 2176*64 = 139264 bytes; device = 139264*2048 = 285212672 bytes (~272 MB)
     */

    /* Addressing definitions: Logical addresses are 32 bits covering FLASH_SIZE_BYTES */
    typedef uint32_t NAND_Addr;

    /* Bit counts per address field according to datasheet page 11 */
    #define ROW_ADDRESS_BLOCK_BITS   11
    #define ROW_ADDRESS_PAGE_BITS    6
    #define ROW_ADDRESS_BITS         24
    #define COL_ADDRESS_BITS         12
    /* Structure for physical addressing within NAND flash */
    typedef struct {
        uint16_t plane       : 1;                       // Plane number (1 bit)
        uint16_t block       : ROW_ADDRESS_BLOCK_BITS;  // Block number (11 bits)
        uint16_t page        : ROW_ADDRESS_PAGE_BITS;   // Page number (6 bits)
        uint32_t rowAddr     : ROW_ADDRESS_BITS;        // Combined row address (24 bits)
        uint32_t colAddr     : COL_ADDRESS_BITS;        // Column (offset) address within page (12 bits)
    } PhysicalAddrs;

    /* Macros to extract parts of the logical address */
    #define ADDRESS_2_BLOCK(Address)    ((uint16_t)((Address) >> 17))      // Divide by 131072 bytes per block
    #define ADDRESS_2_PLANE(Address)    (ADDRESS_2_BLOCK(Address) & 0x1)     // The lowest bit of the block number
    #define ADDRESS_2_PAGE(Address)     ((uint16_t)(((Address) >> 11) & 0x3F))
    #define ADDRESS_2_COL(Address)      ((uint32_t)((Address) & 0x07FF))     // Lower 11 bits

    /* Macro to check the Operation In Progress (OIP) bit in Status Register */
    #define CHECK_OIP(status_reg)       ((status_reg) & SPI_NAND_OIP)

    /* Command Code Definitions per datasheet page 13 */
    typedef enum {
        SPI_NAND_RESET                  = 0xFF,
        SPI_NAND_GET_FEATURES           = 0x0F,
        SPI_NAND_SET_FEATURES           = 0x1F,
        SPI_NAND_READ_ID                = 0x9F,
        SPI_NAND_PAGE_READ              = 0x13,
        SPI_NAND_READ_PAGE_CACHE_RANDOM = 0x30,
        SPI_NAND_READ_PAGE_CACHE_LAST   = 0x3F,
        SPI_NAND_READ_CACHE_X1          = 0x03,
        SPI_NAND_READ_CACHE_X2          = 0x3B, // dual I/O mode
        SPI_NAND_READ_CACHE_X4          = 0x6B, // quad I/O mode
        SPI_NAND_READ_CACHE_DUAL_IO     = 0xBB,
        SPI_NAND_READ_CACHE_QUAD_IO     = 0xEB,
        SPI_NAND_WRITE_ENABLE           = 0x06,
        SPI_NAND_WRITE_DISABLE          = 0x04,
        SPI_NAND_BLOCK_ERASE            = 0xD8,
        SPI_NAND_PROGRAM_EXEC           = 0x10,
        SPI_NAND_PROGRAM_LOAD_X1        = 0x02,
        SPI_NAND_PROGRAM_LOAD_X4        = 0x32,
        SPI_NAND_PROGRAM_LOAD_RANDOM_X1 = 0x84,
        SPI_NAND_PROGRAM_LOAD_RANDOM_X4 = 0x34,
        SPI_NAND_PERMANENT_BLK_LOCK     = 0x2C
    } CommandCodes;

    /* Register address definitions for Get/Set Feature commands (datasheet page 37) */
    typedef enum {
        SPI_NAND_BLKLOCK_REG_ADDR = 0xA0,
        SPI_NAND_CFG_REG_ADDR     = 0xB0,
        SPI_NAND_STATUS_REG_ADDR  = 0xC0,
        SPI_NAND_DIE_SEL_REG_ADDR = 0xD0,
    } RegisterAddr;

    /* Internal time constants (in ms) used in NAND commands */
    #define T_POR           2  /* Power on reset wait time: minimum 1.25 ms, rounded up */
    #define TIME_MAX_ERS    0  /* Optional maximum erase time */
    #define TIME_MAX_PGM    0  /* Optional maximum program time */

#endif  // MT29F2G01ABAGD

/******************************************************************************
 *                            Internal Functions Prototypes
 *****************************************************************************/
NAND_SPI_ReturnType __write_enable(SPI_HandleTypeDef *hspi);
NAND_SPI_ReturnType __write_disable(SPI_HandleTypeDef *hspi);

/******************************************************************************
 *                            List of APIs
 *****************************************************************************/
/* Status operations */
NAND_ReturnType NAND_Reset(SPI_HandleTypeDef *hspi);
NAND_ReturnType NAND_Wait_Until_Ready(SPI_HandleTypeDef *hspi);

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
