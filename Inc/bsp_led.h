#ifndef __BSP_LED_H
#define __BSP_LED_H

#ifdef __cplusplus
 extern "C" {
#endif
                                              
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

typedef enum 
{
  LED1 = 0,
  LED2 = 1
}Led_TypeDef;

#define LEDn                                    2

void BSP_LED_Init(Led_TypeDef Led);
void BSP_LED_On(Led_TypeDef Led);
void BSP_LED_Off(Led_TypeDef Led);
void BSP_LED_Toggle(Led_TypeDef Led);



#ifdef __cplusplus
}
#endif

#endif /* __BSP_LED_H */
