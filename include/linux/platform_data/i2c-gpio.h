/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * i2c-gpio interface to platform code
 *
 * Copyright (C) 2007 Atmel Corporation
 */
#ifndef _LINUX_I2C_GPIO_H
#define _LINUX_I2C_GPIO_H

#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

/**
 * struct i2c_gpio_platform_data - Platform-dependent data for i2c-gpio
 * @udelay: signal toggle delay. SCL frequency is (500 / udelay) kHz
 * @timeout: clock stretching timeout in jiffies. If the slave keeps
 *	SCL low for longer than this, the transfer will time out.
 * @sda_is_open_drain: SDA is configured as open drain, i.e. the pin
 *	isn't actively driven high when setting the output value high.
 *	gpio_get_value() must return the actual pin state even if the
 *	pin is configured as an output.
 * @scl_is_open_drain: SCL is set up as open drain. Same requirements
 *	as for sda_is_open_drain apply.
 * @scl_is_output_only: SCL output drivers cannot be turned off.
 */
struct i2c_gpio_platform_data {
	int		udelay;
	int		timeout;
	unsigned int	sda_is_open_drain:1;
	unsigned int	scl_is_open_drain:1;
	unsigned int	scl_is_output_only:1;
};

struct i2c_gpio_private_data {
	struct gpio_desc *sda;
	struct gpio_desc *scl;
	struct i2c_adapter adap;
	struct i2c_algo_bit_data bit_data;
	struct i2c_gpio_platform_data pdata;
#ifdef CONFIG_I2C_GPIO_FAULT_INJECTOR
	struct dentry *debug_dir;
	/* these must be protected by bus lock */
	struct completion scl_irq_completion;
	u64 scl_irq_data;
#endif
};

int __init i2c_gpio_init(void);
void __exit i2c_gpio_exit(void);

#endif /* _LINUX_I2C_GPIO_H */
