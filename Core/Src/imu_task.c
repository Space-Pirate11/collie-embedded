/* Core/Src/imu_task.c */

#include "imu_task.h"
#include "bmi270_port.h" // Include the port file for interface init
#include "bmi270.h"      // BMI270 specific definitions
#include "main.h"        // For HAL_GetTick() and potentially error handling
#include <stdio.h>       // For printf
#include <string.h>      // For memset

// --- Configuration ---
#define IMU_POLL_INTERVAL_MS (1000 / 100) // 100 Hz = 10ms interval

// --- Local Variables ---
// Correct struct type for getting sensor data
static struct bmi2_sens_data sensor_data = { { 0 } }; // Use {{0}} or specific init for nested struct
static imu_data_t last_valid_data = { 0 };
static bool imu_initialized = false;
static uint32_t last_poll_time = 0;
static struct bmi2_dev *s_bmi_dev; // Pointer to the device structure passed during init


// --- Function Definitions ---

/**
 * @brief Initializes the BMI270 sensor and its driver interface.
 */
bool imu_task_init(struct bmi2_dev *bmi_dev_ptr) {
    int8_t rslt;

    if (bmi_dev_ptr == NULL) {
        printf("IMU Error: bmi_dev_ptr is NULL in init\r\n");
        return false;
    }
    s_bmi_dev = bmi_dev_ptr; // Store pointer locally

    // Initialize the platform interface (I2C, delay)
    rslt = bmi2_interface_init(s_bmi_dev);
    if (rslt != BMI2_OK) {
        printf("IMU Error: bmi2_interface_init failed with code %d\r\n", rslt);
        return false;
    }

    // Initialize BMI270 driver structure
    // This function loads the config file into the sensor RAM.
    // Ensure the bmi270 driver source includes the config file data (e.g., bmi270_config_file.h)
    rslt = bmi270_init(s_bmi_dev);
    if (rslt != BMI2_OK) {
        printf("IMU Error: bmi270_init failed with code %d\r\n", rslt);
        // Potentially add a retry mechanism here
        return false;
    }
    printf("IMU Info: BMI270 Init OK. Chip ID: 0x%X\r\n", s_bmi_dev->chip_id);


    // --- Configure Sensor ---
    // Array to configure accelerometer and gyroscope sensor
    struct bmi2_sens_config sens_cfg[2];

    // Configure Accelerometer
    sens_cfg[0].type = BMI2_ACCEL;
    sens_cfg[0].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;      // Bandwidth parameter (Normal mode)
    sens_cfg[0].cfg.acc.odr = BMI2_ACC_ODR_100HZ;        // Output Data Rate (100 Hz)
    sens_cfg[0].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE; // Filter performance (Power optimized)
    sens_cfg[0].cfg.acc.range = BMI2_ACC_RANGE_4G;       // Range (+/- 4G) - Adjust as needed

    // Configure Gyroscope
    sens_cfg[1].type = BMI2_GYRO;
    sens_cfg[1].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE; // Filter performance (Power optimized)
    sens_cfg[1].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;     // Bandwidth parameter (Normal mode)
    sens_cfg[1].cfg.gyr.odr = BMI2_GYR_ODR_100HZ;        // Output Data Rate (100 Hz)
    sens_cfg[1].cfg.gyr.range = BMI2_GYR_RANGE_1000;     // Range (+/- 1000 dps) - Adjust as needed
    sens_cfg[1].cfg.gyr.ois_range = BMI2_GYR_OIS_2000;    // OIS Range (Not used here, but required by driver config struct)
    sens_cfg[1].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE; // Noise performance (Power optimized)


    // Set the sensor configurations
    rslt = bmi2_set_sensor_config(sens_cfg, 2, s_bmi_dev);
    if (rslt != BMI2_OK) {
        printf("IMU Error: bmi2_set_sensor_config failed with code %d\r\n", rslt);
        return false;
    }
    printf("IMU Info: Sensor Config Set OK.\r\n");

    // Enable the sensors
    uint8_t sens_list[] = { BMI2_ACCEL, BMI2_GYRO };
    rslt = bmi2_sensor_enable(sens_list, 2, s_bmi_dev);
     if (rslt != BMI2_OK) {
        printf("IMU Error: bmi2_sensor_enable failed with code %d\r\n", rslt);
        return false;
    }
    printf("IMU Info: Sensors Enabled OK.\r\n");

    imu_initialized = true;
    last_poll_time = HAL_GetTick();
    memset(&last_valid_data, 0, sizeof(imu_data_t)); // Clear initial data

    return true;
}

