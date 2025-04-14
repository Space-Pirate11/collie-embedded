/* Core/Src/gps_task.c */

#include "gps_task.h"
#include "main.h" // Required for HAL handles and definitions
#include "stm32u3xx_hal.h"
#include "nmea_parse.h"     // NMEA parser library header - defines GPS struct
#include "system_control.h"
#include <string.h>     // For memcpy, memset, strtok, strlen
#include <stdio.h>      // For printf
#include <stdlib.h>     // For atof, atoi
#include <math.h>       // For floor, isnan, isinf

// --- Configuration ---
#define GPS_POLL_INTERVAL_MS 50 // How often to check buffer (ms)

// Define missing constant if not in nmea_parse.h (adjust size if needed)
#define NMEA_MAX_SENTENCE_LEN 256

// --- HAL Handle Extern Declarations ---
extern UART_HandleTypeDef huart4;
// --------------------------------------

// --- Local Variables ---
static uint8_t gps_uart_byte;              // Single byte received via interrupt/DMA
static uint8_t gps_rx_buffer[GPS_UART_BUFFER_SIZE]; // Circular buffer
static volatile uint16_t gps_rx_write_index = 0;
static volatile uint16_t gps_rx_read_index = 0;
static volatile bool gps_rx_overflow = false;

static char nmea_sentence_buffer[NMEA_MAX_SENTENCE_LEN + 1];
static uint16_t nmea_sentence_index = 0;

static bool gps_initialized = false;

// Pointer to the parser's internal state structure (passed during init)
static GPS *s_gps_parser_state_ptr = NULL; // Use GPS type
// Application's structure to hold processed data
static gps_data_t current_app_gps_data = { .data_valid = false };


// --- Helper Functions ---
static float nmea_to_decimal_degrees(double nmea_coord) { // Use double input
    if (nmea_coord == 0.0 || isnan(nmea_coord) || isinf(nmea_coord)) return 0.0f;
    double degrees = floor(nmea_coord / 100.0);
    double minutes = nmea_coord - (degrees * 100.0);
    return (float)(degrees + (minutes / 60.0));
}

// --- Function Definitions ---
bool gps_task_init(GPS *gps_ptr) { // Use GPS type
    if (gps_ptr == NULL) {
        printf("GPS Error: GPS state pointer is NULL in init\r\n");
        return false;
    }
    s_gps_parser_state_ptr = gps_ptr;
    memset(s_gps_parser_state_ptr, 0, sizeof(GPS)); // Initialize parser state

    gps_rx_read_index = 0;
    gps_rx_write_index = 0;
    gps_rx_overflow = false;
    nmea_sentence_index = 0;
    memset(nmea_sentence_buffer, 0, sizeof(nmea_sentence_buffer));
    memset(&current_app_gps_data, 0, sizeof(gps_data_t));
    current_app_gps_data.data_valid = false;

    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart4, &gps_uart_byte, 1);
    if (status != HAL_OK) {
        printf("GPS Error: HAL_UART_Receive_IT failed (%d)\r\n", status);
        return false;
    }
    gps_task_set_reset(false);
    gps_initialized = true;
    printf("GPS Info: Task Initialized, UART RX IT Started.\r\n");
    return true;
}

void gps_task_uart_rx_callback(uint8_t received_byte) {
    uint16_t next_write_index = (gps_rx_write_index + 1) % GPS_UART_BUFFER_SIZE;
    if (next_write_index == gps_rx_read_index) {
        gps_rx_overflow = true;
    } else {
        gps_rx_buffer[gps_rx_write_index] = received_byte;
        gps_rx_write_index = next_write_index;
        gps_rx_overflow = false;
    }
}

