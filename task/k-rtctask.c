#include "task/k-rtctask.h"
#include "stdio.h"
#include "string.h"
//RTCTask Handle list head.
static struct RTCTask_TypeDef *RTCTask_HeadHandler = NULL;

//RTCTask_TypeDef ticks
static RTCTask_DateTime_TypeDef _RTCTask_CurrentTime;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in function RTCTask_Test, design the function you need and initialize it in the main function.
  * @step 3:  Call the time update function RTCTask_GetCurrentTime every second.
  * @step 4:  An infinite loop calls function RTCTask_Loop.
  */

#include "idebug.h"

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @brief  Initializes the RTCTask struct handle.
  * @param  Handle: the RTCTask handle strcut.
  * @param  Timeout_cb: Timeout callback.
  * @param  Repeat: Repeat interval time.
  * @retval None
  */
void RTCTask_Init(struct RTCTask_TypeDef *Handle, void(*Timeout_cb)(), uint8_t AddHours, uint8_t AddMinutes, uint8_t AddSeconds)
{
    // memset(Handle, sizeof(struct RTCTask_TypeDef), 0);
    Handle->Timeout_cb = Timeout_cb;

    Handle->DeltaTime.Hours = AddHours;
    Handle->DeltaTime.Minutes = AddMinutes;
    Handle->DeltaTime.Seconds = AddSeconds;

    Handle->ExpiredTime.Year = _RTCTask_CurrentTime.Year;
    Handle->ExpiredTime.Month = _RTCTask_CurrentTime.Month;
    Handle->ExpiredTime.Date = _RTCTask_CurrentTime.Date;
    Handle->ExpiredTime.Hours = _RTCTask_CurrentTime.Hours + Handle->DeltaTime.Hours;
    Handle->ExpiredTime.Minutes = _RTCTask_CurrentTime.Minutes + Handle->DeltaTime.Minutes;
    Handle->ExpiredTime.Seconds = _RTCTask_CurrentTime.Seconds + Handle->DeltaTime.Seconds;
}

/**
  * @brief  Deinitializes the RTCTask struct handle.
  * @param  Handle: the RTCTask handle strcut.
  * @retval None
  */
void RTCTask_Deinit(struct RTCTask_TypeDef *Handle)
{
    memset(Handle, sizeof(struct RTCTask_TypeDef), 0);
}

/**
  * @brief  Start the RTCTask work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int RTCTask_Start(struct RTCTask_TypeDef *Handle)
{
    struct RTCTask_TypeDef *target = RTCTask_HeadHandler;

    while(target)
    {
        if(target == Handle)
        {
            return -1;    //already exist.
        }

        target = target->Next;
    }

    Handle->Next = RTCTask_HeadHandler;
    RTCTask_HeadHandler = Handle;
    return 0;
}

/**
  * @brief  Stop the RTCTask work, remove the handle off work list.
  * @param  Handle: target handle strcut.
  * @retval None
  */
void RTCTask_Stop(struct RTCTask_TypeDef *Handle)
{
    struct RTCTask_TypeDef **curr;

    for(curr = &RTCTask_HeadHandler; *curr;)
    {
        struct RTCTask_TypeDef *entry = *curr;

        if(entry == Handle)
        {
            *curr = entry->Next;
//      kfree(entry);
        }
        else
            curr = &entry->Next;
    }
}

/**
  * @brief  .
  * @param  None.
  * @retval None
  */
static uint8_t RTCTask_Expired(RTCTask_TypeDef *Target)
{
    uint8_t tmp_timecmp1[12];
    uint8_t tmp_timecmp2[12];
    int8_t tmp_res = 0;

    snprintf((char *)tmp_timecmp1, sizeof(tmp_timecmp1), "%02d%02d%02d%02d%02d%02d", _RTCTask_CurrentTime.Year\
        , _RTCTask_CurrentTime.Month
        , _RTCTask_CurrentTime.Date
        , _RTCTask_CurrentTime.Hours
        , _RTCTask_CurrentTime.Minutes
        , _RTCTask_CurrentTime.Seconds);

    snprintf((char *)tmp_timecmp2, sizeof(tmp_timecmp2), "%02d%02d%02d%02d%02d%02d", Target->ExpiredTime.Year\
        , Target->ExpiredTime.Month
        , Target->ExpiredTime.Date
        , Target->ExpiredTime.Hours
        , Target->ExpiredTime.Minutes
        , Target->ExpiredTime.Seconds);

    tmp_res = strncmp((char *)tmp_timecmp1, (char *)tmp_timecmp2, sizeof(tmp_timecmp1));

    if(tmp_res < 0)
        return 0;
    else
        return 1;

}

