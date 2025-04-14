/* Core/Src/usb_cdc_task.c */

#include "usb_cdc_task.h"
#include "main.h" // For HAL_Delay, HAL_GetTick
#include "app_usbx_device.h" // USBX Application header
#include "ux_api.h" // Include UX_API for core USBX structures like _ux_system_slave
#include "ux_device_class_cdc_acm.h" // USBX CDC ACM Class header
#include <stdio.h>  // For sprintf, snprintf
#include <string.h> // For strlen, memcpy
#include <stdbool.h> // For bool type

// --- Configuration ---
#define USB_CDC_SEND_BUFFER_SIZE 512 // Size of the buffer for formatted strings
#define USB_CDC_SEND_TIMEOUT_MS  500 // Timeout for trying to send the whole message

// --- Global Variables (Extern from USBX Stack) ---
// This pointer is initialized by the USBX CDC ACM class code, typically in
// ux_device_class_cdc_acm_initialize() or ux_device_class_cdc_acm_activate().
// It is declared extern here because its definition resides within the USBX stack/class code.
extern UX_SLAVE_CLASS_CDC_ACM *cdc_acm_instance;
// -------------------------------------------------

// --- Local Variables ---
// Note: In standalone mode, USBX functions manage their own buffers if configured.
// If using UX_DEVICE_CLASS_CDC_ACM_OWN_ENDPOINT_BUFFER, a buffer here might be needed.
// Assuming standard standalone operation without buffer ownership by this task for now.
// static uint8_t usb_send_buffer[USB_CDC_SEND_BUFFER_SIZE]; // Potentially needed depending on config
static volatile bool usb_device_ready = false; // Flag updated by is_ready check
static uint32_t last_send_warning_time = 0; // Used for rate-limiting error messages

// --- Function Definitions ---

/**
 * @brief Initializes the USB CDC Task module state.
 * Actual USB peripheral and USBX stack init is done elsewhere (main.c -> MX_USB_PCD_Init, MX_USBX_Device_Init)
 */
void usb_cdc_task_init(void) {
    printf("USB CDC: Task initialized for Standalone mode. Waiting for USB enumeration...\r\n");
    usb_device_ready = false; // Assume not ready until host configures the device
    last_send_warning_time = HAL_GetTick();
    // Instance pointer (cdc_acm_instance) is checked dynamically in is_ready() or before use
}

/**
 * @brief Performs periodic checks/updates for the USB CDC task.
 * In standalone mode, USBX tasks need to be run explicitly. This is typically done
 * in the main loop via MX_USBX_Device_Process(). This function can check readiness.
 */
void usb_cdc_task_run(void) {
    // Periodically check if the device is configured and ready
    usb_cdc_task_is_ready();

    // Note: In standalone mode, data reception also uses a '_run' function,
    // e.g., _ux_device_class_cdc_acm_read_run(), called periodically.
    // Add reception handling here if needed.
}

/**
 * @brief Sends a pre-formatted null-terminated string over the USB CDC VCP using standalone USBX write.
 * @param data_string: Pointer to the null-terminated string to send.
 * @return true if the data was successfully sent according to USBX, false otherwise (timeout or error).
 */
