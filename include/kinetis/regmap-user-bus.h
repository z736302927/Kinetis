/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Regmap bus adapter for I2C/SPI soft implementation
 *
 * Provides regmap framework integration for the custom I2C and SPI
 * software implementations used in this project.
 */

#ifndef __REGMAP_USER_BUS_H
#define __REGMAP_USER_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/regmap.h>

/* Forward declarations */
struct iic_master;
struct spi_master;

/*
 * Context structures for bus-specific operations
 */

/* I2C soft bus context */
struct regmap_iic_soft_context {
	struct iic_master *master;
	u8 slave_addr;
};

/* SPI soft bus context */
struct regmap_spi_soft_context {
	struct spi_master *master;
};

/*
 * Initialization API
 *
 * These functions create and initialize a regmap instance for the
 * specified bus and device configuration.
 */

/**
 * regmap_init_iic_soft - Initialize regmap for I2C soft bus
 * @master: I2C master instance
 * @slave_addr: 7-bit I2C slave address
 * @config: Regmap configuration for the device
 *
 * Return: regmap pointer on success, ERR_PTR on failure
 */
struct regmap *regmap_init_iic_soft(struct iic_master *master,
				     u8 slave_addr,
				     const struct regmap_config *config);

/**
 * regmap_init_spi_soft - Initialize regmap for SPI soft bus
 * @master: SPI master instance
 * @config: Regmap configuration for the device
 *
 * Return: regmap pointer on success, ERR_PTR on failure
 */
struct regmap *regmap_init_spi_soft(struct spi_master *master,
				     const struct regmap_config *config);

/*
 * Bus type identification
 */
enum regmap_user_bus_type {
	REGMAP_BUS_IIC_SOFT = 0,
	REGMAP_BUS_SPI_SOFT,
};

#ifdef __cplusplus
}
#endif

#endif /* __REGMAP_USER_BUS_H */
