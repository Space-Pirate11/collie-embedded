#ifndef INC_GPS_H_
#define INC_GPS_H_

#include "stm32u3xx_hal.h"
#include "custom_types.h" // Include the definition of GPS_Fix_t

/* Initialize GPS module (start UART reception, etc.) */
void GPS_Init(void);

/* Retrieve the latest parsed GPS fix.
   The fix data is copied into the provided structure.
   Returns 1 if a valid fix was available and copied, 0 otherwise.
*/
uint8_t GPS_GetLatestFix(GPS_Fix_t *fix);

/* Callback function to be called from HAL_UART_RxCpltCallback */
/* Ensure this is called in stm32u3xx_it.c or main.c's HAL_UART_RxCpltCallback */
void GPS_UART_RxCpltCallback(UART_HandleTypeDef *huart);


#endif /* INC_GPS_H_ */
