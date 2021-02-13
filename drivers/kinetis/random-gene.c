#include "kinetis/random-gene.h"

#include "stdlib.h"
#include "string.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "rng.h"

#define USING_HARDWARE_RNG
#define USING_C_LIBRARY

static inline u32 random_get_int(void)
{
#ifdef USING_HARDWARE_RNG
    return HAL_RNG_GetRandomNumber(&hrng);
#else
    return 0;
#endif
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

u8 random_get8bit(void)
{
    return random_get_int() % 0xFF;
}

u16 random_get16bit(void)
{
    return random_get_int() % 0xFFFF;
}

u32 random_get32bit(void)
{
    return random_get_int() % 0xFFFFFFFF;
}

u64 random_get64bit(void)
{
    u64 tmp;

    tmp = random_get_int() % 0xFFFFFFFF;
    tmp |= ((u64)random_get_int() % 0xFFFFFFFF) << 32;

    return tmp;
}

void random_get_array(void *pdata, u32 length, u8 bits)
{
    u32 i;
    u8 *data_8bits, *data_16bits, *data_32bits, *data_64bits;

    switch (bits) {
        case RNG_8BITS:
            data_8bits = pdata;

            for (i = 0; i < length; ++i)
                data_8bits[i] = random_get_int() % 0xFF;

            break;

        case RNG_16BITS:
            data_16bits = pdata;

            for (i = 0; i < length; ++i)
                data_16bits[i] = random_get_int() % 0xFFFF;

            break;

        case RNG_32BITS:
            data_32bits = pdata;

            for (i = 0; i < length; ++i)
                data_32bits[i] = random_get_int() % 0xFFFFFFFF;

            break;

        case RNG_64BITS:
            data_64bits = pdata;

            for (i = 0; i < length; ++i) {
                data_64bits[i] = random_get_int() % 0xFFFFFFFF;
                data_64bits[i] |= ((u64)random_get_int() % 0xFFFFFFFF) << 32;
            }

            break;
    }
}

#ifdef DESIGN_VERIFICATION_DELAY
#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/printk.h>

#include "stdio.h"

int t_random_number(int argc, char **argv)
{
    printk(KERN_DEBUG "8 bits random number is %u", random_get8bit());

    printk(KERN_DEBUG "16 bits random number is %u", random_get16bit());

    printk(KERN_DEBUG "32 bits random number is %u", random_get32bit());

    printk(KERN_DEBUG "64 bits random number is %llu", random_get64bit());

    return PASS;
}

int t_random_array(int argc, char **argv)
{
    u32 *pdata;
    u32 length = 10;
    u8 bits = RNG_32BITS;
    u16 i;

    if (argc > 1)
        length = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        bits = strtoul(argv[2], &argv[2], 10);


    pdata = kmalloc(length, __GFP_ZERO);

    if (pdata == NULL) {
        printk(KERN_DEBUG "Random array malloc failed !");
        return FAIL;
    }

    random_get_array(pdata, length, bits);
    printk(KERN_DEBUG "random number is the following");

    for (i = 0; i < length; ++i) {
        printk(KERN_DEBUG "%u", pdata[i]);

        if ((i % 4) == 0)
            printf("\n");
    }

    return PASS;
}

#endif

