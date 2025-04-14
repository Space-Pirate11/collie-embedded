/* Core/Src/bms_task.c */

#include "bms_task.h"
#include "main.h" // Required for HAL handles and definitions
#include "stm32u3xx_hal.h"
#include "system_control.h" // Added include
#include <stdio.h>  // For printf
#include <string.h> // For memset

// --- Configuration ---
#define BMS_POLL_INTERVAL_MS 1000 // Check status every 1 second
#define BMS_I2C_TIMEOUT      100  // Milliseconds

// BQ25629 Register Addresses (Refer to BQ25629 Datasheet)
#define BQ_REG_CHARGE_STATUS_0     0x00
#define BQ_REG_CHARGE_STATUS_1     0x01
#define BQ_REG_FAULT_STATUS_0      0x02
#define BQ_REG_FAULT_STATUS_1      0x03
#define BQ_REG_MIN_SYS_VOLTAGE     0x04
#define BQ_REG_CHARGE_CURRENT      0x06
#define BQ_REG_PRECHG_TERM_CURR    0x08
#define BQ_REG_CHARGE_VOLTAGE      0x0A
#define BQ_REG_CHARGE_TERM_TIMER   0x0C
#define BQ_REG_CHARGE_SAFETY_TIMER 0x0E
#define BQ_REG_TEMP_CONTROL        0x10 // NTC related
#define BQ_REG_FAULT_MASK_0        0x12
#define BQ_REG_FAULT_MASK_1        0x13
#define BQ_REG_STATUS_MASK_0       0x14
#define BQ_REG_STATUS_MASK_1       0x15
#define BQ_REG_CHARGER_CONTROL_0   0x16
#define BQ_REG_CHARGER_CONTROL_1   0x17
#define BQ_REG_CHARGER_CONTROL_2   0x18
#define BQ_REG_CHARGER_CONTROL_3   0x19
#define BQ_REG_CHARGER_CONTROL_4   0x1A
#define BQ_REG_PART_REVISION       0x1B
#define BQ_REG_VBUS_ADC            0x1C
#define BQ_REG_CHARGE_CURRENT_ADC  0x1E
#define BQ_REG_VBAT_ADC            0x20
#define BQ_REG_VSYS_ADC            0x22
#define BQ_REG_TS_ADC              0x24 // Requires NTC configuration

// BQ_REG_CHARGER_CONTROL_0 bits
#define WD_RST_BIT          (1 << 2)
#define EN_HIZ_BIT          (1 << 4)
#define EN_CHG_BIT          (1 << 5)

// BQ_REG_CHARGE_STATUS_0 bits
#define CHG_STAT_MASK       (0b11 << 4)
#define CHG_STAT_NO_CHG     (0b00 << 4)
#define CHG_STAT_PRECHG     (0b01 << 4)
#define CHG_STAT_FASTCHG    (0b10 << 4)
#define CHG_STAT_CHGDONE    (0b11 << 4)
#define VBUS_STAT_MASK      (0b111 << 0) // VBUS Status bits

// BQ_REG_FAULT_STATUS_0 bits
#define WATCHDOG_FAULT_BIT  (1 << 7)
#define INPUT_FAULT_BIT     (1 << 6)
#define THERMAL_SHUTDOWN_BIT (1 << 5)
#define SAFETY_TIMER_EXP_BIT (1 << 4)
// Add other fault bits as needed

// --- HAL Handle Extern Declarations ---
extern I2C_HandleTypeDef hi2c1;
// --------------------------------------

// --- Local Variables ---
static bool bms_initialized = false;
static uint32_t last_poll_time = 0;
static volatile bool bms_interrupt_flag = false; // Set by ISR
static bms_status_t last_valid_status = { .data_valid = false };
// ---------------------

// --- Helper Functions ---
static bms_charge_state_t decode_charge_status_reg(uint8_t status_reg0) {
    uint8_t chg_stat = status_reg0 & CHG_STAT_MASK;
    switch (chg_stat) {
        case CHG_STAT_PRECHG:  return BMS_CHG_PRECHARGE;
        case CHG_STAT_FASTCHG: return BMS_CHG_FASTCHARGE;
        case CHG_STAT_CHGDONE: return BMS_CHG_CHARGE_DONE;
        case CHG_STAT_NO_CHG:
        default:               return BMS_CHG_NOT_CHARGING;
    }
}

