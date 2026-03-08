// SPDX-License-Identifier: GPL-2.0-only
/*
 * Regmap bus adapter for I2C/SPI soft implementation
 *
 * This module provides regmap framework integration for the custom
 * I2C and SPI software implementations used in this project.
 */

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/err.h>

#define pr_fmt(fmt) "regmap-user-bus: " fmt

#include "kinetis/regmap-user-bus.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"

/*
 * I2C Soft Bus Regmap Implementation
 */

static int regmap_iic_soft_write(void *context, const void *data, size_t count)
{
	struct regmap_iic_soft_context *ctx = context;
	const u8 *buf = data;
	u8 reg;
	const u8 *val;
	size_t val_len;

	if (!ctx || !ctx->master || !data || count < 1)
		return -EINVAL;

	reg = buf[0];
	val = count > 1 ? &buf[1] : NULL;
	val_len = count > 1 ? count - 1 : 0;

	if (val_len == 0)
		return 0;

	return iic_master_port_multi_transmit(ctx->master, ctx->slave_addr,
					       reg, (u8 *)val, (u8)val_len);
}

static int regmap_iic_soft_gather_write(void *context, const void *reg,
					 size_t reg_len, const void *val,
					 size_t val_len)
{
	struct regmap_iic_soft_context *ctx = context;
	u8 reg_addr;

	if (!ctx || !ctx->master || !reg || reg_len < 1)
		return -EINVAL;

	reg_addr = *(const u8 *)reg;

	if (!val || val_len == 0)
		return 0;

	return iic_master_port_multi_transmit(ctx->master, ctx->slave_addr,
					       reg_addr, (u8 *)val, (u8)val_len);
}

static int regmap_iic_soft_read(void *context, const void *reg_buf,
				 size_t reg_size, void *val_buf, size_t val_size)
{
	struct regmap_iic_soft_context *ctx = context;
	u8 reg;

	if (!ctx || !ctx->master || !reg_buf || reg_size < 1)
		return -EINVAL;

	reg = *(const u8 *)reg_buf;

	if (!val_buf || val_size == 0)
		return 0;

	return iic_master_port_multi_receive(ctx->master, ctx->slave_addr,
					      reg, val_buf, (u8)val_size);
}

static void regmap_iic_soft_free_context(void *context)
{
	kfree(context);
}

static struct regmap_bus regmap_bus_iic_soft = {
	.write = regmap_iic_soft_write,
	.gather_write = regmap_iic_soft_gather_write,
	.read = regmap_iic_soft_read,
	.free_context = regmap_iic_soft_free_context,
	.reg_format_endian_default = REGMAP_ENDIAN_BIG,
	.val_format_endian_default = REGMAP_ENDIAN_BIG,
};

/*
 * SPI Soft Bus Regmap Implementation
 */

static int regmap_spi_soft_write(void *context, const void *data, size_t count)
{
	struct regmap_spi_soft_context *ctx = context;
	const u8 *buf = data;
	u8 reg;
	const u8 *val;
	size_t val_len;

	if (!ctx || !ctx->master || !data || count < 1)
		return -EINVAL;

	reg = buf[0] & 0x7F;  /* Write: clear bit 7 */
	val = count > 1 ? &buf[1] : NULL;
	val_len = count > 1 ? count - 1 : 0;

	if (val_len == 0)
		return 0;

	return spi_master_port_transmit(ctx->master, reg, (u8 *)val, (u8)val_len);
}

static int regmap_spi_soft_gather_write(void *context, const void *reg,
					 size_t reg_len, const void *val,
					 size_t val_len)
{
	struct regmap_spi_soft_context *ctx = context;
	u8 reg_addr;

	if (!ctx || !ctx->master || !reg || reg_len < 1)
		return -EINVAL;

	reg_addr = *(const u8 *)reg & 0x7F;  /* Write: clear bit 7 */

	if (!val || val_len == 0)
		return 0;

	return spi_master_port_transmit(ctx->master, reg_addr, (u8 *)val, (u8)val_len);
}

static int regmap_spi_soft_read(void *context, const void *reg_buf,
				 size_t reg_size, void *val_buf, size_t val_size)
{
	struct regmap_spi_soft_context *ctx = context;
	u8 reg;

	if (!ctx || !ctx->master || !reg_buf || reg_size < 1)
		return -EINVAL;

	reg = *(const u8 *)reg_buf | 0x80;  /* Read: set bit 7 */

	if (!val_buf || val_size == 0)
		return 0;

	return spi_master_port_receive(ctx->master, reg, val_buf, (u8)val_size);
}

static void regmap_spi_soft_free_context(void *context)
{
	kfree(context);
}

static struct regmap_bus regmap_bus_spi_soft = {
	.write = regmap_spi_soft_write,
	.gather_write = regmap_spi_soft_gather_write,
	.read = regmap_spi_soft_read,
	.free_context = regmap_spi_soft_free_context,
	.read_flag_mask = 0x80,
	.reg_format_endian_default = REGMAP_ENDIAN_BIG,
	.val_format_endian_default = REGMAP_ENDIAN_BIG,
};

/*
 * Initialization API
 */

struct regmap *regmap_init_iic_soft(struct iic_master *master,
				     u8 slave_addr,
				     const struct regmap_config *config)
{
	struct regmap_iic_soft_context *ctx;
	struct regmap *map;

	if (!master || !config) {
		pr_err("invalid parameters\n");
		return ERR_PTR(-EINVAL);
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->master = master;
	ctx->slave_addr = slave_addr;

	map = regmap_init(NULL, &regmap_bus_iic_soft, ctx, config);
	if (IS_ERR(map)) {
		pr_err("failed to initialize regmap: %ld\n", PTR_ERR(map));
		kfree(ctx);
		return map;
	}

	pr_debug("I2C regmap initialized for device 0x%02X\n", slave_addr);
	return map;
}

struct regmap *regmap_init_spi_soft(struct spi_master *master,
				     const struct regmap_config *config)
{
	struct regmap_spi_soft_context *ctx;
	struct regmap *map;

	if (!master || !config) {
		pr_err("invalid parameters\n");
		return ERR_PTR(-EINVAL);
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->master = master;

	map = regmap_init(NULL, &regmap_bus_spi_soft, ctx, config);
	if (IS_ERR(map)) {
		pr_err("failed to initialize regmap: %ld\n", PTR_ERR(map));
		kfree(ctx);
		return map;
	}

	pr_debug("SPI regmap initialized\n");
	return map;
}
