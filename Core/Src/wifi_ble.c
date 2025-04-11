#include "wifi_ble.h"
#include "nand_spi.h"  // Re-use our SPI wrapper functions

/* WiFi/BLE chip select definitions (different from NAND) */
#define WIFI_NCS_PIN    GPIO_PIN_4
#define WIFI_NCS_PORT   GPIOA

/* Internal helper functions to control WiFi/BLE CS */
static void WIFI_CS_Low(void) {
    HAL_GPIO_WritePin(WIFI_NCS_PORT, WIFI_NCS_PIN, GPIO_PIN_RESET);
}

static void WIFI_CS_High(void) {
    HAL_GPIO_WritePin(WIFI_NCS_PORT, WIFI_NCS_PIN, GPIO_PIN_SET);
}

/* Initialize the WiFi/BLE module:
   - Set WIFI_EN (PA3) HIGH to enable the module.
   - Wait ~100 ms for the module to boot.
*/
HAL_StatusTypeDef WiFiBLE_Init(SPI_HandleTypeDef *hspi) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); // WIFI_EN = HIGH
    HAL_Delay(100);  // Wait for module boot-up
    // Optionally, perform a self-test command here.
    return HAL_OK;
}

/* Send data to the module.
   Protocol: 1 byte length, then data payload.
*/
HAL_StatusTypeDef WiFiBLE_SendData(SPI_HandleTypeDef *hspi, const uint8_t *data, uint16_t length) {
    if(length > 255)
        length = 255;
    uint8_t len_byte = (uint8_t)length;
    SPI_Params tx;
    WIFI_CS_Low();
    tx.buffer = &len_byte;
    tx.length = 1;
    if(NAND_SPI_Send(hspi, &tx) != SPI_OK) {
        WIFI_CS_High();
        return HAL_ERROR;
    }
    tx.buffer = (uint8_t*)data;
    tx.length = length;
    if(NAND_SPI_Send(hspi, &tx) != SPI_OK) {
        WIFI_CS_High();
        return HAL_ERROR;
    }
    WIFI_CS_High();
    return HAL_OK;
}