bool usb_cdc_task_send_string(const char *data_string) {
    UINT status;
    ULONG actual_length = 0; // Initialize actual_length
    ULONG string_len;
    uint32_t start_time;

    // Check readiness flag first (updated by usb_cdc_task_is_ready/run)
    // Also check if the cdc_acm_instance pointer obtained from USBX stack is valid
    if (!usb_cdc_task_is_ready() || cdc_acm_instance == NULL || data_string == NULL) {
         // Only print warning if device was previously ready or periodically
        if (usb_device_ready && (HAL_GetTick() - last_send_warning_time > 1000)) {
             printf("USB CDC Warn: Send skipped, device not ready or invalid instance/string.\r\n");
            last_send_warning_time = HAL_GetTick();
        }
        return false;
    }

    string_len = strlen(data_string);
    if (string_len == 0) {
        return true; // Nothing to send
    }

    // --- Standalone Write Logic ---
    // _ux_device_class_cdc_acm_write_run needs to be called potentially multiple times.
    // It manages its internal state. We wrap it in a timeout loop.

    // IMPORTANT: _ux_device_class_cdc_acm_write_run takes a non-const buffer.
    // If the data_string is truly const, we might need to copy it to a temporary buffer.
    // For simplicity here, we cast away const, assuming the underlying USBX/HAL layer
    // won't modify the buffer for a write operation. If issues arise, use memcpy to a temp buffer.
    UCHAR* non_const_buffer = (UCHAR*)data_string;

    // Reset state for the write operation before starting
    // (This typically happens internally in _run function on first call with state UX_STATE_RESET)
    // We ensure the state machine starts fresh by setting the internal state if possible,
    // or rely on the _run function's first call logic. Here, we initiate the call.
    // Note: We pass `&actual_length` to potentially get the length sent on success.

    // We need to call _uxe_device_class_cdc_acm_write_run which includes checks.
    // Let's manage the state explicitly for clarity, although _run handles it internally.
    cdc_acm_instance -> ux_device_class_cdc_acm_write_state = UX_STATE_RESET; // Ensure reset state
    start_time = HAL_GetTick();

    do {
        // Call the standalone run function. Pass the buffer, total length, and pointer for actual length.
        // The function will process a chunk or check status based on its internal state.
        status = _uxe_device_class_cdc_acm_write_run(cdc_acm_instance, non_const_buffer, string_len, &actual_length);

        // Check for completion or error states
        if (status == UX_STATE_NEXT) { // Operation completed successfully
            // Verify completion code and actual length
             if (cdc_acm_instance->ux_device_class_cdc_acm_write_status == UX_SUCCESS && actual_length == string_len) {
                last_send_warning_time = HAL_GetTick(); // Reset warning timestamp on success
                return true;
            } else {
                 // Error during transfer even if state machine advanced
                 if (HAL_GetTick() - last_send_warning_time > 1000) {
                      printf("USB CDC Warn: Send completed with issue (Status: %u, Sent: %lu/%lu, CmplCode: %u)\r\n",
                           status, actual_length, string_len, cdc_acm_instance->ux_device_class_cdc_acm_write_status);
                    last_send_warning_time = HAL_GetTick();
                }
                return false;
            }
        } else if (status == UX_STATE_ERROR || status == UX_STATE_EXIT) { // Unrecoverable error
             if (HAL_GetTick() - last_send_warning_time > 1000) {
                printf("USB CDC Warn: Send failed (State: %u, Status: %u)\r\n", status, cdc_acm_instance->ux_device_class_cdc_acm_write_status);
                last_send_warning_time = HAL_GetTick();
            }
            // If the handle became invalid (e.g., device deconfigured), update status
            if (cdc_acm_instance->ux_device_class_cdc_acm_write_status == UX_DEVICE_HANDLE_UNKNOWN ||
                cdc_acm_instance->ux_device_class_cdc_acm_write_status == UX_CONFIGURATION_HANDLE_UNKNOWN) {
                usb_device_ready = false;
            }
            return false;
        }
        // UX_STATE_WAIT or other intermediate states mean keep calling _run

        // Add a small delay or yield if in a cooperative environment, prevent tight loop hogging CPU
        // HAL_Delay(1); // Or a more appropriate yield/sleep function if available

        // Check for timeout
        if ((HAL_GetTick() - start_time) > USB_CDC_SEND_TIMEOUT_MS) {
             if (HAL_GetTick() - last_send_warning_time > 1000) {
                 printf("USB CDC Warn: Send timed out after %d ms.\r\n", USB_CDC_SEND_TIMEOUT_MS);
                last_send_warning_time = HAL_GetTick();
            }
            // Abort the transfer if possible (optional, depends on whether _run handles this)
            // _ux_device_stack_transfer_abort might be needed here if the transfer is stuck
            return false;
        }

        // IMPORTANT: Ensure the main loop calls MX_USBX_Device_Process() or equivalent
        // frequently enough for the underlying USB driver state machines to run.
        // If this send function blocks the main loop completely, USBX standalone won't work.
        // This function assumes it's called from a context where MX_USBX_Device_Process()
        // is also being called elsewhere periodically.

    } while (1); // Loop until success, error, or timeout

    // Should not be reached due to returns in loop
    return false;
}


