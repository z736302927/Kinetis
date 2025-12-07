/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  arch/arm/include/asm/checksum.h
 *
 * IP checksum routines
 *
 * Copyright (C) Original authors of ../asm-i386/checksum.h
 * Copyright (C) 1996-1999 Russell King
 */
#ifndef __ASM_ARM_CHECKSUM_H
#define __ASM_ARM_CHECKSUM_H

#include <linux/in6.h>
#include <linux/uaccess.h>

/*
 * computes the checksum of a memory block at buff, length len,
 * and adds in "sum" (32-bit)
 *
 * returns a 32-bit number suitable for feeding into itself
 * or csum_tcpudp_magic
 *
 * this function must be called with even lengths, except
 * for the last fragment, which may be odd
 *
 * it's best to have buff aligned on a 32-bit boundary
 */
__wsum csum_partial(const void *buff, int len, __wsum sum);

/*
 * the same as csum_partial, but copies from src while it
 * checksums, and handles user-space pointer exceptions correctly, when needed.
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */

__wsum
csum_partial_copy_nocheck(const void *src, void *dst, int len);

__wsum
csum_partial_copy_from_user(const void __user *src, void *dst, int len);

/* Internal generic function for csum partial copy */
__wsum csum_partial_copy_generic(const void *src, void *dst, int len, __wsum sum);

#define _HAVE_ARCH_COPY_AND_CSUM_FROM_USER
#define _HAVE_ARCH_CSUM_AND_COPY
static inline
__wsum csum_and_copy_from_user(const void __user *src, void *dst, int len)
{
	if (!access_ok(src, len))
		return 0;

	return csum_partial_copy_from_user(src, dst, len);
}

/*
 * 	Fold a partial checksum without adding pseudo headers
 */
static inline __sum16 csum_fold(__wsum sum)
{
	u32 tmp = sum;
	tmp = (tmp & 0xffff) + (tmp >> 16);
	tmp = (tmp & 0xffff) + (tmp >> 16);
	return (__force __sum16)(~tmp & 0xffff);
}

/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 */
static inline __sum16
ip_fast_csum(const void *iph, unsigned int ihl)
{
	unsigned int tmp1;
	__wsum sum;
	const unsigned int *ptr = (const unsigned int *)iph;
	
	sum = ptr[0];
	tmp1 = ptr[1];
	sum += tmp1;
	ihl -= 5;
	
	tmp1 = ptr[2];
	sum += tmp1;
	tmp1 = ptr[3];
	sum += tmp1;
	
	while (ihl > 0) {
		tmp1 = ptr[4];
		sum += tmp1;
		ptr++;
		ihl--;
	}
	
	sum += 0; /* Add in carry */
	return csum_fold(sum);
}

static inline __wsum
csum_tcpudp_nofold(__be32 saddr, __be32 daddr, __u32 len,
		   __u8 proto, __wsum sum)
{
	u32 lenprot = len + proto;
	
	if (__builtin_constant_p(sum) && sum == 0) {
		sum = daddr + saddr;
#ifdef __ARMEB__
		sum += lenprot;
#else
		sum += (lenprot << 8) | (lenprot >> 24);
#endif
	} else {
		sum = sum + daddr + saddr;
#ifdef __ARMEB__
		sum += lenprot;
#else
		sum += (lenprot << 8) | (lenprot >> 24);
#endif
	}
	/* Add final carry */
	if (sum < lenprot)
		sum++;
	return sum;
}	
/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */
static inline __sum16
csum_tcpudp_magic(__be32 saddr, __be32 daddr, __u32 len,
		  __u8 proto, __wsum sum)
{
	return csum_fold(csum_tcpudp_nofold(saddr, daddr, len, proto, sum));
}


/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */
static inline __sum16
ip_compute_csum(const void *buff, int len)
{
	return csum_fold(csum_partial(buff, len, 0));
}

#define _HAVE_ARCH_IPV6_CSUM
extern __wsum
__csum_ipv6_magic(const struct in6_addr *saddr, const struct in6_addr *daddr, __be32 len,
		__be32 proto, __wsum sum);

static inline __sum16
csum_ipv6_magic(const struct in6_addr *saddr, const struct in6_addr *daddr,
		__u32 len, __u8 proto, __wsum sum)
{
	return csum_fold(__csum_ipv6_magic(saddr, daddr, htonl(len),
					   htonl(proto), sum));
}
#endif
