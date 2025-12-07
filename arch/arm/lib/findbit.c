/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/findbit.S
 *
 *  Copyright (C) 1995-2000 Russell King
 *
 * 16th March 2001 - John Ripley <jripley@sonicblue.com>
 *   Fixed so that "size" is an exclusive not an inclusive quantity.
 *   All users of these functions expect exclusive sizes, and may
 *   also call with zero size.
 * Reworked by rmk.
 */
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/errno.h>

/* Helper function to find the position of the first set bit */
static inline int find_first_set_bit_pos(unsigned char value)
{
	int pos = 0;
	
	if (value == 0)
		return -1;
	
	/* Find the first set bit position (0-7) */
	if ((value & 0x0F) == 0) {
		pos += 4;
		value >>= 4;
	}
	if ((value & 0x03) == 0) {
		pos += 2;
		value >>= 2;
	}
	if ((value & 0x01) == 0) {
		pos += 1;
	}
	
	return pos;
}

/* Internal helper function with correct signature */
static int find_first_zero_bit_internal(const unsigned char *p, unsigned int maxbit)
{
	unsigned int bit;
	
	if (maxbit == 0)
		return maxbit;
	
	for (bit = 0; bit < maxbit; bit += 8) {
		unsigned char byte = p[bit >> 3];
		unsigned char inverted = ~byte;
		
		if (inverted != 0) {
			int pos = find_first_set_bit_pos(inverted);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
	}
	
	return maxbit;
}

static int find_next_zero_bit_internal(const unsigned char *p, unsigned int maxbit, unsigned int offset)
{
	unsigned int bit = offset;
	
	if (maxbit == 0 || offset >= maxbit)
		return maxbit;
	
	/* Handle the first byte with offset */
	if ((offset & 7) != 0) {
		unsigned char byte = p[bit >> 3];
		unsigned char inverted = ~byte;
		int pos_in_byte = offset & 7;
		
		/* Shift off unused bits */
		inverted >>= pos_in_byte;
		
		if (inverted != 0) {
			int pos = find_first_set_bit_pos(inverted);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
		
		/* Align to next byte boundary */
		bit = (bit + 7) & ~7;
	}
	
	/* Continue with byte-aligned search */
	return find_first_zero_bit_internal(p, maxbit);
}

static int find_first_bit_internal(const unsigned char *p, unsigned int maxbit)
{
	unsigned int bit;
	
	if (maxbit == 0)
		return maxbit;
	
	for (bit = 0; bit < maxbit; bit += 8) {
		unsigned char byte = p[bit >> 3];
		
		if (byte != 0) {
			int pos = find_first_set_bit_pos(byte);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
	}
	
	return maxbit;
}

static int find_next_bit_internal(const unsigned char *p, unsigned int maxbit, unsigned int offset)
{
	unsigned int bit = offset;
	
	if (maxbit == 0 || offset >= maxbit)
		return maxbit;
	
	/* Handle the first byte with offset */
	if ((offset & 7) != 0) {
		unsigned char byte = p[bit >> 3];
		int pos_in_byte = offset & 7;
		
		/* Shift off unused bits */
		byte >>= pos_in_byte;
		
		if (byte != 0) {
			int pos = find_first_set_bit_pos(byte);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
		
		/* Align to next byte boundary */
		bit = (bit + 7) & ~7;
	}
	
	/* Continue with byte-aligned search */
	return find_first_bit_internal(p, maxbit);
}

/* Little endian versions - matching header declarations */
int _find_first_zero_bit_le(const unsigned long *addr, unsigned int maxbit)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_first_zero_bit_internal(p, maxbit);
}

int _find_next_zero_bit_le(const unsigned long *addr, int maxbit, int offset)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_next_zero_bit_internal(p, maxbit, offset);
}

int _find_first_bit_le(const unsigned long *addr, unsigned int maxbit)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_first_bit_internal(p, maxbit);
}

int _find_next_bit_le(const unsigned long *addr, int maxbit, int offset)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_next_bit_internal(p, maxbit, offset);
}

#ifdef __ARMEB__

