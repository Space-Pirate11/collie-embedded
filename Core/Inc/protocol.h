#ifndef INC_PROTOCOL_H_
#define INC_PROTOCOL_H_

#include "bmi270.h"
#include "stm32u3xx_hal.h"
#include "gps.h"
#include "bq25629.h"

/*
   Protocol Module:
   - Formats sensor data into CSV text lines.
   - Sends data over USB CDC using USBX (CDC ACM class) and optionally over BLE.
   - Processes incoming commands.
*/

/* Initialize protocol module */
void Protocol_Init(void);

/* Send IMU data (CSV line with: "IMU,timestamp,ax,ay,az,gx,gy,gz\r\n") */
void Protocol_SendIMU(const BMI270_Data *data);

/* Send environmental data (CSV line with: "ENV,timestamp,temperature,battery,lat,lon,fixQuality,sats\r\n") */
void Protocol_SendEnvironmental(float temperature, float battery_voltage, const GPS_Fix_t *gpsFix, uint32_t timestamp);

/* Construct a combined CSV log line for NAND flash logging.
   Format: "timestamp,ax,ay,az,gx,gy,gz,temperature,battery,lat,lon,numSats\r\n"
   The formatted log line is written into outBuffer.
*/
void Protocol_LogData(const BMI270_Data *imuData, float temperature, float battery_voltage,
                        const GPS_Fix_t *gpsFix, uint32_t timestamp, char *outBuffer, size_t bufSize);

/* Process incoming commands from USB (or BLE) */
void Protocol_ProcessIncoming(void);

#endif /* INC_PROTOCOL_H_ */
