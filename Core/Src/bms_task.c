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

// BQ25628 Register Addresses (Refer to BQ25629 Datasheet Table 8-7)
#define BQ_REG_CHARGE_CURRENT_LIMIT      0x02 // + 0x03
#define BQ_REG_CHARGE_VOLTAGE_LIMIT      0x04 // + 0x05
#define BQ_REG_INPUT_CURRENT_LIMIT       0x06 // + 0x07
#define BQ_REG_INPUT_VOLTAGE_LIMIT       0x08 // + 0x09
#define BQ_REG_VOTG_REGULATION           0x0C // + 0x0D
#define BQ_REG_MINIMAL_SYSTEM_VOLTAGE    0x0E // + 0x0F
#define BQ_REG_PRECHARGE_CONTROL         0x10 // + 0x11
#define BQ_REG_TERMINATION_CONTROL       0x12 // + 0x13
#define BQ_REG_CHARGE_CONTROL            0x14
#define BQ_REG_CHARGE_TIMER_CONTROL      0x15
#define BQ_REG_CHARGER_CONTROL_0         0x16
#define BQ_REG_CHARGER_CONTROL_1         0x17
#define BQ_REG_CHARGER_CONTROL_2         0x18
#define BQ_REG_CHARGER_CONTROL_3         0x19
#define BQ_REG_NTC_CONTROL_0             0x1A
#define BQ_REG_NTC_CONTROL_1             0x1B
#define BQ_REG_NTC_CONTROL_2             0x1C
#define BQ_REG_CHARGER_STATUS_0          0x1D
#define BQ_REG_CHARGER_STATUS_1          0x1E
#define BQ_REG_FAULT_STATUS_0            0x1F
#define BQ_REG_CHARGER_FLAG_0            0x20
#define BQ_REG_CHARGER_FLAG_1            0x21
#define BQ_REG_FAULT_FLAG_0              0x22
#define BQ_REG_CHARGER_MASK_0            0x23
#define BQ_REG_CHARGER_MASK_1            0x24
#define BQ_REG_FAULT_MASK_0              0x25
#define BQ_REG_ADC_CONTROL               0x26
#define BQ_REG_ADC_FUNC_DISABLE_0        0x27
#define BQ_REG_IBUS_ADC                  0x28 // + 0x29
#define BQ_REG_IBAT_ADC                  0x2A // + 0x2B
#define BQ_REG_VBUS_ADC                  0x2C // + 0x2D
#define BQ_REG_VPMID_ADC                 0x2E // + 0x2F
#define BQ_REG_VBAT_ADC                  0x30 // + 0x31
#define BQ_REG_VSYS_ADC                  0x32 // + 0x33
#define BQ_REG_TS_ADC                    0x34 // + 0x35
#define BQ_REG_TDIE_ADC                  0x36 // + 0x37
#define BQ_REG_PART_INFORMATION          0x38

// --- BQ25628 Bit Definitions ---

// REG0x16: Charger Control 0
#define CHG_CTRL0_WATCHDOG_MASK   (0b11 << 0)
#define CHG_CTRL0_WATCHDOG_50S    (0b01 << 0) // Default
#define CHG_CTRL0_WD_RST_BIT      (1 << 2)
#define CHG_CTRL0_FORCE_PMID_DIS  (1 << 3)
#define CHG_CTRL0_EN_HIZ_BIT      (1 << 4)
#define CHG_CTRL0_EN_CHG_BIT      (1 << 5) // Default = 1
#define CHG_CTRL0_FORCE_IBATDIS   (1 << 6)
#define CHG_CTRL0_EN_AUTO_IBATDIS (1 << 7) // Default = 1

