/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/setbit.S
 *
 *  Copyright (C) 1995-1996 Russell King
 *  Converted from assembly to C
 */
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/irqflags.h>
#include <asm/barrier.h>

/*
 * Atomic bit operations for ARM
 * These functions are called by the macros in asm/bitops.h
 */

/**
 * _set_bit - Set a bit in memory (atomic version for ARM)
 * @nr: the bit to set
 * @addr: the address to start counting from
 * 
 * This is the atomic version called by the set_bit() macro.
 * On ARM, we disable interrupts to ensure atomicity.
 */
void _set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long flags;
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	/* Disable interrupts to make this atomic */
	raw_local_irq_save(flags);
	*p |= mask;
	raw_local_irq_restore(flags);
}



/**
 * _change_bit - Toggle a bit in memory (atomic version for ARM)
 * @nr: the bit to change
 * @addr: the address to start counting from
 * 
 * This is the atomic version called by the change_bit() macro.
 * On ARM, we disable interrupts to ensure atomicity.
 */
void _change_bit(int nr, volatile unsigned long *addr)
{
	unsigned long flags;
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	/* Disable interrupts to make this atomic */
	raw_local_irq_save(flags);
	*p ^= mask;
	raw_local_irq_restore(flags);
}

/**
 * _test_and_set_bit - Set a bit and return its old value (atomic version for ARM)
 * @nr: Bit to set
 * @addr: Address to count from
 * 
 * This is the atomic version called by the test_and_set_bit() macro.
 * On ARM, we disable interrupts to ensure atomicity.
 */
int _test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long flags;
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;
	int result;
	
	/* Disable interrupts to make this atomic */
	raw_local_irq_save(flags);
	old = *p;
	*p = old | mask;
	raw_local_irq_restore(flags);
	
	result = (old & mask) != 0;
	return result;
}

/**
 * _test_and_clear_bit - Clear a bit and return its old value (atomic version for ARM)
 * @nr: Bit to clear
 * @addr: Address to count from
 * 
 * This is the atomic version called by the test_and_clear_bit() macro.
 * On ARM, we disable interrupts to ensure atomicity.
 */
int _test_and_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long flags;
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;
	int result;
	
	/* Disable interrupts to make this atomic */
	raw_local_irq_save(flags);
	old = *p;
	*p = old & ~mask;
	raw_local_irq_restore(flags);
	
	result = (old & mask) != 0;
	return result;
}

/**
 * _test_and_change_bit - Change a bit and return its old value (atomic version for ARM)
 * @nr: Bit to change
 * @addr: Address to count from
 * 
 * This is the atomic version called by the test_and_change_bit() macro.
 * On ARM, we disable interrupts to ensure atomicity.
 */
int _test_and_change_bit(int nr, volatile unsigned long *addr)
{
	unsigned long flags;
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;
	int result;
	
	/* Disable interrupts to make this atomic */
	raw_local_irq_save(flags);
	old = *p;
	*p = old ^ mask;
	raw_local_irq_restore(flags);
	
	result = (old & mask) != 0;
	return result;
}