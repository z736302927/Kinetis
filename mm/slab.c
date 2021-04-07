// SPDX-License-Identifier: GPL-2.0
/*
 * linux/mm/slab.c
 * Written by Mark Hemment, 1996/97.
 * (markhe@nextd.demon.co.uk)
 *
 * kmem_cache_destroy() + some cleanup - 1999 Andrea Arcangeli
 *
 * Major cleanup, different bufctl logic, per-cpu arrays
 *	(c) 2000 Manfred Spraul
 *
 * Cleanup, make the head arrays unconditional, preparation for NUMA
 * 	(c) 2002 Manfred Spraul
 *
 * An implementation of the Slab Allocator as described in outline in;
 *	UNIX Internals: The New Frontiers by Uresh Vahalia
 *	Pub: Prentice Hall	ISBN 0-13-101908-2
 * or with a little more detail in;
 *	The Slab Allocator: An Object-Caching Kernel Memory Allocator
 *	Jeff Bonwick (Sun Microsystems).
 *	Presented at: USENIX Summer 1994 Technical Conference
 *
 * The memory is organized in caches, one cache for each object type.
 * (e.g. inode_cache, dentry_cache, buffer_head, vm_area_struct)
 * Each cache consists out of many slabs (they are small (usually one
 * page long) and always contiguous), and each slab contains multiple
 * initialized objects.
 *
 * This means, that your constructor is used only for newly allocated
 * slabs and you must pass objects with the same initializations to
 * kmem_cache_free.
 *
 * Each cache can only support one memory type (GFP_DMA, GFP_HIGHMEM,
 * normal). If you need a special memory type, then must create a new
 * cache for that memory type.
 *
 * In order to reduce fragmentation, the slabs are sorted in 3 groups:
 *   full slabs with 0 free objects
 *   partial slabs
 *   empty slabs with no allocated objects
 *
 * If partial slabs exist, then new allocations come from these slabs,
 * otherwise from empty slabs or new slabs are allocated.
 *
 * kmem_cache_destroy() CAN CRASH if you try to allocate from the cache
 * during kmem_cache_destroy(). The caller must prevent concurrent allocs.
 *
 * Each cache has a short per-cpu head array, most allocs
 * and frees go into that array, and if that array overflows, then 1/2
 * of the entries in the array are given back into the global cache.
 * The head array is strictly LIFO and should improve the cache hit rates.
 * On SMP, it additionally reduces the spinlock operations.
 *
 * The c_cpuarray may not be read with enabled local interrupts -
 * it's changed with a smp_call_function().
 *
 * SMP synchronization:
 *  constructors and destructors are called without any locking.
 *  Several members in struct kmem_cache and struct slab never change, they
 *	are accessed without any locking.
 *  The per-cpu arrays are never accessed from the wrong cpu, no locking,
 *  	and local interrupts are disabled so slab code is preempt-safe.
 *  The non-constant members are protected with a per-cache irq spinlock.
 *
 * Many thanks to Mark Hemment, who wrote another per-cpu slab patch
 * in 2000 - many ideas in the current implementation are derived from
 * his patch.
 *
 * Further notes from the original documentation:
 *
 * 11 April '97.  Started multi-threading - markhe
 *	The global cache-chain is protected by the mutex 'slab_mutex'.
 *	The sem is only needed when accessing/extending the cache-chain, which
 *	can never happen inside an interrupt (kmem_cache_create(),
 *	kmem_cache_shrink() and kmem_cache_reap()).
 *
 *	At present, each engine can be growing a cache.  This should be blocked.
 *
 * 15 March 2005. NUMA slab allocator.
 *	Shai Fultheim <shai@scalex86.org>.
 *	Shobhit Dayal <shobhit@calsoftinc.com>
 *	Alok N Kataria <alokk@calsoftinc.com>
 *	Christoph Lameter <christoph@lameter.com>
 *
 *	Modified the slab allocator to be node aware on NUMA systems.
 *	Each node has its own list of partial, free and full slabs.
 *	All object allocations for a node occur from node specific slab lists.
 */
 
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/string.h>

#undef abs

#include "stdlib.h"
#include "string.h"

//#define MALLOC_RECORD

#ifdef MALLOC_RECORD
static u32 *memory[100];
static u32 pos = 0;
#endif

/**
 * kfree - free previously allocated memory
 * @objp: pointer returned by kmalloc.
 *
 * If @objp is NULL, no operation is performed.
 *
 * Don't free memory not originally allocated by kmalloc()
 * or you will run into trouble.
 */
void kfree(const void *objp)
{
    free((void *)objp);

#ifdef MALLOC_RECORD
    u32 i;

    for (i = 0; i < 100; i++) {
        if (memory[i] == objp)
            memory[i] = NULL;
    }

#endif
}

