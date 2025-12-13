#include <linux/errno.h>

#include "kinetis/user-shell.h"


#include <stdio.h>

/* The following program is modified by the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/**
 * @brief Get user input string with backspace support
 * @param buffer Buffer to store the input string
 * @param max_len Maximum length of the buffer
 * @return 0 on success, -1 on failure
 *
 * Uses getchar() to read characters one by one, supporting backspace (0x08 or 0x7F)
 * Press Enter to complete input, press Backspace to delete characters
 */
int get_user_input_string(char *buffer, int max_len)
{
	char ch;
	int pos = 0;

	if (buffer == NULL || max_len <= 0)
		return -ENOMEM;

	// Read characters one by one
	while (1) {
		ch = getchar();

        // Check for Enter key (CR or LF)
		if (ch == '\r' || ch == '\n') {
			buffer[pos] = '\0';  // Null terminate the string
			return 0;
		}

		// Check for backspace (ASCII 8 or 127)
		else if (ch == 0x08 || ch == 0x7F) {
			if (pos > 0) {
				pos--;
				printf("\b \b");  // Move cursor back, print space, move back again
			}
		}

		// Regular character
		else if (pos < max_len - 1 && ch >= 32 && ch <= 126) {
			buffer[pos] = ch;
			// Character will be echoed by terminal automatically
			pos++;
		}
	}
}




