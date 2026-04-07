#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include "kinetis/hc-05.h"
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

/* Parse AT command and update HC-05 state, return response */

static void hc_05_set_response(char *response, const char *text)
{
	if (!response || !text) {
		return;
	}

	snprintf(response, SERIAL_PORT_BUFFER_SIZE, "%s", text);
}

static int hc_05_query(struct hc_05_device *device, const char *cmd,
	char *buffer, int size, u32 timeout_ms)
{
	int ret;

	if (!device || !device->serial || !cmd || !buffer || size <= 0) {
		return -EINVAL;
	}

	ret = serial_port_transmit_bytes(device->serial, cmd, strlen(cmd));
	if (ret < 0) {
		return ret;
	}

	ret = serial_port_receive_bytes(device->serial, buffer, size, timeout_ms);
	if (ret < 0) {
		return ret;
	}

	if (ret < size) {
		buffer[ret] = '\0';
	} else {
		buffer[size - 1] = '\0';
	}

	return ret;
}

static int hc_05_buf_append(char *buffer, int size, int *pos, const char *fmt, ...)
{
	int written;
	va_list args;

	if (!buffer || !pos || !fmt || size <= 0) {
		return -EINVAL;
	}

	if (*pos >= size) {
		return -ENOSPC;
	}

	va_start(args, fmt);
	written = vsnprintf(buffer + *pos, size - *pos, fmt, args);
	va_end(args);

	if (written < 0) {
		return written;
	}

	if (written >= size - *pos) {
		*pos = size - 1;
		return -ENOSPC;
	}

	*pos += written;
	return 0;
}

static void hc_05_parse_command(char *request, char *response, void *context)
{
	struct hc_05_device *device = (struct hc_05_device *)context;

	if (strncmp(request, "AT", 2) != 0) {
		hc_05_set_response(response, "ERROR");
	} else if (strcmp(request, "AT") == 0) {
		/* AT - Test */
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+RESET") == 0) {
		/* AT+RESET - Reset */
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+ORGL") == 0) {
		/* AT+ORGL - Restore Factory */
		strncpy(device->name, "HC-05-Default", sizeof(device->name) - 1);
		device->name[sizeof(device->name) - 1] = '\0';
		strncpy(device->password, "1234", sizeof(device->password) - 1);
		device->password[sizeof(device->password) - 1] = '\0';
		device->baudrate = 9600;
		device->role = 0;
		device->cmode = 1;
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+VERSION?") == 0) {
		/* AT+VERSION? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+VERSION:%s\r\nOK", device->version);
	} else if (strcmp(request, "AT+NAME?") == 0) {
		/* AT+NAME? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+NAME:%s\r\nOK", device->name);
	} else if (strncmp(request, "AT+NAME=", 8) == 0) {
		/* AT+NAME= */
		strncpy(device->name, request + 8, sizeof(device->name) - 1);
		device->name[sizeof(device->name) - 1] = '\0';
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+PSWD?") == 0) {
		/* AT+PSWD? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+PSWD:%s\r\nOK", device->password);
	} else if (strncmp(request, "AT+PSWD=", 8) == 0) {
		/* AT+PSWD= */
		strncpy(device->password, request + 8, sizeof(device->password) - 1);
		device->password[sizeof(device->password) - 1] = '\0';
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+UART?") == 0) {
		/* AT+UART? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+UART:%d,%d,%d\r\nOK",
			device->baudrate, device->stopbit, device->parity);
	} else if (strncmp(request, "AT+UART=", 8) == 0) {
		/* AT+UART= */
		if (sscanf(request + 8, "%d,%d,%d", &device->baudrate,
			&device->stopbit, &device->parity) == 3) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+ROLE?") == 0) {
		/* AT+ROLE? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+ROLE:%d\r\nOK", device->role);
	} else if (strncmp(request, "AT+ROLE=", 8) == 0) {
		/* AT+ROLE= */
		if (sscanf(request + 8, "%d", &device->role) == 1) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+CMODE?") == 0) {
		/* AT+CMODE? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+CMODE:%d\r\nOK", device->cmode);
	} else if (strncmp(request, "AT+CMODE=", 9) == 0) {
		/* AT+CMODE= */
		if (sscanf(request + 9, "%d", &device->cmode) == 1) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+ADDR?") == 0) {
		/* AT+ADDR? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+ADDR:%s\r\nOK", device->addr);
	} else if (strcmp(request, "AT+BIND?") == 0) {
		/* AT+BIND? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+BIND:%s\r\nOK", device->bind_addr);
	} else if (strcmp(request, "AT+STATE?") == 0) {
		/* AT+STATE? */
		const char *state_str[] = {"INITIALIZED", "CONNECTED", "DISCONNECTED"};
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+STATE:%s\r\nOK",
			(device->state >= 0 && device->state <= 2) ?
			state_str[device->state] : "UNKNOWN");
	} else if (strcmp(request, "AT+ADCN?") == 0) {
		/* AT+ADCN? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+ADCN:%d\r\nOK", device->paired_count);
	} else if (strcmp(request, "AT+PIO?") == 0) {
		/* AT+PIO? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+PIO:%d\r\nOK", device->pio);
	} else if (strncmp(request, "AT+PIO=", 8) == 0) {
		/* AT+PIO= */
		if (sscanf(request + 8, "%d", &device->pio) == 1) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+MPIO?") == 0) {
		/* AT+MPIO? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+MPIO:%d\r\nOK", device->mpio);
	} else if (strncmp(request, "AT+MPIO=", 8) == 0) {
		/* AT+MPIO= */
		if (sscanf(request + 8, "%d", &device->mpio) == 1) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+IPSCAN?") == 0) {
		/* AT+IPSCAN? */
		hc_05_set_response(response, "+IPSCAN:1024,512,48,18\r\nOK");
	} else if (strncmp(request, "AT+IPSCAN=", 9) == 0) {
		/* AT+IPSCAN= */
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+UARTMODE?") == 0) {
		/* AT+UARTMODE? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+UARTMODE:%d\r\nOK", device->uartmode);
	} else if (strncmp(request, "AT+UARTMODE=", 12) == 0) {
		/* AT+UARTMODE= */
		if (sscanf(request + 12, "%d", &device->uartmode) == 1) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+ENAPWD?") == 0) {
		/* AT+ENAPWD? */
		snprintf(response, SERIAL_PORT_BUFFER_SIZE, "+ENAPWD:%d\r\nOK", device->enapwd);
	} else if (strncmp(request, "AT+ENAPWD=", 10) == 0) {
		/* AT+ENAPWD= */
		if (sscanf(request + 10, "%d", &device->enapwd) == 1) {
			hc_05_set_response(response, "OK");
		} else {
			hc_05_set_response(response, "ERROR");
		}
	} else if (strcmp(request, "AT+INQ") == 0) {
		/* AT+INQ */
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+INQC") == 0) {
		/* AT+INQC */
		hc_05_set_response(response, "OK");
	} else if (strncmp(request, "AT+LINK=", 8) == 0) {
		/* AT+LINK= */
		device->state = 1; /* CONNECTED */
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+DISC") == 0) {
		/* AT+DISC */
		device->state = 2; /* DISCONNECTED */
		hc_05_set_response(response, "OK");
	} else if (strcmp(request, "AT+RMAAD") == 0) {
		/* AT+RMAAD */
		device->paired_count = 0;
		hc_05_set_response(response, "OK");
	} else if (strncmp(request, "AT+PAIR=", 8) == 0) {
		/* AT+PAIR= */
		device->paired_count++;
		hc_05_set_response(response, "OK");
	} else {
		hc_05_set_response(response, "ERROR");
	}

	pr_info("request: %s", request);
	pr_info("response: %s", response);
}

