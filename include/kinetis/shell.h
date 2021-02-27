#ifndef __K_SHELL_H
#define __K_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void shell_init(void);
bool shell_get_user_input(char *cur_pos);

#ifdef __cplusplus
}
#endif

#endif /* __K_SHELL_H */
