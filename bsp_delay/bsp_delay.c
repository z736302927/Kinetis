#include "timer/bsp_delay.h"
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

#define DEBUG
#include "idebug.h"

#define Delay_printf                  p_dbg

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
  {
    Delay_printf("Inputing clock is too large, please modify the delay unit.");
  }
  
  Delay_EnableTimer();
  Delay_ClearFlag();
  Delay_s(1);
}

void Delay_SetTimerPara(uint16_t Prescaler, uint32_t Period)
{
#ifdef DELAY_USING_STM32LIB
  htim2.Init.Prescaler = Prescaler;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = Period;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
#else
  TIM2->PSC = Prescaler;
  TIM2->CNT = Period;
#endif
  TIM2->EGR |= 1;
  TIM2->SR = 0;
}

void Delay_SetFlag(void)
{
#ifdef DELAY_USING_STM32LIB
  DelayFlag = 1;
#endif
}

void Delay_ClearFlag(void)
{
#ifdef DELAY_USING_STM32LIB
  DelayFlag = 0;
#endif
}

uint8_t Delay_GetFlag(void)
{
  return DelayFlag;
}

void Delay_WaitCountEnd(void)
{
#ifdef DELAY_USING_STM32LIB
  while(Delay_GetFlag() == 0){}
  Delay_ClearFlag();
#else
  while((TIM2->SR & TIM_FLAG_UPDATE) != SET);
  TIM2->SR &= ~((uint16_t)TIM_FLAG_UPDATE);
#endif
}

void Delay_us(uint32_t Delay)
{
  DelayPrescaler = (DelayInputClock / DelayUnit / 1000000) - 1;
  DelayPeriod = DelayUnit * Delay - 1;
  Delay_SetTimerPara(DelayPrescaler, DelayPeriod);
  Delay_WaitCountEnd();
}

void Delay_ms(uint32_t Delay)
{
  DelayPrescaler = (DelayInputClock / DelayUnit / 1000) - 1;
  DelayPeriod = DelayUnit * Delay - 1;
  Delay_SetTimerPara(DelayPrescaler, DelayPeriod);
  Delay_WaitCountEnd();
}

void Delay_s(uint32_t Delay)
{
  Delay_ms(Delay * 1000);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/
