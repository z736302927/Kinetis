#include "kinetis/button.h"
#include "kinetis/tim-task.h"

#include <linux/slab.h>
#include <linux/errno.h>

#include "string.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Button_Test function, register the corresponding button and Button_Callback callback function.
  * @step 3:  Call function Button_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Button_Test once in function main.
  */

static void button_task_callback(void)
{
    button_ticks();
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
#define DEBOUNCE_TICKS    3 //MAX 8
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
int button_add(u32 unique_id, u8(*pin_level)(void), u8 active_level,
    button_callback callback)
{
    struct button *button;
    u32 i;

    button = kmalloc(sizeof(*button), GFP_KERNEL);

    if (!button)
        return -ENOMEM;

    button->unique_id = unique_id;
    button->event = (u8)NONE_PRESS;
    button->hal_button_level = pin_level;
    button->button_level = button->hal_button_level();
    button->active_level = active_level;

    for (i = 0; i < PRESSEVENT_NBR; i++)
        button->callback[i] = callback;

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
    return (enum press_button_event)(button->event);
}

/**
  * @brief  button driver core function, driver state machine.
  * @param  button: the button button strcut.
  * @retval None
  */
static void button_handler(struct button *button)
{
    u8 read_gpio_level = button->hal_button_level();

    /* Ticks counter working.. */
    if ((button->state) > 0)
        button->ticks++;

    /* button debounce */
    if (read_gpio_level != button->button_level) {
        //not equal to prev one
        //continue read 3 times same new level change
        if (++(button->debounce_cnt) >= DEBOUNCE_TICKS) {
            button->button_level = read_gpio_level;
            button->debounce_cnt = 0;
        }
    } else {
        // leved not change ,counter reset.
        button->debounce_cnt = 0;
    }

    /* State machine */
    switch (button->state) {
        case PRESS_DOWN:
            if (button->button_level == button->active_level) {
                button->event = (u8)PRESS_DOWN;

                if (button->callback[PRESS_DOWN])
                    button->callback[PRESS_DOWN](button);

                button->ticks  = 0;
                button->repeat = 1;
                button->state  = 1;
            } else
                button->event = (u8)NONE_PRESS;

            break;

        case PRESS_UP:
            if (button->button_level != button->active_level) {
                button->event = (u8)PRESS_UP;

                if (button->callback[PRESS_UP])
                    button->callback[PRESS_UP](button);

                button->ticks = 0;
                button->state = 2;

            } else if (button->ticks > LONG_TICKS) {
                button->event = (u8)LONG_RRESS_START;

                if (button->callback[LONG_RRESS_START])
                    button->callback[LONG_RRESS_START](button);

                button->state = 5;
            }

            break;

        case PRESS_REPEAT:
            if (button->button_level == button->active_level) {
                button->event = (u8)PRESS_DOWN;

                if (button->callback[PRESS_DOWN])
                    button->callback[PRESS_DOWN](button);

                button->repeat++;

                button->event = (u8)PRESS_REPEAT;

                if (button->callback[PRESS_REPEAT])
                    button->callback[PRESS_REPEAT](button);

                button->ticks = 0;
                button->state = 3;

            } else if (button->ticks > SHORT_TICKS) {
                if (button->repeat == 1) {
                    button->event = (u8)SINGLE_CLICK;

                    if (button->callback[SINGLE_CLICK])
                        button->callback[SINGLE_CLICK](button);
                } else if (button->repeat == 2) {
                    button->event = (u8)DOUBLE_CLICK;

                    if (button->callback[DOUBLE_CLICK])
                        button->callback[DOUBLE_CLICK](button);
                }

                button->state = 0;
            }

            break;

        case SINGLE_CLICK:
            if (button->button_level != button->active_level) {
                button->event = (u8)PRESS_UP;

                if (button->callback[PRESS_UP])
                    button->callback[PRESS_UP](button);

                if (button->ticks < SHORT_TICKS) {
                    button->ticks = 0;
                    button->state = 2;
                } else
                    button->state = 0;
            }

            break;

        case LONG_RRESS_START:
            if (button->button_level == button->active_level) {
                button->event = (u8)LONG_PRESS_HOLD;

                if (button->callback[LONG_PRESS_HOLD])
                    button->callback[LONG_PRESS_HOLD](button);
            } else {
                button->event = (u8)PRESS_UP;

                if (button->callback[PRESS_UP])
                    button->callback[PRESS_UP](button);

                button->state = 0;
            }

            break;
    }
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void button_ticks(void)
{
    struct button *button;

    if (list_empty(&button_head))
        return;

    list_for_each_entry(button, &button_head, list)
    button_handler(button);
}

#ifdef DESIGN_VERIFICATION_BUTTON
#include "kinetis/test-kinetis.h"
#include "kinetis/hydrology.h"
#include "kinetis/idebug.h"

#include "stm32f4xx_hal.h"

static u8 button_read_pin(void)
{
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);
}

static void button_test_callback(void *button)
{
    u32 btn_event_val;

    btn_event_val = get_button_event(button);

    switch (btn_event_val) {
        case PRESS_DOWN:
//            HydrologyProcessSend(Test);
            printk(KERN_DEBUG "Button press down");
            break;

        case PRESS_UP:
            printk(KERN_DEBUG "Button press up");
            break;

        case PRESS_REPEAT:
            printk(KERN_DEBUG "Button press repeat");
            break;

        case SINGLE_CLICK:
            printk(KERN_DEBUG "Button single click");
            break;

        case DOUBLE_CLICK:
            printk(KERN_DEBUG "Button double click");
            break;

        case LONG_RRESS_START:
            printk(KERN_DEBUG "Button long press start");
            break;

        case LONG_PRESS_HOLD:
            printk(KERN_DEBUG "Button long press hold");
            break;
    }
}

int t_button_add(int argc, char **argv)
{
    button_task_init();

    button_add(1, button_read_pin, 0, button_test_callback);

    printk(KERN_DEBUG "Button test is running, please push the button.");

    return PASS;
}

int t_button_drop(int argc, char **argv)
{
    button_task_exit();

    if (list_empty(&button_head))
        return PASS;

    button_drop(1);

    printk(KERN_DEBUG "Button test is over");

    return PASS;
}

#endif

