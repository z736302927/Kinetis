#ifndef __K_GENERAL_H
#define __K_GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "peripheral/serialport.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

typedef enum {
    AT_NONE,
    AT_TEST,
    AT_READ,
    AT_SET,
    AT_EXCUTE
} AT_CommandProperty;

typedef struct _GeneralCommand_TypeDef {
    char *AT_Command;
    char *pArgument;
    char *pExpectRes;
    AT_CommandProperty Property;
    u8 ErrorRepetition;
    u8 CurrentRepetition;
    u8 ErrorFlag;
    u8 TimeoutFlag;
    u16 WaitTime;
    u16 Interval;
    char *Delimiter;
    char **argv;
    u16 argc;
    SerialPort_TypeDef *SerialPort;
} GeneralCommand_TypeDef;

void General_ProcessCommand(GeneralCommand_TypeDef *Command);


#ifdef __cplusplus
}
#endif

#endif /* __K_GENERAL_H */
