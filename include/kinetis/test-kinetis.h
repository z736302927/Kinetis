#ifndef __K_TEST_KINETIS_H
#define __K_TEST_KINETIS_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

typedef enum {
    PASS = 0U,
    FAIL,
    NOT_EXSIST
} test_status;

void k_test_case_schedule(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_TEST_KINETIS_H */
