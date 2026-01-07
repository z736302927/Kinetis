#include <linux/iopoll.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "kinetis/general.h"
#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

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
	u16 cmd_len = 0;

	if (cmd == NULL || cmd->serial_port == NULL) {
		pr_err("General CMD: Invalid parameters\n");
		return;
	}

	cmd->serial_port->port_nbr = 3;
	cmd->serial_port->tmp_buffer_size = 200;
	cmd->serial_port->rx_scan_interval = 10;

	switch (cmd->property) {
	case AT_NONE:
		cmd_len = strlen(cmd->at_cmd) + 2;
		cmd->serial_port->tmp_buffer_size = 128;
		cmd->serial_port->rx_scan_interval = 10;
		snprintf((char *)cmd->serial_port->tx_buffer,
			cmd->serial_port->tx_buffer_size, "%s\r\n",
			cmd->at_cmd);
		break;

	case AT_TEST:
		cmd_len = strlen(cmd->at_cmd) + strlen("?\r\n");
		cmd->serial_port->tmp_buffer_size = 128;
		cmd->serial_port->rx_scan_interval = 10;
		snprintf((char *)cmd->serial_port->tx_buffer,
			cmd->serial_port->tx_buffer_size, "%s?\r\n",
			cmd->at_cmd);
		break;

	case AT_READ:
		cmd_len = strlen(cmd->at_cmd) + strlen("?\r\n");
		cmd->serial_port->tmp_buffer_size = 128;
		cmd->serial_port->rx_scan_interval = 10;
		snprintf((char *)cmd->serial_port->tx_buffer,
			cmd->serial_port->tx_buffer_size, "%s?\r\n",
			cmd->at_cmd);
		break;

	case AT_SET:
		if (cmd->argu != NULL) {
			cmd_len = strlen(cmd->at_cmd) + strlen("=") + strlen(cmd->argu) + 2;
		} else {
			cmd_len = strlen(cmd->at_cmd) + 2;
		}
		cmd->serial_port->tmp_buffer_size = 128;
		cmd->serial_port->rx_scan_interval = 10;
		if (cmd->argu != NULL) {
			snprintf((char *)cmd->serial_port->tx_buffer,
				cmd->serial_port->tx_buffer_size, "%s=%s\r\n",
				cmd->at_cmd, cmd->argu);
		} else {
			snprintf((char *)cmd->serial_port->tx_buffer,
				cmd->serial_port->tx_buffer_size, "%s\r\n",
				cmd->at_cmd);
		}
		break;

	case AT_EXCUTE:
		cmd_len = strlen(cmd->at_cmd) + 2;
		cmd->serial_port->tmp_buffer_size = 128;
		cmd->serial_port->rx_scan_interval = 10;
		snprintf((char *)cmd->serial_port->tx_buffer,
			cmd->serial_port->tx_buffer_size, "%s\r\n",
			cmd->at_cmd);
		break;
	}

	cmd->serial_port->tx_buffer_size = cmd_len + 1;

	kfree(cmd->serial_port->tx_buffer);
	cmd->serial_port->tx_buffer = kmalloc(cmd->serial_port->tx_buffer_size, GFP_KERNEL);

	if (cmd->serial_port->tx_buffer == NULL) {
		pr_err("General CMD: Failed to allocate tx_buffer\n");
		return;
	}

	serial_port_open(cmd->serial_port);
}

static void general_transmmit_cmd(struct general_cmd *cmd)
{
	serial_port_send(cmd->serial_port);
}

static void general_receive_cmd(struct general_cmd *cmd)
{
	bool arrived;
	int ret;
	u32 start_time = basic_timer_get_ms();

	while ((basic_timer_get_ms() - start_time) < cmd->wait_time) {
		arrived = serial_port_receive(cmd->serial_port);
		if (arrived) {
			return;
		}
	}

	cmd->timeout_flag = true;
}

static void general_decompose_result(struct general_cmd *cmd)
{
	char **argv = cmd->argv;
	u16 *argc = &cmd->argc;
	char *buffer_ptr = (char *)cmd->serial_port->rx_buffer;

	if (buffer_ptr == NULL || cmd->delimiter == NULL) {
		return;
	}

	do {
		argv[*argc] = strsep(&buffer_ptr, cmd->delimiter);
		pr_debug("[%d] %s", *argc, argv[*argc]);
		(*argc)++;
	} while (buffer_ptr != NULL);
}

void general_process_cmd(struct general_cmd *cmd)
{
	if (cmd == NULL || cmd->serial_port == NULL) {
		pr_err("General CMD: Invalid parameters\n");
		return;
	}

	cmd->current_repetition = 0;
	cmd->error_flag = 0;
	cmd->timeout_flag = 0;

	while (cmd->current_repetition < cmd->error_repetition) {
		general_generate_cmd(cmd);
		general_transmmit_cmd(cmd);
		general_receive_cmd(cmd);

		if (cmd->timeout_flag) {
			pr_debug("General CMD: Timeout on attempt %d\n", cmd->current_repetition + 1);
			cmd->current_repetition++;
			serial_port_close(cmd->serial_port);
			continue;
		}

		general_decompose_result(cmd);

		if (cmd->argc == 0 || cmd->argv[0] == NULL) {
			pr_debug("General CMD: No response received on attempt %d\n", cmd->current_repetition + 1);
			cmd->error_flag = 1;
			cmd->current_repetition++;
			serial_port_close(cmd->serial_port);
			continue;
		}

		if (strcmp(cmd->expect_res, cmd->argv[0]) != 0) {
			pr_debug("General CMD: Expected '%s', got '%s' on attempt %d\n",
				cmd->expect_res, cmd->argv[0], cmd->current_repetition + 1);
			cmd->error_flag = 1;
			cmd->current_repetition++;
			serial_port_close(cmd->serial_port);
		} else {
			cmd->error_flag = 0;
			break;
		}
	}

	serial_port_close(cmd->serial_port);
}

