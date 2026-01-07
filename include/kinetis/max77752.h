#ifndef __MAX77752_H
#define __MAX77752_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include "kinetis/core_common.h"

/* Register addresses */
#define MAX77752_REG_DEV_ID             0x00
#define MAX77752_REG_DEV_REV            0x01
#define MAX77752_REG_STATUS             0x02
#define MAX77752_REG_INT                0x03
#define MAX77752_REG_INT_MSK            0x04
#define MAX77752_REG_CONFIG             0x05
#define MAX77752_REG_RESET              0x06
#define MAX77752_REG_OTG_CFG            0x07

/* Power control registers */
#define MAX77752_REG_PWR_SLA            0x08
#define MAX77752_REG_PWR_MSK            0x09
#define MAX77752_REG_PWR_FAULT          0x0A
#define MAX77752_REG_PWR_REQ            0x0B

/* Battery management registers */
#define MAX77752_REG_BAT_CFG            0x0C
#define MAX77752_REG_BAT_STATUS         0x0D
#define MAX77752_REG_CHG_CFG            0x0E
#define MAX77752_REG_CHG_STATUS         0x0F
#define MAX77752_REG_CHG_INT            0x10
#define MAX77752_REG_CHG_INT_MSK        0x11

/* Voltage monitoring registers */
#define MAX77752_REG_VSYS_MON           0x12
#define MAX77752_REG_VBAT_MON           0x13
#define MAX77752_REG_VBUS_MON           0x14
#define MAX77752_REG_VOTG_MON           0x15

/* Current monitoring registers */
#define MAX77752_REG_ICHG_MON           0x16
#define MAX77752_REG_ISYS_MON           0x17
#define MAX77752_REG_IOTG_MON           0x18

/* Thermistor and thermal registers */
#define MAX77752_REG_TEMP_CFG           0x19
#define MAX77752_REG_TEMP_STATUS        0x1A
#define MAX77752_REG_THERMISTOR         0x1B

/* LED control registers */
#define MAX77752_REG_LED_CFG            0x1C
#define MAX77752_REG_LED1_CFG           0x1D
#define MAX77752_REG_LED2_CFG           0x1E

/* Device address (7-bit, 0x3C is typical for MAX77752) */
#define MAX77752_I2C_ADDR               0x3C

/* Status register bits */
#define MAX77752_STATUS_PWR_SLA         0x01
#define MAX77752_STATUS_SYS_STATE       0x0E
#define MAX77752_STATUS_SHDN_L          0x10
#define MAX77752_STATUS_BAT_PRES        0x20
#define MAX77752_STATUS_USB_DET         0x40
#define MAX77752_STATUS_VBUS_PRES       0x80

/* Power control bits */
#define MAX77752_PWR_SLA_WAKE_EN        0x01
#define MAX77752_PWR_SLA_EN             0x02
#define MAX77752_PWR_SLA_ILIM_EN        0x04

/* Battery configuration bits */
#define MAX77752_BAT_CFG_BAT_EN         0x01
#define MAX77752_BAT_CFG_BAT_TYPE       0x06
#define MAX77752_BAT_CFG_JEITA_EN       0x08
#define MAX77752_BAT_CFG_LOW_BAT        0x10

/* Charge configuration bits */
#define MAX77752_CHG_CFG_EN             0x01
#define MAX77752_CHG_CFG_FAST_EN        0x02
#define MAX77752_CHG_CFG_AUTO_SAFE_EN   0x04
#define MAX77752_CHG_CFG_TMR_EN         0x08

/* Temperature limits */
#define MAX77752_TEMP_LOW_COLD          0
#define MAX77752_TEMP_LOW_COOL          10
#define MAX77752_TEMP_COOL_WARM         20
#define MAX77752_TEMP_WARM_HOT          45
#define MAX77752_TEMP_HOT               60

/* Voltage thresholds (in mV) */
#define MAX77752_VBUS_OVP_THRESHOLD     5500
#define MAX77752_VBAT_OVP_THRESHOLD     4200
#define MAX77752_VBAT_LOW_THRESHOLD     3000
#define MAX77752_VSYS_LOW_THRESHOLD     2800

