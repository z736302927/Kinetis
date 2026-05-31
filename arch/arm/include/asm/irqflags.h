/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ARM_IRQFLAGS_H
#define __ASM_ARM_IRQFLAGS_H

#ifdef __KERNEL__

#include <asm/ptrace.h>

/*
 * CPU interrupt mask handling.
 * All Cortex-M cores (M0/M0+/M3/M4/M7) use PRIMASK register;
 * classic ARM (ARMv4/v5/v6) uses CPSR I-bit.
 */
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#define IRQMASK_REG_NAME_R "primask"
#define IRQMASK_REG_NAME_W "primask"
#define IRQMASK_I_BIT	1
#else
#define IRQMASK_REG_NAME_R "cpsr"
#define IRQMASK_REG_NAME_W "cpsr_c"
#define IRQMASK_I_BIT	PSR_I_BIT
#endif

#if __LINUX_ARM_ARCH__ >= 6

/*
 * Cortex-M3/M4: PRIMASK = 0 (IRQ enabled), PRIMASK = 1 (IRQ disabled, only NMI).
 * Uses CMSIS __get/__set_PRIMASK intrinsics (compiler builtins, no inline asm).
 */
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include <cmsis_gcc.h>
#endif

#define arch_local_irq_save arch_local_irq_save
static inline unsigned long arch_local_irq_save(void)
{
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	unsigned long flags = __get_PRIMASK();
	__disable_irq();
	return flags;
#else
	return 0;
#endif
}

#define arch_local_irq_enable arch_local_irq_enable
static inline void arch_local_irq_enable(void)
{
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	__enable_irq();
#endif
}

#define arch_local_irq_disable arch_local_irq_disable
static inline void arch_local_irq_disable(void)
{
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	__disable_irq();
#endif
}

#define local_fiq_enable()	do { } while (0)
#define local_fiq_disable()	do { } while (0)

#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#define local_abt_enable()	do { } while (0)
#define local_abt_disable()	do { } while (0)
#else
#define local_abt_enable()
#define local_abt_disable()
#endif
#else

/*
 * Save the current interrupt enable state & disable IRQs
 */
#define arch_local_irq_save arch_local_irq_save
static inline unsigned long arch_local_irq_save(void)
{
	return 0;
}

/*
 * Enable IRQs
 */
#define arch_local_irq_enable arch_local_irq_enable
static inline void arch_local_irq_enable(void)
{
}

/*
 * Disable IRQs
 */
#define arch_local_irq_disable arch_local_irq_disable
static inline void arch_local_irq_disable(void)
{
}

/*
 * Enable FIQs
 */
#define local_fiq_enable()

/*
 * Disable FIQs
 */
#define local_fiq_disable()

#define local_fiq_enable()	do { } while (0)
#define local_fiq_disable()	do { } while (0)
#define local_abt_enable()	do { } while (0)
#define local_abt_disable()	do { } while (0)
#endif

/*
 * Save the current interrupt enable state.
 */
#define arch_local_save_flags arch_local_save_flags
static inline unsigned long arch_local_save_flags(void)
{
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	return __get_PRIMASK();
#else
	return 0;
#endif
}

/*
 * restore saved IRQ & FIQ state
 */
#define arch_local_irq_restore arch_local_irq_restore
static inline void arch_local_irq_restore(unsigned long flags)
{
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	__set_PRIMASK(flags);
#endif
}

#define arch_irqs_disabled_flags arch_irqs_disabled_flags
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return flags & IRQMASK_I_BIT;
}

#include <asm-generic/irqflags.h>

#endif /* ifdef __KERNEL__ */
#endif /* ifndef __ASM_ARM_IRQFLAGS_H */
