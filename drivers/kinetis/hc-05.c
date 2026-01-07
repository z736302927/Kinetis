#include "kinetis/hc-05.h"
#include "kinetis/general.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

#include <linux/string.h>
#include <linux/slab.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

int hc_05_init(struct hc_05_device *device, struct serial_port *serial_port)
{
	if (device == NULL || serial_port == NULL) {
		pr_err("HC-05: Invalid parameters\n");
		return -1;
	}

	memset(device, 0, sizeof(struct hc_05_device));
	device->serial_port = serial_port;
	device->state = HC05_STATE_IDLE;
	device->mode = HC05_MODE_DATA;
	device->connection_timeout_ms = 10000;

	strncpy(device->config.name, "HC-05", sizeof(device->config.name) - 1);
	device->config.baud_rate = 9600;
	device->config.role = HC05_ROLE_SLAVE;
	device->config.pin_code = 1234;

	pr_info("HC-05: Device initialized\n");
	return 0;
}

void hc_05_deinit(struct hc_05_device *device)
{
	if (device == NULL) {
		return;
	}

	if (device->serial_port != NULL) {
		serial_port_close(device->serial_port);
	}

	memset(device, 0, sizeof(struct hc_05_device));
	pr_info("HC-05: Device deinitialized\n");
}

int hc_05_enter_command_mode(struct hc_05_device *device)
{
	struct general_cmd cmd;

	if (device == NULL) {
		return -1;
	}

	pr_debug("HC-05: Entering command mode\n");

	device->mode = HC05_MODE_COMMAND;

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
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to enter command mode\n");
		device->state = HC05_STATE_ERROR;
		return -1;
	}

	device->state = HC05_STATE_READY;
	return 0;
}

int hc_05_exit_command_mode(struct hc_05_device *device)
{
	if (device == NULL) {
		return -1;
	}

	pr_debug("HC-05: Exiting command mode\n");
	device->mode = HC05_MODE_DATA;
	return 0;
}

int hc_05_set_baud_rate(struct hc_05_device *device, u32 baud_rate)
{
	struct general_cmd cmd;
	char baud_str[32];

	if (device == NULL || baud_rate == 0) {
		return -1;
	}

	pr_info("HC-05: Setting baud rate to %u\n", baud_rate);

	snprintf(baud_str, sizeof(baud_str), "%u", baud_rate);

	cmd.at_cmd = "AT+UART";
	cmd.property = AT_SET;
	cmd.argu = baud_str;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to set baud rate\n");
		device->state = HC05_STATE_ERROR;
		return -1;
	}

	device->config.baud_rate = baud_rate;
	return 0;
}

int hc_05_set_name(struct hc_05_device *device, const char *name)
{
	struct general_cmd cmd;

	if (device == NULL || name == NULL) {
		return -1;
	}

	pr_info("HC-05: Setting name to %s\n", name);

	cmd.at_cmd = "AT+NAME";
	cmd.property = AT_SET;
	cmd.argu = (char *)name;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to set name\n");
		device->state = HC05_STATE_ERROR;
		return -1;
	}

	strncpy(device->config.name, name, sizeof(device->config.name) - 1);
	return 0;
}

int hc_05_set_role(struct hc_05_device *device, enum hc_05_role role)
{
	struct general_cmd cmd;
	char role_str[16];

	if (device == NULL) {
		return -1;
	}

	pr_info("HC-05: Setting role to %d\n", role);

	snprintf(role_str, sizeof(role_str), "%d", role);

	cmd.at_cmd = "AT+ROLE";
	cmd.property = AT_SET;
	cmd.argu = role_str;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to set role\n");
		device->state = HC05_STATE_ERROR;
		return -1;
	}

	device->config.role = role;
	return 0;
}

