#include "algorithm/k-crc.h"
#include "stdbool.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "core/idebug.h"

int CRC16_Check(char *input, int inputlen)
{
    short crcRet = 0;
    short inputCrc = 0;
    int retValue = false;

//    crcRet = CRC16_Calculate(input, inputlen - 2);

    inputCrc = (input[inputlen - 2] << 8) | input[inputlen - 1];

    if(crcRet != inputCrc)
    {
        kinetis_debug_trace(KERN_DEBUG, "CRC16 check failed !");
        retValue = false;
    }
    else
        retValue = true;

    return retValue;
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef DESIGN_VERIFICATION_CRC

#endif

