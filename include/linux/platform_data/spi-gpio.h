/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * spi-gpio interface to platform code
 *
 * Copyright (C) 2007 Atmel Corporation
 */
#ifndef _LINUX_SPI_GPIO_H
#define _LINUX_SPI_GPIO_H

#include <linux/init.h>

int __init spi_gpio_init(void);
void __exit spi_gpio_exit(void);

#endif /* _LINUX_SPI_GPIO_H */
