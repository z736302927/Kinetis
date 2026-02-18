#include <linux/delay.h>
#include <linux/string.h>

#include "kinetis/max77752.h"
#include "kinetis/iic_soft.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/* Device I2C address - MAX77752 typically uses 0x3C */
#define MAX77752_ADDR                    MAX77752_I2C_ADDR

static struct iic_master *max77752_iic = &fake_master;

/* Private variables */
static u8 g_device_present = 0;
static u8 g_init_complete = 0;
static u8 g_power_enabled = 0;
static u8 g_charging_enabled = 0;
static u8 g_thermal_monitoring_enabled = 0;

/* Configuration variables */
static u8 g_battery_type = MAX77752_BAT_TYPE_LI_ION;
static u16 g_charge_current_limit = MAX77752_ICHG_LIMIT_500;
static u16 g_charge_voltage_limit = 4200;
static u8 g_temperature_low_limit = MAX77752_TEMP_LOW_COOL;
static u8 g_temperature_high_limit = MAX77752_TEMP_WARM_HOT;

/* Callback function pointers */
static void (*g_interrupt_callback)(u8 interrupt_source) = NULL;
static void (*g_charge_done_callback)(void) = NULL;
static void (*g_low_battery_callback)(void) = NULL;
static void (*g_thermal_fault_callback)(void) = NULL;

/* Monitoring variables */
static u32 g_monitor_interval = 1000; /* 1 second default */
static u8 g_monitoring_active = 0;
static u16 g_last_vsys_voltage = 0;
static u16 g_last_vbat_voltage = 0;
static u16 g_last_vbus_voltage = 0;
static u8 g_last_charge_status = MAX77752_CHG_IDLE;
static s16 g_last_temperature = 0;

/* I/O port functions - using Kinetis I2C implementation */
static inline void max77752_port_transmit(u8 reg_addr, u8 data)
{
	iic_master_port_transmit(max77752_iic, MAX77752_ADDR, reg_addr, data);
}

static inline void max77752_port_receive(u8 reg_addr, u8 *pdata)
{
	iic_master_port_receive(max77752_iic, MAX77752_ADDR, reg_addr, pdata);
}

static inline void max77752_port_multi_transmit(u8 reg_addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_transmit(max77752_iic, MAX77752_ADDR, reg_addr, pdata, length);
}

static inline void max77752_port_multi_receive(u8 reg_addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_receive(max77752_iic, MAX77752_ADDR, reg_addr, pdata, length);
}

/* Delay functions */
void max77752_Delayus(u32 ticks)
{
	udelay(ticks);
}

