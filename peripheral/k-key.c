#include "peripheral/k-key.h"
#include "algorithm/k-slist.h"

static struct Button_TypeDef *head_handle = NULL;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Button_Test function, register the corresponding button and Button_Callback callback function.
  * @step 3:  Call function Button_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Button_Test once in function main.
  */

#include "string.h"
#include "task/k-timtask.h"
#include "protocol/hydrology.h"
#include "idebug.h"

/* According to your need to modify the constants. */
#define TICKS_INTERVAL    5 //ms
#define DEBOUNCE_TICKS    3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

struct TimTask_TypeDef ButtonTask;

void ButtonTask_Callback(void)
{
    Button_Ticks();
}

void ButtonTask_Init(void)
{
    TimTask_Init(&ButtonTask, ButtonTask_Callback, 5, 5);
    TimTask_Start(&ButtonTask);
}

void ButtonTask_Deinit(void)
{
    TimTask_Stop(&ButtonTask);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define EVENT_CB(ev) if(handle->CB[ev]) handle->CB[ev]((Button_TypeDef*)handle)

/**
  * @brief  Initializes the button struct handle.
  * @param  handle: the button handle strcut.
  * @param  pin_level: read the pin of the connet button level.
  * @param  ActiveLevel: pin pressed level.
  * @retval None
  */
void Button_Init(struct Button_TypeDef *handle, uint8_t(*pin_level)(void), uint8_t ActiveLevel)
{
    memset(handle, 0, sizeof(struct Button_TypeDef));
    handle->Event = (uint8_t)NONE_PRESS;
    handle->HALButtonLevel = pin_level;
    handle->ButtonLevel = handle->HALButtonLevel();
    handle->ActiveLevel = ActiveLevel;
}

/**
  * @brief  Deinitializes the button struct handle.
  * @param  handle: the button handle strcut.
  * @retval None
  */
void Button_Deinit(struct Button_TypeDef *handle)
{
    memset(handle, 0, sizeof(struct Button_TypeDef));
}

/**
  * @brief  Attach the button event callback function.
  * @param  handle: the button handle strcut.
  * @param  event: trigger event type.
  * @param  CB: callback function.
  * @retval None
  */
void Button_Attach(struct Button_TypeDef *handle, PressEvent Event, BtnCallback CB)
{
    handle->CB[Event] = CB;
}

/**
  * @brief  Detach the button event callback function.
  * @param  handle: the button handle strcut.
  * @param  event: trigger event type.
  * @retval None
  */
void Button_Detach(struct Button_TypeDef *handle, PressEvent Event)
{
    handle->CB[Event] = NULL;
}

/**
  * @brief  Inquire the button event happen.
  * @param  handle: the button handle strcut.
  * @retval button event.
  */
PressEvent Get_Button_Event(struct Button_TypeDef *handle)
{
    return (PressEvent)(handle->Event);
}

/**
  * @brief  Start the button work, add the handle into work list.
  * @param  handle: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int Button_Start(struct Button_TypeDef *handle)
{
    struct Button_TypeDef *target = head_handle;

    while(target)
    {
        if(target == handle)
        {
            return -1;  //already exist.
        }

        target = target->Next;
    }

    handle->Next = head_handle;
    head_handle = handle;

    return 0;
}

/**
  * @brief  Stop the button work, remove the handle off work list.
  * @param  handle: target handle strcut.
  * @retval None
  */
void Button_Stop(struct Button_TypeDef *handle)
{
    struct Button_TypeDef **curr;

    for(curr = &head_handle; *curr;)
    {
        struct Button_TypeDef *entry = *curr;

        if(entry == handle)
            *curr = entry->Next;
        else
            curr = &entry->Next;
    }
}

/**
  * @brief  button driver core function, driver state machine.
  * @param  handle: the button handle strcut.
  * @retval None
  */
static void Button_Handler(struct Button_TypeDef *handle)
{
    uint8_t read_gpio_level = handle->HALButtonLevel();

    //Ticks counter working..
    if((handle->State) > 0)
        handle->Ticks++;

    /*------------button debounce handle---------------*/
    if(read_gpio_level != handle->ButtonLevel)
    {
        //not equal to prev one
        //continue read 3 times same new level change
        if(++(handle->DebounceCnt) >= DEBOUNCE_TICKS)
        {
            handle->ButtonLevel = read_gpio_level;
            handle->DebounceCnt = 0;
        }
    }
    else
    {
        // leved not change ,counter reset.
        handle->DebounceCnt = 0;
    }

    /*-----------------State machine-------------------*/
    switch(handle->State)
    {
        case 0:
            if(handle->ButtonLevel == handle->ActiveLevel)
            {
                handle->Event = (uint8_t)PRESS_DOWN;
                EVENT_CB(PRESS_DOWN);
                handle->Ticks  = 0;
                handle->Repeat = 1;
                handle->State  = 1;
            }
            else
                handle->Event = (uint8_t)NONE_PRESS;

            break;

        case 1:
            if(handle->ButtonLevel != handle->ActiveLevel)
            {
                handle->Event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);
                handle->Ticks = 0;
                handle->State = 2;

            }
            else if(handle->Ticks > LONG_TICKS)
            {
                handle->Event = (uint8_t)LONG_RRESS_START;
                EVENT_CB(LONG_RRESS_START);
                handle->State = 5;
            }

            break;

        case 2:
            if(handle->ButtonLevel == handle->ActiveLevel)
            {
                handle->Event = (uint8_t)PRESS_DOWN;
                EVENT_CB(PRESS_DOWN);
                handle->Repeat++;

                handle->Event = (uint8_t)PRESS_REPEAT;
                EVENT_CB(PRESS_REPEAT);
                handle->Ticks = 0;
                handle->State = 3;

            }
            else if(handle->Ticks > SHORT_TICKS)
            {
                if(handle->Repeat == 1)
                {
                    handle->Event = (uint8_t)SINGLE_CLICK;
                    EVENT_CB(SINGLE_CLICK);
                }
                else if(handle->Repeat == 2)
                {
                    handle->Event = (uint8_t)DOUBLE_CLICK;
                    EVENT_CB(DOUBLE_CLICK);
                }

                handle->State = 0;
            }

            break;

        case 3:
            if(handle->ButtonLevel != handle->ActiveLevel)
            {
                handle->Event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);

                if(handle->Ticks < SHORT_TICKS)
                {
                    handle->Ticks = 0;
                    handle->State = 2;
                }
                else
                    handle->State = 0;
            }

            break;

        case 5:
            if(handle->ButtonLevel == handle->ActiveLevel)
            {
                handle->Event = (uint8_t)LONG_PRESS_HOLD;
                EVENT_CB(LONG_PRESS_HOLD);
            }
            else
            {
                handle->Event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);

                handle->State = 0;
            }

            break;
    }
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void Button_Ticks(void)
{
    struct Button_TypeDef *target;

    for(target = head_handle; target != NULL; target = target->Next)
        Button_Handler(target);
}

