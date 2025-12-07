/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  arch/arm/include/asm/atomic.h
 *
 *  Copyright (C) 1996 Russell King.
 *  Copyright (C) 2002 Deep Blue Solutions Ltd.
 */
#ifndef __ASM_ARM_ATOMIC_H
#define __ASM_ARM_ATOMIC_H

#include <linux/compiler.h>
#include <linux/prefetch.h>
#include <linux/types.h>
#include <linux/irqflags.h>
#include <linux/string.h>
#include <asm/barrier.h>
#include <asm/cmpxchg.h>

#ifdef __KERNEL__

/*
 * On ARM, ordinary assignment (str instruction) doesn't clear the local
 * strex/ldrex monitor on some implementations. The reason we can use it for
 * atomic_set() is the clrex or dummy strex done on every exception return.
 */
#define atomic_read(v)	READ_ONCE((v)->counter)
#define atomic_set(v,i)	WRITE_ONCE(((v)->counter), (i))

#if __LINUX_ARM_ARCH__ >= 6

/*
 * ARMv6 UP and SMP safe atomic ops.  We use load exclusive and
 * store exclusive to ensure that these are atomic.  We may loop
 * to ensure that the update happens.
 */

#define ATOMIC_OP(op, c_op, asm_op)					\
static inline void atomic_##op(int i, atomic_t *v)			\
{									\
	unsigned long flags;						\
	int old_val;							\
									\
	prefetchw(&v->counter);						\
									\
	/* Use interrupt disable for atomic operation */		\
	raw_local_irq_save(flags);					\
	old_val = v->counter;						\
	v->counter c_op i;						\
	raw_local_irq_restore(flags);					\
}									\

#define ATOMIC_OP_RETURN(op, c_op, asm_op)				\
static inline int atomic_##op##_return_relaxed(int i, atomic_t *v)	\
{									\
	unsigned long flags;						\
	int result;							\
									\
	prefetchw(&v->counter);						\
									\
	/* Use interrupt disable for atomic operation */		\
	raw_local_irq_save(flags);					\
	v->counter c_op i;						\
	result = v->counter;						\
	raw_local_irq_restore(flags);					\
									\
	return result;							\
}

#define ATOMIC_FETCH_OP(op, c_op, asm_op)				\
static inline int atomic_fetch_##op##_relaxed(int i, atomic_t *v)	\
{									\
	unsigned long flags;						\
	int result, val;						\
									\
	prefetchw(&v->counter);						\
									\
	/* Use interrupt disable for atomic operation */		\
	raw_local_irq_save(flags);					\
	result = v->counter;						\
	v->counter c_op i;						\
	raw_local_irq_restore(flags);					\
									\
	return result;							\
}

#define atomic_add_return_relaxed	atomic_add_return_relaxed
#define atomic_sub_return_relaxed	atomic_sub_return_relaxed
#define atomic_fetch_add_relaxed	atomic_fetch_add_relaxed
#define atomic_fetch_sub_relaxed	atomic_fetch_sub_relaxed

#define atomic_fetch_and_relaxed	atomic_fetch_and_relaxed
#define atomic_fetch_andnot_relaxed	atomic_fetch_andnot_relaxed
#define atomic_fetch_or_relaxed		atomic_fetch_or_relaxed
#define atomic_fetch_xor_relaxed	atomic_fetch_xor_relaxed

static inline int atomic_cmpxchg_relaxed(atomic_t *ptr, int old, int new)
{
	unsigned long flags;
	int oldval;

	prefetchw(&ptr->counter);

	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	oldval = ptr->counter;
	if (oldval == old)
		ptr->counter = new;
	raw_local_irq_restore(flags);

	return oldval;
}
#define atomic_cmpxchg_relaxed		atomic_cmpxchg_relaxed

static inline int atomic_fetch_add_unless(atomic_t *v, int a, int u)
{
	unsigned long flags;
	int oldval, newval;

	smp_mb();
	prefetchw(&v->counter);

	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	oldval = v->counter;
	if (oldval != u) {
		v->counter += a;
	}
	raw_local_irq_restore(flags);

	if (oldval != u)
		smp_mb();

	return oldval;
}
#define atomic_fetch_add_unless		atomic_fetch_add_unless

#else /* ARM_ARCH_6 */

#ifdef CONFIG_SMP
#error SMP not supported on pre-ARMv6 CPUs
#endif

#define ATOMIC_OP(op, c_op, asm_op)					\
static inline void atomic_##op(int i, atomic_t *v)			\
{									\
	unsigned long flags;						\
									\
	raw_local_irq_save(flags);					\
	v->counter c_op i;						\
	raw_local_irq_restore(flags);					\
}									\

