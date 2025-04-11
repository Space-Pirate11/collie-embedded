#include "protocol.h"

/* Use USBX CDC ACM instead of the old USBD CDC interface */
#include "ux_device_class_cdc_acm.h"
#include "ux_device_descriptors.h"
#include "wifi_ble.h"
#include <stdio.h>
#include <string.h>

/* Global flag for live streaming; if you need it elsewhere, declare it as extern in main.h */
volatile uint8_t liveStreamingEnabled = 1;

/* Temporary buffer for transmissions */
static char txBuffer[256];

/* The CDC ACM instance pointer is created in the USBX device application.
   Make sure that your USBX device init code assigns this variable.
   (For example, in app_usbx_device.c define and initialize a global variable.)
*/
extern UX_SLAVE_CLASS_CDC_ACM *cdc_acm_instance;

/**
  * @brief  Initialize protocol-specific settings.
  */
void Protocol_Init(void) {
    /* No special initialization is needed right now.
       Add any protocol init code if necessary.
    */
}

/**
  * @brief  Format and transmit IMU data over USBX CDC (and optionally BLE).
  *         CSV Format: "IMU,timestamp,ax,ay,az,gx,gy,gz\r\n"
  * @param  data: Pointer to a BMI270_Data structure.
  */
void Protocol_SendIMU(const BMI270_Data *data) {
    /* Format the CSV line using snprintf */
    snprintf(txBuffer, sizeof(txBuffer), "IMU,%lu,%d,%d,%d,%d,%d,%d\r\n",
             data->timestamp, data->ax, data->ay, data->az,
             data->gx, data->gy, data->gz);

    /* Transmit the data using USBX CDC ACM write function */
    UINT status;
    status = ux_device_class_cdc_acm_write(cdc_acm_instance,
                                           (uint8_t*)txBuffer,
                                           (ULONG)strlen(txBuffer),
                                           NULL,
                                           UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {
        // Optional: add error handling here.
    }

#ifdef ENABLE_BLE
    /* If BLE is enabled, also transmit over BLE.
       (Assumes WiFiBLE_SendData is implemented.)
    */
    WiFiBLE_SendData(&hspi1, (uint8_t*)txBuffer, (uint16_t)strlen(txBuffer));
#endif
}

/**
  * @brief  Format and transmit environmental data over USBX CDC (and optionally BLE).
  *         CSV Format: "ENV,timestamp,temperature,battery,lat,lon,fixQuality,sats\r\n"
  * @param  temperature: Temperature value (°C).
  * @param  battery_voltage: Battery voltage (V).
  * @param  gpsFix: Pointer to a GPS_Fix_t structure.
  * @param  timestamp: Current timestamp.
  */
void Protocol_SendEnvironmental(float temperature, float battery_voltage, const GPS_Fix_t *gpsFix, uint32_t timestamp) {
    char lat_str[16] = "0.000000";
    char lon_str[16] = "0.000000";

    if (gpsFix && (gpsFix->fix_quality > 0)) {
        snprintf(lat_str, sizeof(lat_str), "%.6f", gpsFix->latitude);
        snprintf(lon_str, sizeof(lon_str), "%.6f", gpsFix->longitude);
    }
    snprintf(txBuffer, sizeof(txBuffer), "ENV,%lu,%.2f,%.3f,%s,%s,%d,%d\r\n",
             timestamp, temperature, battery_voltage,
             lat_str, lon_str,
             (gpsFix != NULL) ? gpsFix->fix_quality : 0,
             (gpsFix != NULL) ? gpsFix->num_sats : 0);

    UINT status;
    status = ux_device_class_cdc_acm_write(cdc_acm_instance,
                                           (uint8_t*)txBuffer,
                                           (ULONG)strlen(txBuffer),
                                           NULL,
                                           UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {
        // Optional: add error handling here.
    }

#ifdef ENABLE_BLE
    WiFiBLE_SendData(&hspi1, (uint8_t*)txBuffer, (uint16_t)strlen(txBuffer));
#endif
}

/**
  * @brief  Format a combined CSV log line for NAND flash logging.
  *         Format: "timestamp,ax,ay,az,gx,gy,gz,temperature,battery,lat,lon,numSats\r\n"
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

    if (gpsFix && (gpsFix->fix_quality > 0)) {
        snprintf(lat_str, sizeof(lat_str), "%.6f", gpsFix->latitude);
        snprintf(lon_str, sizeof(lon_str), "%.6f", gpsFix->longitude);
    }

    snprintf(outBuffer, bufSize,
             "%lu,%d,%d,%d,%d,%d,%d,%.2f,%.3f,%s,%s,%d\r\n",
             timestamp,
             imuData->ax, imuData->ay, imuData->az,
             imuData->gx, imuData->gy, imuData->gz,
             temperature, battery_voltage,
             lat_str, lon_str,
             (gpsFix && (gpsFix->fix_quality > 0)) ? gpsFix->num_sats : 0);
}

/**
  * @brief  Process incoming commands from the host.
  */
void Protocol_ProcessIncoming(void) {
    /* This is a stub – implement processing (for example "STREAM ON"/"STREAM OFF") as required. */
}
