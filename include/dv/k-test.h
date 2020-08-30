#ifndef __K_TEST_H
#define __K_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

typedef enum
{
    PASS = 0U,
    FAIL,
    NOT_EXSIST
} TestStatus;

void k_TestCase_Schedule(void);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __K_TEST_H */
