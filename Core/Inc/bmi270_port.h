/* Core/Inc/bmi270_port.h */

#ifndef INC_BMI270_PORT_H_
#define INC_BMI270_PORT_H_

#include "bmi2.h" // Include base BMI2 definitions

/* BMI270 I2C Address */
// Datasheet specifies 0x68 or 0x69 depending on SDO pin.
// Assume SDO is grounded -> 0x68
#define BMI270_I2C_ADDR     (0x68 << 1) // STM32 HAL uses 8-bit address

/* Function prototypes for BMI2 platform specific functions */

/**
 * @brief Platform specific I2C write function
 * @param reg_addr : Register address
 * @param reg_data : Pointer to data buffer
 * @param len      : Length of data buffer
 * @param intf_ptr : Void pointer that can enable the linking of interface specific parameters
 * @return Result of API execution status
 */
int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * @brief Platform specific I2C read function
 * @param reg_addr : Register address
 * @param reg_data : Pointer to data buffer
 * @param len      : Length of data buffer
 * @param intf_ptr : Void pointer that can enable the linking of interface specific parameters
 * @return Result of API execution status
 */
int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * @brief Platform specific delay function (microseconds)
 * @param period   : Delay in microseconds
 * @param intf_ptr : Void pointer that can enable the linking of interface specific parameters
 * @return void
 */
void bmi2_delay_us(uint32_t period, void *intf_ptr);

/**
 * @brief Initialize the platform specific interface for BMI270
 * (Assign HAL handles, etc.)
 * @return BMI2_OK on success, error code otherwise
 */
int8_t bmi2_interface_init(struct bmi2_dev *bmi_dev);

#endif /* INC_BMI270_PORT_H_ */
