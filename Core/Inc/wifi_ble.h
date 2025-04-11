#ifndef INC_WIFI_BLE_H_
#define INC_WIFI_BLE_H_

#include "stm32u3xx_hal.h"

/*
   Interface for the WiFi/BLE module (INP1014, Talaria TWO).
   The module uses SPI. We assume a simple protocol: a one‑byte length field followed by data.
*/

/* Initialize the WiFi/BLE module. This includes powering it up and waiting for it to boot. */
HAL_StatusTypeDef WiFiBLE_Init(SPI_HandleTypeDef *hspi);

/* Transmit data over SPI to the WiFi/BLE module.
   The protocol: send 1 byte length (max 255) then the data payload.
*/
HAL_StatusTypeDef WiFiBLE_SendData(SPI_HandleTypeDef *hspi, const uint8_t *data, uint16_t length);

#endif /* INC_WIFI_BLE_H_ */