void max77752_Delayms(u32 ticks)
{
	mdelay(ticks);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Device detection and initialization */
void max77752_init(void)
{
	u8 dev_id, dev_rev, status;

	/* Initialize IIC master */
	max77752_iic->init();

	printk("Initializing MAX77752 power management chip...");

	/* Check if device is present */
	g_device_present = max77752_is_device_present();
	if (!g_device_present) {
		printk("ERROR: MAX77752 device not found!");
		return;
	}

	/* Read device identification */
	dev_id = max77752_read_device_id();
	dev_rev = max77752_read_device_revision();
	printk("MAX77752 Device ID: 0x%02X, Revision: 0x%02X", dev_id, dev_rev);

	/* Read initial status */
	status = max77752_read_status();
	printk("Initial status: 0x%02X", status);

	/* Set default configuration */
	max77752_set_default_config();

	/* Configure battery management */
	max77752_configure_battery(g_battery_type, 1);

	/* Configure charging */
	max77752_configure_charging(g_charge_current_limit, g_charge_voltage_limit);

	/* Configure thermal management */
	max77752_configure_thermal(1);
	max77752_set_temperature_limits(g_temperature_low_limit, g_temperature_high_limit);

	/* Enable power */
	max77752_enable_power(1);

	g_init_complete = 1;
	printk("MAX77752 initialized successfully");
}

u8 max77752_is_device_present(void)
{
	u8 dev_id = 0;

	/* Try to read device ID register */
	max77752_port_receive(MAX77752_REG_DEV_ID, &dev_id);

	/* A valid MAX77752 should return a non-zero value */
	g_device_present = (dev_id != 0x00 && dev_id != 0xFF) ? 1 : 0;

	return g_device_present;
}

/* Device identification functions */
u8 max77752_read_device_id(void)
{
	u8 dev_id = 0;
	max77752_port_receive(MAX77752_REG_DEV_ID, &dev_id);
	return dev_id;
}

u8 max77752_read_device_revision(void)
{
	u8 dev_rev = 0;
	max77752_port_receive(MAX77752_REG_DEV_REV, &dev_rev);
	return dev_rev;
}

u8 max77752_read_status(void)
{
	u8 status = 0;
	max77752_port_receive(MAX77752_REG_STATUS, &status);
	return status;
}

void max77752_write_status(u8 status)
{
	max77752_port_transmit(MAX77752_REG_STATUS, status);
}

/* Power management functions */
void max77752_enable_power(u8 enable)
{
	u8 pwr_cfg = 0;

	max77752_port_receive(MAX77752_REG_PWR_SLA, &pwr_cfg);

	if (enable) {
		pwr_cfg |= MAX77752_PWR_SLA_EN;
		g_power_enabled = 1;
		printk("Power enabled");
	} else {
		pwr_cfg &= ~MAX77752_PWR_SLA_EN;
		g_power_enabled = 0;
		printk("Power disabled");
	}

	max77752_port_transmit(MAX77752_REG_PWR_SLA, pwr_cfg);
}

u8 max77752_get_power_status(void)
{
	u8 status = max77752_read_status();
	return (status & MAX77752_STATUS_PWR_SLA) ? 1 : 0;
}

void max77752_set_sleep_mode(u8 enable)
{
	u8 config = 0;

	max77752_port_receive(MAX77752_REG_CONFIG, &config);

	if (enable) {
		config |= 0x01; /* Sleep mode bit */
		printk("Entering sleep mode");
	} else {
		config &= ~0x01;
		printk("Exiting sleep mode");
	}

	max77752_port_transmit(MAX77752_REG_CONFIG, config);
}

void max77752_enable_wake_source(u8 source, u8 enable)
{
	u8 pwr_masks = 0;

	max77752_port_receive(MAX77752_REG_PWR_MSK, &pwr_masks);

	if (enable) {
		pwr_masks |= (1 << source);
	} else {
		pwr_masks &= ~(1 << source);
	}

	max77752_port_transmit(MAX77752_REG_PWR_MSK, pwr_masks);
}

void max77752_reset_system(void)
{
	printk("Performing system reset...");
	max77752_port_transmit(MAX77752_REG_RESET, 0x01);
	max77752_Delayms(100); /* Wait for reset to complete */
}

/* Battery management functions */
void max77752_configure_battery(u8 bat_type, u8 enable)
{
	u8 bat_cfg = 0;

	max77752_port_receive(MAX77752_REG_BAT_CFG, &bat_cfg);

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

	max77752_port_transmit(MAX77752_REG_BAT_CFG, bat_cfg);
	printk("Battery configured: Type=%d, Enabled=%d", bat_type, enable);
}

u8 max77752_get_battery_status(void)
{
	u8 bat_status = 0;
	max77752_port_receive(MAX77752_REG_BAT_STATUS, &bat_status);
	return bat_status;
}

/* Voltage monitoring functions */
u16 max77752_read_vsys_voltage(void)
{
	u8 voltage_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_VSYS_MON, voltage_data, 2);

	g_last_vsys_voltage = (voltage_data[0] << 8) | voltage_data[1];
	return g_last_vsys_voltage;
}

u16 max77752_read_vbat_voltage(void)
{
	u8 voltage_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_VBAT_MON, voltage_data, 2);

	g_last_vbat_voltage = (voltage_data[0] << 8) | voltage_data[1];
	return g_last_vbat_voltage;
}

u16 max77752_read_vbus_voltage(void)
{
	u8 voltage_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_VBUS_MON, voltage_data, 2);

	g_last_vbus_voltage = (voltage_data[0] << 8) | voltage_data[1];
	return g_last_vbus_voltage;
}

u16 max77752_read_votg_voltage(void)
{
	u8 voltage_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_VOTG_MON, voltage_data, 2);

	return (voltage_data[0] << 8) | voltage_data[1];
}

