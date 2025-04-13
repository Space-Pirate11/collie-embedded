/**
  * @file    gps.c
  * @brief   Implementation of the GPS driver for the SAM-M10Q module.
  * Based on the SAM-M10Q Integration Manual (UBX-22020019).
  *
  * This implementation configures UART reception for NMEA sentences, and parses (for example)
  * GGA or RMC sentences to extract latitude, longitude, fix quality, and satellite count.
  * For simplicity, error handling is basic. In production, more robust parsing is advised.
  */

#include "gps.h"
#include "main.h" // Include main header for HAL handles like huart4
#include "custom_types.h" // Include for GPS_Fix_t definition
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Buffer for receiving NMEA sentences */
#define GPS_RX_BUFFER_SIZE 128
static char gps_rx_buffer[GPS_RX_BUFFER_SIZE]; // Complete sentence buffer
static uint8_t gps_rx_index = 0;
static char gps_rx_char; // Single character buffer for UART IT

/* Latest GPS fix - declared static */
static GPS_Fix_t currentGPSFix = {0};
static uint8_t new_fix_available = 0; // Flag to indicate new data

/* Function prototypes for internal parsing */
static void GPS_ParseSentence(const char *sentence);

/* GPS_Init: Initialize GPS reception. */
void GPS_Init(void) {
    /* Clear buffer index */
    gps_rx_index = 0;
    memset(gps_rx_buffer, 0, GPS_RX_BUFFER_SIZE);
    memset(&currentGPSFix, 0, sizeof(GPS_Fix_t));
    new_fix_available = 0;

    /* Start UART reception, 1 byte at a time using interrupt */
    // Ensure huart4 is initialized before calling this
    if (HAL_UART_Receive_IT(&huart4, (uint8_t *)&gps_rx_char, 1) != HAL_OK) {
        // Handle error, maybe call Error_Handler() defined in main.c
        Error_Handler();
    }
}

/* GPS_GetLatestFix: Copy the latest GPS fix if available.
   Returns 1 if a new, valid fix was copied, 0 otherwise.
*/
uint8_t GPS_GetLatestFix(GPS_Fix_t *fix) {
    // Check if the fix pointer is valid
    if (fix == NULL) {
        return 0;
    }

    // Check if a new fix is available and quality is valid (>0)
    if (new_fix_available && currentGPSFix.fix_quality > 0) {
        // Use disable/enable interrupt or a mutex here if RTOS is used
        // For bare-metal, temporarily disable UART IRQ if preemption is a concern
        // HAL_NVIC_DisableIRQ(UART4_IRQn); // Requires UART4_IRQn to be defined

        *fix = currentGPSFix; // Copy the latest fix data
        new_fix_available = 0; // Clear the flag

        // HAL_NVIC_EnableIRQ(UART4_IRQn);
        return 1; // Indicate new data was copied
    }
    return 0; // No new valid data
}

/* GPS_UART_RxCpltCallback: To be called from HAL_UART_RxCpltCallback.
   Processes received byte, assembles sentence, calls parser, and restarts reception.
*/
void GPS_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART4) {
        // Process the received character (gps_rx_char)
        if (gps_rx_char == '\n' || gps_rx_char == '\r') { // Sentence end detected
            if (gps_rx_index > 0) { // Check if buffer has data
                 gps_rx_buffer[gps_rx_index] = '\0'; // Null-terminate the sentence
                 GPS_ParseSentence(gps_rx_buffer); // Parse the complete sentence
            }
            gps_rx_index = 0; // Reset buffer index for the next sentence
            // No need to memset here, index reset handles overwrite
        } else if (gps_rx_char == '$') { // Start of a new sentence detected
             gps_rx_index = 0; // Reset index
             gps_rx_buffer[gps_rx_index++] = gps_rx_char; // Store '$'
        } else {
             // Append character to buffer if space available
            if (gps_rx_index < (GPS_RX_BUFFER_SIZE - 1)) {
                gps_rx_buffer[gps_rx_index++] = gps_rx_char;
            } else {
                // Buffer overflow, reset index (discarding current sentence)
                gps_rx_index = 0;
            }
        }

        // Restart UART reception for the next character
        if (HAL_UART_Receive_IT(&huart4, (uint8_t *)&gps_rx_char, 1) != HAL_OK) {
             // Handle UART reception restart error if necessary
             // Maybe log an error or attempt re-initialization
             Error_Handler(); // Example error handling
        }
    }
}


