
#include <linux/printk.h>
#include <linux/kernel.h>

#include "kinetis/test-kinetis.h"

int board_init(void)
{
    int ret = 0;

    return ret;
}

int app_main(void)
{
    return k_test_case_schedule();
}
