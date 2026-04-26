/**
 * @file host_spi_multi.h
 * @brief 4-way parallel SPI DMA LED driver for host
 * @note Manages 4 SPI peripherals with DMA for parallel LED data output,
 *       plus LAT latch control to update LED drivers simultaneously
 */

#ifndef POV_HOST_SPI_MULTI_H
#define POV_HOST_SPI_MULTI_H

#include <linux/types.h>
#include "host_pov_config.h"

/*********************************************************************
 * Host SPI Multi Device Structure
 *********************************************************************/

struct host_spi_multi_device {
	u8 initialized;
	u8 dma_busy;                   /* 1 if DMA transfer in progress */

	/* Data buffers for each SPI group */
	u8 group_data[POV_SPI_GROUPS][POV_BYTES_PER_GROUP];

#if MCU_PLATFORM_STM32
	/* HAL handles would go here:
	 * SPI_HandleTypeDef hspi1, hspi2, hspi3, hspi4;
	 * DMA_HandleTypeDef hdma_spi1_tx, hdma_spi2_tx, hdma_spi3_tx, hdma_spi4_tx;
	 * GPIO LAT pin handle
	 */
#endif
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize 4-way SPI with DMA
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 * @note Configures SPI1~SPI4 at 20MHz with DMA channels on STM32H743
 */
int host_spi_multi_init(struct host_spi_multi_device *dev);

/**
 * @brief Send column data to all 4 SPI groups in parallel
 * @param dev: Device pointer
 * @param col_data: Column data buffer (POV_BYTES_PER_COL = 768 bytes)
 * @return 0 on success, negative error code on failure
 * @note Data layout: [group0:192B][group1:192B][group2:192B][group3:192B]
 */
int host_spi_multi_send(struct host_spi_multi_device *dev, const u8 *col_data);

/**
 * @brief Latch data to LED outputs (pulse LAT pin)
 * @param dev: Device pointer
 * @note Must be called after host_spi_multi_send() completes
 */
void host_spi_multi_latch(struct host_spi_multi_device *dev);

/**
 * @brief Check if DMA transfer is complete
 * @param dev: Device pointer
 * @return 1 if complete/idle, 0 if busy
 */
int host_spi_multi_is_complete(struct host_spi_multi_device *dev);

/**
 * @brief Wait for DMA transfer to complete with timeout
 * @param dev: Device pointer
 * @param timeout_us: Timeout in microseconds
 * @return 0 on success, -ETIMEDOUT on timeout
 */
int host_spi_multi_wait_complete(struct host_spi_multi_device *dev, u32 timeout_us);

/**
 * @brief Deinitialize SPI peripherals
 * @param dev: Device pointer
 */
void host_spi_multi_deinit(struct host_spi_multi_device *dev);

#endif /* POV_HOST_SPI_MULTI_H */