int hc_05_set_pin_code(struct hc_05_device *device, u8 pin_code)
{
	struct general_cmd cmd;
	char pin_str[16];

	if (device == NULL) {
		return -1;
	}

	pr_info("HC-05: Setting pin code to %d\n", pin_code);

	snprintf(pin_str, sizeof(pin_str), "%d", pin_code);

	cmd.at_cmd = "AT+PSWD";
	cmd.property = AT_SET;
	cmd.argu = pin_str;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to set pin code\n");
		device->state = HC05_STATE_ERROR;
		return -1;
	}

	device->config.pin_code = pin_code;
	return 0;
}

int hc_05_connect(struct hc_05_device *device, const char *address)
{
	struct general_cmd cmd;

	if (device == NULL || address == NULL) {
		return -1;
	}

	pr_info("HC-05: Connecting to %s\n", address);

	cmd.at_cmd = "AT+LINK";
	cmd.property = AT_SET;
	cmd.argu = (char *)address;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = device->connection_timeout_ms;
	cmd.interval = 500;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to connect\n");
		device->state = HC05_STATE_ERROR;
		return -1;
	}

	device->state = HC05_STATE_CONNECTED;
	return 0;
}

int hc_05_disconnect(struct hc_05_device *device)
{
	struct general_cmd cmd;

	if (device == NULL) {
		return -1;
	}

	pr_info("HC-05: Disconnecting\n");

	cmd.at_cmd = "AT+DISC";
	cmd.property = AT_NONE;
	cmd.argu = NULL;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	kfree(cmd.argv);

	device->state = HC05_STATE_READY;
	return 0;
}

int hc_05_send_data(struct hc_05_device *device, const u8 *data, u16 length)
{
	if (device == NULL || data == NULL || length == 0) {
		return -1;
	}

	if (device->mode == HC05_MODE_COMMAND) {
		pr_err("HC-05: Cannot send data in command mode\n");
		return -1;
	}

	if (device->state != HC05_STATE_CONNECTED) {
		pr_err("HC-05: Not connected\n");
		return -1;
	}

	kfree(device->serial_port->tx_buffer);
	device->serial_port->tx_buffer = kmalloc(length, GFP_KERNEL);
	if (device->serial_port->tx_buffer == NULL) {
		pr_err("HC-05: Memory allocation failed\n");
		return -1;
	}

	memcpy(device->serial_port->tx_buffer, data, length);
	device->serial_port->tx_buffer_size = length;

	serial_port_send(device->serial_port);

	pr_debug("HC-05: Sent %d bytes\n", length);
	return 0;
}

int hc_05_receive_data(struct hc_05_device *device, u8 *buffer, u16 length, u16 timeout_ms)
{
	u32 start_time;
	bool received;

	if (device == NULL || buffer == NULL || length == 0) {
		return -1;
	}

	if (device->mode == HC05_MODE_COMMAND) {
		pr_err("HC-05: Cannot receive data in command mode\n");
		return -1;
	}

	start_time = basic_timer_get_ms();

	while ((basic_timer_get_ms() - start_time) < timeout_ms) {
		received = serial_port_receive(device->serial_port);
		if (received && device->serial_port->rx_buffer_size > 0) {
			u16 copy_len = device->serial_port->rx_buffer_size < length ?
				device->serial_port->rx_buffer_size : length;
			memcpy(buffer, device->serial_port->rx_buffer, copy_len);
			kfree(device->serial_port->rx_buffer);
			device->serial_port->rx_buffer = NULL;
			return copy_len;
		}
	}

	pr_debug("HC-05: Receive timeout\n");
	return -1;
}

int hc_05_get_version(struct hc_05_device *device, char *version, u16 max_len)
{
	struct general_cmd cmd;

	if (device == NULL || version == NULL) {
		return -1;
	}

	cmd.at_cmd = "AT+VERSION";
	cmd.property = AT_NONE;
	cmd.argu = NULL;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	if (cmd.argc > 0 && cmd.argv[0] != NULL) {
		strncpy(version, cmd.argv[0], max_len - 1);
		version[max_len - 1] = '\0';
	}

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to get version\n");
		return -1;
	}

	strncpy(device->version, version, sizeof(device->version) - 1);
	return 0;
}

