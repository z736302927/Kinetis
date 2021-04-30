/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * i2c-gpio interface to platform code
 *
 * Copyright (C) 2007 Atmel Corporation
 */
#ifndef _LINUX_STM32F4_H
#define _LINUX_STM32F4_H

#include <linux/init.h>
#include <linux/i2c.h>

/**
 * struct stm32f4_i2c_msg - client specific data
 * @addr: 8-bit slave addr, including r/w bit
 * @count: number of bytes to be transferred
 * @buf: data buffer
 * @result: result of the transfer
 * @stop: last I2C msg to be sent, i.e. STOP to be generated
 */
struct stm32f4_i2c_msg {
	u8 addr;
	u32 count;
	u8 *buf;
	int result;
	bool stop;
};

/**
 * struct stm32f4_i2c_dev - private data of the controller
 * @adap: I2C adapter for this controller
 * @dev: device for this controller
 * @base: virtual memory area
 * @complete: completion of I2C message
 * @clk: hw i2c clock
 * @speed: I2C clock frequency of the controller. Standard or Fast are supported
 * @parent_rate: I2C clock parent rate in MHz
 * @msg: I2C transfer information
 */
struct stm32f4_i2c_dev {
	struct i2c_adapter adap;
	struct device *dev;
	void __iomem *base;
	struct completion complete;
	int clk;
	int speed;
	int parent_rate;
	struct stm32f4_i2c_msg msg;
};

int __init i2c_stm32f4_init(void);
void __exit i2c_stm32f4_exit(void);
int __init stm32_spi_init(void);
void __exit stm32_spi_exit(void);

#endif /* _LINUX_STM32F4_H */