static float decode_adc_value(uint8_t reg_msb, uint8_t reg_lsb, float lsb_value, float offset) {
    uint16_t raw_adc = ((uint16_t)reg_msb << 8) | reg_lsb;
    return ((float)raw_adc * lsb_value) + offset;
}

// --- Function Definitions ---

/**
 * @brief Reads a specific register from the BQ25629.
 */
bool bms_task_read_register(uint8_t reg_addr, uint8_t *data) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, BQ25629_I2C_ADDR, reg_addr,
                                               I2C_MEMADD_SIZE_8BIT, data, 1, BMS_I2C_TIMEOUT);
    return (status == HAL_OK);
}

/**
 * @brief Writes a specific register to the BQ25629.
 */
bool bms_task_write_register(uint8_t reg_addr, uint8_t data) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, BQ25629_I2C_ADDR, reg_addr,
                                                I2C_MEMADD_SIZE_8BIT, &data, 1, BMS_I2C_TIMEOUT);
    return (status == HAL_OK);
}


/**
 * @brief Initializes the BMS task (I2C peripheral, GPIOs for INT/CHG/RESET).
 */
bool bms_task_init(void) {
    uint8_t data;
    bool success = true;

    // Assuming I2C1, GPIOs (PB8 Input, PH3 EXTI, PA15 Output) are initialized by CubeMX

    // 1. Check communication by reading Part/Revision register
    if (!bms_task_read_register(BQ_REG_PART_REVISION, &data)) {
        printf("BMS Error: Failed to read revision register\r\n");
        return false;
    }
    printf("BMS Info: BQ25629 Part/Rev: 0x%02X\r\n", data);
    // Add check for expected revision if necessary

    // 2. Configure charging parameters (CRITICAL - SET ACCORDING TO YOUR BATTERY DATASHEET)
    // Example: Set Charge Voltage Limit to ~4.2V (4208mV typical)
    // Formula: V_REG = 2304mV + VREG_CODE * 16mV. Target 4208mV -> Code = (4208-2304)/16 = 119 = 0x77
    // REG0x0A[7:0] = VREG[7:0] = 0x77
    // REG0x0B[7]   = VREG[8]   = 0
    success &= bms_task_write_register(BQ_REG_CHARGE_VOLTAGE, 0x77); // Lower byte
    success &= bms_task_write_register(BQ_REG_CHARGE_VOLTAGE + 1, 0x00); // Upper byte (only bit 7 used for VREG[8])
    if (!success) printf("BMS Error: Failed to set charge voltage\r\n");

    // Example: Set Charge Current Limit to 512mA
    // Formula: I_CHG = 64mA + ICHG_CODE * 64mA. Target 512mA -> Code = (512-64)/64 = 7 = 0x07
    // REG0x06[7:0] = ICHG[7:0] = 0x07
    success &= bms_task_write_register(BQ_REG_CHARGE_CURRENT, 0x07);
    if (!success) printf("BMS Error: Failed to set charge current\r\n");

    // Example: Set Precharge Current to 128mA
    // Formula: I_PRECHG = 64mA + IPRECHG_CODE * 64mA. Target 128mA -> Code = (128-64)/64 = 1 = 0x01
    // REG0x08[7:4] = IPRECHG[3:0] = 0b0001
    // Read current value, modify, write back
    if (bms_task_read_register(BQ_REG_PRECHG_TERM_CURR, &data)) {
        data &= 0x0F; // Clear upper nibble
        data |= (0x01 << 4); // Set IPRECHG bits
        success &= bms_task_write_register(BQ_REG_PRECHG_TERM_CURR, data);
        if (!success) printf("BMS Error: Failed to set precharge current\r\n");
    } else {
        printf("BMS Error: Failed read before setting precharge current\r\n");
        success = false;
    }

    // Example: Set Termination Current to 64mA (minimum)
    // Formula: I_TERM = 64mA + ITERM_CODE * 64mA. Target 64mA -> Code = 0 = 0x00
    // REG0x08[3:0] = ITERM[3:0] = 0b0000
    if (bms_task_read_register(BQ_REG_PRECHG_TERM_CURR, &data)) {
        data &= 0xF0; // Clear lower nibble
        data |= 0x00; // Set ITERM bits
        success &= bms_task_write_register(BQ_REG_PRECHG_TERM_CURR, data);
        if (!success) printf("BMS Error: Failed to set termination current\r\n");
    } else {
        printf("BMS Error: Failed read before setting term current\r\n");
        success = false;
    }


    // 3. Configure Timers (Example: Enable default safety timer)
    // REG0x0E: CHG_TIMER[2:1] -> 01b = 5 hours (default)
    // REG0x0E: EN_TIMER[0] -> 1b = Enable (default)
    // success &= bms_task_write_register(BQ_REG_CHARGE_SAFETY_TIMER, 0b00001011); // Default values

    // 4. Enable charging (it's enabled by default, REG0x16[5]=1)
    // success &= bms_task_write_register(BQ_REG_CHARGER_CONTROL_0, (1 << 5) | (1 << 0)); // Ensure EN_CHG=1, WATCHDOG=50s

    // 5. Configure Interrupt Masks (Example: enable watchdog fault interrupt)
    // REG0x12[7] = WATCHDOG_FAULT_MASK = 0 (0 = enable interrupt)
    // success &= bms_task_write_register(BQ_REG_FAULT_MASK_0, 0x00); // Enable all faults to trigger INT

    if (!success) {
        printf("BMS Error: Initialization failed during configuration writes.\r\n");
        return false;
    }

    printf("BMS Info: BQ25629 Initialized and Configured.\r\n");
    bms_initialized = true;
    last_poll_time = HAL_GetTick();
    memset(&last_valid_status, 0, sizeof(bms_status_t));
    last_valid_status.data_valid = false;

    return true;
}

