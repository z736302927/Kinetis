#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

//According to your need to modify the constants.
#define TICKS_INTERVAL    5 //ms
#define DEBOUNCE_TICKS    3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

typedef void (*BtnCallback)(void*);

typedef enum {
  PRESS_DOWN = 0,
  PRESS_UP,
  PRESS_REPEAT,
  SINGLE_CLICK,
  DOUBLE_CLICK,
  LONG_RRESS_START,
  LONG_PRESS_HOLD,
  number_of_event,
  NONE_PRESS
}PressEvent;

typedef struct Button_TypeDef {
  uint16_t Ticks;
  uint8_t  Repeat      : 4;
  uint8_t  Event       : 4;
  uint8_t  State       : 3;
  uint8_t  DebounceCnt : 3; 
  uint8_t  ActiveLevel : 1;
  uint8_t  ButtonLevel : 1;
  uint8_t  (*HALButtonLevel)(void);
  BtnCallback  CB[number_of_event];
  struct Button_TypeDef* Next;
}Button_TypeDef;

void Button_Init(struct Button_TypeDef* handle, uint8_t(*pin_level)(void), uint8_t ActiveLevel);
void Button_Attach(struct Button_TypeDef* handle, PressEvent Event, BtnCallback CB);
PressEvent Get_Button_Event(struct Button_TypeDef* handle);
int  Button_Start(struct Button_TypeDef* handle);
void Button_Stop(struct Button_TypeDef* handle);
void Button_Ticks(void);
int Multi_Button_Test(void);

void ButtonTask_Init(void);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
} 
#endif

#endif /* __BSP_KEY_H */
