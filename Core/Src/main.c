/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "protocol.h" // Includes custom_types, gps, bq25629
#include "nand_m79a.h"  // Includes nand_m79a_lld for Ret_Success etc.
#include "wifi_ble.h"
/* Do not include usbx_device.h if using app_usbx_device.c for init */

/* Include BMI2 driver files */
#include "bmi2.h"
#include "bmi270.h"
#include "bmi2_defs.h" // Include for BMI2 defines like BMI2_CONT_MODE
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Use PAGE_DATA_SIZE from LLD header
#define LOG_PAGE_SIZE PAGE_DATA_SIZE // Should be 2048 based on nand_m79a_lld.h
#define BMI270_I2C_ADDR BMI2_I2C_PRIM_ADDR // Or BMI2_I2C_SEC_ADDR depending on hardware
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart4;

PCD_HandleTypeDef hpcd_USB_DRD_FS;

/* USER CODE BEGIN PV */
/* Global variables */
volatile uint8_t liveStreamingEnabled = 1;  // Enable live streaming by default
uint32_t system_start_tick;

/* BMI270 Device Structure */
static struct bmi2_dev bmi270_dev;

/* NAND log circular buffer configuration */
static uint8_t logPageBuffer[LOG_PAGE_SIZE];
static uint16_t logBufIndex = 0;
// Using NUM_BLOCKS and NUM_PAGES_PER_BLOCK from nand_m79a_lld.h
static uint32_t currentLogBlock = 0; // Track current block index
static uint32_t currentLogPageInBlock = 0; // Track current page within block

/* No need to extern pool buffer if MX_USBX_Device_Init takes no arguments */
/* extern UCHAR ux_device_byte_pool_buffer[]; */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_UART4_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_ICACHE_Init(void);
static void MX_FLASH_Init(void);
/* USER CODE BEGIN PFP */
float Read_Temperature_C(void); // Add prototype for local function
/* Helper function Get_Current_Logical_Page_Index removed as it wasn't used */
static void Advance_NAND_Page(void); // Helper to advance NAND page/block index

/* BMI270 HAL Wrapper Function Prototypes */
static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);
static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);
static void bmi2_delay_us(uint32_t period, void *intf_ptr);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Function for temperature reading from ADC */
float Read_Temperature_C(void) {
    HAL_StatusTypeDef adc_status;

    // Start ADC conversion
    adc_status = HAL_ADC_Start(&hadc1);
    if (adc_status != HAL_OK) {
        return -999.0f;
    }

    // Poll for conversion completion
    adc_status = HAL_ADC_PollForConversion(&hadc1, 10);
    if (adc_status == HAL_OK) {
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        // Convert ADC for TMP235A4
        // Assume VREF+ is connected to Vdda (typically 3.3V)
        // Vref value might need adjustment based on actual Vdda
        float vref = 3.3f;
        float v_out_mv = ((float)adc_val / 4095.0f) * vref * 1000.0f;
        // TMP235 Transfer function: Vout = (10 mV/°C * T) + 500 mV
        // T = (Vout - 500 mV) / (10 mV/°C)
        float temperature = (v_out_mv - 500.0f) / 10.0f;
        return temperature;
    } else {
        HAL_ADC_Stop(&hadc1);
        return -999.0f;
    }
}

// Helper to advance NAND page/block index, handling wrap-around and erase
static void Advance_NAND_Page(void) {
    currentLogPageInBlock++;
    if (currentLogPageInBlock >= NUM_PAGES_PER_BLOCK) {
        currentLogPageInBlock = 0;
        currentLogBlock++;
        if (currentLogBlock >= NUM_BLOCKS) {
            currentLogBlock = 0; // Wrap around blocks
        }
        // Erase the *next* block before writing to its first page
        // Add Bad Block Management here! Skip bad blocks.
        PhysicalAddrs eraseAddr = { .block = currentLogBlock }; // Block erase only needs block number
        NAND_ReturnType erase_status = NAND_Block_Erase(&hspi1, &eraseAddr);
        if (erase_status != Ret_Success) {
            // Consider handling erase error (e.g., marking block as bad and trying next)
            Error_Handler();
        }
    }
}

/* HAL I2C Read Wrapper for BMI270 */
static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef*)intf_ptr;
    HAL_StatusTypeDef status;

    // BMI270 uses 7-bit address, left-shifted by 1 for HAL functions
    uint16_t dev_addr = BMI270_I2C_ADDR << 1;

    status = HAL_I2C_Mem_Read(hi2c, dev_addr, (uint16_t)reg_addr, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY);

    return (status == HAL_OK) ? BMI2_OK : BMI2_E_COM_FAIL;
}

