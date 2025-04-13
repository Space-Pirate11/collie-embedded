/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  * This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* Exported variables --------------------------------------------------------*/
/* Declare HAL handles as extern so they can be used in other files */
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart4;
extern PCD_HandleTypeDef hpcd_USB_DRD_FS;

/* Global flag: When set to 1, live streaming (USB/BLE) is enabled */
/* Define this in main.c, declare extern here */
extern volatile uint8_t liveStreamingEnabled;


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define WIFI_INT_Pin GPIO_PIN_0
#define WIFI_INT_GPIO_Port GPIOH
#define GPS_RST_Pin GPIO_PIN_1
#define GPS_RST_GPIO_Port GPIOH
#define TEMP_Pin GPIO_PIN_2
#define TEMP_GPIO_Port GPIOA
#define WIFI_EN_Pin GPIO_PIN_3
#define WIFI_EN_GPIO_Port GPIOA
#define CS_WIFI_Pin GPIO_PIN_4
#define CS_WIFI_GPIO_Port GPIOA
#define LDO_EN_Pin GPIO_PIN_13
#define LDO_EN_GPIO_Port GPIOB
#define CS_NAND_Pin GPIO_PIN_14
#define CS_NAND_GPIO_Port GPIOB
#define BMS_RST_Pin GPIO_PIN_15
#define BMS_RST_GPIO_Port GPIOA
#define GPS_INT_Pin GPIO_PIN_4
#define GPS_INT_GPIO_Port GPIOB
#define BMS_INT_Pin GPIO_PIN_3
#define BMS_INT_GPIO_Port GPIOH
#define BMS_CHG_Pin GPIO_PIN_8
#define BMS_CHG_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
