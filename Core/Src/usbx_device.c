/* Collie_V0/Core/Src/usbx_device.c */
/* Comment out the entire file content to use the version in app_usbx_device.c */
/* Only keep USBX_Device_Process if it's needed and defined here */
#if 0 // Disable this entire file's content

#include "ux_api.h" // Include core USBX API first
#include "usbx_device.h"
#include "ux_system.h"
#include "ux_device_stack.h"
#include "ux_device_class_cdc_acm.h"
#include "ux_dcd_stm32.h"           // Include DCD header
#include "ux_device_descriptors.h" // Include descriptor functions header

#include "stm32u3xx_hal.h"
#include "stm32u3xx_hal_pcd.h"
#include "main.h"  // For Error_Handler and HAL handles

#include <string.h> // For memset

// External HAL handle declared in main.h
extern PCD_HandleTypeDef hpcd_USB_DRD_FS;

// Memory pool for USBX Device Stack
#define UX_DEVICE_MEM_POOL_SIZE    (8192u) // Adjust size as needed
ALIGN_TYPE ux_device_mem_pool[UX_DEVICE_MEM_POOL_SIZE / sizeof(ALIGN_TYPE)]; // Use ALIGN_TYPE for pool

// The CDC ACM instance pointer - declared extern in ux_device_cdc_acm.h
extern UX_SLAVE_CLASS_CDC_ACM *cdc_acm_instance;

// --- USBX CDC ACM Callbacks (Prototypes defined in ux_device_cdc_acm.h) ---
extern VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_class);
extern VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_class);
extern VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_class);

// --- USBX Device State Change Callback Prototype ---
// This function is called by USBX stack to notify application of state changes.
// Note: The version in app_usbx_device.c is static. This should be the one passed
// to ux_device_stack_initialize if needed, but it's set to UX_NULL below for simplicity.
// If needed, it should be implemented here or elsewhere and passed.
/* static UINT USBD_ChangeFunction(ULONG Device_State); */

// --- USBX Device Initialization ---

/**
  * @brief USBX Device Initialization Function
  * @param None
  * @retval UX_SUCCESS or UX_ERROR
  */
UINT MX_USBX_Device_Init(void)
{
    UINT status;
    UCHAR *device_framework_high_speed;
    UCHAR *device_framework_full_speed;
    ULONG device_framework_hs_length;
    ULONG device_framework_fs_length;
    ULONG string_framework_length;
    ULONG language_id_framework_length;
    UCHAR *string_framework;
    UCHAR *language_id_framework;

    /* Initialize USBX system memory pool */
    // Call this ONLY ONCE at the very beginning of the USB initialization process.
    status = ux_system_initialize(ux_device_mem_pool, UX_DEVICE_MEM_POOL_SIZE, UX_NULL, 0);
    if (status != UX_SUCCESS)
    {
        Error_Handler(); // Ensure Error_Handler is defined (typically in main.c)
        return UX_ERROR; // Return error status
    }

    /* Get Device Framework High Speed and get the length */
    /* Assuming FS only for this device based on CubeMX config */
    device_framework_high_speed = UX_NULL; // Not used for FS only
    device_framework_hs_length = 0;

    /* Get Device Framework Full Speed and get the length */
    device_framework_full_speed = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED,
                                                                  &device_framework_fs_length);

    /* Get String Framework and get the length */
    string_framework = USBD_Get_String_Framework(&string_framework_length);

    /* Get Language Id Framework and get the length */
    language_id_framework = USBD_Get_Language_Id_Framework(&language_id_framework_length);

    /* Initialize the USB device stack using device descriptors */
    status = ux_device_stack_initialize(device_framework_high_speed,
                                        device_framework_hs_length,
                                        device_framework_full_speed,
                                        device_framework_fs_length,
                                        string_framework,
                                        string_framework_length,
                                        language_id_framework,
                                        language_id_framework_length,
                                        UX_NULL); // Use UX_NULL for system change callback if not needed
    if (status != UX_SUCCESS)
    {
        Error_Handler();
        return status; // Return the specific error code
    }

    /* Register the CDC-ACM class */
    {
        UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_param;
        memset(&cdc_acm_param, 0, sizeof(UX_SLAVE_CLASS_CDC_ACM_PARAMETER));
        cdc_acm_param.ux_slave_class_cdc_acm_instance_activate   = USBD_CDC_ACM_Activate;
        cdc_acm_param.ux_slave_class_cdc_acm_instance_deactivate = USBD_CDC_ACM_Deactivate;
        cdc_acm_param.ux_slave_class_cdc_acm_parameter_change    = USBD_CDC_ACM_ParameterChange;

        // Determine Configuration and Interface IDs (Check your descriptors!)
        // Use USBD_Get functions from ux_device_descriptors.c
        UINT config_id    = USBD_Get_Configuration_Number(CLASS_TYPE_CDC_ACM, 0);
        UINT interface_id = USBD_Get_Interface_Number(CLASS_TYPE_CDC_ACM, 0); // Get Comm interface number

        status = ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name,
                                                ux_device_class_cdc_acm_entry,
                                                config_id,
                                                interface_id,
                                                (VOID *)&cdc_acm_param);
        if (status != UX_SUCCESS)
        {
            Error_Handler();
            return status;
        }
    }

    /* Initialize the STM32 USB device controller driver */
    /* Pass the correct DCD instance based on FS/HS and USB peripheral used */
    status = ux_dcd_stm32_initialize((ULONG)USB_DRD_FS, (ULONG)&hpcd_USB_DRD_FS);
    if (status != UX_SUCCESS)
    {
        Error_Handler();
        return status;
    }

    /* Start the USB device */
    HAL_PCD_Start(&hpcd_USB_DRD_FS);

    return UX_SUCCESS; // Return success if all initializations passed
}

/* Keep this function if it's called from main.c */
/* If not using RTOS threads, call this periodically in main loop */
void USBX_Device_Process(void)
{
    /* Run Device Stack tasks */
    ux_device_stack_tasks_run();
}

/* Optional: Wrapper function for transmitting data */
UINT USB_CDC_Transmit(uint8_t *data, ULONG length)
{
    UINT status = UX_ERROR;
    ULONG actual_length;

    // Check if instance is valid before using
    if (cdc_acm_instance != UX_NULL)
    {
        // Use the standard 4-argument write function
        status = ux_device_class_cdc_acm_write(cdc_acm_instance, data, length, &actual_length);
    }
    return status;
}

#endif // End of #if 0 block
