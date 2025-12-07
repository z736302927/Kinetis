/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/csumpartialcopygeneric.c
 *
 *  Copyright (C) 1995-2001 Russell King
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

/* Helper functions for unaligned access */
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

static inline void put_unaligned_le16(u16 val, u8 *p)
{
#ifdef __LITTLE_ENDIAN
	p[0] = val;
	p[1] = val >> 8;
#else
	p[1] = val;
	p[0] = val >> 8;
#endif
}

static inline void put_unaligned_be16(u16 val, u8 *p)
{
#ifdef __LITTLE_ENDIAN
	p[1] = val;
	p[0] = val >> 8;
#else
	p[0] = val;
	p[1] = val >> 8;
#endif
}

/* Align destination pointer and handle initial bytes */
static void align_dst(const unsigned char **src, unsigned char **dst, 
		      int *len, u32 *sum)
{
	unsigned char *d = *dst;
	const unsigned char *s = *src;
	int l = *len;
	u32 checksum = *sum;
	
	/* Handle odd destination alignment */
	if ((unsigned long)d & 1) {
		u8 byte = *s++;
		checksum = add_with_carry(checksum, (u32)byte << 8);
		*d++ = byte;
		l--;
		
		if (l <= 7) {
			*dst = d;
			*src = s;
			*len = l;
			*sum = checksum;
			return;
		}
	}
	
	/* Handle 16-bit destination alignment */
	if ((unsigned long)d & 2) {
		u16 halfword;
#ifdef __ARMEB__
		halfword = get_unaligned_be16(s);
#else
		halfword = get_unaligned_le16(s);
#endif
		checksum = add_with_carry(checksum, halfword & 0xff);
		checksum = add_with_carry(checksum, halfword >> 8);
		
#ifdef __ARMEB__
		put_unaligned_be16(halfword, d);
#else
		put_unaligned_le16(halfword, d);
#endif
		
		s += 2;
		d += 2;
		l -= 2;
	}
	
	*dst = d;
	*src = s;
	*len = l;
	*sum = checksum;
}

/* Handle remaining bytes (0-7) */
static void handle_remainder(const unsigned char **src, unsigned char **dst,
			    int *len, u32 *sum)
{
	const unsigned char *s = *src;
	unsigned char *d = *dst;
	int l = *len;
	u32 checksum = *sum;
	
	/* Align destination if needed */
	while (l > 0 && ((unsigned long)d & 1)) {
		u8 byte = *s++;
		checksum = add_with_carry(checksum, (u32)byte << 8);
		*d++ = byte;
		l--;
	}
	
	/* Process 2-byte chunks */
	while (l >= 2) {
		u16 halfword;
#ifdef __ARMEB__
		halfword = get_unaligned_be16(s);
#else
		halfword = get_unaligned_le16(s);
#endif
		checksum = add_with_carry(checksum, halfword & 0xff);
		checksum = add_with_carry(checksum, halfword >> 8);
		
#ifdef __ARMEB__
		put_unaligned_be16(halfword, d);
#else
		put_unaligned_le16(halfword, d);
#endif
		
		s += 2;
		d += 2;
		l -= 2;
	}
	
	/* Process final byte */
	if (l == 1) {
		u8 byte = *s++;
		checksum = add_with_carry(checksum, byte);
		*d++ = byte;
	}
	
	*dst = d;
	*src = s;
	*len = l;
	*sum = checksum;
}

/* Aligned source and destination copy */
static void copy_aligned(const unsigned char **src, unsigned char **dst,
			int *len, u32 *sum)
{
	const unsigned char *s = *src;
	unsigned char *d = *dst;
	int l = *len;
	u32 checksum = *sum;
	
	/* Process 16-byte blocks */
	while (l >= 16) {
		const u32 *sptr = (const u32 *)s;
		u32 *dptr = (u32 *)d;
		
		u32 w0 = sptr[0];
		u32 w1 = sptr[1];
		u32 w2 = sptr[2];
		u32 w3 = sptr[3];
		
		dptr[0] = w0;
		dptr[1] = w1;
		dptr[2] = w2;
		dptr[3] = w3;
		
		checksum = add_with_carry(checksum, w0);
		checksum = add_with_carry(checksum, w1);
		checksum = add_with_carry(checksum, w2);
		checksum = add_with_carry(checksum, w3);
		
		s += 16;
		d += 16;
		l -= 16;
	}
	
	/* Process 8-byte blocks */
	if (l >= 8) {
		const u32 *sptr = (const u32 *)s;
		u32 *dptr = (u32 *)d;
		
		u32 w0 = sptr[0];
		u32 w1 = sptr[1];
		
		dptr[0] = w0;
		dptr[1] = w1;
		
		checksum = add_with_carry(checksum, w0);
		checksum = add_with_carry(checksum, w1);
		
		s += 8;
		d += 8;
		l -= 8;
	}
	
	/* Process 4-byte block */
	if (l >= 4) {
		u32 word = *(const u32 *)s;
		*(u32 *)d = word;
		checksum = add_with_carry(checksum, word);
		s += 4;
		d += 4;
		l -= 4;
	}
	
	*dst = d;
	*src = s;
	*len = l;
	*sum = checksum;
}

