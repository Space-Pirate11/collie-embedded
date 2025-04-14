/* Core/Src/storage_task.c */

#include "storage_task.h"
#include "nand_m79a_lld.h" // Use LLD header provided by user
#include "main.h"          // For HAL handles, HAL_Delay()
#include <stdio.h>         // For printf
#include <string.h>        // For memset, memcpy

// --- Configuration ---
#define LOG_STORAGE_START_BLOCK 1   // Avoid block 0 usually
// Use NUM_BLOCKS definition from nand_m79a_lld.h (or define locally if missing)
#ifndef NUM_BLOCKS
#define NUM_BLOCKS 2048 // Default if not in header
#endif
#define LOG_STORAGE_END_BLOCK   (NUM_BLOCKS - 1)

// --- HAL Handle Extern Declarations ---
extern SPI_HandleTypeDef hspi1;
// --------------------------------------

// --- Local Variables ---
static bool storage_initialized = false;

// Log State
static uint32_t current_log_block_idx = LOG_STORAGE_START_BLOCK;
static uint32_t current_log_page_idx = 0;
static bool current_block_needs_erase = true;

// --- Helper Function ---
static void fill_physical_address(uint32_t block_idx, uint32_t page_idx, PhysicalAddrs *pAddr) {
    if (!pAddr) return;
    pAddr->block = (uint16_t)block_idx;
    pAddr->page = (uint16_t)page_idx;
    pAddr->rowAddr = ((block_idx & 0x7FF) << ROW_ADDRESS_PAGE_BITS) | (page_idx & 0x3F);
    pAddr->colAddr = 0; // Default to start of data area
    pAddr->plane = (block_idx >> ROW_ADDRESS_BLOCK_BITS) & 1;
}

static bool find_next_available_page(void) {
    NAND_ReturnType status_lld;
    uint8_t page_buf[4];
    PhysicalAddrs currentAddr;

    printf("STORAGE: Searching for next available page...\r\n");
    for (uint32_t block = LOG_STORAGE_START_BLOCK; block <= LOG_STORAGE_END_BLOCK; block++) {
        for (uint32_t page = 0; page < NUM_PAGES_PER_BLOCK; page++) {
             fill_physical_address(block, page, &currentAddr);
             currentAddr.colAddr = 0;
             // Use NAND_Page_Read from LLD
             status_lld = NAND_Page_Read(&hspi1, &currentAddr, page_buf, 1);
             if (status_lld != Ret_Success) {
                  printf("STORAGE Error: Read fail B%lu,P%lu (%d)\r\n", block, page, status_lld);
                  break;
             }
             if (page_buf[0] == 0xFF) {
                 current_log_block_idx = block;
                 current_log_page_idx = page;
                 current_block_needs_erase = (page == 0);
                 printf("STORAGE: Next empty page: B%lu, P%lu\r\n", block, page);
                 return true;
             }
        }
    }
    printf("STORAGE Error: Flash full or no empty page found.\r\n");
    return false;
}

static bool erase_current_block(void) {
    NAND_ReturnType status_lld;
    PhysicalAddrs currentAddr;
    fill_physical_address(current_log_block_idx, 0, &currentAddr);

    printf("STORAGE: Erasing block %lu...\r\n", current_log_block_idx);
    status_lld = NAND_Block_Erase(&hspi1, &currentAddr); // Use LLD function
    if (status_lld == Ret_Success) {
        printf("STORAGE: Block %lu erased.\r\n", current_log_block_idx);
        current_block_needs_erase = false;
        current_log_page_idx = 0;
        return true;
    } else {
         printf("STORAGE Error: Erase B%lu failed (%d)\r\n", current_log_block_idx, status_lld);
         return false;
    }
}

// --- Function Definitions ---
bool storage_task_init(void) {
    NAND_ReturnType status_lld;
    NAND_ID dev_ID;

    // SPI Port init happens in main.c via MX_SPI1_Init()

    printf("STORAGE: Resetting NAND...\r\n");
    status_lld = NAND_Reset(&hspi1); // Use LLD function
    if (status_lld != Ret_Success) {
        printf("STORAGE Error: NAND_Reset failed (%d)\r\n", status_lld);
        return false;
    }

    printf("STORAGE: Reading NAND ID...\r\n");
    status_lld = NAND_Read_ID(&hspi1, &dev_ID); // Use LLD function
     if (status_lld == Ret_Success) {
         printf("STORAGE Info: NAND Manuf ID: 0x%02X, Device ID: 0x%02X\r\n",
                dev_ID.manufacturer_ID, dev_ID.device_ID);
         if (dev_ID.manufacturer_ID != NAND_ID_MANUFACTURER || dev_ID.device_ID != NAND_ID_DEVICE) {
              printf("STORAGE Warning: Unexpected JEDEC ID!\r\n");
         }
     } else {
         printf("STORAGE Error: Failed to read NAND JEDEC ID (%d)\r\n", status_lld);
         return false;
     }

    if (!find_next_available_page()) {
        printf("STORAGE Warning: Could not find empty page. Needs format?\r\n");
        // Consider formatting or returning error
        // return false;
    }

    storage_initialized = true;
    printf("STORAGE Info: Task Initialized. Logging to start at B%lu, P%lu\r\n", current_log_block_idx, current_log_page_idx);
    return true;
}

