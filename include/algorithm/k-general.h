#ifndef __K_GENERAL_H
#define __K_GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"
#include "peripheral/k-serialport.h"

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

typedef enum
{
    AT_NONE,
    AT_TEST,
    AT_READ,
    AT_SET,
    AT_EXCUTE
} AT_CommandProperty;

typedef struct _GeneralCommand_TypeDef
{
    char *AT_Command;
    char *pArgument;
    char *pExpectRes;
    AT_CommandProperty Property;
    uint8_t ErrorRepetition;
    uint8_t CurrentRepetition;
    uint8_t ErrorFlag;
    uint8_t TimeoutFlag;
    uint16_t WaitTime;
    uint16_t Interval;
    char *Delimiter;
    char **argv;
    uint16_t argc;
    SerialPort_TypeDef *SerialPort;
} GeneralCommand_TypeDef;

void General_ProcessCommand(GeneralCommand_TypeDef *Command);


#ifdef __cplusplus
}
#endif

#endif /* __K_GENERAL_H */
