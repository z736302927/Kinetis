/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __W25QXX_PORT_H
#define __W25QXX_PORT_H

#include <linux/types.h>

#include "driver_w25qxx.h"

/*
 * @note  type values: W25Q10=0xEF10(1Mb) ... W25Q128=0xEF17(128Mb) ... W25Q02=0xEF21(2Gb)
 * @note  capacity = 2^(low_nibble + 17) bytes
 */
#define W25QXX_CAPACITY_BYTES(type)  (1UL << (((type) & 0x0F) + 17))
#define W25QXX_MAX_ADDR(type)         (W25QXX_CAPACITY_BYTES(type) - 1)

int w25qxx_port_init(w25qxx_handle_t *handle);

#endif /* __W25QXX_PORT_H */