/**
 * @brief Performs a single run/update cycle for the BMS task.
 */
void bms_task_run(bms_status_t *p_bms_status) {
    uint8_t status0, status1, fault0, fault1;
    uint8_t adc_msb, adc_lsb;
    bool success = true;

    if (!bms_initialized || p_bms_status == NULL) {
        return;
    }

    uint32_t current_time = HAL_GetTick();
    bool check_now = (current_time - last_poll_time) >= BMS_POLL_INTERVAL_MS;
    bool interrupt_pending = bms_interrupt_flag; // Atomically read and clear flag
    bms_interrupt_flag = false;

    if (check_now || interrupt_pending) {
        last_poll_time = current_time;

        // --- Read Status and Fault Registers ---
        success &= bms_task_read_register(BQ_REG_CHARGE_STATUS_0, &status0);
        success &= bms_task_read_register(BQ_REG_CHARGE_STATUS_1, &status1);
        success &= bms_task_read_register(BQ_REG_FAULT_STATUS_0, &fault0);
        success &= bms_task_read_register(BQ_REG_FAULT_STATUS_1, &fault1);

        // --- Read ADC Values ---
        // VBUS ADC (LSB = 2.6 mV, Offset = 2600 mV - approx) - Check datasheet for exact values
        success &= bms_task_read_register(BQ_REG_VBUS_ADC, &adc_msb);
        success &= bms_task_read_register(BQ_REG_VBUS_ADC + 1, &adc_lsb);
        if (success) p_bms_status->vbus_voltage_mv = decode_adc_value(adc_msb, adc_lsb, 2.6f, 2600.0f);

        // Charge Current ADC (LSB = 50 mA, Offset = 0 mA)
        success &= bms_task_read_register(BQ_REG_CHARGE_CURRENT_ADC, &adc_msb);
        success &= bms_task_read_register(BQ_REG_CHARGE_CURRENT_ADC + 1, &adc_lsb);
         if (success) p_bms_status->charge_current_ma = decode_adc_value(adc_msb, adc_lsb, 50.0f, 0.0f);

        // VBAT ADC (LSB = 2.6 mV, Offset = 2304 mV - approx)
        success &= bms_task_read_register(BQ_REG_VBAT_ADC, &adc_msb);
        success &= bms_task_read_register(BQ_REG_VBAT_ADC + 1, &adc_lsb);
        if (success) p_bms_status->battery_voltage_mv = decode_adc_value(adc_msb, adc_lsb, 2.6f, 2304.0f);

        // VSYS ADC (LSB = 2.6 mV, Offset = 2304 mV - approx)
        success &= bms_task_read_register(BQ_REG_VSYS_ADC, &adc_msb);
        success &= bms_task_read_register(BQ_REG_VSYS_ADC + 1, &adc_lsb);
        if (success) p_bms_status->system_voltage_mv = decode_adc_value(adc_msb, adc_lsb, 2.6f, 2304.0f);


        if (success) {
            // --- Decode Status ---
            p_bms_status->charge_status = decode_charge_status_reg(status0);
            p_bms_status->fault_flags = fault0; // Store fault flags (can decode specific bits if needed)
            p_bms_status->interrupt_active = interrupt_pending; // Store if INT triggered this check
            p_bms_status->data_valid = true;

            // Store locally
            memcpy(&last_valid_status, p_bms_status, sizeof(bms_status_t));

            // --- Handle Faults (Example: Reset watchdog if fault occurred) ---
            if (fault0 & WATCHDOG_FAULT_BIT) {
                 printf("BMS Warning: Watchdog timer expired! Resetting timer.\r\n");
                 uint8_t ctrl0_val;
                 if (bms_task_read_register(BQ_REG_CHARGER_CONTROL_0, &ctrl0_val)) {
                     ctrl0_val |= WD_RST_BIT; // Set reset bit
                     if (!bms_task_write_register(BQ_REG_CHARGER_CONTROL_0, ctrl0_val)) {
                         printf("BMS Error: Failed to write watchdog reset.\r\n");
                     }
                     // Note: WD_RST bit self-clears
                 } else {
                      printf("BMS Error: Failed to read control reg before watchdog reset.\r\n");
                 }
                 // Clear the fault bit locally after handling
                 p_bms_status->fault_flags &= ~WATCHDOG_FAULT_BIT;
                 last_valid_status.fault_flags = p_bms_status->fault_flags;
            }
            // Add handling for other critical faults as needed

            // --- Debug Print ---
            // printf("BMS: VBUS=%.0fmV, VBAT=%.0fmV, VSYS=%.0fmV, ICHG=%.0fmA, Status=%d, Fault=0x%02X\r\n",
            //        p_bms_status->vbus_voltage_mv, p_bms_status->battery_voltage_mv, p_bms_status->system_voltage_mv,
            //        p_bms_status->charge_current_ma, p_bms_status->charge_status, p_bms_status->fault_flags);
            // -------------------

        } else {
            printf("BMS Error: Failed to read status/ADC registers.\r\n");
            p_bms_status->data_valid = false;
            last_valid_status.data_valid = false;
        }
    } else {
        // If no poll interval and no interrupt, just copy last known status
        if (p_bms_status != NULL) {
            memcpy(p_bms_status, &last_valid_status, sizeof(bms_status_t));
        }
    }
}


