#include <linux/slab.h>
#include <linux/errno.h>

#include "kinetis/led.h"
#include "kinetis/design_verification.h"

//LED Handle list head.
static LIST_HEAD(led_head);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/LEDn_Type/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

int led_add(u32 unique_id, void (*config_color)(u8 red, u8 green, u8 blue), void *group)
{
	struct led_device *led;

	if (!config_color) {
		return -EINVAL;
	}

	list_for_each_entry(led, &led_head, list) {
		if (led->unique_id == unique_id) {
			return -EINVAL;
		}
	}

	led = kzalloc(sizeof(*led), GFP_KERNEL);
	if (!led) {
		return -ENOMEM;
	}

	led->unique_id = unique_id;
	led->color = LED_COLOR_NONE;
	led->rgb.red = 0;
	led->rgb.green = 0;
	led->rgb.blue = 0;
	led->group = group;
	led->config_color = config_color;

	list_add_tail(&led->list, &led_head);

	return 0;
}

void led_drop(u32 unique_id)
{
	struct led_device *led, *tmp;

	list_for_each_entry_safe(led, tmp, &led_head, list) {
		if (led->unique_id == unique_id) {
			list_del(&led->list);
			kfree(led);
			break;
		}
	}
}

int led_config_color(u32 unique_id, enum led_color color)
{
	struct led_device *led;
	u8 red, green, blue;

	switch (color) {
	case LED_COLOR_NONE:
		red = 0;
		green = 0;
		blue = 0;
		break;
	case LED_COLOR_RED:
		red = 255;
		green = 0;
		blue = 0;
		break;
	case LED_COLOR_GREEN:
		red = 0;
		green = 255;
		blue = 0;
		break;
	case LED_COLOR_BLUE:
		red = 0;
		green = 0;
		blue = 255;
		break;
	default:
		return -EINVAL;
	}

	list_for_each_entry(led, &led_head, list) {
		if (led->unique_id == unique_id) {
			if (!led->config_color) {
				return -EINVAL;
			}
			led->color = color;
			led->rgb.red = red;
			led->rgb.green = green;
			led->rgb.blue = blue;
			led->config_color(red, green, blue);
			return 0;
		}
	}

	return -ENODEV;
}

#ifdef DESIGN_VERIFICATION_LED
#include "kinetis/test-kinetis.h"
#include "kinetis/tim-task.h"

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"

static void led_hal_set(u8 red, u8 green, u8 blue)
{
	if (red == 255) {
		HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_SET);
	} else if (red == 0) {
		HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_RESET);
	}
}
#elif KINETIS_FAKE_SIM

static void led_hal_set(u8 red, u8 green, u8 blue)
{
	pr_info("LED color configured to R:%d G:%d B:%d", red, green, blue);
}
#else
#endif

int t_led_add(int argc, char **argv)
{
	int ret;

	ret = led_add(1, led_hal_set, NULL);
	if (ret) {
		return ret;
	}

	led_config_color(1, LED_COLOR_RED);

	pr_debug("led test is running.");

	return 0;
}

int t_led_drop(int argc, char **argv)
{
	if (list_empty(&led_head)) {
		return 0;
	}

	led_drop(1);

	pr_debug("led test is over");

	return 0;
}

#endif
