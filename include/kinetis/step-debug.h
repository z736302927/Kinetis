#ifndef __K_STEP_DEBUG_H
#define __K_STEP_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>

extern int debug_step_counter;

// 简单的调试宏
#define DEBUG_STEP() \
    do { \
        printf("[STEP %d | %s:%d in %s] \n", debug_step_counter++, __FILE__, __LINE__, __FUNCTION__); \
    } while(0)

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_STEP_DEBUG_H */
