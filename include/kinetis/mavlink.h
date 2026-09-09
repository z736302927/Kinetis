#ifndef KINETIS_MAVLINK_H
#define KINETIS_MAVLINK_H

#include <kinetis/serial-port.h>

#undef current
#undef ERROR
#undef OK
#include <kinetis/mavlink/mavlink_dialect/mavlink.h>

#ifdef KINETIS_FAKE_SIM
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAVLINK_MOTOR_COUNT        4

#define MAVLINK_RX_BUF_SIZE        SERIAL_PORT_BUFFER_SIZE

/* Direction (driver-internal representation) */
#define MAVLINK_DIRECTION_FORWARD  1
#define MAVLINK_DIRECTION_REVERSE -1
#define MAVLINK_DIRECTION_STOP     0

/* Motor status (matches MAV_MOTOR_STATUS enum) */
#define MAVLINK_MOTOR_STOPPED      0
#define MAVLINK_MOTOR_RUNNING      1
#define MAVLINK_MOTOR_FAULT        2

/* File transfer ack result */
#define MAV_FT_RESULT_READY        0
#define MAV_FT_RESULT_DENIED       1
#define MAV_FT_RESULT_NO_SPACE     2
#define MAV_FT_RESULT_NOT_FOUND    3
#define MAV_FT_RESULT_BUSY         4

/* File transfer error codes */
#define MAV_FT_ERR_NONE            0
#define MAV_FT_ERR_TIMEOUT         1
#define MAV_FT_ERR_CRC             2
#define MAV_FT_ERR_PROTOCOL        3

/* Bootloader ack result */
#define MAV_BL_ACK_ACCEPTED        0
#define MAV_BL_ACK_DENIED          1
#define MAV_BL_ACK_ERROR           2
#define MAV_BL_ACK_INVALID_CMD     3
#define MAV_BL_ACK_COMPLETED       4

#define MAVLINK_RING_PAYLOAD_LEN      MAVLINK_MSG_RING_CMD_FIELD_PAYLOAD_LEN

struct mavlink_motor_state {
	s32 target_speed;    /* RPM */
	s32 current_speed;   /* RPM */
	s8 direction;        /* 0=stop, 1=fwd, -1=rev */
	u8 status;           /* 0=stopped, 1=running, 2=fault */
};

struct mavlink_image_cmd {
	u8 pending;          /* 1 = new command received, reset after read */
	u8 action;           /* MAV_IMAGE_ACTION_REPLAY / NEXT / PREVIOUS / PLAY_INDEX */
	u16 image_index;     /* target index for PLAY_INDEX */
};

struct mavlink_ft_state {
	u8 active;           /* 1 = transfer in progress */
	u8 direction;        /* MAV_FILE_TRANSFER_DIR_UPLOAD / DOWNLOAD */
	u8 state;            /* MAV_FILE_TRANSFER_STATE_* */
	u8 error_code;       /* MAV_FT_ERR_* */
	char path[64];   	 /* Destination path on the device file system */
	u32 total_bytes;     /* Total file size */
	u32 transferred;     /* Bytes transferred so far */
};

struct mavlink_bl_cmd {
	u8 pending;          /* 1 = update command received, app should process it */
	u8 command;          /* MAV_BL_CMD_PREPARE_UPDATE / START_APP / RESET / QUERY_INFO / PREPARE_STATOR_UPDATE */
	u32 firmware_size;   /* Expected firmware size */
	u32 firmware_crc32;  /* Expected firmware CRC32 */
	char filename[32];   /* Target firmware filename */
};

struct mavlink_ring_cmd_state {
	u8 pending;
	u16 seq;
	u16 cmd;
	u8 src_arm;
	u8 dst_arm;
	u8 current_arm;
	u8 direction;
	u8 payload_len;
	u8 payload[MAVLINK_RING_PAYLOAD_LEN];
};

struct mavlink_ring_ack_state {
	u8 pending;
	u16 seq;
	u8 node;
	u8 result;
};

struct mavlink_ring_error_state {
	u8 pending;
	u16 seq;
	u8 error_code;
	u8 error_node;
	u8 error_link_from;
	u8 error_link_to;
};

struct mavlink_device {
	struct serial_port *serial;
	u8 sysid;
	u8 compid;