/* GPS_ParseSentence: Parse a complete NMEA sentence.
   This sample implementation only parses the RMC sentence for basic fix info.
   Example RMC: $GPRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,,,A*6A
*/
static void GPS_ParseSentence(const char *sentence) {
    // Check for $GPRMC sentence type
    if (strncmp(sentence, "$GPRMC", 6) == 0) {
        char copy[GPS_RX_BUFFER_SIZE];
        strncpy(copy, sentence, GPS_RX_BUFFER_SIZE - 1);
        copy[GPS_RX_BUFFER_SIZE - 1] = '\0'; // Ensure null termination

        char *token;
        char *saveptr; // For strtok_r if needed, but simple strtok is ok here if not nested
        int field_index = 0;
        GPS_Fix_t tempFix = {0}; // Temporary structure to hold parsed data

        // Tokenize the sentence by comma
        token = strtok_r(copy, ",", &saveptr); // Use strtok_r for safety if used elsewhere

        while (token != NULL && field_index < 13) { // RMC has up to 13 fields
            switch (field_index) {
                case 2: // Field 2: Status (A=Active/Valid, V=Void)
                    if (token[0] == 'A') {
                        tempFix.fix_quality = 1; // Set basic fix quality if status is 'A'
                    } else {
                        tempFix.fix_quality = 0; // Invalid fix
                        // No need to parse further if fix is invalid
                        goto update_fix; // Skip remaining parsing
                    }
                    break;
                case 3: // Field 3: Latitude (ddmm.mmmm)
                    if (tempFix.fix_quality > 0) {
                        double rawLat = atof(token);
                        int degLat = (int)(rawLat / 100.0);
                        double minLat = rawLat - (degLat * 100.0);
                        tempFix.latitude = degLat + (minLat / 60.0);
                    }
                    break;
                case 4: // Field 4: N/S Indicator
                    if (tempFix.fix_quality > 0 && token[0] == 'S') {
                        tempFix.latitude = -tempFix.latitude;
                    }
                    break;
                case 5: // Field 5: Longitude (dddmm.mmmm)
                    if (tempFix.fix_quality > 0) {
                        double rawLon = atof(token);
                        int degLon = (int)(rawLon / 100.0);
                        double minLon = rawLon - (degLon * 100.0);
                        tempFix.longitude = degLon + (minLon / 60.0);
                    }
                    break;
                case 6: // Field 6: E/W Indicator
                     if (tempFix.fix_quality > 0 && token[0] == 'W') {
                        tempFix.longitude = -tempFix.longitude;
                    }
                    break;
                // Add cases for other fields if needed (e.g., speed, course, date)
                // Field 7: Speed over ground (knots)
                // Field 8: Track angle (degrees true)
                // Field 9: Date (ddmmyy)
            }
            field_index++;
            token = strtok_r(NULL, ",", &saveptr);
        }

update_fix:
        // If fix was valid, update the global structure and set flag
        // Use disable/enable interrupt or mutex here if RTOS is used
        // HAL_NVIC_DisableIRQ(UART4_IRQn);
        if (tempFix.fix_quality > 0) {
             // RMC doesn't contain satellite count, parse GGA for that.
             // We keep the previous satellite count if available or set a default.
             // tempFix.num_sats = currentGPSFix.num_sats > 0 ? currentGPSFix.num_sats : 0; // Example
             currentGPSFix = tempFix; // Update the global fix
             new_fix_available = 1;   // Set the flag
        } else {
            // If fix is invalid, update quality but keep old lat/lon? Or clear all?
            // Let's clear quality and flag, keeping old lat/lon might be confusing.
             currentGPSFix.fix_quality = 0;
             // new_fix_available = 0; // No new valid data
        }
        // HAL_NVIC_EnableIRQ(UART4_IRQn);

    }
    // Add parsing logic for other sentence types like GGA if needed
    // Example: else if (strncmp(sentence, "$GPGGA", 6) == 0) { ... parse GGA ... }
    // GGA sentence provides altitude and number of satellites.
}
