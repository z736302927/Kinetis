/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/delay.S
 *
 *  Copyright (C) 1995, 1996 Russell King
 */
#include <linux/linkage.h>
#include <asm/delay.h>

extern unsigned long loops_per_jiffy;

/*
 * loops = r0 * HZ * loops_per_jiffy / 1000000
 *
 * r0  <= 2000
 * HZ  <= 1000
 */

void __loop_delay(unsigned long loops)
{
	while (--loops)
		;
}

void __loop_const_udelay(unsigned long xloops)
{
	unsigned long long tmp;
	unsigned long loops;

	tmp  = (unsigned long long)xloops * loops_per_jiffy;
	tmp += 0xFFFFFFFFULL;
	loops = (unsigned long)(tmp >> 31);

	if (loops == 0)
		return;

	__loop_delay(loops);
}

void __loop_udelay(unsigned long usecs)
{
	__loop_const_udelay(usecs * UDELAY_MULT);
}
