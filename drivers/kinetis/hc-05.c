#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "kinetis/hc-05.h"
#include "kinetis/general.h"
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

/* HC-05 Virtual AT Commands for Simulation */
static const struct virtual_at_command hc_05_at_commands[] = {
	/* Basic Commands */
	{ .request = "AT", .response = "OK" },
	{ .request = "AT+RESET", .response = "OK" },
	{ .request = "AT+ORGL", .response = "OK" },
	{ .request = "AT+VERSION?", .response = "+VERSION:2.0-20100601\r\nOK" },
	{ .request = "AT+NAME?", .response = "+NAME:HC-05-Default\r\nOK" },
	{ .request = "AT+NAME", .response = "OK" },

	/* UART Configuration */
	{ .request = "AT+UART?", .response = "+UART:9600,0,0\r\nOK" },
	{ .request = "AT+UART=9600,0,0", .response = "OK" },
	{ .request = "AT+UART=115200,0,0", .response = "OK" },
	{ .request = "AT+UART=38400,0,0", .response = "OK" },
	{ .request = "AT+UART=57600,0,0", .response = "OK" },

	/* Role Configuration */
	{ .request = "AT+ROLE?", .response = "+ROLE:0\r\nOK" },
	{ .request = "AT+ROLE=0", .response = "OK" },
	{ .request = "AT+ROLE=1", .response = "OK" },
	{ .request = "AT+ROLE=2", .response = "OK" },

	/* Connection Mode */
	{ .request = "AT+CMODE?", .response = "+CMODE:1\r\nOK" },
	{ .request = "AT+CMODE=0", .response = "OK" },
	{ .request = "AT+CMODE=1", .response = "OK" },

	/* Device Address */
	{ .request = "AT+ADDR?", .response = "+ADDR:98d3:32:70a0b8\r\nOK" },

	/* Password/PIN */
	{ .request = "AT+PSWD?", .response = "+PSWD:1234\r\nOK" },
	{ .request = "AT+PSWD=1234", .response = "OK" },

	/* Pairing/Binding */
	{ .request = "AT+BIND?", .response = "+BIND:98d3:32:70a0b8\r\nOK" },
	{ .request = "AT+BIND", .response = "OK" },

	/* Connection State */
	{ .request = "AT+STATE?", .response = "+STATE:INITIALIZED\r\nOK" },
	{ .request = "AT+STATE?", .response = "+STATE:CONNECTED\r\nOK" },
	{ .request = "AT+STATE?", .response = "+STATE:DISCONNECTED\r\nOK" },

	/* Paired Device Count */
	{ .request = "AT+ADCN?", .response = "+ADCN:1\r\nOK" },
	{ .request = "AT+ADCN?", .response = "+ADCN:3\r\nOK" },

	/* Link Mode */
	{ .request = "AT+LINK?", .response = "+LINK:98d3:32:70a0b8\r\nOK" },

	/* Inquire/Scan */
	{ .request = "AT+INQ", .response = "OK" },
	{ .request = "AT+INQC", .response = "OK" },

	/* Connect */
	{ .request = "AT+CONNECT", .response = "OK" },

	/* RMAAD - Remove All Authenticated Devices */
	{ .request = "AT+RMAAD", .response = "OK" },

	/* RNAD - Remove Authenticated Device */
	{ .request = "AT+RNAD", .response = "OK" },

	/* FSAD - Find Authenticated Device */
	{ .request = "AT+FSAD", .response = "OK" },

	/* MRAD - Multi-point Role Authenticated Device */
	{ .request = "AT+MRAD", .response = "OK" },

	/* IPSCAN */
	{ .request = "AT+IPSCAN?", .response = "+IPSCAN:1024,512,48,18\r\nOK" },
	{ .request = "AT+IPSCAN=1024,512,48,18", .response = "OK" },

	/* SENI */
	{ .request = "AT+SENI?", .response = "+SENI:0\r\nOK" },
	{ .request = "AT+SENI=0", .response = "OK" },

	/* SENA */
	{ .request = "AT+SENA?", .response = "+SENA:0\r\nOK" },
	{ .request = "AT+SENA=0", .response = "OK" },

	/* IAC */
	{ .request = "AT+IAC?", .response = "+IAC:9e8b33\r\nOK" },
	{ .request = "AT+IAC=9e8b33", .response = "OK" },

	/* CLASS */
	{ .request = "AT+CLASS?", .response = "+CLASS:1f00\r\nOK" },
	{ .request = "AT+CLASS=1f00", .response = "OK" },

	/* IACM */
	{ .request = "AT+IACM?", .response = "+IACM:1\r\nOK" },
	{ .request = "AT+IACM=1", .response = "OK" },

	/* PNM - Pair Name */
	{ .request = "AT+PNM?", .response = "+PNM:1\r\nOK" },
	{ .request = "AT+PNM=1", .response = "OK" },

	/* TIAM - Timer Inquiry Access Mode */
	{ .request = "AT+TIAM?", .response = "+TIAM:0\r\nOK" },
	{ .request = "AT+TIAM=0", .response = "OK" },

	/* PIO - PIO Status */
	{ .request = "AT+PIO?", .response = "+PIO:0\r\nOK" },

	/* MPIO - Multi PIO */
	{ .request = "AT+MPIO?", .response = "+MPIO:0\r\nOK" },

	/* MPSWITCH */
	{ .request = "AT+MPSWITCH?", .response = "+MPSWITCH:0\r\nOK" },

	/* UARTMODE */
	{ .request = "AT+UARTMODE?", .response = "+UARTMODE:0\r\nOK" },
	{ .request = "AT+UARTMODE=0", .response = "OK" },

	/* ENAPWD - Enable Password */
	{ .request = "AT+ENAPWD?", .response = "+ENAPWD:0\r\nOK" },
	{ .request = "AT+ENAPWD=0", .response = "OK" },

	/* DEFAULT */
	{ .request = NULL, .response = NULL }
};