static void RTCTask_Special_AddDay(RTCTask_DateTime_TypeDef *TmpCalendar)
{
    if(TmpCalendar->Month == 2)/* Is it February?,28,29 */
    {
        /* The system will certainly not be in use until 2100, so it only judges whether it is divisible by 4 */
        if(TmpCalendar->Year % 4 == 0)//Leap year
        {
            if(TmpCalendar->Date < 30)  //No more than 29 days
                return ;

            TmpCalendar->Date -= 29;    //Minus the overflow of 29
            ++TmpCalendar->Month;        //Month +1
        }
        else
        {
            if(TmpCalendar->Date < 29)  //No more than 28 days
                return ;

            TmpCalendar->Date -= 28;    //Minus the overflow of 28
            ++TmpCalendar->Month;        //Month +1
        }
    }

    // Is it a 30-day month
    if(TmpCalendar->Month == 4 || TmpCalendar->Month == 6 || TmpCalendar->Month == 9 || TmpCalendar->Month == 11)
    {
        if(TmpCalendar->Date < 31)    //No more than 30 days
            return;

        TmpCalendar->Date -= 30;      //Minus the overflow of 30
        ++TmpCalendar->Month;          //Month+1
    }

    // Is it a 31-day month
    if(TmpCalendar->Month == 1 || TmpCalendar->Month == 3 || TmpCalendar->Month == 5 || TmpCalendar->Month == 7 || \
        TmpCalendar->Month == 8 || TmpCalendar->Month == 10 || TmpCalendar->Month == 12)
    {
        if(TmpCalendar->Date < 32)    //No more than 31 days
            return;

        TmpCalendar->Date -= 31;      //Minus the overflow of 30
        ++TmpCalendar->Month;          //Month +1
    }

    if(TmpCalendar->Month < 13)      //No more than December
        return ;

    TmpCalendar->Month -= 12;        //Minus 12 months of overflow
    ++TmpCalendar->Year;            //Year +1

    return;

}

static void RTCTask_Time_AddSecond(RTCTask_DateTime_TypeDef *TmpCalendar, uint8_t Seconds)
{
    if(Seconds > 60)
        return ;

    TmpCalendar->Seconds += Seconds;//Plus the number of Seconds

    if(TmpCalendar->Seconds < 60)
    {
        return ; //Finished
    }

    TmpCalendar->Seconds -= 60;//Minute + 1
    ++TmpCalendar->Hours;

    if(TmpCalendar->Minutes < 60)
    {
        return ; //Finished
    }

    TmpCalendar->Minutes -= 60;//Hours + 1
    ++TmpCalendar->Hours;

    if(TmpCalendar->Hours < 24)
    {
        return ;//Finished
    }

    TmpCalendar->Hours -= 24;//Days + 1;
    ++TmpCalendar->Date;

    RTCTask_Special_AddDay(TmpCalendar);
}

static void RTCTask_Time_AddMinute(RTCTask_DateTime_TypeDef *TmpCalendar, uint8_t Minutes)// Minutes cannot be greater than 60 minutes
{
    if(Minutes > 60)
        return ;

    TmpCalendar->Minutes += Minutes;    //Plus the number of Minutes

    if(TmpCalendar->Minutes < 60)
    {
        return ;            //Finished
    }

    TmpCalendar->Minutes -= 60;        //Hours + 1
    ++TmpCalendar->Hours;

    if(TmpCalendar->Hours < 24)
    {
        return ;            //Finished
    }

    TmpCalendar->Hours -= 24;        //Days + 1;
    ++TmpCalendar->Date;

    RTCTask_Special_AddDay(TmpCalendar);
}

static void RTCTask_Time_AddHour(RTCTask_DateTime_TypeDef *TmpCalendar, uint8_t Hours)// An hour cannot be greater than 24
{
    if(Hours > 24)
        return;

    TmpCalendar->Hours += Hours;

    if(TmpCalendar->Hours < 24)
    {
        return ;            //Finished
    }

    TmpCalendar->Hours -= 24;        //Days + 1;
    ++TmpCalendar->Date;

    RTCTask_Special_AddDay(TmpCalendar);
}

//static void RTCTask_Time_AddDay(RTCTask_DateTime_TypeDef *TmpCalendar, uint8_t Days)
//{
//    if(Days > 28)          //Make sure january doesn't jump to march
//        return;

//    TmpCalendar->Date += Days;
//    RTCTask_Special_AddDay(TmpCalendar);
//}

