/* Core/Inc/storage_task.h */

#ifndef INC_STORAGE_TASK_H_
#define INC_STORAGE_TASK_H_

#include <stdbool.h>
#include <stdint.h>
#include "data_structures.h" // Common data structures

// Define NAND Flash properties based on MT29F2G01ABAGDWB datasheet
// These might be better placed in nand_spi_port.h or a dedicated config file
#define NAND_PAGE_SIZE          2176  // Bytes (2048 data + 128 spare)
#define NAND_PAGES_PER_BLOCK    64
#define NAND_BLOCK_SIZE         (NAND_PAGE_SIZE * NAND_PAGES_PER_BLOCK)
#define NAND_NUM_BLOCKS         2048  // (2 planes * 1024 blocks/plane)
#define NAND_DEVICE_SIZE        ((uint64_t)NAND_NUM_BLOCKS * NAND_BLOCK_SIZE)

/**
 * @brief Initializes the NAND Flash memory and the storage task module.
 * This includes initializing the low-level NAND driver.
 * @return true on success, false on failure.
 */
bool storage_task_init(void);

/**
 * @brief Writes a log record to the next available page in NAND flash.
 * Handles page programming, block erasing when necessary, and potentially
 * basic bad block management.
 * @param record Pointer to the log_record_t data to write.
 * @return true on successful write, false on failure.
 */
bool storage_task_write_record(const log_record_t *record);

/**
 * @brief Reads a log record from a specific logical address or index (optional).
 * (Implementation depends on chosen storage strategy - simple append log might not support easy random reads)
 * @param record_index The index or address of the record to read.
 * @param record Pointer to the log_record_t structure to fill with read data.
 * @return true on successful read, false if record not found or error.
 */
// bool storage_task_read_record(uint32_t record_index, log_record_t *record); // Example if needed

/**
 * @brief Performs periodic maintenance tasks for storage, if any (e.g., checking flash status).
 */
void storage_task_run(void);

/**
 * @brief Formats/erases the entire NAND flash (Use with caution!).
 * @return true on success, false on failure.
 */
bool storage_task_format(void);


#endif /* INC_STORAGE_TASK_H_ */