static int hc_05_detect_device(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT", strlen("AT"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		pr_err("HC-05 not responding correctly, received: %.*s", ret, at_ack);
		return -ENODEV;
	}

	return 0;
}

/* Basic Commands */

static int hc_05_test(struct hc_05_device *device)
{
	return hc_05_detect_device(device);
}

int hc_05_reset(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+RESET", strlen("AT+RESET"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_version(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+VERSION?", strlen("AT+VERSION?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_restore_factory(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+ORGL", strlen("AT+ORGL"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

/* Configuration Commands */

static int hc_05_set_name(struct hc_05_device *device, const char *name)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+NAME=%s", name);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_name(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+NAME?", strlen("AT+NAME?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_password(struct hc_05_device *device, const char *password)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+PSWD=%s", password);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_password(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+PSWD?", strlen("AT+PSWD?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_uart(struct hc_05_device *device, int baudrate, int stopbit, int parity)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+UART=%d,%d,%d", baudrate, stopbit, parity);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_uart(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+UART?", strlen("AT+UART?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_role(struct hc_05_device *device, int role)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+ROLE=%d", role);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_role(struct hc_05_device *device, int *role)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+ROLE?", strlen("AT+ROLE?"));

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), 1000);
	if (ret < 0) {
		return ret;
	}

	if (sscanf(buffer, "+ROLE:%d", role) != 1) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_set_cmode(struct hc_05_device *device, int mode)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+CMODE=%d", mode);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_cmode(struct hc_05_device *device, int *mode)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+CMODE?", strlen("AT+CMODE?"));

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), 1000);
	if (ret < 0) {
		return ret;
	}

	if (sscanf(buffer, "+CMODE:%d", mode) != 1) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_set_bind(struct hc_05_device *device, int nap, int uap, int lap)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+BIND=%d,%d,%d", nap, uap, lap);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_bind(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+BIND?", strlen("AT+BIND?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_iac(struct hc_05_device *device, const char *iac)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+IAC=%s", iac);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_iac(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+IAC?", strlen("AT+IAC?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_class(struct hc_05_device *device, int class)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+CLASS=%d", class);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_class(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+CLASS?", strlen("AT+CLASS?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_inqm(struct hc_05_device *device, int mode, int max, int timeout)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+INQM=%d,%d,%d", mode, max, timeout);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_inqm(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+INQM?", strlen("AT+INQM?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_ipscan(struct hc_05_device *device, int interval, int window, int slot, int mode)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+IPSCAN=%d,%d,%d,%d", interval, window, slot, mode);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_ipscan(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+IPSCAN?", strlen("AT+IPSCAN?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_sniff(struct hc_05_device *device, int max_interval, int min_interval, int attempt, int timeout)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+SNIFF=%d,%d,%d,%d", max_interval, min_interval, attempt, timeout);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_sniff(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+SNIFF?", strlen("AT+SNIFF?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_set_pio(struct hc_05_device *device, int pio)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+PIO=%d", pio);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_pio(struct hc_05_device *device, int *pio)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+PIO?", strlen("AT+PIO?"));

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), 1000);
	if (ret < 0) {
		return ret;
	}

	if (sscanf(buffer, "+PIO:%d", pio) != 1) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_set_mpio(struct hc_05_device *device, int mpio)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+MPIO=%d", mpio);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_mpio(struct hc_05_device *device, int *mpio)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+MPIO?", strlen("AT+MPIO?"));

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), 1000);
	if (ret < 0) {
		return ret;
	}

	if (sscanf(buffer, "+MPIO:%d", mpio) != 1) {
		return -EINVAL;
	}

	return 0;
}

