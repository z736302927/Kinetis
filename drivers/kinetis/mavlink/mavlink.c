#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/string.h>

#include <kinetis/serial-port.h>
#undef current
#include <kinetis/mavlink.h>
#include <kinetis/mavlink/mavlink/mavlink_msg_motor_control.h>
#include <kinetis/mavlink/mavlink/mavlink_msg_motor_status.h>
#include <kinetis/mavlink/mavlink/mavlink_msg_motor_ack.h>

#define MAVLINK_MOTOR_COUNT    4
#define MAV_MOTOR_STOPPED      0
#define MAV_MOTOR_RUNNING      1
#define MAV_MOTOR_FAULT        2

#define MAV_DIRECTION_STOP     0
#define MAV_DIRECTION_FORWARD  1
#define MAV_DIRECTION_REVERSE  2

#define MAV_ACK_OK             0
#define MAV_ACK_ERROR          1
#define MAV_ACK_INVALID_PARAM  2
#define MAV_ACK_MOTOR_FAULT    3

struct mavlink_motor_state {
    u8 motor_id;
    s32 target_speed;
    s32 current_speed;
    u8 direction;
    u8 status;
    u64 last_update;
};

struct mavlink_device {
    serial_port_t *serial;
    u8 sysid;
    u8 compid;
    u8 target_sysid;
    u8 target_compid;
    struct mavlink_motor_state motors[MAVLINK_MOTOR_COUNT];
    u32 tx_count;
    u32 rx_count;
    u32 rx_errors;
    u8 thread_running;
    pthread_t rx_thread;
    mavlink_status_t mav_status;
    mavlink_message_t rx_msg;
};

static void mavlink_motor_simulate(struct mavlink_device *dev, u8 motor_id)
{
    struct mavlink_motor_state *motor = &dev->motors[motor_id];
    u64 now = ktime_get_ns();
    
    if (motor->last_update == 0) {
        motor->last_update = now;
        motor->status = MAV_MOTOR_STOPPED;
        motor->direction = MAV_DIRECTION_STOP;
        return;
    }
    
    u64 delta_ns = now - motor->last_update;
    if (delta_ns < 5000000) {
        return;
    }
    
    f64 delta_sec = (f64)delta_ns / 1000000000.0;
    
    if (motor->target_speed == 0) {
        if (abs(motor->current_speed) > 0) {
            s32 decel = (motor->current_speed > 0 ? 100 : -100);
            motor->current_speed -= decel;
            
            if (abs(motor->current_speed) < 50) {
                motor->current_speed = 0;
                motor->direction = MAV_DIRECTION_STOP;
                motor->status = MAV_MOTOR_STOPPED;
            }
        }
    } else {
        u8 target_direction = motor->target_speed > 0 ? MAV_DIRECTION_FORWARD : MAV_DIRECTION_REVERSE;
        
        if (motor->direction != target_direction) {
            if (abs(motor->current_speed) > 0) {
                s32 decel = (motor->current_speed > 0 ? 200 : -200);
                motor->current_speed -= decel;
                
                if (abs(motor->current_speed) < 20) {
                    motor->current_speed = 0;
                    motor->direction = target_direction;
                }
            } else {
                motor->direction = target_direction;
            }
        }
        
        if (motor->direction == target_direction) {
            s32 speed_diff = motor->target_speed - motor->current_speed;
            s32 accel = speed_diff * 0.05;
            
            if (abs(accel) > 50) {
                accel = accel > 0 ? 50 : -50;
            }
            
            motor->current_speed += accel;
        }
    }
    
    if (abs(motor->current_speed) > 0) {
        motor->status = MAV_MOTOR_RUNNING;
    }
    
    motor->last_update = now;
}

int mavlink_init(mavlink_context_t *ctx, serial_port_t *port, mavlink_role_t role)
{
    if (!ctx || !port) {
        return -EINVAL;
    }
    
    memset(ctx, 0, sizeof(*ctx));
    ctx->port = port;
    ctx->role = role;
    
    mavlink_status_t *status = &ctx->status;
    status->parse_state = MAVLINK_PARSE_STATE_IDLE;
    status->packet_rx_drop_count = 0;
    status->packet_rx_success_count = 0;
    status->current_rx_seq = 0;
    status->current_tx_seq = 0;
    status->flags = 0;
    
    return 0;
}

int mavlink_send_message(mavlink_context_t *ctx, const mavlink_message_t *msg)
{
    if (!ctx || !msg) {
        return -EINVAL;
    }
    
    u8 buffer[MAVLINK_MAX_PACKET_LEN];
    u16 len = mavlink_msg_to_send_buffer(buffer, msg);
    
    if (len == 0) {
        return -EINVAL;
    }
    
    int ret = serial_port_transmit_bytes(ctx->port, buffer, len);
    if (ret >= 0) {
        ctx->tx_count++;
        ctx->last_tx_time = ktime_get_ns();
    }
    
    return ret;
}

