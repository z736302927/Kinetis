#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/iopoll.h>
#include <linux/irqflags.h>

#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/hc-05.h"
#include "kinetis/design_verification.h"

#define pr_fmt(fmt) "serial-port: " fmt

#define CONFIG_SERIAL_PORT_RING_BUFFER	1

#if KINETIS_FAKE_SIM
#include <unistd.h>

#define LOAD(value) 			__atomic_load_n(value, __ATOMIC_ACQUIRE)
#define STORE(value, next) 		__atomic_store_n(value, next, __ATOMIC_RELEASE)

static u16 serial_port_available_space(struct serial_port *serial)
{
	u16 producer = LOAD(&serial->producer);
	u16 consumer = LOAD(&serial->consumer);

	return (consumer + SERIAL_PORT_BUFFER_SIZE - producer - 1) % SERIAL_PORT_BUFFER_SIZE;
}

static u32 mini_random(struct communication_fault_state *fault)
{
	u32 value = fault->seed;
	
	if (!value)
		value = 1;
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	fault->seed = value;

	return value;
}

static enum communication_fault_mode communication_fault_pick(struct serial_port *serial)
{
	struct communication_fault_state *fault = serial->fault;
	enum communication_fault_mode mode;

	if (!fault)
		return COMMUNICATION_FAULT_NONE;

	mode = fault->mode;
	if (mode == COMMUNICATION_FAULT_NONE || !fault->remaining)
		return COMMUNICATION_FAULT_NONE;
	if (mini_random(fault) % 1000 >= fault->rate_per_mille)
		return COMMUNICATION_FAULT_NONE;
	if (mode == COMMUNICATION_FAULT_RANDOM)
		mode = (mini_random(fault) & 1) ?
			COMMUNICATION_FAULT_DROP : COMMUNICATION_FAULT_CORRUPT;

	fault->remaining--;
	fault->injected++;

	return mode;
}

int communication_fault_alloc(struct serial_port *serial,
	enum communication_fault_mode mode, u16 rate_per_mille,
	u16 count, u32 seed)
{
	serial->fault = kmalloc(sizeof(*serial->fault), GFP_KERNEL);
	if (!serial->fault)
		return -ENOMEM;

	serial->fault->seed = seed ? seed : 1;
	serial->fault->rate_per_mille = min(rate_per_mille, 1000u);
	serial->fault->remaining = count;
	serial->fault->injected = 0;
	serial->fault->mode = mode;

	return 0;
}

void communication_fault_free(struct serial_port *serial)
{
	kfree(serial->fault);
	serial->fault = NULL;
}

u16 communication_fault_injected(struct serial_port *serial)
{
	return serial->fault ? serial->fault->injected : 0;
}

void *serial_port_copy_to_self(void *para)
{
	struct serial_port *serial = (struct serial_port *)para;
	char response[SERIAL_PORT_BUFFER_SIZE];
	int i, index;

	while (LOAD(&serial->thread_switch)) {
		if (LOAD(&serial->transmitted_size) > 0) {
			size_t resp_len;

			response[0] = '\0';
			resp_len = serial->sim_callback(serial->tx_buffer, response, serial->private);

			while (LOAD(&serial->thread_switch) && serial_port_available_space(serial) < resp_len)
				usleep(100);
			if (!LOAD(&serial->thread_switch))
				break;

			index = LOAD(&serial->producer);
			for (i = 0; i < resp_len; i++) {
				serial->rx_buffer[index] = response[i];
				index = (index + 1) % SERIAL_PORT_BUFFER_SIZE;
			}
			serial->ops->update_producer(serial);
			STORE(&serial->transmitted_size, 0);
		} else {
			usleep(100);
		}
	}

	return NULL;
}

