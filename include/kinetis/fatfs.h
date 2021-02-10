#ifndef __K_FATFS_H
#define __K_FATFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "ff.h"

void FatFs_Init(void);
void Printf_FatFs_Err(FRESULT fresult);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_FATFS_H */