/**
 * @brief Formats the latest sensor data into a string and sends it over USB CDC.
 * @param p_imu: Pointer to the latest IMU data.
 * @param p_gps: Pointer to the latest GPS data.
 * @param p_temp: Pointer to the latest Temperature data.
 * @param p_bms: Pointer to the latest BMS status.
 * @return true if data was successfully formatted and sent, false otherwise.
 */
bool usb_cdc_task_send_sensor_data(const imu_data_t *p_imu,
                                   const gps_data_t *p_gps,
                                   const temp_data_t *p_temp,
                                   const bms_status_t *p_bms)
{
    // Use a static buffer to avoid large stack allocation if called frequently
    static char temp_buffer[USB_CDC_SEND_BUFFER_SIZE];
    int len = 0;

    // Check readiness and valid pointers first
     if (!usb_cdc_task_is_ready() || cdc_acm_instance == NULL || p_imu == NULL || p_gps == NULL || p_temp == NULL || p_bms == NULL) {
        // Don't spam warnings here, send_string handles readiness checks/warnings
        return false;
    }

    // Format data into a string (Example CSV format)
    // Includes checks for data validity flags within each structure
    len = snprintf(temp_buffer, sizeof(temp_buffer),
                   "T:%.2f,A:%d,%d,%d,G:%d,%d,%d,GPS:%d,%.4f,%.4f,Fix:%d,Sat:%d,BMS:%d,%.0f,%.0f,%d\r\n",
                   p_temp->data_valid ? p_temp->temperature_c : -999.99f, // Use placeholder for invalid temp
                   p_imu->accel_x, p_imu->accel_y, p_imu->accel_z,
                   p_imu->gyro_x, p_imu->gyro_y, p_imu->gyro_z,
                   p_gps->data_valid, // Send validity flag
                   p_gps->data_valid ? p_gps->latitude : 0.0f,  // Send 0 if invalid
                   p_gps->data_valid ? p_gps->longitude : 0.0f, // Send 0 if invalid
                   p_gps->data_valid ? p_gps->fix_quality : 0,
                   p_gps->data_valid ? p_gps->satellites_tracked : 0,
                   p_bms->data_valid, // Send validity flag
                   p_bms->data_valid ? p_bms->battery_voltage_mv : -1.0f, // Use placeholder for invalid BMS values
                   p_bms->data_valid ? p_bms->charge_current_ma : -1.0f,
                   p_bms->data_valid ? p_bms->charge_status : -1 // Use placeholder for invalid status
                  );

    // Check for snprintf errors (negative return value) or buffer overflow
    if (len < 0) {
        printf("USB CDC Error: snprintf error during data formatting.\r\n");
        return false;
    }
    if (len >= sizeof(temp_buffer)) {
         printf("USB CDC Error: snprintf buffer overflow during data formatting. Increase USB_CDC_SEND_BUFFER_SIZE.\r\n");
        // Send truncated data? Or return error? Returning error is safer.
        return false;
        // Or: temp_buffer[sizeof(temp_buffer) - 1] = '\0'; // Null-terminate truncated string
    }


    // Send the formatted string using the standalone-compatible function
    return usb_cdc_task_send_string(temp_buffer);
}

/**
 * @brief Checks if the USB device is configured and the CDC instance is valid.
 * Updates the internal usb_device_ready flag.
 * @return true if connected and configured, false otherwise.
 */
bool usb_cdc_task_is_ready(void) {
    // Check the device state via the USBX global structure _ux_system_slave
    // Requires ux_api.h to be included
    // Also check if the specific CDC instance pointer we have is valid
    // In standalone mode, cdc_acm_instance might be NULL until activation callback runs.
    if (cdc_acm_instance != NULL && _ux_system_slave != NULL &&
        _ux_system_slave->ux_system_slave_device.ux_slave_device_state == UX_DEVICE_CONFIGURED)
    {
        if (!usb_device_ready) {
             printf("USB CDC Info: Device configured and ready.\r\n");
            usb_device_ready = true;
        }
    } else {
        if (usb_device_ready) {
             printf("USB CDC Info: Device disconnected or not configured.\r\n");
            usb_device_ready = false;
        }
    }
    return usb_device_ready;
}
