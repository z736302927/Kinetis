#ifndef __SHT20_H
#define __SHT20_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"


void SHT20_Init(void);
void SHT20_Read_TempAndRH(float *Temperature, float *Humidit);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __SHT20_H */
