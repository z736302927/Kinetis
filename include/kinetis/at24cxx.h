#ifndef __AT24CXX_H
#define __AT24CXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#include <linux/types.h>

#include "kinetis/core_common.h"

void at24cxx_byte_write(u8 addr, u8 tmp);
int at24cxx_write_data(u8 addr, u8 *tmp, u32 length);
int at24cxx_current_addr_read(u8 *tmp);
int at24cxx_read_data(u8 addr, u8 *tmp, u32 length);
int at24cxx_sequential_read(u8 *tmp, u32 length);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __AT24CXX_H */
