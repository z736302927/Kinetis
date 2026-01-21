#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>

#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/hc-05.h"
#include "kinetis/design_verification.h"

#include <unistd.h>

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

const struct at_command *at_command_find(struct at_command *array, const char *command_name)
{
	int i;

	if (command_name == NULL) {
		return NULL;
	}

	for (i = 0; array[i].command != NULL; i++) {
		if (strcmp(array[i].command, command_name) == 0) {
			return &array[i];
		}
	}

	return NULL;
}

void *serial_port_monitor(void *para)
{
	struct serial_port *serial = (struct serial_port *)para;
	struct at_command *at_cmd;

	while (1) {
		if (serial->tx_buffer[0] != '\0') {
			pr_info("AT CMD: %s", serial->tx_buffer);
			at_cmd = at_command_find(serial->at_cmd_set, serial->tx_buffer);
			if (at_cmd != NULL)	{
				if (get_random_int() % 10 < 2) {
					for (int i = 0; i < strlen(at_cmd->error_response); i++) {
						serial->rx_buffer[serial->producer] = at_cmd->error_response[i];
						serial->producer = (serial->producer + 1) % SERIAL_PORT_BUFFER_SIZE;
					}
				} else {
					for (int i = 0; i < strlen(at_cmd->response); i++) {
						serial->rx_buffer[serial->producer] = at_cmd->response[i];
						serial->producer = (serial->producer + 1) % SERIAL_PORT_BUFFER_SIZE;
					}
				}
			} else {
				pr_err("Unknown AT command: %s\n", serial->tx_buffer);
			}
			memset(serial->tx_buffer, 0, serial->transmited_size);
		}

		usleep(1000);
	}

	return NULL;
}

void serial_port_init(struct serial_port *serial, struct at_command *at_cmd_set)
{
	serial->rx_complete = false;
	serial->at_cmd_set = at_cmd_set;

	pthread_create(&serial->thread, NULL, serial_port_monitor, serial);
}

void serial_port_deinit(struct serial_port *serial)
{
	if (serial->thread) {
		pthread_cancel(serial->thread);
		pthread_join(serial->thread, NULL);
		serial->thread = 0;
	}
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
	if (size > SERIAL_PORT_BUFFER_SIZE) {
		size = SERIAL_PORT_BUFFER_SIZE;
		pr_warn("Transmit size() too large, truncating to %d bytes", size, SERIAL_PORT_BUFFER_SIZE);
	}

	memcpy(serial->tx_buffer, data, size);
	serial->transmited_size = size;

	return 0;
}

#ifdef DESIGN_VERIFICATION_SEIRALPORT
#include "kinetis/test-kinetis.h"

static const struct at_command fake_at_commands[] = {
	{
		.command = "AT",
		.type = AT_CMD_TYPE_BASIC,
		.description = "Test communication with module",
		.params = "None",
		.default_value = "N/A",
		.response = "OK",
		.error_response = "ERROR"
	},
	{
		.command = "AT+VERSION",
		.type = AT_CMD_TYPE_READ,
		.description = "Query firmware version",
		.params = "None",
		.default_value = "N/A",
		.response = "+VERSION:1.0.0\r\nOK",
		.error_response = "ERROR"
	},
	{
		.command = "AT+NAME?",
		.type = AT_CMD_TYPE_READ,
		.description = "Query device name",
		.params = "None",
		.default_value = "FAKE_DEVICE",
		.response = "+NAME:FAKE_DEVICE\r\nOK",
		.error_response = "ERROR"
	},
	{
		.command = "AT+NAME",
		.type = AT_CMD_TYPE_WRITE,
		.description = "Set device name",
		.params = "<name> (max 20 characters)",
		.default_value = "FAKE_DEVICE",
		.response = "OK",
		.error_response = "ERROR"
	}
};

int t_serial_port_interactive(int argc, char **argv)
{
	struct serial_port *serial;
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial = kzalloc(sizeof(struct serial_port), GFP_KERNEL);
	if (serial == NULL) {
		return -ENOMEM;
	}

	serial_port_init(serial, fake_at_commands);

	for (int i = 0; i < ARRAY_SIZE(fake_at_commands); i++) {
		serial_port_transmit_bytes(serial, fake_at_commands[i].command, strlen(fake_at_commands[i].command));

		ret = serial_port_get_data(serial, at_ack, sizeof(at_ack), 1000);
		if (ret < 0) {
			return ret;
		}
		pr_info("AT ACK: %.*s", ret, at_ack);
	}

	return 0;
}

#endif
