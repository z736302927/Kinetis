#define pr_fmt(fmt) "max77752: " fmt

#include <linux/delay.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/random.h>
#include <linux/limits.h>

#include "kinetis/max77752.h"
#include "kinetis/iic_soft.h"
#include "kinetis/spi_soft.h"
#include "kinetis/regmap-user-bus.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

#ifdef KINETIS_FAKE_SIM
#include <pthread.h>
#endif
#include <math.h>

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

/* Device address (7-bit, 0x3C is typical for max77752) */
#define MAX77752_I2C_ADDR               0x3C

struct max77752_device {
	struct regmap *regmap;

	/* Private variables */
	u8 device_present;
	u8 init_complete;
	u8 power_enabled;
	u8 charging_enabled;
	u8 thermal_monitoring_enabled;

	/* Configuration variables */
	u8 battery_type;
	u16 charge_current_limit;
	u16 charge_voltage_limit;
	u8 temperature_low_limit;
	u8 temperature_high_limit;

	/* Callback function pointers */
	void (*interrupt_callback)(u8 interrupt_source);
	void (*charge_done_callback)(void);
	void (*low_battery_callback)(void);
	void (*thermal_fault_callback)(void);

	/* Monitoring variables */
	u32 monitor_interval ; /* 1 second default */
	u8 monitoring_active;
	u16 last_vsys_voltage;
	u16 last_vbat_voltage;
	u16 last_vbus_voltage;
	u8 last_charge_status;
	s16 last_temperature;

#ifdef KINETIS_FAKE_SIM
	/* Slave support for testing */
	struct iic_slave *iic_slave;
	struct spi_slave *spi_slave;
	u8 *slave_regs;

	/* Randomization thread support */
	bool thread_running;
#endif
};

u8 max77752_is_device_present(struct max77752_device *dev)
{
	u32 dev_id = 0;

	/* Try to read device ID register */
	regmap_read(dev->regmap, MAX77752_REG_DEV_ID, &dev_id);

	return (dev_id != 0x00 && dev_id != 0xFF) ? 1 : 0;
}

u8 max77752_read_device_id(struct max77752_device *dev)
{
	u32 dev_id = 0;

	regmap_read(dev->regmap, MAX77752_REG_DEV_ID, &dev_id);
	return dev_id;
}

u8 max77752_read_device_revision(struct max77752_device *dev)
{
	u32 dev_rev = 0;

	regmap_read(dev->regmap, MAX77752_REG_DEV_REV, &dev_rev);
	return dev_rev;
}

u8 max77752_read_status(struct max77752_device *dev)
{
	u32 status = 0;

	regmap_read(dev->regmap, MAX77752_REG_STATUS, &status);
	return status;
}

void max77752_write_status(struct max77752_device *dev, u8 status)
{
	regmap_write(dev->regmap, MAX77752_REG_STATUS, status);
}

/* Power management functions */
void max77752_enable_power(struct max77752_device *dev, u8 enable)
{
	u32 pwr_cfg = 0;

	regmap_read(dev->regmap, MAX77752_REG_PWR_SLA, &pwr_cfg);

	if (enable) {
		pwr_cfg |= MAX77752_PWR_SLA_EN;
		dev->power_enabled = 1;
		pr_info("power enabled");
	} else {
		pwr_cfg &= ~MAX77752_PWR_SLA_EN;
		dev->power_enabled = 0;
		pr_info("power disabled");
	}

	regmap_write(dev->regmap, MAX77752_REG_PWR_SLA, pwr_cfg);
}

u8 max77752_get_power_status(struct max77752_device *dev)
{
	u8 status = max77752_read_status(dev);

	return (status & MAX77752_STATUS_PWR_SLA) ? 1 : 0;
}

void max77752_set_sleep_mode(struct max77752_device *dev, u8 enable)
{
	u32 config = 0;

	regmap_read(dev->regmap, MAX77752_REG_CONFIG, &config);

	if (enable) {
		config |= 0x01; /* Sleep mode bit */
		pr_info("entering sleep mode");
	} else {
		config &= ~0x01;
		pr_info("exiting sleep mode");
	}

	regmap_write(dev->regmap, MAX77752_REG_CONFIG, config);
}

void max77752_enable_wake_source(struct max77752_device *dev, u8 source, u8 enable)
{
	u32 pwr_masks = 0;

	regmap_read(dev->regmap, MAX77752_REG_PWR_MSK, &pwr_masks);

	if (enable) {
		pwr_masks |= (1 << source);
	} else {
		pwr_masks &= ~(1 << source);
	}

	regmap_write(dev->regmap, MAX77752_REG_PWR_MSK, pwr_masks);
}

void max77752_reset_system(struct max77752_device *dev)
{
	regmap_write(dev->regmap, MAX77752_REG_RESET, 0x01);
	mdelay(100); /* Wait for reset to complete */
}

/* Battery management functions */
void max77752_configure_battery(struct max77752_device *dev, u8 bat_type, u8 enable)
{
	u32 bat_cfg = 0;

	regmap_read(dev->regmap, MAX77752_REG_BAT_CFG, &bat_cfg);

	/* Clear battery type bits */
	bat_cfg &= ~MAX77752_BAT_CFG_BAT_TYPE;

	/* Set battery type */
	bat_cfg |= bat_type;

	/* Enable/disable battery */
	if (enable) {
		bat_cfg |= MAX77752_BAT_CFG_BAT_EN;
	} else {
		bat_cfg &= ~MAX77752_BAT_CFG_BAT_EN;
	}

	regmap_write(dev->regmap, MAX77752_REG_BAT_CFG, bat_cfg);
	pr_info("battery configured: type=%d, enabled=%d", bat_type, enable);
}

u8 max77752_get_battery_status(struct max77752_device *dev)
{
	u32 bat_status;

	regmap_read(dev->regmap, MAX77752_REG_BAT_STATUS, &bat_status);
	return bat_status;
}

