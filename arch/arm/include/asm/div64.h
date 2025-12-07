/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ARM_DIV64
#define __ASM_ARM_DIV64

#include <linux/types.h>
#include <asm/compiler.h>

/*
 * The semantics of __div64_32() are:
 *
 * uint32_t __div64_32(uint64_t *n, uint32_t base)
 * {
 * 	uint32_t remainder = *n % base;
 * 	*n = *n / base;
 * 	return remainder;
 * }
 *
 * In other words, a 64-bit dividend with a 32-bit divisor producing
 * a 64-bit result and a 32-bit remainder.  To accomplish this optimally
 * we override the generic version in lib/div64.c to call our __do_div64
 * assembly implementation with completely non standard calling convention
 * for arguments and results (beware).
 */

#ifdef __ARMEB__
#define __xh "r0"
#define __xl "r1"
#else
#define __xl "r0"
#define __xh "r1"
#endif

static inline uint32_t __div64_32(uint64_t *n, uint32_t base)
{
	uint64_t dividend = *n;
	uint32_t remainder;
	
	if (base == 0) {
		/* Division by zero - undefined behavior */
		return 0;
	}
	
	remainder = dividend % base;
	*n = dividend / base;
	
	return remainder;
}
#define __div64_32 __div64_32

#if !defined(CONFIG_AEABI)

/*
 * In OABI configurations, some uses of the do_div function
 * cause gcc to run out of registers. To work around that,
 * we can force the use of the out-of-line version for
 * configurations that build a OABI kernel.
 */
#define do_div(n, base) __div64_32(&(n), base)

#else

/*
 * gcc versions earlier than 4.0 are simply too problematic for the
 * __div64_const32() code in asm-generic/div64.h. First there is
 * gcc PR 15089 that tend to trig on more complex constructs, spurious
 * .global __udivsi3 are inserted even if none of those symbols are
 * referenced in the generated code, and those gcc versions are not able
 * to do constant propagation on long long values anyway.
 */

#define __div64_const32_is_OK (__GNUC__ >= 4)

static inline uint64_t __arch_xprod_64(uint64_t m, uint64_t n, bool bias)
{
	uint64_t res;
	uint64_t tmp = 0;

	if (!bias) {
		/* Simple 32x32 multiply, result in lower 64 bits */
		res = (uint64_t)((uint32_t)m) * ((uint32_t)n);
	} else if (!(m & ((1ULL << 63) | (1ULL << 31)))) {
		/* No sign bits in problematic positions */
		res = m;
		res += (uint64_t)((uint32_t)m) * ((uint32_t)n);
	} else {
		/* More complex case - handle sign bits properly */
		res = (uint64_t)((uint32_t)m) * ((uint32_t)n);
		/* Handle carry from lower 32 bits */
		if (res + (uint64_t)((uint32_t)m) < res) {
			tmp = 1;
		}
		res += (uint64_t)((uint32_t)m) * ((uint32_t)n);
		res += tmp;
	}

	/* Upper 32 bits multiplication */
	if (!(m & ((1ULL << 63) | (1ULL << 31)))) {
		res += ((uint64_t)((uint32_t)(m >> 32))) * ((uint32_t)n);
		res += ((uint64_t)((uint32_t)m)) * ((uint32_t)(n >> 32));
		res += ((uint64_t)((uint32_t)(m >> 32))) * ((uint32_t)(n >> 32)) << 32;
	} else {
		res += ((uint64_t)((uint32_t)(m >> 32))) * ((uint32_t)n);
		res += ((uint64_t)((uint32_t)m)) * ((uint32_t)(n >> 32));
		tmp = (res + ((uint64_t)((uint32_t)(m >> 32))) * ((uint32_t)(n >> 32))) < res;
		res += ((uint64_t)((uint32_t)(m >> 32))) * ((uint32_t)(n >> 32));
		res += tmp;
	}

	return res;
}
#define __arch_xprod_64 __arch_xprod_64

#include <asm-generic/div64.h>

#endif

#endif