void gps_task_run(GPS *gps_ptr, gps_data_t *p_gps_data) { // Use GPS type
     bool data_updated_this_cycle = false;
     if (!gps_initialized || gps_ptr == NULL || p_gps_data == NULL) {
        if(p_gps_data != NULL) { memcpy(p_gps_data, &current_app_gps_data, sizeof(gps_data_t)); }
        return;
     }
     uint16_t current_write_idx = gps_rx_write_index;
     while (gps_rx_read_index != current_write_idx) {
         uint8_t byte = gps_rx_buffer[gps_rx_read_index];
         gps_rx_read_index = (gps_rx_read_index + 1) % GPS_UART_BUFFER_SIZE;
         if (byte == '$') {
             nmea_sentence_index = 0;
             memset(nmea_sentence_buffer, 0, sizeof(nmea_sentence_buffer));
         } else if (nmea_sentence_index < NMEA_MAX_SENTENCE_LEN) {
             if(byte == '\r' || byte == '\n') {
                 if (nmea_sentence_index > 5) {
                     nmea_sentence_buffer[nmea_sentence_index] = '\0';
                     char temp_sentence[NMEA_MAX_SENTENCE_LEN + 1];
                     strncpy(temp_sentence, nmea_sentence_buffer, NMEA_MAX_SENTENCE_LEN);
                     temp_sentence[NMEA_MAX_SENTENCE_LEN] = '\0';
                     // Pass the buffer starting AFTER the '$' if parser expects that,
                     // otherwise pass the whole buffer including '$'.
                     // The provided nmea_parse uses strtok on '$', so pass the whole buffer.
                     nmea_parse(gps_ptr, (uint8_t*)temp_sentence);
                     data_updated_this_cycle = true;
                 }
                 nmea_sentence_index = 0;
             } else {
                  nmea_sentence_buffer[nmea_sentence_index++] = byte;
             }
         } else { nmea_sentence_index = 0; }
     }
     if (data_updated_this_cycle) {
         current_app_gps_data.latitude = nmea_to_decimal_degrees(gps_ptr->latitude);
         if (gps_ptr->latSide == 'S') { current_app_gps_data.latitude *= -1.0f; }
         current_app_gps_data.longitude = nmea_to_decimal_degrees(gps_ptr->longitude);
         if (gps_ptr->lonSide == 'W') { current_app_gps_data.longitude *= -1.0f; }
         current_app_gps_data.altitude_msl = gps_ptr->altitude;
         current_app_gps_data.fix_quality = gps_ptr->fix;
         current_app_gps_data.satellites_tracked = gps_ptr->satelliteCount;
         current_app_gps_data.speed_knots = 0.0f;
         current_app_gps_data.course_degrees = 0.0f;
         if (strlen(gps_ptr->lastMeasure) >= 6) {
             char hh[3] = { gps_ptr->lastMeasure[0], gps_ptr->lastMeasure[1], '\0'};
             char mm[3] = { gps_ptr->lastMeasure[2], gps_ptr->lastMeasure[3], '\0'};
             char ss[3] = { gps_ptr->lastMeasure[4], gps_ptr->lastMeasure[5], '\0'};
             current_app_gps_data.hour = atoi(hh);
             current_app_gps_data.minute = atoi(mm);
             current_app_gps_data.second = atoi(ss);
             current_app_gps_data.time_valid = true;
         } else { current_app_gps_data.time_valid = false; }
         current_app_gps_data.date_valid = false;
         current_app_gps_data.year = 0;
         current_app_gps_data.month = 0;
         current_app_gps_data.day = 0;
         current_app_gps_data.data_valid = (gps_ptr->fix == 1);
     }
    if (gps_rx_overflow) { printf("GPS Warning: UART RX Buffer Overflow!\r\n"); gps_rx_overflow = false; }
    memcpy(p_gps_data, &current_app_gps_data, sizeof(gps_data_t));
}

void gps_task_set_reset(bool assert_reset) { system_control_set_gps_reset(assert_reset); }
void gps_task_get_last_data(gps_data_t *p_gps_data) { if (p_gps_data != NULL) { memcpy(p_gps_data, &current_app_gps_data, sizeof(gps_data_t)); } }
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) { if (huart->Instance == UART4) { gps_task_uart_rx_callback(gps_uart_byte); if (HAL_UART_Receive_IT(&huart4, &gps_uart_byte, 1) != HAL_OK) { /* Error */ } } }
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) { if (huart->Instance == UART4) { uint32_t error_code = HAL_UART_GetError(huart); printf("GPS UART Error! Code: 0x%lX\r\n", error_code); __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF); HAL_UART_AbortReceive_IT(&huart4); gps_rx_read_index = 0; gps_rx_write_index = 0; gps_rx_overflow = false; nmea_sentence_index = 0; if (HAL_UART_Receive_IT(&huart4, &gps_uart_byte, 1) != HAL_OK) { /* Error */ } } }