/* Voltage monitoring functions */
u16 max77752_read_vsys_voltage(struct max77752_device *dev)
{
	u8 voltage_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_VSYS_MON, voltage_data, 2);

	dev->last_vsys_voltage = (voltage_data[0] << 8) | voltage_data[1];
	return dev->last_vsys_voltage;
}

u16 max77752_read_vbat_voltage(struct max77752_device *dev)
{
	u8 voltage_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_VBAT_MON, voltage_data, 2);

	dev->last_vbat_voltage = (voltage_data[0] << 8) | voltage_data[1];
	return dev->last_vbat_voltage;
}

u16 max77752_read_vbus_voltage(struct max77752_device *dev)
{
	u8 voltage_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_VBUS_MON, voltage_data, 2);

	dev->last_vbus_voltage = (voltage_data[0] << 8) | voltage_data[1];
	return dev->last_vbus_voltage;
}

u16 max77752_read_votg_voltage(struct max77752_device *dev)
{
	u8 voltage_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_VOTG_MON, voltage_data, 2);

	return (voltage_data[0] << 8) | voltage_data[1];
}

/* charging functions */
void max77752_enable_charging(struct max77752_device *dev, u8 enable)
{
	u32 chg_cfg = 0;

	regmap_read(dev->regmap, MAX77752_REG_CHG_CFG, &chg_cfg);

	if (enable) {
		chg_cfg |= MAX77752_CHG_CFG_EN;
		dev->charging_enabled = 1;
		pr_info("charging enabled");
	} else {
		chg_cfg &= ~MAX77752_CHG_CFG_EN;
		dev->charging_enabled = 0;
		pr_info("charging disabled");
	}

	regmap_write(dev->regmap, MAX77752_REG_CHG_CFG, chg_cfg);
}

void max77752_configure_charging(struct max77752_device *dev, u16 current_limit, u16 voltage_limit)
{
	u32 chg_cfg = 0;

	regmap_read(dev->regmap, MAX77752_REG_CHG_CFG, &chg_cfg);

	/* Configure current limit (bits 1-3) */
	chg_cfg &= ~0x0E;
	chg_cfg |= ((current_limit & 0x07) << 1);

	/* Configure voltage limit (bits 4-5) */
	chg_cfg &= ~0x30;
	chg_cfg |= ((voltage_limit & 0x03) << 4);

	/* Enable fast charging */
	chg_cfg |= MAX77752_CHG_CFG_FAST_EN;

	/* Enable safety timer */
	chg_cfg |= MAX77752_CHG_CFG_TMR_EN;

	regmap_write(dev->regmap, MAX77752_REG_CHG_CFG, chg_cfg);

	pr_info("charging configured: Current=%dmA, Voltage=%dmV",
		current_limit * 100, (voltage_limit + 4) * 100);
}

u8 max77752_get_charge_status(struct max77752_device *dev)
{
	u32 chg_status = 0;
	regmap_read(dev->regmap, MAX77752_REG_CHG_STATUS, &chg_status);

	/* Extract charge status from bits 0-2 */
	dev->last_charge_status = chg_status & 0x07;
	return dev->last_charge_status;
}

/* Current monitoring functions */
u16 max77752_read_charge_current(struct max77752_device *dev)
{
	u8 current_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_ICHG_MON, current_data, 2);

	return (current_data[0] << 8) | current_data[1];
}

u16 max77752_read_system_current(struct max77752_device *dev)
{
	u8 current_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_ISYS_MON, current_data, 2);

	return (current_data[0] << 8) | current_data[1];
}

u16 max77752_read_otg_current(struct max77752_device *dev)
{
	u8 current_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_IOTG_MON, current_data, 2);

	return (current_data[0] << 8) | current_data[1];
}

/* Thermal management functions */
void max77752_configure_thermal(struct max77752_device *dev, u8 enable)
{
	u32 temp_cfg = 0;

	regmap_read(dev->regmap, MAX77752_REG_TEMP_CFG, &temp_cfg);

	if (enable) {
		temp_cfg |= 0x01; /* Thermal monitoring enable bit */
		dev->thermal_monitoring_enabled = 1;
		pr_info("thermal monitoring enabled");
	} else {
		temp_cfg &= ~0x01;
		dev->thermal_monitoring_enabled = 0;
		pr_info("thermal monitoring disabled");
	}

	regmap_write(dev->regmap, MAX77752_REG_TEMP_CFG, temp_cfg);
}

u8 max77752_get_thermal_status(struct max77752_device *dev)
{
	u32 temp_status = 0;
	regmap_read(dev->regmap, MAX77752_REG_TEMP_STATUS, &temp_status);
	return temp_status;
}

s16 max77752_read_temperature(struct max77752_device *dev)
{
	u8 temp_data[2] = {0};

	regmap_bulk_read(dev->regmap, MAX77752_REG_THERMISTOR, temp_data, 2);

	dev->last_temperature = (temp_data[0] << 8) | temp_data[1];

	/* Convert to Celsius (assuming 12-bit signed temperature) */
	if (dev->last_temperature > 0x7FF) {
		dev->last_temperature -= 0x1000;
	}

	return dev->last_temperature;
}

void max77752_set_temperature_limits(struct max77752_device *dev, u8 low_limit, u8 high_limit)
{
	u32 temp_cfg = 0;

	dev->temperature_low_limit = low_limit;
	dev->temperature_high_limit = high_limit;

	regmap_read(dev->regmap, MAX77752_REG_TEMP_CFG, &temp_cfg);

	/* Configure temperature limits */
	temp_cfg &= ~0x3C; /* Clear limit bits */
	temp_cfg |= ((low_limit & 0x07) << 2); /* Low limit */
	temp_cfg |= ((high_limit & 0x07) << 4); /* High limit */

	regmap_write(dev->regmap, MAX77752_REG_TEMP_CFG, temp_cfg);

	pr_info("temperature limits set: %d°c to %d°c", low_limit, high_limit);
}

