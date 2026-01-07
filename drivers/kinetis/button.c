#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/bitmap.h>
#include <linux/kernel.h>
#include <linux/random.h>

#include "kinetis/button.h"
#include "kinetis/tim-task.h"
#include "kinetis/basic-timer.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify Multi_Button_Test function, register the corresponding button and Button_Callback callback function.
  * @step 3:  Call function Button_Ticks periodically for 5 ms.
  * @step 4:  Call function Multi_Button_Test once in function main.
  */

static struct tim_task button_task;

static void button_task_callback(struct tim_task *task)
{
	button_polling();
}

int button_task_init(void)
{
	return tim_task_add(&button_task, "button_task",
			5, true, false, button_task_callback);
}

void button_task_exit(void)
{
	tim_task_drop(&button_task);
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

	if (!button) {
		return -ENOMEM;
	}

	button->unique_id = unique_id;
	button->event = (u8)NONE_PRESS;
	button->hal_button_level = pin_level;

	if (active_level) {
		bitmap_set(button->button_level, 0, DEBOUNCE_CNT);
	} else {
		bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
	}

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

		if (button->callback) {
			button->callback(button);
		}
	} else {
		button->event = (u8)SINGLE_CLICK;

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
	static DECLARE_BITMAP(target_level, DEBOUNCE_CNT) = {~0};

	/* State machine */
	switch (button->state) {
	case NONE_PRESS:
		if (gpio_level == button->active_level) {
			button->state = PRESS_DEBOUNCE;
		}

		break;

	case PRESS_DEBOUNCE:
		/* press down debounce */
		bitmap_shift_left(button->button_level, button->button_level,
			1, DEBOUNCE_CNT);

		if (gpio_level == button->active_level) {
			bitmap_set(button->button_level, 0, 1);
		} else {
			bitmap_clear(button->button_level, 0, 1);
		}

		if (bitmap_equal(target_level, button->button_level, DEBOUNCE_CNT)) {
			bitmap_clear(button->button_level, 0, DEBOUNCE_CNT);
			button->valid_ticks = basic_timer_get_ms();
			button->state = PRESS_DOWN;
		}

		break;

	case PRESS_DOWN:
		if ((basic_timer_get_ms() - button->valid_ticks) > 3000) {
			button->event = (u8)PRESS_REPEAT;

			if (button->callback) {
				button->callback(button);
			}
		}

		/* press up debounce */
		bitmap_shift_left(button->button_level, button->button_level,
			1, DEBOUNCE_CNT);

		if (gpio_level != button->active_level) {
			bitmap_set(button->button_level, 0, 1);
		} else {
			bitmap_clear(button->button_level, 0, 1);
		}

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
				tim_task_add(&button->task, "button_task",
					300, true, false, click);
		} else if (button->valid_ticks > 1500 && button->valid_ticks <= 3000) {
			button->event = (u8)LONG_RRESS;

			if (button->callback) {
				button->callback(button);
			}
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

	if (list_empty(&button_head)) {
		return;
	}

	list_for_each_entry(button, &button_head, list) {
		button_handler(button);
	}
}

#ifdef DESIGN_VERIFICATION_BUTTON
#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

#include "hydrology.h"

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#endif

#ifdef KINETIS_FAKE_SIM
/* Button simulation state for fake mode */
struct button_sim_state {
	u64 press_start_time;    /* When button was pressed */
	u64 release_time;        /* When button was released */
	u8 current_level;        /* Current GPIO level (0=low, 1=high) */
	u8 debounce_counter;      /* Debounce counter */
	enum press_button_event next_event;  /* Next event to generate */
	bool in_debounce;        /* Currently in debounce state */
};

static struct button_sim_state button_sim[4] = {0};

/* Initialize button simulation state */
static void button_sim_init(u32 unique_id)
{
	if (unique_id < 4) {
		button_sim[unique_id].press_start_time = 0;
		button_sim[unique_id].release_time = 0;
		button_sim[unique_id].current_level = 1;  /* Default high (not pressed) */
		button_sim[unique_id].debounce_counter = 0;
		button_sim[unique_id].next_event = NONE_PRESS;
		button_sim[unique_id].in_debounce = false;
	}
}

/* Simulate button press with debounce behavior */
static void button_sim_press(u32 unique_id)
{
	if (unique_id < 4) {
		button_sim[unique_id].press_start_time = basic_timer_get_ms();
		button_sim[unique_id].current_level = 0;  /* Press = low */
		button_sim[unique_id].debounce_counter = 0;
		button_sim[unique_id].in_debounce = true;
		button_sim[unique_id].next_event = NONE_PRESS;
	}
}

/* Simulate button release with debounce behavior */
static void button_sim_release(u32 unique_id)
{
	if (unique_id < 4) {
		button_sim[unique_id].release_time = basic_timer_get_ms();
		button_sim[unique_id].current_level = 1;  /* Release = high */
		button_sim[unique_id].debounce_counter = 0;
		button_sim[unique_id].in_debounce = true;
	}
}

/* Generate random button event */
static enum press_button_event button_sim_random_event(void)
{
	u8 rand_val = get_random_int() % 10;

	if (rand_val < 5) {
		return SINGLE_CLICK;      /* 50% chance */
	} else if (rand_val < 8) {
		return DOUBLE_CLICK;      /* 30% chance */
	} else if (rand_val < 9) {
		return LONG_RRESS;        /* 10% chance */
	} else {
		return PRESS_REPEAT;      /* 10% chance */
	}
}

/* Simulate button reading with debounce behavior */
static u8 button_sim_read_with_debounce(u32 unique_id)
{
	struct button_sim_state *sim = &button_sim[unique_id];
	u64 current_time = basic_timer_get_ms();
	u8 simulated_level;

	/* Auto-generate button events randomly */
	if (sim->current_level == 1 && sim->next_event == NONE_PRESS) {
		/* Button is released, randomly press it */
		if ((get_random_int() % 100) < 5) {  /* 5% chance per call */
			enum press_button_event event = button_sim_random_event();
			sim->next_event = event;
			button_sim_press(unique_id);
		}
	}

	/* Handle button state based on next event */
	switch (sim->next_event) {
	case SINGLE_CLICK:
		/* Press for 100-200ms, then release */
		if (current_time - sim->press_start_time > 150) {
			button_sim_release(unique_id);
		}
		break;

	case DOUBLE_CLICK:
		/* First press: 100ms, release, 50ms, press again 100ms, release */
		if (current_time - sim->press_start_time > 100 &&
			current_time - sim->press_start_time < 150) {
			button_sim_release(unique_id);
		} else if (current_time - sim->press_start_time > 150 &&
			current_time - sim->press_start_time < 200) {
			button_sim_press(unique_id);
		} else if (current_time - sim->press_start_time > 250) {
			button_sim_release(unique_id);
		}
		break;

	case LONG_RRESS:
		/* Press for 2000-3000ms, then release */
		if (current_time - sim->press_start_time > 2500) {
			button_sim_release(unique_id);
		}
		break;

	case PRESS_REPEAT:
		/* Press for 3500ms, then release */
		if (current_time - sim->press_start_time > 3500) {
			button_sim_release(unique_id);
		}
		break;

	default:
		break;
	}

	/* Simulate debounce behavior */
	if (sim->in_debounce) {
		sim->debounce_counter++;

		/* Debounce period: 15ms (3 ticks * 5ms per tick) */
		if (sim->debounce_counter < 3) {
			/* During debounce, randomly flip bits to simulate bounce */
			if ((get_random_int() % 10) < 3) {  /* 30% chance to flip */
				simulated_level = !sim->current_level;
			} else {
				simulated_level = sim->current_level;
			}
		} else {
			/* Debounce complete */
			sim->in_debounce = false;
			simulated_level = sim->current_level;

			/* Reset next_event after release */
			if (simulated_level == 1 && sim->next_event != NONE_PRESS) {
				sim->next_event = NONE_PRESS;
			}
		}
	} else {
		simulated_level = sim->current_level;
	}

	return simulated_level;
}
#endif /* KINETIS_FAKE_SIM */

static u8 button_read_pin(struct button *button)
{
	switch (button->unique_id) {
	case 1:
#if MCU_PLATFORM_STM32
		return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);
#elif KINETIS_FAKE_SIM
		return button_sim_read_with_debounce(1);
#else
		return 0;
#endif

	case 2:
#if MCU_PLATFORM_STM32
		return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);
#elif KINETIS_FAKE_SIM
		return button_sim_read_with_debounce(2);
#else
		return 0;
#endif

	case 3:
#if MCU_PLATFORM_STM32
		return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);
#elif KINETIS_FAKE_SIM
		return button_sim_read_with_debounce(3);
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
		pr_debug("Button[%d] single click\n",
			button->unique_id);
		break;

	case DOUBLE_CLICK:
		pr_debug("Button[%d] double click\n",
			button->unique_id);
		break;

	case LONG_RRESS:
		pr_debug("Button[%d] long press\n",
			button->unique_id);
		break;

	case PRESS_REPEAT:
		pr_debug("Button[%d] press repeat\n",
			button->unique_id);
		break;
	}
}

int t_button_add(int argc, char **argv)
{
#ifdef KINETIS_FAKE_SIM
	/* Initialize button simulation states */
	button_sim_init(1);
	button_sim_init(2);
	button_sim_init(3);
#endif

	button_task_init();

	button_add(1, button_read_pin, 0, button_test_callback);
	button_add(2, button_read_pin, 0, button_test_callback);
	button_add(3, button_read_pin, 0, button_test_callback);

	pr_debug("Button test is running, please push the button.\n");

	return PASS;
}

int t_button_drop(int argc, char **argv)
{
	button_task_exit();

	button_drop(1);
	button_drop(2);
	button_drop(3);

	pr_debug("Button test is over\n");

	return PASS;
}

#endif