/* Status Query Commands */

static int hc_05_get_state(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+STATE?", strlen("AT+STATE?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_get_addr(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+ADDR?", strlen("AT+ADDR?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_get_adcn(struct hc_05_device *device, int *count)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+ADCN?", strlen("AT+ADCN?"));

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), 1000);
	if (ret < 0) {
		return ret;
	}

	if (sscanf(buffer, "+ADCN:%d", count) != 1) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_rname(struct hc_05_device *device, const char *addr, char *buffer, int size)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+RNAME?%s", addr);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_get_rssi(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+RSSI?", strlen("AT+RSSI?"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

/* Operation Commands */

static int hc_05_init_spp(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+INIT", strlen("AT+INIT"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_inquiry(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+INQ", strlen("AT+INQ"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_inquiry_cancel(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+INQC", strlen("AT+INQC"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_pair(struct hc_05_device *device, const char *addr, int timeout)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+PAIR=%s,%d", addr, timeout);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 10000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_connect(struct hc_05_device *device, const char *addr)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+LINK=%s", addr);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 10000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

int hc_05_disconnect(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+DISC", strlen("AT+DISC"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_remove_all_devices(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+RMAAD", strlen("AT+RMAAD"));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_remove_device(struct hc_05_device *device, const char *addr)
{
	char cmd[SERIAL_PORT_BUFFER_SIZE];
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	snprintf(cmd, sizeof(cmd), "AT+FSAD=%s", addr);

	serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));

	ret = serial_port_get_data(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int hc_05_get_mraddr(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	serial_port_transmit_bytes(device->serial, "AT+MRAD", strlen("AT+MRAD"));

	ret = serial_port_get_data(device->serial, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

/* Application Functions */

int hc_05_quick_setup(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("HC-05 Quick Setup - Name: %s, Password: %s, Baudrate: %d\n", name, password, baudrate);

	ret = hc_05_set_name(device, name);
	if (ret) {
		pr_err("Failed to set name: %s\n", name);
		return ret;
	}

	ret = hc_05_set_password(device, password);
	if (ret) {
		pr_err("Failed to set password: %s\n", password);
		return ret;
	}

	ret = hc_05_set_uart(device, baudrate, 0, 0);
	if (ret) {
		pr_err("Failed to set uart: %d\n", baudrate);
		return ret;
	}

	ret = hc_05_reset(device);
	if (ret) {
		pr_err("Failed to reset device\n");
		return ret;
	}

	pr_info("HC-05 Quick Setup completed successfully\n");
	return 0;
}

int hc_05_setup_master(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("HC-05 Master Setup - Name: %s\n", name);

	ret = hc_05_quick_setup(device, name, password, baudrate);
	if (ret) {
		return ret;
	}

	ret = hc_05_set_role(device, 1);
	if (ret) {
		pr_err("Failed to set master role\n");
		return ret;
	}

	ret = hc_05_set_cmode(device, 0);
	if (ret) {
		pr_err("Failed to set connection mode\n");
		return ret;
	}

	pr_info("HC-05 Master Setup completed successfully\n");
	return 0;
}

int hc_05_setup_slave(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("HC-05 Slave Setup - Name: %s\n", name);

	ret = hc_05_quick_setup(device, name, password, baudrate);
	if (ret) {
		return ret;
	}

	ret = hc_05_set_role(device, 0);
	if (ret) {
		pr_err("Failed to set slave role\n");
		return ret;
	}

	ret = hc_05_set_cmode(device, 1);
	if (ret) {
		pr_err("Failed to set connection mode\n");
		return ret;
	}

	pr_info("HC-05 Slave Setup completed successfully\n");
	return 0;
}

int hc_05_auto_connect(struct hc_05_device *device, const char *target_addr, int timeout)
{
	int ret;

	pr_info("HC-05 Auto Connect to: %s\n", target_addr);

	ret = hc_05_init_spp(device);
	if (ret) {
		pr_err("Failed to initialize SPP\n");
		return ret;
	}

	ret = hc_05_pair(device, target_addr, timeout);
	if (ret) {
		pr_err("Failed to pair with device: %s\n", target_addr);
		return ret;
	}

	ret = hc_05_connect(device, target_addr);
	if (ret) {
		pr_err("Failed to connect to device: %s\n", target_addr);
		return ret;
	}

	pr_info("HC-05 Connected to: %s successfully\n", target_addr);
	return 0;
}

int hc_05_scan_and_connect(struct hc_05_device *device, const char *target_name, int max_devices, int timeout)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	char scan_cmd[64];
	int ret;

	pr_info("HC-05 Scan and Connect - Target: %s, Max devices: %d\n", target_name, max_devices);

	ret = hc_05_init_spp(device);
	if (ret) {
		pr_err("Failed to initialize SPP\n");
		return ret;
	}

	ret = hc_05_set_inqm(device, 0, max_devices, timeout);
	if (ret) {
		pr_err("Failed to set inquiry mode\n");
		return ret;
	}

	ret = hc_05_inquiry(device);
	if (ret) {
		pr_err("Failed to start inquiry\n");
		return ret;
	}

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), (timeout + 2) * 1000);
	if (ret < 0) {
		pr_warn("No devices found or timeout\n");
		return ret;
	}

	pr_info("Found devices:\n%.*s\n", ret, buffer);

	snprintf(scan_cmd, sizeof(scan_cmd), "AT+RNAME?%s", target_name);
	serial_port_transmit_bytes(device->serial, scan_cmd, strlen(scan_cmd));

	ret = serial_port_get_data(device->serial, buffer, sizeof(buffer), 5000);
	if (ret < 0) {
		pr_err("Failed to get device name\n");
		return ret;
	}

	if (strstr(buffer, target_name) == NULL) {
		pr_err("Target device not found\n");
		return -ENODEV;
	}

	pr_info("Target device found\n");
	return 0;
}

int hc_05_factory_reset_and_setup(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("HC-05 Factory Reset and Setup\n");

	ret = hc_05_restore_factory(device);
	if (ret) {
		pr_err("Failed to restore factory settings\n");
		return ret;
	}

	mdelay(1000);

	ret = hc_05_quick_setup(device, name, password, baudrate);
	if (ret) {
		pr_err("Failed to setup after factory reset\n");
		return ret;
	}

	pr_info("HC-05 Factory Reset and Setup completed successfully\n");
	return 0;
}

int hc_05_get_device_info(struct hc_05_device *device, char *buffer, int size)
{
	char temp[SERIAL_PORT_BUFFER_SIZE];
	int pos = 0;
	int role;
	int count;
	int mode;
	int ret;

	pr_info("Getting HC-05 device information\n");

	ret = snprintf(buffer + pos, size - pos, "=== HC-05 Device Information ===\n");
	pos += ret;

	ret = hc_05_get_version(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "Version: %.*s\n", ret, temp);
		pos += ret;
	}

	ret = hc_05_get_name(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "Name: %.*s\n", ret, temp);
		pos += ret;
	}

	ret = hc_05_get_addr(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "Address: %.*s\n", ret, temp);
		pos += ret;
	}

	ret = hc_05_get_password(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "Password: %.*s\n", ret, temp);
		pos += ret;
	}

	ret = hc_05_get_uart(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "UART: %.*s\n", ret, temp);
		pos += ret;
	}

	ret = hc_05_get_role(device, &role);
	if (ret == 0) {
		ret = snprintf(buffer + pos, size - pos, "Role: %d\n", role);
		pos += ret;
	}

	ret = hc_05_get_cmode(device, &mode);
	if (ret == 0) {
		ret = snprintf(buffer + pos, size - pos, "Connection Mode: %d\n", mode);
		pos += ret;
	}

	ret = hc_05_get_state(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "State: %.*s\n", ret, temp);
		pos += ret;
	}

	ret = hc_05_get_adcn(device, &count);
	if (ret == 0) {
		ret = snprintf(buffer + pos, size - pos, "Paired Devices: %d\n", count);
		pos += ret;
	}

	snprintf(buffer + pos, size - pos, "================================\n");

	pr_info("Device information retrieved\n");
	return pos;
}

