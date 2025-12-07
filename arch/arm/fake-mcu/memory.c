
#include <linux/slab.h>

#include <fake-mcu/types.h>
#include <fake-mcu/memory.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//int kmalloc_nr_allocated = 0;
//int kmalloc_verbose = 0;
//
//void *__kmalloc(size_t size, gfp_t gfp)
//{
//	void *ret;
//
//	if (!(gfp & __GFP_DIRECT_RECLAIM))
//		return NULL;
//
//	ret = malloc(size);
//	kmalloc_nr_allocated++;
//	if (kmalloc_verbose)
//		printf("Allocating %p from malloc\n", ret);
//	if (gfp & __GFP_ZERO)
//		memset(ret, 0, size);
//	return ret;
//}
//
//void kfree(const void *p)
//{
//	if (!p)
//		return;
//	kmalloc_nr_allocated++;
//	if (kmalloc_verbose)
//		printf("Freeing %p to malloc\n", p);
//	free(p);
//}