#ifdef DESIGN_VERIFICATION_BUTTON
#include "k-test.h"

static struct Button_TypeDef Button_Test_Inst;

static uint8_t Button_Read_Pin(void)
{
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);
}

static void Button_Callback(void *btn)
{
    uint32_t btn_event_val;

    btn_event_val = Get_Button_Event((struct Button_TypeDef *)btn);

    switch(btn_event_val)
    {
        case PRESS_DOWN:
            HydrologyProcessSend(Test);
            kinetis_debug_trace(KERN_DEBUG, "Button press down");
            break;

        case PRESS_UP:
            kinetis_debug_trace(KERN_DEBUG, "Button press up");
            break;

        case PRESS_REPEAT:
            kinetis_debug_trace(KERN_DEBUG, "Button press repeat");
            break;

        case SINGLE_CLICK:
            kinetis_debug_trace(KERN_DEBUG, "Button single click");
            break;

        case DOUBLE_CLICK:
            kinetis_debug_trace(KERN_DEBUG, "Button double click");
            break;

        case LONG_RRESS_START:
            kinetis_debug_trace(KERN_DEBUG, "Button long press start");
            break;

        case LONG_PRESS_HOLD:
            kinetis_debug_trace(KERN_DEBUG, "Button long press hold");
            break;
    }
}

int t_Button_Attach(int argc, char **argv)
{
    ButtonTask_Init();

    /* low level drive */
    Button_Init(&Button_Test_Inst, Button_Read_Pin, 0);
    Button_Attach(&Button_Test_Inst, PRESS_DOWN, Button_Callback);
    Button_Attach(&Button_Test_Inst, PRESS_UP, Button_Callback);
    Button_Attach(&Button_Test_Inst, PRESS_REPEAT, Button_Callback);
    Button_Attach(&Button_Test_Inst, SINGLE_CLICK, Button_Callback);
    Button_Attach(&Button_Test_Inst, DOUBLE_CLICK, Button_Callback);
    Button_Attach(&Button_Test_Inst, LONG_RRESS_START, Button_Callback);
    Button_Attach(&Button_Test_Inst, LONG_PRESS_HOLD, Button_Callback);
    Button_Start(&Button_Test_Inst);

    kinetis_debug_trace(KERN_DEBUG, "Button test is running, please push the button.");

    return PASS;
}

int t_Button_Detach(int argc, char **argv)
{
    ButtonTask_Deinit();

    /* low level drive */
    Button_Deinit(&Button_Test_Inst, Button_Read_Pin, 0);
    Button_Detach(&Button_Test_Inst, PRESS_DOWN);
    Button_Detach(&Button_Test_Inst, PRESS_UP);
    Button_Detach(&Button_Test_Inst, PRESS_REPEAT);
    Button_Detach(&Button_Test_Inst, SINGLE_CLICK);
    Button_Detach(&Button_Test_Inst, DOUBLE_CLICK);
    Button_Detach(&Button_Test_Inst, LONG_RRESS_START);
    Button_Detach(&Button_Test_Inst, LONG_PRESS_HOLD);
    Button_Stop(&Button_Test_Inst);

    kinetis_debug_trace(KERN_DEBUG, "Button test is over");

    return PASS;
}

#endif

