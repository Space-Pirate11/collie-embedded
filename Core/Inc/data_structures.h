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
    BMS_CHG_UNKNOWN,        // Initial or indeterminate state
    BMS_CHG_NOT_CHARGING,   // Includes charge disabled or completed (no top-off)
    BMS_CHG_PRECHARGE,      // Pre-charge phase (though BQ25628 might report this as CC)
    BMS_CHG_FASTCHARGE,     // Constant Current (CC) phase
    BMS_CHG_TAPER,          // Constant Voltage (CV) or Taper phase
    BMS_CHG_TOP_OFF,        // Optional Top-off timer active after CV phase
    BMS_CHG_CHARGE_DONE     // Charge cycle fully complete (after CV/Top-off) - Note: BQ25628 might report 'Not Charging' (00) after termination. Distinguish based on termination flags/current if needed.
} bms_charge_state_t;

typedef struct {
    float battery_voltage_mv;
    float vbus_voltage_mv;
    float system_voltage_mv;
    float charge_current_ma;    // Measured battery current (IBAT_ADC) - positive for charge, negative for discharge
    bms_charge_state_t charge_status; // Derived from I2C registers (REG0x1E primarily)
    uint8_t fault_flags;        // Raw fault conditions from I2C register REG0x1F
    bool data_valid;            // Flag indicating if the status is valid/recent
    bool interrupt_active;      // Flag indicating if BMS interrupt triggered the last update
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
