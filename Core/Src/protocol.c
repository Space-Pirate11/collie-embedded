/* Collie_V0/Core/Src/protocol.c */
#include "app_usbx_device.h" // Include first for core USBX types
#include "protocol.h"

/* Use USBX CDC ACM instead of the old USBD CDC interface */
#include "ux_api.h" // Include for UX_SUCCESS etc.
#include "ux_device_class_cdc_acm.h"
#include "ux_device_descriptors.h" // Corrected include name
#include "wifi_ble.h"
#include "main.h" // Include main for HAL handles like hspi1 if ENABLE_BLE is used
#include <stdio.h>
#include <string.h>

/* Global flag for live streaming defined in main.c */
extern volatile uint8_t liveStreamingEnabled;

/* Temporary buffer for transmissions */
static char txBuffer[256];

/* The CDC ACM instance pointer - declared extern in ux_device_cdc_acm.h */
extern UX_SLAVE_CLASS_CDC_ACM *cdc_acm_instance;

// SPI handle needed if BLE is enabled
#ifdef ENABLE_BLE
extern SPI_HandleTypeDef hspi1; // Declared extern in main.h
#endif

/**
  * @brief  Initialize protocol-specific settings.
  */
void Protocol_Init(void) {
    /* No special initialization is needed right now. */
}

/**
  * @brief  Format and transmit IMU data over USBX CDC (and optionally BLE).
  * CSV Format: "IMU,timestamp,ax,ay,az,gx,gy,gz\r\n"
  * @param  data: Pointer to a BMI270_Data structure.
  */
void Protocol_SendIMU(const BMI270_Data *data) {
    UINT status;
    ULONG actual_length; // Needed for _run function

    /* Check if CDC instance is valid */
    if (cdc_acm_instance == UX_NULL) {
        return; // Or handle error appropriately
    }

    /* Format the CSV line using snprintf */
    int len = snprintf(txBuffer, sizeof(txBuffer), "IMU,%lu,%d,%d,%d,%d,%d,%d\r\n",
                       (unsigned long)data->timestamp, data->ax, data->ay, data->az, // Cast timestamp
                       data->gx, data->gy, data->gz);

    // Ensure snprintf was successful and string fits buffer
    if (len < 0 || len >= sizeof(txBuffer)) {
        // Handle formatting error
        return;
    }

    /* Transmit the data using USBX CDC ACM write_run function */
    status = ux_device_class_cdc_acm_write_run(cdc_acm_instance,
                                                (UCHAR*)txBuffer, // Cast to UCHAR*
                                                (ULONG)len,       // Use actual formatted length
                                                &actual_length); // Pass actual length pointer

    /* Check only for non-success errors */
    if (status != UX_SUCCESS)
    {
        // Optional: add error handling here.
        // Error_Handler();
    }

#ifdef ENABLE_BLE
    /* If BLE is enabled, also transmit over BLE. */
    // Check if hspi1 handle is valid before using
    if (WiFiBLE_SendData(&hspi1, (uint8_t*)txBuffer, (uint16_t)len) != HAL_OK) {
         // Handle BLE transmit error
    }
#endif
}

/**
  * @brief  Format and transmit environmental data over USBX CDC (and optionally BLE).
  * CSV Format: "ENV,timestamp,temperature,battery,lat,lon,fixQuality,sats\r\n"
  * @param  temperature: Temperature value (°C).
  * @param  battery_voltage: Battery voltage (V).
  * @param  gpsFix: Pointer to a GPS_Fix_t structure.
  * @param  timestamp: Current timestamp.
  */
