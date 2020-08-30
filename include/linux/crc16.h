/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *	crc16.h - CRC-16 routine
 *
 * Implements the standard CRC-16:
 *   Width 16
 *   Poly  0x8005 (x^16 + x^15 + x^2 + 1)
 *   Init  0
 *
 * Copyright (c) 2005 Ben Gardner <bgardner@wabtec.com>
 */

#ifndef __CRC16_H
#define __CRC16_H

#include "stdint.h"

extern uint16_t const crc16_table[256];

extern uint16_t crc16(uint16_t crc, const uint8_t *buffer, uint32_t len);

static inline uint16_t crc16_byte(uint16_t crc, const uint8_t data)
{
    return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

#endif /* __CRC16_H */

