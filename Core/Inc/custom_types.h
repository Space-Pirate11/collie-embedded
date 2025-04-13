#ifndef INC_CUSTOM_TYPES_H_
#define INC_CUSTOM_TYPES_H_

#include <stdint.h> // Include for standard types like int16_t, uint32_t, uint8_t

// Structure to hold IMU data and timestamp
typedef struct {
    int16_t ax;         // Accelerometer X-axis
    int16_t ay;         // Accelerometer Y-axis
    int16_t az;         // Accelerometer Z-axis
    int16_t gx;         // Gyroscope X-axis
    int16_t gy;         // Gyroscope Y-axis
    int16_t gz;         // Gyroscope Z-axis
    uint32_t timestamp; // Timestamp of the reading (e.g., HAL_GetTick())
} BMI270_Data; // Renamed from BMI2_Data to match usage in protocol.h/main.c

// Structure to hold GPS fix data
typedef struct {
    float latitude;     // Latitude in degrees
    float longitude;    // Longitude in degrees
    uint8_t fix_quality;// Fix quality (e.g., 0=No Fix, 1=GPS Fix, 2=DGPS Fix)
    uint8_t num_sats;   // Number of satellites used in fix
} GPS_Fix_t; // Corrected members

#endif // INC_CUSTOM_TYPES_H_
