/* Core/Inc/gps_task.h */

#ifndef INC_GPS_TASK_H_
#define INC_GPS_TASK_H_

#include <stdbool.h>
#include <stdint.h>
#include "nmea_parse.h"      // Included first - Defines GPS struct type
#include "data_structures.h" // Common data structures

// Size of the circular buffer for incoming UART data
#define GPS_UART_BUFFER_SIZE 256

/**
 * @brief Initializes the GPS task, including UART peripheral for GPS communication.
 * @param gps_ptr Pointer to the GPS state structure (defined in nmea_parse.h).
 * @return true on success, false on failure.
 */
bool gps_task_init(GPS *gps_ptr); // Use GPS type

/**
 * @brief Performs a single run/update cycle for the GPS task.
 * Processes data received in the UART buffer and calls the parser.
 * @param gps_ptr Pointer to the GPS state structure (defined in nmea_parse.h).
 * @param p_gps_data Pointer to the application's gps_data_t structure to be filled.
 */
void gps_task_run(GPS *gps_ptr, gps_data_t *p_gps_data); // Use GPS type

/**
 * @brief Callback function to be called from UART RX Interrupt Handler.
 * Places the received byte into the circular buffer.
 * @param received_byte The byte received via UART.
 */
void gps_task_uart_rx_callback(uint8_t received_byte);

/**
 * @brief Toggles the GPS Reset Pin (PH1).
 * @param assert_reset If true, pull reset low; if false, release reset high.
 */
void gps_task_set_reset(bool assert_reset);

/**
 * @brief Get the last successfully parsed GPS data (from application structure).
 * @param p_gps_data Pointer to copy the last data into.
 */
void gps_task_get_last_data(gps_data_t *p_gps_data);


#endif /* INC_GPS_TASK_H_ */