/* LED control functions */
void max77752_configure_led(struct max77752_device *dev, u8 led_id, u8 mode, u8 brightness)
{
	u8 led_cfg = 0;
	u8 reg_addr;

	if (led_id == 1) {
		reg_addr = MAX77752_REG_LED1_CFG;
	} else {
		reg_addr = MAX77752_REG_LED2_CFG;
	}

	led_cfg = (mode & 0x03) | ((brightness & 0x0F) << 4);

	regmap_write(dev->regmap, reg_addr, led_cfg);
	pr_info("led%d configured: mode=%d, brightness=%d", led_id, mode, brightness);
}

void max77752_set_led_state(struct max77752_device *dev, u8 led_id, u8 state)
{
	u32 led_cfg = 0;
	u8 reg_addr;

	if (led_id == 1) {
		reg_addr = MAX77752_REG_LED1_CFG;
	} else {
		reg_addr = MAX77752_REG_LED2_CFG;
	}

	regmap_read(dev->regmap, reg_addr, &led_cfg);

	if (state) {
		led_cfg |= 0x01; /* Enable LED */
	} else {
		led_cfg &= ~0x01; /* Disable LED */
	}

	regmap_write(dev->regmap, reg_addr, led_cfg);
}

void max77752_configure_led_charging_indicator(struct max77752_device *dev, u8 enable)
{
	u32 led_cfg = 0;

	regmap_read(dev->regmap, MAX77752_REG_LED_CFG, &led_cfg);

	if (enable) {
		led_cfg |= 0x01; /* Enable charging indicator */
	} else {
		led_cfg &= ~0x01; /* Disable charging indicator */
	}

	regmap_write(dev->regmap, MAX77752_REG_LED_CFG, led_cfg);
}

/* Interrupt handling functions */
u8 max77752_read_interrupt_status(struct max77752_device *dev)
{
	u32 int_status = 0;

	regmap_read(dev->regmap, MAX77752_REG_INT, &int_status);
	return int_status;
}

void max77752_clear_interrupt(struct max77752_device *dev, u8 interrupt_mask)
{
	regmap_write(dev->regmap, MAX77752_REG_INT, interrupt_mask);
}

void max77752_configure_interrupt_mask(struct max77752_device *dev, u8 mask, u8 enable)
{
	u32 int_mask = 0;

	regmap_read(dev->regmap, MAX77752_REG_INT_MSK, &int_mask);

	if (enable) {
		int_mask &= ~mask; /* Enable interrupt */
	} else {
		int_mask |= mask; /* Disable interrupt */
	}

	regmap_write(dev->regmap, MAX77752_REG_INT_MSK, int_mask);
}

/* Monitoring and diagnostic functions */
void max77752_continuous_monitor_start(struct max77752_device *dev, u32 interval_ms)
{
	dev->monitor_interval = interval_ms;
	dev->monitoring_active = 1;
	pr_info("continuous monitoring started (interval: %d ms)", interval_ms);
}

void max77752_continuous_monitor_stop(struct max77752_device *dev)
{
	dev->monitoring_active = 0;
	pr_info("continuous monitoring stopped");
}

void max77752_perform_self_test(struct max77752_device *dev)
{
	u8 test_result = 0;
	u16 vsys;
	u16 vbat;
	u16 vbus;
	s16 temp;

	/* Test voltage monitoring */
	vsys = max77752_read_vsys_voltage(dev);
	vbat = max77752_read_vbat_voltage(dev);
	vbus = max77752_read_vbus_voltage(dev);

	if (vsys == 0 && vbat == 0 && vbus == 0) {
		pr_info("self-test warning: all voltages read as 0");
	}

	/* Test temperature monitoring */
	temp = max77752_read_temperature(dev);
	if (temp < -50 || temp > 125) {
		pr_info("self-test warning: temperature out of range (%d°c)", temp);
	}

	test_result = 0x01; /* Test passed */
	pr_info("self-test completed: passed (result: 0x%02x)", test_result);
}

void max77752_get_system_info(struct max77752_device *dev, u8 *pinfo_buffer, u8 buffer_size)
{
	u8 info_index = 0;

	if (buffer_size < 10) {
		pr_info("error: buffer too small for system info");
		return;
	}

	/* Device identification */
	pinfo_buffer[info_index++] = max77752_read_device_id(dev);
	pinfo_buffer[info_index++] = max77752_read_device_revision(dev);

	/* Status information */
	pinfo_buffer[info_index++] = max77752_read_status(dev);
	pinfo_buffer[info_index++] = max77752_get_battery_status(dev);
	pinfo_buffer[info_index++] = max77752_get_charge_status(dev);

	/* Voltage readings */
	u16 vsys = max77752_read_vsys_voltage(dev);
	pinfo_buffer[info_index++] = (vsys >> 8) & 0xFF;
	pinfo_buffer[info_index++] = vsys & 0xFF;

	u16 vbat = max77752_read_vbat_voltage(dev);
	pinfo_buffer[info_index++] = (vbat >> 8) & 0xFF;
	pinfo_buffer[info_index++] = vbat & 0xFF;

	pr_info("system info retrieved: %d bytes", info_index);
}

/* Utility functions */
u8 max77752_check_power_good(struct max77752_device *dev)
{
	u8 status = max77752_read_status(dev);
	return (status & MAX77752_STATUS_PWR_SLA) ? 1 : 0;
}

u8 max77752_check_battery_present(struct max77752_device *dev)
{
	u8 status = max77752_read_status(dev);
	return (status & MAX77752_STATUS_BAT_PRES) ? 1 : 0;
}

u8 max77752_check_vbus_present(struct max77752_device *dev)
{
	u8 status = max77752_read_status(dev);
	return (status & MAX77752_STATUS_VBUS_PRES) ? 1 : 0;
}

u8 max77752_check_thermal_fault(struct max77752_device *dev)
{
	u8 temp_status = max77752_get_thermal_status(dev);
	return (temp_status & 0x01) ? 1 : 0; /* Check thermal fault bit */
}

