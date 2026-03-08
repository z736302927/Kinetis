#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/bitmap.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/string.h>

#include "kinetis/button.h"
#include "kinetis/tim-task.h"
#include "kinetis/basic-timer.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Button_Test function, register the corresponding button and Button_Callback callback function.
  * @step 3:  Call function Button_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Button_Test once in function main.
  */

static struct tim_task *button_task;
static bool button_task_running = false;

static void button_task_callback(struct tim_task *task)
{
	button_polling();
}

int button_task_init(void)
{
	int ret;

	if (button_task_running) {
		return 0;
	}

	button_task = tim_task_add("button_task",
			5, true, false, button_task_callback);
	if (!IS_ERR_OR_NULL(button_task)) {
		button_task_running = true;
	}

	return ret;
}

void button_task_exit(void)
{
	if (!button_task_running) {
		return;
	}

	tim_task_drop(button_task);
	button_task_running = false;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

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

	if (!pin_level) {
		return -EINVAL;
	}

	list_for_each_entry(button, &button_head, list) {
		if (button->unique_id == unique_id) {
			pr_warn("button %u already registered\n", unique_id);
			return -EINVAL;
		}
	}

	button = kzalloc(sizeof(*button), GFP_KERNEL);
	if (!button) {
		return -ENOMEM;
	}

	button->unique_id = unique_id;
	button->event = NONE_PRESS;
	button->state = PRESS_IDLE;
	button->valid_ticks = 0;
	button->cnt = 0;
	button->debounce_cnt = 0;
	button->hal_button_level = pin_level;

	if (active_level) {
		bitmap_set(button->button_level, 0, DEBOUNCE_CNT);
	} else {
		bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
	}

	button->active_level = !!active_level;
	button->callback = callback;

	list_add_tail(&button->list, &button_head);

	button->sim_state.unique_id = unique_id;
	button->sim_state.press_time = 0;
	button->sim_state.release_time = 0;
	button->sim_state.active_level = active_level;
	button->sim_state.current_level = !active_level;
	button->sim_state.debounce_counter = 0;
	button->sim_state.click_count = 0;
	button->sim_state.start = true;
	button->sim_state.next_event = NONE_PRESS;
	button->sim_state.next_state = PRESS_IDLE;

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
	u64 current_time = basic_timer_get_ms();

	list_for_each_entry_safe(button, tmp, &button_head, list) {
		if (button->unique_id == unique_id) {
			button->sim_state.start = false;
			while (1) {
				tim_task_loop();

				if (button->state == PRESS_IDLE && button->sim_state.next_state == PRESS_IDLE) {
					break;
				}

				if (basic_timer_get_ms() - current_time >= 10000) {
					pr_warn("button %u timeout waiting for IDLE state\n", unique_id);
					return;
				}
			}

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
static enum button_event get_button_event(struct button *button)
{
	return (button->event);
}

static void click(struct tim_task *tim_task)
{
	struct button *button = container_of(tim_task, struct button, task);

	if (button->cnt >= 2) {
		button->event = DOUBLE_CLICK;
		if (button->callback) {
			button->callback(button);
		}
	} else {
		button->event = SINGLE_CLICK;
		if (button->callback) {
			button->callback(button);
		}
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
	static DECLARE_BITMAP(target_level, DEBOUNCE_CNT);
	u64 current_time;
	u8 old_state = button->state;
	char bitmap_str[16];
	int i;

	bitmap_fill(target_level, DEBOUNCE_CNT);

	/* State machine */
	switch (button->state) {
	case PRESS_IDLE:
		if (gpio_level == button->active_level) {
			button->state = PRESS_DEBOUNCE;
		}
		break;

	case PRESS_DEBOUNCE:
		bitmap_shift_left(button->button_level, button->button_level,
			1, DEBOUNCE_CNT);

		if (gpio_level == button->active_level) {
			bitmap_set(button->button_level, 0, 1);
		} else {
			bitmap_clear(button->button_level, 0, 1);
		}

		for (i = 0; i < DEBOUNCE_CNT; i++) {
			bitmap_str[i] = test_bit(i, button->button_level) ? '1' : '0';
		}
		bitmap_str[DEBOUNCE_CNT] = '\0';
		pr_debug("button_handler: button %u button_level %s\n",
			button->unique_id, bitmap_str);

		if (bitmap_full(button->button_level, DEBOUNCE_CNT)) {
			bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
			button->valid_ticks = basic_timer_get_ms();
			button->state = PRESS_DOWN;
		}
		break;

	case PRESS_DOWN:
		if ((basic_timer_get_ms() - button->valid_ticks) > 3000) {
			button->event = REPEAT_PRESS;
			if (button->callback) {
				button->callback(button);
			}
		}

		if (gpio_level != button->active_level) {
			button->state = RELEASE_DEBOUNCE;
		}
		break;

	case RELEASE_DEBOUNCE:
		bitmap_shift_left(button->button_level, button->button_level,
			1, DEBOUNCE_CNT);

		if (gpio_level != button->active_level) {
			bitmap_set(button->button_level, 0, 1);
		} else {
			bitmap_clear(button->button_level, 0, 1);
		}

		for (i = 0; i < DEBOUNCE_CNT; i++) {
			bitmap_str[i] = test_bit(i, button->button_level) ? '1' : '0';
		}
		bitmap_str[DEBOUNCE_CNT] = '\0';
		pr_debug("button_handler: button %u button_level %s\n",
			button->unique_id, bitmap_str);

		if (bitmap_full(button->button_level, DEBOUNCE_CNT)) {
			bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
			button->valid_ticks = basic_timer_get_ms() - button->valid_ticks;
			pr_debug("button_handler: button %u effective pressing time %llu\n",
				button->unique_id, button->valid_ticks);
			button->state = PRESS_UP;
		}
		break;

	case PRESS_UP:
		if (button->valid_ticks >= 0 && button->valid_ticks <= 1500) {
			button->cnt++;

// 			if (button->cnt == 1) {
// 				tim_task_add(&button->task, NULL,
// 					300, false, false, click);
// 			}
			button->event = SINGLE_CLICK;
			if (button->callback) {
				button->callback(button);
			}
		} else if (button->valid_ticks > 1500 && button->valid_ticks <= 3000) {
			button->event = LONG_PRESS;
			if (button->callback) {
				button->callback(button);
			}
		}

		button->event = NONE_PRESS;
		button->state = PRESS_IDLE;
		button->valid_ticks = 0;
		break;
	}

	if (button->state != old_state) {
		pr_debug("button_handler, button %u state %s\n",
			button->unique_id, button_state_to_string((enum button_state_machine)button->state));
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

	if (list_empty(&button_head)) {
		return;
	}

	list_for_each_entry(button, &button_head, list) {
		button_handler(button);
	}
}

#ifdef DESIGN_VERIFICATION_BUTTON
#include "kinetis/test-kinetis.h"

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#endif

#ifdef KINETIS_FAKE_SIM

/* Simulate button reading with debounce behavior */
static u8 button_sim_read_with_debounce(struct button_sim_state *sim)
{
	u64 current_time = basic_timer_get_ms();
	u32 wait_time, time_diff;
	u8 old_state = sim->next_state;

	/* Handle button state based on next event */
	switch (sim->next_state) {
	case PRESS_IDLE:
		if ((get_random_int() % 100) < 50 && sim->start == true) {
			sim->next_event = get_random_int() % PRESS_EVENT_NBR;
			if (sim->next_event != NONE_PRESS) {
				sim->click_count = 0;
				sim->next_state = PRESS_FIRST_EDGE;
				pr_debug("button_sim, button %u event->%s\n",
					sim->unique_id,
					button_event_to_string(sim->next_event));
			}
		} else {
			sim->next_event = NONE_PRESS;
			sim->next_state = PRESS_IDLE;
		}
		break;

	case PRESS_FIRST_EDGE:
		sim->press_time = basic_timer_get_ms();
		sim->current_level = sim->active_level;
		sim->debounce_counter = 0;
		sim->next_state = PRESS_DEBOUNCE;
		break;

	case PRESS_DEBOUNCE:
		sim->debounce_counter++;

		/* Debounce period: 15ms (3 ticks * 5ms per tick) */
		if (sim->debounce_counter < DEBOUNCE_CNT) {
			/* During debounce, randomly flip bits to simulate bounce */
			if ((get_random_int() % 10) < 3) {  /* 30% chance to flip */
				sim->current_level = !sim->active_level;
			} else {
				sim->current_level = sim->active_level;
			}
		} else {
			sim->current_level = sim->active_level;
			sim->debounce_counter = 0;
			sim->next_state = PRESS_DOWN;
		}
		break;

	case PRESS_DOWN:
		/* Press for 100-200ms, then release */
		if (sim->next_event == SINGLE_CLICK || sim->next_event == DOUBLE_CLICK) {
			wait_time = 150;
		} else {
			wait_time = 2000;
		}

		if (current_time - sim->press_time >= wait_time) {
			sim->current_level = !sim->active_level;
			sim->next_state = RELEASE_DEBOUNCE;
		}
		break;

	case RELEASE_DEBOUNCE:
		sim->debounce_counter++;

		/* Debounce period: 15ms (3 ticks * 5ms per tick) */
		if (sim->debounce_counter < DEBOUNCE_CNT) {
			/* During debounce, randomly flip bits to simulate bounce */
			if ((get_random_int() % 10) < 3) {  /* 30% chance to flip */
				sim->current_level = sim->active_level;
			} else {
				sim->current_level = !sim->active_level;
			}
		} else {
			sim->current_level = !sim->active_level;
			sim->debounce_counter = 0;
			sim->next_state = PRESS_UP;
			sim->click_count++;
		}
		break;

	case PRESS_UP:
		if (sim->next_event == DOUBLE_CLICK) {
			time_diff = current_time - sim->press_time;
			if (time_diff >= 200) {
				if (sim->click_count < 2) {
					sim->next_state = PRESS_FIRST_EDGE;
				} else {
					sim->next_state = PRESS_IDLE;
				}
			}
		} else {
			sim->next_state = PRESS_IDLE;
		}
		break;

	default:
		break;
	}

	if (sim->next_state != old_state) {
		pr_debug("button_sim, button %u state->%s",
			sim->unique_id, button_state_to_string(sim->next_state));
	}

	return sim->current_level;
}
#endif /* KINETIS_FAKE_SIM */

static u8 button_read_pin(struct button *button)
{
#if KINETIS_FAKE_SIM
	return button_sim_read_with_debounce(&button->sim_state);
#endif

	switch (button->unique_id) {
	case 1:
#if MCU_PLATFORM_STM32
		return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);
#else
		return 0;
#endif

	case 2:
#if MCU_PLATFORM_STM32
		return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);
#else
		return 0;
#endif

	case 3:
#if MCU_PLATFORM_STM32
		return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);
#else
		return 0;
#endif

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
		pr_debug("button[%d] single click\n",
			button->unique_id);
		break;

	case DOUBLE_CLICK:
		pr_debug("button[%d] double click\n",
			button->unique_id);
		break;

	case LONG_PRESS:
		pr_debug("button[%d] long press\n",
			button->unique_id);
		break;

	case REPEAT_PRESS:
		pr_debug("button[%d] press repeat\n",
			button->unique_id);
		break;
	}
}

int t_button_task(int argc, char **argv)
{
	if (argc < 2) {
		return -EINVAL;
	}

	if (strcmp(argv[1], "on") == 0) {
		return button_task_init();
	} else if (strcmp(argv[1], "off") == 0) {
		button_task_exit();
		return 0;
	}

	pr_debug("usage: t_button_task <on|off>\n");
	return -EINVAL;
}

int t_button_add(int argc, char **argv)
{
	button_add(1, button_read_pin, 0, button_test_callback);
	button_add(2, button_read_pin, 0, button_test_callback);
	button_add(3, button_read_pin, 0, button_test_callback);

	pr_debug("button test is running, please push the button.\n");

	return PASS;
}

int t_button_drop(int argc, char **argv)
{
	button_drop(1);
	button_drop(2);
	button_drop(3);

	pr_debug("button test is over\n");

	return PASS;
}

#endif