bool storage_task_write_record(const log_record_t *record) {
    NAND_ReturnType status_lld;
    PhysicalAddrs currentAddr;
    uint8_t write_buf[PAGE_SIZE]; // Use definition from nand_m79a_lld.h

    if (!storage_initialized || record == NULL) return false;
    if (current_log_block_idx > LOG_STORAGE_END_BLOCK) return false; // Flash full

    if (current_log_page_idx == 0 && current_block_needs_erase) {
        if (!erase_current_block()) {
            printf("STORAGE Info: Erase failed, skipping B%lu.\r\n", current_log_block_idx);
            current_log_block_idx++;
            current_log_page_idx = 0;
            current_block_needs_erase = true;
            if (current_log_block_idx > LOG_STORAGE_END_BLOCK) { return false; } // Full
            printf("STORAGE Info: Trying next block %lu.\r\n", current_log_block_idx);
            return storage_task_write_record(record); // Retry
        }
    }

    memset(write_buf, 0xFF, PAGE_SIZE);
    if (sizeof(log_record_t) > PAGE_DATA_SIZE) { // Check against DATA size
         printf("STORAGE Error: Record size (%u) > page data size (%u)!\r\n",
                (unsigned int)sizeof(log_record_t), PAGE_DATA_SIZE);
         return false;
    }
    memcpy(write_buf, record, sizeof(log_record_t));

    fill_physical_address(current_log_block_idx, current_log_page_idx, &currentAddr);
    currentAddr.colAddr = 0;

    // Use NAND_Page_Program from LLD
    // Program only the record size for efficiency? Check if driver/HW supports partial page program.
    // For simplicity and safety, programming the whole data area (PAGE_DATA_SIZE) is often done.
    // Let's program only the record size for now.
    status_lld = NAND_Page_Program(&hspi1, &currentAddr, write_buf, sizeof(log_record_t));

    if (status_lld == Ret_Success) {
        current_log_page_idx++;
        current_block_needs_erase = false;
        if (current_log_page_idx >= NUM_PAGES_PER_BLOCK) {
            current_log_block_idx++;
            current_log_page_idx = 0;
            current_block_needs_erase = true;
            printf("STORAGE: Moving to next block: %lu\r\n", current_log_block_idx);
            if (current_log_block_idx > LOG_STORAGE_END_BLOCK) {
                 printf("STORAGE Warning: Reached end of usable flash space.\r\n");
            }
        }
        return true;
    } else {
        printf("STORAGE Error: Program B%lu,P%lu failed (%d).\r\n", current_log_block_idx, current_log_page_idx, status_lld);
        // Basic bad block handling: skip to next block
        current_log_block_idx++;
        current_log_page_idx = 0;
        current_block_needs_erase = true;
        if (current_log_block_idx > LOG_STORAGE_END_BLOCK) {
            printf("STORAGE Error: Flash full after program failure.\r\n");
        }
        return false;
    }
}

void storage_task_run(void) { /* No periodic action */ }

bool storage_task_format(void) {
    printf("STORAGE WARNING: Formatting NAND Flash! Erasing blocks %d to %d...\r\n",
           LOG_STORAGE_START_BLOCK, LOG_STORAGE_END_BLOCK);
    NAND_ReturnType status_lld;
    PhysicalAddrs currentAddr;
    bool all_erased = true;

    for (uint32_t block = LOG_STORAGE_START_BLOCK; block <= LOG_STORAGE_END_BLOCK; block++) {
         printf("STORAGE: Erasing block %lu...\r\n", block);
         fill_physical_address(block, 0, &currentAddr);
         status_lld = NAND_Block_Erase(&hspi1, &currentAddr); // Use LLD function
         if (status_lld != Ret_Success) {
              printf("STORAGE Error: Failed to erase block %lu during format (%d).\r\n", block, status_lld);
              all_erased = false;
         }
         HAL_Delay(5);
    }

    current_log_block_idx = LOG_STORAGE_START_BLOCK;
    current_log_page_idx = 0;
    current_block_needs_erase = true;

    if (all_erased) { printf("STORAGE: Format completed.\r\n"); return true; }
    else { printf("STORAGE Warning: Format completed with errors.\r\n"); return false; }
}
