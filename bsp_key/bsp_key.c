#include "bsp_key.h"

#define EVENT_CB(ev) if(handle->CB[ev]) handle->CB[ev]((Button_TypeDef*)handle)
  
static struct Button_TypeDef* head_handle = NULL;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#include "string.h"


static void Button_Handler(struct Button_TypeDef* handle);


/**
  * @brief  Initializes the button struct handle.
  * @param  handle: the button handle strcut.
  * @param  pin_level: read the pin of the connet button level.
  * @param  ActiveLevel: pin pressed level.
  * @retval None
  */
void Button_Init(struct Button_TypeDef* handle, uint8_t(*pin_level)(void), uint8_t ActiveLevel)
{
  memset(handle, 0, sizeof(struct Button_TypeDef));
  handle->Event = (uint8_t)NONE_PRESS;
  handle->HALButtonLevel = pin_level;
  handle->ButtonLevel = handle->HALButtonLevel();
  handle->ActiveLevel = ActiveLevel;
}

/**
  * @brief  Attach the button event callback function.
  * @param  handle: the button handle strcut.
  * @param  event: trigger event type.
  * @param  CB: callback function.
  * @retval None
  */
void Button_Attach(struct Button_TypeDef* handle, PressEvent Event, BtnCallback CB)
{
  handle->CB[Event] = CB;
}

/**
  * @brief  Inquire the button event happen.
  * @param  handle: the button handle strcut.
  * @retval button event.
  */
PressEvent Get_Button_Event(struct Button_TypeDef* handle)
{
  return (PressEvent)(handle->Event);
}

/**
  * @brief  Start the button work, add the handle into work list.
  * @param  handle: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int Button_Start(struct Button_TypeDef* handle)
{
  struct Button_TypeDef* target = head_handle;
  
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
void Button_Stop(struct Button_TypeDef* handle)
{
  struct Button_TypeDef** curr;
  
  for(curr = &head_handle; *curr;) 
  {
    struct Button_TypeDef* entry = *curr;
    
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
void Button_Ticks(void)
{
  struct Button_TypeDef* target;
  
  for(target = head_handle; target != NULL; target = target->Next)
  {
    Button_Handler(target);
  }
}


/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/



/**
  * @brief  button driver core function, driver state machine.
  * @param  handle: the button handle strcut.
  * @retval None
  */
static void Button_Handler(struct Button_TypeDef* handle)
{
  uint8_t read_gpio_level = handle->HALButtonLevel();

  //Ticks counter working..
  if((handle->State) > 0) 
  {
    handle->Ticks++;
  }

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
  switch (handle->State) 
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
    {
      handle->Event = (uint8_t)NONE_PRESS;
    }
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
      {
        handle->State = 0;
      }
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




