#include "kinetis/hc-05.h"
#include "kinetis/general.h"
#include "kinetis/idebug.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void hc_05_test_cmd(void)
{
    struct general_cmd cmd;

    cmd.at_cmd = "AT";
    cmd.property = AT_NONE;
    cmd.argu = NULL;
    cmd.expect_res = "OK";
    cmd.error_repetition = 3;
    cmd.wait_time = 1000;
    cmd.interval = 1000;

    general_process_cmd(&cmd);

    if (cmd.error_flag == true && cmd.error_repetition == 0) {

    }

    if (cmd.timeout_flag == true) {

    }
}

#ifdef DESIGN_VERIFICATION_HC_05
#include "kinetis/test-kinetis.h"

int t_hc_05_test_cmd(int argc, char **argv)
{
    hc_05_test_cmd();

    return PASS;
}

#endif


