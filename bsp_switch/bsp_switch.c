#include "peripheral/bsp_switch.h"
  
static struct Switch_TypeDef* head_handle = NULL;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Switch_Test function, register the corresponding button and Switch_Callback callback function.
  * @step 3:  Call function Switch_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Switch_Test once in function main.
  */

#include "string.h"
#include "task/bsp_timtask.h"
#include "protocol/hydrology.h"

#define DEBUG
#include "idebug.h"

#define Switch_printf    p_dbg
   
//According to your need to modify the constants.
#define TICKS_INTERVAL    20 //ms
#define DEBOUNCE_TICKS    3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

static void Switch_Handler(struct Switch_TypeDef* handle);

static struct Switch_TypeDef Switch_Test_Inst;

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

#if 0
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
        hydrologyProcessSend(Test);
        Switch_printf("Switch press down"); 
    break; 

    case SWITCH_UP: 
        Switch_printf("Switch press up");
    break;
  }
}

int Multi_Switch_Test(void)
{
  /* low level drive */
  Switch_Init  (&Switch_Test_Inst, Switch_Read_Pin, 0);
  Switch_Attach(&Switch_Test_Inst, SWITCH_DOWN, Switch_Callback);
  Switch_Attach(&Switch_Test_Inst, SWITCH_UP, Switch_Callback);
  Switch_Start (&Switch_Test_Inst);

  return 0; 
}
#endif

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define EVENT_CB(ev) if(handle->CB[ev]) handle->CB[ev]((Switch_TypeDef*)handle)

/**
  * @brief  Initializes the button struct handle.
  * @param  handle: the button handle strcut.
  * @param  pin_level: read the pin of the connet button level.
  * @param  ActiveLevel: pin pressed level.
  * @retval None
  */
void Switch_Init(struct Switch_TypeDef* handle, uint8_t(*pin_level)(void), uint8_t ActiveLevel)
{
  memset(handle, 0, sizeof(struct Switch_TypeDef));
  handle->Event = (uint8_t)NONE_SWITCH;
  handle->HALSwitchLevel = pin_level;
  handle->SwitchLevel = handle->HALSwitchLevel();
  handle->ActiveLevel = ActiveLevel;
}

/**
  * @brief  Attach the button event callback function.
  * @param  handle: the button handle strcut.
  * @param  event: trigger event type.
  * @param  CB: callback function.
  * @retval None
  */
void Switch_Attach(struct Switch_TypeDef* handle, SwitchEvent Event, SwitchCallback CB)
{
  handle->CB[Event] = CB;
}

/**
  * @brief  Inquire the button event happen.
  * @param  handle: the button handle strcut.
  * @retval button event.
  */
SwitchEvent Get_Switch_Event(struct Switch_TypeDef* handle)
{
  return (SwitchEvent)(handle->Event);
}

/**
  * @brief  Start the button work, add the handle into work list.
  * @param  handle: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int Switch_Start(struct Switch_TypeDef* handle)
{
  struct Switch_TypeDef* target = head_handle;
  
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
void Switch_Stop(struct Switch_TypeDef* handle)
{
  struct Switch_TypeDef** curr;
  
  for(curr = &head_handle; *curr;) 
  {
    struct Switch_TypeDef* entry = *curr;
    
    if (entry == handle) 
    {
      *curr = entry->Next;
    } 
    else
    {
      curr = &entry->Next;
    }
  }
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void Switch_Ticks(void)
{
  struct Switch_TypeDef* target;
  
  for(target = head_handle; target != NULL; target = target->Next)
  {
    Switch_Handler(target);
  }
}

/**
  * @brief  button driver core function, driver state machine.
  * @param  handle: the button handle strcut.
  * @retval None
  */
static void Switch_Handler(struct Switch_TypeDef* handle)
{
  uint8_t read_gpio_level = handle->HALSwitchLevel();

  /*------------button debounce handle---------------*/
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
  switch (handle->State) 
  {
  case 0:
    if(handle->SwitchLevel == handle->ActiveLevel) 
    { 
      handle->Event = (uint8_t)SWITCH_DOWN;
      EVENT_CB(SWITCH_DOWN);
      handle->State  = 1; 
    } 
    else 
    {
      handle->Event = (uint8_t)NONE_SWITCH;
    }
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