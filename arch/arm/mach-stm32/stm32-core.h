/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __STM32_CORE_H
#define __STM32_CORE_H

#include <linux/mtd/spi-nor.h>

struct stm32_val {
    struct spi_nor *nor;
};

struct stm32_val *lib_get_stm32_val(void);

#endif	/* __STM32_CORE_H */