void Protocol_SendEnvironmental(float temperature, float battery_voltage, const GPS_Fix_t *gpsFix, uint32_t timestamp) {
    UINT status;
    ULONG actual_length; // Needed for _run function
    char lat_str[16] = "0.000000";
    char lon_str[16] = "0.000000";
    uint8_t fix_quality = 0;
    uint8_t num_sats = 0;

    /* Check if CDC instance is valid */
    if (cdc_acm_instance == UX_NULL) {
        return; // Or handle error appropriately
    }

    // Safely access gpsFix members only if pointer is not NULL
    if (gpsFix != NULL) {
        fix_quality = gpsFix->fix_quality;
        num_sats = gpsFix->num_sats;
        if (fix_quality > 0) { // Only format lat/lon if fix is valid
            snprintf(lat_str, sizeof(lat_str), "%.6f", gpsFix->latitude);
            snprintf(lon_str, sizeof(lon_str), "%.6f", gpsFix->longitude);
        }
    }

    int len = snprintf(txBuffer, sizeof(txBuffer), "ENV,%lu,%.2f,%.3f,%s,%s,%d,%d\r\n",
                       (unsigned long)timestamp, temperature, battery_voltage, // Cast timestamp
                       lat_str, lon_str,
                       fix_quality,
                       num_sats);

    // Ensure snprintf was successful and string fits buffer
    if (len < 0 || len >= sizeof(txBuffer)) {
        // Handle formatting error
        return;
    }

    /* Transmit the data using USBX CDC ACM write_run function */
    status = ux_device_class_cdc_acm_write_run(cdc_acm_instance,
                                                (UCHAR*)txBuffer, // Cast to UCHAR*
                                                (ULONG)len,       // Use actual formatted length
                                                &actual_length); // Pass actual length pointer

    /* Check only for non-success errors */
    if (status != UX_SUCCESS)
    {
        // Optional: add error handling here.
    }

#ifdef ENABLE_BLE
    // Check if hspi1 handle is valid before using
    if (WiFiBLE_SendData(&hspi1, (uint8_t*)txBuffer, (uint16_t)len) != HAL_OK) {
        // Handle BLE transmit error
    }
#endif
}

/**
  * @brief  Format a combined CSV log line for NAND flash logging.
  * Format: "timestamp,ax,ay,az,gx,gy,gz,temperature,battery,lat,lon,numSats\r\n"
  * @param  imuData: Pointer to a BMI270_Data structure.
  * @param  temperature: Temperature value (°C).
  * @param  battery_voltage: Battery voltage (V).
  * @param  gpsFix: Pointer to a GPS_Fix_t structure.
  * @param  timestamp: Timestamp for log entry.
  * @param  outBuffer: Output buffer for the log line.
  * @param  bufSize: Size of the output buffer.
  */
void Protocol_LogData(const BMI270_Data *imuData, float temperature, float battery_voltage,
                        const GPS_Fix_t *gpsFix, uint32_t timestamp,
                        char *outBuffer, size_t bufSize) {
    char lat_str[16] = "0.000000";
    char lon_str[16] = "0.000000";
    uint8_t num_sats = 0;
    uint8_t fix_quality = 0;

    // Ensure imuData and outBuffer are not NULL before accessing members/writing
    if (!imuData || !outBuffer || bufSize == 0) return;

    // Safely access gpsFix members
    if (gpsFix != NULL) {
         fix_quality = gpsFix->fix_quality;
         num_sats = gpsFix->num_sats;
        if (fix_quality > 0) { // Only format lat/lon if fix is valid
            snprintf(lat_str, sizeof(lat_str), "%.6f", gpsFix->latitude);
            snprintf(lon_str, sizeof(lon_str), "%.6f", gpsFix->longitude);
        }
    }

    snprintf(outBuffer, bufSize,
             "%lu,%d,%d,%d,%d,%d,%d,%.2f,%.3f,%s,%s,%d\r\n",
             (unsigned long)timestamp, // Cast timestamp
             imuData->ax, imuData->ay, imuData->az,
             imuData->gx, imuData->gy, imuData->gz,
             temperature, battery_voltage,
             lat_str, lon_str,
             num_sats); // Log num_sats regardless of fix for consistency? Or only if fix_quality > 0? Currently logs always.
}

/**
  * @brief  Process incoming commands from the host.
  */
void Protocol_ProcessIncoming(void) {
    UCHAR read_buffer[64];
    ULONG actual_length = 0;
    UINT status;

    if (cdc_acm_instance != UX_NULL) {
        // Use the standard 4-argument read_run function
        status = ux_device_class_cdc_acm_read_run(cdc_acm_instance, read_buffer, sizeof(read_buffer) - 1, &actual_length);

        if (status == UX_SUCCESS && actual_length > 0) {
            read_buffer[actual_length] = '\0'; // Null-terminate
            // Process command in read_buffer
            if (strncmp((char*)read_buffer, "STREAM OFF", 10) == 0) {
                liveStreamingEnabled = 0;
            } else if (strncmp((char*)read_buffer, "STREAM ON", 9) == 0) {
                liveStreamingEnabled = 1;
            }
            // Add other commands as needed (e.g., ERASE NAND, CONFIG WIFI, etc.)
        }
         /* Check only for non-success errors */
         else if (status != UX_SUCCESS) {
            // Handle read error (optional)
            // Example: log error, reset state, etc.
        }
    }
}
