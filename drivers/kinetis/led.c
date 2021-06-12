#include "kinetis/led.h"

#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/errno.h>

//LED Handle list head.
static LIST_HEAD(led_head);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#include "stm32f4xx_hal.h"

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/LEDn_Type/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

static inline void led_init(struct led *led)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Enable the GPIO_LED Clock */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure the GPIO_LED pin */
    GPIO_InitStruct.Pin = led->num;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(led->group, &GPIO_InitStruct);

    HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_RESET);
}

static inline void led_set_status(struct led *led, enum led_status status)
{
    switch (status) {
        case ON:
            HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_SET);
            break;

        case OFF:
            HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_RESET);
            break;

        case TOGGLE:
            HAL_GPIO_TogglePin(led->group, led->num);
            break;

        default:
            ;
    }
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @brief  Start the LED work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int led_add(u32 unique_id, enum led_color color, void *group, u32 num)
{
    struct led *led;

    led = kmalloc(sizeof(*led), GFP_KERNEL);

    if (!led)
        return -ENOMEM;

    led->unique_id = unique_id;
    led->color = color;
    led->group = group;
    led->num = num;
    led_init(led);

    list_add_tail(&led->list, &led_head);

    return 0;
}

/**
  * @brief  Stop the LED work, remove the handle off work list.
  * @param  Handle: target handle strcut.
  * @retval None
  */
void led_drop(u32 unique_id)
{
    struct led *led, *tmp;

    list_for_each_entry_safe(led, tmp, &led_head, list) {
        if (led->unique_id == unique_id) {
            list_del(&led->list);
            kfree(led);
            break;
        }
    }
}

/**
  * @brief  Turns selected LED On.
  * @param  LED: Specifies the LED to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void led_operation(u32 unique_id, enum led_status status)
{
    struct led *led;

    list_for_each_entry(led, &led_head, list) {
        if (led->unique_id == unique_id) {
            led_set_status(led, status);
            break;
        }
    }
}

#ifdef DESIGN_VERIFICATION_LED
#include "kinetis/test-kinetis.h"
#include "kinetis/tim-task.h"

static void led_task_callback(void)
{
    led_operation(1, TOGGLE);
    led_operation(2, TOGGLE);
}

int t_led_add(int argc, char **argv)
{
    int ret;

    ret = led_add(1, RED, GPIOC, GPIO_PIN_4);

    if (ret)
        return FAIL;

    ret = led_add(2, RED, GPIOC, GPIO_PIN_5);

    if (ret)
        return FAIL;

    ret = tim_task_add(1000, true, led_task_callback);

    if (ret)
        return FAIL;

    return PASS;
}

int t_led_drop(int argc, char **argv)
{
    tim_task_drop(led_task_callback);

    led_drop(1);
    led_drop(2);

    return PASS;
}
#endif
