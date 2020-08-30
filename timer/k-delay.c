#include "timer/k-delay.h"
#include "timer/k-basictimer.h"
#include "stdint.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "tim.h"
#include "core/idebug.h"

//#define DELAY_USING_HARDWARE

#ifdef DELAY_USING_HARDWARE
uint32_t DelayInputClock = 0;
uint32_t DelayUnit = 0;
uint32_t DelayPrescaler = 0;
uint16_t DelayPeriod = 0;
volatile uint8_t DelayFlag = 0;

void Delay_EnableTimer(void)
{
    HAL_TIM_Base_Start(&htim2);
}

void Delay_DisableTimer(void)
{
    HAL_TIM_Base_Stop(&htim2);
}

void Delay_Init(void)
{
    DelayInputClock = SystemCoreClock / 2;
    DelayUnit = 10;
    DelayPrescaler = (DelayInputClock / DelayUnit / 1000000) - 1;
    DelayPeriod = DelayUnit - 1;

    if(DelayInputClock >= 600000000)
        kinetis_debug_trace(KERN_DEBUG, "Inputing clock is too large, please modify the delay unit.");

    Delay_EnableTimer();
#ifdef DELAY_USING_STM32LIB
    Delay_ClearFlag();
#endif
    Delay_s(1);
}

void Delay_SetTimerPara(uint16_t Prescaler, uint32_t Period)
{
#ifdef DELAY_USING_STM32LIB
    htim2.Init.Prescaler = Prescaler;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = Period;

    if(HAL_TIM_Base_Init(&htim2) != HAL_OK)
        Error_Handler();

#else
    TIM2->PSC = Prescaler;
    TIM2->CNT = Period;
#endif
    TIM2->EGR |= 1;
    TIM2->SR = 0;
}

#ifdef DELAY_USING_STM32LIB
void Delay_SetFlag(void)
{
    DelayFlag = 1;
}

void Delay_ClearFlag(void)
{
    DelayFlag = 0;
}

uint8_t Delay_GetFlag(void)
{
    return DelayFlag;
}
#endif

void Delay_WaitCountEnd(void)
{
#ifdef DELAY_USING_STM32LIB

    while(Delay_GetFlag() == 0) {}

    Delay_ClearFlag();
#else

    while((TIM2->SR & TIM_FLAG_UPDATE) != SET);

    TIM2->SR &= ~((uint16_t)TIM_FLAG_UPDATE);
#endif
}
#endif

/*
 * The latency ranges from 0 to 2^32, but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void Delay_us(uint32_t Delay)
{
#ifdef DELAY_USING_HARDWARE
    DelayPrescaler = (DelayInputClock / DelayUnit / 1000000) - 1;
    DelayPeriod = DelayUnit * Delay - 1;
    Delay_SetTimerPara(DelayPrescaler, DelayPeriod);
    Delay_WaitCountEnd();
#else
    uint32_t refer = 0;
    uint32_t delta = 0;
    uint32_t ticks = 0;

    if(Delay > DELAY_TIMER_UNIT)
    {
        ticks = DELAY_TIMER_UNIT;
        refer = BasicTimer_GetUSTick();

        while(delta < ticks)
        {
            delta = BasicTimer_GetUSTick() >= refer ?
                BasicTimer_GetUSTick() - refer :
                BasicTimer_GetUSTick() + (DELAY_TIMER_UNIT - refer);
        }

        Delay_us(Delay - DELAY_TIMER_UNIT);
    }
    else
    {
        ticks = Delay;
        refer = BasicTimer_GetUSTick();

        while(delta < ticks)
        {
            delta = BasicTimer_GetUSTick() >= refer ?
                BasicTimer_GetUSTick() - refer :
                BasicTimer_GetUSTick() + (DELAY_TIMER_UNIT - refer);
        }
    }

#endif
}

/*
 * The latency ranges from 0 to (2^32 / 1000), but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void Delay_ms(uint32_t Delay)
{
#ifdef DELAY_USING_HARDWARE
    DelayPrescaler = (DelayInputClock / DelayUnit / 1000) - 1;
    DelayPeriod = DelayUnit * Delay - 1;
    Delay_SetTimerPara(DelayPrescaler, DelayPeriod);
    Delay_WaitCountEnd();
#else
    uint32_t refer = 0;
    uint32_t delta = 0;
    uint32_t ticks = 0;

    if(Delay > DELAY_TIMER_UNIT)
    {
        ticks = DELAY_TIMER_UNIT;
        refer = BasicTimer_GetMSTick();

        while(delta < ticks)
        {
            delta = BasicTimer_GetMSTick() >= refer ?
                BasicTimer_GetMSTick() - refer :
                BasicTimer_GetMSTick() + (DELAY_TIMER_UNIT - refer);
        }

        Delay_ms(Delay - DELAY_TIMER_UNIT);
    }
    else
    {
        ticks = Delay;
        refer = BasicTimer_GetMSTick();

        while(delta < ticks)
        {
            delta = BasicTimer_GetMSTick() >= refer ?
                BasicTimer_GetMSTick() - refer :
                BasicTimer_GetMSTick() + (DELAY_TIMER_UNIT - refer);
        }
    }

#endif
}

/*
 * The latency ranges from 0 to (2^32 / 1000000), but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void Delay_s(uint32_t Delay)
{
    Delay_ms(Delay * 1000);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef DESIGN_VERIFICATION_DELAY
#include "dv/k-test.h"

int t_Delay(int argc, char **argv)
{
    uint32_t Timestamp = 0;

    Timestamp = BasicTimer_GetUSTick();
    Delay_us(1000);
    Timestamp = BasicTimer_GetUSTick() - Timestamp;
    kinetis_debug_trace(KERN_DEBUG, "Delay 1000 us, The result = %lu us.", Timestamp);

    Timestamp = BasicTimer_GetMSTick();
    Delay_ms(1000);
    Timestamp = BasicTimer_GetMSTick() - Timestamp;
    kinetis_debug_trace(KERN_DEBUG, "Delay 1000 ms, The result = %lu ms.", Timestamp);

    Timestamp = BasicTimer_GetSSTick();
    Delay_s(3);
    Timestamp = BasicTimer_GetSSTick() - Timestamp;
    kinetis_debug_trace(KERN_DEBUG, "Delay 3 s, The result = %lu s.", Timestamp);

    return PASS;
}

#endif

