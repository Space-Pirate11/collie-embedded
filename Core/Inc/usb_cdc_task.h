/* Core/Inc/usb_cdc_task.h */

#ifndef INC_USB_CDC_TASK_H_
#define INC_USB_CDC_TASK_H_

#include <stdbool.h>
#include <stdint.h>
#include "data_structures.h" // Include common data structures

/**
 * @brief Initializes the USB CDC Task module.
 * (Note: USB peripheral and USBX stack initialization is handled by CubeMX generated code)
 */
void usb_cdc_task_init(void);

/**
 * @brief Performs a single run/update cycle for the USB CDC task.
 * Checks if USB is connected and configured, and if there's data to send.
 */
void usb_cdc_task_run(void);

/**
 * @brief Sends data formatted as a string over the USB CDC VCP.
 * @param data_string Pointer to the null-terminated string to send.
 * @return true if the data was successfully queued for transmission, false otherwise (e.g., buffer full, disconnected).
 */
bool usb_cdc_task_send_string(const char *data_string);

/**
 * @brief Formats and sends the latest sensor data over USB CDC.
 * @param p_imu Pointer to the latest IMU data.
 * @param p_gps Pointer to the latest GPS data.
 * @param p_temp Pointer to the latest Temperature data.
 * @param p_bms Pointer to the latest BMS status.
 * @return true if data was successfully queued, false otherwise.
 */
bool usb_cdc_task_send_sensor_data(const imu_data_t *p_imu,
                                   const gps_data_t *p_gps,
                                   const temp_data_t *p_temp,
                                   const bms_status_t *p_bms);

/**
 * @brief Checks if the USB VCP is connected and ready for transmission.
 * @return true if connected and configured, false otherwise.
 */
bool usb_cdc_task_is_ready(void);

#endif /* INC_USB_CDC_TASK_H_ */
