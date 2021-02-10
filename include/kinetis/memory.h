#ifndef __KMEMORY_H
#define __KMEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "linux/gfp.h"

void kfree(void *);
void *kmalloc(unsigned int size, unsigned int flags);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __KMEMORY_H */
