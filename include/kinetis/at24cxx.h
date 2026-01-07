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

/* Enhanced function declarations */
void at24cxx_page_write(u8 addr, u8 *pdata, u32 length);

/* Device identification and status checking */
int at24cxx_device_detect(void);
int at24cxx_check_write_protection(void);

/* Error handling and retry mechanism */
int at24cxx_write_data_with_retry(u8 addr, u8 *pdata, u32 length, u8 max_retry);
int at24cxx_verify_write(u8 addr, u8 *pdata, u32 length);

/* Wear leveling and write counting */
void at24cxx_init_wear_leveling(void);
void at24cxx_update_write_cycle_count(u8 addr, u32 length);
void at24cxx_check_wear_leveling(void);
void at24cxx_get_wear_leveling_info(u32 *p_max_cycles, u32 *p_min_cycles, u32 *p_total_cycles);


#ifdef __cplusplus
}
#endif

#endif /* __AT24CXX_H */