/**
 * @brief Performs a single run/update cycle for the IMU task.
 */
bool imu_task_run(struct bmi2_dev *bmi_dev_ptr, imu_data_t *p_imu_data) {
    int8_t rslt;
    uint8_t status_reg = 0; // Renamed from 'status' to avoid confusion
    bool new_data_read = false;

    if (!imu_initialized || bmi_dev_ptr == NULL || p_imu_data == NULL) {
        return false;
    }

    // Simple time-based polling
    uint32_t current_time = HAL_GetTick();
    if ((current_time - last_poll_time) >= IMU_POLL_INTERVAL_MS) {
        last_poll_time = current_time;

        // Check data ready status register (recommended)
        rslt = bmi2_get_status(&status_reg, s_bmi_dev);
        if (rslt == BMI2_OK) {
            // Check if both accel and gyro data are ready (bits 7 and 6 in status register)
            if ((status_reg & BMI2_DRDY_ACC) && (status_reg & BMI2_DRDY_GYR)) {

                // Get sensor data for Accel and Gyro
                // Correct function call: bmi2_get_sensor_data(struct bmi2_sens_data *data, struct bmi2_dev *dev);
                rslt = bmi2_get_sensor_data(&sensor_data, s_bmi_dev);

                if (rslt == BMI2_OK) {
                    // Data struct already contains accel and gyro if enabled.
                    // The driver might have internal status flags per sensor in the struct,
                    // but the main check is the status register read previously.

                    // Copy data to the output structure
                    p_imu_data->accel_x = sensor_data.acc.x;
                    p_imu_data->accel_y = sensor_data.acc.y;
                    p_imu_data->accel_z = sensor_data.acc.z;
                    p_imu_data->gyro_x = sensor_data.gyr.x;
                    p_imu_data->gyro_y = sensor_data.gyr.y;
                    p_imu_data->gyro_z = sensor_data.gyr.z;
                    p_imu_data->sensor_time = sensor_data.sens_time; // Use sens_time

                    // Store locally as last valid data
                    memcpy(&last_valid_data, p_imu_data, sizeof(imu_data_t));
                    new_data_read = true;

                    // --- Debug Print ---
                    // printf("IMU: A(%d,%d,%d) G(%d,%d,%d)\r\n",
                    //        p_imu_data->accel_x, p_imu_data->accel_y, p_imu_data->accel_z,
                    //        p_imu_data->gyro_x, p_imu_data->gyro_y, p_imu_data->gyro_z);
                    // ---------------

                } else {
                     printf("IMU Warn: bmi2_get_sensor_data failed (rslt=%d)\r\n", rslt);
                }
            } else {
                 // Data not ready according to status register
                 // printf("IMU Info: Data not ready (Status=0x%X)\r\n", status_reg);
            }
        } else {
            printf("IMU Error: bmi2_get_status failed (%d)\r\n", rslt);
            // Consider re-initialization or error state handling
        }
    }

    return new_data_read;
}


/**
 * @brief Get the last successfully read IMU data.
 */
void imu_task_get_last_data(imu_data_t *p_imu_data) {
    if (p_imu_data != NULL) {
        memcpy(p_imu_data, &last_valid_data, sizeof(imu_data_t));
    }
}
