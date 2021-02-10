#ifndef __W25QXXX_H
#define __W25QXXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#define W25Q128                         0x17
#define W25Q256                         0x18

u8 w25qxxx_read_status_reg(u8 w25qxxx, u8 num);
void w25qxxx_write_status_reg(u8 w25qxxx, u8 num, u8 tmp);
void w25qxxx_read_data(u8 w25qxxx, u32 addr, u8 *pdata, u32 length);
void w25qxxx_write_data(u8 w25qxxx, u32 addr, u8 *pdata, u16 length);
void w25qxxx_sector_erase(u8 w25qxxx, u32 addr);
u8 w25qxxx_release_device_id(u8 w25qxxx);
void w25qxxx_init(u8 w25qxxx);
void w25qxxx_read_info(u8 w25qxxx);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __W25QXXX_H */