int hc_05_diagnostics(struct hc_05_device *device, char *buffer, int size)
{
	char temp[SERIAL_PORT_BUFFER_SIZE];
	int pos = 0;
	int ret;
	int role, pio, mpio;

	pr_info("Running HC-05 diagnostics\n");

	ret = snprintf(buffer + pos, size - pos, "=== HC-05 Diagnostics ===\n");
	pos += ret;

	if (hc_05_test(device) == 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] Device responds to AT command\n");
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Device does not respond\n");
	}
	pos += ret;

	ret = hc_05_get_version(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] Version: %.*s\n", ret, temp);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read version\n");
	}
	pos += ret;

	ret = hc_05_get_state(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] State: %.*s\n", ret, temp);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read state\n");
	}
	pos += ret;

	ret = hc_05_get_role(device, &role);
	if (ret == 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] Role: %d\n", role);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read role\n");
	}
	pos += ret;

	ret = hc_05_get_pio(device, &pio);
	if (ret == 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] PIO: %d\n", pio);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read PIO\n");
	}
	pos += ret;

	ret = hc_05_get_mpio(device, &mpio);
	if (ret == 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] MPIO: %d\n", mpio);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read MPIO\n");
	}
	pos += ret;

	ret = hc_05_get_addr(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] Address: %.*s\n", ret, temp);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read address\n");
	}
	pos += ret;

	ret = hc_05_get_uart(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] UART: %.*s\n", ret, temp);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[FAIL] Cannot read UART config\n");
	}
	pos += ret;

	ret = hc_05_get_bind(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = snprintf(buffer + pos, size - pos, "[OK] Bind address: %.*s\n", ret, temp);
	} else {
		ret = snprintf(buffer + pos, size - pos, "[WARN] Cannot read bind address\n");
	}
	pos += ret;

	snprintf(buffer + pos, size - pos, "=========================\n");

	pr_info("Diagnostics completed\n");
	return pos;
}

