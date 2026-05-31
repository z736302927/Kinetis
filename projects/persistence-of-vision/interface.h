/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __STM32_INTERFACE_H
#define __STM32_INTERFACE_H

#include <linux/types.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/tim-task.h>

extern struct iic_master general_iic_master;
extern struct spi_master general_spi_master;
extern struct rtc_device general_rtc;
extern struct serial_port_ops stm32_usart2_ops;
extern struct serial_port_ops stm32_usart6_ops;
extern struct flash_ops general_flash_ops;
extern struct bootloader_ops general_bl_ops;
extern struct serial_port *stm32_serial_port[5];
extern struct hall_device *hall_dev;

u32 hall_read_rotated_time(struct hall_device *dev);
void hall_fake_pulse(struct tim_task *task);

#endif	/* __STM32_INTERFACE_H */