#define ATOMIC_OP_RETURN(op, c_op, asm_op)				\
static inline int atomic_##op##_return(int i, atomic_t *v)		\
{									\
	unsigned long flags;						\
	int val;							\
									\
	raw_local_irq_save(flags);					\
	v->counter c_op i;						\
	val = v->counter;						\
	raw_local_irq_restore(flags);					\
									\
	return val;							\
}

#define ATOMIC_FETCH_OP(op, c_op, asm_op)				\
static inline int atomic_fetch_##op(int i, atomic_t *v)			\
{									\
	unsigned long flags;						\
	int val;							\
									\
	raw_local_irq_save(flags);					\
	val = v->counter;						\
	v->counter c_op i;						\
	raw_local_irq_restore(flags);					\
									\
	return val;							\
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	int ret;
	unsigned long flags;

	raw_local_irq_save(flags);
	ret = v->counter;
	if (likely(ret == old))
		v->counter = new;
	raw_local_irq_restore(flags);

	return ret;
}

#define atomic_fetch_andnot		atomic_fetch_andnot

#endif /* __LINUX_ARM_ARCH__ */

#define ATOMIC_OPS(op, c_op, asm_op)					\
	ATOMIC_OP(op, c_op, asm_op)					\
	ATOMIC_OP_RETURN(op, c_op, asm_op)				\
	ATOMIC_FETCH_OP(op, c_op, asm_op)

ATOMIC_OPS(add, +=, add)
ATOMIC_OPS(sub, -=, sub)

#define atomic_andnot atomic_andnot

#undef ATOMIC_OPS
#define ATOMIC_OPS(op, c_op, asm_op)					\
	ATOMIC_OP(op, c_op, asm_op)					\
	ATOMIC_FETCH_OP(op, c_op, asm_op)

ATOMIC_OPS(and, &=, and)
ATOMIC_OPS(andnot, &= ~, bic)
ATOMIC_OPS(or,  |=, orr)
ATOMIC_OPS(xor, ^=, eor)

#undef ATOMIC_OPS
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN
#undef ATOMIC_OP

#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

#ifndef CONFIG_GENERIC_ATOMIC64
typedef struct {
	s64 counter;
} atomic64_t;

#define ATOMIC64_INIT(i) { (i) }

#ifdef CONFIG_ARM_LPAE
static inline s64 atomic64_read(const atomic64_t *v)
{
	s64 result;

	/* Direct read of 64-bit value */
	result = v->counter;

	return result;
}

static inline void atomic64_set(atomic64_t *v, s64 i)
{
	/* Direct write of 64-bit value */
	v->counter = i;
}
#else
static inline s64 atomic64_read(const atomic64_t *v)
{
	s64 result;

	/* Direct read of 64-bit value */
	result = v->counter;

	return result;
}

static inline void atomic64_set(atomic64_t *v, s64 i)
{
	unsigned long flags;

	prefetchw(&v->counter);
	
	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	v->counter = i;
	raw_local_irq_restore(flags);
}
#endif

#define ATOMIC64_OP(op, op1, op2)					\
static inline void atomic64_##op(s64 i, atomic64_t *v)			\
{									\
	unsigned long flags;							\
									\
	prefetchw(&v->counter);						\
									\
	/* Use interrupt disable for atomic operation */		\
	raw_local_irq_save(flags);					\
	v->counter += i; /* atomic operation: op */		\
	raw_local_irq_restore(flags);					\
}									\

