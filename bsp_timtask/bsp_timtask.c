#include "bsp_timtask/bsp_timtask.h"

//TimTask Handle list head.
static struct TimTask_TypeDef* TimTask_HeadHandler = NULL;

//TimTask_TypeDef ticks
static uint32_t _TimTask_ticks = 0;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in function TimTask_Test, design the function you need and initialize it in the main function.
  * @step 3:  Call function TimTask_Ticks periodically at the frequency you want.
  * @step 4:  An infinite loop calls function TimTask_Loop.
  * @step 5:  Note that the base frequency determines your actual call time.Please calculate in advance.
  */

#define DEBUG
#include "idebug/idebug.h"

#define TimTask_printf    p_dbg

struct TimTask_TypeDef TimTask1;

void TimTask1_Callback(void)
{
//  TimTask_printf("TimTask1 timeout!");
}

void TimTask_Test(void)
{
  TimTask_Init(&TimTask1, TimTask1_Callback, 1000, 1000); //1s loop
  TimTask_Start(&TimTask1);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @brief  Initializes the TimTask struct handle.
  * @param  Handle: the TimTask handle strcut.
  * @param  Timeout_cb: Timeout callback.
  * @param  Repeat: Repeat interval time.
  * @retval None
  */
void TimTask_Init(struct TimTask_TypeDef* Handle, void(*Timeout_cb)(), uint32_t Timeout, uint32_t Repeat)
{
  // memset(Handle, sizeof(struct TimTask_TypeDef), 0);
  Handle->Timeout_cb = Timeout_cb;
  Handle->Timeout = _TimTask_ticks + Timeout;
  Handle->Repeat = Repeat;
}

/**
  * @brief  Start the TimTask work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int TimTask_Start(struct TimTask_TypeDef* Handle)
{
  struct TimTask_TypeDef* target = TimTask_HeadHandler;
  
  while(target) 
  {
    if(target == Handle) 
      return -1;  //already exist.
    target = target->Next;
  }
  Handle->Next = TimTask_HeadHandler;
  TimTask_HeadHandler = Handle;
  return 0;
}

/**
  * @brief  Stop the TimTask work, remove the handle off work list.
  * @param  Handle: target handle strcut.
  * @retval None
  */
void TimTask_Stop(struct TimTask_TypeDef* Handle)
{
  struct TimTask_TypeDef** curr;
  
  for(curr = &TimTask_HeadHandler; *curr;) 
  {
    struct TimTask_TypeDef* entry = *curr;
    if (entry == Handle) 
    {
      *curr = entry->Next;
//      free(entry);
    } 
    else
      curr = &entry->Next;
  }
}

/**
  * @brief  main loop.
  * @param  None.
  * @retval None
  */
void TimTask_Loop(void)
{
  struct TimTask_TypeDef* target;
  
  for(target = TimTask_HeadHandler; target; target = target->Next)
  {
    if(_TimTask_ticks >= target->Timeout) 
    {
      if(target->Repeat == 0) 
      {
        TimTask_Stop(target);
      }
      else 
      {
        target->Timeout = _TimTask_ticks + target->Repeat;
      }
      target->Timeout_cb();
    }
  }
}

/**
  * @brief  background ticks, TimTask repeat invoking interval 1ms.
  * @param  None.
  * @retval None.
  */
void TimTask_Ticks(void)
{
  _TimTask_ticks++;
}

