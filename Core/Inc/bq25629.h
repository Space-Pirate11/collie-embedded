#ifndef INC_BQ25629_H_
#define INC_BQ25629_H_

#include "stm32u3xx_hal.h"

/* BQ25629 I²C address (7-bit) */
#define BQ25629_I2C_ADDR    0x6A

/* Register definitions (refer to datasheet for exact addresses) */
#define BQ25629_REG_ILIM         0x00    // Input current limit register
#define BQ25629_REG_VINDPM_OS    0x01    // Input voltage DPM over-sensing
#define BQ25629_REG_ADC_CTRL     0x02    // ADC control register
#define BQ25629_REG_SYS_CTRL     0x03    // System control register
#define BQ25629_REG_ICHG         0x04    // Charge current setting register
#define BQ25629_REG_VREG         0x06    // Charge voltage setting register
#define BQ25629_REG_TIMER        0x07    // Timer settings register
#define BQ25629_REG_BAT_COMP     0x08    // Battery compensation register
#define BQ25629_REG_CTRL1        0x09    // Control register 1
#define BQ25629_REG_STATUS       0x0B    // Status register
#define BQ25629_REG_VINDPM       0x0D    // Input voltage DPM threshold
#define BQ25629_REG_BATV         0x0E    // Battery voltage reading register

/* Function prototypes for BQ25629 driver */
HAL_StatusTypeDef BQ25629_Init(I2C_HandleTypeDef *hi2c);
float BQ25629_ReadBatteryVoltage(I2C_HandleTypeDef *hi2c);

/* Extended API functions: */

/* Set charge current limit (mA) */
HAL_StatusTypeDef BQ25629_SetChargeCurrent(I2C_HandleTypeDef *hi2c, uint16_t current_mA);

/* Set charge voltage limit (mV) */
HAL_StatusTypeDef BQ25629_SetChargeVoltage(I2C_HandleTypeDef *hi2c, uint16_t voltage_mV);

/* Set input current limit (mA) */
HAL_StatusTypeDef BQ25629_SetInputCurrentLimit(I2C_HandleTypeDef *hi2c, uint16_t current_mA);

/* Read actual charge current (mA) */
float BQ25629_ReadChargeCurrent(I2C_HandleTypeDef *hi2c);

/* Read battery current (mA) */
float BQ25629_ReadBatteryCurrent(I2C_HandleTypeDef *hi2c);

/* Calculate State of Charge (SOC) based on battery voltage.
   Using a linear approximation: 100% at 4.2V, 0% at 3.0V.
*/
float BQ25629_GetStateOfCharge(float battery_voltage);

/* Calculate a simple State of Health (SOH) estimation (placeholder). */
float BQ25629_GetStateOfHealth(void);

#endif /* INC_BQ25629_H_ */
