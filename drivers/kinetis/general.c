#include "kinetis/general.h"
#include "kinetis/slist.h"
#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"

#include "string.h"
#include "stdio.h"

#include <linux/iopoll.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

static void general_generate_cmd(struct general_cmd *cmd)
{
    cmd->serial_port->port_nbr = 3;
    cmd->serial_port->tmp_buffer_size = 200;
    cmd->serial_port->rx_buffer_size = 50;
    cmd->serial_port->rx_scan_interval = 10;

    switch (cmd->property) {
        case AT_NONE:
            break;

        case AT_TEST:
            cmd->serial_port->tx_buffer_size = strlen(cmd->at_cmd) + strlen("?\r\n");
            cmd->serial_port->tmp_buffer_size = 128;
            cmd->serial_port->rx_scan_interval = 10;
            snprintf((char *)cmd->serial_port->tx_buffer,
                cmd->serial_port->tx_buffer_size, "%s?\r\n",
                cmd->at_cmd);
            break;

        case AT_READ:
            cmd->serial_port->tx_buffer_size = strlen(cmd->at_cmd) + strlen("?\r\n");
            cmd->serial_port->tmp_buffer_size = 128;
            cmd->serial_port->rx_scan_interval = 10;
            snprintf((char *)cmd->serial_port->tx_buffer,
                cmd->serial_port->tx_buffer_size, "%s?\r\n",
                cmd->at_cmd);
            break;

        case AT_SET:
            cmd->serial_port->tx_buffer_size = strlen(cmd->at_cmd) + strlen("?\r\n");
            cmd->serial_port->tmp_buffer_size = 128;
            cmd->serial_port->rx_scan_interval = 10;
            snprintf((char *)cmd->serial_port->tx_buffer,
                cmd->serial_port->tx_buffer_size, "%s?\r\n",
                cmd->at_cmd);
            break;

        case AT_EXCUTE:
            cmd->serial_port->tx_buffer_size = strlen(cmd->at_cmd) + strlen("?\r\n");
            cmd->serial_port->tmp_buffer_size = 128;
            cmd->serial_port->rx_scan_interval = 10;
            snprintf((char *)cmd->serial_port->tx_buffer,
                cmd->serial_port->tx_buffer_size, "%s?\r\n",
                cmd->at_cmd);
            break;
    }

    serial_port_open(cmd->serial_port);
}

static void general_transmmit_cmd(struct general_cmd *cmd)
{
    serial_port_open(cmd->serial_port);
    serial_port_send(cmd->serial_port);
}

static void general_receive_cmd(struct general_cmd *cmd)
{
    bool arrived;
    int ret;
    
    ret = readx_poll_timeout_atomic(serial_port_receive, cmd->serial_port,
        arrived, arrived == true,
        0, cmd->wait_time);
    
    if (ret) {
        cmd->timeout_flag = true;
    }

    serial_port_close(cmd->serial_port);
}

static void general_decompose_result(struct general_cmd *cmd)
{
    char **argv = cmd->argv;
    u16 *argc = &cmd->argc;

    do {
        argv[*argc] = strsep((char **) & (cmd->serial_port->rx_buffer), cmd->delimiter);
        printk(KERN_DEBUG "[%d] %s", *argc, argv[*argc]);
        (*argc)++;
    } while (cmd->serial_port->rx_buffer);
}

void general_process_cmd(struct general_cmd *cmd)
{
    while (cmd->error_repetition) {
        general_generate_cmd(cmd);
        general_transmmit_cmd(cmd);
        general_receive_cmd(cmd);
        general_decompose_result(cmd);

        cmd->error_repetition--;

        if (strcmp(cmd->expect_res, cmd->argv[0]) != 0)
            cmd->error_flag = true;
        else
            break;
    }
}

#ifdef DESIGN_VERIFICATION_GENERAL
#include "kinetis/test-kinetis.h"

int t_general_success(int argc, char **argv)
{

    return PASS;
}

int t_general_error(int argc, char **argv)
{

    return PASS;
}

int t_general_timeout(int argc, char **argv)
{

    return PASS;
}

#endif