// REG0x1E: Charger Status 1
#define CHG_STAT1_CHG_STAT_MASK   (0b11 << 3)
#define CHG_STAT1_CHG_STAT_NOT_CHG (0b00 << 3) // Not Charging or Terminated
#define CHG_STAT1_CHG_STAT_CC_CHG (0b01 << 3) // Trickle, Precharge, or Fast Charge (CC)
#define CHG_STAT1_CHG_STAT_CV_CHG (0b10 << 3) // Taper Charge (CV)
#define CHG_STAT1_CHG_STAT_TOP_OFF (0b11 << 3)// Top-off Timer Active
#define CHG_STAT1_VBUS_STAT_MASK  (0b111 << 0) // VBUS Status bits (See Datasheet Table 8-27 for BQ25628 meanings)
#define CHG_STAT1_VBUS_STAT_NO_INPUT (0b000 << 0)
#define CHG_STAT1_VBUS_STAT_UNKNOWN  (0b100 << 0) // Default IINDPM
#define CHG_STAT1_VBUS_STAT_OTG      (0b111 << 0) // Boost OTG mode

// REG0x1F: FAULT Status 0
#define FAULT_STAT0_TS_STAT_MASK         (0b111 << 0)
#define FAULT_STAT0_TSHUT_STAT_BIT       (1 << 3)
#define FAULT_STAT0_OTG_FAULT_STAT_BIT   (1 << 4)
#define FAULT_STAT0_SYS_FAULT_STAT_BIT   (1 << 5)
#define FAULT_STAT0_BAT_FAULT_STAT_BIT   (1 << 6) // Includes BATOVP, BATOCP
#define FAULT_STAT0_VBUS_FAULT_STAT_BIT  (1 << 7) // Includes VBUS OVP, Sleep

// REG0x1D: Charger Status 0
#define CHG_STAT0_WD_STAT_BIT            (1 << 0)
#define CHG_STAT0_SAFETY_TMR_STAT_BIT    (1 << 1)
#define CHG_STAT0_VINDPM_STAT_BIT        (1 << 2)
#define CHG_STAT0_IINDPM_STAT_BIT        (1 << 3)
#define CHG_STAT0_VSYS_STAT_BIT          (1 << 4) // VSYSMIN Regulation Status
#define CHG_STAT0_TREG_STAT_BIT          (1 << 5)
#define CHG_STAT0_ADC_DONE_STAT_BIT      (1 << 6) // One-shot only

// REG0x22: FAULT Flag 0 (Mirrors FAULT_Status_0 bits, read to clear)
#define FAULT_FLAG0_TS_FLAG_BIT         (1 << 0)
#define FAULT_FLAG0_TSHUT_FLAG_BIT      (1 << 3)
#define FAULT_FLAG0_OTG_FAULT_FLAG_BIT  (1 << 4)
#define FAULT_FLAG0_SYS_FAULT_FLAG_BIT  (1 << 5)
#define FAULT_FLAG0_BAT_FAULT_FLAG_BIT  (1 << 6)
#define FAULT_FLAG0_VBUS_FAULT_FLAG_BIT (1 << 7)

// REG0x38: Part Information
#define PART_INFO_DEV_REV_MASK        (0b111 << 0)
#define PART_INFO_PN_MASK             (0b111 << 3)
#define PART_INFO_PN_BQ25628          (0b010 << 3)

// --- HAL Handle Extern Declarations ---
extern I2C_HandleTypeDef hi2c1;
// --------------------------------------

// --- Local Variables ---
static bool bms_initialized = false;
static uint32_t last_poll_time = 0;
static volatile bool bms_interrupt_flag = false; // Set by ISR
static bms_status_t last_valid_status = { .data_valid = false };
// ---------------------

// --- Helper Function Prototypes ---
static bool bms_task_read_register_byte(uint8_t reg_addr, uint8_t *data);
static bool bms_task_write_register_byte(uint8_t reg_addr, uint8_t data);
static bool bms_task_read_register_word(uint8_t reg_addr_lsb, uint16_t *data);
static bool bms_task_write_register_word(uint8_t reg_addr_lsb, uint16_t data);
static bms_charge_state_t decode_charge_status(uint8_t status_reg1);
static float decode_adc_value_signed(uint16_t raw_adc, float lsb_value, float offset);
static float decode_adc_value_unsigned(uint16_t raw_adc, float lsb_value, float offset);

// --- Function Definitions ---

