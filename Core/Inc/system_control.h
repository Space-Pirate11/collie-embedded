/* Core/Inc/system_control.h */

#ifndef INC_SYSTEM_CONTROL_H_
#define INC_SYSTEM_CONTROL_H_

#include <stdbool.h>

/**
 * @brief Initializes system control elements (e.g., sets initial GPIO states).
 */
void system_control_init(void);

/**
 * @brief Controls the GPS Reset pin (PH1).
 * @param enable If true, asserts reset (typically low); if false, de-asserts reset.
 */
void system_control_set_gps_reset(bool assert_reset);

/**
 * @brief Controls the WiFi Enable pin (PA3).
 * @param enable If true, enable the WiFi module; if false, disable it.
 */
void system_control_set_wifi_en(bool enable);

/**
 * @brief Controls the 2.5V LDO Enable pin (PB13) for WiFi VDDIO.
 * @param enable If true, enable the LDO; if false, disable it.
 */
void system_control_set_ldo_en(bool enable);

/**
 * @brief Controls the BMS Reset pin (PA15 - if used).
 * @param assert_reset If true, asserts reset (typically high for BQ25629); if false, de-asserts.
 */
void system_control_set_bms_reset(bool assert_reset);


#endif /* INC_SYSTEM_CONTROL_H_ */