/* Current limits (in mA) */
#define MAX77752_ICHG_LIMIT_100         100
#define MAX77752_ICHG_LIMIT_150         150
#define MAX77752_ICHG_LIMIT_200         200
#define MAX77752_ICHG_LIMIT_300         300
#define MAX77752_ICHG_LIMIT_500         500
#define MAX77752_ICHG_LIMIT_1000        1000
#define MAX77752_ICHG_LIMIT_1500        1500
#define MAX77752_ICHG_LIMIT_2000        2000

/* LED modes */
#define MAX77752_LED_MODE_OFF           0x00
#define MAX77752_LED_MODE_ON            0x01
#define MAX77752_LED_MODE_BLINK         0x02
#define MAX77752_LED_MODE_BREATH        0x03

/* Battery types */
#define MAX77752_BAT_TYPE_LI_ION        0x00
#define MAX77752_BAT_TYPE_LI_POLYMER    0x02
#define MAX77752_BAT_TYPE_LIFEPO4       0x04

/* Charge status values */
#define MAX77752_CHG_IDLE               0x00
#define MAX77752_CHG_TRICKLE            0x01
#define MAX77752_CHG_FAST_CC            0x02
#define MAX77752_CHG_FAST_CV            0x03
#define MAX77752_CHG_DONE               0x04
#define MAX77752_CHG_SUSPEND            0x05
#define MAX77752_CHG_TIMEOUT            0x06
#define MAX77752_CHG_ERROR              0x07

/* Function declarations */
void max77752_init(void);
u8 max77752_is_device_present(void);
u8 max77752_read_device_id(void);
u8 max77752_read_device_revision(void);
u8 max77752_read_status(void);
void max77752_write_status(u8 status);

/* Power management functions */
void max77752_enable_power(u8 enable);
u8 max77752_get_power_status(void);
void max77752_set_sleep_mode(u8 enable);
void max77752_enable_wake_source(u8 source, u8 enable);
void max77752_reset_system(void);

/* Battery management functions */
void max77752_configure_battery(u8 bat_type, u8 enable);
u8 max77752_get_battery_status(void);
u16 max77752_read_vsys_voltage(void);
u16 max77752_read_vbat_voltage(void);
u16 max77752_read_vbus_voltage(void);
u16 max77752_read_votg_voltage(void);

/* Charging functions */
void max77752_enable_charging(u8 enable);
void max77752_configure_charging(u16 current_limit, u16 voltage_limit);
u8 max77752_get_charge_status(void);
u16 max77752_read_charge_current(void);
u16 max77752_read_system_current(void);
u16 max77752_read_otg_current(void);

/* Thermal management functions */
void max77752_configure_thermal(u8 enable);
u8 max77752_get_thermal_status(void);
s16 max77752_read_temperature(void);
void max77752_set_temperature_limits(u8 low_limit, u8 high_limit);

/* LED control functions */
void max77752_configure_led(u8 led_id, u8 mode, u8 brightness);
void max77752_set_led_state(u8 led_id, u8 state);
void max77752_configure_led_charging_indicator(u8 enable);

/* Interrupt handling functions */
u8 max77752_read_interrupt_status(void);
void max77752_clear_interrupt(u8 interrupt_mask);
void max77752_configure_interrupt_mask(u8 mask, u8 enable);
void max77752_register_callback(void (*callback)(u8 interrupt_source));

/* Monitoring and diagnostic functions */
void max77752_continuous_monitor_start(u32 interval_ms);
void max77752_continuous_monitor_stop(void);
void max77752_perform_self_test(void);
void max77752_get_system_info(u8 *pinfo_buffer, u8 buffer_size);

/* Utility functions */
u8 max77752_check_power_good(void);
u8 max77752_check_battery_present(void);
u8 max77752_check_vbus_present(void);
u8 max77752_check_thermal_fault(void);
void max77752_set_default_config(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __MAX77752_H */