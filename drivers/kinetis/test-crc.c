#include <linux/types.h>

#include "kinetis/core_common.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_CRC
#include "kinetis/test-kinetis.h"

#include <linux/crc4.h>
#include <linux/crc7.h>
#include <linux/crc8.h>
#include <linux/crc16.h>
#include <linux/crc32.h>
#include <linux/string.h>

int t_crc(int argc, char **argv)
{
//    u8 bits = 16;
//    u8 buffer[10];
//    u32 seed = 0xFFFF;
//    u32 i;

//    if (argc > 1)
//        bits = simple_strtoul(argv[1], &argv[1], 10);

//    for (i = 0; i < sizeof(buffer); ++i)
//        buffer[i] = i;

//    switch (bits) {
//        case 4:
//            crc4(seed, 10, 4);
//            break;

//        case 7:
//            seed = 0xFF;
//            crc7_be(seed, buffer, sizeof(buffer));
//            break;

//        case 8:
//            seed = 0xFF;
//            crc8(NULL, buffer, sizeof(buffer), seed);
//            break;

//        case 16:
//            seed = 0xFFFF;
//            crc16(seed, buffer, sizeof(buffer));
//            break;

//        case 32:
//            seed = 0xFFFFFFFF;
//            crc32(seed, buffer, sizeof(buffer));
//            break;

//        case 64:
//            seed = 0xFFFFFFFF;
//            crc64_be(seed, buffer, sizeof(buffer));
//            break;

//    }

    return PASS;
}

#endif