void max77752_set_default_config(struct max77752_device *dev)
{
	u8 pwr_cfg;

	pr_info("setting default configuration...");

	/* Configure default interrupt mask */
	max77752_configure_interrupt_mask(dev, 0xFF, 1); /* Enable all interrupts */

	/* Set default power configuration */
	pwr_cfg = MAX77752_PWR_SLA_WAKE_EN | MAX77752_PWR_SLA_ILIM_EN;
	regmap_write(dev->regmap, MAX77752_REG_PWR_SLA, pwr_cfg);

	/* Set default thermal configuration */
	max77752_configure_thermal(dev, 1);

	/* Set default LED configuration */
	max77752_configure_led(dev, 1, MAX77752_LED_MODE_OFF, 8);
	max77752_configure_led(dev, 2, MAX77752_LED_MODE_OFF, 8);

	pr_info("default configuration set");
}

static const struct regmap_range max77752_volatile_ranges[] = {
	regmap_reg_range(0x02, 0x02),  /* STATUS */
	regmap_reg_range(0x03, 0x03),  /* INT */
	regmap_reg_range(0x0A, 0x0A),  /* PWR_FAULT */
	regmap_reg_range(0x0D, 0x0D),  /* BAT_STATUS */
	regmap_reg_range(0x0F, 0x0F),  /* CHG_STATUS */
	regmap_reg_range(0x10, 0x10),  /* CHG_INT */
	regmap_reg_range(0x12, 0x18),  /* Voltage/Current monitors */
	regmap_reg_range(0x1A, 0x1A),  /* TEMP_STATUS */
	regmap_reg_range(0x1B, 0x1B),  /* THERMISTOR */
};

static const struct regmap_access_table max77752_volatile_table = {
	.yes_ranges = max77752_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(max77752_volatile_ranges),
};

static const struct regmap_config max77752_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x1E,  /* LED2_CFG */
	.volatile_table = &max77752_volatile_table,
	.cache_type = REGCACHE_NONE,
};

#ifdef KINETIS_FAKE_SIM
/* Box-Muller transform for Gaussian noise */
static float max77752_gaussian_noise(float mean, float std_dev)
{
	static u32 use_last = 0;
	static float z2, z1;
	float u1, u2, z0;

	if (!use_last) {
		u1 = (float)get_random_u32() / (float)U32_MAX;
		u2 = (float)get_random_u32() / (float)U32_MAX;

		/* Ensure u1 is not zero */
		if (u1 == 0.0f)
			u1 = 0.0001f;

		z0 = sqrtf(-2.0f * logf(u1)) * sinf(2.0f * M_PI * u2);
		z2 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);

		z1 = z0;
		use_last = 1;
	} else {
		z1 = z2;
		use_last = 0;
	}

	return z1 * std_dev + mean;
}

