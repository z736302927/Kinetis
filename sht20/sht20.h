#ifndef __SHT20_H
#define __SHT20_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"


#define SHT20_IIC_ADDR              0x40
#define SHT20_MEASURE_TEMP_CMD      0xE3
#define SHT20_MEASURE_RH_CMD        0xE5
#define SHT20_SOFT_RESET_CMD        0xFE

void SHT20_Init(void);
void SHT20_Read_TempAndRH(float *Temperature, float *Humidit);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
} 
#endif

#endif /* __SHT20_H */
