/* Core/Src/system_control.c */

#include "system_control.h"
#include "main.h" // Required for HAL handles and GPIO definitions
#include "stm32u3xx_hal.h"

/**
 * @brief Initializes system control elements (e.g., sets initial GPIO states).
 */
void system_control_init(void) {
    // Set initial states defined in IOC/hardware design
    // These HAL_GPIO_WritePin calls might be redundant if CubeMX initializes
    // the pins correctly, but explicit setting ensures the desired state.

    // Ensure Chip Selects are inactive (High)
    HAL_GPIO_WritePin(CS_NAND_GPIO_Port, CS_NAND_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CS_WIFI_GPIO_Port, CS_WIFI_Pin, GPIO_PIN_SET); // WiFi CS inactive

    // Set initial state for Resets (assuming active low reset for GPS)
    system_control_set_gps_reset(false); // De-assert GPS reset (High)

    // Set initial state for BMS Reset (assuming active high for BQ25629)
    system_control_set_bms_reset(false); // De-assert BMS reset (Low)

    // Set initial state for Enables (assuming WiFi/LDO disabled initially)
    system_control_set_wifi_en(false); // Disable WiFi module
    system_control_set_ldo_en(false);  // Disable WiFi LDO
}

/**
 * @brief Controls the GPS Reset pin (PH1).
 */
void system_control_set_gps_reset(bool assert_reset) {
    // Assuming GPS_RST_Pin is active LOW
    if (assert_reset) {
        HAL_GPIO_WritePin(GPS_RST_GPIO_Port, GPS_RST_Pin, GPIO_PIN_RESET); // Assert Low
    } else {
        HAL_GPIO_WritePin(GPS_RST_GPIO_Port, GPS_RST_Pin, GPIO_PIN_SET);   // De-assert High
    }
}

/**
 * @brief Controls the WiFi Enable pin (PA3).
 */
void system_control_set_wifi_en(bool enable) {
    if (enable) {
        HAL_GPIO_WritePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin, GPIO_PIN_SET); // Enable High
    } else {
        HAL_GPIO_WritePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin, GPIO_PIN_RESET); // Disable Low
    }
}

/**
 * @brief Controls the 2.5V LDO Enable pin (PB13) for WiFi VDDIO.
 */
void system_control_set_ldo_en(bool enable) {
    if (enable) {
        HAL_GPIO_WritePin(LDO_EN_GPIO_Port, LDO_EN_Pin, GPIO_PIN_SET); // Enable High
    } else {
        HAL_GPIO_WritePin(LDO_EN_GPIO_Port, LDO_EN_Pin, GPIO_PIN_RESET); // Disable Low
    }
}

/**
 * @brief Controls the BMS Reset pin (PA15 - if used).
 */
void system_control_set_bms_reset(bool assert_reset) {
    // Assuming BQ25629 RESET pin is active HIGH (Datasheet Fig 9-1)
    // and connected to PA15 (BMS_RST_Pin defined in main.h)
    if (assert_reset) {
        HAL_GPIO_WritePin(BMS_RST_GPIO_Port, BMS_RST_Pin, GPIO_PIN_SET);   // Assert High
    } else {
        HAL_GPIO_WritePin(BMS_RST_GPIO_Port, BMS_RST_Pin, GPIO_PIN_RESET); // De-assert Low
    }
}