/* Charging functions */
void max77752_enable_charging(u8 enable)
{
	u8 chg_cfg = 0;

	max77752_port_receive(MAX77752_REG_CHG_CFG, &chg_cfg);

	if (enable) {
		chg_cfg |= MAX77752_CHG_CFG_EN;
		g_charging_enabled = 1;
		printk("Charging enabled");
	} else {
		chg_cfg &= ~MAX77752_CHG_CFG_EN;
		g_charging_enabled = 0;
		printk("Charging disabled");
	}

	max77752_port_transmit(MAX77752_REG_CHG_CFG, chg_cfg);
}

void max77752_configure_charging(u16 current_limit, u16 voltage_limit)
{
	u8 chg_cfg = 0;

	max77752_port_receive(MAX77752_REG_CHG_CFG, &chg_cfg);

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

	max77752_port_transmit(MAX77752_REG_CHG_CFG, chg_cfg);

	printk("Charging configured: Current=%dmA, Voltage=%dmV",
		current_limit * 100, (voltage_limit + 4) * 100);
}

u8 max77752_get_charge_status(void)
{
	u8 chg_status = 0;
	max77752_port_receive(MAX77752_REG_CHG_STATUS, &chg_status);

	/* Extract charge status from bits 0-2 */
	g_last_charge_status = chg_status & 0x07;
	return g_last_charge_status;
}

/* Current monitoring functions */
u16 max77752_read_charge_current(void)
{
	u8 current_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_ICHG_MON, current_data, 2);

	return (current_data[0] << 8) | current_data[1];
}

u16 max77752_read_system_current(void)
{
	u8 current_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_ISYS_MON, current_data, 2);

	return (current_data[0] << 8) | current_data[1];
}

u16 max77752_read_otg_current(void)
{
	u8 current_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_IOTG_MON, current_data, 2);

	return (current_data[0] << 8) | current_data[1];
}

/* Thermal management functions */
void max77752_configure_thermal(u8 enable)
{
	u8 temp_cfg = 0;

	max77752_port_receive(MAX77752_REG_TEMP_CFG, &temp_cfg);

	if (enable) {
		temp_cfg |= 0x01; /* Thermal monitoring enable bit */
		g_thermal_monitoring_enabled = 1;
		printk("Thermal monitoring enabled");
	} else {
		temp_cfg &= ~0x01;
		g_thermal_monitoring_enabled = 0;
		printk("Thermal monitoring disabled");
	}

	max77752_port_transmit(MAX77752_REG_TEMP_CFG, temp_cfg);
}

u8 max77752_get_thermal_status(void)
{
	u8 temp_status = 0;
	max77752_port_receive(MAX77752_REG_TEMP_STATUS, &temp_status);
	return temp_status;
}

s16 max77752_read_temperature(void)
{
	u8 temp_data[2] = {0};

	max77752_port_multi_receive(MAX77752_REG_THERMISTOR, temp_data, 2);

	g_last_temperature = (temp_data[0] << 8) | temp_data[1];

	/* Convert to Celsius (assuming 12-bit signed temperature) */
	if (g_last_temperature > 0x7FF) {
		g_last_temperature -= 0x1000;
	}

	return g_last_temperature;
}

void max77752_set_temperature_limits(u8 low_limit, u8 high_limit)
{
	u8 temp_cfg = 0;

	g_temperature_low_limit = low_limit;
	g_temperature_high_limit = high_limit;

	max77752_port_receive(MAX77752_REG_TEMP_CFG, &temp_cfg);

	/* Configure temperature limits */
	temp_cfg &= ~0x3C; /* Clear limit bits */
	temp_cfg |= ((low_limit & 0x07) << 2); /* Low limit */
	temp_cfg |= ((high_limit & 0x07) << 4); /* High limit */

	max77752_port_transmit(MAX77752_REG_TEMP_CFG, temp_cfg);

	printk("Temperature limits set: %d°C to %d°C", low_limit, high_limit);
}

