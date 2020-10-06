#include "algorithm/k-general.h"
#include "algorithm/k-slist.h"
#include "peripheral/k-serialport.h"
#include "timer/k-delay.h"
#include "timer/k-basictimer.h"
#include "string.h"
#include "stdio.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "core/idebug.h"

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void General_GenerateCommand(GeneralCommand_TypeDef *Command)
{
    Command->SerialPort->PortNbr = 3;
    Command->SerialPort->TempBuffer_Size = 200;
    Command->SerialPort->RxBuffer_Size = 50;
    Command->SerialPort->RxScanInterval = 10;

    switch (Command->Property) {
        case AT_NONE:
            break;

        case AT_TEST:
            Command->SerialPort->TxBuffer_Size = strlen(Command->AT_Command) + strlen("?\r\n");
            Command->SerialPort->TempBuffer_Size = 128;
            Command->SerialPort->RxScanInterval = 10;
            snprintf((char *)Command->SerialPort->TxBuffer,
                Command->SerialPort->TxBuffer_Size, "%s?\r\n",
                Command->AT_Command);
            break;

        case AT_READ:
            Command->SerialPort->TxBuffer_Size = strlen(Command->AT_Command) + strlen("?\r\n");
            Command->SerialPort->TempBuffer_Size = 128;
            Command->SerialPort->RxScanInterval = 10;
            snprintf((char *)Command->SerialPort->TxBuffer,
                Command->SerialPort->TxBuffer_Size, "%s?\r\n",
                Command->AT_Command);
            break;

        case AT_SET:
            Command->SerialPort->TxBuffer_Size = strlen(Command->AT_Command) + strlen("?\r\n");
            Command->SerialPort->TempBuffer_Size = 128;
            Command->SerialPort->RxScanInterval = 10;
            snprintf((char *)Command->SerialPort->TxBuffer,
                Command->SerialPort->TxBuffer_Size, "%s?\r\n",
                Command->AT_Command);
            break;

        case AT_EXCUTE:
            Command->SerialPort->TxBuffer_Size = strlen(Command->AT_Command) + strlen("?\r\n");
            Command->SerialPort->TempBuffer_Size = 128;
            Command->SerialPort->RxScanInterval = 10;
            snprintf((char *)Command->SerialPort->TxBuffer,
                Command->SerialPort->TxBuffer_Size, "%s?\r\n",
                Command->AT_Command);
            break;
    }

    SerialPort_Open(Command->SerialPort);
}

void General_TransmmitCommand(GeneralCommand_TypeDef *Command)
{
    SerialPort_Open(Command->SerialPort);
    SerialPort_Send(Command->SerialPort);
}

void General_ReceiveCommand(GeneralCommand_TypeDef *Command)
{
    uint32_t Refer = 0;
    uint32_t Delta = 0;

    Refer = BasicTimer_GetUSTick();

    while (1) {
        if (SerialPort_Receive(Command->SerialPort) == true)
            break;

        Delta = BasicTimer_GetUSTick() >= Refer ?
            BasicTimer_GetUSTick() - Refer :
            BasicTimer_GetUSTick() + (DELAY_TIMER_UNIT - Refer);

        if (Delta > Command->WaitTime) {
            Command->TimeoutFlag = true;
            break;
        }
    }

    SerialPort_Close(Command->SerialPort);
}

char *strsep(char **s, const char *ct);

void General_DecomposeResult(GeneralCommand_TypeDef *Command)
{
    char **argv = Command->argv;
    uint16_t *argc = &Command->argc;

    do {
        argv[*argc] = strsep((char **) & (Command->SerialPort->RxBuffer), Command->Delimiter);
        kinetis_debug_trace(KERN_DEBUG, "[%d] %s", *argc, argv[*argc]);
        (*argc)++;
    } while (Command->SerialPort->RxBuffer);
}

void General_ProcessCommand(GeneralCommand_TypeDef *Command)
{
    while (Command->ErrorRepetition) {
        General_GenerateCommand(Command);
        General_TransmmitCommand(Command);
        General_ReceiveCommand(Command);
        General_DecomposeResult(Command);

        Command->ErrorRepetition--;

        if (strcmp(Command->pExpectRes, Command->argv[0]) != 0)
            Command->ErrorFlag = true;
        else
            break;
    }
}

#ifdef DESIGN_VERIFICATION_GENERAL
#include "dv/k-test.h"

int t_General_Success(int argc, char **argv)
{

    return PASS;
}

int t_General_Error(int argc, char **argv)
{

    return PASS;
}

int t_General_Timeout(int argc, char **argv)
{

    return PASS;
}

#endif

