#include <linux/slab.h>
#include <linux/errno.h>

#include "kinetis/switch.h"
#include "kinetis/tim-task.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Switch_Test function, register the corresponding Switch and Switch_Callback callback function.
  * @step 3:  Call function Switch_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Switch_Test once in function main.
  */

static struct tim_task switch_task;

static void switch_task_callback(struct tim_task *task)
{
    switch_ticks();
}

int switch_task_init(void)
{
	return tim_task_add(&switch_task, "switch task",
			5, true, false, switch_task_callback);
}

void switch_task_exit(void)
{
    tim_task_drop(&switch_task);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

//According to your need to modify the constants.
#define TICKS_INTERVAL    20 //ms
#define DEBOUNCE_TICKS    3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

static LIST_HEAD(switch_head);

/**
  * @brief  Initializes the switch struct _switch.
  * @param  switch: the switch struct.
  * @param  pin_level: read the pin of the connet switch level.
  * @param  active_level: pin pressed level.
  * @retval None
  */
int switch_add(u32 unique_id, u8(*pin_level)(void), u8 active_level,
    switch_callback callback)
{
    struct _switch *_switch;
    u32 i;

    _switch = kzalloc(sizeof(*_switch), GFP_KERNEL);

    if (!_switch)
        return -ENOMEM;

    _switch->unique_id = unique_id;
    _switch->event = (u8)NONE_SWITCH;
    _switch->hal_switch_level = pin_level;
    _switch->switch_level = _switch->hal_switch_level();
    _switch->active_level = active_level;

    for (i = 0; i < SWITCHEVENT_NBR; i++)
        _switch->callback[i] = callback;

    list_add_tail(&_switch->list, &switch_head);

    return 0;
}

/**
  * @brief  Deinitializes the switch struct _switch.
  * @param  switch: the switch switch strcut.
  * @retval None
  */
void switch_drop(u32 unique_id)
{
    struct _switch *_switch, *tmp;

    list_for_each_entry_safe(_switch, tmp, &switch_head, list) {
        if (_switch->unique_id == unique_id) {
            list_del(&_switch->list);
            kfree(_switch);
            break;
        }
    }
}

/**
  * @brief  Inquire the switch event happen.
  * @param  switch: the switch switch strcut.
  * @retval switch event.
  */
static enum switch_event get_switch_event(struct _switch *_switch)
{
    return (enum switch_event)(_switch->event);
}


/**
  * @brief  Switch driver core function, driver state machine.
  * @param  switch: the Switch switch strcut.
  * @retval None
  */
static void switch_handler(struct _switch *_switch)
{
    u8 read_gpio_level = _switch->hal_switch_level();

    /* Switch debounce switch */
    if (read_gpio_level != _switch->switch_level) {
        //not equal to prev one
        //continue read 3 times same new level change
        if (++(_switch->debounce_cnt) >= DEBOUNCE_TICKS) {
            _switch->switch_level = read_gpio_level;
            _switch->debounce_cnt = 0;
        }
    } else {
        // leved not change ,counter reset.
        _switch->debounce_cnt = 0;
    }

    /* State machine */
    switch (_switch->state) {
        case SWITCH_DOWN:
            if (_switch->switch_level == _switch->active_level) {
                _switch->event = (u8)SWITCH_DOWN;

                if (_switch->callback[SWITCH_DOWN])
                    _switch->callback[SWITCH_DOWN](_switch);

                _switch->state  = 1;
            } else
                _switch->event = (u8)NONE_SWITCH;

            break;

        case SWITCH_UP:
            if (_switch->switch_level != _switch->active_level) {
                _switch->event = (u8)SWITCH_UP;

                if (_switch->callback[SWITCH_UP])
                    _switch->callback[SWITCH_UP](_switch);

                _switch->state = 0;
            }

            break;
    }
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void switch_ticks(void)
{
    struct _switch *_switch;

    if (list_empty(&switch_head))
        return;

    list_for_each_entry(_switch, &switch_head, list) {
        switch_handler(_switch);
    }
}

#ifdef DESIGN_VERIFICATION_SWITCH
#include "kinetis/test-kinetis.h"
#include "kinetis/tim-task.h"
#include "kinetis/idebug.h"

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#else
#endif

static u8 switch_read_pin(void)
{
#if MCU_PLATFORM_STM32
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);
#else
    /* 模拟仿真：模拟开关状态切换，用于测试 */
    static u32 counter = 0;
    static u8 current_state = 0;
    counter++;
    /* 每100个tick周期切换一次开关状态（开/关） */
    if (counter % 100 == 0) {
        current_state = !current_state;
    }
    return current_state;
#endif
}

static void switch_test_callback(void *_switch)
{
    u32 switch_event_val;

    switch_event_val = get_switch_event((struct _switch *)_switch);

    switch (switch_event_val) {
        case SWITCH_DOWN:
            printk("Switch press down");
            break;

        case SWITCH_UP:
            printk("Switch press up");
            break;
    }
}

int t_switch_add(int argc, char **argv)
{
    switch_task_init();

    switch_add(1, switch_read_pin, 0, switch_test_callback);

    pr_debug("Button test is running, please push the switch.");

    return PASS;
}

int t_switch_drop(int argc, char **argv)
{
    switch_task_exit();

    if (list_empty(&switch_head))
        return PASS;

    switch_drop(1);

    pr_debug("Button test is over");

    return PASS;
}

#endif