/* HAL I2C Write Wrapper for BMI270 */
static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef*)intf_ptr;
    HAL_StatusTypeDef status;

    // BMI270 uses 7-bit address, left-shifted by 1 for HAL functions
    uint16_t dev_addr = BMI270_I2C_ADDR << 1;

    status = HAL_I2C_Mem_Write(hi2c, dev_addr, (uint16_t)reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, len, HAL_MAX_DELAY);

    return (status == HAL_OK) ? BMI2_OK : BMI2_E_COM_FAIL;
}

/* HAL Delay Wrapper for BMI270 */
static void bmi2_delay_us(uint32_t period, void *intf_ptr) {
    /* Implement microsecond delay using appropriate timer or HAL_Delay */
    /* HAL_Delay provides millisecond delay, need a more precise method for us */
    /* Example using HAL_Delay for approx: */
    if (period < 1000) {
        // For small delays, a busy wait or NOP loop might be needed if no us timer
        // Warning: This is not accurate for very short delays and depends on clock speed
        volatile uint32_t wait_loop_index = (period * (SystemCoreClock / 1000000U)) / 4; // Rough estimate
         while(wait_loop_index != 0U)
         {
           wait_loop_index--;
         }
    } else {
        HAL_Delay(period / 1000);
    }
    UX_PARAMETER_NOT_USED(intf_ptr); // Parameter not used in this simple implementation
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  int8_t bmi2_rslt;
  uint8_t sensor_list[] = { BMI2_ACCEL, BMI2_GYRO }; // Enable Accel and Gyro
  struct bmi2_sens_config sens_cfg;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_UART4_Init();
  MX_USB_PCD_Init();
  MX_ICACHE_Init();
  MX_FLASH_Init();
  MX_USBX_Device_Init();
  /* USER CODE BEGIN 2 */


  /* Initialize Device Drivers */
  system_start_tick = HAL_GetTick();

  // Initialize BMI270
  bmi270_dev.chip_id = BMI270_CHIP_ID; // Set expected chip ID
  bmi270_dev.intf = BMI2_I2C_INTF;     // Set interface type
  bmi270_dev.read = bmi2_i2c_read;     // Assign HAL read function
  bmi270_dev.write = bmi2_i2c_write;    // Assign HAL write function
  bmi270_dev.delay_us = bmi2_delay_us; // Assign HAL delay function
  bmi270_dev.intf_ptr = &hi2c1;        // Pass I2C handle to wrappers
  bmi270_dev.read_write_len = 32;      // Max I2C read/write length (adjust if needed)
  bmi270_dev.config_file_ptr = NULL;   // Use internal config file (bmi270_config_file)

  // Initialize BMI2 sensor library (loads config file)
  bmi2_rslt = bmi270_init(&bmi270_dev);
  if (bmi2_rslt != BMI2_OK) {
      Error_Handler();
  }

  // Enable Accel and Gyro sensors
  bmi2_rslt = bmi270_sensor_enable(sensor_list, sizeof(sensor_list)/sizeof(sensor_list[0]), &bmi270_dev);
   if (bmi2_rslt != BMI2_OK) {
      Error_Handler();
  }

   // Configure Accel
  sens_cfg.type = BMI2_ACCEL;
  bmi2_rslt = bmi2_get_sensor_config(&sens_cfg, 1, &bmi270_dev); // Read current config first
  if (bmi2_rslt != BMI2_OK) { Error_Handler(); }
  sens_cfg.cfg.acc.odr = BMI2_ACC_ODR_100HZ;         // Set ODR to 100Hz
  sens_cfg.cfg.acc.range = BMI2_ACC_RANGE_2G;       // Set range (e.g., 2G)
  sens_cfg.cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;      // Set bandwidth param (Average 4 samples)
  sens_cfg.cfg.acc.filter_perf = BMI2_PERF_OPT_MODE; // Corrected macro (Optimized performance mode)
  bmi2_rslt = bmi2_set_sensor_config(&sens_cfg, 1, &bmi270_dev);
  if (bmi2_rslt != BMI2_OK) { Error_Handler(); }

  // Configure Gyro
  sens_cfg.type = BMI2_GYRO;
  bmi2_rslt = bmi2_get_sensor_config(&sens_cfg, 1, &bmi270_dev); // Read current config first
  if (bmi2_rslt != BMI2_OK) { Error_Handler(); }
  sens_cfg.cfg.gyr.odr = BMI2_GYR_ODR_100HZ;         // Set ODR to 100Hz
  sens_cfg.cfg.gyr.range = BMI2_GYR_RANGE_2000;      // Set range (e.g., 2000 dps)
  sens_cfg.cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;       // Set bandwidth param (Normal mode)
  sens_cfg.cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE; // Set noise performance (Power optimized)
  sens_cfg.cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE; // Corrected macro (Optimized performance mode)
  bmi2_rslt = bmi2_set_sensor_config(&sens_cfg, 1, &bmi270_dev);
  if (bmi2_rslt != BMI2_OK) { Error_Handler(); }

  // Initialize BQ25629 BMS
  if(BQ25629_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
  }

  // Initialize NAND Flash
  if(NAND_Init(&hspi1) != Ret_Success) {
        Error_Handler();
  }
  // Erase the first block on startup
  PhysicalAddrs eraseAddr = { .block = currentLogBlock }; // Block erase only needs block number
  NAND_ReturnType erase_status = NAND_Block_Erase(&hspi1, &eraseAddr);
  if (erase_status != Ret_Success) {
        Error_Handler();
  }

  // Initialize WiFi/BLE Module
  if(WiFiBLE_Init(&hspi1) != HAL_OK) {
        Error_Handler();
  }

  // Initialize Protocol module
  Protocol_Init();

  // Initialize GPS module
  GPS_Init();


  /* Main loop timing variables */
  uint32_t lastIMUTick = HAL_GetTick();
  uint32_t lastEnvTick = HAL_GetTick();

  // Sensor data variables
  struct bmi2_sens_data sensor_data = { { 0 } }; // Use standard BMI2 struct
  BMI270_Data imuData = {0}; // Keep custom struct for protocol/logging if needed
  float temperature = 0.0f;
  float battery_voltage = 0.0f;
  GPS_Fix_t gpsFix = {0};

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    /* --- Sensor Polling --- */

    /* Poll BMI270 IMU at 100 Hz (every 10 ms) */
    if(now - lastIMUTick >= 10) {
        lastIMUTick = now;

        // Read Accel and Gyro data using bmi2_get_sensor_data
        bmi2_rslt = bmi2_get_sensor_data(&sensor_data, &bmi270_dev);
        if(bmi2_rslt == BMI2_OK)
        {
            // Populate custom imuData struct from standard sensor_data
            imuData.ax = sensor_data.acc.x;
            imuData.ay = sensor_data.acc.y;
            imuData.az = sensor_data.acc.z;
            imuData.gx = sensor_data.gyr.x;
            imuData.gy = sensor_data.gyr.y;
            imuData.gz = sensor_data.gyr.z;
            imuData.timestamp = now - system_start_tick; // Use HAL ticks for timestamp

            if(liveStreamingEnabled) {
                Protocol_SendIMU(&imuData);
            }
        } else {
            // Handle IMU read error
        }
    }

    /* Poll environmental sensors at 1 Hz */
    if(now - lastEnvTick >= 1000) {
        lastEnvTick = now;
        temperature = Read_Temperature_C();
        battery_voltage = BQ25629_ReadBatteryVoltage(&hi2c1);
        GPS_GetLatestFix(&gpsFix); // Tries to update gpsFix if new data

        if(liveStreamingEnabled) {
            Protocol_SendEnvironmental(temperature, battery_voltage, &gpsFix, now - system_start_tick);
        }

        /* --- NAND Logging --- */
        char logLine[256];
        // Pass data from imuData (populated above) to logger
        Protocol_LogData(&imuData, temperature, battery_voltage, &gpsFix, now - system_start_tick, logLine, sizeof(logLine));
        uint16_t lineLen = (uint16_t)strlen(logLine);

        if (lineLen > 0 && lineLen < sizeof(logLine)) {
            // Check if current line fits in the remaining buffer space
            if((logBufIndex + lineLen) > LOG_PAGE_SIZE) {
                // Buffer full: Write current buffer content to NAND
                PhysicalAddrs writeAddr = { .block = currentLogBlock, .page = currentLogPageInBlock, .colAddr = 0 };
                if(NAND_Page_Program(&hspi1, &writeAddr, logPageBuffer, logBufIndex) != Ret_Success) { // Use Page_Program
                    Error_Handler(); // Handle NAND write error
                }
                // Clear buffer (fill with 0xFF for NAND is common practice)
                memset(logPageBuffer, 0xFF, LOG_PAGE_SIZE);
                logBufIndex = 0; // Reset buffer index
                Advance_NAND_Page(); // Move to next page/block (handles erase if needed)
            }
            // Append new data to buffer (ensure it fits before memcpy)
            if ((logBufIndex + lineLen) <= LOG_PAGE_SIZE) {
                 memcpy(&logPageBuffer[logBufIndex], logLine, lineLen);
                 logBufIndex += lineLen;
             } else {
                  // This case should ideally not happen if the check above works correctly
                  // Maybe log an error that data couldn't be buffered
             }
        }
    }

    /* --- Command Processing --- */
    Protocol_ProcessIncoming();

    /* --- USBX Task Processing --- */
    /* If not using RTOS, call USBX task runner periodically */
    USBX_Device_Process(); // Make sure this is defined somewhere (e.g., app_usbx_device.c)

    /* --- Yield/Delay --- */
    // If using RTOS, replace HAL_Delay with task yield/sleep
    // HAL_Delay(1); // Use minimal delay only if absolutely necessary without RTOS

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Enable Epod Booster
  */
  if (HAL_RCCEx_EpodBoosterClkConfig(RCC_EPODBOOSTER_SOURCE_MSIS, RCC_EPODBOOSTER_DIV1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_PWREx_EnableEpodBooster() != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Set Flash latency before increasing MSIS
  */
  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_2);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSIS;
  RCC_OscInitStruct.MSISState = RCC_MSI_ON;
  RCC_OscInitStruct.MSISSource = RCC_MSI_RC0;
  RCC_OscInitStruct.MSISDiv = RCC_MSI_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSIS;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief FLASH Initialization Function
  * @param None
  * @retval None
  */
static void MX_FLASH_Init(void)
{

  /* USER CODE BEGIN FLASH_Init 0 */
  /* USER CODE END FLASH_Init 0 */

  /* USER CODE BEGIN FLASH_Init 1 */
  /* USER CODE END FLASH_Init 1 */
  /* USER CODE BEGIN FLASH_Init 2 */
  /* USER CODE END FLASH_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */
  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */
  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x009032AE;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */
  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */
  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */
  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */
  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
  /* USER CODE END SPI1_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

  /* USER CODE BEGIN SPI1_Init 1 */
  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x7;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi1, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */
  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */
  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXINVERT_INIT|UART_ADVFEATURE_SWAP_INIT;
  huart4.AdvancedInit.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
  huart4.AdvancedInit.Swap = UART_ADVFEATURE_SWAP_ENABLE;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */
  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */
  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */
  /* USER CODE END USB_Init 1 */
  hpcd_USB_DRD_FS.Instance = USB_DRD_FS;
  hpcd_USB_DRD_FS.Init.dev_endpoints = 8;
  hpcd_USB_DRD_FS.Init.speed = USBD_FS_SPEED;
  hpcd_USB_DRD_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_DRD_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.battery_charging_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.bulk_doublebuffer_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.iso_singlebuffer_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_DRD_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */
  /* USER CODE END USB_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPS_RST_GPIO_Port, GPS_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, WIFI_EN_Pin|CS_WIFI_Pin|BMS_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LDO_EN_Pin|CS_NAND_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : WIFI_INT_Pin BMS_INT_Pin */
  GPIO_InitStruct.Pin = WIFI_INT_Pin|BMS_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : GPS_RST_Pin */
  GPIO_InitStruct.Pin = GPS_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPS_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : WIFI_EN_Pin CS_WIFI_Pin BMS_RST_Pin */
  GPIO_InitStruct.Pin = WIFI_EN_Pin|CS_WIFI_Pin|BMS_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LDO_EN_Pin CS_NAND_Pin */
  GPIO_InitStruct.Pin = LDO_EN_Pin|CS_NAND_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : GPS_INT_Pin BMS_CHG_Pin */
  GPIO_InitStruct.Pin = GPS_INT_Pin|BMS_CHG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  UART Receive Complete Callback.
  * Called by HAL_UART_IRQHandler -> HAL_UART_RxCpltCallback when defined.
  * Redirects the call to the GPS driver's callback.
  * @param  huart: UART handle.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Check if the interrupt is from the GPS UART */
  if (huart->Instance == UART4)
  {
    /* Call the GPS driver's callback function */
    GPS_UART_RxCpltCallback(huart); // Defined in gps.c
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
      // Blink an LED or use debugger
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
   printf("Assertion failed: file %s on line %ld\r\n", (char *)file, line); // Cast file pointer
   Error_Handler(); // Halt on assert failure
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