int hc_05_get_address(struct hc_05_device *device, u8 *address)
{
	struct general_cmd cmd;

	if (device == NULL || address == NULL) {
		return -1;
	}

	cmd.at_cmd = "AT+ADDR";
	cmd.property = AT_NONE;
	cmd.argu = NULL;
	cmd.expect_res = "OK";
	cmd.error_repetition = 3;
	cmd.current_repetition = 0;
	cmd.error_flag = 0;
	cmd.timeout_flag = 0;
	cmd.wait_time = 2000;
	cmd.interval = 200;
	cmd.delimiter = "\r\n";
	cmd.argv = (char **)kmalloc(10 * sizeof(char *), GFP_KERNEL);
	cmd.argc = 0;
	cmd.serial_port = device->serial_port;

	general_process_cmd(&cmd);

	if (cmd.argc > 0 && cmd.argv[0] != NULL) {
		strncpy((char *)address, cmd.argv[0], 12);
		address[12] = '\0';
	}

	kfree(cmd.argv);

	if (cmd.error_flag || cmd.timeout_flag) {
		pr_err("HC-05: Failed to get address\n");
		return -1;
	}

	memcpy(device->address, address, 13);
	return 0;
}

enum hc_05_state hc_05_get_state(struct hc_05_device *device)
{
	if (device == NULL) {
		return HC05_STATE_ERROR;
	}

	return device->state;
}

#ifdef DESIGN_VERIFICATION_HC_05
#include "kinetis/test-kinetis.h"

static struct serial_port test_serial_port;
static u8 test_rx_buffer[256];
static u16 test_rx_index = 0;
static char test_response_buffer[256] = {0};
static u8 test_error_mode = 0;
static u8 test_timeout_mode = 0;

static void mock_hc05_reset(void)
{
	memset(test_response_buffer, 0, sizeof(test_response_buffer));
	test_rx_index = 0;
	test_error_mode = 0;
	test_timeout_mode = 0;
}

static void mock_hc05_set_response(const char *response)
{
	strncpy(test_response_buffer, response, sizeof(test_response_buffer) - 1);
}

static u16 mock_hc05_read_data(u8 *buffer, u16 max_len)
{
	u16 available = strlen(test_response_buffer);
	u16 to_read = available < max_len ? available : max_len;

	memcpy(buffer, test_response_buffer, to_read);
	memset(test_response_buffer, 0, sizeof(test_response_buffer));

	return to_read;
}

int t_hc05_init_success(int argc, char **argv)
{
	struct hc_05_device device;
	int ret;

	pr_info("=== HC-05 Init Success Test ===\n");

	ret = hc_05_init(&device, &test_serial_port);
	if (ret != 0) {
		pr_err("FAIL - hc_05_init failed\n");
		return FAIL;
	}

	if (device.state != HC05_STATE_IDLE) {
		pr_err("FAIL - Invalid initial state\n");
		return FAIL;
	}

	pr_info("PASS - HC-05 initialized successfully\n");
	return PASS;
}

int t_hc05_command_mode_success(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Command Mode Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	int ret = hc_05_enter_command_mode(&device);

	if (ret != 0) {
		pr_err("FAIL - Failed to enter command mode\n");
		return FAIL;
	}

	if (device.mode != HC05_MODE_COMMAND) {
		pr_err("FAIL - Mode not set to command\n");
		return FAIL;
	}

	pr_info("PASS - Command mode entered successfully\n");
	return PASS;
}

