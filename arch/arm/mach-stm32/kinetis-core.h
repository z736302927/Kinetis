/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __STM32_CORE_H
#define __STM32_CORE_H

#include <linux/mtd/spi-nor.h>
#include <linux/i2c.h>

struct at24_user_input {

	unsigned int write_max;
	unsigned int num_addresses;
	unsigned int offset_adj;

	u32 address_width;
	u32 byte_len;
	u16 page_size;
	u8 flags;
};

struct gpio_user_input {
	unsigned int gpio_bank;
	u32 **base;
	const struct of_device_id *match_data;
};

struct stm32_dt {
	struct at24_user_input at24;
	struct gpio_user_input gpio;
};

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
	struct clk *clk;
	int speed;
	int parent_rate;
	struct stm32f4_i2c_msg msg;
};

struct kineits_system {
	struct stm32_dt dt;
    struct spi_nor *nor;
};

struct kineits_system *lib_get_stm32_val(void);

#endif	/* __STM32_CORE_H */