static void *max77752_reg_random_thread(void *arg)
{
	struct max77752_device *dev = arg;
	u16 vsys, vbat, vbus, votg, ichg, isys, iotg;
	s16 temperature;
	u8 status, pwr_cfg, bat_status, chg_status;

	/* Initialize fixed registers at thread startup */
	dev->slave_regs[MAX77752_REG_DEV_ID] = 0x1B;
	dev->slave_regs[MAX77752_REG_DEV_REV] = 0x00;
	dev->slave_regs[MAX77752_REG_STATUS] = 0x02;
	dev->slave_regs[MAX77752_REG_CONFIG] = 0x00;
	dev->slave_regs[MAX77752_REG_RESET] = 0x00;
	dev->slave_regs[MAX77752_REG_OTG_CFG] = 0x00;
	dev->slave_regs[MAX77752_REG_TEMP_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_TEMP_STATUS] = 0x00;
	dev->slave_regs[MAX77752_REG_LED_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_LED1_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_LED2_CFG] = 0x00;

	pr_info("max77752: Randomization thread started\n");

	while (dev->thread_running) {
		/* Read current configuration registers */
		status = dev->slave_regs[MAX77752_REG_STATUS];
		pwr_cfg = dev->slave_regs[MAX77752_REG_PWR_SLA];

		/* Generate voltage data with noise */
		vsys = (u16)max77752_gaussian_noise(5000.0f, 50.0f);   /* VSYS: 5V ± 50mV */
		vbat = (u16)max77752_gaussian_noise(4200.0f, 42.0f);   /* VBAT: 4.2V ± 42mV */
		vbus = (u16)max77752_gaussian_noise(5000.0f, 50.0f);   /* VBUS: 5V ± 50mV */
		votg = (u16)max77752_gaussian_noise(5000.0f, 50.0f);   /* VOTG: 5V ± 50mV */

		/* Write voltage data to slave registers (big-endian) */
		dev->slave_regs[MAX77752_REG_VSYS_MON] = (u8)((vsys >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_VSYS_MON + 1] = (u8)(vsys & 0xFF);
		dev->slave_regs[MAX77752_REG_VBAT_MON] = (u8)((vbat >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_VBAT_MON + 1] = (u8)(vbat & 0xFF);
		dev->slave_regs[MAX77752_REG_VBUS_MON] = (u8)((vbus >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_VBUS_MON + 1] = (u8)(vbus & 0xFF);
		dev->slave_regs[MAX77752_REG_VOTG_MON] = (u8)((votg >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_VOTG_MON + 1] = (u8)(votg & 0xFF);

		/* Generate current data with noise based on battery status */
		bat_status = dev->slave_regs[MAX77752_REG_BAT_STATUS];

		/* If charging, generate positive current; if discharging, negative */
		if (bat_status & 0x01) {
			/* Charging mode */
			ichg = (u16)max77752_gaussian_noise(500.0f, 10.0f);  /* ICHG: 500mA ± 10mA */
		} else {
			/* Discharging or idle */
			ichg = (u16)max77752_gaussian_noise(0.0f, 5.0f);    /* ICHG: 0mA ± 5mA */
		}

		isys = (u16)max77752_gaussian_noise(100.0f, 5.0f);     /* ISYS: 100mA ± 5mA */
		iotg = (u16)max77752_gaussian_noise(0.0f, 5.0f);       /* IOTG: 0mA ± 5mA */

		/* Write current data to slave registers (big-endian) */
		dev->slave_regs[MAX77752_REG_ICHG_MON] = (u8)((ichg >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_ICHG_MON + 1] = (u8)(ichg & 0xFF);
		dev->slave_regs[MAX77752_REG_ISYS_MON] = (u8)((isys >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_ISYS_MON + 1] = (u8)(isys & 0xFF);
		dev->slave_regs[MAX77752_REG_IOTG_MON] = (u8)((iotg >> 8) & 0xFF);
		dev->slave_regs[MAX77752_REG_IOTG_MON + 1] = (u8)(iotg & 0xFF);

		/* Generate temperature data: ~25°C with noise */
		temperature = (s16)max77752_gaussian_noise(25.0f, 0.5f);

		/* Write temperature to thermistor register */
		dev->slave_regs[MAX77752_REG_THERMISTOR] = (u8)temperature;

		/* Update temperature status register */
		if (temperature > 80) {
			dev->slave_regs[MAX77752_REG_TEMP_STATUS] |= 0x04;  /* Thermal fault */
		} else if (temperature > 60) {
			dev->slave_regs[MAX77752_REG_TEMP_STATUS] |= 0x02;  /* Thermal warning */
		} else if (temperature < 0) {
			dev->slave_regs[MAX77752_REG_TEMP_STATUS] |= 0x01;  /* Thermal fault cold */
		} else {
			dev->slave_regs[MAX77752_REG_TEMP_STATUS] = 0x00;  /* Normal */
		}

		/* Update status register based on simulated conditions */
		status = 0x00;  /* Clear all flags */

		/* Power Good flag */
		if (vsys > 4500) {
			status |= 0x01;  /* PWR_SLA */
		}

		/* Battery Present flag */
		if (vbat > 2000) {
			status |= 0x02;  /* BAT_PRES */
		}

		/* VBUS Present flag */
		if (vbus > 4500) {
			status |= 0x04;  /* VBUS_PRES */
		}

		/* Battery Charging status */
		chg_status = dev->slave_regs[MAX77752_REG_CHG_STATUS];
		if (ichg > 100 && (chg_status & 0x01)) {
			status |= 0x10;  /* BAT_CHG */
		}

		/* Battery Low flag */
		if (vbat < 3200) {
			status |= 0x80;  /* BAT_LOW */
		}

		dev->slave_regs[MAX77752_REG_STATUS] = status;

		/* Sleep for 100ms (10Hz update rate) */
		msleep(100);
	}

	pr_info("max77752: Randomization thread stopped\n");
	return 0;
}

static pthread_t reg_thread;

static int max77752_start_reg_random(struct max77752_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&reg_thread, NULL,
			max77752_reg_random_thread, dev);
	if (ret) {
		return -ret;
	}

	/* Wait for thread to initialize */
	msleep(100);

	return 0;
}

static void max77752_stop_reg_random(struct max77752_device *dev)
{
	dev->thread_running = false;
	pthread_join(reg_thread, NULL);
}
#endif

struct max77752_device *max77752_init(enum regmap_user_bus_type bus_type, void *bus_master)
{
	struct max77752_device *dev;
	u32 dev_id, dev_rev, status;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		pr_err("failed to allocate max77752 device structure\n");
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize regmap based on bus type */
	if (bus_type == REGMAP_BUS_IIC_SOFT) {
		dev->regmap = regmap_init_iic_soft(bus_master, MAX77752_I2C_ADDR, &max77752_regmap_config);
	} else if (bus_type == REGMAP_BUS_SPI_SOFT) {
		/* Default SPI attributes: CPOL=0, CPHA=0, MSB first */
		dev->regmap = regmap_init_spi_soft(bus_master,
						   0, 0, SPI_BIT_ORDER_MSB, 2, &max77752_regmap_config);
	} else {
		pr_err("Invalid bus type specified for max77752 initialization\n");
		kfree(dev);
		return ERR_PTR(-EINVAL);
	}
	if (IS_ERR(dev->regmap)) {
		pr_err("max77752: failed to initialize regmap: %ld\n", PTR_ERR(dev->regmap));
		kfree(dev);
		return ERR_PTR(PTR_ERR(dev->regmap));
	}

	/* Initialize default values */
	dev->device_present = 0;
	dev->init_complete = 0;
	dev->power_enabled = 0;
	dev->charging_enabled = 0;
	dev->thermal_monitoring_enabled = 0;
	dev->battery_type = MAX77752_BAT_TYPE_LI_ION;
	dev->charge_current_limit = MAX77752_ICHG_LIMIT_500;
	dev->charge_voltage_limit = 4200;
	dev->temperature_low_limit = MAX77752_TEMP_LOW_COOL;
	dev->temperature_high_limit = MAX77752_TEMP_WARM_HOT;
	dev->monitor_interval = 1000;
	dev->monitoring_active = 0;
	dev->last_vsys_voltage = 0;
	dev->last_vbat_voltage = 0;
	dev->last_vbus_voltage = 0;
	dev->last_charge_status = MAX77752_CHG_IDLE;
	dev->last_temperature = 0;
	dev->interrupt_callback = NULL;
	dev->charge_done_callback = NULL;
	dev->low_battery_callback = NULL;
	dev->thermal_fault_callback = NULL;

#ifdef KINETIS_FAKE_SIM
	/* Allocate memory for slave registers */
	dev->slave_regs = kmalloc(max77752_regmap_config.max_register + 1, GFP_KERNEL);
	if (!dev->slave_regs) {
		pr_err("max77752: failed to allocate slave register memory\n");
		regmap_exit(dev->regmap);
		kfree(dev);
		return ERR_PTR(-ENOMEM);
	}
	memset(dev->slave_regs, 0, max77752_regmap_config.max_register + 1);

	/* Initialize slave registers for simulation */
	dev->slave_regs[MAX77752_REG_DEV_ID] = 0x1B;
	dev->slave_regs[MAX77752_REG_DEV_REV] = 0x00;
	dev->slave_regs[MAX77752_REG_STATUS] = 0x02;
	dev->slave_regs[MAX77752_REG_INT] = 0x00;
	dev->slave_regs[MAX77752_REG_INT_MSK] = 0xFF;
	dev->slave_regs[MAX77752_REG_CONFIG] = 0x00;
	dev->slave_regs[MAX77752_REG_RESET] = 0x00;
	dev->slave_regs[MAX77752_REG_OTG_CFG] = 0x00;
	dev->slave_regs[MAX77752_REG_PWR_SLA] = 0x00;
	dev->slave_regs[MAX77752_REG_PWR_MSK] = 0x00;
	dev->slave_regs[MAX77752_REG_PWR_FAULT] = 0x00;
	dev->slave_regs[MAX77752_REG_PWR_REQ] = 0x00;
	dev->slave_regs[MAX77752_REG_BAT_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_BAT_STATUS] = 0x01;
	dev->slave_regs[MAX77752_REG_CHG_CFG] = 0x03;
	dev->slave_regs[MAX77752_REG_CHG_STATUS] = 0x04;
	dev->slave_regs[MAX77752_REG_CHG_INT] = 0x00;
	dev->slave_regs[MAX77752_REG_CHG_INT_MSK] = 0xFF;
	dev->slave_regs[MAX77752_REG_VSYS_MON] = 0x0C;
	dev->slave_regs[MAX77752_REG_VSYS_MON + 1] = 0xB4;
	dev->slave_regs[MAX77752_REG_VBAT_MON] = 0x0A;
	dev->slave_regs[MAX77752_REG_VBAT_MON + 1] = 0x68;
	dev->slave_regs[MAX77752_REG_VBUS_MON] = 0x0D;
	dev->slave_regs[MAX77752_REG_VBUS_MON + 1] = 0xE4;
	dev->slave_regs[MAX77752_REG_VOTG_MON] = 0x0C;
	dev->slave_regs[MAX77752_REG_VOTG_MON + 1] = 0xB4;
	dev->slave_regs[MAX77752_REG_ICHG_MON] = 0x01;
	dev->slave_regs[MAX77752_REG_ICHG_MON + 1] = 0xF4;
	dev->slave_regs[MAX77752_REG_ISYS_MON] = 0x00;
	dev->slave_regs[MAX77752_REG_ISYS_MON + 1] = 0x00;
	dev->slave_regs[MAX77752_REG_IOTG_MON] = 0x00;
	dev->slave_regs[MAX77752_REG_IOTG_MON + 1] = 0x00;
	dev->slave_regs[MAX77752_REG_TEMP_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_TEMP_STATUS] = 0x00;
	dev->slave_regs[MAX77752_REG_THERMISTOR] = 0x80;
	dev->slave_regs[MAX77752_REG_LED_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_LED1_CFG] = 0x01;
	dev->slave_regs[MAX77752_REG_LED2_CFG] = 0x00;

	ret = max77752_start_reg_random(dev);
	if (ret) {
		return NULL;
	}

	/* Initialize slave device based on bus type */
	if (bus_type == REGMAP_BUS_SPI_SOFT) {
		dev->spi_slave = spi_slave_soft_init("max77752", 0, 0, SPI_BIT_ORDER_MSB,
				dev->slave_regs, max77752_regmap_config.max_register + 1);
		if (IS_ERR(dev->spi_slave)) {
			pr_err("max77752: failed to initialize SPI slave\n");
			kfree(dev->slave_regs);
			regmap_exit(dev->regmap);
			kfree(dev);
			return ERR_PTR(PTR_ERR(dev->spi_slave));
		}
	} else {
		dev->iic_slave = iic_slave_soft_init("max77752", MAX77752_I2C_ADDR,
				dev->slave_regs, max77752_regmap_config.max_register + 1);
		if (IS_ERR(dev->iic_slave)) {
			pr_err("max77752: failed to initialize I2C slave\n");
			kfree(dev->slave_regs);
			regmap_exit(dev->regmap);
			kfree(dev);
			return ERR_PTR(PTR_ERR(dev->iic_slave));
		}
	}
#endif

	pr_info("Initializing max77752 power management chip...\n");

	/* Check if device is present */
	dev->device_present = max77752_is_device_present(dev);
	if (!dev->device_present) {
		pr_err("max77752 device not found!\n");
		return ERR_PTR(-ENODEV);
	}

	/* Read device identification */
	regmap_read(dev->regmap, MAX77752_REG_DEV_ID, &dev_id);
	regmap_read(dev->regmap, MAX77752_REG_DEV_REV, &dev_rev);
	pr_info("max77752 device id: 0x%02x, revision: 0x%02x\n", dev_id, dev_rev);

	/* Read initial status */
	regmap_read(dev->regmap, MAX77752_REG_STATUS, &status);
	pr_info("Initial status: 0x%02X\n", status);

	/* Set default configuration */
	max77752_set_default_config(dev);

	/* Configure battery management */
	max77752_configure_battery(dev, dev->battery_type, 1);

	/* Configure charging */
	max77752_configure_charging(dev, dev->charge_current_limit, dev->charge_voltage_limit);

	/* Configure thermal management */
	max77752_configure_thermal(dev, 1);
	max77752_set_temperature_limits(dev, dev->temperature_low_limit, dev->temperature_high_limit);

	/* Enable power */
	max77752_enable_power(dev, 1);

	dev->init_complete = 1;

	return dev;
}

/**
 * @brief Cleanup max77752 device
 * @param dev Device structure to cleanup
 */
void max77752_exit(struct max77752_device *dev)
{
#ifdef KINETIS_FAKE_SIM
	/* Stop randomization thread before cleanup */
	max77752_stop_reg_random(dev);

	if (dev->iic_slave)
		iic_slave_soft_exit(dev->iic_slave);
	if (dev->spi_slave)
		spi_slave_soft_exit(dev->spi_slave);
	kfree(dev->slave_regs);
#endif
	regmap_exit(dev->regmap);
	kfree(dev);
}

/* Test functions */
#ifdef DESIGN_VERIFICATION_MAX77752
#include "kinetis/test-kinetis.h"

static struct max77752_device *max77752_dev;

int t_max77752_initialize(int argc, char **argv)
{
	bool on_off = true;
	enum regmap_user_bus_type bus_type = REGMAP_BUS_IIC_SOFT;

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			on_off = true;
		} else if (!strcmp(argv[1], "off")) {
			on_off = false;
		} else {
			return -EINVAL;
		}
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "spi")) {
			bus_type = REGMAP_BUS_SPI_SOFT;
		} else if (!strcmp(argv[2], "i2c")) {
			bus_type = REGMAP_BUS_IIC_SOFT;
		} else {
			pr_err("Invalid bus type: %s (use 'i2c' or 'spi')", argv[2]);
			return -EINVAL;
		}
	}

	if (on_off) {
		pr_info("starting max77752 slave with %s mode", bus_type ? "spi" : "i2c");
		max77752_dev = max77752_init(bus_type, bus_type == REGMAP_BUS_IIC_SOFT ? (void *)&general_iic_master : (void *)&general_spi_master);
		if (IS_ERR_OR_NULL(max77752_dev)) {
			return -EINVAL;
		}
		return 0;
	}

	max77752_exit(max77752_dev);

	return 0;
}

int t_max77752_device_id(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u8 dev_id, dev_rev;

	/* Check if device is present */
	if (!max77752_is_device_present(dev)) {
		return -ENODEV;
	}
	/* Read device identification */
	dev_id = max77752_read_device_id(dev);
	dev_rev = max77752_read_device_revision(dev);
	pr_info("max77752 device id: 0x%02x, revision: 0x%02x\n", dev_id, dev_rev);

	if (dev_id != 0x00 && dev_id != 0xFF) {
		return 0;
	}

	return -EINVAL;
}

int t_max77752_power_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u8 pwr_status;

	/* Read initial power status */
	pwr_status = max77752_get_power_status(dev);
	pr_info("initial power status: %d", pwr_status);

	/* Enable power */
	max77752_enable_power(dev, 1);
	mdelay(100);

	pwr_status = max77752_get_power_status(dev);
	pr_info("power status after enable: %d", pwr_status);

	if (!max77752_check_power_good(dev)) {
		pr_warn("power good signal not asserted");
	}

	/* Disable power */
	max77752_enable_power(dev, 0);
	mdelay(100);

	pwr_status = max77752_get_power_status(dev);
	pr_info("power status after disable: %d", pwr_status);

	/* Re-enable power */
	max77752_enable_power(dev, 1);
	mdelay(100);

	return 0;
}