/* Data Transfer Functions */

int hc_05_send_data(struct hc_05_device *device, const u8 *data, u32 size, u16 packet_size, u32 delay_ms)
{
	int ret;
	u32 offset = 0;

	if (device == NULL || data == NULL || size == 0) {
		pr_err("Invalid parameters\n");
		return -EINVAL;
	}

	if (packet_size == 0 || packet_size > size) {
		packet_size = size;
	}

	while (offset < size) {
		u32 chunk_size = (packet_size > (size - offset)) ? (size - offset) : packet_size;

		ret = serial_port_transmit_bytes(device->serial, data + offset, chunk_size);
		if (ret) {
			pr_err("Failed to send data at offset %u\n", offset);
			return ret;
		}

		offset += chunk_size;

		if (offset < size && delay_ms > 0) {
			mdelay(delay_ms);
		}
	}

	pr_debug("Sent %u bytes in chunks\n", size);
	return size;
}

int hc_05_send_string(struct hc_05_device *device, const char *str, u16 packet_size, u32 delay_ms)
{
	int len;

	if (device == NULL || str == NULL) {
		pr_err("Invalid parameters\n");
		return -EINVAL;
	}

	len = strlen(str);
	if (len == 0) {
		return 0;
	}

	return hc_05_send_data(device, (const u8 *)str, len, packet_size, delay_ms);
}

