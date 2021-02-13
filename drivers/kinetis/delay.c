#include "kinetis/delay.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"

#include <linux/printk.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "tim.h"

//#define DELAY_USING_HARDWARE

#ifdef DELAY_USING_HARDWARE

struct delay_para
{
	u32 input_clock;
	u32 unit;
	u32 prescaler;
	u16 period;
	u8 flag;
};

struct delay_para delay_priv_val;

void delay_enable_timer(void)
{
    HAL_TIM_Base_Start(&htim2);
}

void delay_disable_timer(void)
{
    HAL_TIM_Base_Stop(&htim2);
}

void delay_init(void)
{
    delay_priv_val.input_clock = SystemCoreClock / 2;
    delay_priv_val.unit = 10;
    delay_priv_val.prescaler = (delay_priv_val.input_clock / delay_priv_val.unit / 1000000) - 1;
    delay_priv_val.period = delay_priv_val.unit - 1;

    if (delay_priv_val.input_clock >= 600000000)
        printk(KERN_DEBUG
            "Inputing clock is too large, please modify the delay unit.");

    delay_enable_timer();
#ifdef DELAY_USING_STM32LIB
    delay_clear_flag();
#endif
    sdelay(1);
}

static void delay_set_timer_para(u16 prescaler, u32 period)
{
#ifdef DELAY_USING_STM32LIB
    htim2.Init.Prescaler = prescaler;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = period;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
        Error_Handler();

#else
    TIM2->PSC = prescaler;
    TIM2->CNT = period;
#endif
    TIM2->EGR |= 1;
    TIM2->SR = 0;
}

#ifdef DELAY_USING_STM32LIB
void delay_set_flag(void)
{
    delay_priv_val.flag = 1;
}

void delay_clear_flag(void)
{
    delay_priv_val.flag = 0;
}

u8 delay_get_flag(void)
{
    return delay_priv_val.flag;
}
#endif

static void delay_wait_cnt_end(void)
{
#ifdef DELAY_USING_STM32LIB

    while (delay_get_flag() == 0) {}

    delay_clear_flag();
#else

    while ((TIM2->SR & TIM_FLAG_UPDATE) != SET);

    TIM2->SR &= ~((u16)TIM_FLAG_UPDATE);
#endif
}
#endif

/*
 * The latency ranges from 0 to 2^32, but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void udelay(u32 delay)
{
    if (delay > 1000000) {
        printk(KERN_ERR
            "The %s() input parameter is greater than 1000000, "
            "please use mdelay()", __func__);
        return;
    }

#ifdef DELAY_USING_HARDWARE
    delay_priv_val.prescaler = (delay_priv_val.input_clock / delay_priv_val.unit / 1000000) - 1;
    delay_priv_val.period = delay_priv_val.unit * delay - 1;
    delay_set_timer_para(delay_priv_val.prescaler, delay_priv_val.period);
    delay_wait_cnt_end();
#else
    u32 refer = 0;
    u32 delta = 0;
    u32 ticks = 0;

    if (delay > DELAY_TIMER_UNIT) {
        ticks = DELAY_TIMER_UNIT;
        refer = basic_timer_get_timer_cnt();

        while (delta < ticks) {
            delta = basic_timer_get_timer_cnt() >= refer ?
                basic_timer_get_timer_cnt() - refer :
                basic_timer_get_timer_cnt() + (DELAY_TIMER_UNIT - refer);
        }

        udelay(delay - DELAY_TIMER_UNIT);
    } else {
        ticks = delay;
        refer = basic_timer_get_timer_cnt();

        while (delta < ticks) {
            delta = basic_timer_get_timer_cnt() >= refer ?
                basic_timer_get_timer_cnt() - refer :
                basic_timer_get_timer_cnt() + (DELAY_TIMER_UNIT - refer);
        }
    }

#endif
}

/*
 * The latency ranges from 0 to (2^32 / 1000), but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void mdelay(u32 delay)
{
    if (delay > 1000000) {
        printk(KERN_ERR
            "The %s() input parameter is greater than 1000000, "
            "please use sdelay()", __func__);
        return;
    }

#ifdef DELAY_USING_HARDWARE
    delay_priv_val.prescaler = (delay_priv_val.input_clock / delay_priv_val.unit / 1000) - 1;
    delay_priv_val.period = delay_priv_val.unit * delay - 1;
    delay_set_timer_para(delay_priv_val.prescaler, delay_priv_val.period);
    delay_wait_cnt_end();
#else
    u32 refer = 0;
    u32 delta = 0;
    u32 ticks = 0;

    if (delay > DELAY_TIMER_UNIT) {
        ticks = DELAY_TIMER_UNIT;
        refer = basic_timer_get_ms();

        while (delta < ticks) {
            delta = basic_timer_get_ms() >= refer ?
                basic_timer_get_ms() - refer :
                basic_timer_get_ms() + (DELAY_TIMER_UNIT - refer);
        }

        mdelay(delay - DELAY_TIMER_UNIT);
    } else {
        ticks = delay;
        refer = basic_timer_get_ms();

        while (delta < ticks) {
            delta = basic_timer_get_ms() >= refer ?
                basic_timer_get_ms() - refer :
                basic_timer_get_ms() + (DELAY_TIMER_UNIT - refer);
        }
    }

#endif
}

/*
 * The latency ranges from 0 to (2^32 / 1000000), but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void sdelay(u32 delay)
{
    if (delay > 1000) {
        printk(KERN_ERR
            "The %s() input parameter is greater than 1000, "
            "please correct", __func__);
        return;
    }

    mdelay(delay * 1000);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_DELAY
#include "kinetis/test-kinetis.h"

int t_delay(int argc, char **argv)
{
    u32 time_stamp = 0;

    time_stamp = basic_timer_get_timer_cnt();
    udelay(1000);
    time_stamp = basic_timer_get_timer_cnt() - time_stamp;
    printk(KERN_DEBUG "Delay 1000 us, The result = %u us.", time_stamp);

    time_stamp = basic_timer_get_ms();
    mdelay(1000);
    time_stamp = basic_timer_get_ms() - time_stamp;
    printk(KERN_DEBUG "Delay 1000 ms, The result = %u ms.", time_stamp);

    time_stamp = basic_timer_get_ss();
    sdelay(3);
    time_stamp = basic_timer_get_ss() - time_stamp;
    printk(KERN_DEBUG "Delay 3 s, The result = %u s.", time_stamp);

    return PASS;
}

#endif