//static void RTCTask_Time_AddMonth(RTCTask_DateTime_TypeDef *TmpCalendar, uint8_t Months)
//{
//    if(Months > 12)
//        return;

//    TmpCalendar->Month += Months;

//    if(TmpCalendar->Month < 13)
//        return;

//    TmpCalendar->Month -= 12;
//    ++TmpCalendar->Year;
//}

/**
  * @brief  Use round and align.
  * @param  None.
  * @retval None
  */
static void RTCTask_UpdateTime(RTCTask_TypeDef *Target)
{
    Target->ExpiredTime.Year = _RTCTask_CurrentTime.Year;
    Target->ExpiredTime.Month = _RTCTask_CurrentTime.Month;
    Target->ExpiredTime.Date = _RTCTask_CurrentTime.Date;
    Target->ExpiredTime.Hours = _RTCTask_CurrentTime.Hours;
    Target->ExpiredTime.Minutes = _RTCTask_CurrentTime.Minutes;
    Target->ExpiredTime.Seconds = _RTCTask_CurrentTime.Seconds;

    if(Target->DeltaTime.Hours != 0)
    {
        Target->ExpiredTime.Seconds = 0;
        Target->ExpiredTime.Minutes = 0;
        Target->ExpiredTime.Hours -= Target->ExpiredTime.Hours % Target->DeltaTime.Hours;
        RTCTask_Time_AddHour(&(Target->ExpiredTime), Target->DeltaTime.Hours);
    }

    if(Target->DeltaTime.Minutes != 0)
    {
        Target->ExpiredTime.Seconds = 0;
        Target->ExpiredTime.Minutes -= Target->ExpiredTime.Minutes % Target->DeltaTime.Minutes;
        RTCTask_Time_AddMinute(&(Target->ExpiredTime), Target->DeltaTime.Minutes);
    }

    if(Target->DeltaTime.Seconds != 0)
        RTCTask_Time_AddSecond(&(Target->ExpiredTime), Target->DeltaTime.Seconds);
}

/**
  * @brief  Task loop.
  * @param  None.
  * @retval None
  */
void RTCTask_Loop(void)
{
    struct RTCTask_TypeDef *target;

    for(target = RTCTask_HeadHandler; target; target = target->Next)
    {
        if(RTCTask_Expired(target))
        {
            if(target->DeltaTime.Hours == 0 && target->DeltaTime.Minutes == 0 && target->DeltaTime.Seconds == 0)
                RTCTask_Stop(target);
            else
                RTCTask_UpdateTime(target);

            target->Timeout_cb();
        }
    }
}

/**
  * @brief  background ticks, RTCTask repeat invoking interval 1s.
  * @param  None.
  * @retval None.
  */
void RTCTask_GetCurrentTime(uint8_t TmpYear, uint8_t TmpMonth, uint8_t TmpDate, uint8_t TmpHours, uint8_t TmpMinutes, uint8_t TmpSeconds)
{
    _RTCTask_CurrentTime.Year = TmpYear;
    _RTCTask_CurrentTime.Month = TmpMonth;
    _RTCTask_CurrentTime.Date = TmpDate;
    _RTCTask_CurrentTime.Hours = TmpHours;
    _RTCTask_CurrentTime.Minutes = TmpMinutes;
    _RTCTask_CurrentTime.Seconds = TmpSeconds;
}

#ifdef DESIGN_VERIFICATION_RTCTASK
#include "k-test.h"
#include "timer/k-timeout.h"

struct RTCTask_TypeDef RTCTask1;
static uint8_t RTCTask_Flag = 0;

void RTCTask1_Callback(void)
{
    RTCTask_Flag = true;
    kinetis_debug_trace(KERN_DEBUG, "RTCTask1 timeout!");
}

int t_RTCTask_Add(int argc, char **argv)
{
    uint32_t Timestamp = 0;

    Timestamp = BasicTimer_GetMSTick();

    RTCTask_Init(&RTCTask1, RTCTask1_Callback, 0, 0, 1);
    RTCTask_Start(&RTCTask1);
    Timeout_WaitMSDone(&RTCTask_Flag, true, 2000);

    Timestamp = BasicTimer_GetMSTick() - Timestamp;
    kinetis_debug_trace(KERN_DEBUG, "RTCTask elapse time = %lu ms.", Timestamp);

    if(Timestamp > 1100)
        return FAIL;
    else
        return PASS;
}

int t_RTCTask_Delete(int argc, char **argv)
{
    RTCTask_Deinit(&RTCTask1);
    RTCTask_Stop(&RTCTask1);

    return PASS;
}

#endif