int hc_05_receive_data(struct hc_05_device *device, u8 *buffer, int size, u32 timeout_ms)
{
	int ret;

	if (device == NULL || buffer == NULL || size <= 0) {
		pr_err("Invalid parameters\n");
		return -EINVAL;
	}

	ret = serial_port_get_data(device->serial, (char *)buffer, size, timeout_ms);
	if (ret < 0) {
		pr_err("Failed to receive data: %d\n", ret);
		return ret;
	}

	if (ret > 0) {
		pr_debug("Received %d bytes\n", ret);
	}

	return ret;
}

int hc_05_receive_string(struct hc_05_device *device, char *buffer, int size, u32 timeout_ms)
{
	int ret;

	if (device == NULL || buffer == NULL || size <= 0) {
		pr_err("Invalid parameters\n");
		return -EINVAL;
	}

	ret = serial_port_get_data(device->serial, buffer, size, timeout_ms);
	if (ret < 0) {
		pr_err("Failed to receive string: %d\n", ret);
		return ret;
	}

	if (ret < size) {
		buffer[ret] = '\0';
	} else {
		buffer[size - 1] = '\0';
	}

	pr_debug("Received string: %s\n", buffer);
	return ret;
}

int hc_05_send_and_wait(struct hc_05_device *device, const u8 *data, u32 size, u8 *response, int resp_size, u32 timeout_ms)
{
	int ret;

	if (device == NULL || data == NULL || size == 0) {
		pr_err("Invalid send parameters\n");
		return -EINVAL;
	}

	ret = hc_05_send_data(device, data, size, 0, 0);
	if (ret < 0) {
		return ret;
	}

	if (response != NULL && resp_size > 0) {
		ret = hc_05_receive_data(device, response, resp_size, timeout_ms);
		if (ret < 0) {
			pr_err("Failed to receive response\n");
			return ret;
		}
	}

	return ret;
}

int hc_05_wait_for_data(struct hc_05_device *device, int expected_bytes, u32 timeout_ms)
{
	u8 buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	if (device == NULL || expected_bytes <= 0) {
		pr_err("Invalid parameters\n");
		return -EINVAL;
	}

	ret = serial_port_get_data(device->serial, (char *)buffer, sizeof(buffer), timeout_ms);
	if (ret < 0) {
		pr_err("Failed to wait for data\n");
		return ret;
	}

	if (ret < expected_bytes) {
		pr_warn("Received %d bytes, expected %d\n", ret, expected_bytes);
		return -ETIMEDOUT;
	}

	return ret;
}

int hc_05_flush_rx_buffer(struct hc_05_device *device)
{
	u8 buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	if (device == NULL) {
		pr_err("Invalid parameter\n");
		return -EINVAL;
	}

	ret = serial_port_get_data(device->serial, (char *)buffer, sizeof(buffer), 10);
	if (ret > 0) {
		pr_debug("Flushed %d bytes from RX buffer\n", ret);
	}

	return 0;
}

