/* Core/Inc/bms_task.h */

#ifndef INC_BMS_TASK_H_
#define INC_BMS_TASK_H_

#include <stdbool.h>
#include <stdint.h>
#include "data_structures.h" // Common data structures

// BQ25629 I2C Address (based on datasheet)
#define BQ25629_I2C_ADDR    (0x6B << 1) // STM32 HAL uses 8-bit address

/**
 * @brief Initializes the BMS task (I2C peripheral, GPIOs for INT/CHG/RESET).
 * Configures the BQ25629 with desired charging parameters.
 * @return true on success, false on failure.
 */
bool bms_task_init(void);

/**
 * @brief Performs a single run/update cycle for the BMS task.
 * Reads status registers, checks interrupt/status pins, updates data structure.
 * @param p_bms_status Pointer to the structure where latest BMS status will be stored.
 */
void bms_task_run(bms_status_t *p_bms_status);

/**
 * @brief Callback function to be potentially called from EXTI Interrupt Handler for BMS_INT (PH3).
 * Sets an internal flag indicating the interrupt occurred.
 */
void bms_task_interrupt_callback(void);

/**
 * @brief Toggles the BMS Reset Pin (PA15). (If connected and used)
 * @param assert_reset If true, pull reset high; if false, pull low.
 */
void bms_task_set_reset(bool assert_reset);

/**
 * @brief Reads the state of the BMS Charge Status Pin (PB8).
 * @return true if the pin is high (Charge Done / Not Charging), false if low (Charging).
 */
bool bms_task_read_charge_status_pin(void);

/**
 * @brief Reads a specific register from the BQ25629.
 * @param reg_addr The register address to read.
 * @param data Pointer to store the read byte.
 * @return true on success, false on failure.
 */
bool bms_task_read_register(uint8_t reg_addr, uint8_t *data);

/**
 * @brief Writes a specific register to the BQ25629.
 * @param reg_addr The register address to write.
 * @param data The byte to write.
 * @return true on success, false on failure.
 */
bool bms_task_write_register(uint8_t reg_addr, uint8_t data);

/**
 * @brief Get the last successfully read BMS status.
 * @param p_bms_status Pointer to copy the last status into.
 */
void bms_task_get_last_status(bms_status_t *p_bms_status);

#endif /* INC_BMS_TASK_H_ */
