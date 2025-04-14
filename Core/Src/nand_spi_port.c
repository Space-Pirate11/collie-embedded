/* Core/Src/nand_spi_port.c */

#include "nand_spi_port.h"
#include "main.h" // Required for HAL handles and definitions
#include "stm32u3xx_hal.h"

// Timeout for SPI operations (milliseconds)
#define NAND_SPI_TIMEOUT 100

// --- HAL Handle Extern Declarations (Ensure these match your main.c) ---
extern SPI_HandleTypeDef hspi1;
// -----------------------------------------------------------------------

/**
 * @brief Select the NAND flash chip via its CS pin.
 */
void nand_cs_select(void) {
    // Assumes CS_NAND_Pin and CS_NAND_GPIO_Port are defined in main.h
    HAL_GPIO_WritePin(CS_NAND_GPIO_Port, CS_NAND_Pin, GPIO_PIN_RESET); // Active Low
    // Small delay might be needed between CS assert and clock start for some devices
    // Consider adding a short delay_us here if needed.
    // e.g. for(volatile int i=0; i<10; i++);
}

/**
 * @brief Deselect the NAND flash chip via its CS pin.
 */
void nand_cs_deselect(void) {
     // Small delay might be needed between clock stop and CS deassert for some devices
     // Consider adding a short delay_us here if needed.
     // e.g. for(volatile int i=0; i<10; i++);
    HAL_GPIO_WritePin(CS_NAND_GPIO_Port, CS_NAND_Pin, GPIO_PIN_SET); // Active Low
}

/**
 * @brief Perform SPI transaction (transmit and receive).
 */
int nand_spi_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len) {
    HAL_StatusTypeDef status;

    // Use HAL_SPI_TransmitReceive for simultaneous Tx/Rx
    // If only transmitting, rx_buf can be NULL, but HAL expects a valid pointer.
    // If only receiving, tx_buf can be NULL, but HAL expects a valid pointer.

    // Handle cases where only Tx or Rx is needed
    if (tx_buf != NULL && rx_buf != NULL) {
        status = HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)tx_buf, rx_buf, len, NAND_SPI_TIMEOUT);
    } else if (tx_buf != NULL) {
        // Transmit only - provide dummy receive buffer or use HAL_SPI_Transmit
        status = HAL_SPI_Transmit(&hspi1, (uint8_t*)tx_buf, len, NAND_SPI_TIMEOUT);
    } else if (rx_buf != NULL) {
        // Receive only - provide dummy transmit buffer or use HAL_SPI_Receive
        // Often requires sending dummy bytes (e.g., 0xFF) to clock data out
        // This simple implementation assumes the caller handles dummy bytes if needed.
        // A more robust version might create a dummy tx buffer here.
         status = HAL_SPI_Receive(&hspi1, rx_buf, len, NAND_SPI_TIMEOUT);
        // Or: status = HAL_SPI_TransmitReceive(&hspi1, dummy_tx, rx_buf, len, NAND_SPI_TIMEOUT);
    } else {
        return -1; // Invalid arguments
    }


    if (status == HAL_OK) {
        return 0; // Success
    } else {
        return -1; // Failure
    }
}

/**
 * @brief Platform specific delay function (milliseconds).
 */
void nand_delay_ms(uint32_t period) {
    HAL_Delay(period);
}


/**
 * @brief Initialize the platform specific interface for NAND Flash
 */
int nand_interface_init(void) {
    // Ensure CS pin is initialized High (inactive)
    nand_cs_deselect();

    // No specific HAL initialization needed here, assuming SPI1 and GPIOs
    // are already initialized by CubeMX generated code in main.c.

    // Could perform a quick check here, e.g., read NAND ID, but better
    // done in the storage_task_init after this port is initialized.

    return 0; // Success
}
