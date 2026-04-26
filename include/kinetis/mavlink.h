#ifndef KINETIS_MAVLINK_H
#define KINETIS_MAVLINK_H

#include <kinetis/mavlink/mavlink_dialect/mavlink.h>

#include <kinetis/serial-port.h>

#include <pthread.h>

#define MAVLINK_MOTOR_COUNT         4
#define MAVLINK_DIRECTION_FORWARD -1
#define MAVLINK_DIRECTION_REVERSE  0
#define MAVLINK_MOTOR_STOPPED      0
#define MAVLINK_MOTOR_RUNNING      1
#define MAVLINK_MOTOR_FAULT        2

#define MAV_ACK_OK                 0
#define MAV_ACK_ERROR              1
#define MAV_ACK_INVALID_PARAM      2
#define MAV_ACK_MOTOR_FAULT        3

struct mavlink_motor_state {
	u8 motor_id;
	s32 target_speed;    /* RPM */
	s32 current_speed;   /* RPM */
	s8 direction;        /* 0=stop, 1=fwd, -1=rev */
	u8 status;           /* 0=stopped, 1=running, 2=fault */
};

struct mavlink_device {
	struct serial_port *serial;
	u8 sysid;
	u8 compid;
	u8 target_sysid;
	u8 target_compid;
	struct mavlink_motor_state motors[MAVLINK_MOTOR_COUNT];
	/* Message statistics */
	u32 tx_count;
	u32 rx_count;
	u32 rx_errors;
	/* Thread control */
	u8 thread_running;
	pthread_t rx_thread;
	mavlink_status_t rx_status;
	mavlink_message_t rx_msg;
};

/* Function declarations */
struct mavlink_device *mavlink_init(struct serial_port *serial, u8 sysid, u8 compid);
void mavlink_free(struct mavlink_device *dev);
int mavlink_send_motor_control(struct mavlink_device *dev, u32 target_id, s32 speed, s8 direction);
int mavlink_send_motor_status(struct mavlink_device *dev, u16 motor_id, s32 current_speed, s8 direction, u8 status);
int mavlink_send_motor_ack(struct mavlink_device *dev, u8 motor_id, u8 ack_result);
int mavlink_receive_and_process(struct mavlink_device *dev, u32 timeout_ms);
int mavlink_start_rx_thread(struct mavlink_device *dev);
void mavlink_stop_rx_thread(struct mavlink_device *dev);

#endif /* KINETIS_MAVLINK_H */