/* Handle unaligned source */
static void copy_unaligned_src(const unsigned char **src, unsigned char **dst,
			      int *len, u32 *sum)
{
	const unsigned char *s = *src;
	unsigned char *d = *dst;
	int l = *len;
	u32 checksum = *sum;
	u32 carry_buffer = 0;
	int src_alignment = (unsigned long)s & 3;
	
	/* Align source pointer */
	const unsigned char *aligned_src = (const unsigned char *)((unsigned long)s & ~3);
	u32 first_word = *(const u32 *)aligned_src;
	
	switch (src_alignment) {
	case 1: /* 1 byte offset */
		carry_buffer = first_word >> 8;
		break;
	case 2: /* 2 byte offset */
		carry_buffer = first_word >> 16;
		break;
	case 3: /* 3 byte offset */
		carry_buffer = first_word >> 24;
		break;
	}
	
	s = aligned_src + 4;
	
	/* Process blocks */
	while (l >= 4) {
		u32 current_word = *(const u32 *)s;
		u32 assembled_word;
		
		switch (src_alignment) {
		case 1:
			assembled_word = carry_buffer | (current_word << 24);
			carry_buffer = current_word >> 8;
			break;
		case 2:
			assembled_word = carry_buffer | (current_word << 16);
			carry_buffer = current_word >> 16;
			break;
		case 3:
			assembled_word = carry_buffer | (current_word << 8);
			carry_buffer = current_word >> 24;
			break;
		default:
			assembled_word = 0;
			break;
		}
		
		*(u32 *)d = assembled_word;
		checksum = add_with_carry(checksum, assembled_word);
		
		s += 4;
		d += 4;
		l -= 4;
	}
	
	/* Handle remaining bytes */
	if (l > 0) {
		/* Extract remaining bytes from carry_buffer */
		const u8 *remaining = (const u8 *)&carry_buffer;
		
		if (l >= 1) {
			*d++ = remaining[0];
			checksum = add_with_carry(checksum, remaining[0]);
			l--;
		}
		if (l >= 1) {
			*d++ = remaining[1];
			checksum = add_with_carry(checksum, remaining[1]);
			l--;
		}
		if (l >= 1) {
			*d++ = remaining[2];
			checksum = add_with_carry(checksum, remaining[2]);
			l--;
		}
		if (l >= 1) {
			*d++ = remaining[3];
			checksum = add_with_carry(checksum, remaining[3]);
		}
	}
	
	*dst = d;
	*src = s;
	*len = l;
	*sum = checksum;
}

/*
 * Generic csum partial copy function
 * unsigned int csum_partial_copy_generic(const char *src, char *dst, int len, int sum)
 */
__wsum csum_partial_copy_generic(const void *src, void *dst, int len, __wsum sum)
{
	const unsigned char *s = (const unsigned char *)src;
	unsigned char *d = (unsigned char *)dst;
	u32 checksum = sum;
	void *orig_dst = dst;
	
	if (len <= 0) {
		return checksum;
	}
	
	/* Handle small copies */
	if (len <= 7) {
		handle_remainder(&s, &d, &len, &checksum);
	} else {
		/* Align destination */
		align_dst(&s, &d, &len, &checksum);
		
		/* Check source alignment */
		if (((unsigned long)s & 3) == 0) {
			/* Aligned source and destination */
			copy_aligned(&s, &d, &len, &checksum);
		} else {
			/* Unaligned source */
			copy_unaligned_src(&s, &d, &len, &checksum);
		}
		
		/* Handle any remaining bytes */
		if (len > 0) {
			handle_remainder(&s, &d, &len, &checksum);
		}
	}
	
	/* If destination was odd-aligned, rotate checksum */
	if ((unsigned long)orig_dst & 1) {
		checksum = (checksum << 24) | (checksum >> 8);
	}
	
	return checksum;
}
EXPORT_SYMBOL(csum_partial_copy_generic);