int hc_05_clear_buffers(struct hc_05_device *device)
{
	if (device == NULL) {
		pr_err("Invalid parameter\n");
		return -EINVAL;
	}

	hc_05_flush_rx_buffer(device);

	memset(device->serial->rx_buffer, 0, SERIAL_PORT_BUFFER_SIZE);
	memset(device->serial->tx_buffer, 0, SERIAL_PORT_BUFFER_SIZE);
	device->serial->producer = 0;
	device->serial->consumer = 0;
	device->serial->received_size = 0;
	device->serial->transmited_size = 0;

	pr_debug("Cleared all buffers\n");
	return 0;
}

struct hc_05_device *hc_05_alloc()
{
	struct hc_05_device *device;
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	device = kzalloc(sizeof(struct hc_05_device), GFP_KERNEL);
	if (device == NULL) {
		return ERR_PTR(-ENOMEM);
	}

	device->serial = serial_port_alloc(hc_05_at_commands);

	serial_port_transmit_bytes(device->serial, "AT", strlen("AT"));

	ret = hc_05_detect_device(device);
	if (ret) {
		return ERR_PTR(ret);
	}

	return device;
}

void hc_05_exit(struct hc_05_device *device)
{
	serial_port_free(device->serial);
	kfree(device);
}

// void hc_05_test_cmd(void)
// {
//     struct general_cmd cmd;
//
//     cmd.at_cmd = "AT";
//     cmd.property = AT_NONE;
//     cmd.argu = NULL;
//     cmd.expect_res = "OK";
//     cmd.error_repetition = 3;
//     cmd.wait_time = 1000;
//     cmd.interval = 1000;
//
//     general_process_cmd(&cmd);
//
//     if (cmd.error_flag == true && cmd.error_repetition == 0) {
//
//     }
//
//     if (cmd.timeout_flag == true) {
//
//     }
// }

#ifdef DESIGN_VERIFICATION_HC_05
#include "kinetis/test-kinetis.h"