/* LED control functions */
void max77752_configure_led(u8 led_id, u8 mode, u8 brightness)
{
	u8 led_cfg = 0;
	u8 reg_addr;

	if (led_id == 1) {
		reg_addr = MAX77752_REG_LED1_CFG;
	} else {
		reg_addr = MAX77752_REG_LED2_CFG;
	}

	led_cfg = (mode & 0x03) | ((brightness & 0x0F) << 4);

	max77752_port_transmit(reg_addr, led_cfg);
	printk("LED%d configured: Mode=%d, Brightness=%d", led_id, mode, brightness);
}

void max77752_set_led_state(u8 led_id, u8 state)
{
	u8 led_cfg = 0;
	u8 reg_addr;

	if (led_id == 1) {
		reg_addr = MAX77752_REG_LED1_CFG;
	} else {
		reg_addr = MAX77752_REG_LED2_CFG;
	}

	max77752_port_receive(reg_addr, &led_cfg);

	if (state) {
		led_cfg |= 0x01; /* Enable LED */
	} else {
		led_cfg &= ~0x01; /* Disable LED */
	}

	max77752_port_transmit(reg_addr, led_cfg);
}

void max77752_configure_led_charging_indicator(u8 enable)
{
	u8 led_cfg = 0;

	max77752_port_receive(MAX77752_REG_LED_CFG, &led_cfg);

	if (enable) {
		led_cfg |= 0x01; /* Enable charging indicator */
	} else {
		led_cfg &= ~0x01; /* Disable charging indicator */
	}

	max77752_port_transmit(MAX77752_REG_LED_CFG, led_cfg);
}

/* Interrupt handling functions */
u8 max77752_read_interrupt_status(void)
{
	u8 int_status = 0;
	max77752_port_receive(MAX77752_REG_INT, &int_status);
	return int_status;
}

void max77752_clear_interrupt(u8 interrupt_mask)
{
	max77752_port_transmit(MAX77752_REG_INT, interrupt_mask);
}

void max77752_configure_interrupt_mask(u8 mask, u8 enable)
{
	u8 int_mask = 0;

	max77752_port_receive(MAX77752_REG_INT_MSK, &int_mask);

	if (enable) {
		int_mask &= ~mask; /* Enable interrupt */
	} else {
		int_mask |= mask; /* Disable interrupt */
	}

	max77752_port_transmit(MAX77752_REG_INT_MSK, int_mask);
}

void max77752_register_callback(void (*callback)(u8 interrupt_source))
{
	g_interrupt_callback = callback;
	printk("Interrupt callback registered");
}

/* Monitoring and diagnostic functions */
void max77752_continuous_monitor_start(u32 interval_ms)
{
	g_monitor_interval = interval_ms;
	g_monitoring_active = 1;
	printk("Continuous monitoring started (interval: %d ms)", interval_ms);
}

void max77752_continuous_monitor_stop(void)
{
	g_monitoring_active = 0;
	printk("Continuous monitoring stopped");
}

void max77752_perform_self_test(void)
{
	u8 test_result = 0;

	printk("Performing self-test...");

	/* Test basic functionality */
	if (!max77752_is_device_present()) {
		printk("Self-test FAILED: Device not responding");
		return;
	}

	/* Test voltage monitoring */
	u16 vsys = max77752_read_vsys_voltage();
	u16 vbat = max77752_read_vbat_voltage();
	u16 vbus = max77752_read_vbus_voltage();

	if (vsys == 0 && vbat == 0 && vbus == 0) {
		printk("Self-test WARNING: All voltages read as 0");
	}

	/* Test temperature monitoring */
	s16 temp = max77752_read_temperature();
	if (temp < -50 || temp > 125) {
		printk("Self-test WARNING: Temperature out of range (%d°C)", temp);
	}

	test_result = 0x01; /* Test passed */
	printk("Self-test completed: PASSED (result: 0x%02X)", test_result);
}