/**
 * @brief Reads a single byte from a specific register from the BQ25628.
 */
static bool bms_task_read_register_byte(uint8_t reg_addr, uint8_t *data) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, BQ25629_I2C_ADDR, reg_addr,
                                               I2C_MEMADD_SIZE_8BIT, data, 1, BMS_I2C_TIMEOUT);
    if (status != HAL_OK) {
         printf("BMS I2C Read Byte Error: Addr 0x%02X, Status %d\r\n", reg_addr, status);
    }
    return (status == HAL_OK);
}

/**
 * @brief Writes a single byte to a specific register to the BQ25628.
 */
static bool bms_task_write_register_byte(uint8_t reg_addr, uint8_t data) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, BQ25629_I2C_ADDR, reg_addr,
                                                I2C_MEMADD_SIZE_8BIT, &data, 1, BMS_I2C_TIMEOUT);
     if (status != HAL_OK) {
         printf("BMS I2C Write Byte Error: Addr 0x%02X, Data 0x%02X, Status %d\r\n", reg_addr, data, status);
     }
    return (status == HAL_OK);
}

/**
 * @brief Reads a 16-bit word (little-endian) from the BQ25628 starting at the LSB address.
 */
static bool bms_task_read_register_word(uint8_t reg_addr_lsb, uint16_t *data) {
    uint8_t bytes[2];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, BQ25629_I2C_ADDR, reg_addr_lsb,
                                               I2C_MEMADD_SIZE_8BIT, bytes, 2, BMS_I2C_TIMEOUT);
    if (status == HAL_OK) {
        *data = ((uint16_t)bytes[1] << 8) | bytes[0]; // Combine LSB and MSB
        return true;
    } else {
        printf("BMS I2C Read Word Error: Addr 0x%02X, Status %d\r\n", reg_addr_lsb, status);
    }
    return false;
}

/**
 * @brief Writes a 16-bit word (little-endian) to the BQ25628 starting at the LSB address.
 */
static bool bms_task_write_register_word(uint8_t reg_addr_lsb, uint16_t data) {
    uint8_t bytes[2];
    bytes[0] = (uint8_t)(data & 0xFF);         // LSB
    bytes[1] = (uint8_t)((data >> 8) & 0xFF); // MSB
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, BQ25629_I2C_ADDR, reg_addr_lsb,
                                                 I2C_MEMADD_SIZE_8BIT, bytes, 2, BMS_I2C_TIMEOUT);
    if (status != HAL_OK) {
        printf("BMS I2C Write Word Error: Addr 0x%02X, Data 0x%04X, Status %d\r\n", reg_addr_lsb, data, status);
    }
    return (status == HAL_OK);
}

/**
 * @brief Decodes the charge status bits from REG0x1E.
 * Requires data_structures.h to be updated with BMS_CHG_TAPER and BMS_CHG_TOP_OFF.
 */
static bms_charge_state_t decode_charge_status(uint8_t status_reg1) {
    uint8_t chg_stat = status_reg1 & CHG_STAT1_CHG_STAT_MASK;
    switch (chg_stat) {
        // Note: BQ25628 datasheet Figure 8-2 lumps Precharge with Fast Charge (CC) in the diagram,
        // but CHG_STAT description Table 8-27 maps 01b to CC phase (Trickle/Pre/Fast).
        case CHG_STAT1_CHG_STAT_CC_CHG:  return BMS_CHG_FASTCHARGE; // Could also be Precharge/Trickle
        case CHG_STAT1_CHG_STAT_CV_CHG:  return BMS_CHG_TAPER;      // Taper Charge (CV phase)
        case CHG_STAT1_CHG_STAT_TOP_OFF: return BMS_CHG_TOP_OFF;    // Top-off timer active
        case CHG_STAT1_CHG_STAT_NOT_CHG:
        default:                         return BMS_CHG_NOT_CHARGING; // Not Charging or Charge Done/Terminated
    }
    // To distinguish Precharge from Fastcharge, you might need to check VBAT against VBAT_LOWV threshold.
    // To distinguish Charge Done from Not Charging (Disabled), check EN_CHG bit and termination status/current.
}

