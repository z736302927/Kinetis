/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ARM_CMPXCHG_H
#define __ASM_ARM_CMPXCHG_H

#include <linux/irqflags.h>
#include <linux/prefetch.h>
#include <asm/barrier.h>

#if defined(CONFIG_CPU_SA1100) || defined(CONFIG_CPU_SA110)
/*
 * On the StrongARM, "swp" is terminally broken since it bypasses the
 * cache totally.  This means that the cache becomes inconsistent, and,
 * since we use normal loads/stores as well, this is really bad.
 * Typically, this causes oopsen in filp_close, but could have other,
 * more disastrous effects.  There are two work-arounds:
 *  1. Disable interrupts and emulate the atomic swap
 *  2. Clean the cache, perform atomic swap, flush the cache
 *
 * We choose (1) since its the "easiest" to achieve here and is not
 * dependent on the processor type.
 *
 * NOTE that this solution won't work on an SMP system, so explcitly
 * forbid it here.
 */
#define swp_is_buggy
#endif

static inline void __bad_xchg(volatile void *ptr, int size)
{
	/* This function is called when xchg() is used with an unsupported size */
	(void)ptr;  /* Avoid unused parameter warning */
	(void)size;  /* Avoid unused parameter warning */
}

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	unsigned long ret;
#ifdef swp_is_buggy
	unsigned long flags;
#endif
#if __LINUX_ARM_ARCH__ >= 6
	unsigned int tmp;
#endif

	prefetchw((const void *)ptr);

	switch (size) {
#if __LINUX_ARM_ARCH__ >= 6
#ifndef CONFIG_CPU_V6 /* MIN ARCH >= V6K */
	case 1:
		do {
			ret = *(volatile unsigned char *)ptr;
			*(volatile unsigned char *)ptr = (unsigned char)x;
		} while (0);
		break;
	case 2:
		do {
			ret = *(volatile unsigned short *)ptr;
			*(volatile unsigned short *)ptr = (unsigned short)x;
		} while (0);
		break;
#endif
	case 4:
		do {
			ret = *(volatile unsigned long *)ptr;
			*(volatile unsigned long *)ptr = x;
		} while (0);
		break;
#elif defined(swp_is_buggy)
#ifdef CONFIG_SMP
#error SMP is not supported on this platform
#endif
	case 1:
		raw_local_irq_save(flags);
		ret = *(volatile unsigned char *)ptr;
		*(volatile unsigned char *)ptr = x;
		raw_local_irq_restore(flags);
		break;

	case 4:
		raw_local_irq_save(flags);
		ret = *(volatile unsigned long *)ptr;
		*(volatile unsigned long *)ptr = x;
		raw_local_irq_restore(flags);
		break;
#else
	case 1:
		ret = *(volatile unsigned char *)ptr;
		*(volatile unsigned char *)ptr = (unsigned char)x;
		break;
	case 4:
		ret = *(volatile unsigned long *)ptr;
		*(volatile unsigned long *)ptr = x;
		break;
#endif
	default:
		/* Cause a link-time error, the xchg() size is not supported */
		__bad_xchg(ptr, size), ret = 0;
		break;
	}

	return ret;
}

#define xchg_relaxed(ptr, x) ({						\
	(__typeof__(*(ptr)))__xchg((unsigned long)(x), (ptr),		\
				   sizeof(*(ptr)));			\
})

#include <asm-generic/cmpxchg-local.h>

#if __LINUX_ARM_ARCH__ < 6
/* min ARCH < ARMv6 */

#ifdef CONFIG_SMP
#error "SMP is not supported on this platform"
#endif

#define xchg xchg_relaxed

/*
 * cmpxchg_local and cmpxchg64_local are atomic wrt current CPU. Always make
 * them available.
 */
#define cmpxchg_local(ptr, o, n) ({					\
	(__typeof(*ptr))__cmpxchg_local_generic((ptr),			\
					        (unsigned long)(o),	\
					        (unsigned long)(n),	\
					        sizeof(*(ptr)));	\
})

#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))

#include <asm-generic/cmpxchg.h>

#else	/* min ARCH >= ARMv6 */

static inline void __bad_cmpxchg(volatile void *ptr, int size)
{
	/* This function is called when cmpxchg() is used with an unsupported size */
	(void)ptr;  /* Avoid unused parameter warning */
	(void)size;  /* Avoid unused parameter warning */
}

/*
 * cmpxchg only support 32-bits operands on ARMv6.
 */

static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
				      unsigned long new, int size)
{
	unsigned long oldval, res;

	prefetchw((const void *)ptr);

	switch (size) {
#ifndef CONFIG_CPU_V6	/* min ARCH >= ARMv6K */
	case 1:
		do {
			oldval = *(volatile unsigned char *)ptr;
			if (oldval == (unsigned char)old) {
				*(volatile unsigned char *)ptr = (unsigned char)new;
				res = 0;
			} else {
				res = 1;
			}
		} while (res);
		break;
	case 2:
		do {
			oldval = *(volatile unsigned short *)ptr;
			if (oldval == (unsigned short)old) {
				*(volatile unsigned short *)ptr = (unsigned short)new;
				res = 0;
			} else {
				res = 1;
			}
		} while (res);
		break;
#endif
	case 4:
		do {
			oldval = *(volatile unsigned long *)ptr;
			if (oldval == old) {
				*(volatile unsigned long *)ptr = new;
				res = 0;
			} else {
				res = 1;
			}
		} while (res);
		break;
	default:
		__bad_cmpxchg(ptr, size);
		oldval = 0;
	}

	return oldval;
}

#define cmpxchg_relaxed(ptr,o,n) ({					\
	(__typeof__(*(ptr)))__cmpxchg((ptr),				\
				      (unsigned long)(o),		\
				      (unsigned long)(n),		\
				      sizeof(*(ptr)));			\
})

static inline unsigned long __cmpxchg_local(volatile void *ptr,
					    unsigned long old,
					    unsigned long new, int size)
{
	unsigned long ret;

	switch (size) {
#ifdef CONFIG_CPU_V6	/* min ARCH == ARMv6 */
	case 1:
	case 2:
		ret = __cmpxchg_local_generic(ptr, old, new, size);
		break;
#endif
	default:
		ret = __cmpxchg(ptr, old, new, size);
	}

	return ret;
}

#define cmpxchg_local(ptr, o, n) ({					\
	(__typeof(*ptr))__cmpxchg_local((ptr),				\
				        (unsigned long)(o),		\
				        (unsigned long)(n),		\
				        sizeof(*(ptr)));		\
})

static inline unsigned long long __cmpxchg64(unsigned long long *ptr,
					     unsigned long long old,
					     unsigned long long new)
{
	unsigned long long oldval;
	unsigned long res;

	prefetchw(ptr);

	do {
		oldval = *ptr;
		if (oldval == old) {
			*ptr = new;
			res = 0;
		} else {
			res = 1;
			break;
		}
	} while (res);

	return oldval;
}

#define cmpxchg64_relaxed(ptr, o, n) ({					\
	(__typeof__(*(ptr)))__cmpxchg64((ptr),				\
					(unsigned long long)(o),	\
					(unsigned long long)(n));	\
})

#define cmpxchg64_local(ptr, o, n) cmpxchg64_relaxed((ptr), (o), (n))

#endif	/* __LINUX_ARM_ARCH__ >= 6 */

#endif /* __ASM_ARM_CMPXCHG_H */
