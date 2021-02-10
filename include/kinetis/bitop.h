#ifndef __K_BITOP_H
#define __K_BITOP_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

/** @addtogroup Exported_macro
  * @{
  */
#define K_SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define K_CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define K_READ_BIT(REG, BIT)    ((REG) & (BIT))

#define K_CLEAR_REG(REG)        ((REG) = (0x0))

#define K_WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define K_READ_REG(REG)         ((REG))

#define K_MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define K_POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))

static inline u32 kreadl(const volatile void *addr)
{
    return *((const volatile u32 *)addr);
}

static inline void kwritel(u32 b, volatile void *addr)
{
    (*(volatile u32 *)addr) = b;
}

#define K_WRITE_MASK(ADDR,DATA,MASK)  kwritel(((kreadl(ADDR) & ~(MASK)) | ((DATA) & (MASK))), ADDR)

#define K_CLEAR_MASK(ADDR,MASK)       kwritel(kreadl(ADDR) & ~(MASK), ADDR)

#define K_SET_MASK(ADDR,MASK)         kwritel(kreadl(ADDR) | (MASK), ADDR)

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_BITOP_H */
