#include "kinetis/timeout.h"
#include <linux/delay.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

u8 Timeout_WaitUSDone(int *SrcValue, int DstValue, u32 Timeout)
{
    do {
        if (*SrcValue == DstValue)
            return true;
        else
            udelay(1);
    } while (Timeout--);

    return false;
}

u8 Timeout_WaitMSDone(int *SrcValue, int DstValue, u32 Timeout)
{
    do {
        if (*SrcValue == DstValue)
            return true;
        else
            mdelay(1);
    } while (Timeout--);

    return false;
}
