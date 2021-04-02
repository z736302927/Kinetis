// SPDX-License-Identifier: GPL-2.0-only
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/err.h>


/**
 * kfree_const - conditionally free memory
 * @x: pointer to the memory
 *
 * Function calls kfree only if @x is not in .rodata section.
 */
void kfree_const(const void *x)
{
	kfree(x);
}
EXPORT_SYMBOL(kfree_const);

/**
 * kstrdup - allocate space for and copy an existing string
 * @s: the string to duplicate
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Return: newly allocated copy of @s or %NULL in case of error
 */
char *kstrdup(const char *s, gfp_t gfp)
{
	size_t len;
	char *buf;

	if (!s)
		return NULL;

	len = strlen(s) + 1;
	buf = kmalloc_track_caller(len, gfp);
	if (buf)
		memcpy(buf, s, len);
	return buf;
}
EXPORT_SYMBOL(kstrdup);

/**
 * kstrdup_const - conditionally duplicate an existing const string
 * @s: the string to duplicate
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Note: Strings allocated by kstrdup_const should be freed by kfree_const and
 * must not be passed to krealloc().
 *
 * Return: source string if it is in .rodata section otherwise
 * fallback to kstrdup.
 */
const char *kstrdup_const(const char *s, gfp_t gfp)
{
	return kstrdup(s, gfp);
}
EXPORT_SYMBOL(kstrdup_const);

/**
 * kstrndup - allocate space for and copy an existing string
 * @s: the string to duplicate
 * @max: read at most @max chars from @s
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Note: Use kmemdup_nul() instead if the size is known exactly.
 *
 * Return: newly allocated copy of @s or %NULL in case of error
 */
char *kstrndup(const char *s, size_t max, gfp_t gfp)
{
	size_t len;
	char *buf;

	if (!s)
		return NULL;

	len = strnlen(s, max);
	buf = kmalloc_track_caller(len+1, gfp);
	if (buf) {
		memcpy(buf, s, len);
		buf[len] = '\0';
	}
	return buf;
}
EXPORT_SYMBOL(kstrndup);

/**
 * kmemdup - duplicate region of memory
 *
 * @src: memory region to duplicate
 * @len: memory region length
 * @gfp: GFP mask to use
 *
 * Return: newly allocated copy of @src or %NULL in case of error
 */
void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
	void *p;

	p = kmalloc_track_caller(len, gfp);
	if (p)
		memcpy(p, src, len);
	return p;
}
EXPORT_SYMBOL(kmemdup);

/**
 * kmemdup_nul - Create a NUL-terminated string from unterminated data
 * @s: The data to stringify
 * @len: The size of the data
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 *
 * Return: newly allocated copy of @s with NUL-termination or %NULL in
 * case of error
 */
char *kmemdup_nul(const char *s, size_t len, gfp_t gfp)
{
	char *buf;

	if (!s)
		return NULL;

	buf = kmalloc_track_caller(len + 1, gfp);
	if (buf) {
		memcpy(buf, s, len);
		buf[len] = '\0';
	}
	return buf;
}
EXPORT_SYMBOL(kmemdup_nul);

/**
 * memdup_user - duplicate memory region from user space
 *
 * @src: source address in user space
 * @len: number of bytes to copy
 *
 * Return: an ERR_PTR() on failure.  Result is physically
 * contiguous, to be freed by kfree().
 */
void *memdup_user(const void __user *src, size_t len)
{
	void *p;

	p = kmalloc_track_caller(len, GFP_USER | __GFP_NOWARN);
	if (!p)
		return ERR_PTR(-ENOMEM);

	memcpy(p, src, len);

	return p;
}
EXPORT_SYMBOL(memdup_user);

/**
 * vmemdup_user - duplicate memory region from user space
 *
 * @src: source address in user space
 * @len: number of bytes to copy
 *
 * Return: an ERR_PTR() on failure.  Result may be not
 * physically contiguous.  Use kvfree() to free.
 */
void *vmemdup_user(const void __user *src, size_t len)
{
	void *p;

	p = kmalloc(len, GFP_USER);
	if (!p)
		return ERR_PTR(-ENOMEM);

	memcpy(p, src, len);

	return p;
}
EXPORT_SYMBOL(vmemdup_user);

/**
 * strndup_user - duplicate an existing string from user space
 * @s: The string to duplicate
 * @n: Maximum number of bytes to copy, including the trailing NUL.
 *
 * Return: newly allocated copy of @s or an ERR_PTR() in case of error
 */
char *strndup_user(const char __user *s, long n)
{
	char *p;
	long length;

	length = strnlen(s, n);

	if (!length)
		return ERR_PTR(-EFAULT);

	if (length > n)
		return ERR_PTR(-EINVAL);

	p = memdup_user(s, length);

	if (IS_ERR(p))
		return p;

	p[length - 1] = '\0';

	return p;
}
EXPORT_SYMBOL(strndup_user);

/**
 * memdup_user_nul - duplicate memory region from user space and NUL-terminate
 *
 * @src: source address in user space
 * @len: number of bytes to copy
 *
 * Return: an ERR_PTR() on failure.
 */
void *memdup_user_nul(const void __user *src, size_t len)
{
	char *p;

	/*
	 * Always use GFP_KERNEL, since copy_from_user() can sleep and
	 * cause pagefault, which makes it pointless to use GFP_NOFS
	 * or GFP_ATOMIC.
	 */
	p = kmalloc_track_caller(len + 1, GFP_KERNEL);
	if (!p)
		return ERR_PTR(-ENOMEM);

	memcpy(p, src, len);

	p[len] = '\0';

	return p;
}
EXPORT_SYMBOL(memdup_user_nul);

/**
 * get_cmdline() - copy the cmdline value to a buffer.
 * @task:     the task whose cmdline value to copy.
 * @buffer:   the buffer to copy to.
 * @buflen:   the length of the buffer. Larger cmdline values are truncated
 *            to this length.
 *
 * Return: the size of the cmdline field copied. Note that the copy does
 * not guarantee an ending NULL byte.
 */
//int get_cmdline(struct task_struct *task, char *buffer, int buflen)
//{
//	int res = 0;
//	unsigned int len;
//	struct mm_struct *mm = get_task_mm(task);
//	unsigned long arg_start, arg_end, env_start, env_end;
//	if (!mm)
//		goto out;
//	if (!mm->arg_end)
//		goto out_mm;	/* Shh! No looking before we're done */
//
//	spin_lock(&mm->arg_lock);
//	arg_start = mm->arg_start;
//	arg_end = mm->arg_end;
//	env_start = mm->env_start;
//	env_end = mm->env_end;
//	spin_unlock(&mm->arg_lock);
//
//	len = arg_end - arg_start;
//
//	if (len > buflen)
//		len = buflen;
//
//	res = access_process_vm(task, arg_start, buffer, len, FOLL_FORCE);
//
//	/*
//	 * If the nul at the end of args has been overwritten, then
//	 * assume application is using setproctitle(3).
//	 */
//	if (res > 0 && buffer[res-1] != '\0' && len < buflen) {
//		len = strnlen(buffer, res);
//		if (len < res) {
//			res = len;
//		} else {
//			len = env_end - env_start;
//			if (len > buflen - res)
//				len = buflen - res;
//			res += access_process_vm(task, env_start,
//						 buffer+res, len,
//						 FOLL_FORCE);
//			res = strnlen(buffer, res);
//		}
//	}
//out_mm:
//	mmput(mm);
//out:
//	return res;
//}
