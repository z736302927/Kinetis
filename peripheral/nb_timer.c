#include "peripheral/nb_timer.h"
#include "peripheral/nb_bc95.h"

static NB_TimeoutCallback  NB_IOT_TimeoutCallback;

static uint32_t NB_IOT_TickStart = 0;
static uint32_t NB_IOT_TickDelta = 0;
static uint8_t NB_IOT_TimeOpenFlag = 0;


/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  You just need to implement NB_IOT_GetTick, an ms timer, increments every ms, up to 2 to the 32, and then zero.
  */

static uint32_t NB_IOT_GetTick(void)
{
    return BasicTimer_GetMSTick();
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


void NB_IOT_SetTim(NB_TimeoutCallback cb)
{
    NB_IOT_TimeoutCallback = cb;
}

void NB_IOT_StartTim(uint32_t ms)
{
    NB_IOT_TickDelta = ms;
    NB_IOT_TickStart = NB_IOT_GetTick();
    NB_IOT_TimeOpenFlag = 1;
}

void NB_IOT_StopTim(void)
{
    NB_IOT_TimeOpenFlag = 0;
}

void NB_IOT_PollTim(void)
{
    uint32_t tick;

    if (NB_IOT_TimeOpenFlag == 1) {
        tick = NB_IOT_GetTick();
        uint32_t delta = tick >= NB_IOT_TickStart ? tick - NB_IOT_TickStart : tick + UINT32_MAX - NB_IOT_TickStart;

        if (NB_IOT_TickDelta > delta) {
            NB_IOT_TickDelta -= delta;
            NB_IOT_TickStart = tick;
        } else {
            NB_IOT_TickDelta = 0;
            NB_IOT_TimeOpenFlag = 0;

            if (NB_IOT_TimeoutCallback)
                NB_IOT_TimeoutCallback();
        }

    }
}