#define ATOMIC64_OP_RETURN(op, op1, op2)				\
static inline s64							\
atomic64_##op##_return_relaxed(s64 i, atomic64_t *v)			\
{									\
	unsigned long flags;						\
	s64 result;							\
									\
	prefetchw(&v->counter);						\
									\
	/* Use interrupt disable for atomic operation */		\
	raw_local_irq_save(flags);					\
	if (strcmp(#op, "add") == 0) {				\
		v->counter += i;						\
	} else if (strcmp(#op, "sub") == 0) {			\
		v->counter -= i;						\
	} else if (strcmp(#op, "and") == 0) {			\
		v->counter &= i;						\
	} else if (strcmp(#op, "andnot") == 0) {			\
		v->counter &= ~i;					\
	} else if (strcmp(#op, "or") == 0) {				\
		v->counter |= i;						\
	} else if (strcmp(#op, "xor") == 0) {			\
		v->counter ^= i;						\
	}								\
	result = v->counter;						\
	raw_local_irq_restore(flags);					\
									\
	return result;							\
}

#define ATOMIC64_FETCH_OP(op, op1, op2)					\
static inline s64							\
atomic64_fetch_##op##_relaxed(s64 i, atomic64_t *v)			\
{									\
	unsigned long flags;						\
	s64 result, val;						\
									\
	prefetchw(&v->counter);						\
									\
	/* Use interrupt disable for atomic operation */		\
	raw_local_irq_save(flags);					\
	result = v->counter;						\
	if (strcmp(#op, "add") == 0) {				\
		v->counter += i;						\
	} else if (strcmp(#op, "sub") == 0) {			\
		v->counter -= i;						\
	} else if (strcmp(#op, "and") == 0) {			\
		v->counter &= i;						\
	} else if (strcmp(#op, "andnot") == 0) {			\
		v->counter &= ~i;					\
	} else if (strcmp(#op, "or") == 0) {				\
		v->counter |= i;						\
	} else if (strcmp(#op, "xor") == 0) {			\
		v->counter ^= i;						\
	}								\
	raw_local_irq_restore(flags);					\
									\
	return result;							\
}

#define ATOMIC64_OPS(op, op1, op2)					\
	ATOMIC64_OP(op, op1, op2)					\
	ATOMIC64_OP_RETURN(op, op1, op2)				\
	ATOMIC64_FETCH_OP(op, op1, op2)

ATOMIC64_OPS(add, adds, adc)
ATOMIC64_OPS(sub, subs, sbc)

#define atomic64_add_return_relaxed	atomic64_add_return_relaxed
#define atomic64_sub_return_relaxed	atomic64_sub_return_relaxed
#define atomic64_fetch_add_relaxed	atomic64_fetch_add_relaxed
#define atomic64_fetch_sub_relaxed	atomic64_fetch_sub_relaxed

#undef ATOMIC64_OPS
#define ATOMIC64_OPS(op, op1, op2)					\
	ATOMIC64_OP(op, op1, op2)					\
	ATOMIC64_FETCH_OP(op, op1, op2)

#define atomic64_andnot atomic64_andnot

ATOMIC64_OPS(and, and, and)
ATOMIC64_OPS(andnot, bic, bic)
ATOMIC64_OPS(or,  orr, orr)
ATOMIC64_OPS(xor, eor, eor)

#define atomic64_fetch_and_relaxed	atomic64_fetch_and_relaxed
#define atomic64_fetch_andnot_relaxed	atomic64_fetch_andnot_relaxed
#define atomic64_fetch_or_relaxed	atomic64_fetch_or_relaxed
#define atomic64_fetch_xor_relaxed	atomic64_fetch_xor_relaxed

#undef ATOMIC64_OPS
#undef ATOMIC64_FETCH_OP
#undef ATOMIC64_OP_RETURN
#undef ATOMIC64_OP

static inline s64 atomic64_cmpxchg_relaxed(atomic64_t *ptr, s64 old, s64 new)
{
	unsigned long flags;
	s64 oldval;

	prefetchw(&ptr->counter);

	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	oldval = ptr->counter;
	if (oldval == old)
		ptr->counter = new;
	raw_local_irq_restore(flags);

	return oldval;
}
#define atomic64_cmpxchg_relaxed	atomic64_cmpxchg_relaxed

static inline s64 atomic64_xchg_relaxed(atomic64_t *ptr, s64 new)
{
	unsigned long flags;
	s64 result;

	prefetchw(&ptr->counter);

	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	result = ptr->counter;
	ptr->counter = new;
	raw_local_irq_restore(flags);

	return result;
}
#define atomic64_xchg_relaxed		atomic64_xchg_relaxed

static inline s64 atomic64_dec_if_positive(atomic64_t *v)
{
	unsigned long flags;
	s64 result;

	smp_mb();
	prefetchw(&v->counter);

	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	result = v->counter;
	if (result > 0) {
		result--;
		v->counter = result;
	}
	raw_local_irq_restore(flags);

	smp_mb();

	return result;
}
#define atomic64_dec_if_positive atomic64_dec_if_positive

static inline s64 atomic64_fetch_add_unless(atomic64_t *v, s64 a, s64 u)
{
	unsigned long flags;
	s64 oldval;

	smp_mb();
	prefetchw(&v->counter);

	/* Use interrupt disable for atomic operation */
	raw_local_irq_save(flags);
	oldval = v->counter;
	if (oldval != u) {
		v->counter += a;
	}
	raw_local_irq_restore(flags);

	if (oldval != u)
		smp_mb();

	return oldval;
}
#define atomic64_fetch_add_unless atomic64_fetch_add_unless

#endif /* !CONFIG_GENERIC_ATOMIC64 */
#endif
#endif
