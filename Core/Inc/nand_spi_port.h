/* Core/Inc/nand_spi_port.h */

#ifndef INC_NAND_SPI_PORT_H_
#define INC_NAND_SPI_PORT_H_

#include <stdint.h>
#include <stddef.h> // For size_t

/* Function prototypes for NAND driver platform specific functions */

/**
 * @brief Select the NAND flash chip via its CS pin.
 */
void nand_cs_select(void);

/**
 * @brief Deselect the NAND flash chip via its CS pin.
 */
void nand_cs_deselect(void);

/**
 * @brief Perform SPI transaction (transmit and receive).
 * @param tx_buf Pointer to the data buffer to be transmitted. Can be NULL if only receiving.
 * @param rx_buf Pointer to the data buffer for received data. Can be NULL if only transmitting.
 * @param len Number of bytes to transmit/receive.
 * @return 0 on success, non-zero on failure.
 */
int nand_spi_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len);

/**
 * @brief Platform specific delay function (milliseconds).
 * @param period Delay in milliseconds.
 */
void nand_delay_ms(uint32_t period);

/**
 * @brief Initialize the platform specific interface for NAND Flash
 * (Assign HAL SPI handle, GPIO ports/pins for CS, etc.)
 * @return 0 on success, non-zero on failure
 */
int nand_interface_init(void);


#endif /* INC_NAND_SPI_PORT_H_ */