void *serial_port_copy_to_others(void *para)
{
	struct serial_port *serial_boy = (struct serial_port *)para;
	struct serial_port *serial_girl = (struct serial_port *)serial_boy->private;

	while (LOAD(&serial_boy->thread_switch)) {
		u16 size = LOAD(&serial_boy->transmitted_size);

		if (size > 0) {
			u16 pos;
			u16 tail;
			enum communication_fault_mode fault_mode = communication_fault_pick(serial_boy);

			if (fault_mode == COMMUNICATION_FAULT_DROP) {
				STORE(&serial_boy->transmitted_size, 0);
				continue;
			}

			while (LOAD(&serial_boy->thread_switch) && serial_port_available_space(serial_girl) < size)
				usleep(100);
			if (!LOAD(&serial_boy->thread_switch))
				break;

			pos = LOAD(&serial_girl->producer);
			tail = SERIAL_PORT_BUFFER_SIZE - pos;

			if (size <= tail) {
				memcpy(serial_girl->rx_buffer + pos, serial_boy->tx_buffer, size);
			} else {
				memcpy(serial_girl->rx_buffer + pos, serial_boy->tx_buffer, tail);
				memcpy(serial_girl->rx_buffer, serial_boy->tx_buffer + tail, size - tail);
			}

			if (fault_mode == COMMUNICATION_FAULT_CORRUPT) {
				u16 offset = mini_random(serial_boy->fault) % size;
				u8 bit = 1u << (mini_random(serial_boy->fault) % 8);

				serial_girl->rx_buffer[(pos + offset) % SERIAL_PORT_BUFFER_SIZE] ^= bit;
			}

			STORE(&serial_girl->producer, (pos + size) % SERIAL_PORT_BUFFER_SIZE);
			STORE(&serial_boy->transmitted_size, 0);
		} else {
			usleep(100);
		}
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
		STORE(&serial->thread_switch, 1);
		pthread_create(&serial->thread, NULL, serial_port_copy_to_self, serial);
		break;
	case SERIAL_PORT_DF_OTHERS:
		STORE(&serial->thread_switch, 1);
		pthread_create(&serial->thread, NULL, serial_port_copy_to_others, serial);
		break;
	default:
		break;
	}
}

void serial_port_stop_thread(struct serial_port *serial)
{
	if (serial->thread) {
		STORE(&serial->thread_switch, 0);
		pthread_join(serial->thread, NULL);
		serial->thread = 0;
	}
}
#else

#define LOAD(value) 			(*(value))
#define STORE(value, next) 		do { *(value) = (next); } while (0)

#endif

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

	if (!serial->ops || !serial->ops->init || !serial->ops->disable || !serial->ops->enable) {
		pr_err("serial_port_alloc: ops, init, disable, or enable is NULL\n");
		kfree(serial);
		return ERR_PTR(-EINVAL);
	}

	serial->ops->init(serial);
	serial->ops->disable();

	return serial;
}

void serial_port_free(struct serial_port *serial)
{
#ifdef KINETIS_FAKE_SIM
	serial_port_stop_thread(serial);
	communication_fault_free(serial);
#endif

	if (serial->name)
		kfree(serial->name);
	kfree(serial);
}

void serial_port_extract_data(struct serial_port *serial)
{
	unsigned long flags;
	u16 consumer = LOAD(&serial->consumer);
	u16 producer = LOAD(&serial->producer);

	serial->received_size = (producer + SERIAL_PORT_BUFFER_SIZE - consumer) % SERIAL_PORT_BUFFER_SIZE;
	serial->rx_complete = (serial->received_size > 0);
}

