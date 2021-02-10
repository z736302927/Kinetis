#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#define IIC_1                           1
#define IIC_2                           2
#define IIC_3                           3

void iic_soft_init(void);
int iic_soft_start(void);
void iic_soft_stop(void);
int iic_soft_wait_ack(void);
void iic_soft_send_byte(u8 tmp);
u8 iic_soft_read_byte(u8 ack);
void iic_port_transmmit(u8 iic, u8 slave_addr, u16 reg, u8 tmp);
void iic_port_receive(u8 iic, u8 slave_addr, u16 reg, u8 *tmp);
void iic_port_multi_transmmit(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);
void iic_port_multi_receive(u8 iic, u8 slave_addr, u16 reg,
    u8 *pdata, u8 length);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#endif /* __IIC_SOFT_H */