// Decode signed ADC values (e.g., IBUS, IBAT, TDIE) - assumes 2's complement
static float decode_adc_value_signed(uint16_t raw_adc, float lsb_value, float offset) {
    // Check if the raw value indicates an invalid conversion (e.g., 0x8000 for IBAT polarity change)
    // The IBAT ADC uses bits 15:2. If raw_adc corresponds to 0x8000 in the original 16 bits,
    // the value read from bits 15:2 would be 0x2000.
    if (raw_adc == 0x2000) { // Specific check for IBAT error code
         // printf("BMS ADC Warning: Invalid IBAT reading (0x%04X)\r\n", raw_adc);
         // Return a specific error value or handle as needed
         return -9999.0f; // Example error value
    }

    // Sign extend based on the actual number of bits used by the register field
    // For IBAT (bits 15:2 -> 14 bits effective, MSB is bit 15)
    // For IBUS (bits 15:1 -> 15 bits effective, MSB is bit 15)
    // For TDIE (bits 11:0 -> 12 bits effective, MSB is bit 11)
    // Assuming raw_adc contains *only* the relevant bits already shifted down:
    int16_t signed_adc;
    if (raw_adc & (1 << 13)) { // Check sign bit (assuming 14-bit IBAT value, shifted down)
         signed_adc = raw_adc | ~((1 << 14) - 1); // Manual sign extension for 14 bits
    } else {
         signed_adc = raw_adc;
    }
    // This sign extension logic needs careful verification based on which bits are passed in raw_adc

    // Let's assume raw_adc holds the *full 16-bit register word* for simplicity here,
    // and the shifts happen during calculation.
    signed_adc = (int16_t)raw_adc; // Cast full word to signed

    return ((float)signed_adc * lsb_value) + offset;
}

// Decode unsigned ADC values (e.g., VBUS, VBAT, VSYS, VPMID, TS)
static float decode_adc_value_unsigned(uint16_t raw_adc, float lsb_value, float offset) {
    // raw_adc here should contain *only* the relevant bits, shifted down appropriately.
    return ((float)raw_adc * lsb_value) + offset;
}


// --- Public Function Definitions ---

/**
 * @brief Reads a specific register from the BQ25628 (single byte).
 * Use bms_task_read_register_word for 16-bit registers.
 */
bool bms_task_read_register(uint8_t reg_addr, uint8_t *data) {
    return bms_task_read_register_byte(reg_addr, data);
}

/**
 * @brief Writes a specific register to the BQ25628 (single byte).
 * Use bms_task_write_register_word for 16-bit registers.
 */
bool bms_task_write_register(uint8_t reg_addr, uint8_t data) {
     return bms_task_write_register_byte(reg_addr, data);
}


/**
 * @brief Initializes the BMS task (I2C peripheral, GPIOs for INT/CHG/RESET).
 */