int mavlink_receive_message(mavlink_context_t *ctx, mavlink_message_t *msg)
{
    if (!ctx || !msg) {
        return -EINVAL;
    }
    
    u8 buffer[256];
    int ret = serial_port_receive_bytes(ctx->port, buffer, sizeof(buffer));
    
    if (ret <= 0) {
        return ret;
    }
    
    for (int i = 0; i < ret; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], msg, &ctx->status)) {
            ctx->rx_count++;
            ctx->last_rx_time = ktime_get_ns();
            return 1;
        }
    }
    
    return 0;
}

void mavlink_dump_message(const mavlink_message_t *msg)
{
    if (!msg) {
        pr_info("MAVLink: NULL message\n");
        return;
    }
    
    pr_info("MAVLink Message: msgid=%u, sysid=%u, compid=%u, len=%u, seq=%u\n",
            msg->msgid, msg->sysid, msg->compid, msg->len, msg->seq);
}

static void *mavlink_slave_thread(void *arg)
{
    mavlink_context_t *ctx = (mavlink_context_t *)arg;
    mavlink_message_t msg;
    mavlink_motor_control_t motor_ctrl;
    
    pr_info("MAVLink slave thread started\n");
    
    while (1) {
        int ret = mavlink_receive_message(ctx, &msg);
        if (ret <= 0) {
            msleep(1);
            continue;
        }
        
        switch (msg.msgid) {
            case MAVLINK_MSG_ID_MOTOR_CONTROL:
                mavlink_msg_motor_control_decode(&msg, &motor_ctrl);
                
                pr_info("MAVLink slave: Received MOTOR_CONTROL: target_id=%u, speed=%d, direction=%u\n",
                       motor_ctrl.target_id, motor_ctrl.speed, motor_ctrl.direction);
                
                if (motor_ctrl.target_id >= MAVLINK_MOTOR_COUNT && motor_ctrl.target_id != 255) {
                    mavlink_motor_ack_t ack_msg;
                    ack_msg.motor_id = motor_ctrl.target_id;
                    ack_msg.ack_result = MAV_ACK_INVALID_PARAM;
                    
                    mavlink_message_t ack;
                    mavlink_msg_motor_ack_encode(1, 1, &ack, &ack_msg);
                    mavlink_send_message(ctx, &ack);
                    break;
                }
                
                mavlink_motor_status_t status_msg;
                status_msg.motor_id = motor_ctrl.target_id;
                status_msg.current_speed = motor_ctrl.speed;
                status_msg.direction = motor_ctrl.direction;
                status_msg.status = MAV_MOTOR_RUNNING;
                
                mavlink_message_t status;
                mavlink_msg_motor_status_encode(1, 1, &status, &status_msg);
                mavlink_send_message(ctx, &status);
                
                mavlink_motor_ack_t ack_msg;
                ack_msg.motor_id = motor_ctrl.target_id;
                ack_msg.ack_result = MAV_ACK_OK;
                
                mavlink_message_t ack;
                mavlink_msg_motor_ack_encode(1, 1, &ack, &ack_msg);
                mavlink_send_message(ctx, &ack);
                break;
                
            default:
                pr_info("MAVLink slave: Unknown message id=%u\n", msg.msgid);
                break;
        }
    }
    
    return NULL;
}

