#include "kinetis/delay.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/random-gene.h"

#include <linux/printk.h>
#include <linux/jiffies.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


//#define DELAY_USING_HARDWARE

#ifdef DELAY_USING_HARDWARE

#include "tim.h"

struct delay_para {
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
            "Inputing clock is too large, please modify the delay unit.\n");

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
void udelay(u64 usecs)
{
    u64 timeout = basic_timer_get_us() + usecs;

    while (time_after64(basic_timer_get_us(), timeout));
}

/*
 * The latency ranges from 0 to (2^32 / 1000), but this leads to an increase in the number
 * of iterations and eventually causes a stack overflow.Therefore, try to enter
 * smaller parameters.
 */
void msleep(unsigned long msecs)
{
    unsigned long timeout = jiffies + msecs;

    while (time_after(jiffies, timeout));
}

/**
 * usleep_range - Sleep for an approximate time
 * @min: Minimum time in usecs to sleep
 * @max: Maximum time in usecs to sleep
 *
 */
void usleep_range(unsigned long min, unsigned long max)
{
	u32 delta = get_random_range(min, max);

	udelay(delta);
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_DELAY
#include "kinetis/test-kinetis.h"

#include <linux/delay.h>

int t_delay(int argc, char **argv)
{
    u32 time_stamp = 0;

    time_stamp = basic_timer_get_us();
    udelay(1000);
    time_stamp = basic_timer_get_us() - time_stamp;
    printk(KERN_DEBUG "Delay 1000 us, The result = %u us.\n", time_stamp);

    time_stamp = basic_timer_get_ms();
    mdelay(1000);
    time_stamp = basic_timer_get_ms() - time_stamp;
    printk(KERN_DEBUG "Delay 1000 ms, The result = %u ms.\n", time_stamp);

    time_stamp = basic_timer_get_ss();
    ssleep(3);
    time_stamp = basic_timer_get_ss() - time_stamp;
    printk(KERN_DEBUG "Delay 3 s, The result = %u s.\n", time_stamp);

    return PASS;
}

#endif