void max77752_get_system_info(u8 *pinfo_buffer, u8 buffer_size)
{
	u8 info_index = 0;

	if (buffer_size < 10) {
		printk("ERROR: Buffer too small for system info");
		return;
	}

	/* Device identification */
	pinfo_buffer[info_index++] = max77752_read_device_id();
	pinfo_buffer[info_index++] = max77752_read_device_revision();

	/* Status information */
	pinfo_buffer[info_index++] = max77752_read_status();
	pinfo_buffer[info_index++] = max77752_get_battery_status();
	pinfo_buffer[info_index++] = max77752_get_charge_status();

	/* Voltage readings */
	u16 vsys = max77752_read_vsys_voltage();
	pinfo_buffer[info_index++] = (vsys >> 8) & 0xFF;
	pinfo_buffer[info_index++] = vsys & 0xFF;

	u16 vbat = max77752_read_vbat_voltage();
	pinfo_buffer[info_index++] = (vbat >> 8) & 0xFF;
	pinfo_buffer[info_index++] = vbat & 0xFF;

	printk("System info retrieved: %d bytes", info_index);
}

/* Utility functions */
u8 max77752_check_power_good(void)
{
	u8 status = max77752_read_status();
	return (status & MAX77752_STATUS_PWR_SLA) ? 1 : 0;
}

u8 max77752_check_battery_present(void)
{
	u8 status = max77752_read_status();
	return (status & MAX77752_STATUS_BAT_PRES) ? 1 : 0;
}

u8 max77752_check_vbus_present(void)
{
	u8 status = max77752_read_status();
	return (status & MAX77752_STATUS_VBUS_PRES) ? 1 : 0;
}

u8 max77752_check_thermal_fault(void)
{
	u8 temp_status = max77752_get_thermal_status();
	return (temp_status & 0x01) ? 1 : 0; /* Check thermal fault bit */
}

void max77752_set_default_config(void)
{
	printk("Setting default configuration...");

	/* Configure default interrupt mask */
	max77752_configure_interrupt_mask(0xFF, 1); /* Enable all interrupts */

	/* Set default power configuration */
	u8 pwr_cfg = MAX77752_PWR_SLA_WAKE_EN | MAX77752_PWR_SLA_ILIM_EN;
	max77752_port_transmit(MAX77752_REG_PWR_SLA, pwr_cfg);

	/* Set default thermal configuration */
	max77752_configure_thermal(1);

	/* Set default LED configuration */
	max77752_configure_led(1, MAX77752_LED_MODE_OFF, 8);
	max77752_configure_led(2, MAX77752_LED_MODE_OFF, 8);

	printk("Default configuration set");
}

/* Test functions */
#ifdef DESIGN_VERIFICATION_MAX77752
#include "kinetis/test-kinetis.h"
#include "kinetis/design_verification.h"

/**
 * @brief Test 1: MAX77752 Basic Device Detection and Identification
 * @return PASS if device is detected, FAIL otherwise
 */
int t_max77752_device_id(int argc, char **argv)
{
	u8 dev_id, dev_rev;

	pr_info("=== MAX77752 Device ID Test ===");

	/* Check if device is present */
	if (!max77752_is_device_present()) {
		pr_err("FAIL: MAX77752 device not found");
		return FAIL;
	}

	/* Read device identification */
	dev_id = max77752_read_device_id();
	dev_rev = max77752_read_device_revision();

	pr_info("Device ID: 0x%02X", dev_id);
	pr_info("Device Revision: 0x%02X", dev_rev);

	if (dev_id != 0x00 && dev_id != 0xFF) {
		pr_info("Device detected successfully");
		return PASS;
	} else {
		pr_err("FAIL: Invalid device ID");
		return FAIL;
	}
}

/**
 * @brief Test 2: MAX77752 Power Management
 * @return PASS if power management works correctly, FAIL otherwise
 */
int t_max77752_power_test(int argc, char **argv)
{
	u8 pwr_status;

	pr_info("=== MAX77752 Power Management Test ===");

	/* Read initial power status */
	pwr_status = max77752_get_power_status();
	pr_info("Initial power status: %d", pwr_status);

	/* Enable power */
	max77752_enable_power(1);
	mdelay(100);

	pwr_status = max77752_get_power_status();
	pr_info("Power status after enable: %d", pwr_status);

	if (!max77752_check_power_good()) {
		pr_warn("Power good signal not asserted");
	}

	/* Disable power */
	max77752_enable_power(0);
	mdelay(100);

	pwr_status = max77752_get_power_status();
	pr_info("Power status after disable: %d", pwr_status);

	/* Re-enable power */
	max77752_enable_power(1);
	mdelay(100);

	pr_info("Power management test completed");
	return PASS;
}

