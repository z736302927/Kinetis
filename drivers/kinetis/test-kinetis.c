
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errname.h>

#include "kinetis/design_verification.h"
#include "kinetis/test-kinetis.h"
#include "kinetis/user-shell.h"
#include "kinetis/rtc-task.h"
#include "kinetis/tim-task.h"
#include "kinetis/basic-timer.h"
#include "kinetis/button.h"
// #include "kinetis/switch.h"
#include "kinetis/fatfs.h"
#include "kinetis/iic_soft.h"
#include "kinetis/ak8975.h"
#include "kinetis/switch.h"
#include "kinetis/mpu6050.h"
#include "kinetis/max77752.h"
#include "kinetis/max30205.h"
#include "kinetis/spi_soft.h"

#include "w25qxxx.h"
#include "hydrology.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#ifdef DESIGN_VERIFICATION_AK8975
int t_ak8975_initialize(int argc, char **argv);
int t_ak8975_basic_info(int argc, char **argv);
int t_ak8975_magnetic(int argc, char **argv);
int t_ak8975_selftest(int argc, char **argv);
int t_ak8975_fuse_rom_access(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MPU6050
int t_mpu6050_device_id(int argc, char **argv);
int t_mpu6050_sensor_data(int argc, char **argv);
int t_mpu6050_selftest(int argc, char **argv);
int t_mpu6050_gyro_calibration(int argc, char **argv);
int t_mpu6050_accel_calibration(int argc, char **argv);
int t_mpu6050_fifo_test(int argc, char **argv);
int t_mpu6050_interrupt_test(int argc, char **argv);
int t_mpu6050_power_test(int argc, char **argv);
int t_mpu6050_fullscale_test(int argc, char **argv);
int t_mpu6050_dlpf_test(int argc, char **argv);
int t_mpu6050_initialize(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MAX77752
int t_max77752_device_id(int argc, char **argv);
int t_max77752_power_test(int argc, char **argv);
int t_max77752_battery_test(int argc, char **argv);
int t_max77752_voltage_test(int argc, char **argv);
int t_max77752_charging_test(int argc, char **argv);
int t_max77752_current_test(int argc, char **argv);
int t_max77752_temperature_test(int argc, char **argv);
int t_max77752_led_test(int argc, char **argv);
int t_max77752_interrupt_test(int argc, char **argv);
int t_max77752_selftest(int argc, char **argv);
int t_max77752_sleep_test(int argc, char **argv);
int t_max77752_monitoring_test(int argc, char **argv);
int t_max77752_initialize(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MAX30205
int t_max30205_initialize(int argc, char **argv);
int t_max30205_device_id(int argc, char **argv);
int t_max30205_temperature_single(int argc, char **argv);
int t_max30205_temperature_continuous(int argc, char **argv);
int t_max30205_threshold_test(int argc, char **argv);
int t_max30205_config_test(int argc, char **argv);
int t_max30205_os_flag_test(int argc, char **argv);
int t_max30205_timeout_test(int argc, char **argv);
int t_max30205_oneshot_test(int argc, char **argv);
int t_max30205_calibration_test(int argc, char **argv);
int t_max30205_range_test(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_AT24CXX
int t_at24cxx_loopback(int argc, char **argv);
int t_at24cxx_current_addr_read(int argc, char **argv);
int t_at24cxx_current_random_read(int argc, char **argv);
int t_at24cxx_sequential_read(int argc, char **argv);
int t_at24cxx_loopback_speed(int argc, char **argv);
int t_at24cxx_program_thread(int argc, char **argv);
int t_at24cxx_device_detect(int argc, char **argv);
int t_at24cxx_edge_cases(int argc, char **argv);
int t_at24cxx_wear_leveling_info(int argc, char **argv);
int t_at24cxx_write_protection_check(int argc, char **argv);
int t_at24cxx_write_with_retry(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_DS3231
int t_ds3231_set_clock(int argc, char **argv);
int t_ds3231_get_clock(int argc, char **argv);
int t_ds3231_set_alarm1(int argc, char **argv);
int t_ds3231_set_alarm2(int argc, char **argv);
int t_ds3231_square_wave(int argc, char **argv);
int t_ds3231_32khz_wave(int argc, char **argv);
int t_ds3231_get_temprature(int argc, char **argv);
int t_ds3231_comprehensive_test(int argc, char **argv);
int t_ds3231_initialize(int argc, char **argv);
int t_ds3231_stress_test(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_BASICTIMER
int t_basic_timer_get_tick(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_CRC
int t_crc(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_BUTTON
int t_button_task(int argc, char **argv);
int t_button_add(int argc, char **argv);
int t_button_drop(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_DELAY
int t_delay(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_FATFS
int t_fatfs_operate(int argc, char **argv);
int t_fatfs_loopback(int argc, char **argv);
int t_fatfs_miscellaneous(int argc, char **argv);
int t_fatfs_file_check(int argc, char **argv);
int t_fatfs_scan_files(int argc, char **argv);
int t_fatfs_append(int argc, char **argv);
int t_fatfs_delete_node(int argc, char **argv);
int t_fatfs_expand(int argc, char **argv);
int t_fatfs_diskio(int argc, char **argv);
int t_fatfs_contiguous_file(int argc, char **argv);
int t_fatfs_raw_speed(int argc, char **argv);
#if !FF_FS_READONLY
int t_fatfs_truncate(int argc, char **argv);
int t_fatfs_sync(int argc, char **argv);
#endif
#if FF_USE_CHMOD
int t_fatfs_chmod(int argc, char **argv);
int t_fatfs_utime(int argc, char **argv);
#endif
#if FF_FS_RPATH >= 2
int t_fatfs_chdir(int argc, char **argv);
#endif
#if FF_USE_FIND
int t_fatfs_find(int argc, char **argv);
#endif
#if FF_USE_LABEL
int t_fatfs_label(int argc, char **argv);
#endif
#if FF_USE_STRFUNC
int t_fatfs_char_io(int argc, char **argv);
int t_fatfs_gets(int argc, char **argv);
#endif
int t_fatfs_file_status(int argc, char **argv);
int t_fatfs_unmount(int argc, char **argv);
#if FF_USE_FORWARD
int t_fatfs_forward(int argc, char **argv);
#endif
#if FF_CODE_PAGE == 0
int t_fatfs_setcp(int argc, char **argv);
#endif
#if FF_MULTI_PARTITION
int t_fatfs_disk_partition(int argc, char **argv);
#endif
int t_fatfs_speed(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_FSM
int t_fsm_example(int argc, char **argv);
int t_fsm_basic(int argc, char **argv);
int t_fsm_state_transitions(int argc, char **argv);
int t_fsm_error_handling(int argc, char **argv);
int t_fsm_performance(int argc, char **argv);
int t_fsm_validation(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_GENERAL
int t_general_success(int argc, char **argv);
int t_general_error(int argc, char **argv);
int t_general_timeout(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_HC_05
int t_hc_05_basic_init(int argc, char **argv);
int t_hc_05_info_query(int argc, char **argv);
int t_hc_05_cleanup(int argc, char **argv);
int t_hc_05_config_basic(int argc, char **argv);
int t_hc_05_config_role(int argc, char **argv);
int t_hc_05_config_advanced(int argc, char **argv);
int t_hc_05_data_comm(int argc, char **argv);
int t_hc_05_connection(int argc, char **argv);
int t_hc_05_pairing(int argc, char **argv);
int t_hc_05_performance(int argc, char **argv);
int t_hc_05_comprehensive(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_HYDROLOGY
int t_hydrology(int argc, char **argv);
int t_hydrology_init(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_LED
int t_led_add(int argc, char **argv);
int t_led_drop(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RNG
int t_random_number(int argc, char **argv);
int t_random_range(int argc, char **argv);
int t_random_array(int argc, char **argv);
int t_random_statistics(int argc, char **argv);
int t_random_uniqueness(int argc, char **argv);
int t_random_bits(int argc, char **argv);
int t_random_array_multi(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RS485

#endif

#ifdef DESIGN_VERIFICATION_RTC
int t_rtc_set_clock(int argc, char **argv);
int t_rtc_get_clock(int argc, char **argv);
int t_rtc_validation(int argc, char **argv);
int t_rtc_backup(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RTCTASK
int t_rtc_task_add(int argc, char **argv);
int t_rtc_task_drop(int argc, char **argv);
int t_rtc_task_validation(int argc, char **argv);
int t_rtc_task_priority(int argc, char **argv);
int t_rtc_task_performance(int argc, char **argv);
int t_rtc_task_cleanup(int argc, char **argv);
int t_rtc_task_short_interval(int argc, char **argv);
int t_rtc_task_boundary(int argc, char **argv);
int t_rtc_task_suspend_resume(int argc, char **argv);
int t_rtc_task_concurrent(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_SEIRALPORT
int t_serial_port_interactive(int argc, char **argv);
int t_serial_port_basic(int argc, char **argv);
int t_serial_port_boundary(int argc, char **argv);
int t_serial_port_performance(int argc, char **argv);
int t_serial_port_stress(int argc, char **argv);
int t_serial_port_timeout(int argc, char **argv);
int t_serial_port_null_checks(int argc, char **argv);
int t_serial_port_zero_timeout(int argc, char **argv);
int t_serial_port_ring_buffer(int argc, char **argv);
int t_serial_port_error_injection(int argc, char **argv);
int t_serial_port_special_characters(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_SHELL

#endif

#ifdef DESIGN_VERIFICATION_SLIST

#endif

#ifdef DESIGN_VERIFICATION_SWITCH
int t_switch_add(int argc, char **argv);
int t_switch_drop(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_TIMTASK
int t_tim_task_add(int argc, char **argv);
int t_tim_task_drop(int argc, char **argv);
int t_tim_task_validation(int argc, char **argv);
int t_tim_task_priority(int argc, char **argv);
int t_tim_task_performance(int argc, char **argv);
int t_tim_task_cleanup(int argc, char **argv);
int t_tim_task_boundary(int argc, char **argv);
int t_tim_task_concurrent(int argc, char **argv);
int t_tim_task_short_interval(int argc, char **argv);
int t_tim_task_suspend_resume(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MY9221
int t_my9221_send_packet(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_TLC5971
int t_tlc5971_send_packet(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_W25QXXX
int t_w25qxxx_device_init(int argc, char **argv);
int t_w25qxxx_read_info(int argc, char **argv);
int t_w25qxxx_loopback(int argc, char **argv);
int t_w25qxxx_chip_erase(int argc, char **argv);
int t_w25qxxx_sector_erase(int argc, char **argv);
int t_w25qxxx_block_erase_32kb(int argc, char **argv);
int t_w25qxxx_block_erase_64kb(int argc, char **argv);
int t_w25qxxx_page_program(int argc, char **argv);
int t_w25qxxx_status_register(int argc, char **argv);
int t_w25qxxx_power_test(int argc, char **argv);
int t_w25qxxx_reset_test(int argc, char **argv);
int t_w25qxxx_fast_read(int argc, char **argv);
int t_w25qxxx_block_protection(int argc, char **argv);
int t_w25qxxx_suspend_resume(int argc, char **argv);
int t_w25qxxx_multi_sector_program(int argc, char **argv);
int t_w25qxxx_comprehensive(int argc, char **argv);
int t_w25qxxx_boundary(int argc, char **argv);
int t_w25qxxx_performance(int argc, char **argv);
int t_w25qxxx_stress(int argc, char **argv);
int t_w25qxxx_address_mode(int argc, char **argv);
int t_w25qxxx_write_protect(int argc, char **argv);
int t_w25qxxx_security_registers(int argc, char **argv);
int t_w25qxxx_large_transfer(int argc, char **argv);
int t_w25qxxx_unique_id(int argc, char **argv);
int t_w25qxxx_sfdp(int argc, char **argv);
#endif
#ifdef DESIGN_VERIFICATION_SPI
int t_spi_system_init(int argc, char **argv);
int t_spi_system_exit(int argc, char **argv);
int t_spi_loopback(int argc, char **argv);
int t_spi_set_mode(int argc, char **argv);
int t_spi_edge_cases(int argc, char **argv);
int t_spi_performance(int argc, char **argv);
int t_spi_stress(int argc, char **argv);
int t_spi_read_write_reg(int argc, char **argv);
int t_spi_boundary_large(int argc, char **argv);
#endif
#ifdef DESIGN_VERIFICATION_IIC
int t_iic_slave_basic(int argc, char **argv);
int t_iic_transfer_byte(int argc, char **argv);
int t_iic_transfer_bytes(int argc, char **argv);
int t_iic_edge_cases(int argc, char **argv);
int t_iic_performance(int argc, char **argv);
int t_iic_stress(int argc, char **argv);
int t_iic_read_write_reg(int argc, char **argv);
int t_iic_boundary_large(int argc, char **argv);
int t_iic_address_modes(int argc, char **argv);
int t_iic_start_stop(int argc, char **argv);
#endif

static int t_function(int argc, char **argv)
{
	pr_err("ToDo, please implement this function.");
	return FAIL;
}

struct test_case_typedef {
	char *command;
	int (*function)(int argc, char **argv);
};

struct test_case_typedef kinetis_case_table[] = {

#ifdef DESIGN_VERIFICATION_AK8975
	{"ak8975.init",               t_ak8975_initialize},
	{"ak8975.basic-info",           t_ak8975_basic_info},
	{"ak8975.magnetic",             t_ak8975_magnetic},
	{"ak8975.selftest",             t_ak8975_selftest},
	{"ak8975.fuse-rom-access",      t_ak8975_fuse_rom_access},
#endif
#ifdef DESIGN_VERIFICATION_MPU6050
	{"mpu6050.init",               t_mpu6050_initialize},
	{"mpu6050.sensor-data",          t_mpu6050_sensor_data},
	{"mpu6050.selftest",            t_mpu6050_selftest},
	{"mpu6050.gyro-calibration",     t_mpu6050_gyro_calibration},
	{"mpu6050.accel-calibration",    t_mpu6050_accel_calibration},
	{"mpu6050.fifo-test",            t_mpu6050_fifo_test},
	{"mpu6050.interrupt-test",      t_mpu6050_interrupt_test},
	{"mpu6050.power-test",          t_mpu6050_power_test},
	{"mpu6050.fullscale-test",       t_mpu6050_fullscale_test},
	{"mpu6050.dlpf-test",            t_mpu6050_dlpf_test},
#endif
#ifdef DESIGN_VERIFICATION_MAX77752
	{"max77752.init",             t_max77752_initialize},
	{"max77752.power-test",         t_max77752_power_test},
	{"max77752.battery-test",       t_max77752_battery_test},
	{"max77752.voltage-test",       t_max77752_voltage_test},
	{"max77752.charging-test",      t_max77752_charging_test},
	{"max77752.current-test",       t_max77752_current_test},
	{"max77752.temperature-test",   t_max77752_temperature_test},
	{"max77752.led-test",           t_max77752_led_test},
	{"max77752.interrupt-test",     t_max77752_interrupt_test},
	{"max77752.selftest",           t_max77752_selftest},
	{"max77752.sleep-test",         t_max77752_sleep_test},
	{"max77752.monitoring-test",    t_max77752_monitoring_test},
#endif
#ifdef DESIGN_VERIFICATION_MAX30205
	{"max30205.init",             t_max30205_initialize},
	{"max30205.temp-single",        t_max30205_temperature_single},
	{"max30205.temp-continuous",    t_max30205_temperature_continuous},
	{"max30205.threshold-test",     t_max30205_threshold_test},
	{"max30205.config-test",        t_max30205_config_test},
	{"max30205.os-flag-test",       t_max30205_os_flag_test},
	{"max30205.timeout-test",       t_max30205_timeout_test},
	{"max30205.oneshot-test",       t_max30205_oneshot_test},
	{"max30205.calibration-test",   t_max30205_calibration_test},
	{"max30205.range-test",         t_max30205_range_test},
#endif
#ifdef DESIGN_VERIFICATION_AT24CXX
	{"at24cxx.init",            	t_at24cxx_program_thread},
	{"at24cxx.loopback",            t_at24cxx_loopback},
	{"at24cxx.current-addr-read",   t_at24cxx_current_addr_read},
	{"at24cxx.random-read",         t_at24cxx_current_random_read},
	{"at24cxx.seq-read",            t_at24cxx_sequential_read},
	{"at24cxx.lb-speed",            t_at24cxx_loopback_speed},
	{"at24cxx.device-detect",      t_at24cxx_device_detect},
	{"at24cxx.edge-cases",         t_at24cxx_edge_cases},
	{"at24cxx.wear-leveling",      t_at24cxx_wear_leveling_info},
	{"at24cxx.write-protect",      t_at24cxx_write_protection_check},
	{"at24cxx.write-retry",        t_at24cxx_write_with_retry},
#endif
#ifdef DESIGN_VERIFICATION_BMI160
	{"bmi160.", t_function},
#endif
#ifdef DESIGN_VERIFICATION_DS3231
	{"ds3231.init",               t_ds3231_initialize},
	{"ds3231.set-clock",            t_ds3231_set_clock},
	{"ds3231.get-clock",            t_ds3231_get_clock},
	{"ds3231.set-alarm1",           t_ds3231_set_alarm1},
	{"ds3231.set-alarm2",           t_ds3231_set_alarm2},
	{"ds3231.square-wave",          t_ds3231_square_wave},
	{"ds3231.32khz-wave",           t_ds3231_32khz_wave},
	{"ds3231.get-temprature",       t_ds3231_get_temprature},
	{"ds3231.comprehensive",        t_ds3231_comprehensive_test},
	{"ds3231.stress",               t_ds3231_stress_test},
#endif
#ifdef DESIGN_VERIFICATION_ESP32
	{"esp32.", t_function},
#endif
#ifdef DESIGN_VERIFICATION_GPRS
	{"gprs.", t_function},
#endif
#ifdef DESIGN_VERIFICATION_GSM
	{"gsm.", t_function},
#endif
#ifdef DESIGN_VERIFICATION_GT9271
	{"gt9271.",                     t_function},
#endif
#ifdef DESIGN_VERIFICATION_HC_05
	{"hc-05.basic-init",           t_hc_05_basic_init},
	{"hc-05.info-query",           t_hc_05_info_query},
	{"hc-05.cleanup",              t_hc_05_cleanup},
	{"hc-05.config-basic",         t_hc_05_config_basic},
	{"hc-05.config-role",          t_hc_05_config_role},
	{"hc-05.config-advanced",      t_hc_05_config_advanced},
	{"hc-05.data-comm",            t_hc_05_data_comm},
	{"hc-05.connection",           t_hc_05_connection},
	{"hc-05.pairing",              t_hc_05_pairing},
	{"hc-05.performance",          t_hc_05_performance},
	{"hc-05.comprehensive",         t_hc_05_comprehensive},
#endif
#ifdef DESIGN_VERIFICATION_IIC
	{"iic.slave-basic",             t_iic_slave_basic},
	{"iic.transfer-byte",           t_iic_transfer_byte},
	{"iic.transfer-bytes",          t_iic_transfer_bytes},
	{"iic.edge-cases",             	t_iic_edge_cases},
	{"iic.performance",             t_iic_performance},
	{"iic.stress",                 	t_iic_stress},
	{"iic.read-write-reg",        	t_iic_read_write_reg},
	{"iic.boundary-large",         	t_iic_boundary_large},
	{"iic.address-modes",          	t_iic_address_modes},
	{"iic.start-stop",             	t_iic_start_stop},
#endif
#ifdef DESIGN_VERIFICATION_HMC5883L
	{"hmc5883l.", 					t_function},
#endif
#ifdef DESIGN_VERIFICATION_HYDROLOGY
	{"hydrology.init",              t_hydrology_init},
	{"hydrology.test",              t_hydrology},
#endif
#ifdef DESIGN_VERIFICATION_ICM20602
	{"icm20602.",                   t_function},
#endif
#ifdef DESIGN_VERIFICATION_IS25LPWP256D
	{"is25lpwp256d.", t_function},
#endif
#ifdef DESIGN_VERIFICATION_BASICTIMER
	{"basic-timer.get-tick",        t_basic_timer_get_tick},
#endif
#ifdef DESIGN_VERIFICATION_CHINESE
	{"chinese.", 					t_function},
#endif
#ifdef DESIGN_VERIFICATION_CRC
	{"crc.test",                    t_crc},
#endif
#ifdef DESIGN_VERIFICATION_DELAY
	{"delay.test",                  t_delay},
#endif
#ifdef DESIGN_VERIFICATION_FATFS
	{"fatfs.operate",               t_fatfs_operate},
	{"fatfs.loopback",              t_fatfs_loopback},
	{"fatfs.miscellaneous",         t_fatfs_miscellaneous},
	{"fatfs.file-check",            t_fatfs_file_check},
	{"fatfs.scan-files",            t_fatfs_scan_files},
	{"fatfs.append",                t_fatfs_append},
	{"fatfs.delete-node",           t_fatfs_delete_node},
	{"fatfs.expand",                t_fatfs_expand},
	{"fatfs.diskio",                t_fatfs_diskio},
	{"fatfs.contiguous-file",       t_fatfs_contiguous_file},
	{"fatfs.raw-speed",             t_fatfs_raw_speed},
#if !FF_FS_READONLY
	{"fatfs.truncate",              t_fatfs_truncate},
	{"fatfs.sync",                  t_fatfs_sync},
#endif
#if FF_USE_CHMOD
	{"fatfs.chmod",                 t_fatfs_chmod},
	{"fatfs.utime",                 t_fatfs_utime},
#endif
#if FF_FS_RPATH >= 2
	{"fatfs.chdir",                 t_fatfs_chdir},
#endif
#if FF_USE_FIND
	{"fatfs.find",                  t_fatfs_find},
#endif
#if FF_USE_LABEL
	{"fatfs.label",                 t_fatfs_label},
#endif
#if FF_USE_STRFUNC
	{"fatfs.char-io",               t_fatfs_char_io},
	{"fatfs.gets",                  t_fatfs_gets},
#endif
	{"fatfs.file-status",           t_fatfs_file_status},
	{"fatfs.unmount",               t_fatfs_unmount},
#if FF_USE_FORWARD
	{"fatfs.forward",               t_fatfs_forward},
#endif
#if FF_CODE_PAGE == 0
	{"fatfs.setcp",                 t_fatfs_setcp},
#endif
#if FF_MULTI_PARTITION
	{"fatfs.disk-partition",        t_fatfs_disk_partition},
#endif
	{"fatfs.speed",                 t_fatfs_speed},
#endif
#ifdef DESIGN_VERIFICATION_FSM
	{"fsm.example",                 t_fsm_example},
	{"fsm.basic",                   t_fsm_basic},
	{"fsm.state-transitions",       t_fsm_state_transitions},
	{"fsm.error-handling",          t_fsm_error_handling},
	{"fsm.performance",             t_fsm_performance},
	{"fsm.validation",              t_fsm_validation},
#endif
#ifdef DESIGN_VERIFICATION_GENERAL
//	{"general.success",             t_general_success},
//	{"general.error",               t_general_error},
//	{"general.timeout",             t_general_timeout},
#endif
#ifdef DESIGN_VERIFICATION_BUTTON
	{"button.task",                 t_button_task},
	{"button.add",                  t_button_add},
	{"button.drop",                 t_button_drop},
#endif
#ifdef DESIGN_VERIFICATION_LCD
	{"test", t_function},
#endif
#ifdef DESIGN_VERIFICATION_LED
	{"led.add",                     t_led_add},
	{"led.drop",                    t_led_drop},
#endif
#ifdef DESIGN_VERIFICATION_RNG
	{"random.number",               t_random_number},
	{"random.range",                t_random_range},
	{"random.array",                t_random_array},
	{"random.statistics",           t_random_statistics},
	{"random.uniqueness",           t_random_uniqueness},
	{"random.bits",                 t_random_bits},
	{"random.array-multi",          t_random_array_multi},
#endif
#ifdef DESIGN_VERIFICATION_RS485
	{"test", t_function},
#endif
#ifdef DESIGN_VERIFICATION_RTC
	{"rtc.set-clock",               t_rtc_set_clock},
	{"rtc.get-clock",               t_rtc_get_clock},
	{"rtc.validation",              t_rtc_validation},
	{"rtc.backup",                 	t_rtc_backup},
#endif
#ifdef DESIGN_VERIFICATION_RTCTASK
	{"rtc-task.add",                t_rtc_task_add},
	{"rtc-task.drop",               t_rtc_task_drop},
	{"rtc-task.validation",         t_rtc_task_validation},
	{"rtc-task.priority",           t_rtc_task_priority},
	{"rtc-task.performance",        t_rtc_task_performance},
	{"rtc-task.cleanup",            t_rtc_task_cleanup},
	{"rtc-task.short-interval",      t_rtc_task_short_interval},
	{"rtc-task.boundary",           t_rtc_task_boundary},
	{"rtc-task.suspend-resume",     t_rtc_task_suspend_resume},
	{"rtc-task.concurrent",         t_rtc_task_concurrent},
#endif
#ifdef DESIGN_VERIFICATION_SEIRALPORT
	{"serial-port.interactive",     t_serial_port_interactive},
	{"serial-port.basic",           t_serial_port_basic},
	{"serial-port.boundary",        t_serial_port_boundary},
	{"serial-port.performance",     t_serial_port_performance},
	{"serial-port.stress",          t_serial_port_stress},
	{"serial-port.timeout",         t_serial_port_timeout},
	{"serial-port.null-checks",     t_serial_port_null_checks},
	{"serial-port.zero-timeout",    t_serial_port_zero_timeout},
	{"serial-port.ring-buffer",     t_serial_port_ring_buffer},
	{"serial-port.error-injection", t_serial_port_error_injection},
	{"serial-port.special-characters", t_serial_port_special_characters},
#endif
#ifdef DESIGN_VERIFICATION_SHELL
	{"test", t_function},
#endif
#ifdef DESIGN_VERIFICATION_SLIST
	{"test", t_function},
#endif
#ifdef DESIGN_VERIFICATION_SWITCH
	{"switch.add",                  t_switch_add},
	{"switch.drop",                 t_switch_drop},
#endif
#ifdef DESIGN_VERIFICATION_TIMTASK
	{"tim-task.add",                t_tim_task_add},
	{"tim-task.drop",               t_tim_task_drop},
	{"tim-task.validation",         t_tim_task_validation},
	{"tim-task.priority",           t_tim_task_priority},
	{"tim-task.performance",        t_tim_task_performance},
	{"tim-task.cleanup",            t_tim_task_cleanup},
	{"tim-task.boundary",           t_tim_task_boundary},
	{"tim-task.concurrent",         t_tim_task_concurrent},
	{"tim-task.short-interval",     t_tim_task_short_interval},
	{"tim-task.suspend-resume",     t_tim_task_suspend_resume},
#endif
#ifdef DESIGN_VERIFICATION_TOUCHSCREEN
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_MY9221
	{"my9221.send",                 t_my9221_send_packet},
#endif
#ifdef DESIGN_VERIFICATION_NBIOT
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_OLED
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_RAK477
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_SHT20
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_SPL06
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_SSD1306
	{"test",                        t_function},
#endif
#ifdef DESIGN_VERIFICATION_TLC5971
	{"tlc5971.send",                t_tlc5971_send_packet},
#endif
#ifdef DESIGN_VERIFICATION_W25QXXX
	{"w25qxxx.device-init",         t_w25qxxx_device_init},
	{"w25qxxx.read-info",           t_w25qxxx_read_info},
	{"w25qxxx.loopback",            t_w25qxxx_loopback},
	{"w25qxxx.erase",               t_w25qxxx_chip_erase},
	{"w25qxxx.sector-erase",        t_w25qxxx_sector_erase},
	{"w25qxxx.block-erase-32kb",    t_w25qxxx_block_erase_32kb},
	{"w25qxxx.block-erase-64kb",    t_w25qxxx_block_erase_64kb},
	{"w25qxxx.page-program",        t_w25qxxx_page_program},
	{"w25qxxx.status-register",     t_w25qxxx_status_register},
	{"w25qxxx.power-test",          t_w25qxxx_power_test},
	{"w25qxxx.reset-test",          t_w25qxxx_reset_test},
	{"w25qxxx.fast-read",           t_w25qxxx_fast_read},
	{"w25qxxx.block-protection",    t_w25qxxx_block_protection},
	{"w25qxxx.suspend-resume",      t_w25qxxx_suspend_resume},
	{"w25qxxx.multi-sector-program",t_w25qxxx_multi_sector_program},
	{"w25qxxx.comprehensive",       t_w25qxxx_comprehensive},
	{"w25qxxx.boundary",            t_w25qxxx_boundary},
	{"w25qxxx.performance",         t_w25qxxx_performance},
	{"w25qxxx.stress",              t_w25qxxx_stress},
	{"w25qxxx.address-mode",        t_w25qxxx_address_mode},
	{"w25qxxx.write-protect",       t_w25qxxx_write_protect},
	{"w25qxxx.security-registers",  t_w25qxxx_security_registers},
	{"w25qxxx.large-transfer",      t_w25qxxx_large_transfer},
	{"w25qxxx.unique-id",           t_w25qxxx_unique_id},
	{"w25qxxx.sfdp",                t_w25qxxx_sfdp},
#endif
#ifdef DESIGN_VERIFICATION_SPI
	{"spi.init",                    t_spi_system_init},
	{"spi.exit",                   	t_spi_system_exit},
	{"spi.loopback",                t_spi_loopback},
	{"spi.mode",                   	t_spi_set_mode},
	{"spi.edge-cases",              t_spi_edge_cases},
	{"spi.performance",             t_spi_performance},
	{"spi.stress",                  t_spi_stress},
	{"spi.read-write-reg",          t_spi_read_write_reg},
	{"spi.boundary-large",          t_spi_boundary_large},
#endif
#ifdef DESIGN_VERIFICATION_XMODEM
	{"test",                        t_function},
#endif
};

static int idle_task_init(void)
{
	int ret;

	ret = fatfs_init();
	if (ret) {
		goto err;
	}

	shell_init_async();

	// 	ret = hydrology_device_reboot();
	// 	if (ret) {
	// 		goto err;
	// 	}

	ret = button_task_init();
	if (ret) {
		goto err;
	}

	ret = switch_task_init();
	if (ret) {
		goto err;
	}

	return 0;
err:
	pr_err("Failed to init test platform, error code: %d\n", ret);
	return ret;
}

static void idle_task_exit(void)
{
	// button_task_exit();
	// 	switch_task_init();
}

static void idle_task_schedule(void)
{
	tim_task_loop();
	// rtc_task_loop();
}

int parse_test_all_case(char *cmd)
{
	u32 i = 0;
	u32 argc = 0;
	char *argv[128];
	int ret;

	do {
		if (argc >= 128) {
			pr_err("too many arguments, maximum 128 allowed\n");
			break;
		}
		argv[argc] = strsep(&cmd, " ");
		if (!argv[argc]) {
			break;
		}
		pr_info("[%d] %s\n", argc, argv[argc]);
		argc++;
	} while (cmd);

	for (i = 0; i < ARRAY_SIZE(kinetis_case_table); i++) {
		if (!strcmp(kinetis_case_table[i].command, argv[0]) &&
			kinetis_case_table[i].function != NULL) {
			 ret = kinetis_case_table[i].function(argc, argv);
			 if (ret) {
				 pr_err("test case %s failed, error code: %s\n", argv[0], errname(ret));
				 return FAIL;
			 } else {
				 return PASS;
			 }
		}
	}

	return NOT_EXSIST;
}

int k_test_case_schedule(void)
{
	char *buffer;
	int ret;

	ret = idle_task_init();
	if (ret) {
		goto err;
	}

	pr_info("/----------Test platform has been activated.----------/");

	buffer = kmalloc(256, __GFP_ZERO);
	if (!buffer) {
		pr_err("Failed to allocate input buffer\n");
		ret = -ENOMEM;
		goto err;
	}

	// Force log flush
	pr_info("/ # \n");

	while (1) {
		ret = get_user_input_string(buffer, 256);

		if (ret == 0) {
			if (buffer[0] == '\0') {
				printf("/ # ");
				continue;  // Skip to next iteration
			} else if (buffer[0] == 27) {
				pr_info("\n");
				break;
			} else if (buffer[0] != '\0') {
				ret = parse_test_all_case(buffer);

				if (ret == PASS) {
					pr_info("TEST PASS\n");
				} else if (ret == FAIL) {
					pr_info("TEST FAIL\n");
				} else if (ret == NOT_EXSIST) {
					pr_info("TEST NOT EXIST\n");
				}
				printf("/ # ");
				continue;  // Skip to next iteration
			}
		} else if (ret == -ETIMEDOUT) {
			idle_task_schedule();
		} else {
			pr_err("get_user_input_string() failed, ret=%d\n", ret);
			break;
		}
	}

	kfree(buffer);

err:
	idle_task_exit();

	return ret;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */
