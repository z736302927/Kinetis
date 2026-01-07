#ifndef __KRANDOM_GENE_H
#define __KRANDOM_GENE_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"

#define RNG_8BITS                       8
#define RNG_32BITS                      16
#define RNG_16BITS                      32
#define RNG_64BITS                      64

u8 random_get8bit(void);
u16 random_get16bit(void);
u32 random_get32bit(void);
void random_get_array(void *pdata, u32 length, u8 bits);
u32 get_random_range(u32 min, u32 max);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __KRANDOM_GENE_H */
