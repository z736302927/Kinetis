#include "peripheral/k-switch.h"

static LIST_HEAD(head_handle);

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Switch_Test function, register the corresponding Switch and Switch_Callback callback function.
  * @step 3:  Call function Switch_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Switch_Test once in function main.
  */

#include "string.h"

#define DEBUG
#include "idebug.h"

#define Switch_printf    p_dbg

//According to your need to modify the constants.
#define TICKS_INTERVAL    20 //ms
#define DEBOUNCE_TICKS    3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

struct TimTask_TypeDef SwitchTask;

void SwitchTask_Callback(void)
{
    Switch_Ticks();
}

void SwitchTask_Init(void)
{
    TimTask_Init(&SwitchTask, SwitchTask_Callback, 20, 20); //20ms loop
    TimTask_Start(&SwitchTask);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define EVENT_CB(ev) if(handle->CB[ev]) handle->CB[ev]((Switch_TypeDef*)handle)

/**
  * @brief  Initializes the Switch struct handle.
  * @param  handle: the Switch handle strcut.
  * @param  pin_level: read the pin of the connet Switch level.
  * @param  ActiveLevel: pin pressed level.
  * @retval None
  */
void Switch_Init(struct Switch_TypeDef *handle, uint8_t(*pin_level)(void), uint8_t ActiveLevel)
{
    memset(handle, 0, sizeof(struct Switch_TypeDef));
    handle->Event = (uint8_t)NONE_SWITCH;
    handle->HALSwitchLevel = pin_level;
    handle->SwitchLevel = handle->HALSwitchLevel();
    handle->ActiveLevel = ActiveLevel;
}

/**
  * @brief  Deinitializes the Switch struct handle.
  * @param  handle: the Switch handle strcut.
  * @retval None
  */
void Switch_Deinit(struct Switch_TypeDef *handle)
{
    memset(handle, 0, sizeof(struct Switch_TypeDef));
}

/**
  * @brief  Attach the Switch event callback function.
  * @param  handle: the Switch handle strcut.
  * @param  event: trigger event type.
  * @param  CB: callback function.
  * @retval None
  */
void Switch_Attach(struct Switch_TypeDef *handle, SwitchEvent Event, SwitchCallback CB)
{
    handle->CB[Event] = CB;
}

/**
  * @brief  Detach the Switch event callback function.
  * @param  handle: the Switch handle strcut.
  * @param  event: trigger event type.
  * @retval None
  */
void Switch_Detach(struct Switch_TypeDef *handle, PressEvent Event)
{
    handle->CB[Event] = NULL;
}

/**
  * @brief  Inquire the Switch event happen.
  * @param  handle: the Switch handle strcut.
  * @retval Switch event.
  */
SwitchEvent Get_Switch_Event(struct Switch_TypeDef *handle)
{
    return (SwitchEvent)(handle->Event);
}

/**
  * @brief  Start the Switch work, add the handle into work list.
  * @param  handle: target handle strcut.
  * @retval None
  */
void Switch_Start(struct Switch_TypeDef *handle)
{
    list_add(&handle->Entry, &head_handle);
}

/**
  * @brief  Stop the Switch work, remove the handle off work list.
  * @param  handle: target handle strcut.
  * @retval None
  */
void Switch_Stop(struct Switch_TypeDef *handle)
{
    list_del(&handle->Entry);
}

/**
  * @brief  Switch driver core function, driver state machine.
  * @param  handle: the Switch handle strcut.
  * @retval None
  */
static void Switch_Handler(struct Switch_TypeDef *handle)
{
    uint8_t read_gpio_level = handle->HALSwitchLevel();

    /*------------Switch debounce handle---------------*/
    if(read_gpio_level != handle->SwitchLevel)
    {
        //not equal to prev one
        //continue read 3 times same new level change
        if(++(handle->DebounceCnt) >= DEBOUNCE_TICKS)
        {
            handle->SwitchLevel = read_gpio_level;
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
            if(handle->SwitchLevel == handle->ActiveLevel)
            {
                handle->Event = (uint8_t)SWITCH_DOWN;
                EVENT_CB(SWITCH_DOWN);
                handle->State  = 1;
            }
            else
                handle->Event = (uint8_t)NONE_SWITCH;

            break;

        case 1:
            if(handle->SwitchLevel != handle->ActiveLevel)
            {
                handle->Event = (uint8_t)SWITCH_UP;
                EVENT_CB(SWITCH_UP);
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
void Switch_Ticks(void)
{
    struct Switch_TypeDef *target;

    list_for_each_entry(target, &head_handle, Entry)
    {
        Switch_Handler(target);
    }
}

#ifdef DESIGN_VERIFICATION_SWITCH
#include "k-test.h"
#include "task/k-timtask.h"

static struct Switch_TypeDef Switch_Test_Inst;

static uint8_t Switch_Read_Pin(void)
{
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);
}

static void Switch_Callback(void *btn)
{
    uint32_t btn_event_val;

    btn_event_val = Get_Switch_Event((struct Switch_TypeDef *)btn);

    switch(btn_event_val)
    {
        case SWITCH_DOWN:
            Switch_printf("Switch press down");
            break;

        case SWITCH_UP:
            Switch_printf("Switch press up");
            break;
    }
}

int Multi_Switch_Test(void)
{
    SwitchTask_Init();

    /* low level drive */
    Switch_Init(&Switch_Test_Inst, Switch_Read_Pin, 0);
    Switch_Attach(&Switch_Test_Inst, SWITCH_DOWN, Switch_Callback);
    Switch_Attach(&Switch_Test_Inst, SWITCH_UP, Switch_Callback);
    Switch_Start(&Switch_Test_Inst);

    return 0;
}

int t_Switch_Attach(int argc, char **argv)
{
    SwitchTask_Init();

    /* low level drive */
    Switch_Init(&Switch_Test_Inst, Switch_Read_Pin, 0);
    Switch_Attach(&Switch_Test_Inst, SWITCH_DOWN, Switch_Callback);
    Switch_Attach(&Switch_Test_Inst, SWITCH_UP, Switch_Callback);
    Switch_Start(&Switch_Test_Inst);

    Switch_printf("Switch test is running, please push the Switch.");

    return PASS;
}

int t_Switch_Detach(int argc, char **argv)
{
    SwitchTask_Deinit();

    /* low level drive */
    Switch_Deinit(&Switch_Test_Inst, Switch_Read_Pin, 0);
    Switch_Detach(&Switch_Test_Inst, SWITCH_DOWN, Switch_Callback);
    Switch_Detach(&Switch_Test_Inst, SWITCH_UP, Switch_Callback);
    Switch_Stop(&Switch_Test_Inst);

    Switch_printf("Switch test is over");

    return PASS;
}

#endif