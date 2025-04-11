/**
  * @file    gps.c
  * @brief   Implementation of the GPS driver for the SAM-M10Q module.
  *          Based on the SAM-M10Q Integration Manual (UBX-22020019).
  *
  * This implementation configures UART reception for NMEA sentences, and parses (for example)
  * GGA or RMC sentences to extract latitude, longitude, fix quality, and satellite count.
  * For simplicity, error handling is basic. In production, more robust parsing is advised.
  */

#include "gps.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Buffer for receiving NMEA sentences */
#define GPS_RX_BUFFER_SIZE 128
static char gps_rx_buffer[GPS_RX_BUFFER_SIZE] = {0};
static uint8_t gps_rx_index = 0;

/* Latest GPS fix */
static GPS_Fix_t currentGPSFix = {0};

/* Function prototypes for internal parsing */
static void GPS_ParseSentence(const char *sentence);

/* GPS_Init: Initialize GPS reception.
   For this example, we assume UART4 is used.
   Configure UART interrupts (this is normally done in HAL_UART_MspInit).
*/
void GPS_Init(void) {
    /* Clear buffer index */
    gps_rx_index = 0;
    memset(gps_rx_buffer, 0, GPS_RX_BUFFER_SIZE);
    /* Enable UART receive interrupt using HAL_UART_Receive_IT in your main init */
    HAL_UART_Receive_IT(&huart4, (uint8_t *)&gps_rx_buffer[gps_rx_index], 1);
}

/* GPS_GetLatestFix: Return the latest GPS fix parsed.
   Returns 1 if a valid fix is available, 0 otherwise.
*/
uint8_t GPS_GetLatestFix(GPS_Fix_t *fix) {
    if(currentGPSFix.fix_quality > 0) {
        *fix = currentGPSFix;
        return 1;
    }
    return 0;
}

/* This callback must be called by the UART IRQ handler when a byte is received.
   You can call this from HAL_UART_RxCpltCallback.
*/
void GPS_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == UART4) {
        char received = gps_rx_buffer[gps_rx_index];
        if(received == '\n') {
            gps_rx_buffer[gps_rx_index] = '\0'; // Terminate the sentence
            GPS_ParseSentence(gps_rx_buffer);
            gps_rx_index = 0;
            memset(gps_rx_buffer, 0, GPS_RX_BUFFER_SIZE);
        } else {
            gps_rx_index++;
            if(gps_rx_index >= GPS_RX_BUFFER_SIZE) {
                gps_rx_index = 0;  // Buffer overflow, reset
            }
        }
        HAL_UART_Receive_IT(&huart4, (uint8_t *)&gps_rx_buffer[gps_rx_index], 1);
    }
}

/* GPS_ParseSentence: Parse a complete NMEA sentence.
   This sample implementation only parses the RMC sentence.
   An example RMC sentence:
   $GPRMC,hhmmss.sss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
*/
static void GPS_ParseSentence(const char *sentence) {
    if(strncmp(sentence, "$GPRMC", 6) == 0) {
        // Tokenize the sentence by comma
        char copy[GPS_RX_BUFFER_SIZE];
        strncpy(copy, sentence, GPS_RX_BUFFER_SIZE);
        char *tokens[20] = {0};
        uint8_t i = 0;
        char *token = strtok(copy, ",");
        while(token != NULL && i < 20) {
            tokens[i++] = token;
            token = strtok(NULL, ",");
        }
        // Expected tokens:
        // tokens[2]: Status (A = data valid)
        // tokens[3]: Latitude in ddmm.mmmm
        // tokens[4]: N/S indicator
        // tokens[5]: Longitude in dddmm.mmmm
        // tokens[6]: E/W indicator
        // tokens[7]: Speed (knots, not used)
        // tokens[8]: Course
        // tokens[9]: Date in ddmmyy
        if(i >= 10 && tokens[2] != NULL && tokens[2][0] == 'A') {
            // Parse latitude
            float rawLat = atof(tokens[3]);
            int degLat = (int)(rawLat / 100);
            float minLat = rawLat - (degLat * 100);
            float latitude = degLat + (minLat / 60.0f);
            if(tokens[4][0] == 'S')
                latitude = -latitude;
            // Parse longitude
            float rawLon = atof(tokens[5]);
            int degLon = (int)(rawLon / 100);
            float minLon = rawLon - (degLon * 100);
            float longitude = degLon + (minLon / 60.0f);
            if(tokens[6][0] == 'W')
                longitude = -longitude;
            // For fix quality, we use token[2] ('A' means valid) and set fix_quality = 1.
            currentGPSFix.fix_quality = 1;
            // For number of satellites, typically it's not in RMC; use a default (e.g., 6).
            currentGPSFix.num_sats = 6;
            currentGPSFix.latitude = latitude;
            currentGPSFix.longitude = longitude;
        } else {
            currentGPSFix.fix_quality = 0; // Not valid
        }
    }
}