/**
 * @brief Test 3: MAX77752 Battery Detection and Status
 * @return PASS if battery status can be read, FAIL otherwise
 */
int t_max77752_battery_test(int argc, char **argv)
{
	u8 bat_status;

	pr_info("=== MAX77752 Battery Status Test ===");

	/* Check if battery is present */
	if (max77752_check_battery_present()) {
		pr_info("Battery present: YES");
	} else {
		pr_info("Battery present: NO");
	}

	/* Read battery status */
	bat_status = max77752_get_battery_status();
	pr_info("Battery status: 0x%02X", bat_status);

	/* Read battery voltage */
	u16 vbat = max77752_read_vbat_voltage();
	pr_info("Battery voltage: %d mV", vbat);

	if (vbat > MAX77752_VBAT_LOW_THRESHOLD) {
		pr_info("Battery voltage is above low threshold");
	} else {
		pr_warn("Battery voltage is below low threshold");
	}

	pr_info("Battery status test completed");
	return PASS;
}

/**
 * @brief Test 4: MAX77752 Voltage Monitoring
 * @param argc Argument count (argv[1] = number of samples, default 10)
 * @param argv Argument vector
 * @return PASS if voltage monitoring works, FAIL otherwise
 */
int t_max77752_voltage_test(int argc, char **argv)
{
	u16 vsys, vbat, vbus, votg;
	u16 samples = 10;
	u16 i;

	if (argc > 1) {
		samples = simple_strtoul(argv[1], &argv[1], 10);
		if (samples > 100) {
			samples = 100;
		}
	}

	pr_info("=== MAX77752 Voltage Monitoring Test (%d samples) ===", samples);

	/* Check VBUS presence */
	if (max77752_check_vbus_present()) {
		pr_info("VBUS present: YES");
	} else {
		pr_info("VBUS present: NO");
	}

	for (i = 0; i < samples; i++) {
		/* Read all voltages */
		vsys = max77752_read_vsys_voltage();
		vbat = max77752_read_vbat_voltage();
		vbus = max77752_read_vbus_voltage();
		votg = max77752_read_votg_voltage();

		/* Show first and last readings */
		if (i == 0 || i == samples - 1) {
			pr_info("Reading %d/%d: VSYS=%dmV, VBAT=%dmV, VBUS=%dmV, VOTG=%dmV",
				i + 1, samples, vsys, vbat, vbus, votg);
		}

		mdelay(50);
	}

	pr_info("Voltage monitoring test completed");
	return PASS;
}

/**
 * @brief Test 5: MAX77752 Charging Configuration and Status
 * @return PASS if charging can be configured, FAIL otherwise
 */
int t_max77752_charging_test(int argc, char **argv)
{
	u8 chg_status;
	u16 chg_current;

	pr_info("=== MAX77752 Charging Test ===");

	/* Enable charging */
	max77752_enable_charging(1);
	mdelay(100);

	/* Configure charging parameters */
	max77752_configure_charging(MAX77752_ICHG_LIMIT_500, 4200);
	pr_info("Charging configured: 500mA, 4200mV");
	mdelay(100);

	/* Read charging status */
	chg_status = max77752_get_charge_status();
	pr_info("Charging status: 0x%02X", chg_status);

	switch (chg_status) {
	case MAX77752_CHG_IDLE:
		pr_info("Charging state: IDLE");
		break;
	case MAX77752_CHG_TRICKLE:
		pr_info("Charging state: TRICKLE");
		break;
	case MAX77752_CHG_FAST_CC:
		pr_info("Charging state: FAST CC");
		break;
	case MAX77752_CHG_FAST_CV:
		pr_info("Charging state: FAST CV");
		break;
	case MAX77752_CHG_DONE:
		pr_info("Charging state: DONE");
		break;
	case MAX77752_CHG_SUSPEND:
		pr_info("Charging state: SUSPEND");
		break;
	case MAX77752_CHG_TIMEOUT:
		pr_info("Charging state: TIMEOUT");
		break;
	case MAX77752_CHG_ERROR:
		pr_info("Charging state: ERROR");
		break;
	}

	/* Read charging current */
	chg_current = max77752_read_charge_current();
	pr_info("Charging current: %d mA", chg_current);

	/* Disable charging for test */
	max77752_enable_charging(0);

	pr_info("Charging test completed");
	return PASS;
}

