#ifndef __K_KEY_H
#define __K_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

typedef void (*BtnCallback)(void *);

typedef enum {
    PRESS_DOWN = 0,
    PRESS_UP,
    PRESS_REPEAT,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_RRESS_START,
    LONG_PRESS_HOLD,
    PRESSEVENT_NBR,
    NONE_PRESS
} PressEvent;

typedef struct Button_TypeDef {
    u16 Ticks;
    u8  Repeat      : 4;
    u8  Event       : 4;
    u8  State       : 3;
    u8  DebounceCnt : 3;
    u8  ActiveLevel : 1;
    u8  ButtonLevel : 1;
    u8 (*HALButtonLevel)(void);
    BtnCallback  CB[PRESSEVENT_NBR];
    struct Button_TypeDef *Next;
} Button_TypeDef;

void Button_Init(struct Button_TypeDef *handle, u8(*pin_level)(void), u8 ActiveLevel);
void Button_Attach(struct Button_TypeDef *handle, PressEvent Event, BtnCallback CB);
PressEvent Get_Button_Event(struct Button_TypeDef *handle);
int  Button_Start(struct Button_TypeDef *handle);
void Button_Stop(struct Button_TypeDef *handle);
void Button_Ticks(void);
int Multi_Button_Test(void);

void ButtonTask_Init(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_KEY_H */