void serial_port_clear_rx(struct serial_port *serial)
{
	serial_port_extract_data(serial);

	for (int i = 0; i < serial->received_size; i++) {
		u16 consumer = LOAD(&serial->consumer);

		serial->rx_buffer[consumer] = '\0';
		STORE(&serial->consumer, (consumer + 1) % SERIAL_PORT_BUFFER_SIZE);
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
			u16 consumer = LOAD(&serial->consumer);

			buffer[i] = serial->rx_buffer[consumer];
			serial->rx_buffer[consumer] = '\0';
			STORE(&serial->consumer, (consumer + 1) % SERIAL_PORT_BUFFER_SIZE);
		}
		if (serial->ops->receive_callback) {
			serial->ops->receive_callback(serial);
		}
		serial->rx_complete = false;
#ifndef CONFIG_SERIAL_PORT_RING_BUFFER
		serial->producer = 0;
		serial->consumer = 0;
#endif
		if (serial->overflow) {
			if (serial_port_rx_used(serial) <= SERIAL_PORT_BUFFER_SIZE * 95 / 100) {
				serial->ops->enable();
				serial->overflow = 0;
			}
		}
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

#if KINETIS_FAKE_SIM
	while (LOAD(&serial->transmitted_size) > 0) {
		udelay(1);
	}
#endif

	memcpy(serial->tx_buffer, data, size);
	STORE(&serial->transmitted_size, size);
	snprintf(prefix, sizeof(prefix), "sp-%s tx: ", SP_NAME(serial));
// 	print_hex_dump(KERN_DEBUG, prefix, DUMP_PREFIX_OFFSET,
// 		16, 1,
// 		serial->tx_buffer, serial->transmitted_size, false);

	if (serial->ops->transmit_bytes)
		return serial->ops->transmit_bytes(serial->tx_buffer, size);

	return 0;
}

int serial_port_transmit_byte(struct serial_port *serial, u8 byte)
{
	char prefix[64];
	u16 tx_size;

#if KINETIS_FAKE_SIM
	while (LOAD(&serial->transmitted_size) > 0) {
		udelay(1);
	}
#endif

	serial->tx_buffer[0] = byte;
	STORE(&serial->transmitted_size, 1);
	snprintf(prefix, sizeof(prefix), "sp-%s tx: ", SP_NAME(serial));
// 	print_hex_dump(KERN_DEBUG, prefix, DUMP_PREFIX_OFFSET,
// 		16, 1,
// 		serial->tx_buffer, serial->transmitted_size, false);

	if (serial->ops->transmit_bytes)
		return serial->ops->transmit_bytes(serial->tx_buffer, 1);

	return 0;
}

bool serial_port_tx_idle(struct serial_port *serial)
{
#ifdef KINETIS_FAKE_SIM
	return LOAD(&serial->transmitted_size) == 0;
#else
	return true;
#endif
}

int serial_port_data_available(struct serial_port *serial)
{
	serial_port_extract_data(serial);
	return serial->rx_complete ? serial->received_size : 0;
}

bool serial_port_rx_empty(struct serial_port *serial)
{
	return LOAD(&serial->producer) == LOAD(&serial->consumer);
}

bool serial_port_rx_full(struct serial_port *serial)
{
	u16 producer = LOAD(&serial->producer);
	u16 consumer = LOAD(&serial->consumer);

	return ((producer + 1) % SERIAL_PORT_BUFFER_SIZE) == consumer;
}

u16 serial_port_rx_used(struct serial_port *serial)
{
	u16 producer = LOAD(&serial->producer);
	u16 consumer = LOAD(&serial->consumer);

	if (producer >= consumer)
		return producer - consumer;
	else
		return SERIAL_PORT_BUFFER_SIZE - (consumer - producer);
}

u16 serial_port_rx_free(struct serial_port *serial)
{
	u16 producer = LOAD(&serial->producer);
	u16 consumer = LOAD(&serial->consumer);

	/* Leave a space of one unit */
	if (producer >= consumer)
		return SERIAL_PORT_BUFFER_SIZE - (producer - consumer) - 1;
	else
		return (consumer - producer) - 1;
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

#ifdef KINETIS_FAKE_SIM
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

	serial = serial_port_alloc(&usart1_serial_port_ops, NULL);
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
#endif /* KINETIS_FAKE_SIM */

#endif