int t_hc_05_test_cmd(int argc, char **argv)
{
	struct hc_05_device *device;
	int ret;

	pr_info("=== HC-05 AT Command Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_test(device);
	if (ret) {
		pr_err("AT test failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	ret = hc_05_get_version(device, (char *)argv[1], 32);
	if (ret < 0) {
		pr_err("Get version failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	ret = hc_05_reset(device);
	if (ret) {
		pr_err("Reset failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 AT Command Test PASSED ===\n");
	return PASS;
}

int t_hc_05_setup(int argc, char **argv)
{
	struct hc_05_device *device;
	int ret;

	pr_info("=== HC-05 Setup Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_quick_setup(device, "TestDevice", "1234", 9600);
	if (ret) {
		pr_err("Quick setup failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 Setup Test PASSED ===\n");
	return PASS;
}

int t_hc_05_slave_mode(int argc, char **argv)
{
	struct hc_05_device *device;
	int ret;

	pr_info("=== HC-05 Slave Mode Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_setup_slave(device, "HC05-Slave", "1234", 115200);
	if (ret) {
		pr_err("Slave setup failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 Slave Mode Test PASSED ===\n");
	return PASS;
}

int t_hc_05_master_mode(int argc, char **argv)
{
	struct hc_05_device *device;
	int ret;

	pr_info("=== HC-05 Master Mode Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_setup_master(device, "HC05-Master", "1234", 115200);
	if (ret) {
		pr_err("Master setup failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 Master Mode Test PASSED ===\n");
	return PASS;
}

int t_hc_05_data_transfer(int argc, char **argv)
{
	struct hc_05_device *device;
	u8 tx_data[128];
	u8 rx_data[128];
	int ret;
	int i;

	pr_info("=== HC-05 Data Transfer Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_quick_setup(device, "DataTest", "1234", 9600);
	if (ret) {
		pr_err("Setup failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	for (i = 0; i < 128; i++) {
		tx_data[i] = (u8)i;
	}

	ret = hc_05_send_data(device, tx_data, 128, 64, 10);
	if (ret < 0) {
		pr_err("Send data failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	ret = hc_05_send_string(device, "Hello, HC-05!", 0, 0);
	if (ret < 0) {
		pr_err("Send string failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 Data Transfer Test PASSED ===\n");
	return PASS;
}

int t_hc_05_device_info(int argc, char **argv)
{
	struct hc_05_device *device;
	char info_buffer[512];
	int ret;

	pr_info("=== HC-05 Device Info Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_quick_setup(device, "InfoTest", "1234", 9600);
	if (ret) {
		pr_err("Setup failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	ret = hc_05_get_device_info(device, info_buffer, sizeof(info_buffer));
	if (ret < 0) {
		pr_err("Get device info failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	pr_info("Device Info:\n%s\n", info_buffer);

	hc_05_exit(device);

	pr_info("=== HC-05 Device Info Test PASSED ===\n");
	return PASS;
}

int t_hc_05_diagnostics(int argc, char **argv)
{
	struct hc_05_device *device;
	char diag_buffer[512];
	int ret;

	pr_info("=== HC-05 Diagnostics Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_diagnostics(device, diag_buffer, sizeof(diag_buffer));
	if (ret < 0) {
		pr_err("Diagnostics failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	pr_info("Diagnostics:\n%s\n", diag_buffer);

	hc_05_exit(device);

	pr_info("=== HC-05 Diagnostics Test PASSED ===\n");
	return PASS;
}

int t_hc_05_factory_reset(int argc, char **argv)
{
	struct hc_05_device *device;
	int ret;

	pr_info("=== HC-05 Factory Reset Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	ret = hc_05_factory_reset_and_setup(device, "NewDevice", "5678", 9600);
	if (ret) {
		pr_err("Factory reset and setup failed\n");
		hc_05_exit(device);
		return FAIL;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 Factory Reset Test PASSED ===\n");
	return PASS;
}

int t_hc_05_full_test(int argc, char **argv)
{
	struct hc_05_device *device;
	char buffer[512];
	int ret;

	pr_info("=== HC-05 Full Test ===\n");

	device = hc_05_alloc();
	if (device == NULL) {
		pr_err("Failed to allocate HC-05 device\n");
		return FAIL;
	}

	pr_info("Test 1: Factory Reset and Setup\n");
	ret = hc_05_factory_reset_and_setup(device, "FullTest", "1234", 115200);
	if (ret) {
		pr_err("Factory reset test failed\n");
		goto fail;
	}

	pr_info("Test 2: Get Device Info\n");
	ret = hc_05_get_device_info(device, buffer, sizeof(buffer));
	if (ret < 0) {
		pr_err("Get device info test failed\n");
		goto fail;
	}

	pr_info("Test 3: Run Diagnostics\n");
	ret = hc_05_diagnostics(device, buffer, sizeof(buffer));
	if (ret < 0) {
		pr_err("Diagnostics test failed\n");
		goto fail;
	}

	pr_info("Test 4: Slave Mode Setup\n");
	ret = hc_05_setup_slave(device, "SlaveTest", "1234", 9600);
	if (ret) {
		pr_err("Slave mode test failed\n");
		goto fail;
	}

	pr_info("Test 5: Master Mode Setup\n");
	ret = hc_05_setup_master(device, "MasterTest", "1234", 9600);
	if (ret) {
		pr_err("Master mode test failed\n");
		goto fail;
	}

	pr_info("Test 6: Reset Device\n");
	ret = hc_05_reset(device);
	if (ret) {
		pr_err("Reset test failed\n");
		goto fail;
	}

	hc_05_exit(device);

	pr_info("=== HC-05 Full Test PASSED ===\n");
	return PASS;

fail:
	hc_05_exit(device);
	pr_info("=== HC-05 Full Test FAILED ===\n");
	return FAIL;
}

#endif