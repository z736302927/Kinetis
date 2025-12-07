/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/csumpartialcopy.c
 *
 *  Copyright (C) 1995-1998 Russell King
 *  Converted from assembly to C
 */
#include <linux/types.h>
#include <linux/module.h>
#include <asm/checksum.h>

/*
 * Function: __u32 csum_partial_copy_nocheck(const char *src, char *dst, int len)
 * Params  : src = source, dst = destination, len = length
 * Returns : new checksum
 */
__wsum csum_partial_copy_nocheck(const void *src, void *dst, int len)
{
	/* Initialize sum to 0 (will be set to -1 in the generic function) */
	return csum_partial_copy_generic(src, dst, len, 0);
}
EXPORT_SYMBOL(csum_partial_copy_nocheck);