/* Big endian versions - need to handle byte ordering */
static inline unsigned int adjust_be_bit_position(unsigned int bit)
{
	/* In big endian, bit positions within a byte are reversed */
	unsigned int byte_pos = bit >> 3;
	unsigned int bit_in_byte = bit & 7;
	unsigned char mask = 0x80 >> bit_in_byte; /* MSB is bit 0 in big endian */
	
	return (byte_pos << 3) + bit_in_byte;
}

static int find_first_zero_bit_be_internal(const unsigned char *p, unsigned int maxbit)
{
	unsigned int bit;
	
	if (maxbit == 0)
		return maxbit;
	
	for (bit = 0; bit < maxbit; bit += 8) {
		unsigned int adjusted_bit = adjust_be_bit_position(bit);
		unsigned char byte = p[adjusted_bit >> 3];
		unsigned char inverted = ~byte;
		
		if (inverted != 0) {
			int pos = find_first_set_bit_pos(inverted);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
	}
	
	return maxbit;
}

static int find_next_zero_bit_be_internal(const unsigned char *p, unsigned int maxbit, unsigned int offset)
{
	unsigned int bit = offset;
	
	if (maxbit == 0 || offset >= maxbit)
		return maxbit;
	
	/* Handle the first byte with offset */
	if ((offset & 7) != 0) {
		unsigned int adjusted_bit = adjust_be_bit_position(bit);
		unsigned char byte = p[adjusted_bit >> 3];
		unsigned char inverted = ~byte;
		int pos_in_byte = offset & 7;
		
		/* Shift off unused bits (big endian order) */
		inverted >>= pos_in_byte;
		
		if (inverted != 0) {
			int pos = find_first_set_bit_pos(inverted);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
		
		/* Align to next byte boundary */
		bit = (bit + 7) & ~7;
	}
	
	/* Continue with byte-aligned search */
	return find_first_zero_bit_be_internal(p, maxbit);
}

static int find_first_bit_be_internal(const unsigned char *p, unsigned int maxbit)
{
	unsigned int bit;
	
	if (maxbit == 0)
		return maxbit;
	
	for (bit = 0; bit < maxbit; bit += 8) {
		unsigned int adjusted_bit = adjust_be_bit_position(bit);
		unsigned char byte = p[adjusted_bit >> 3];
		
		if (byte != 0) {
			int pos = find_first_set_bit_pos(byte);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
	}
	
	return maxbit;
}

static int find_next_bit_be_internal(const unsigned char *p, unsigned int maxbit, unsigned int offset)
{
	unsigned int bit = offset;
	
	if (maxbit == 0 || offset >= maxbit)
		return maxbit;
	
	/* Handle the first byte with offset */
	if ((offset & 7) != 0) {
		unsigned int adjusted_bit = adjust_be_bit_position(bit);
		unsigned char byte = p[adjusted_bit >> 3];
		int pos_in_byte = offset & 7;
		
		/* Shift off unused bits (big endian order) */
		byte >>= pos_in_byte;
		
		if (byte != 0) {
			int pos = find_first_set_bit_pos(byte);
			if (pos >= 0) {
				bit += pos;
				return (bit < maxbit) ? bit : maxbit;
			}
		}
		
		/* Align to next byte boundary */
		bit = (bit + 7) & ~7;
	}
	
	/* Continue with byte-aligned search */
	return find_first_bit_be_internal(p, maxbit);
}

/* Big endian versions - matching header declarations */
int _find_first_zero_bit_be(const unsigned long *addr, unsigned int maxbit)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_first_zero_bit_be_internal(p, maxbit);
}

int _find_next_zero_bit_be(const unsigned long *addr, int maxbit, int offset)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_next_zero_bit_be_internal(p, maxbit, offset);
}

int _find_first_bit_be(const unsigned long *addr, unsigned int maxbit)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_first_bit_be_internal(p, maxbit);
}

int _find_next_bit_be(const unsigned long *addr, int maxbit, int offset)
{
	const unsigned char *p = (const unsigned char *)addr;
	return find_next_bit_be_internal(p, maxbit, offset);
}

#endif /* __ARMEB__ */