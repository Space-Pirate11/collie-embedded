/* Core/Inc/temp_task.h */

#ifndef INC_TEMP_TASK_H_
#define INC_TEMP_TASK_H_

#include <stdbool.h>
#include "data_structures.h" // Common data structures

/**
 * @brief Initializes the Temperature Sensor task (ADC peripheral).
 * @return true on success, false on failure.
 */
bool temp_task_init(void);

/**
 * @brief Performs a single run/update cycle for the Temperature task.
 * Reads ADC value, converts to temperature, and updates the data structure.
 * @param p_temp_data Pointer to the structure where latest temperature data will be stored.
 * @return true if a new reading was successfully taken, false otherwise.
 */
bool temp_task_run(temp_data_t *p_temp_data);

/**
 * @brief Get the last successfully read Temperature data.
 * @param p_temp_data Pointer to copy the last data into.
 */
void temp_task_get_last_data(temp_data_t *p_temp_data);

#endif /* INC_TEMP_TASK_H_ */
