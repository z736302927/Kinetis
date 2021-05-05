// SPDX-License-Identifier: GPL-2.0+
/*
 * This file contains the jiffies based clocksource.
 *
 * Copyright (C) 2004, 2005 IBM, John Stultz (johnstul@us.ibm.com)
 */
#include <linux/export.h>
#include <linux/jiffies.h>
#include <linux/init.h>

unsigned long volatile jiffies = 0;

/* Since jiffies uses a simple TICK_NSEC multiplier
 * conversion, the .shift value could be zero. However
 * this would make NTP adjustments impossible as they are
 * in units of 1/2^.shift. Thus we use JIFFIES_SHIFT to
 * shift both the nominator and denominator the same
 * amount, and give ntp adjustments in units of 1/2^8
 *
 * The value 8 is somewhat carefully chosen, as anything
 * larger can result in overflows. TICK_NSEC grows as HZ
 * shrinks, so values greater than 8 overflow 32bits when
 * HZ=100.
 */
#if HZ < 34
#define JIFFIES_SHIFT	6
#elif HZ < 67
#define JIFFIES_SHIFT	7
#else
#define JIFFIES_SHIFT	8
#endif

static u64 jiffies_read(void)
{
	return (u64) jiffies;
}

#if (BITS_PER_LONG < 64)
u64 get_jiffies_64(void)
{
	return jiffies_64;
}
EXPORT_SYMBOL(get_jiffies_64);
#endif

EXPORT_SYMBOL(jiffies);