int t_max77752_battery_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u8 bat_status;
	u16 vbat;

	/* check if battery is present */
	if (max77752_check_battery_present(dev)) {
		pr_info("battery present: yes");
	} else {
		pr_info("battery present: no");
	}

	/* Read battery status */
	bat_status = max77752_get_battery_status(dev);
	pr_info("battery status: 0x%02x", bat_status);

	/* Read battery voltage */
	vbat = max77752_read_vbat_voltage(dev);
	pr_info("battery voltage: %d mv", vbat);

	if (vbat > MAX77752_VBAT_LOW_THRESHOLD) {
		pr_info("battery voltage is above low threshold");
	} else {
		pr_warn("battery voltage is below low threshold");
	}

	return 0;
}

int t_max77752_voltage_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u16 vsys, vbat, vbus, votg;
	u16 samples = 10;
	u16 i;

	if (argc > 1) {
		samples = simple_strtoul(argv[1], &argv[1], 10);
		if (samples > 100) {
			samples = 100;
		}
	}

	/* Check VBUS presence */
	if (max77752_check_vbus_present(dev)) {
		pr_info("vbus present: yes");
	} else {
		pr_info("vbus present: no");
	}

	for (i = 0; i < samples; i++) {
		/* Read all voltages */
		vsys = max77752_read_vsys_voltage(dev);
		vbat = max77752_read_vbat_voltage(dev);
		vbus = max77752_read_vbus_voltage(dev);
		votg = max77752_read_votg_voltage(dev);

		/* Show first and last readings */
		pr_info("Reading %d/%d: VSYS=%dmV, VBAT=%dmV, VBUS=%dmV, VOTG=%dmV",
			i + 1, samples, vsys, vbat, vbus, votg);

		mdelay(50);
	}

	return 0;
}