bool bms_task_init(void) {
    uint8_t data_byte;
    uint16_t data_word;
    bool success = true;

    printf("BMS Info: Initializing BQ25628...\r\n");

    // Assuming I2C1, GPIOs (PB8 Input=STAT, PH3 EXTI=INT, PA15 Output=?) are initialized by CubeMX

    // 1. Check communication by reading Part/Revision register
    if (!bms_task_read_register_byte(BQ_REG_PART_INFORMATION, &data_byte)) {
        printf("BMS Error: Failed to read part info register\r\n");
        return false;
    }
    printf("BMS Info: BQ2562x Part Info: 0x%02X (PN=0x%01X, Rev=0x%01X)\r\n",
           data_byte, (data_byte & PART_INFO_PN_MASK) >> 3, (data_byte & PART_INFO_DEV_REV_MASK));

    // Check if it's actually BQ25628
    if ((data_byte & PART_INFO_PN_MASK) != PART_INFO_PN_BQ25628) {
         printf("BMS Warning: Detected Part Number is not BQ25628 (PN=0x%01X). Configuration might be incorrect.\r\n", (data_byte & PART_INFO_PN_MASK)>>3);
    }

    // --- Configure Charging Parameters ---
    // IMPORTANT: Adjust these values based on your specific battery datasheet!

    // Charge Voltage Limit: 4.2V
    // Reg Addr: 0x04/0x05, Bits 11:3, Step: 10mV, Base: 3500mV
    // Code = (4200-3500)/10 = 70 = 0x46. Shifted left by 3 -> 0x230
    data_word = 0x0230;
    success &= bms_task_write_register_word(BQ_REG_CHARGE_VOLTAGE_LIMIT, data_word);
    if (!success) { printf("BMS Error: Failed to set charge voltage\r\n"); goto init_fail; }
    printf("BMS Cfg: Charge Voltage Limit set to 4200mV\r\n");

    // Charge Current Limit: 1000mA (Adjust based on battery C-rate!)
    // Reg Addr: 0x02/0x03, Bits 10:5, Step: 40mA, Base: 40mA
    // Code = (1000-40)/40 = 24 = 0x18. Shifted left by 5 -> 0x300
    data_word = 0x0300;
    success &= bms_task_write_register_word(BQ_REG_CHARGE_CURRENT_LIMIT, data_word);
    if (!success) { printf("BMS Error: Failed to set charge current\r\n"); goto init_fail; }
     printf("BMS Cfg: Charge Current Limit set to 1000mA\r\n");

    // Precharge Current: 100mA (~10% of ICHG)
    // Reg Addr: 0x10/0x11, Bits 7:3, Step: 10mA, Base: 10mA
    // Code = (100-10)/10 = 9 = 0x09. Shifted left by 3 -> 0x48
    data_word = 0x0048;
    success &= bms_task_write_register_word(BQ_REG_PRECHARGE_CONTROL, data_word);
    if (!success) { printf("BMS Error: Failed to set precharge current\r\n"); goto init_fail; }
     printf("BMS Cfg: Precharge Current set to 100mA\r\n");

    // Termination Current: 100mA (~10% of ICHG)
    // Reg Addr: 0x12/0x13, Bits 7:2, Step: 5mA, Base: 5mA
    // Code = (100-5)/5 = 19 = 0x13. Shifted left by 2 -> 0x4C
    data_word = 0x004C;
    success &= bms_task_write_register_word(BQ_REG_TERMINATION_CONTROL, data_word);
    if (!success) { printf("BMS Error: Failed to set termination current\r\n"); goto init_fail; }
     printf("BMS Cfg: Termination Current set to 100mA\r\n");

    // Minimum System Voltage: 3.52V (Default)
    // Reg Addr: 0x0E/0x0F, Bits 11:6, Step: 80mV, Base: 2560mV
    // Code = (3520-2560)/80 = 12 = 0x0C. Shifted left by 6 -> 0x300 -> Reg Value 0x0B00
    data_word = 0x0B00;
    success &= bms_task_write_register_word(BQ_REG_MINIMAL_SYSTEM_VOLTAGE, data_word);
     if (!success) { printf("BMS Error: Failed to set min sys voltage\r\n"); goto init_fail; }
     printf("BMS Cfg: Min System Voltage set to 3520mV\r\n");

    // Input Current Limit: Default = 3.2A (A0h), BQ25628 default uses ILIM pin (REG0x19[2]=1)
    // Keep default behavior (use ILIM pin) unless specific I2C limit is required.
    // If setting via I2C: Clear REG0x19[2], then write to REG0x06/07.
    printf("BMS Cfg: Input Current Limit controlled by ILIM pin (default)\r\n");

    // Configure Timers & Watchdog: Use defaults
    // REG0x15: EN_SAFETY_TMRS=1, TMR2X_EN=1, PRECHG_TMR=0(2.5h), CHG_TMR=0(14.5h) -> 0x5C (default)
    success &= bms_task_write_register_byte(BQ_REG_CHARGE_TIMER_CONTROL, 0x5C);
    if (!success) { printf("BMS Error: Failed to set timer control\r\n"); goto init_fail; }
    printf("BMS Cfg: Timers set to default (Safety Enabled, 2.5h pre, 14.5h fast)\r\n");

    // REG0x16: WATCHDOG=01(50s), EN_CHG=1, EN_AUTO_IBATDIS=1 -> 0xA1 (default)
    success &= bms_task_write_register_byte(BQ_REG_CHARGER_CONTROL_0, 0xA1);
    if (!success) { printf("BMS Error: Failed to set charger control 0\r\n"); goto init_fail; }
    printf("BMS Cfg: Watchdog enabled (50s), Charging enabled\r\n");

    // Configure Interrupt Masks: Enable desired interrupts (e.g., Faults, WD)
    // Example: Enable all Fault interrupts (TS, TSHUT, OTG, SYS, BAT, VBUS) -> REG0x25 = 0x00
    success &= bms_task_write_register_byte(BQ_REG_FAULT_MASK_0, 0x00);
    if (!success) { printf("BMS Error: Failed to set fault mask\r\n"); goto init_fail; }
    // Example: Enable Watchdog interrupt -> REG0x23 = 0xFE (clear bit 0)
    success &= bms_task_write_register_byte(BQ_REG_CHARGER_MASK_0, 0xFE);
     if (!success) { printf("BMS Error: Failed to set charger mask 0\r\n"); goto init_fail; }
    printf("BMS Cfg: All Fault and Watchdog interrupts enabled\r\n");

    // Configure ADC: Enable continuous, 9-bit (default), avg disabled
    // REG0x26: ADC_EN=1, ADC_RATE=0 (Cont), ADC_SAMPLE=3 (9bit), ADC_AVG=0, ADC_AVG_INIT=0 -> 0xC0
    success &= bms_task_write_register_byte(BQ_REG_ADC_CONTROL, 0xC0);
    if (!success) { printf("BMS Error: Failed to enable ADC\r\n"); goto init_fail; }
    // Ensure desired ADC channels are enabled (REG0x27 - default is all enabled=0)
    success &= bms_task_write_register_byte(BQ_REG_ADC_FUNC_DISABLE_0, 0x00); // Enable all channels
    if (!success) { printf("BMS Error: Failed to enable ADC channels\r\n"); goto init_fail; }
    printf("BMS Cfg: ADC enabled (Continuous, 9-bit)\r\n");

    // Final check
    if (!success) {
init_fail: // Label for goto on failure
        printf("BMS Error: Initialization failed.\r\n");
        return false;
    }

    printf("BMS Info: BQ25628 Initialized and Configured Successfully.\r\n");
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
    uint8_t status0_byte=0, status1_byte=0, fault0_byte=0;
    uint16_t vbus_adc_raw=0, ibat_adc_raw=0, vbat_adc_raw=0, vsys_adc_raw=0; // Add others as needed
    bool read_success = true;
    bool adc_success = true;


    if (!bms_initialized || p_bms_status == NULL) {
        return;
    }

    uint32_t current_time = HAL_GetTick();
    bool check_now = (current_time - last_poll_time) >= BMS_POLL_INTERVAL_MS;
    bool interrupt_pending = bms_interrupt_flag; // Atomically read and clear flag
    bms_interrupt_flag = false;

    if (check_now || interrupt_pending) {
        last_poll_time = current_time;

        // --- Clear flags by reading flag registers if interrupt occurred ---
        // This ensures flags are cleared even if we fail to read status/ADC later
        if (interrupt_pending) {
            uint8_t dummy_flags;
            bms_task_read_register_byte(BQ_REG_CHARGER_FLAG_0, &dummy_flags);
            bms_task_read_register_byte(BQ_REG_CHARGER_FLAG_1, &dummy_flags);
            bms_task_read_register_byte(BQ_REG_FAULT_FLAG_0, &dummy_flags);
            // printf("BMS Debug: Interrupt processed, flags cleared.\r\n");
        }

        // --- Read Status and Fault Registers ---
        read_success &= bms_task_read_register_byte(BQ_REG_CHARGER_STATUS_0, &status0_byte);
        read_success &= bms_task_read_register_byte(BQ_REG_CHARGER_STATUS_1, &status1_byte);
        read_success &= bms_task_read_register_byte(BQ_REG_FAULT_STATUS_0, &fault0_byte);

        // --- Read ADC Values (Assuming ADC is enabled continuously) ---
        // Read the registers for the channels enabled in REG0x27
        adc_success &= bms_task_read_register_word(BQ_REG_VBUS_ADC, &vbus_adc_raw);
        adc_success &= bms_task_read_register_word(BQ_REG_IBAT_ADC, &ibat_adc_raw); // Reads Battery Current (Charge/Discharge)
        adc_success &= bms_task_read_register_word(BQ_REG_VBAT_ADC, &vbat_adc_raw);
        adc_success &= bms_task_read_register_word(BQ_REG_VSYS_ADC, &vsys_adc_raw);
        // Read other ADCs if needed (IBUS, VPMID, TS, TDIE)


        if (read_success) {
            // --- Decode Status ---
            p_bms_status->charge_status = decode_charge_status(status1_byte);
            p_bms_status->fault_flags = fault0_byte; // Store raw fault flags (REG0x1F)
            p_bms_status->interrupt_active = interrupt_pending; // Store if INT triggered this check

            if (adc_success) {
                 // Decode ADC values using datasheet LSBs/Offsets
                 // VBUS: Reg Addr 0x2C/2D, Bits 14:2 used, LSB=3.97mV, Unsigned
                 p_bms_status->vbus_voltage_mv = decode_adc_value_unsigned(vbus_adc_raw >> 2, 3.97f, 0.0f);

                 // IBAT: Reg Addr 0x2A/2B, Bits 15:2 used, LSB=4mA, Signed (2's Comp)
                 // Need to handle sign extension correctly if decode_adc_value_signed doesn't.
                 // Pass only relevant bits after shifting:
                 uint16_t ibat_val = ibat_adc_raw >> 2; // Keep bits 15:2 shifted down to 13:0
                 p_bms_status->charge_current_ma = decode_adc_value_signed(ibat_val, 4.0f, 0.0f); // Pass shifted value

                 // VBAT: Reg Addr 0x30/31, Bits 12:1 used, LSB=1.99mV, Unsigned
                 p_bms_status->battery_voltage_mv = decode_adc_value_unsigned(vbat_adc_raw >> 1, 1.99f, 0.0f);

                 // VSYS: Reg Addr 0x32/33, Bits 12:1 used, LSB=1.99mV, Unsigned
                 p_bms_status->system_voltage_mv = decode_adc_value_unsigned(vsys_adc_raw >> 1, 1.99f, 0.0f);

                 p_bms_status->data_valid = true;
            } else {
                 printf("BMS Warning: Failed to read ADC values.\r\n");
                 p_bms_status->data_valid = false; // ADC data invalid
                 p_bms_status->vbus_voltage_mv = 0;
                 p_bms_status->charge_current_ma = 0;
                 p_bms_status->battery_voltage_mv = 0;
                 p_bms_status->system_voltage_mv = 0;
            }


            // Store locally (even if ADC failed, status/fault might be valid)
            memcpy(&last_valid_status, p_bms_status, sizeof(bms_status_t));

            // --- Handle Faults / Status ---
            if (status0_byte & CHG_STAT0_WD_STAT_BIT) { // Check Watchdog Status Bit
                 printf("BMS Warning: Watchdog timer expired! Resetting timer.\r\n");
                 uint8_t ctrl0_val;
                 if (bms_task_read_register_byte(BQ_REG_CHARGER_CONTROL_0, &ctrl0_val)) {
                     ctrl0_val |= CHG_CTRL0_WD_RST_BIT; // Set WD_RST bit
                     if (!bms_task_write_register_byte(BQ_REG_CHARGER_CONTROL_0, ctrl0_val)) {
                         printf("BMS Error: Failed to write watchdog reset.\r\n");
                     }
                     // Note: WD_RST bit self-clears, WD_STAT bit clears after WD_RST write.
                 } else {
                      printf("BMS Error: Failed to read control reg before watchdog reset.\r\n");
                 }
            }

            // Check fault bits in fault0_byte (REG0x1F)
            if (fault0_byte & FAULT_STAT0_VBUS_FAULT_STAT_BIT) {
                // printf("BMS Fault: VBUS OVP or Sleep active.\r\n");
            }
            if (fault0_byte & FAULT_STAT0_BAT_FAULT_STAT_BIT) {
                // printf("BMS Fault: Battery OVP or OCP active.\r\n");
            }
            if (fault0_byte & FAULT_STAT0_SYS_FAULT_STAT_BIT) {
                 // printf("BMS Fault: System Short or OVP active.\r\n");
            }
            if (fault0_byte & FAULT_STAT0_TSHUT_STAT_BIT) {
                 // printf("BMS Fault: Thermal Shutdown active.\r\n");
            }
            if ((fault0_byte & FAULT_STAT0_TS_STAT_MASK) != 0) { // Check if any TS fault/status other than normal
                 // printf("BMS Fault/Status: TS temperature out of normal range (TS_STAT=0x%X).\r\n", fault0_byte & FAULT_STAT0_TS_STAT_MASK);
            }


            // --- Debug Print ---
            //  if (p_bms_status->data_valid) {
            //      printf("BMS: VBUS=%.0fmV, VBAT=%.0fmV, VSYS=%.0fmV, IBAT=%.0fmA, ChgStat=%d, VBUSStat=0x%X, Fault=0x%02X\r\n",
            //             p_bms_status->vbus_voltage_mv, p_bms_status->battery_voltage_mv, p_bms_status->system_voltage_mv,
            //             p_bms_status->charge_current_ma, p_bms_status->charge_status, (status1_byte & CHG_STAT1_VBUS_STAT_MASK), fault0_byte);
            //  }
            // -------------------

        } else {
            printf("BMS Error: Failed to read status/fault registers.\r\n");
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
    // The main loop should read the Flag registers (0x20, 0x21, 0x22) to determine the cause
    // and clear the flags.
    bms_interrupt_flag = true;
}

/**
 * @brief Toggles the BMS Reset Pin (PA15). (Not a BQ25628 pin, assumes external control)
 */
void bms_task_set_reset(bool assert_reset) {
    // Forward call to system control (assuming this controls an external reset mechanism)
    system_control_set_bms_reset(assert_reset);
}

/**
 * @brief Reads the state of the BMS Charge Status Pin (PB8).
 * Corresponds to STAT pin on BQ25628.
 */
bool bms_task_read_charge_status_pin(void) {
    // STAT pin (REG0x1D[1]) is open-drain.
    // LOW = Charging in progress (CC, CV, Precharge, Trickle)
    // HIGH = Not charging (Includes Charge Done, Charge Disabled, No Adapter, OTG mode)
    // Blinking (1Hz) = Charge Suspend (TS Fault, Safety Timer Expire, etc.)
    // Assumes PB8 (BMS_CHG_Pin) is connected to STAT and configured as Input with external pull-up.
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(BMS_CHG_GPIO_Port, BMS_CHG_Pin);

    // This function only gives instantaneous state. Blinking needs temporal analysis.
    // Check REG0x1D/1E/1F for definitive status.
    return (pin_state == GPIO_PIN_SET); // Return true if High (Generally Not Charging/Done/Idle)
}


/**
 * @brief Get the last successfully read BMS status.
 */
void bms_task_get_last_status(bms_status_t *p_bms_status) {
     if (p_bms_status != NULL) {
        // Consider critical section if interrupts can modify last_valid_status
        // uint32_t primask_bit = __get_PRIMASK(); // Save interrupt state
        // __disable_irq(); // Disable interrupts
        memcpy(p_bms_status, &last_valid_status, sizeof(bms_status_t));
        // if (!primask_bit) { // Restore interrupt state only if it was enabled
        //     __enable_irq(); // Enable interrupts
        // }
    }
}
