#ifndef INC_GPS_H_
#define INC_GPS_H_

#include "stm32u3xx_hal.h"

/* GPS Fix data structure */
typedef struct {
    float latitude;
    float longitude;
    uint8_t fix_quality;  // 0 = no fix, 1 = GPS fix, etc.
    uint8_t num_sats;     // Number of satellites used
} GPS_Fix_t;

/* Initialize GPS module (start UART reception, etc.) */
void GPS_Init(void);

/* Retrieve the latest parsed GPS fix.
   Returns 1 if new data available, 0 otherwise.
*/
uint8_t GPS_GetLatestFix(GPS_Fix_t *fix);

#endif /* INC_GPS_H_ */
