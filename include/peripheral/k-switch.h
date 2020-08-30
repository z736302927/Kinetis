#ifndef __K_SWITCH_H
#define __K_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"
#include "linux/list.h"

typedef void (*SwitchCallback)(void *);

typedef enum
{
    SWITCH_DOWN = 0,
    SWITCH_UP,
    SWITCHEVENT_NBR,
    NONE_SWITCH
} SwitchEvent;

typedef struct Switch_TypeDef
{
    uint8_t  Event       : 4;
    uint8_t  State       : 3;
    uint8_t  DebounceCnt : 3;
    uint8_t  ActiveLevel : 1;
    uint8_t  SwitchLevel : 1;
    uint8_t (*HALSwitchLevel)(void);
    SwitchCallback  CB[SWITCHEVENT_NBR];
    struct list_head Entry;
} Switch_TypeDef;

void Switch_Init(struct Switch_TypeDef *handle, uint8_t(*pin_level)(void), uint8_t ActiveLevel);
void Switch_Attach(struct Switch_TypeDef *handle, SwitchEvent Event, SwitchCallback CB);
SwitchEvent Get_Switch_Event(struct Switch_TypeDef *handle);
void Switch_Start(struct Switch_TypeDef *handle);
void Switch_Stop(struct Switch_TypeDef *handle);
void Switch_Ticks(void);
int Multi_Switch_Test(void);

void SwitchTask_Init(void);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
}
#endif

#endif /* __K_SWITCH_H */
