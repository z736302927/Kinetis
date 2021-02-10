#ifndef __K_LED_H
#define __K_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

typedef enum {
    LED1 = 0,
    LED2,
    LED3,
    LED4
} LEDn_Type;

typedef struct LED_TypeDef {
    u8 UniqueID;
    char *Color;
    struct LED_TypeDef *Next;
} LED_TypeDef;

#define LEDn              4

void K_LED_Init(LEDn_Type LED);
void K_LED_On(LEDn_Type LED);
void K_LED_Off(LEDn_Type LED);
void K_LED_Toggle(LEDn_Type LED);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_LED_H */