#ifdef DESIGN_VERIFICATION_GENERAL
#include "kinetis/test-kinetis.h"

static struct serial_port test_serial_port;
static u8 test_tx_buffer[256];
static u16 test_tmp_buffer[256];
static u8 test_rx_buffer[256];
static u16 test_rx_index = 0;
static char test_response_buffer[256] = {0};

static void general_test_reset(void)
{
	memset(test_response_buffer, 0, sizeof(test_response_buffer));
	test_rx_index = 0;
}

static void general_test_set_response(const char *response)
{
	strncpy(test_response_buffer, response, sizeof(test_response_buffer) - 1);
}

static void general_test_simulate_receive(const char *data)
{
	strncpy((char *)test_rx_buffer, data, sizeof(test_rx_buffer) - 1);
}

int t_general_success(int argc, char **argv)
{
	struct general_cmd cmd;

	pr_info("=== General CMD Success Test ===\n");

	general_test_reset();
	general_test_set_response("OK\r\n");

	memset(&test_serial_port, 0, sizeof(test_serial_port));
	test_serial_port.tmp_buffer = test_tmp_buffer;
	test_serial_port.tmp_buffer_size = 256;
	test_serial_port.tx_buffer = test_tx_buffer;
	test_serial_port.rx_buffer = test_rx_buffer;
	test_serial_port.rx_buffer_size = 0;
	test_serial_port.rx_scan_interval = 10;
	test_serial_port.rx_head = 0;
	test_serial_port.rx_tail = 0;
	test_serial_port.rx_write_index = 0;

	cmd.at_cmd = "AT";
	cmd.property = AT_NONE;
	cmd.argu = NULL;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 1000;
	cmd.interval = 100;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = &test_serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag) {
		pr_err("FAIL - Command failed\n");
		return FAIL;
	}

	if (cmd.timeout_flag) {
		pr_err("FAIL - Command timed out\n");
		return FAIL;
	}

	pr_info("PASS - General CMD success test passed\n");
	return PASS;
}

int t_general_error(int argc, char **argv)
{
	struct general_cmd cmd;

	pr_info("=== General CMD Error Test ===\n");

	general_test_reset();
	general_test_set_response("ERROR\r\n");

	memset(&test_serial_port, 0, sizeof(test_serial_port));
	test_serial_port.tmp_buffer = test_tmp_buffer;
	test_serial_port.tmp_buffer_size = 256;
	test_serial_port.tx_buffer = test_tx_buffer;
	test_serial_port.rx_buffer = test_rx_buffer;
	test_serial_port.rx_buffer_size = 0;
	test_serial_port.rx_scan_interval = 10;
	test_serial_port.rx_head = 0;
	test_serial_port.rx_tail = 0;
	test_serial_port.rx_write_index = 0;

	cmd.at_cmd = "AT";
	cmd.property = AT_NONE;
	cmd.argu = NULL;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 1000;
	cmd.interval = 100;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = &test_serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (!cmd.error_flag) {
		pr_err("FAIL - Should have detected error\n");
		return FAIL;
	}

	pr_info("PASS - General CMD error test passed\n");
	return PASS;
}

int t_general_timeout(int argc, char **argv)
{
	struct general_cmd cmd;

	pr_info("=== General CMD Timeout Test ===\n");

	general_test_reset();
	general_test_set_response("");

	memset(&test_serial_port, 0, sizeof(test_serial_port));
	test_serial_port.tmp_buffer = test_tmp_buffer;
	test_serial_port.tmp_buffer_size = 256;
	test_serial_port.tx_buffer = test_tx_buffer;
	test_serial_port.rx_buffer = test_rx_buffer;
	test_serial_port.rx_buffer_size = 0;
	test_serial_port.rx_scan_interval = 10;
	test_serial_port.rx_head = 0;
	test_serial_port.rx_tail = 0;
	test_serial_port.rx_write_index = 0;

	cmd.at_cmd = "AT";
	cmd.property = AT_NONE;
	cmd.argu = NULL;
	cmd.expect_res = "OK";
	cmd.error_repetition = 2;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 100;
	cmd.interval = 100;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = &test_serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (!cmd.timeout_flag) {
		pr_err("FAIL - Should have timed out\n");
		return FAIL;
	}

	pr_info("PASS - General CMD timeout test passed\n");
	return PASS;
}

int t_general_set_command(int argc, char **argv)
{
	struct general_cmd cmd;

	pr_info("=== General CMD SET Command Test ===\n");

	general_test_reset();
	general_test_set_response("OK\r\n");

	memset(&test_serial_port, 0, sizeof(test_serial_port));
	test_serial_port.tmp_buffer = test_tmp_buffer;
	test_serial_port.tmp_buffer_size = 256;
	test_serial_port.tx_buffer = test_tx_buffer;
	test_serial_port.rx_buffer = test_rx_buffer;
	test_serial_port.rx_buffer_size = 0;
	test_serial_port.rx_scan_interval = 10;
	test_serial_port.rx_head = 0;
	test_serial_port.rx_tail = 0;
	test_serial_port.rx_write_index = 0;

	cmd.at_cmd = "AT+UART";
	cmd.property = AT_SET;
	cmd.argu = "115200,0,0";
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 1000;
	cmd.interval = 100;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = &test_serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag) {
		pr_err("FAIL - SET command failed\n");
		return FAIL;
	}

	pr_info("PASS - General CMD SET command test passed\n");
	return PASS;
}

#endif