/**
 * @brief Callback function to be potentially called from EXTI Interrupt Handler for BMS_INT (PH3).
 */
void bms_task_interrupt_callback(void) {
    // This function should be called from the HAL_GPIO_EXTI_Callback
    // Keep it short - just set a flag
    bms_interrupt_flag = true;
}

/**
 * @brief Toggles the BMS Reset Pin (PA15).
 */
void bms_task_set_reset(bool assert_reset) {
    // Forward call to system control
    system_control_set_bms_reset(assert_reset);
}

/**
 * @brief Reads the state of the BMS Charge Status Pin (PB8).
 */
bool bms_task_read_charge_status_pin(void) {
    // STAT pin is open-drain, Low = Charging, High-Z = Done/Not Charging
    // Assumes PB8 (BMS_CHG_Pin) is configured as Input with external pull-up (as user stated)
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(BMS_CHG_GPIO_Port, BMS_CHG_Pin);
    return (pin_state == GPIO_PIN_SET); // Return true if High (Not Charging/Done)
}


/**
 * @brief Get the last successfully read BMS status.
 */
void bms_task_get_last_status(bms_status_t *p_bms_status) {
     if (p_bms_status != NULL) {
        // Critical section might be needed if an ISR could modify last_valid_status
        // For now, assume simple non-preemptive or infrequent updates
        memcpy(p_bms_status, &last_valid_status, sizeof(bms_status_t));
    }
}
