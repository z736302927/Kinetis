#include "peripheral/hc-05.h"
#include "algorithm/k-general.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define DEBUG
#include "core/idebug.h"

#define hc_05_printf                  p_dbg

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void hc_05_TestCommand(void)
{
    GeneralCommand_TypeDef Command;

    Command.AT_Command = "AT";
    Command.Property = AT_NONE;
    Command.pArgument = NULL;
    Command.pExpectRes = "OK";
    Command.ErrorRepetition = 3;
    Command.WaitTime = 1000;
    Command.Interval = 1000;

    General_ProcessCommand(&Command);

    if(Command.ErrorFlag == true && Command.ErrorRepetition == 0)
    {

    }

    if(Command.TimeoutFlag == true)
    {

    }
}

#ifdef DESIGN_VERIFICATION_HC_05
#include "dv/k-test.h"

int t_hc_05_TestCommand(int argc, char **argv)
{
    hc_05_TestCommand();

    return PASS;
}

#endif