static int hc_05_detect_device(struct hc_05_device *device)
{
	char at_ack[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	serial_port_transmit_bytes(device->serial, "AT", strlen("AT"));

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
	if (ret < 0) {
		return ret;
	}

	if (strncmp(at_ack, "OK", 2) != 0) {
		pr_err("hc-05 not responding correctly, received: %.*s", ret, at_ack);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+VERSION?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+NAME?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+PSWD?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+UART?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+ROLE?", buffer, sizeof(buffer), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+CMODE?", buffer, sizeof(buffer), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+BIND?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+IAC?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+CLASS?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+INQM?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+IPSCAN?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+SNIFF?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+PIO?", buffer, sizeof(buffer), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+MPIO?", buffer, sizeof(buffer), 1000);
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

	ret = hc_05_query(device, "AT+STATE?", buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_get_addr(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	ret = hc_05_query(device, "AT+ADDR?", buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_get_adcn(struct hc_05_device *device, int *count)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	ret = hc_05_query(device, "AT+ADCN?", buffer, sizeof(buffer), 1000);
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

	ret = hc_05_query(device, cmd, buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int hc_05_get_rssi(struct hc_05_device *device, char *buffer, int size)
{
	int ret;

	ret = hc_05_query(device, "AT+RSSI?", buffer, size, 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 10000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 10000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = serial_port_receive_bytes(device->serial, at_ack, sizeof(at_ack), 1000);
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

	ret = hc_05_query(device, "AT+MRAD", buffer, size, 1000);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

/* Application Functions */

int hc_05_quick_setup(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("hc-05 quick setup - name: %s, password: %s, baudrate: %d\n", name, password, baudrate);

	ret = hc_05_set_name(device, name);
	if (ret) {
		pr_err("failed to set name: %s\n", name);
		return ret;
	}

	ret = hc_05_set_password(device, password);
	if (ret) {
		pr_err("failed to set password: %s\n", password);
		return ret;
	}

	ret = hc_05_set_uart(device, baudrate, 0, 0);
	if (ret) {
		pr_err("failed to set uart: %d\n", baudrate);
		return ret;
	}

	ret = hc_05_reset(device);
	if (ret) {
		pr_err("failed to reset device\n");
		return ret;
	}

	pr_info("hc-05 quick setup completed successfully\n");
	return 0;
}

int hc_05_setup_master(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("hc-05 master setup - name: %s\n", name);

	ret = hc_05_quick_setup(device, name, password, baudrate);
	if (ret) {
		return ret;
	}

	ret = hc_05_set_role(device, 1);
	if (ret) {
		pr_err("failed to set master role\n");
		return ret;
	}

	ret = hc_05_set_cmode(device, 0);
	if (ret) {
		pr_err("failed to set connection mode\n");
		return ret;
	}

	pr_info("hc-05 master setup completed successfully\n");
	return 0;
}

int hc_05_setup_slave(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("hc-05 slave setup - name: %s\n", name);

	ret = hc_05_quick_setup(device, name, password, baudrate);
	if (ret) {
		return ret;
	}

	ret = hc_05_set_role(device, 0);
	if (ret) {
		pr_err("failed to set slave role\n");
		return ret;
	}

	ret = hc_05_set_cmode(device, 1);
	if (ret) {
		pr_err("failed to set connection mode\n");
		return ret;
	}

	pr_info("hc-05 slave setup completed successfully\n");
	return 0;
}

int hc_05_auto_connect(struct hc_05_device *device, const char *target_addr, int timeout)
{
	int ret;

	pr_info("hc-05 auto connect to: %s\n", target_addr);

	ret = hc_05_init_spp(device);
	if (ret) {
		pr_err("failed to initialize spp\n");
		return ret;
	}

	ret = hc_05_pair(device, target_addr, timeout);
	if (ret) {
		pr_err("failed to pair with device: %s\n", target_addr);
		return ret;
	}

	ret = hc_05_connect(device, target_addr);
	if (ret) {
		pr_err("failed to connect to device: %s\n", target_addr);
		return ret;
	}

	pr_info("hc-05 connected to: %s successfully\n", target_addr);
	return 0;
}

int hc_05_scan_and_connect(struct hc_05_device *device, const char *target_name, int max_devices, int timeout)
{
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	char scan_cmd[64];
	int ret;

	pr_info("hc-05 scan and connect - target: %s, max devices: %d\n", target_name, max_devices);

	ret = hc_05_init_spp(device);
	if (ret) {
		pr_err("failed to initialize spp\n");
		return ret;
	}

	ret = hc_05_set_inqm(device, 0, max_devices, timeout);
	if (ret) {
		pr_err("failed to set inquiry mode\n");
		return ret;
	}

	ret = hc_05_inquiry(device);
	if (ret) {
		pr_err("failed to start inquiry\n");
		return ret;
	}

	ret = serial_port_receive_bytes(device->serial, buffer, sizeof(buffer), (timeout + 2) * 1000);
	if (ret < 0) {
		pr_warn("no devices found or timeout\n");
		return ret;
	}

	if (ret < (int)sizeof(buffer)) {
		buffer[ret] = '\0';
	} else {
		buffer[sizeof(buffer) - 1] = '\0';
	}

	pr_info("found devices:\n%.*s\n", ret, buffer);

	if (snprintf(scan_cmd, sizeof(scan_cmd), "AT+RNAME?%s", target_name) >= sizeof(scan_cmd)) {
		pr_err("target name too long\n");
		return -ENAMETOOLONG;
	}

	ret = hc_05_query(device, scan_cmd, buffer, sizeof(buffer), 5000);
	if (ret < 0) {
		pr_err("failed to get device name\n");
		return ret;
	}

	if (strstr(buffer, target_name) == NULL) {
		pr_err("target device not found\n");
		return -ENODEV;
	}

	pr_info("target device found\n");
	return 0;
}

int hc_05_factory_reset_and_setup(struct hc_05_device *device, const char *name, const char *password, int baudrate)
{
	int ret;

	pr_info("hc-05 factory reset and setup\n");

	ret = hc_05_restore_factory(device);
	if (ret) {
		pr_err("failed to restore factory settings\n");
		return ret;
	}

	mdelay(1000);

	ret = hc_05_quick_setup(device, name, password, baudrate);
	if (ret) {
		pr_err("failed to setup after factory reset\n");
		return ret;
	}

	pr_info("hc-05 factory reset and setup completed successfully\n");
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

	pr_info("getting hc-05 device information\n");

	ret = hc_05_buf_append(buffer, size, &pos, "=== hc-05 device information ===\n");
	if (ret < 0) {
		return ret;
	}

	ret = hc_05_get_version(device, temp, sizeof(temp));
	if (ret >= 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Version: %.*s\n", ret, temp) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_name(device, temp, sizeof(temp));
	if (ret >= 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Name: %.*s\n", ret, temp) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_addr(device, temp, sizeof(temp));
	if (ret >= 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Address: %.*s\n", ret, temp) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_password(device, temp, sizeof(temp));
	if (ret >= 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Password: %.*s\n", ret, temp) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_uart(device, temp, sizeof(temp));
	if (ret >= 0) {
		if (hc_05_buf_append(buffer, size, &pos, "UART: %.*s\n", ret, temp) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_role(device, &role);
	if (ret == 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Role: %d\n", role) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_cmode(device, &mode);
	if (ret == 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Connection Mode: %d\n", mode) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_state(device, temp, sizeof(temp));
	if (ret >= 0) {
		if (hc_05_buf_append(buffer, size, &pos, "State: %.*s\n", ret, temp) < 0) {
			return -ENOSPC;
		}
	}

	ret = hc_05_get_adcn(device, &count);
	if (ret == 0) {
		if (hc_05_buf_append(buffer, size, &pos, "Paired Devices: %d\n", count) < 0) {
			return -ENOSPC;
		}
	}

	if (hc_05_buf_append(buffer, size, &pos, "================================\n") < 0) {
		return -ENOSPC;
	}

	pr_info("device information retrieved\n");
	return pos;
}

int hc_05_diagnostics(struct hc_05_device *device, char *buffer, int size)
{
	char temp[SERIAL_PORT_BUFFER_SIZE];
	int pos = 0;
	int ret;
	int role, pio, mpio;

	pr_info("running hc-05 diagnostics\n");

	if (hc_05_buf_append(buffer, size, &pos, "=== hc-05 diagnostics ===\n") < 0) {
		return -ENOSPC;
	}

	if (hc_05_test(device) == 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] device responds to at command\n");
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] device does not respond\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_version(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] version: %.*s\n", ret, temp);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read version\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_state(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] state: %.*s\n", ret, temp);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read state\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_role(device, &role);
	if (ret == 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] role: %d\n", role);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read role\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_pio(device, &pio);
	if (ret == 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] pio: %d\n", pio);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read pio\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_mpio(device, &mpio);
	if (ret == 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] mpio: %d\n", mpio);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read mpio\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_addr(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] address: %.*s\n", ret, temp);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read address\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_uart(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] uart: %.*s\n", ret, temp);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[FAIL] cannot read uart config\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	ret = hc_05_get_bind(device, temp, sizeof(temp));
	if (ret >= 0) {
		ret = hc_05_buf_append(buffer, size, &pos, "[OK] bind address: %.*s\n", ret, temp);
	} else {
		ret = hc_05_buf_append(buffer, size, &pos, "[WARN] cannot read bind address\n");
	}
	if (ret < 0) {
		return -ENOSPC;
	}

	if (hc_05_buf_append(buffer, size, &pos, "=========================\n") < 0) {
		return -ENOSPC;
	}

	pr_info("diagnostics completed\n");
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

	ret = serial_port_receive_bytes(device->serial, (char *)buffer, size, timeout_ms);
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

	ret = serial_port_receive_bytes(device->serial, buffer, size, timeout_ms);
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

	ret = serial_port_receive_bytes(device->serial, (char *)buffer, sizeof(buffer), timeout_ms);
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

	ret = serial_port_receive_bytes(device->serial, (char *)buffer, sizeof(buffer), 10);
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
	int ret;

	device = kzalloc(sizeof(struct hc_05_device), GFP_KERNEL);
	if (device == NULL) {
		return ERR_PTR(-ENOMEM);
	}

	strcpy(device->name, "HC-05-Default");
	strcpy(device->password, "1234");
	device->baudrate = 9600;
	device->stopbit = 0;
	device->parity = 0;
	device->role = 0;
	device->cmode = 1;
	strcpy(device->addr, "98d3:32:70a0b8");
	strcpy(device->bind_addr, "98d3:32:70a0b8");
	strcpy(device->version, "2.0-20100601");
	device->state = 0;
	device->paired_count = 0;
	device->pio = 0;
	device->mpio = 0;
	device->uartmode = 0;
	device->enapwd = 0;
	device->initialized = true;

	device->serial = serial_port_alloc(hc_05_parse_command, device);
	if (IS_ERR(device->serial)) {
		ret = PTR_ERR(device->serial);
		kfree(device);
		return ERR_PTR(ret);
	}

	ret = hc_05_detect_device(device);
	if (ret) {
		serial_port_free(device->serial);
		kfree(device);
		return ERR_PTR(ret);
	}

	pr_info("hc-05 device allocated and initialized\n");
	return device;
}

void hc_05_exit(struct hc_05_device *device)
{
	if (device == NULL) {
		pr_warn("device pointer is null in hc_05_exit\n");
		return;
	}

	if (device->serial != NULL) {
		serial_port_free(device->serial);
		device->serial = NULL;
	}

	kfree(device);
	pr_info("hc-05 device freed\n");
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

/*********************************************************************
 * HC-05 Test Framework
 *********************************************************************/

/**
 * @brief Test result structure
 */
struct hc_05_test_result {
	int total;
	int passed;
	int failed;
	char details[512];
};

/**
 * @brief Test framework - Allocate and initialize test device
 * @param test_name: Test name for logging
 * @return Device pointer on success, NULL on failure
 */
static struct hc_05_device *hc_05_test_alloc(const char *test_name)
{
	struct hc_05_device *device;

	pr_info("=== %s START ===\n", test_name);

	device = hc_05_alloc();
	if (!device) {
		pr_err("[%s] failed to allocate device\n", test_name);
		return NULL;
	}

	pr_info("[%s] device initialized\n", test_name);
	return device;
}

/**
 * @brief Test framework - Cleanup test device
 * @param device: Device pointer
 * @param test_name: Test name for logging
 */
static void hc_05_test_free(struct hc_05_device *device, const char *test_name)
{
	if (device) {
		hc_05_exit(device);
		pr_info("=== %s END ===\n", test_name);
	}
}

/**
 * @brief Test framework - Record test result
 * @param result: Test result structure
 * @param test_item: Test item description
 * @param success: Test result (0 = fail, 1 = pass)
 */
static void hc_05_test_record(struct hc_05_test_result *result,
			       const char *test_item, int success)
{
	result->total++;
	if (success) {
		result->passed++;
		pr_info("[PASS] %s\n", test_item);
	} else {
		result->failed++;
		pr_err("[FAIL] %s\n", test_item);
	}
}

/**
 * @brief Test framework - Print test summary
 * @param result: Test result structure
 * @param test_name: Test name
 */
static void hc_05_test_summary(struct hc_05_test_result *result, const char *test_name)
{
	pr_info("\n=== %s SUMMARY ===\n", test_name);
	pr_info("Total: %d, Passed: %d, Failed: %d\n",
		result->total, result->passed, result->failed);

	if (result->failed == 0) {
		pr_info("Result: ALL TESTS PASSED\n");
	} else {
		pr_err("Result: SOME TESTS FAILED\n");
	}
	pr_info("===================\n");
}



int t_hc_05_basic_init(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	char buffer[SERIAL_PORT_BUFFER_SIZE];
	int ret;

	device = hc_05_test_alloc("HC-05 BASIC INIT");
	if (!device)
		return -ENOMEM;

	/* Test 1: AT command response */
	ret = hc_05_test(device);
	hc_05_test_record(&result, "AT Command Response", ret == 0);

	/* Test 2: Get version information */
	ret = hc_05_get_version(device, buffer, sizeof(buffer));
	hc_05_test_record(&result, "Get Version", ret >= 0);
	if (ret >= 0) {
		pr_info("Version: %s\n", buffer);
	}

	/* Test 3: Device reset */
	ret = hc_05_reset(device);
	hc_05_test_record(&result, "Device Reset", ret == 0);

	/* Print test summary */
	hc_05_test_summary(&result, "HC-05 BASIC INIT");

	hc_05_test_free(device, "HC-05 BASIC INIT");

	return result.failed > 0 ? -1 : 0;
}

int t_hc_05_info_query(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	char info_buffer[512];
	int ret;

	device = hc_05_test_alloc("HC-05 INFO QUERY");
	if (!device)
		return -ENOMEM;

	/* Initialize device first */
	ret = hc_05_quick_setup(device, "InfoQueryTest", "1234", 9600);
	if (ret) {
		pr_err("setup failed\n");
		hc_05_test_free(device, "HC-05 INFO QUERY");
		return ret;
	}

	/* Test 1: Get device info */
	ret = hc_05_get_device_info(device, info_buffer, sizeof(info_buffer));
	hc_05_test_record(&result, "Get Device Info", ret >= 0);
	if (ret >= 0) {
		pr_info("Device Info:\n%s\n", info_buffer);
	}

	/* Test 2: Run diagnostics */
	ret = hc_05_diagnostics(device, info_buffer, sizeof(info_buffer));
	hc_05_test_record(&result, "Diagnostics", ret >= 0);
	if (ret >= 0) {
		pr_info("Diagnostics:\n%s\n", info_buffer);
	}

	hc_05_test_summary(&result, "HC-05 INFO QUERY");
	hc_05_test_free(device, "HC-05 INFO QUERY");

	return result.failed > 0 ? -1 : 0;
}

int t_hc_05_cleanup(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	int ret;

	device = hc_05_test_alloc("HC-05 CLEANUP");
	if (!device)
		return -ENOMEM;

	/* Test: Factory reset and setup */
	ret = hc_05_factory_reset_and_setup(device, "CleanDevice", "5678", 115200);
	hc_05_test_record(&result, "Factory Reset and Setup", ret == 0);

	hc_05_test_summary(&result, "HC-05 CLEANUP");
	hc_05_test_free(device, "HC-05 CLEANUP");

	return result.failed > 0 ? -1 : 0;
}

/*********************************************************************
 * HC-05 Configuration Layer Tests
 *********************************************************************/

int t_hc_05_config_basic(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	int ret;

	device = hc_05_test_alloc("HC-05 CONFIG BASIC");
	if (!device)
		return -ENOMEM;

	/* Test 1: Quick setup with default parameters */
	ret = hc_05_quick_setup(device, "ConfigBasic", "1234", 9600);
	hc_05_test_record(&result, "Quick Setup", ret == 0);

	/* Test 2: Set baudrate */
	ret = hc_05_set_uart(device, 115200, 0, 0);
	hc_05_test_record(&result, "Set Baudrate 115200", ret == 0);

	/* Test 3: Set device name */
	ret = hc_05_set_name(device, "HC05-ConfigTest");
	hc_05_test_record(&result, "Set Device Name", ret == 0);

	/* Test 4: Set password */
	ret = hc_05_set_password(device, "4321");
	hc_05_test_record(&result, "Set Password", ret == 0);

	hc_05_test_summary(&result, "HC-05 CONFIG BASIC");
	hc_05_test_free(device, "HC-05 CONFIG BASIC");

	return result.failed > 0 ? -1 : 0;
}

int t_hc_05_config_role(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	int role, ret;

	device = hc_05_test_alloc("HC-05 CONFIG ROLE");
	if (!device)
		return -ENOMEM;

	/* Test 1: Setup slave mode */
	ret = hc_05_setup_slave(device, "HC05-Slave", "1234", 115200);
	hc_05_test_record(&result, "Setup Slave Mode", ret == 0);

	/* Test 2: Get role (should be slave) */
	ret = hc_05_get_role(device, &role);
	hc_05_test_record(&result, "Get Role (Slave)", ret == 0 && role == 0);

	/* Test 3: Setup master mode */
	ret = hc_05_setup_master(device, "HC05-Master", "1234", 115200);
	hc_05_test_record(&result, "Setup Master Mode", ret == 0);

	/* Test 4: Get role (should be master) */
	ret = hc_05_get_role(device, &role);
	hc_05_test_record(&result, "Get Role (Master)", ret == 0 && role == 1);

	hc_05_test_summary(&result, "HC-05 CONFIG ROLE");
	hc_05_test_free(device, "HC-05 CONFIG ROLE");

	return result.failed > 0 ? -1 : 0;
}

int t_hc_05_config_advanced(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	int ret;

	device = hc_05_test_alloc("HC-05 CONFIG ADVANCED");
	if (!device)
		return -ENOMEM;

	ret = hc_05_quick_setup(device, "ConfigAdvanced", "1234", 9600);
	if (ret) {
		pr_err("setup failed\n");
		hc_05_test_free(device, "HC-05 CONFIG ADVANCED");
		return ret;
	}


	/* Test: Set connection mode */
	ret = hc_05_set_cmode(device, 1);  /* Any address */
	hc_05_test_record(&result, "Set Connection Mode", ret == 0);

	hc_05_test_summary(&result, "HC-05 CONFIG ADVANCED");
	hc_05_test_free(device, "HC-05 CONFIG ADVANCED");

	return result.failed > 0 ? -1 : 0;
}

/*********************************************************************
 * HC-05 Function Layer Tests
 *********************************************************************/

int t_hc_05_data_comm(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	u8 tx_data[128];
	u8 rx_data[256];
	int i, ret;

	device = hc_05_test_alloc("HC-05 DATA COMM");
	if (!device)
		return -ENOMEM;

	ret = hc_05_quick_setup(device, "DataComm", "1234", 9600);
	if (ret) {
		pr_err("setup failed\n");
		hc_05_test_free(device, "HC-05 DATA COMM");
		return ret;
	}

	/* Prepare test data */
	for (i = 0; i < 128; i++) {
		tx_data[i] = (u8)i;
	}

	/* Test 1: Send binary data */
	ret = hc_05_send_data(device, tx_data, 128, 64, 10);
	hc_05_test_record(&result, "Send Binary Data", ret == 128);

	/* Test 2: Send string */
	ret = hc_05_send_string(device, "Hello, HC-05!", 0, 0);
	hc_05_test_record(&result, "Send String", ret > 0);

	/* Test 3: Receive data (with timeout) */
	ret = hc_05_receive_data(device, rx_data, sizeof(rx_data), 1000);
	hc_05_test_record(&result, "Receive Data", ret >= 0);

	/* Test 4: Send and wait */
	ret = hc_05_send_and_wait(device, (u8*)"PING", 4, rx_data, sizeof(rx_data), 1000);
	hc_05_test_record(&result, "Send and Wait", ret >= 0);

	/* Test 5: Clear buffers */
	ret = hc_05_clear_buffers(device);
	hc_05_test_record(&result, "Clear Buffers", ret == 0);

	hc_05_test_summary(&result, "HC-05 DATA COMM");
	hc_05_test_free(device, "HC-05 DATA COMM");

	return result.failed > 0 ? -1 : 0;
}

int t_hc_05_connection(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	int ret;

	device = hc_05_test_alloc("HC-05 CONNECTION");
	if (!device)
		return -ENOMEM;

	ret = hc_05_setup_master(device, "ConnTest", "1234", 115200);
	if (ret) {
		pr_err("setup failed\n");
		hc_05_test_free(device, "HC-05 CONNECTION");
		return ret;
	}

	/* Test 1: Connect to device (if address provided) */
	if (argc > 1) {
		ret = hc_05_connect(device, argv[1]);
		hc_05_test_record(&result, "Connect to Device", ret == 0);
	}

	/* Test 2: Disconnect */
	ret = hc_05_disconnect(device);
	hc_05_test_record(&result, "Disconnect", ret == 0);

	hc_05_test_summary(&result, "HC-05 CONNECTION");
	hc_05_test_free(device, "HC-05 CONNECTION");

	return result.failed > 0 ? -1 : 0;
}

int t_hc_05_pairing(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	int ret;

	device = hc_05_test_alloc("HC-05 PAIRING");
	if (!device)
		return -ENOMEM;

	ret = hc_05_setup_slave(device, "PairingSlave", "1234", 115200);
	if (ret) {
		pr_err("setup failed\n");
		hc_05_test_free(device, "HC-05 PAIRING");
		return ret;
	}

	/* Test 1: Enter pair mode */
	ret = hc_05_pair(device, device->addr, 10);
	hc_05_test_record(&result, "Enter Pair Mode", ret == 0);

	/* Note: Actual pairing requires two devices, this is a basic test */

	hc_05_test_summary(&result, "HC-05 PAIRING");
	hc_05_test_free(device, "HC-05 PAIRING");

	return result.failed > 0 ? -1 : 0;
}




/*********************************************************************
 * HC-05 Performance Layer Tests
 *********************************************************************/

int t_hc_05_performance(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	u8 tx_data[256];
	u8 rx_data[256];
	int packet_size = 256;
	int packet_count = 100;
	int test_loops = 5;
	int test_mode = 0;
	int i, loop, ret;

	pr_info("=== hc-05 performance test ===\n");
	pr_info("usage: t_hc_05_performance [packet_size] [packet_count] [loops] [mode]\n");
	pr_info("  mode: 0=send only, 1=receive only, 2=both (send then receive)\n");

	if (argc > 1)
		packet_size = simple_strtoul(argv[1], &argv[1], 10);
	if (packet_size > 256)
		packet_size = 256;

	if (argc > 2)
		packet_count = simple_strtoul(argv[2], &argv[1], 10);

	if (argc > 3)
		test_loops = simple_strtoul(argv[3], &argv[1], 10);

	if (argc > 4)
		test_mode = simple_strtoul(argv[4], &argv[1], 10);

	pr_info("params: %d bytes, %d packets, %d loops, mode=%d\n",
		packet_size, packet_count, test_loops, test_mode);

	device = hc_05_test_alloc("HC-05 PERFORMANCE");
	if (!device)
		return -ENOMEM;

	ret = hc_05_quick_setup(device, "PerfTest", "1234", 115200);
	if (ret) {
		pr_err("setup failed\n");
		hc_05_test_free(device, "HC-05 PERFORMANCE");
		return ret;
	}

	for (i = 0; i < packet_size; i++)
		tx_data[i] = (u8)i;

	if (test_mode == 0 || test_mode == 2) {
		u64 total_bytes = 0;
		u64 total_time = 0;

		pr_info("\n--- send test ---\n");
		for (loop = 0; loop < test_loops; loop++) {
			u64 start, end, bytes = 0;

			start = ktime_get_ns();
			for (i = 0; i < packet_count; i++) {
				ret = hc_05_send_data(device, tx_data, packet_size, 0, 0);
				if (ret > 0)
					bytes += ret;
			}
			end = ktime_get_ns();

			total_bytes += bytes;
			total_time += (end - start) / 1000;

			pr_info("loop %d: %llu bytes, %llu us\n",
				loop, bytes, (end - start) / 1000);
		}

		u64 speed_kb_s = total_time > 0 ? (total_bytes * 1000) / total_time : 0;
		pr_info("send: %llu bytes, %llu us, %llu KB/s\n",
			total_bytes, total_time, speed_kb_s);
		hc_05_test_record(&result, "Send Performance", speed_kb_s > 0);
	}

	if (test_mode == 1 || test_mode == 2) {
		u64 total_bytes = 0;
		u64 total_time = 0;

		pr_info("\n--- receive test ---\n");
		for (loop = 0; loop < test_loops; loop++) {
			u64 start, end, bytes = 0;

			start = ktime_get_ns();
			for (i = 0; i < packet_count; i++) {
				ret = hc_05_receive_data(device, rx_data, sizeof(rx_data), 100);
				if (ret > 0)
					bytes += ret;
			}
			end = ktime_get_ns();

			total_bytes += bytes;
			total_time += (end - start) / 1000;

			pr_info("loop %d: %llu bytes, %llu us\n",
				loop, bytes, (end - start) / 1000);
		}

		u64 speed_kb_s = total_time > 0 ? (total_bytes * 1000) / total_time : 0;
		pr_info("receive: %llu bytes, %llu us, %llu KB/s\n",
			total_bytes, total_time, speed_kb_s);
		hc_05_test_record(&result, "Receive Performance", speed_kb_s > 0);
	}

	hc_05_test_summary(&result, "HC-05 PERFORMANCE");
	hc_05_test_free(device, "HC-05 PERFORMANCE");

	return result.failed > 0 ? -1 : 0;
}

/*********************************************************************
 * HC-05 Comprehensive Layer Tests
 *********************************************************************/

int t_hc_05_comprehensive(int argc, char **argv)
{
	struct hc_05_device *device;
	struct hc_05_test_result result = {0};
	char buffer[512];
	int ret, role;
	int i;
	u64 start, end;

	device = hc_05_test_alloc("HC-05 COMPREHENSIVE");
	if (!device)
		return -ENOMEM;

	pr_info("Running comprehensive test suite...\n");

	/* Phase 1: Basic Tests */
	pr_info("\n--- Phase 1: Basic Tests ---\n");

	ret = hc_05_test(device);
	hc_05_test_record(&result, "1.1 AT Command", ret == 0);

	ret = hc_05_get_version(device, buffer, sizeof(buffer));
	hc_05_test_record(&result, "1.2 Get Version", ret >= 0);

	/* Phase 2: Configuration Tests */
	pr_info("\n--- Phase 2: Configuration ---\n");

	ret = hc_05_quick_setup(device, "ComprehensiveTest", "1234", 115200);
	hc_05_test_record(&result, "2.1 Quick Setup", ret == 0);

	ret = hc_05_set_role(device, 0);  /* Slave mode */
	hc_05_test_record(&result, "2.2 Set Role", ret == 0);

	ret = hc_05_get_role(device, &role);
	hc_05_test_record(&result, "2.3 Get Role", ret == 0 && role == 0);

	/* Phase 3: Information Query */
	pr_info("\n--- Phase 3: Information Query ---\n");

	ret = hc_05_get_device_info(device, buffer, sizeof(buffer));
	hc_05_test_record(&result, "3.1 Device Info", ret >= 0);

	ret = hc_05_diagnostics(device, buffer, sizeof(buffer));
	hc_05_test_record(&result, "3.2 Diagnostics", ret >= 0);

	/* Phase 4: Data Transfer */
	pr_info("\n--- Phase 4: Data Transfer ---\n");

	start = ktime_get_ns();
	ret = hc_05_send_string(device, "Test Message", 0, 0);
	end = ktime_get_ns();
	hc_05_test_record(&result, "4.1 Send String", ret > 0);

	pr_info("Send time: %llu ns\n", end - start);

	/* Phase 5: Performance Test */
	pr_info("\n--- Phase 5: Performance ---\n");

	start = ktime_get_ns();
	for (i = 0; i < 10; i++) {
		hc_05_send_string(device, "PerformanceTest", 0, 0);
	}
	end = ktime_get_ns();

	pr_info("10 sends in %llu ns (avg: %llu ns)\n",
		end - start, (end - start) / 10);
	hc_05_test_record(&result, "5.1 Performance Test", 1);

	/* Phase 6: Cleanup */
	pr_info("\n--- Phase 6: Cleanup ---\n");

	ret = hc_05_reset(device);
	hc_05_test_record(&result, "6.1 Reset Device", ret == 0);

	hc_05_test_summary(&result, "HC-05 COMPREHENSIVE");
	hc_05_test_free(device, "HC-05 COMPREHENSIVE");

	return result.failed > 0 ? -1 : 0;
}

#endif