#ifndef KINETIS_MAVLINK_H
#define KINETIS_MAVLINK_H

#include <kinetis/mavlink/mavlink_types.h>
#include <kinetis/mavlink/protocol.h>
#include <kinetis/mavlink/mavlink_helpers.h>
#include <kinetis/mavlink/mavlink_conversions.h>

#include <kinetis/mavlink/mavlink/mavlink_msg_motor_control.h>
#include <kinetis/mavlink/mavlink/mavlink_msg_motor_status.h>
#include <kinetis/mavlink/mavlink/mavlink_msg_motor_ack.h>

#include <kinetis/serial-port.h>

typedef enum {
    MAVLINK_ROLE_SLAVE,
    MAVLINK_ROLE_MASTER
} mavlink_role_t;

typedef struct {
    serial_port_t *port;
    mavlink_role_t role;
    mavlink_status_t status;
    mavlink_message_t tx_msg;
    mavlink_message_t rx_msg;
    u32 tx_count;
    u32 rx_count;
    u32 error_count;
    u64 last_tx_time;
    u64 last_rx_time;
} mavlink_context_t;

int mavlink_init(mavlink_context_t *ctx, serial_port_t *port, mavlink_role_t role);
int mavlink_send_message(mavlink_context_t *ctx, const mavlink_message_t *msg);
int mavlink_receive_message(mavlink_context_t *ctx, mavlink_message_t *msg);
void mavlink_dump_message(const mavlink_message_t *msg);

int mavlink_test_master_slave_sim(mavlink_context_t *master, mavlink_context_t *slave);
int mavlink_test_msg_pack_parse(mavlink_context_t *ctx);

#endif