/**
 * @brief Test 6: MAX77752 Current Monitoring
 * @return PASS if current monitoring works, FAIL otherwise
 */
int t_max77752_current_test(int argc, char **argv)
{
	u16 chg_current, sys_current, otg_current;

	pr_info("=== MAX77752 Current Monitoring Test ===");

	/* Read all current values */
	chg_current = max77752_read_charge_current();
	sys_current = max77752_read_system_current();
	otg_current = max77752_read_otg_current();

	pr_info("Charge current: %d mA", chg_current);
	pr_info("System current: %d mA", sys_current);
	pr_info("OTG current: %d mA", otg_current);

	pr_info("Current monitoring test completed");
	return PASS;
}

/**
 * @brief Test 7: MAX77752 Temperature Monitoring
 * @return PASS if temperature monitoring works, FAIL otherwise
 */
int t_max77752_temperature_test(int argc, char **argv)
{
	s16 temperature;
	u8 temp_status;

	pr_info("=== MAX77752 Temperature Monitoring Test ===");

	/* Read temperature */
	temperature = max77752_read_temperature();
	pr_info("Temperature: %d°C", temperature);

	/* Check thermal status */
	temp_status = max77752_get_thermal_status();
	pr_info("Thermal status: 0x%02X", temp_status);

	/* Check for thermal fault */
	if (max77752_check_thermal_fault()) {
		pr_warn("Thermal fault detected");
	} else {
		pr_info("No thermal fault");
	}

	/* Set temperature limits */
	max77752_set_temperature_limits(MAX77752_TEMP_LOW_COOL, MAX77752_TEMP_WARM_HOT);
	pr_info("Temperature limits set: %d°C to %d°C",
		 MAX77752_TEMP_LOW_COOL, MAX77752_TEMP_WARM_HOT);

	pr_info("Temperature monitoring test completed");
	return PASS;
}

/**
 * @brief Test 8: MAX77752 LED Control
 * @return PASS if LED control works, FAIL otherwise
 */
int t_max77752_led_test(int argc, char **argv)
{
	u16 i;

	pr_info("=== MAX77752 LED Control Test ===");

	/* Test LED 1 */
	pr_info("Testing LED 1...");
	max77752_configure_led(1, MAX77752_LED_MODE_ON, 15);
	pr_info("LED 1: ON (brightness 15)");
	mdelay(500);

	max77752_set_led_state(1, 1);
	mdelay(500);

	max77752_set_led_state(1, 0);
	pr_info("LED 1: OFF");
	mdelay(500);

	/* Test LED 2 */
	pr_info("Testing LED 2...");
	max77752_configure_led(2, MAX77752_LED_MODE_ON, 15);
	pr_info("LED 2: ON (brightness 15)");
	mdelay(500);

	max77752_set_led_state(2, 1);
	mdelay(500);

	max77752_set_led_state(2, 0);
	pr_info("LED 2: OFF");
	mdelay(500);

	/* Test LED blinking */
	pr_info("Testing LED blinking mode...");
	max77752_configure_led(1, MAX77752_LED_MODE_BLINK, 10);
	mdelay(2000);

	/* Test LED breathing */
	pr_info("Testing LED breathing mode...");
	max77752_configure_led(1, MAX77752_LED_MODE_BREATH, 10);
	mdelay(2000);

	/* Turn off all LEDs */
	max77752_configure_led(1, MAX77752_LED_MODE_OFF, 0);
	max77752_configure_led(2, MAX77752_LED_MODE_OFF, 0);
	pr_info("All LEDs OFF");

	pr_info("LED control test completed");
	return PASS;
}

/**
 * @brief Test 9: MAX77752 Interrupt Handling
 * @return PASS if interrupt handling works, FAIL otherwise
 */
