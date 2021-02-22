#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#define IIC_SW_1                        1
#define IIC_SW_2                        2
#define IIC_HW_1                        3
#define IIC_HW_2                        4

void iic_soft_init(void);
int iic_soft_start(u8 iic);
void iic_soft_stop(u8 iic);
int iic_soft_wait_ack(u8 iic);
void iic_soft_send_byte(u8 iic, u8 tmp);
u8 iic_soft_read_byte(u8 iic, u8 ack);
void iic_port_transmmit(u8 iic, u8 slave_addr, u16 reg, u8 tmp);
void iic_port_receive(u8 iic, u8 slave_addr, u16 reg, u8 *tmp);
void iic_port_multi_transmmit(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);
void iic_port_multi_receive(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#endif /* __IIC_SOFT_H */
