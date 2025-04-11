#ifndef INC_USBX_DEVICE_H_
#define INC_USBX_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_api.h"

/* Initializes the USBX device stack in CDC mode */
void MX_USBX_Device_Init(void);

/* Must be called periodically (e.g. in main loop) to process USBX device tasks */
void USBX_Device_Process(void);

/* Helper to transmit data over USB CDC */
UINT USB_CDC_Transmit(uint8_t *data, ULONG length);

#ifdef __cplusplus
}
#endif

#endif /* INC_USBX_DEVICE_H_ */
