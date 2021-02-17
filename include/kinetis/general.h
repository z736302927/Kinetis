#ifndef __K_GENERAL_H
#define __K_GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "kinetis/serial-port.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

enum at_cmd_property {
    AT_NONE,
    AT_TEST,
    AT_READ,
    AT_SET,
    AT_EXCUTE
};

struct general_cmd {
    char *at_cmd;
    char *argu;
    char *expect_res;
    enum at_cmd_property property;
    u8 error_repetition;
    u8 current_repetition;
    u8 error_flag;
    u8 timeout_flag;
    u16 wait_time;
    u16 interval;
    char *delimiter;
    char **argv;
    u16 argc;
    struct serial_port *serial_port;
};

void general_process_cmd(struct general_cmd *cmd);


#ifdef __cplusplus
}
#endif

#endif /* __K_GENERAL_H */
