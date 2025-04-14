/* Core/Inc/imu_task.h */

#ifndef INC_IMU_TASK_H_
#define INC_IMU_TASK_H_

#include <stdbool.h>
#include "bmi2.h"           // BMI2 driver definitions
#include "data_structures.h" // Common data structures

/**
 * @brief Initializes the BMI270 sensor and its driver interface.
 * @param bmi_dev_ptr Pointer to the BMI270 device structure.
 * @return true on success, false on failure.
 */
bool imu_task_init(struct bmi2_dev *bmi_dev_ptr);

/**
 * @brief Performs a single run/update cycle for the IMU task.
 * Reads sensor data if available based on polling rate.
 * @param bmi_dev_ptr Pointer to the BMI270 device structure.
 * @param p_imu_data Pointer to the structure where latest IMU data will be stored.
 * @return true if new data was read, false otherwise.
 */
bool imu_task_run(struct bmi2_dev *bmi_dev_ptr, imu_data_t *p_imu_data);

/**
 * @brief Get the last successfully read IMU data.
 * @param p_imu_data Pointer to copy the last data into.
 */
void imu_task_get_last_data(imu_data_t *p_imu_data);


#endif /* INC_IMU_TASK_H_ */
