/* Core/Inc/data_structures.h */

#ifndef INC_DATA_STRUCTURES_H_
#define INC_DATA_STRUCTURES_H_

#include <stdint.h>
#include <stdbool.h>

// --- IMU Data ---
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    uint32_t sensor_time; // Optional: BMI270 internal sensor time
} imu_data_t;

// --- GPS Data ---
// Note: NMEA parser might fill a different structure.
// This is a simplified structure to hold processed data.
typedef struct {
    float latitude;       // Degrees North (negative for South)
    float longitude;      // Degrees East (negative for West)
    float altitude_msl;   // Altitude above mean sea level (meters)
    float speed_knots;    // Speed over ground (knots)
    float course_degrees; // Course over ground (degrees)
    uint8_t fix_quality;  // 0=No fix, 1=GPS, 2=DGPS, etc.
    uint8_t satellites_tracked;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool date_valid;
    bool time_valid;
    bool data_valid; // Flag indicating if the overall GPS data is considered valid/recent
} gps_data_t;

// --- Temperature Data ---
typedef struct {
    float temperature_c; // Temperature in degrees Celsius
    bool data_valid;     // Flag indicating if the reading is valid/recent
} temp_data_t;

// --- BMS Status ---
typedef enum {
    BMS_CHG_UNKNOWN,
    BMS_CHG_NOT_CHARGING,
    BMS_CHG_PRECHARGE,
    BMS_CHG_FASTCHARGE,
    BMS_CHG_CHARGE_DONE
} bms_charge_state_t;

typedef struct {
    float battery_voltage_mv;
    float vbus_voltage_mv;
    float system_voltage_mv;
    float charge_current_ma;    // Estimated/measured charge current
    bms_charge_state_t charge_status; // Derived from STAT pin and/or I2C registers
    uint8_t fault_flags;        // Bitmap of fault conditions from I2C registers
    bool data_valid;            // Flag indicating if the status is valid/recent
    bool interrupt_active;      // Flag indicating if BMS interrupt is active
} bms_status_t;


// --- NAND Flash Log Record ---
// Structure defining the data logged to flash at each interval.
// Adjust size/fields as needed for efficiency and requirements.
typedef struct {
    uint32_t timestamp_s;     // System uptime seconds or RTC time
    imu_data_t imu;           // Consider storing raw or scaled/filtered data
    gps_data_t gps;           // Store key GPS fields
    temp_data_t temp;         // Store temperature
    bms_status_t bms;         // Store key BMS fields (e.g., voltage, charge status)
    // Add checksum/CRC if desired
    // uint16_t crc;
} log_record_t;


#endif /* INC_DATA_STRUCTURES_H_ */
