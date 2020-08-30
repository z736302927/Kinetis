#ifndef __K_SHELL_H
#define __K_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"


/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void shell_Init(void);
uint8_t shell_GetUserInput(char *pointer);


#ifdef __cplusplus
}
#endif

#endif /* __K_SHELL_H */
