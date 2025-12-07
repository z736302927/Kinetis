/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/csumpartialcopyuser.c
 *
 *  Copyright (C) 1995-1998 Russell King
 *  Converted from assembly to C
 *
 * 27/03/03 Ian Molton Clean up CONFIG_CPU
 */
#include <linux/types.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/checksum.h>

/* Forward declaration of the generic function */
extern __wsum csum_partial_copy_generic(const void *src, void *dst, int len, __wsum sum);

/*
 * unsigned int
 * csum_partial_copy_from_user(const char *src, char *dst, int len)
 *  src = user source, dst = kernel destination, len = length
 *  Returns : checksum or 0 on fault
 *
 * We report fault by returning 0 csum - impossible in normal case, since
 * we start with 0xffffffff for initial sum.
 */
__wsum csum_partial_copy_from_user(const void __user *src, void *dst, int len)
{
	void *kaddr;
	
	/* Try to access user memory */
	if (!access_ok(src, len)) {
		return 0; /* Access fault */
	}
	
	/* For simplicity, we'll use copy_from_user and then csum_partial_copy_generic
	 * In a real implementation, this would be optimized to avoid double copying
	 */
	kaddr = kmalloc(len, GFP_KERNEL);
	if (!kaddr) {
		return 0; /* Memory allocation failure */
	}
	
	if (copy_from_user(kaddr, src, len)) {
		kfree(kaddr);
		return 0; /* Copy fault */
	}
	
	/* Now copy and checksum */
	__wsum result = csum_partial_copy_generic(kaddr, dst, len, 0);
	
	kfree(kaddr);
	return result;
}
EXPORT_SYMBOL(csum_partial_copy_from_user);