/**
 * kmalloc - allocate memory
 * @size: how many bytes of memory are required.
 * @flags: the type of memory to allocate.
 *
 * kmalloc is the normal method of allocating memory
 * for objects smaller than page size in the kernel.
 *
 * The allocated object address is aligned to at least ARCH_KMALLOC_MINALIGN
 * bytes. For @size of power of two bytes, the alignment is also guaranteed
 * to be at least to the size.
 *
 * The @flags argument may be one of the GFP flags defined at
 * include/linux/gfp.h and described at
 * :ref:`Documentation/core-api/mm-api.rst <mm-api-gfp-flags>`
 *
 * The recommended usage of the @flags is described at
 * :ref:`Documentation/core-api/memory-allocation.rst <memory_allocation>`
 *
 * Below is a brief outline of the most useful GFP flags
 *
 * %GFP_KERNEL
 *	Allocate normal kernel ram. May sleep.
 *
 * %GFP_NOWAIT
 *	Allocation will not sleep.
 *
 * %GFP_ATOMIC
 *	Allocation will not sleep.  May use emergency pools.
 *
 * %GFP_HIGHUSER
 *	Allocate memory from high memory on behalf of user.
 *
 * Also it is possible to set different flags by OR'ing
 * in one or more of the following additional @flags:
 *
 * %__GFP_HIGH
 *	This allocation has high priority and may use emergency pools.
 *
 * %__GFP_NOFAIL
 *	Indicate that this allocation is in no way allowed to fail
 *	(think twice before using).
 *
 * %__GFP_NORETRY
 *	If memory is not immediately available,
 *	then give up at once.
 *
 * %__GFP_NOWARN
 *	If allocation fails, don't issue any warnings.
 *
 * %__GFP_RETRY_MAYFAIL
 *	Try really hard to succeed the allocation but fail
 *	eventually.
 */
void *kmalloc(unsigned int size, unsigned int flags)
{
    void *ret;

    ret = malloc(size);

#ifdef MALLOC_RECORD
    memory[pos++] = ret;
#endif

    if (ret != NULL) {
        if (flags | __GFP_ZERO)
            memset(ret, 0, size);
    }

    return ret;
}

/**
 * krealloc - reallocate memory. The contents will remain unchanged.
 * @p: object to reallocate memory for.
 * @new_size: how many bytes of memory are required.
 * @flags: the type of memory to allocate.
 *
 * The contents of the object pointed to are preserved up to the
 * lesser of the new and old sizes (__GFP_ZERO flag is effectively ignored).
 * If @p is %NULL, krealloc() behaves exactly like kmalloc().  If @new_size
 * is 0 and @p is not a %NULL pointer, the object pointed to is freed.
 *
 * Return: pointer to the allocated memory or %NULL in case of error
 */
void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
	void *ret;

	if (unlikely(!new_size)) {
		kfree(p);
		return ZERO_SIZE_PTR;
	}

	ret = realloc((void *)p, new_size);

	return ret;
}
EXPORT_SYMBOL(krealloc);

/**
 * kmem_cache_alloc - Allocate an object
 * @cachep: The cache to allocate from.
 * @flags: See kmalloc().
 *
 * Allocate an object from this cache.  The flags are only relevant
 * if the cache has no available objects.
 *
 * Return: pointer to the new object or %NULL in case of error
 */
void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
	void *ret = kmalloc(cachep->object_size, flags);
    
    if (ret)
        cachep->ctor(ret);

	return ret;
}
EXPORT_SYMBOL(kmem_cache_alloc);

/**
 * kmem_cache_free - Deallocate an object
 * @cachep: The cache the allocation was from.
 * @objp: The previously allocated object.
 *
 * Free an object which was previously allocated from this
 * cache.
 */
void kmem_cache_free(struct kmem_cache *cachep, void *objp)
{
	kfree(objp);
}
EXPORT_SYMBOL(kmem_cache_free);

/*
 * Determine the size of a slab object
 */
unsigned int kmem_cache_size(struct kmem_cache *s)
{
	return s->object_size;
}
EXPORT_SYMBOL(kmem_cache_size);

/*
 * Figure out what the alignment of the objects will be given a set of
 * flags, a user specified alignment and the size of the objects.
 */
static unsigned int calculate_alignment(slab_flags_t flags,
		unsigned int align, unsigned int size)
{
	/*
	 * If the user wants hardware cache aligned objects then follow that
	 * suggestion if the object is sufficiently large.
	 *
	 * The hardware cache alignment cannot override the specified
	 * alignment though. If that is greater then use it.
	 */
	if (flags & SLAB_HWCACHE_ALIGN) {
		unsigned int ralign;

		ralign = 32;
		while (size <= ralign / 2)
			ralign /= 2;
		align = max(align, ralign);
	}

	if (align < ARCH_SLAB_MINALIGN)
		align = ARCH_SLAB_MINALIGN;

	return ALIGN(align, sizeof(void *));
}

#ifdef CONFIG_DEBUG_VM
static int kmem_cache_sanity_check(const char *name, unsigned int size)
{
	if (!name || in_interrupt() || size < sizeof(void *) ||
		size > KMALLOC_MAX_SIZE) {
		pr_err("kmem_cache_create(%s) integrity check failed\n", name);
		return -EINVAL;
	}

	WARN_ON(strchr(name, ' '));	/* It confuses parsers */
	return 0;
}
#else
static inline int kmem_cache_sanity_check(const char *name, unsigned int size)
{
	return 0;
}
#endif

