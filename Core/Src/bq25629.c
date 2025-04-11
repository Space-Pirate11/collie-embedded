#include "bq25629.h"
#include <stdlib.h>

/* Initialize the BQ25629 charger. Enable ADC conversion for battery voltage measurement. */
HAL_StatusTypeDef BQ25629_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t reg;
    // Read ADC control register (0x02) and enable ADC if not already enabled (assume bit 7)
    if(HAL_I2C_Mem_Read(hi2c, BQ25629_I2C_ADDR << 1, BQ25629_REG_ADC_CTRL, 1, &reg, 1, 100) != HAL_OK)
        return HAL_ERROR;
    if(!(reg & (1 << 7))) {
        reg |= (1 << 7);
        if(HAL_I2C_Mem_Write(hi2c, BQ25629_I2C_ADDR << 1, BQ25629_REG_ADC_CTRL, 1, &reg, 1, 100) != HAL_OK)
            return HAL_ERROR;
    }
    // Additional initialization may be added here.
    return HAL_OK;
}

/* Read battery voltage from register 0x0E.
   Assume a 12-bit ADC result with a conversion factor of 1.99 mV/LSB.
*/
float BQ25629_ReadBatteryVoltage(I2C_HandleTypeDef *hi2c) {
    uint8_t bytes[2];
    if(HAL_I2C_Mem_Read(hi2c, BQ25629_I2C_ADDR << 1, BQ25629_REG_BATV, 1, bytes, 2, 100) != HAL_OK)
        return -1.0f;
    uint16_t adc_code = (bytes[0] << 4) | (bytes[1] & 0x0F);
    float voltage_mV = adc_code * 1.99f;
    return voltage_mV / 1000.0f;
}

/* Set charge current (in mA). Assume register 0x04 with LSB = 50 mA steps. */
HAL_StatusTypeDef BQ25629_SetChargeCurrent(I2C_HandleTypeDef *hi2c, uint16_t current_mA) {
    uint8_t reg_val = current_mA / 50;
    return HAL_I2C_Mem_Write(hi2c, BQ25629_I2C_ADDR << 1, BQ25629_REG_ICHG, 1, &reg_val, 1, 100);
}

/* Set charge voltage (in mV). Assume register 0x06 with LSB = 10 mV and offset 4000 mV. */
HAL_StatusTypeDef BQ25629_SetChargeVoltage(I2C_HandleTypeDef *hi2c, uint16_t voltage_mV) {
    if(voltage_mV < 4000 || voltage_mV > 4200)
        return HAL_ERROR;  // Out of range
    uint8_t reg_val = (voltage_mV - 4000) / 10;
    return HAL_I2C_Mem_Write(hi2c, BQ25629_I2C_ADDR << 1, BQ25629_REG_VREG, 1, &reg_val, 1, 100);
}

/* Set input current limit (in mA). Assume register 0x00 with LSB = 50 mA. */
HAL_StatusTypeDef BQ25629_SetInputCurrentLimit(I2C_HandleTypeDef *hi2c, uint16_t current_mA) {
    uint8_t reg_val = current_mA / 50;
    return HAL_I2C_Mem_Write(hi2c, BQ25629_I2C_ADDR << 1, BQ25629_REG_ILIM, 1, &reg_val, 1, 100);
}

/* Read the actual charge current (mA) from register 0x12 (example scale, LSB = 50 mA) */
float BQ25629_ReadChargeCurrent(I2C_HandleTypeDef *hi2c) {
    uint8_t reg_val;
    if(HAL_I2C_Mem_Read(hi2c, BQ25629_I2C_ADDR << 1, 0x12, 1, &reg_val, 1, 100) != HAL_OK)
        return -1.0f;
    return ((float)reg_val) * 50.0f;
}

/* Read battery current (mA) from simulated registers (example implementation) */
float BQ25629_ReadBatteryCurrent(I2C_HandleTypeDef *hi2c) {
    uint8_t bytes[2];
    if(HAL_I2C_Mem_Read(hi2c, BQ25629_I2C_ADDR << 1, 0x13, 1, bytes, 2, 100) != HAL_OK)
        return -1.0f;
    uint16_t adc_code = (bytes[0] << 4) | (bytes[1] & 0x0F);
    float current_mA = adc_code * 0.5f;   // Example scale: 0.5 mA per LSB
    return current_mA;
}

/* Calculate battery state of charge (SOC) using linear approximation.
   100% at 4.2V and 0% at 3.0V.
*/
float BQ25629_GetStateOfCharge(float battery_voltage) {
    if(battery_voltage >= 4.2f)
        return 100.0f;
    else if(battery_voltage <= 3.0f)
        return 0.0f;
    else {
        return ((battery_voltage - 3.0f) / (4.2f - 3.0f)) * 100.0f;
    }
}

/* Calculate a simple State of Health (SOH) estimation.
   Here we return a constant value as a placeholder.
*/
float BQ25629_GetStateOfHealth(void) {
    return 100.0f;
}
