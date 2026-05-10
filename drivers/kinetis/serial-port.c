#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/iopoll.h>

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

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void *serial_port_copy_to_self(void *para)
{
	struct serial_port *serial = (struct serial_port *)para;
	char response[SERIAL_PORT_BUFFER_SIZE];
	int i, index;

	while (serial->thread_switch) {
		if (serial->transmited_size > 0) {
			size_t resp_len;

			response[0] = '\0';
			resp_len = serial->sim_callback(serial->tx_buffer, response, serial->private);

			index = serial->producer;
			for (i = 0; i < resp_len; i++) {
				serial->rx_buffer[index] = response[i];
				index = (index + 1) % SERIAL_PORT_BUFFER_SIZE;
			}
			serial->ops->update_producer(serial);
			serial->transmited_size = 0;
		}

		usleep(1000);
	}

	return NULL;
}

void *serial_port_copy_to_others(void *para)
{
	struct serial_port *serial_boy = (struct serial_port *)para;
	struct serial_port *serial_girl = (struct serial_port *)serial_boy->private;

	while (serial_boy->thread_switch) {
		if (serial_boy->transmited_size > 0) {
			u16 size = serial_boy->transmited_size;
			u16 pos = serial_girl->producer;
			u16 tail = SERIAL_PORT_BUFFER_SIZE - pos;

			if (size <= tail) {
				memcpy(serial_girl->rx_buffer + pos, serial_boy->tx_buffer, size);
			} else {
				memcpy(serial_girl->rx_buffer + pos, serial_boy->tx_buffer, tail);
				memcpy(serial_girl->rx_buffer, serial_boy->tx_buffer + tail, size - tail);
			}
			serial_girl->producer = (pos + size) % SERIAL_PORT_BUFFER_SIZE;
			serial_boy->transmited_size = 0;
		}

		usleep(1000);
	}

	return NULL;
}

void serial_port_start_thread(struct serial_port *serial, u8 oneself,
	u32(*sim_callback)(char *request, char *response, void *context),
	void *private)
{
	serial->sim_callback = sim_callback;
	serial->private = private;

	switch (oneself) {
	case SERIAL_PORT_DF_SELF:
		serial->thread_switch = 1;
		pthread_create(&serial->thread, NULL, serial_port_copy_to_self, serial);
		break;
	case SERIAL_PORT_DF_OTHERS:
		serial->thread_switch = 1;
		pthread_create(&serial->thread, NULL, serial_port_copy_to_others, serial);
		break;
	default:
		break;
	}
}

void serial_port_stop_thread(struct serial_port *serial)
{
	if (serial->thread) {
		serial->thread_switch = 0;
		pthread_join(serial->thread, NULL);
		serial->thread = 0;
	}
}

struct serial_port *serial_port_alloc(struct serial_port_ops *ops, const char *name)
{
	struct serial_port *serial;

	serial = kzalloc(sizeof(struct serial_port), GFP_KERNEL);
	if (serial == NULL) {
		return ERR_PTR(-ENOMEM);
	}

	if (name)
		serial->name = kstrdup(name, GFP_KERNEL);

	serial->rx_complete = false;
	serial->ops = ops;

	return serial;
}

void serial_port_free(struct serial_port *serial)
{
	serial_port_stop_thread(serial);

	if (serial->name)
		kfree(serial->name);
	kfree(serial);
}

void serial_port_extract_data(struct serial_port *serial)
{
	serial->received_size = (serial->producer + SERIAL_PORT_BUFFER_SIZE - serial->consumer) % SERIAL_PORT_BUFFER_SIZE;
	serial->rx_complete = (serial->received_size > 0);
}

void serial_port_clear_rx(struct serial_port *serial)
{
	serial_port_extract_data(serial);

	for (int i = 0; i < serial->received_size; i++) {
		serial->rx_buffer[serial->consumer] = '\0';
		serial->consumer = (serial->consumer + 1) % SERIAL_PORT_BUFFER_SIZE;
	}
	serial->rx_complete = false;
	serial->received_size = 0;
}

int serial_port_receive_bytes(struct serial_port *serial, char *buffer, int size, u32 timeout_ms)
{
	u32 elapsed_ms = 0;

	if (buffer == NULL) {
		return -EINVAL;
	}

	if (size < SERIAL_PORT_BUFFER_SIZE) {
		pr_warn("serial_port_receive_bytes: buffer size %d is less than SERIAL_PORT_BUFFER_SIZE %d",
			size, SERIAL_PORT_BUFFER_SIZE);
		return -EINVAL;
	}

	if (timeout_ms == 0) {
		while (1) {
			serial_port_extract_data(serial);
			if (serial->rx_complete) {
				break;
			}
			mdelay(1);
		}
	} else {
		while (elapsed_ms < timeout_ms) {
			serial_port_extract_data(serial);
			if (serial->rx_complete) {
				break;
			}
			mdelay(1);
			elapsed_ms++;
		}
		if (elapsed_ms >= timeout_ms && !serial->rx_complete) {
			buffer[0] = '\0';
			pr_err("Cannot get data from serial port within %u ms", timeout_ms);
			return -ETIMEDOUT;
		}
	}

	if (serial->rx_complete) {
		for (int i = 0; i < serial->received_size; i++) {
			buffer[i] = serial->rx_buffer[serial->consumer];
			serial->rx_buffer[serial->consumer] = '\0';
			serial->consumer = (serial->consumer + 1) % SERIAL_PORT_BUFFER_SIZE;
		}
		serial->rx_complete = false;
#ifndef CONFIG_SERIAL_PORT_RING_BUFFER
		serial->producer = 0;
		serial->consumer = 0;
#endif
	}

	return serial->received_size;
}

