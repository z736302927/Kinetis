#ifndef __USER_SHELL_H
#define __USER_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Exported functions --------------------------------------------------------*/
int get_user_input_string(char *buffer, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* __USER_SHELL_H */
