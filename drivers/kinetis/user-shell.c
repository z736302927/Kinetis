#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/printk.h>

#include "kinetis/user-shell.h"

#include <stdio.h>
#ifdef KINETIS_FAKE_SIM
#include <pthread.h>
#include <unistd.h>

static pthread_t input_thread;
#endif

/* Input line buffer for storing complete user input */
static char user_input_buffer[SHELL_INPUT_BUFFER_SIZE];
static u16 user_input_length = 0;
static bool user_input_ready = false;

/* The following program is modified by the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

void *pthread_shell_input(void *para)
{
	while (1) {
		if (fgets(user_input_buffer, SHELL_INPUT_BUFFER_SIZE, stdin) != NULL) {
			// Only process if previous input has been consumed
			if (user_input_ready == false) {
				// Calculate input length
				user_input_length = strlen(user_input_buffer);

				// Remove trailing newline and carriage return if present
				if (user_input_length > 0) {
					if (user_input_buffer[user_input_length - 1] == '\n' ||
					    user_input_buffer[user_input_length - 1] == '\r') {
						user_input_buffer[user_input_length - 1] = '\0';
						user_input_length--;
					}
				}

				// Mark input as ready
				user_input_ready = true;
			}
		}

#ifdef KINETIS_FAKE_SIM
		usleep(1000);
#endif
	}

	return NULL;
}

void shell_init_async(void)
{
	// Initialize input line buffer
	memset(user_input_buffer, 0, sizeof(user_input_buffer));
	user_input_length = 0;
	user_input_ready = false;

#ifdef KINETIS_FAKE_SIM
	// Create input thread to continuously read from stdin
	pthread_create(&input_thread, NULL, pthread_shell_input, NULL);
#endif
}

/**
 * @brief Get user input string (non-blocking)
 * @param buffer Buffer to store the input string
 * @param max_len Maximum length of the buffer
 * @return 0 on success, -ETIMEDOUT if no input, -1 on failure
 *
 * Uses line-based approach - waits for complete line ending with newline
 */
int get_user_input_string(char *buffer, int max_len)
{
	int copy_len;
	bool has_input = false;

	if (buffer == NULL || max_len <= 0) {
		return -EINVAL;
	}

	// Check if there's any input ready
	if (user_input_ready == true) {
		has_input = true;

		if (user_input_length >= max_len) {
			pr_warn("Input buffer is too small: input_len=%d, buffer_size=%d\n",
				user_input_length, max_len);
		}

		// Copy the complete line to user buffer
		copy_len = min(user_input_length, max_len - 1);
		strncpy(buffer, user_input_buffer, copy_len);
		buffer[copy_len] = '\0';

		// Reset the input line buffer for next input
		memset(user_input_buffer, 0, sizeof(user_input_buffer));
		user_input_length = 0;
		user_input_ready = false;
	}

	if (has_input == true) {
		return 0;
	} else {
		// No input ready - return immediately without blocking
		buffer[0] = '\0';
		return -ETIMEDOUT;
	}
}