int serial_port_transmit_bytes(struct serial_port *serial, const u8 *data, u16 size)
{
	char prefix[64];
	u16 tx_size;

	if (size >= SERIAL_PORT_BUFFER_SIZE) {
		size = SERIAL_PORT_BUFFER_SIZE - 1;
		pr_warn("Transmit size(%u) too large, truncating to %d bytes", size, SERIAL_PORT_BUFFER_SIZE - 1);
	}

	memcpy(serial->tx_buffer, data, size);
	serial->transmited_size = size;
	snprintf(prefix, sizeof(prefix), "sp-%s tx: ", SP_NAME(serial));
	print_hex_dump(KERN_DEBUG, prefix, DUMP_PREFIX_OFFSET,
		16, 1,
		serial->tx_buffer, serial->transmited_size, false);

	/* Wait for data to be transmitted (transmited_size goes to 0) */
	return readw_poll_timeout(&serial->transmited_size, tx_size, tx_size == 0, 100, 100000);
}

int serial_port_transmit_byte(struct serial_port *serial, u8 byte)
{
	char prefix[64];
	u16 tx_size;

	serial->tx_buffer[0] = byte;
	serial->transmited_size = 1;
	snprintf(prefix, sizeof(prefix), "sp-%s tx: ", SP_NAME(serial));
	print_hex_dump(KERN_DEBUG, prefix, DUMP_PREFIX_OFFSET,
		16, 1,
		serial->tx_buffer, serial->transmited_size, false);

	/* Wait for data to be transmitted (transmited_size goes to 0) */
	return readw_poll_timeout(&serial->transmited_size, tx_size, tx_size == 0, 100, 100000);
}

int serial_port_data_available(struct serial_port *serial)
{
	serial_port_extract_data(serial);
	return serial->rx_complete ? serial->received_size : 0;
}

int serial_port_config(struct serial_port *serial, u32 baud_rate, u8 parity, u8 data_bits, u8 flow_control)
{
	/* Configure serial port parameters
	 * baud_rate: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, etc
	 * parity: 0=None, 1=Odd, 2=Even
	 * data_bits: 7 or 8
	 * flow_control: 0=None, 1=XON/XOFF, 2=RTS/CTS
	 */

	/* Save current configuration */
	serial->baud_rate = baud_rate;
	serial->parity = parity;
	serial->data_bits = data_bits;
	serial->flow_control = flow_control;

	/* Call external callback to apply hardware configuration */
	if (serial->ops->config) {
		return serial->ops->config(serial, baud_rate, parity, data_bits, flow_control);
	}

	return 0;
}

void serial_port_send_break(struct serial_port *serial)
{
	serial->ops->irq_disable();
	serial->ops->set_tx(0);
	mdelay(250);
	serial->ops->set_tx(1);
	serial->ops->irq_enable();
}

#ifdef DESIGN_VERIFICATION_SERIALPORT
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

static int fake_serial_transmit_bytes(const u8 *data, u16 size)
{
	pr_debug("fake tx: %u bytes", size);
	print_hex_dump(KERN_DEBUG, "fake serial tx: ", DUMP_PREFIX_OFFSET,
		16, 1, data, size, false);
	return 0;
}

static int fake_serial_receive_bytes(u8 *data, u16 size, u32 timeout_ms)
{
	pr_debug("fake rx: request %u bytes, timeout %u ms", size, timeout_ms);
	return 0;
}

static void fake_serial_update_producer(struct serial_port *serial)
{
	serial->producer = (serial->producer + serial->transmited_size) % SERIAL_PORT_BUFFER_SIZE;
	pr_debug("fake update producer: prod=%d cons=%d",
		serial->producer, serial->consumer);
}

static int fake_serial_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	pr_info("fake serial config: baud=%u parity=%u data_bits=%u flow=%u",
		baud_rate, parity, data_bits, flow_control);
	return 0;
}

static void fake_serial_irq_disable(void)
{
	pr_debug("fake irq disable");
}

static void fake_serial_irq_enable(void)
{
	pr_debug("fake irq enable");
}

static void fake_serial_set_tx(u8 state)
{
	pr_debug("fake set tx: %s", state ? "high" : "low");
}

struct serial_port_ops fake_serial_port_ops = {
	.transmit_bytes  = fake_serial_transmit_bytes,
	.receive_bytes   = fake_serial_receive_bytes,
	.update_producer  = fake_serial_update_producer,
	.config          = fake_serial_config,
	.irq_disable     = fake_serial_irq_disable,
	.irq_enable      = fake_serial_irq_enable,
	.set_tx          = fake_serial_set_tx,
};

u32 serial_port_callback(char *request, char *response, void *context)
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

	return i;
}

int t_serial_port_interactive(int argc, char **argv)
{
	struct serial_port *serial;
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial = serial_port_alloc(&fake_serial_port_ops, NULL);
	serial_port_start_thread(serial, SERIAL_PORT_DF_SELF, serial_port_callback, fake_at_commands);

	for (int i = 0; i < ARRAY_SIZE(fake_at_commands); i++) {
		serial_port_transmit_bytes(serial, fake_at_commands[i].request, strlen(fake_at_commands[i].request));

		ret = serial_port_receive_bytes(serial, at_ack, sizeof(at_ack), 1000);
		if (ret < 0) {
			return ret;
		}
		pr_info("AT ACK: %.*s", ret, at_ack);
	}

	serial_port_free(serial);

	return 0;
}

#endif