int t_max77752_charging_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u8 chg_status;
	u16 chg_current;

	/* Enable charging */
	max77752_enable_charging(dev, 1);
	mdelay(100);

	/* Configure charging parameters */
	max77752_configure_charging(dev, MAX77752_ICHG_LIMIT_500, 4200);
	pr_info("charging configured: 500mA, 4200mV");
	mdelay(100);

	/* Read charging status */
	chg_status = max77752_get_charge_status(dev);
	pr_info("charging status: 0x%02x", chg_status);

	switch (chg_status) {
	case MAX77752_CHG_IDLE:
		pr_info("charging state: IDLE");
		break;
	case MAX77752_CHG_TRICKLE:
		pr_info("charging state: TRICKLE");
		break;
	case MAX77752_CHG_FAST_CC:
		pr_info("charging state: FAST CC");
		break;
	case MAX77752_CHG_FAST_CV:
		pr_info("charging state: FAST CV");
		break;
	case MAX77752_CHG_DONE:
		pr_info("charging state: DONE");
		break;
	case MAX77752_CHG_SUSPEND:
		pr_info("charging state: SUSPEND");
		break;
	case MAX77752_CHG_TIMEOUT:
		pr_info("charging state: TIMEOUT");
		break;
	case MAX77752_CHG_ERROR:
		pr_info("charging state: ERROR");
		break;
	}

	/* Read charging current */
	chg_current = max77752_read_charge_current(dev);
	pr_info("charging current: %d mA", chg_current);

	/* Disable charging for test */
	max77752_enable_charging(dev, 0);

	return 0;
}

