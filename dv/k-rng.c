#include "k-rng.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "stdlib.h"
#include "string.h"
#include "rng.h"
#include "idebug.h"

#define USING_HARDWARE_RNG
#define USING_C_LIBRARY

static inline uint32_t Random_GetNumber(void)
{
#ifdef USING_HARDWARE_RNG
    return HAL_RNG_GetRandomNumber(&hrng);
#else
    return 0;
#endif
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

uint8_t Random_Get8bit(void)
{
    return Random_GetNumber() % 0xFF;
}

uint16_t Random_Get16bit(void)
{
    return Random_GetNumber() % 0xFFFF;
}

uint32_t Random_Get32bit(void)
{
    return Random_GetNumber() % 0xFFFFFFFF;
}

uint64_t Random_Get64bit(void)
{
    uint64_t Data;

    Data = Random_GetNumber() % 0xFFFFFFFF;
    Data |= ((uint64_t)Random_GetNumber() % 0xFFFFFFFF) << 32;

    return Data;
}

void Random_GetArray(void *pData, uint32_t Length, uint8_t Bits)
{
    uint32_t i;
    uint8_t *Data_8bits;
    uint8_t *Data_16bits;
    uint8_t *Data_32bits;
    uint8_t *Data_64bits;

    switch(Bits)
    {
        case RNG_8BITS:
            Data_8bits = pData;

            for(i = 0; i < Length; ++i)
                Data_8bits[i] = Random_GetNumber() % 0xFF;

            break;

        case RNG_16BITS:
            Data_16bits = pData;

            for(i = 0; i < Length; ++i)
                Data_16bits[i] = Random_GetNumber() % 0xFFFF;

            break;

        case RNG_32BITS:
            Data_32bits = pData;

            for(i = 0; i < Length; ++i)
                Data_32bits[i] = Random_GetNumber() % 0xFFFFFFFF;

            break;

        case RNG_64BITS:
            Data_64bits = pData;

            for(i = 0; i < Length; ++i)
            {
                Data_64bits[i] = Random_GetNumber() % 0xFFFFFFFF;
                Data_64bits[i] |= ((uint64_t)Random_GetNumber() % 0xFFFFFFFF) << 32;
            }

            break;
    }
}

#ifdef DESIGN_VERIFICATION_DELAY
#include "k-test.h"
#include "k-memory.h"
#include "linux/gfp.h"
#include "stdio.h"

int t_Random_Number(int argc, char **argv)
{
    kinetis_debug_trace(KERN_DEBUG, "8 bits random number is %u", Random_Get8bit());

    kinetis_debug_trace(KERN_DEBUG, "16 bits random number is %u", Random_Get16bit());

    kinetis_debug_trace(KERN_DEBUG, "32 bits random number is %u", Random_Get32bit());

    kinetis_debug_trace(KERN_DEBUG, "64 bits random number is %llu", Random_Get64bit());

    return PASS;
}

int t_Random_Array(int argc, char **argv)
{
    uint32_t *pData;
    uint32_t Length = 10;
    uint8_t Bits = RNG_32BITS;
    uint16_t i;

    if(argc > 1)
        Length = strtoul(argv[1], &argv[1], 10);

    if(argc > 2)
        Bits = strtoul(argv[2], &argv[2], 10);


    pData = kmalloc(Length, __GFP_ZERO);

    if(pData == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "Random array malloc failed !");
        return FAIL;
    }

    Random_GetArray(pData, Length, Bits);
    kinetis_debug_trace(KERN_DEBUG, "random number is the following");

    for(i = 0; i < Length; ++i)
    {
        kinetis_debug_trace(KERN_DEBUG, "%u", pData[i]);

        if((i % 4) == 0)
            printf("\n");
    }

    return PASS;
}

#endif