int t_hc05_set_baud_rate_success(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Set Baud Rate Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.mode = HC05_MODE_COMMAND;
	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	int ret = hc_05_set_baud_rate(&device, 115200);

	if (ret != 0) {
		pr_err("FAIL - Failed to set baud rate\n");
		return FAIL;
	}

	if (device.config.baud_rate != 115200) {
		pr_err("FAIL - Baud rate not saved\n");
		return FAIL;
	}

	pr_info("PASS - Baud rate set successfully\n");
	return PASS;
}

int t_hc05_set_name_success(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Set Name Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.mode = HC05_MODE_COMMAND;
	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	int ret = hc_05_set_name(&device, "MyHC05");

	if (ret != 0) {
		pr_err("FAIL - Failed to set name\n");
		return FAIL;
	}

	if (strcmp(device.config.name, "MyHC05") != 0) {
		pr_err("FAIL - Name not saved\n");
		return FAIL;
	}

	pr_info("PASS - Name set successfully\n");
	return PASS;
}

int t_hc05_set_role_success(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Set Role Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.mode = HC05_MODE_COMMAND;
	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	int ret = hc_05_set_role(&device, HC05_ROLE_MASTER);

	if (ret != 0) {
		pr_err("FAIL - Failed to set role\n");
		return FAIL;
	}

	if (device.config.role != HC05_ROLE_MASTER) {
		pr_err("FAIL - Role not saved\n");
		return FAIL;
	}

	pr_info("PASS - Role set successfully\n");
	return PASS;
}

int t_hc05_get_version_success(int argc, char **argv)
{
	struct hc_05_device device;
	char version[16] = {0};

	pr_info("=== HC-05 Get Version Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.mode = HC05_MODE_COMMAND;
	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	int ret = hc_05_get_version(&device, version, sizeof(version));

	if (ret != 0) {
		pr_err("FAIL - Failed to get version\n");
		return FAIL;
	}

	pr_info("PASS - Version retrieved successfully\n");
	return PASS;
}

int t_hc05_get_address_success(int argc, char **argv)
{
	struct hc_05_device device;
	u8 address[13] = {0};

	pr_info("=== HC-05 Get Address Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.mode = HC05_MODE_COMMAND;
	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	int ret = hc_05_get_address(&device, address);

	if (ret != 0) {
		pr_err("FAIL - Failed to get address\n");
		return FAIL;
	}

	pr_info("PASS - Address retrieved successfully\n");
	return PASS;
}

int t_hc05_send_data_success(int argc, char **argv)
{
	struct hc_05_device device;
	u8 test_data[] = "Hello HC-05";

	pr_info("=== HC-05 Send Data Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.state = HC05_STATE_CONNECTED;
	device.mode = HC05_MODE_DATA;

	int ret = hc_05_send_data(&device, test_data, sizeof(test_data) - 1);

	if (ret != 0) {
		pr_err("FAIL - Failed to send data\n");
		return FAIL;
	}

	pr_info("PASS - Data sent successfully\n");
	return PASS;
}

int t_hc05_receive_data_success(int argc, char **argv)
{
	struct hc_05_device device;
	u8 buffer[64];
	u8 test_data[] = "Test Data";

	pr_info("=== HC-05 Receive Data Success Test ===\n");

	hc_05_init(&device, &test_serial_port);
	device.state = HC05_STATE_CONNECTED;
	device.mode = HC05_MODE_DATA;

	memcpy(device.serial_port->rx_buffer, test_data, sizeof(test_data) - 1);
	device.serial_port->rx_buffer_size = sizeof(test_data) - 1;

	int ret = hc_05_receive_data(&device, buffer, sizeof(buffer), 100);

	if (ret < 0) {
		pr_err("FAIL - Failed to receive data\n");
		return FAIL;
	}

	pr_info("PASS - Data received successfully (%d bytes)\n", ret);
	return PASS;
}

int t_hc05_error_handling(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Error Handling Test ===\n");

	test_error_mode = 1;

	hc_05_init(&device, &test_serial_port);
	mock_hc05_reset();
	mock_hc05_set_response("ERROR\r\n");

	int ret = hc_05_enter_command_mode(&device);

	if (ret == 0) {
		pr_err("FAIL - Should have failed with error response\n");
		return FAIL;
	}

	if (device.state != HC05_STATE_ERROR) {
		pr_err("FAIL - State not set to error\n");
		return FAIL;
	}

	pr_info("PASS - Error handling working correctly\n");
	return PASS;
}

int t_hc05_timeout_handling(int argc, char **argv)
{
	struct hc_05_device device;
	char version[16] = {0};

	pr_info("=== HC-05 Timeout Handling Test ===\n");

	test_timeout_mode = 1;

	hc_05_init(&device, &test_serial_port);
	mock_hc05_reset();
	mock_hc05_set_response("");

	int ret = hc_05_get_version(&device, version, sizeof(version));

	if (ret == 0) {
		pr_err("FAIL - Should have timed out\n");
		return FAIL;
	}

	pr_info("PASS - Timeout handling working correctly\n");
	return PASS;
}

int t_hc05_invalid_parameters(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Invalid Parameters Test ===\n");

	int ret1 = hc_05_init(NULL, &test_serial_port);
	if (ret1 != -1) {
		pr_err("FAIL - Should reject NULL device\n");
		return FAIL;
	}

	int ret2 = hc_05_init(&device, NULL);
	if (ret2 != -1) {
		pr_err("FAIL - Should reject NULL serial_port\n");
		return FAIL;
	}

	hc_05_init(&device, &test_serial_port);

	int ret3 = hc_05_set_baud_rate(&device, 0);
	if (ret3 != -1) {
		pr_err("FAIL - Should reject invalid baud rate\n");
		return FAIL;
	}

	int ret4 = hc_05_set_name(&device, NULL);
	if (ret4 != -1) {
		pr_err("FAIL - Should reject NULL name\n");
		return FAIL;
	}

	pr_info("PASS - Invalid parameters handled correctly\n");
	return PASS;
}

int t_hc05_full_workflow(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Full Workflow Test ===\n");

	hc_05_init(&device, &test_serial_port);

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	if (hc_05_enter_command_mode(&device) != 0) {
		pr_err("FAIL - Command mode entry failed\n");
		return FAIL;
	}

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");
	if (hc_05_set_baud_rate(&device, 115200) != 0) {
		pr_err("FAIL - Baud rate setting failed\n");
		return FAIL;
	}

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");
	if (hc_05_set_name(&device, "TestHC05") != 0) {
		pr_err("FAIL - Name setting failed\n");
		return FAIL;
	}

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");
	if (hc_05_set_role(&device, HC05_ROLE_MASTER) != 0) {
		pr_err("FAIL - Role setting failed\n");
		return FAIL;
	}

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");
	if (hc_05_exit_command_mode(&device) != 0) {
		pr_err("FAIL - Command mode exit failed\n");
		return FAIL;
	}

	pr_info("PASS - Full workflow completed successfully\n");
	return PASS;
}

int t_hc05_integration_test(int argc, char **argv)
{
	struct hc_05_device device;

	pr_info("=== HC-05 Integration Test ===\n");

	hc_05_init(&device, &test_serial_port);

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	if (hc_05_enter_command_mode(&device) != 0) {
		pr_err("FAIL - Integration test: Command mode failed\n");
		return FAIL;
	}

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	char version[16] = {0};
	if (hc_05_get_version(&device, version, sizeof(version)) != 0) {
		pr_err("FAIL - Integration test: Get version failed\n");
		return FAIL;
	}

	mock_hc05_reset();
	mock_hc05_set_response("OK\r\n");

	u8 address[13] = {0};
	if (hc_05_get_address(&device, address) != 0) {
		pr_err("FAIL - Integration test: Get address failed\n");
		return FAIL;
	}

	pr_info("PASS - Integration test passed\n");
	return PASS;
}

#endif
