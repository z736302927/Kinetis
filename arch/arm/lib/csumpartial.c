/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/csumpartial.c
 *
 *  Copyright (C) 1995-1998 Russell King
 *  Converted from assembly to C
 */
#include <linux/types.h>
#include <linux/module.h>
#include <asm/byteorder.h>

static inline u32 add_with_carry(u32 sum, u32 val)
{
	u64 result = (u64)sum + val;
	return (u32)(result + (result >> 32));
}

/* Helper to handle unaligned access */
static inline u16 get_unaligned_le16(const u8 *p)
{
#ifdef __LITTLE_ENDIAN
	return p[0] | (p[1] << 8);
#else
	return p[1] | (p[0] << 8);
#endif
}

static inline u16 get_unaligned_be16(const u8 *p)
{
#ifdef __LITTLE_ENDIAN
	return p[1] | (p[0] << 8);
#else
	return p[0] | (p[1] << 8);
#endif
}

/*
 * Function: __u32 csum_partial(const char *src, int len, __u32 sum)
 * Params  : buf = buffer, len = length, sum = initial checksum
 * Returns : new checksum
 */
__wsum csum_partial(const void *buff, int len, __wsum sum)
{
	const unsigned char *buf = (const unsigned char *)buff;
	const unsigned char *orig_buf = buf;
	u32 checksum = sum;
	u32 td0, td1, td2, td3;
	
	if (len <= 0) {
		return checksum;
	}
	
	/* Handle unaligned buffer start */
	if ((unsigned long)buf & 1) {
		/* Odd address - need to rotate checksum and handle byte */
		checksum = (checksum << 24) | (checksum >> 8);
		td0 = *buf++;
		len--;
		checksum = add_with_carry(checksum, td0 << 8);
	}
	
	if ((unsigned long)buf & 2) {
		/* 16-bit unaligned - handle halfword */
#ifdef __ARMEB__
		td0 = get_unaligned_be16(buf);
#else
		td0 = get_unaligned_le16(buf);
#endif
		buf += 2;
		len -= 2;
		checksum = add_with_carry(checksum, td0);
	}
	
	/* Now we're 32-bit aligned */
	
	/* Process 32-byte blocks */
	while (len >= 32) {
		const u32 *ptr = (const u32 *)buf;
		
		td0 = ptr[0];
		td1 = ptr[1];
		td2 = ptr[2];
		td3 = ptr[3];
		checksum = add_with_carry(checksum, td0);
		checksum = add_with_carry(checksum, td1);
		checksum = add_with_carry(checksum, td2);
		checksum = add_with_carry(checksum, td3);
		
		td0 = ptr[4];
		td1 = ptr[5];
		td2 = ptr[6];
		td3 = ptr[7];
		checksum = add_with_carry(checksum, td0);
		checksum = add_with_carry(checksum, td1);
		checksum = add_with_carry(checksum, td2);
		checksum = add_with_carry(checksum, td3);
		
		buf += 32;
		len -= 32;
	}
	
	/* Process remaining 4-byte chunks */
	while (len >= 4) {
		td0 = *(const u32 *)buf;
		checksum = add_with_carry(checksum, td0);
		buf += 4;
		len -= 4;
	}
	
	/* Process remaining 2-byte chunks */
	while (len >= 2) {
#ifdef __ARMEB__
		td0 = get_unaligned_be16(buf);
#else
		td0 = get_unaligned_le16(buf);
#endif
		checksum = add_with_carry(checksum, td0);
		buf += 2;
		len -= 2;
	}
	
	/* Process final byte */
	if (len == 1) {
		td0 = *buf;
		checksum = add_with_carry(checksum, td0);
	}
	
	/* Add final carry */
	if (checksum < td0)
		checksum++;
	
	/* If original buffer was odd-aligned, rotate checksum back */
	if ((unsigned long)orig_buf & 1) {
		checksum = (checksum << 24) | (checksum >> 8);
	}
	
	return checksum;
}
EXPORT_SYMBOL(csum_partial);
