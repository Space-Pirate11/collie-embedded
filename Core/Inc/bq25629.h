/*
 * bq25629.h
 *
 *  Created on: Apr 11, 2025
 *      Author: amoltandon
 */

#ifndef INC_BQ25629_H_
#define INC_BQ25629_H_

#include "stm32u3xx_hal.h"

#define BQ25629_I2C_ADDR    0x6A

HAL_StatusTypeDef BQ25629_Init(I2C_HandleTypeDef *hi2c);
float BQ25629_ReadBatteryVoltage(I2C_HandleTypeDef *hi2c);

#endif /* INC_BQ25629_H_ */