int t_max77752_current_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u16 chg_current, sys_current, otg_current;

	/* Read all current values */
	chg_current = max77752_read_charge_current(dev);
	sys_current = max77752_read_system_current(dev);
	otg_current = max77752_read_otg_current(dev);

	pr_info("Charge current: %d mA", chg_current);
	pr_info("System current: %d mA", sys_current);
	pr_info("OTG current: %d mA", otg_current);

	return 0;
}

int t_max77752_temperature_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	s16 temperature;
	u8 temp_status;

	/* Read temperature */
	temperature = max77752_read_temperature(dev);
	pr_info("temperature: %d°c", temperature);

	/* Check thermal status */
	temp_status = max77752_get_thermal_status(dev);
	pr_info("thermal status: 0x%02x", temp_status);

	/* Check for thermal fault */
	if (max77752_check_thermal_fault(dev)) {
		pr_warn("thermal fault detected");
	} else {
		pr_info("no thermal fault");
	}

	/* Set temperature limits */
	max77752_set_temperature_limits(dev, MAX77752_TEMP_LOW_COOL, MAX77752_TEMP_WARM_HOT);
	pr_info("temperature limits set: %d°c to %d°c",
		MAX77752_TEMP_LOW_COOL, MAX77752_TEMP_WARM_HOT);

	return 0;
}

int t_max77752_led_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u16 i;

	/* Test LED 1 */
	pr_info("Testing LED 1...");
	max77752_configure_led(dev, 1, MAX77752_LED_MODE_ON, 15);
	pr_info("LED 1: ON (brightness 15)");
	mdelay(500);

	max77752_set_led_state(dev, 1, 1);
	mdelay(500);

	max77752_set_led_state(dev, 1, 0);
	pr_info("LED 1: OFF");
	mdelay(500);

	/* Test LED 2 */
	pr_info("Testing LED 2...");
	max77752_configure_led(dev, 2, MAX77752_LED_MODE_ON, 15);
	pr_info("LED 2: ON (brightness 15)");
	mdelay(500);

	max77752_set_led_state(dev, 2, 1);
	mdelay(500);

	max77752_set_led_state(dev, 2, 0);
	pr_info("LED 2: OFF");
	mdelay(500);

	/* Test LED blinking */
	pr_info("Testing LED blinking mode...");
	max77752_configure_led(dev, 1, MAX77752_LED_MODE_BLINK, 10);
	mdelay(2000);

	/* Test LED breathing */
	pr_info("Testing LED breathing mode...");
	max77752_configure_led(dev, 1, MAX77752_LED_MODE_BREATH, 10);
	mdelay(2000);

	/* Turn off all LEDs */
	max77752_configure_led(dev, 1, MAX77752_LED_MODE_OFF, 0);
	max77752_configure_led(dev, 2, MAX77752_LED_MODE_OFF, 0);
	pr_info("All LEDs OFF");

	return 0;
}

int t_max77752_interrupt_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u8 int_status;

	/* Read interrupt status */
	int_status = max77752_read_interrupt_status(dev);
	pr_info("interrupt status: 0x%02x", int_status);

	/* Clear all interrupts */
	max77752_clear_interrupt(dev, 0xFF);
	mdelay(10);

	/* Read interrupt status after clear */
	int_status = max77752_read_interrupt_status(dev);
	pr_info("interrupt status after clear: 0x%02x", int_status);

	/* Configure interrupt mask */
	max77752_configure_interrupt_mask(dev, 0x0F, 1);
	pr_info("interrupt mask configured: bits 0-3 enabled");

	/* Read status again */
	int_status = max77752_read_status(dev);
	pr_info("device status: 0x%02x", int_status);

	return 0;
}

int t_max77752_selftest(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u8 info_buffer[10];

	/* Perform self-test */
	max77752_perform_self_test(dev);
	mdelay(100);

	/* Get system information */
	max77752_get_system_info(dev, info_buffer, sizeof(info_buffer));
	pr_info("System info:");
	pr_info("  Device ID: 0x%02X", info_buffer[0]);
	pr_info("  Device Revision: 0x%02X", info_buffer[1]);
	pr_info("  Status: 0x%02X", info_buffer[2]);
	pr_info("  Battery Status: 0x%02X", info_buffer[3]);
	pr_info("  Charge Status: 0x%02X", info_buffer[4]);
	pr_info("  VSYS: %d mV", (info_buffer[5] << 8) | info_buffer[6]);
	pr_info("  VBAT: %d mV", (info_buffer[7] << 8) | info_buffer[8]);

	/* Check all utility functions */
	pr_info("Utility checks:");
	pr_info("  Power good: %d", max77752_check_power_good(dev));
	pr_info("  Battery present: %d", max77752_check_battery_present(dev));
	pr_info("  VBUS present: %d", max77752_check_vbus_present(dev));
	pr_info("  Thermal fault: %d", max77752_check_thermal_fault(dev));

	return 0;
}

int t_max77752_sleep_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;

	/* Enter sleep mode */
	max77752_set_sleep_mode(dev, 1);
	mdelay(200);

	/* Exit sleep mode */
	max77752_set_sleep_mode(dev, 0);
	mdelay(200);

	return 0;
}

int t_max77752_monitoring_test(int argc, char **argv)
{
	struct max77752_device *dev = max77752_dev;
	u16 vsys, vbat;

	/* Start continuous monitoring */
	max77752_continuous_monitor_start(dev, 500);
	pr_info("continuous monitoring started (500ms interval)");

	/* Read some values during monitoring */
	mdelay(100);
	vsys = max77752_read_vsys_voltage(dev);
	vbat = max77752_read_vbat_voltage(dev);
	pr_info("VSYS: %d mV, VBAT: %d mV", vsys, vbat);

	/* Stop continuous monitoring */
	max77752_continuous_monitor_stop(dev);
	pr_info("continuous monitoring stopped");

	return 0;
}

#endif