int mavlink_test_master_slave_sim(mavlink_context_t *master, mavlink_context_t *slave)
{
    if (!master || !slave) {
        return -EINVAL;
    }
    
    pr_info("Starting MAVLink master-slave simulation test\n");
    
    pthread_t slave_thread;
    int ret = pthread_create(&slave_thread, NULL, mavlink_slave_thread, slave);
    if (ret != 0) {
        pr_err("Failed to create slave thread: %d\n", ret);
        return ret;
    }
    
    msleep(100);
    
    for (u8 motor_id = 0; motor_id < 4; motor_id++) {
        mavlink_motor_control_t ctrl_msg;
        ctrl_msg.target_id = motor_id;
        ctrl_msg.speed = 1000 + motor_id * 500;
        ctrl_msg.direction = MAV_DIRECTION_FORWARD;
        
        mavlink_message_t msg;
        mavlink_msg_motor_control_encode(1, 0, &msg, &ctrl_msg);
        
        pr_info("MAVLink master: Sending MOTOR_CONTROL for motor %u\n", motor_id);
        ret = mavlink_send_message(master, &msg);
        if (ret < 0) {
            pr_err("Failed to send message: %d\n", ret);
            continue;
        }
        
        msleep(50);
        
        mavlink_message_t rx_msg;
        int timeout = 100;
        while (timeout-- > 0) {
            ret = mavlink_receive_message(master, &rx_msg);
            if (ret > 0) {
                switch (rx_msg.msgid) {
                    case MAVLINK_MSG_ID_MOTOR_STATUS:
                        mavlink_motor_status_t status;
                        mavlink_msg_motor_status_decode(&rx_msg, &status);
                        pr_info("MAVLink master: Received MOTOR_STATUS: id=%u, speed=%d, dir=%u, status=%u\n",
                               status.motor_id, status.current_speed, status.direction, status.status);
                        break;
                        
                    case MAVLINK_MSG_ID_MOTOR_ACK:
                        mavlink_motor_ack_t ack;
                        mavlink_msg_motor_ack_decode(&rx_msg, &ack);
                        pr_info("MAVLink master: Received MOTOR_ACK: id=%u, result=%u\n",
                               ack.motor_id, ack.ack_result);
                        break;
                        
                    default:
                        pr_info("MAVLink master: Unknown message id=%u\n", rx_msg.msgid);
                        break;
                }
                break;
            }
            msleep(1);
        }
        
        if (timeout <= 0) {
            pr_warn("MAVLink master: Timeout waiting for response from motor %u\n", motor_id);
        }
        
        msleep(200);
    }
    
    pthread_cancel(slave_thread);
    pthread_join(slave_thread, NULL);
    
    pr_info("MAVLink master-slave simulation test completed\n");
    return 0;
}

int mavlink_test_msg_pack_parse(mavlink_context_t *ctx)
{
    if (!ctx) {
        return -EINVAL;
    }
    
    mavlink_motor_control_t ctrl_msg;
    ctrl_msg.target_id = 0;
    ctrl_msg.speed = 1500;
    ctrl_msg.direction = MAV_DIRECTION_FORWARD;
    
    mavlink_message_t tx_msg;
    mavlink_msg_motor_control_encode(1, 0, &tx_msg, &ctrl_msg);
    
    pr_info("Testing MAVLink message packing and parsing\n");
    mavlink_dump_message(&tx_msg);
    
    int ret = mavlink_send_message(ctx, &tx_msg);
    if (ret < 0) {
        pr_err("Failed to send message: %d\n", ret);
        return ret;
    }
    
    pr_info("Message sent successfully\n");
    return 0;
}

#ifdef DESIGN_VERIFICATION_MAVLINK

int t_mavlink_master_slave_sim(int argc, char *argv[])
{
    serial_port_t *master_port, *slave_port;
    mavlink_context_t master_ctx, slave_ctx;
    int ret;
    
    pr_info("Starting MAVLink master-slave simulation test\n");
    
    master_port = serial_port_alloc("mavlink_master", 256, 0);
    if (!master_port) {
        pr_err("Failed to allocate master serial port\n");
        return -ENOMEM;
    }
    
    slave_port = serial_port_alloc("mavlink_slave", 256, 0);
    if (!slave_port) {
        pr_err("Failed to allocate slave serial port\n");
        serial_port_free(master_port);
        return -ENOMEM;
    }
    
    serial_port_add_other(slave_port, master_port);
    serial_port_add_other(master_port, slave_port);
    
    ret = mavlink_init(&master_ctx, master_port, MAVLINK_ROLE_MASTER);
    if (ret < 0) {
        pr_err("Failed to init MAVLink master context: %d\n", ret);
        goto cleanup;
    }
    
    ret = mavlink_init(&slave_ctx, slave_port, MAVLINK_ROLE_SLAVE);
    if (ret < 0) {
        pr_err("Failed to init MAVLink slave context: %d\n", ret);
        goto cleanup;
    }
    
    ret = mavlink_test_master_slave_sim(&master_ctx, &slave_ctx);
    
cleanup:
    serial_port_free(master_port);
    serial_port_free(slave_port);
    
    pr_info("MAVLink master-slave simulation test completed with ret=%d\n", ret);
    return ret;
}

int t_mavlink_msg_pack_parse(int argc, char *argv[])
{
    serial_port_t *test_port;
    mavlink_context_t ctx;
    int ret;
    
    pr_info("Starting MAVLink message packing/parsing test\n");
    
    test_port = serial_port_alloc("mavlink_test", 256, 0);
    if (!test_port) {
        pr_err("Failed to allocate test serial port\n");
        return -ENOMEM;
    }
    
    ret = mavlink_init(&ctx, test_port, MAVLINK_ROLE_MASTER);
    if (ret < 0) {
        pr_err("Failed to init MAVLink context: %d\n", ret);
        serial_port_free(test_port);
        return ret;
    }
    
    ret = mavlink_test_msg_pack_parse(&ctx);
    
    serial_port_free(test_port);
    
    pr_info("MAVLink message packing/parsing test completed with ret=%d\n", ret);
    return ret;
}
#endif