
#include <linux/printk.h>
#include <linux/kernel.h>

#include "project.h"

int board_init(void)
{
    int ret = 0;

    pr_info("==== %s v%s ====\n", PROJECT_NAME, PROJECT_VERSION);
    pr_info("Platform: %s, Sim: %s\n",
        MCU_PLATFORM_STM32 ? "STM32" : "Simulated",
        KINETIS_FAKE_SIM ? "Yes" : "No");

    return ret;
}

int app_main(void)
{
    int ret = 0;

#if POV_RUN_MODE == POV_RUN_HOST
    pr_info("POV: Running as HOST (rotating end)\n");
    ret = host_main();
#elif POV_RUN_MODE == POV_RUN_SLAVE
    pr_info("POV: Running as SLAVE (fixed end)\n");
    ret = slave_main();
#elif POV_RUN_MODE == POV_RUN_BOTH
    pr_info("POV: Running HOST + SLAVE simulation\n");
    /* In dual mode, slave would run in a separate thread */
    ret = slave_main();
#else
    pr_err("POV: invalid run mode %d\n", POV_RUN_MODE);
    ret = -EINVAL;
#endif

    return ret;
}