static struct kmem_cache *create_cache(const char *name,
		unsigned int object_size, unsigned int align,
		slab_flags_t flags, unsigned int useroffset,
		unsigned int usersize, void (*ctor)(void *),
		struct kmem_cache *root_cache)
{
	struct kmem_cache *s;
	int err;

	if (WARN_ON(useroffset + usersize > object_size))
		useroffset = usersize = 0;

	err = -ENOMEM;
	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (!s)
		return ERR_PTR(err);

	s->name = name;
	s->size = s->object_size = object_size;
	s->align = align;
	s->ctor = ctor;
	s->useroffset = useroffset;
	s->usersize = usersize;
	s->refcount = 1;

	return s;
}

/**
 * kmem_cache_create_usercopy - Create a cache with a region suitable
 * for copying to userspace
 * @name: A string which is used in /proc/slabinfo to identify this cache.
 * @size: The size of objects to be created in this cache.
 * @align: The required alignment for the objects.
 * @flags: SLAB flags
 * @useroffset: Usercopy region offset
 * @usersize: Usercopy region size
 * @ctor: A constructor for the objects.
 *
 * Cannot be called within a interrupt, but can be interrupted.
 * The @ctor is run when new pages are allocated by the cache.
 *
 * The flags are
 *
 * %SLAB_POISON - Poison the slab with a known test pattern (a5a5a5a5)
 * to catch references to uninitialised memory.
 *
 * %SLAB_RED_ZONE - Insert `Red` zones around the allocated memory to check
 * for buffer overruns.
 *
 * %SLAB_HWCACHE_ALIGN - Align the objects in this cache to a hardware
 * cacheline.  This can be beneficial if you're counting cycles as closely
 * as davem.
 *
 * Return: a pointer to the cache on success, NULL on failure.
 */
struct kmem_cache *
kmem_cache_create_usercopy(const char *name,
		  unsigned int size, unsigned int align,
		  slab_flags_t flags,
		  unsigned int useroffset, unsigned int usersize,
		  void (*ctor)(void *))
{
	struct kmem_cache *s = NULL;
	const char *cache_name;
	int err;

	err = kmem_cache_sanity_check(name, size);
	if (err) {
		goto out_unlock;
	}

	/* Fail closed on bad usersize of useroffset values. */
	if (WARN_ON(!usersize && useroffset) ||
	    WARN_ON(size < usersize || size - usersize < useroffset))
		usersize = useroffset = 0;

	cache_name = kstrdup_const(name, GFP_KERNEL);
	if (!cache_name) {
		err = -ENOMEM;
		goto out_unlock;
	}

	s = create_cache(cache_name, size,
			 calculate_alignment(flags, align, size),
			 flags, useroffset, usersize, ctor, NULL);
	if (IS_ERR(s)) {
		err = PTR_ERR(s);
		kfree(cache_name);
	}

out_unlock:

	if (err) {
		if (flags & SLAB_PANIC)
			pr_err("kmem_cache_create: Failed to create slab '%s'. Error %d\n",
				name, err);
		else {
			pr_warn("kmem_cache_create(%s) failed with error %d\n",
				name, err);
		}
		return NULL;
	}
	return s;
}
EXPORT_SYMBOL(kmem_cache_create_usercopy);

/**
 * kmem_cache_create - Create a cache.
 * @name: A string which is used in /proc/slabinfo to identify this cache.
 * @size: The size of objects to be created in this cache.
 * @align: The required alignment for the objects.
 * @flags: SLAB flags
 * @ctor: A constructor for the objects.
 *
 * Cannot be called within a interrupt, but can be interrupted.
 * The @ctor is run when new pages are allocated by the cache.
 *
 * The flags are
 *
 * %SLAB_POISON - Poison the slab with a known test pattern (a5a5a5a5)
 * to catch references to uninitialised memory.
 *
 * %SLAB_RED_ZONE - Insert `Red` zones around the allocated memory to check
 * for buffer overruns.
 *
 * %SLAB_HWCACHE_ALIGN - Align the objects in this cache to a hardware
 * cacheline.  This can be beneficial if you're counting cycles as closely
 * as davem.
 *
 * Return: a pointer to the cache on success, NULL on failure.
 */
struct kmem_cache *
kmem_cache_create(const char *name, unsigned int size, unsigned int align,
		slab_flags_t flags, void (*ctor)(void *))
{
	return kmem_cache_create_usercopy(name, size, align, flags, 0, 0,
					  ctor);
}
EXPORT_SYMBOL(kmem_cache_create);

void kmem_cache_destroy(struct kmem_cache *s)
{
	if (unlikely(!s))
		return;
    
	kfree(s->name);
	kfree(s);
}
EXPORT_SYMBOL(kmem_cache_destroy);
