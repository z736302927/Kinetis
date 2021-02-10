#include "kinetis/memory.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "stdlib.h"
#include "string.h"

//#define MALLOC_RECORD

#ifdef MALLOC_RECORD
u32 memory[100];
u32 pointer;
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
void kfree(void *objp)
{
    free(objp);

#ifdef MALLOC_RECORD
    u32 i;

    for (i = 0; i < 100; i++) {
        if (memory[i] == (u32)objp)
            memory[i] = 0;
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
 * :ref:`Documentation/core-api/memory-allocation.rst <memory-allocation>`
 *
 * Below is a brief outline of the most useful GFP flags
 *
 * %GFP_KERNEL
 *  Allocate normal kernel ram. May sleep.
 *
 * %GFP_NOWAIT
 *  Allocation will not sleep.
 *
 * %GFP_ATOMIC
 *  Allocation will not sleep.  May use emergency pools.
 *
 * %GFP_HIGHUSER
 *  Allocate memory from high memory on behalf of user.
 *
 * Also it is possible to set different flags by OR'ing
 * in one or more of the following additional @flags:
 *
 * %__GFP_HIGH
 *  This allocation has high priority and may use emergency pools.
 *
 * %__GFP_NOFAIL
 *  Indicate that this allocation is in no way allowed to fail
 *  (think twice before using).
 *
 * %__GFP_NORETRY
 *  If memory is not immediately available,
 *  then give up at once.
 *
 * %__GFP_NOWARN
 *  If allocation fails, don't issue any warnings.
 *
 * %__GFP_RETRY_MAYFAIL
 *  Try really hard to succeed the allocation but fail
 *  eventually.
 */
void *kmalloc(unsigned int size, unsigned int flags)
{
    void *ret;

    ret = malloc(size);

#ifdef MALLOC_RECORD
    memory[pointer++] = (u32)ret;
#endif

    if (ret != NULL) {
        if (flags | __GFP_ZERO)
            memset(ret, 0, size);
    }

    return ret;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_MEMORY
#include "kinetis/test.h"
#include "kinetis/rng.h"

#define DEBUG
#include "kinetis/idebug.h"

int t_memory_Test(int argc, char **argv)
{
    void *ret;
    u16 size;
    u16 i, times = 10;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    for (i = 0; i < times; i++) {
        size = Random_Get8bit();
        kinetis_print_trace(KERN_DEBUG, "malloc size@%u", size);
        ret = kmalloc(size, GFP_KERNEL);

        if (ret == NULL)
            return FAIL;

        kfree(ret);
        kinetis_print_trace(KERN_DEBUG, "malloc addr@%p", ret);
    }

    return PASS;
}

#endif
