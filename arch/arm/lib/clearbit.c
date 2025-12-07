/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/clearbit.S
 *
 *  Copyright (C) 1995-1996 Russell King
 *  Converted from assembly to C
 */
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/irqflags.h>
#include <asm/barrier.h>

/*
 * Atomic bit operations for ARM - clear bit versions
 * These functions are called by the macros in asm/bitops.h
 */

/**
 * _clear_bit - Clear a bit in memory (atomic version for ARM)
 * @nr:    bit to clear
 * @addr:  address to start counting from
 * 
 * This is the atomic version called by the clear_bit() macro.
 * On ARM, we disable interrupts to ensure atomicity.
 * The original assembly used BIC (Bit Clear) instruction.
 */
void _clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long flags;
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	/* Disable interrupts to make this atomic */
	raw_local_irq_save(flags);
	*p &= ~mask;  /* Equivalent to BIC instruction in assembly */
	raw_local_irq_restore(flags);
}