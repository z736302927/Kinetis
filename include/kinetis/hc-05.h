#ifndef __HC_05_H
#define __HC_05_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by user according to hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "kinetis/serial-port.h"

/* The above procedure is modified by user according to hardware device, otherwise the driver cannot run. */

enum hc_05_role {
	HC05_ROLE_SLAVE = 0,
	HC05_ROLE_MASTER = 1,
	HC05_ROLE_SLAVE_LOOPBACK = 2
};

enum hc_05_mode {
	HC05_MODE_COMMAND = 0,
	HC05_MODE_DATA = 1
};

enum hc_05_state {
	HC05_STATE_IDLE,
	HC05_STATE_INITIALIZING,
	HC05_STATE_READY,
	HC05_STATE_CONNECTING,
	HC05_STATE_CONNECTED,
	HC05_STATE_DISCONNECTING,
	HC05_STATE_ERROR
};

struct hc_05_config {
	char name[32];
	u32 baud_rate;
	enum hc_05_role role;
	u8 pin_code;
	u8 parity;
	u8 stop_bits;
};

struct hc_05_device {
	struct serial_port *serial_port;
	struct hc_05_config config;
	enum hc_05_state state;
	enum hc_05_mode mode;
	u8 address[13];
	u8 version[16];
	u32 connection_timeout_ms;
};

int hc_05_init(struct hc_05_device *device, struct serial_port *serial_port);
void hc_05_deinit(struct hc_05_device *device);
int hc_05_enter_command_mode(struct hc_05_device *device);
int hc_05_exit_command_mode(struct hc_05_device *device);
int hc_05_set_baud_rate(struct hc_05_device *device, u32 baud_rate);
int hc_05_set_name(struct hc_05_device *device, const char *name);
int hc_05_set_role(struct hc_05_device *device, enum hc_05_role role);
int hc_05_set_pin_code(struct hc_05_device *device, u8 pin_code);
int hc_05_connect(struct hc_05_device *device, const char *address);
int hc_05_disconnect(struct hc_05_device *device);
int hc_05_send_data(struct hc_05_device *device, const u8 *data, u16 length);
int hc_05_receive_data(struct hc_05_device *device, u8 *buffer, u16 length, u16 timeout_ms);
int hc_05_get_version(struct hc_05_device *device, char *version, u16 max_len);
int hc_05_get_address(struct hc_05_device *device, u8 *address);
enum hc_05_state hc_05_get_state(struct hc_05_device *device);

#ifdef __cplusplus
}
#endif

#endif /* __HC_05_H */