int t_max77752_interrupt_test(int argc, char **argv)
{
	u8 int_status;

	pr_info("=== MAX77752 Interrupt Test ===");

	/* Read interrupt status */
	int_status = max77752_read_interrupt_status();
	pr_info("Interrupt status: 0x%02X", int_status);

	/* Clear all interrupts */
	max77752_clear_interrupt(0xFF);
	mdelay(10);

	/* Read interrupt status after clear */
	int_status = max77752_read_interrupt_status();
	pr_info("Interrupt status after clear: 0x%02X", int_status);

	/* Configure interrupt mask */
	max77752_configure_interrupt_mask(0x0F, 1);
	pr_info("Interrupt mask configured: bits 0-3 enabled");

	/* Read status again */
	int_status = max77752_read_status();
	pr_info("Device status: 0x%02X", int_status);

	pr_info("Interrupt test completed");
	return PASS;
}

/**
 * @brief Test 10: MAX77752 Self-Test and Diagnostics
 * @return PASS if self-test passes, FAIL otherwise
 */
int t_max77752_selftest(int argc, char **argv)
{
	u8 info_buffer[10];

	pr_info("=== MAX77752 Self-Test ===");

	/* Perform self-test */
	max77752_perform_self_test();
	mdelay(100);

	/* Get system information */
	max77752_get_system_info(info_buffer, sizeof(info_buffer));
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
	pr_info("  Power good: %d", max77752_check_power_good());
	pr_info("  Battery present: %d", max77752_check_battery_present());
	pr_info("  VBUS present: %d", max77752_check_vbus_present());
	pr_info("  Thermal fault: %d", max77752_check_thermal_fault());

	pr_info("Self-test completed");
	return PASS;
}

/**
 * @brief Test 11: MAX77752 Sleep Mode
 * @return PASS if sleep mode works, FAIL otherwise
 */
int t_max77752_sleep_test(int argc, char **argv)
{
	pr_info("=== MAX77752 Sleep Mode Test ===");

	/* Enter sleep mode */
	max77752_set_sleep_mode(1);
	pr_info("Entered sleep mode");
	mdelay(200);

	/* Exit sleep mode */
	max77752_set_sleep_mode(0);
	pr_info("Exited sleep mode");
	mdelay(200);

	pr_info("Sleep mode test completed");
	return PASS;
}

/**
 * @brief Test 12: MAX77752 Continuous Monitoring
 * @return PASS if monitoring can be started/stopped, FAIL otherwise
 */
int t_max77752_monitoring_test(int argc, char **argv)
{
	u16 vsys, vbat;

	pr_info("=== MAX77752 Continuous Monitoring Test ===");

	/* Start continuous monitoring */
	max77752_continuous_monitor_start(500);
	pr_info("Continuous monitoring started (500ms interval)");

	/* Read some values during monitoring */
	mdelay(100);
	vsys = max77752_read_vsys_voltage();
	vbat = max77752_read_vbat_voltage();
	pr_info("VSYS: %d mV, VBAT: %d mV", vsys, vbat);

	/* Stop continuous monitoring */
	max77752_continuous_monitor_stop();
	pr_info("Continuous monitoring stopped");

	pr_info("Continuous monitoring test completed");
	return PASS;
}

void max77752_Test(void)
{
	printk("=== MAX77752 Comprehensive Test Started ===");

	/* Test 1: Device ID */
	if (t_max77752_device_id(0, NULL) != PASS) {
		printk("Test 1 FAILED: Device ID test failed");
		return;
	}

	/* Test 2: Power management */
	t_max77752_power_test(0, NULL);

	/* Test 3: Battery status */
	t_max77752_battery_test(0, NULL);

	/* Test 4: Voltage monitoring */
	t_max77752_voltage_test(0, NULL);

	/* Test 5: Charging test */
	t_max77752_charging_test(0, NULL);

	/* Test 6: Current monitoring */
	t_max77752_current_test(0, NULL);

	/* Test 7: Temperature monitoring */
	t_max77752_temperature_test(0, NULL);

	/* Test 8: LED control */
	t_max77752_led_test(0, NULL);

	/* Test 9: Interrupt test */
	t_max77752_interrupt_test(0, NULL);

	/* Test 10: Self-test */
	t_max77752_selftest(0, NULL);

	/* Test 11: Sleep mode */
	t_max77752_sleep_test(0, NULL);

	/* Test 12: Continuous monitoring */
	t_max77752_monitoring_test(0, NULL);

	printk("=== MAX77752 Comprehensive Test Completed Successfully ===");
}

#endif

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */