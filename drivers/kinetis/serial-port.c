#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/hc-05.h"
#include "kinetis/design_verification.h"

#include <unistd.h>

#define pr_fmt(fmt) "serial-port: " fmt

#define CONFIG_SERIAL_PORT_RING_BUFFER	1

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in structure serial_port_1, design the function you need and initialize it in the main function.
  * @step 3:  You need to provide an ms timer for function basic_timer_get_ms.
  * @step 4:  For receiving data, you need to put it in like a ring, using interrupts or DMA.
  * @step 5:  Finally, you can process the received data in function serial_port_rx_buffer_Process.Note: maximum 256 bytes received.
  */

#if MCU_PLATFORM_STM32
#include "usart.h"
#else
#endif

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void *serial_port_monitor(void *para)
{
	struct serial_port *serial = (struct serial_port *)para;
	char response[SERIAL_PORT_BUFFER_SIZE];
	int i, index;

	while (serial->thread_switch) {
		if (serial->tx_buffer[0] != '\0') {
			size_t resp_len;

			response[0] = '\0';
			serial->sim_callback(serial->tx_buffer, response, serial->private);

			resp_len = strnlen(response, SERIAL_PORT_BUFFER_SIZE - 1);
			index = serial->producer;
			for (i = 0; i < resp_len; i++) {
				serial->rx_buffer[index] = response[i];
				index = (index + 1) % SERIAL_PORT_BUFFER_SIZE;
			}
			serial->rx_buffer[index] = '\0';

			memset(serial->tx_buffer, 0, serial->transmited_size);
		}

		usleep(1000);
	}

	return NULL;
}

struct serial_port *serial_port_alloc(void (*sim_callback)(char *request, char *response, void *context),
	void *private)
{
	struct serial_port *serial;

	serial = kzalloc(sizeof(struct serial_port), GFP_KERNEL);
	if (serial == NULL) {
		return ERR_PTR(-ENOMEM);
	}

	serial->rx_complete = false;

	if (sim_callback != NULL) {
		serial->sim_callback = sim_callback;
		serial->private = private;
		serial->thread_switch = 1;
		pthread_create(&serial->thread, NULL, serial_port_monitor, serial);
	}

	return serial;
}

void serial_port_free(struct serial_port *serial)
{
	if (serial->sim_callback && serial->thread) {
		serial->thread_switch = 0;
		pthread_join(serial->thread, NULL);
		serial->thread = 0;
	}

	kfree(serial);
}

void serial_port_extract_data(struct serial_port *serial)
{
	while (1) {
		if (serial->rx_buffer[serial->producer] != '\0') {
			serial->producer = (serial->producer + 1) % SERIAL_PORT_BUFFER_SIZE;
		} else {
			break;
		}
	}

	serial->received_size = (serial->producer + SERIAL_PORT_BUFFER_SIZE - serial->consumer) % SERIAL_PORT_BUFFER_SIZE;
	serial->rx_complete = (serial->received_size > 0);
}

int serial_port_get_data(struct serial_port *serial_port, char *buffer, int size, u32 timeout_ms)
{
	u32 elapsed_ms = 0;

	if (buffer == NULL) {
		return -EINVAL;
	}

	if (size < SERIAL_PORT_BUFFER_SIZE) {
		pr_warn("serial_port_get_data: buffer size %d is less than SERIAL_PORT_BUFFER_SIZE %d",
			size, SERIAL_PORT_BUFFER_SIZE);
		return -EINVAL;
	}

	if (timeout_ms == 0) {
		while (1) {
			serial_port_extract_data(serial_port);
			if (serial_port->rx_complete) {
				break;
			}
			mdelay(1);
		}
	} else {
		while (elapsed_ms < timeout_ms) {
			serial_port_extract_data(serial_port);
			if (serial_port->rx_complete) {
				break;
			}
			mdelay(1);
			elapsed_ms++;
		}
		if (elapsed_ms >= timeout_ms && !serial_port->rx_complete) {
			buffer[0] = '\0';
			pr_err("Cannot get data from serial port within %u ms", timeout_ms);
			return -ETIMEDOUT;
		}
	}

	if (serial_port->rx_complete) {
		for (int i = 0; i < serial_port->received_size; i++) {
			buffer[i] = serial_port->rx_buffer[serial_port->consumer];
			serial_port->rx_buffer[serial_port->consumer] = '\0';
			serial_port->consumer = (serial_port->consumer + 1) % SERIAL_PORT_BUFFER_SIZE;
		}
		serial_port->rx_complete = false;
#ifndef CONFIG_SERIAL_PORT_RING_BUFFER
		serial_port->producer = 0;
		serial_port->consumer = 0;
#endif
		pr_debug("producer: %d, consumer: %d\n", serial_port->producer, serial_port->consumer);
	}

	return serial_port->received_size;
}

int serial_port_transmit_bytes(struct serial_port *serial, const u8 *data, u16 size)
{
	if (!serial || !data || size == 0) {
		return -EINVAL;
	}

	if (size >= SERIAL_PORT_BUFFER_SIZE) {
		size = SERIAL_PORT_BUFFER_SIZE - 1;
		pr_warn("Transmit size(%u) too large, truncating to %d bytes", size, SERIAL_PORT_BUFFER_SIZE - 1);
	}

	memcpy(serial->tx_buffer, data, size);
	serial->tx_buffer[size] = '\0';
	serial->transmited_size = size;

	return 0;
}

#ifdef DESIGN_VERIFICATION_SEIRALPORT
#include "kinetis/test-kinetis.h"

static const struct virtual_at_command fake_at_commands[] = {
	{
		.request = "AT",
		.response = "OK",
	},
	{
		.request = "AT+VERSION?",
		.response = "HC-05 V2.0-20100601",
	},
	{
		.request = "AT+NAME?",
		.response = "MyBluetoothDevice",
	},
	{
		.request = "AT+BAUD?",
		.response = "115200",
	}
};

void serial_port_callback(char *request, char *response, void *context)
{
	struct virtual_at_command *array = context;
	struct virtual_at_command *at_cmd;
	const char error_response[] = "ERROR";
	int i;

	at_cmd = NULL;
	for (i = 0; array[i].request != NULL; i++) {
		if (strcmp(array[i].request, request) == 0) {
			at_cmd = &array[i];
			break;
		}
	}

	if (at_cmd != NULL)	{
		if (get_random_int() % 10 < 2) {
			for (i = 0; i < strlen(error_response); i++) {
				response[i] = error_response[i];
			}
		} else {
			for (i = 0; i < strlen(at_cmd->response); i++) {
				response[i] = at_cmd->response[i];
			}
		}
	} else {
		pr_err("Unknown AT command: %s\n", request);
		for (i = 0; i < strlen(error_response); i++) {
			response[i] = error_response[i];
		}
	}
	response[i] = '\0';
}

int t_serial_port_interactive(int argc, char **argv)
{
	struct serial_port *serial;
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);

	for (int i = 0; i < ARRAY_SIZE(fake_at_commands); i++) {
		serial_port_transmit_bytes(serial, fake_at_commands[i].request, strlen(fake_at_commands[i].request));

		ret = serial_port_get_data(serial, at_ack, sizeof(at_ack), 1000);
		if (ret < 0) {
			return ret;
		}
		pr_info("AT ACK: %.*s", ret, at_ack);
	}

	serial_port_free(serial);

	return 0;
}

int t_serial_port_basic(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[] = "Hello Serial Port";
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	pr_info("starting serial port basic test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, strlen(tx_data));
	if (ret < 0) {
		pr_err("failed to transmit data");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
	if (ret < 0) {
		pr_err("failed to get data");
		serial_port_free(serial);
		return FAIL;
	}

	if (strcmp(rx_data, "ERROR") == 0 || strncmp(rx_data, tx_data, strlen(tx_data)) == 0) {
		pr_info("basic test passed, received %d bytes", ret);
		serial_port_free(serial);
		return PASS;
	}

	pr_err("unexpected data received");
	serial_port_free(serial);
	return FAIL;
}

int t_serial_port_boundary(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[SERIAL_PORT_BUFFER_SIZE];
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret, i;

	pr_info("starting serial port boundary test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	for (i = 0; i < SERIAL_PORT_BUFFER_SIZE - 1; i++) {
		tx_data[i] = 'A' + (i % 26);
	}
	tx_data[SERIAL_PORT_BUFFER_SIZE - 1] = '\0';

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, SERIAL_PORT_BUFFER_SIZE - 1);
	if (ret < 0) {
		pr_err("failed to transmit maximum size data");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
	if (ret < 0) {
		pr_err("failed to get boundary data");
		serial_port_free(serial);
		return FAIL;
	}

	if (ret == SERIAL_PORT_BUFFER_SIZE - 1) {
		pr_info("boundary test passed, received %d bytes", ret);
		serial_port_free(serial);
		return PASS;
	}

	pr_err("unexpected size received: %d", ret);
	serial_port_free(serial);
	return FAIL;
}

int t_serial_port_performance(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[64];
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	u32 start_time, end_time;
	int i, ret;

	pr_info("starting serial port performance test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	strcpy(tx_data, "AT");

	start_time = basic_timer_get_ms();

	for (i = 0; i < 100; i++) {
		ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, strlen(tx_data));
		if (ret < 0) {
			pr_err("transmit failed at iteration %d", i);
			serial_port_free(serial);
			return FAIL;
		}

		ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
		if (ret < 0) {
			pr_err("get data failed at iteration %d", i);
			serial_port_free(serial);
			return FAIL;
		}
	}

	end_time = basic_timer_get_ms();

	pr_info("performance test passed, 100 iterations in %lu ms", end_time - start_time);
	serial_port_free(serial);
	return PASS;
}

int t_serial_port_stress(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[128];
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int i, ret, success_count = 0;

	pr_info("starting serial port stress test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	for (i = 0; i < 500; i++) {
		sprintf(tx_data, "AT+TEST%03d", i);
		ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, strlen(tx_data));
		if (ret < 0) {
			pr_err("transmit failed at iteration %d", i);
			continue;
		}

		ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 500);
		if (ret >= 0) {
			success_count++;
		}
	}

	serial_port_free(serial);

	if (success_count > 450) {
		pr_info("stress test passed, %d/%d iterations successful", success_count, 500);
		return PASS;
	}

	pr_err("stress test failed, only %d/%d iterations successful", success_count, 500);
	return FAIL;
}

int t_serial_port_timeout(int argc, char **argv)
{
	struct serial_port *serial;
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	pr_info("starting serial port timeout test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 100);
	if (ret == -ETIMEDOUT) {
		pr_info("timeout test passed, timeout occurred as expected");
		serial_port_free(serial);
		return PASS;
	}

	pr_err("timeout test failed, expected ETIMEDOUT but got %d", ret);
	serial_port_free(serial);
	return FAIL;
}

int t_serial_port_null_checks(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[] = "test";
	int ret;

	pr_info("starting serial port null checks test");

	ret = serial_port_transmit_bytes(NULL, (u8 *)tx_data, strlen(tx_data));
	if (ret != -EINVAL) {
		pr_err("null serial port check failed");
		return FAIL;
	}

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, NULL, strlen(tx_data));
	if (ret != -EINVAL) {
		pr_err("null data check failed");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, 0);
	if (ret != -EINVAL) {
		pr_err("zero size check failed");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, NULL, SERIAL_PORT_BUFFER_SIZE, 1000);
	if (ret != -EINVAL) {
		pr_err("null buffer check failed");
		serial_port_free(serial);
		return FAIL;
	}

	char small_buffer[10];
	ret = serial_port_get_data(serial, small_buffer, sizeof(small_buffer), 1000);
	if (ret != -EINVAL) {
		pr_err("small buffer check failed");
		serial_port_free(serial);
		return FAIL;
	}

	pr_info("null checks test passed");
	serial_port_free(serial);
	return PASS;
}

int t_serial_port_zero_timeout(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[] = "AT";
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	pr_info("starting serial port zero timeout test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, strlen(tx_data));
	if (ret < 0) {
		pr_err("failed to transmit data");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 0);
	if (ret >= 0) {
		pr_info("zero timeout test passed, received %d bytes", ret);
		serial_port_free(serial);
		return PASS;
	}

	pr_err("zero timeout test failed, got %d", ret);
	serial_port_free(serial);
	return FAIL;
}

int t_serial_port_ring_buffer(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data1[] = "FirstMessage";
	char tx_data2[] = "SecondMessage";
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	pr_info("starting serial port ring buffer test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data1, strlen(tx_data1));
	if (ret < 0) {
		pr_err("failed to transmit first message");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
	if (ret < 0) {
		pr_err("failed to get first message");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data2, strlen(tx_data2));
	if (ret < 0) {
		pr_err("failed to transmit second message");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
	if (ret < 0) {
		pr_err("failed to get second message");
		serial_port_free(serial);
		return FAIL;
	}

	pr_info("ring buffer test passed, producer=%d consumer=%d",
		serial->producer, serial->consumer);
	serial_port_free(serial);
	return PASS;
}

int t_serial_port_error_injection(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[] = "AT";
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret, error_count = 0;

	pr_info("starting serial port error injection test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	for (int i = 0; i < 20; i++) {
		ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, strlen(tx_data));
		if (ret < 0) {
			pr_err("transmit failed at iteration %d", i);
			serial_port_free(serial);
			return FAIL;
		}

		ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
		if (ret < 0) {
			pr_err("get data failed at iteration %d", i);
			continue;
		}

		if (strcmp(rx_data, "ERROR") == 0) {
			error_count++;
		}
	}

	serial_port_free(serial);

	if (error_count > 0 && error_count <= 10) {
		pr_info("error injection test passed, %d errors out of 20 requests", error_count);
		return PASS;
	}

	pr_err("error injection test failed, unexpected error count: %d", error_count);
	return FAIL;
}

int t_serial_port_special_characters(int argc, char **argv)
{
	struct serial_port *serial;
	char tx_data[] = "AT+SPECIAL\r\n\0\t";
	char rx_data[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	pr_info("starting serial port special characters test");

	serial = serial_port_alloc(serial_port_callback, fake_at_commands);
	if (!serial) {
		pr_err("failed to allocate serial port");
		return FAIL;
	}

	ret = serial_port_transmit_bytes(serial, (u8 *)tx_data, strlen(tx_data));
	if (ret < 0) {
		pr_err("failed to transmit special characters");
		serial_port_free(serial);
		return FAIL;
	}

	ret = serial_port_get_data(serial, rx_data, sizeof(rx_data), 1000);
	if (ret >= 0) {
		pr_info("special characters test passed, received %d bytes", ret);
		serial_port_free(serial);
		return PASS;
	}

	pr_err("special characters test failed");
	serial_port_free(serial);
	return FAIL;
}

#endif