	/* Motor subsystem */
	struct mavlink_motor_state motors[MAVLINK_MOTOR_COUNT];

	/* Image subsystem */
	struct mavlink_image_cmd image_cmd;

	/* File transfer - ZMODEM */
	struct mavlink_ft_state ft;

	/* Bootloader */
	struct mavlink_bl_cmd bl;

	/* System status */
	u8 sys_status;       /* MAV_SYSTEM_STATUS_* */
	u16 error_count;
	u32 uptime_s;        /* Uptime in seconds */

	struct mavlink_ring_cmd_state ring_cmd;
	struct mavlink_ring_ack_state ring_ack;
	struct mavlink_ring_error_state ring_error;

	/* Message statistics */
	u32 tx_count;
	u32 rx_count;
	u32 rx_errors;

#ifdef KINETIS_FAKE_SIM
	/* Thread control */
	volatile u8 thread_running;
	pthread_t rx_thread;
#endif

	mavlink_status_t rx_status;
	mavlink_message_t rx_msg;

	const char *name;	/* Human-readable name for debug output (kstrdup'd) */
};

#define MD_NAME(md)  ((md)->name ? (md)->name : "?")

struct mavlink_device *mavlink_init(struct serial_port *serial, u8 sysid, u8 compid, const char *name);
void mavlink_free(struct mavlink_device *dev);

int mavlink_send_motor_control(struct mavlink_device *dev, u8 target_id,
	s32 speed, s8 direction, u32 timeout_ms);
int mavlink_send_motor_status(struct mavlink_device *dev, u16 motor_id,
	s32 current_speed, s8 direction, u8 status);
int mavlink_send_motor_ack(struct mavlink_device *dev, u8 motor_id, u8 ack_result);
int mavlink_send_image_ack(struct mavlink_device *dev, u8 action, u8 ack_result);
int mavlink_send_image_control(struct mavlink_device *dev, u8 action, u16 image_index);
int mavlink_query_motor_status(struct mavlink_device *dev, u8 motor_id, u32 timeout_ms);

int mavlink_send_ft_start(struct mavlink_device *dev, u8 direction,
	const char *path, u32 file_size, u32 timeout_ms);

int mavlink_send_ft_progress(struct mavlink_device *dev, u8 state,
	u32 transferred, u32 total_bytes, u8 error_code);
int mavlink_send_ft_ack(struct mavlink_device *dev, u8 direction,
	u8 result, u32 file_size);

int mavlink_send_ft_cancel(struct mavlink_device *dev, u8 direction);

int mavlink_send_ft_list(struct mavlink_device *dev, u8 file_count,
	u8 list_index, u32 file_size, const char *filename);

int mavlink_ft_enter_passthrough(struct mavlink_device *dev);

void mavlink_ft_exit_passthrough(struct mavlink_device *dev);

static inline int mavlink_ft_is_active(struct mavlink_device *dev)
{
	return dev->ft.active;
}

int mavlink_send_heartbeat(struct mavlink_device *dev, u8 status,
	u32 uptime_s, u16 error_count);

int mavlink_send_bootloader_update(struct mavlink_device *dev, u8 command,
	u32 firmware_size, u32 firmware_crc32, const char *filename,
	u32 timeout_ms);
int mavlink_send_bootloader_ack(struct mavlink_device *dev, u8 command, u8 result);
int mavlink_wait_bootloader_ack(struct mavlink_device *dev, u8 command,
	u8 expected_result, u32 timeout_ms);

int mavlink_send_ring_cmd(struct mavlink_device *dev, u16 seq, u16 cmd,
	u8 src_arm, u8 dst_arm, u8 current_arm, u8 direction,
	const u8 *payload, u8 payload_len);
int mavlink_send_ring_ack(struct mavlink_device *dev, u16 seq,
	u8 ack_node, u8 result);
int mavlink_send_ring_error(struct mavlink_device *dev, u16 seq,
	u8 error_node, u8 error_link_from, u8 error_link_to, u16 error_code);

int mavlink_receive_and_process(struct mavlink_device *dev, u32 timeout_ms);
int mavlink_start_rx_thread(struct mavlink_device *dev);
void mavlink_stop_rx_thread(struct mavlink_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* KINETIS_MAVLINK_H */
