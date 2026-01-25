#ifndef __HC_05_H
#define __HC_05_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by user according to hardware device, otherwise driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "kinetis/serial-port.h"

/* The above procedure is modified by user according to hardware device, otherwise driver cannot run. */

struct hc_05_device {
	struct serial_port *serial;
	char name[32];
	char password[32];
	int baudrate;
	int stopbit;
	int parity;
	int role;
	int cmode;
	char addr[32];
	char bind_addr[32];
	char version[32];
	int state;
	int paired_count;
	int pio;
	int mpio;
	int uartmode;
	int enapwd;
	bool initialized;
};

/* Device Management */
struct hc_05_device *hc_05_alloc(void);
void hc_05_exit(struct hc_05_device *device);

/* Device Configuration */
int hc_05_setup_master(struct hc_05_device *device, const char *name, const char *password, int baudrate);
int hc_05_setup_slave(struct hc_05_device *device, const char *name, const char *password, int baudrate);
int hc_05_quick_setup(struct hc_05_device *device, const char *name, const char *password, int baudrate);
int hc_05_factory_reset_and_setup(struct hc_05_device *device, const char *name, const char *password, int baudrate);

/* Connection Management */
int hc_05_auto_connect(struct hc_05_device *device, const char *target_addr, int timeout);
int hc_05_scan_and_connect(struct hc_05_device *device, const char *target_name, int max_devices, int timeout);
int hc_05_disconnect(struct hc_05_device *device);
int hc_05_reset(struct hc_05_device *device);

/* Device Information */
int hc_05_get_device_info(struct hc_05_device *device, char *buffer, int size);
int hc_05_diagnostics(struct hc_05_device *device, char *buffer, int size);

/* Data Transfer */
int hc_05_send_data(struct hc_05_device *device, const u8 *data, u32 size, u16 packet_size, u32 delay_ms);
int hc_05_send_string(struct hc_05_device *device, const char *str, u16 packet_size, u32 delay_ms);
int hc_05_receive_data(struct hc_05_device *device, u8 *buffer, int size, u32 timeout_ms);
int hc_05_receive_string(struct hc_05_device *device, char *buffer, int size, u32 timeout_ms);
int hc_05_send_and_wait(struct hc_05_device *device, const u8 *data, u32 size, u8 *response, int resp_size, u32 timeout_ms);
int hc_05_flush_rx_buffer(struct hc_05_device *device);
int hc_05_clear_buffers(struct hc_05_device *device);

#ifdef __cplusplus
}
#endif

#endif /* __HC_05_H */
