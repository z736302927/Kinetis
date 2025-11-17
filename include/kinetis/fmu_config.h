/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _FMU_CONFIG_H_
#define _FMU_CONFIG_H_

#include <linux/types.h>
#include <linux/errno.h>

//#include "gpio.h"
//#include "usart.h"

#define USE_MAGNET
#define USE_LENGTH_LIM

/* This call cycle shall be the same as the led_duty(). */
#define FMU_LED_ACCURACY	20
#define FMU_LED_NUM		4

//#define FMU_LED_OPERATION(num, op)	{\
//	switch (num) {	\
//	case 0: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, op ? GPIO_PIN_SET : GPIO_PIN_RESET); break;	\
//	case 1: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, op ? GPIO_PIN_SET : GPIO_PIN_RESET); break;	\
//	case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, op ? GPIO_PIN_SET : GPIO_PIN_RESET); break;	\
//	case 3: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, op ? GPIO_PIN_SET : GPIO_PIN_RESET); break;	\
//	default: BUG_ON(1); break;	\
//	}}

#define FMU_FILE_PATH		"0:/FMU/"
#define FMU_PARA_FILE		"FMU.txt"

static inline int ano_transfer_module(void *buffer, u32 len)
{
//	int ret;
//
//	ret = HAL_UART_Transmit(&huart5, buffer, len, 1000);
//
//	if (ret)
//		return -EPIPE;

	return 0;
}

#define AHRS_FIFO_SIZE	1024

#endif /* _FMU_CONFIG_H_ */
