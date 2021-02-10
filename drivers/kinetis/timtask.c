#include "task/timtask.h"
#include "string.h"

//TimTask Handle list head.
static struct TimTask_TypeDef *TimTask_HeadHandler = NULL;

//TimTask_TypeDef ticks
static u32 _TimTask_ticks = 0;

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in function TimTask_Test, design the function you need and initialize it in the main function.
  * @step 3:  Call function TimTask_Ticks periodically at the frequency you want.
  * @step 4:  An infinite loop calls function TimTask_Loop.
  * @step 5:  Note that the base frequency determines your actual call time.Please calculate in advance.
  */

#include "kinetis/idebug.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @brief  Initializes the TimTask struct handle.
  * @param  Handle: the TimTask handle strcut.
  * @param  Timeout_cb: Timeout callback.
  * @param  Repeat: Repeat interval time.
  * @retval None
  */
void TimTask_Init(struct TimTask_TypeDef *Handle, void(*Timeout_cb)(), u32 Timeout, u32 Repeat)
{
    memset(Handle, 0, sizeof(struct TimTask_TypeDef));
    Handle->Timeout_cb = Timeout_cb;
    Handle->Timeout = _TimTask_ticks + Timeout;
    Handle->Repeat = Repeat;
}

/**
  * @brief  Deinitializes the TimTask struct handle.
  * @param  Handle: the TimTask handle strcut.
  * @retval None
  */
void TimTask_Deinit(struct TimTask_TypeDef *Handle)
{
    memset(Handle, 0, sizeof(struct TimTask_TypeDef));
}

/**
  * @brief  Start the TimTask work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int TimTask_Start(struct TimTask_TypeDef *Handle)
{
    struct TimTask_TypeDef *target = TimTask_HeadHandler;

    while (target) {
        if (target == Handle) {
            return -1;    //already exist.
        }

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
void TimTask_Stop(struct TimTask_TypeDef *Handle)
{
    struct TimTask_TypeDef **curr;

    for (curr = &TimTask_HeadHandler; *curr;) {
        struct TimTask_TypeDef *entry = *curr;

        if (entry == Handle) {
            *curr = entry->Next;
//      kfree(entry);
        } else
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
    struct TimTask_TypeDef *target;

    for (target = TimTask_HeadHandler; target; target = target->Next) {
        if (_TimTask_ticks >= target->Timeout) {
            if (target->Repeat == 0)
                TimTask_Stop(target);
            else
                target->Timeout = _TimTask_ticks + target->Repeat;

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

#ifdef DESIGN_VERIFICATION_TIMTASK
#include "kinetis/test.h"
#include "kinetis/timeout.h"

struct TimTask_TypeDef TimTask1;
static u8 TimTask_Flag = 0;

void TimTask1_Callback(void)
{
    TimTask_Flag = true;
    kinetis_print_trace(KERN_DEBUG, "TimTask1 timeout!");
}

int t_TimTask_Add(int argc, char **argv)
{
    u32 Timestamp = 0;

    Timestamp = basic_timer_get_ms_tick();

    TimTask_Init(&TimTask1, TimTask1_Callback, 1000, 1000); //1s loop
    TimTask_Start(&TimTask1);
    Timeout_WaitMSDone(&TimTask_Flag, true, 2000);

    Timestamp = basic_timer_get_ms_tick() - Timestamp;
    kinetis_print_trace(KERN_DEBUG, "TimTask elapse time = %lu ms.", Timestamp);

    if (Timestamp > 1100)
        return FAIL;
    else
        return PASS;
}

int t_TimTask_Delete(int argc, char **argv)
{
    TimTask_Deinit(&TimTask1);
    TimTask_Stop(&TimTask1);

    return PASS;
}
#endif

