#include "kinetis/random-gene.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

//#include "rng.h"

//#define USING_HARDWARE_RNG
#define USING_C_LIBRARY

static inline u32 random_get_int(void)
{
#ifdef USING_HARDWARE_RNG
#if MCU_PLATFORM_STM32
    return HAL_RNG_GetRandomNumber(&hrng);
#else
    return 0;
#endif
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
    return random_get_int();
}

u64 random_get64bit(void)
{
    u64 tmp;

    tmp = random_get_int();
    tmp |= ((u64)random_get_int()) << 32;

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
                data_32bits[i] = random_get_int();

            break;

        case RNG_64BITS:
            data_64bits = pdata;

            for (i = 0; i < length; ++i) {
                data_64bits[i] = random_get_int();
                data_64bits[i] |= ((u64)random_get_int()) << 32;
            }

            break;
    }
}

// u32 get_random_range(u32 min, u32 max)
// {
//     return random_get_int() % (max - min) + min;
// }

#ifdef DESIGN_VERIFICATION_DELAY
#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/printk.h>

int t_random_number(int argc, char **argv)
{
    pr_debug("8 bits random number is %u\n", random_get8bit());

    pr_debug("16 bits random number is %u\n", random_get16bit());

    pr_debug("32 bits random number is %u\n", random_get32bit());

    pr_debug("64 bits random number is %llu\n", random_get64bit());

    return PASS;
}

int t_random_array(int argc, char **argv)
{
    u32 *pdata;
    u32 length = 10;
    u8 bits = RNG_32BITS;
    u16 i;

    if (argc > 1)
        length = simple_strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        bits = simple_strtoul(argv[2], &argv[2], 10);


    pdata = kmalloc(length, __GFP_ZERO);

    if (pdata == NULL) {
        pr_debug("Random array malloc failed !\n");
        return FAIL;
    }

    random_get_array(pdata, length, bits);
    pr_debug("random number is the following\n");

    print_hex_dump(KERN_DEBUG, "random: ", DUMP_PREFIX_OFFSET,
        16, 1,
        pdata, length, false);

    return PASS;
}

#endif

