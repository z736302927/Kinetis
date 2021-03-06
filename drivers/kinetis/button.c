#include "kinetis/button.h"
#include "kinetis/tim-task.h"
#include "kinetis/basic-timer.h"

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/bitmap.h>
#include <linux/kernel.h>

#include "string.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Button_Test function, register the corresponding button and Button_Callback callback function.
  * @step 3:  Call function Button_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Button_Test once in function main.
  */

static void button_task_callback()
{
    button_polling();
}

int button_task_init(void)
{
    return tim_task_add(5, true, button_task_callback);
}

void button_task_exit(void)
{
    tim_task_drop(button_task_callback);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* According to your need to modify the constants. */
#define TICKS_INTERVAL    5 //ms
#define DEBOUNCE_CNT      3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

static LIST_HEAD(button_head);

/**
  * @brief  Initializes the button struct button.
  * @param  button: the button struct.
  * @param  pin_level: read the pin of the connet button level.
  * @param  active_level: pin pressed level.
  * @retval None
  */
int button_add(u32 unique_id, u8(*pin_level)(struct button *), u8 active_level,
    button_callback callback)
{
    struct button *button;
    u32 i;
    int ret;

    button = kzalloc(sizeof(*button), GFP_KERNEL);

    if (!button)
        return -ENOMEM;

    button->unique_id = unique_id;
    button->event = (u8)NONE_PRESS;
    button->hal_button_level = pin_level;

    if (active_level)
        bitmap_set(button->button_level, 0, DEBOUNCE_CNT);
    else
        bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);

    button->active_level = active_level;
    button->callback = callback;

    list_add_tail(&button->list, &button_head);

    return 0;
}

/**
  * @brief  Deinitializes the button struct button.
  * @param  button: the button button strcut.
  * @retval None
  */
void button_drop(u32 unique_id)
{
    struct button *button, *tmp;

    list_for_each_entry_safe(button, tmp, &button_head, list) {
        if (button->unique_id == unique_id) {
            list_del(&button->list);
            kfree(button);
            break;
        }
    }
}

/**
  * @brief  Inquire the button event happen.
  * @param  button: the button button strcut.
  * @retval button event.
  */
static enum press_button_event get_button_event(struct button *button)
{
    return (button->event);
}

static void click(struct tim_task *tim_task)
{
    struct button *button = container_of(tim_task, struct button, task);

    if (button->cnt >= 2) {
        button->event = (u8)DOUBLE_CLICK;

        if (button->callback)
            button->callback(button);
    } else {
        button->event = (u8)SINGLE_CLICK;

        if (button->callback)
            button->callback(button);
    }

    button->cnt = 0;
}
/**
  * @brief  button driver core function, driver state machine.
  * @param  button: the button button strcut.
  * @retval None
  */
static void button_handler(struct button *button)
{
    u8 gpio_level = button->hal_button_level(button);
    static DECLARE_BITMAP(target_level, DEBOUNCE_CNT) = {~0};

    /* State machine */
    switch (button->state) {
        case NONE_PRESS:
            if (gpio_level == button->active_level)
                button->state = PRESS_DEBOUNCE;

            break;

        case PRESS_DEBOUNCE:
            /* press down debounce */
            bitmap_shift_left(button->button_level, button->button_level,
                1, DEBOUNCE_CNT);

            if (gpio_level == button->active_level)
                bitmap_set(button->button_level, 0, 1);
            else
                bitmap_clear(button->button_level, 0, 1);

            if (bitmap_equal(target_level, button->button_level, DEBOUNCE_CNT)) {
                bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
                button->valid_ticks = basic_timer_get_ms();
                button->state = PRESS_DOWN;
            }

            break;

        case PRESS_DOWN:
            if ((basic_timer_get_ms() - button->valid_ticks) > 3000) {
                button->event = (u8)PRESS_REPEAT;

                if (button->callback)
                    button->callback(button);
            }

            /* press up debounce */
            bitmap_shift_left(button->button_level, button->button_level,
                1, DEBOUNCE_CNT);

            if (gpio_level != button->active_level)
                bitmap_set(button->button_level, 0, 1);
            else
                bitmap_clear(button->button_level, 0, 1);

            if (bitmap_equal(target_level, button->button_level, DEBOUNCE_CNT)) {
                bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
                button->valid_ticks = basic_timer_get_ms() - button->valid_ticks;
                button->state = PRESS_UP;
            }

            break;

        case PRESS_UP:
            if (button->valid_ticks >= 0 && button->valid_ticks <= 1500) {
                button->cnt++;

                if (button->cnt == 1)
                    tim_task_enqueue(&button->task, 300, click);
            } else if (button->valid_ticks > 1500 && button->valid_ticks <= 3000) {
                button->event = (u8)LONG_RRESS;

                if (button->callback)
                    button->callback(button);
            }

            button->state = NONE_PRESS;
            button->valid_ticks = 0;
            break;
    }
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void button_polling(void)
{
    struct button *button;

    if (list_empty(&button_head))
        return;

    list_for_each_entry(button, &button_head, list) {
        button_handler(button);
    }
}

#ifdef DESIGN_VERIFICATION_BUTTON
#include "kinetis/test-kinetis.h"
#include "kinetis/hydrology.h"
#include "kinetis/idebug.h"

#include "stm32f4xx_hal.h"

static u8 button_read_pin(struct button *button)
{
    switch (button->unique_id) {
        case 1:
            return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);

        case 2:
            return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);

        case 3:
            return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);

        default:
            return button->active_level ? 0 : 1;
    }
}

static void button_test_callback(struct button *button)
{
    u32 btn_event_val;

    btn_event_val = get_button_event(button);

    switch (btn_event_val) {
        case SINGLE_CLICK:
            printk(KERN_DEBUG "Button[%d] single click\n",
                button->unique_id);
            break;

        case DOUBLE_CLICK:
            printk(KERN_DEBUG "Button[%d] double click\n",
                button->unique_id);
            break;

        case LONG_RRESS:
            printk(KERN_DEBUG "Button[%d] long press\n",
                button->unique_id);
            break;

        case PRESS_REPEAT:
            printk(KERN_DEBUG "Button[%d] press repeat\n",
                button->unique_id);
            break;
    }
}

int t_button_add(int argc, char **argv)
{
    button_task_init();

    button_add(1, button_read_pin, 0, button_test_callback);
    button_add(2, button_read_pin, 0, button_test_callback);
    button_add(3, button_read_pin, 0, button_test_callback);

    printk(KERN_DEBUG "Button test is running, please push the button.\n");

    return PASS;
}

int t_button_drop(int argc, char **argv)
{
    button_task_exit();

    button_drop(1);
    button_drop(2);
    button_drop(3);

    printk(KERN_DEBUG "Button test is over\n");

    return PASS;
}

#endif

