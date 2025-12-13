
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/printk.h>

#include "kinetis/test-kinetis.h"
#include "kinetis/user-shell.h"
#include "kinetis/rtc-task.h"
#include "kinetis/tim-task.h"
// #include "kinetis/button.h"
// #include "kinetis/switch.h"
#include "kinetis/fatfs.h"

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
int t_ak8975_basic_info(int argc, char **argv);
int t_ak8975_magnetic(int argc, char **argv);
int t_ak8975_selftest(int argc, char **argv);
int t_ak8975_fuse_rom_access(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_AT24CXX
int t_at24cxx_loopback(int argc, char **argv);
int t_at24cxx_current_addr_read(int argc, char **argv);
int t_at24cxx_current_random_read(int argc, char **argv);
int t_at24cxx_sequential_read(int argc, char **argv);
int t_at24cxx_loopback_speed(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_DS3231
int t_ds3231_set_clock(int argc, char **argv);
int t_ds3231_get_clock(int argc, char **argv);
int t_ds3231_set_alarm1(int argc, char **argv);
int t_ds3231_set_alarm2(int argc, char **argv);
int t_ds3231_square_wave(int argc, char **argv);
int t_ds3231_32khz_wave(int argc, char **argv);
int t_ds3231_get_temprature(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_BASICTIMER
int t_basic_timer_get_tick(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_CRC
int t_crc(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_BUTTON
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
int t_fatfs_expend(int argc, char **argv);
int t_fatfs_diskio(int argc, char **argv);
int t_fatfs_contiguous_file(int argc, char **argv);
int t_fatfs_raw_speed(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_FSM
int t_fsm_example(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_GENERAL
int t_general_success(int argc, char **argv);
int t_general_error(int argc, char **argv);
int t_general_timeout(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_HC_05
int t_hc_05_test_cmd(int argc, char **argv);
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
int t_random_array(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RS485
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_RTC
int t_rtc_set_clock(int argc, char **argv);
int t_rtc_get_clock(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RTCTASK
int t_rtc_task_add(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_SEIRALPORT
int t_serial_port_shell(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_SHELL
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_SLIST
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_SWITCH
int t_switch_add(int argc, char **argv);
int t_switch_drop(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_TIMTASK
int t_tim_task_add(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MY9221
int t_my9221_send_packet(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_TLC5971
int t_tlc5971_send_packet(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_W25QXXX
int t_w25qxxx_chip_erase(int argc, char **argv);
int t_w25qxxx_read_info(int argc, char **argv);
int t_w25qxxx_loopback(int argc, char **argv);
#endif

struct test_case_typedef {
	char *command;
	int (*function)(int argc, char **argv);
};

struct test_case_typedef kinetis_case_table[] = {

#ifdef DESIGN_VERIFICATION_AK8975
	{"ak8975.basic-info",           t_ak8975_basic_info},
	{"ak8975.magnetic",             t_ak8975_magnetic},
	{"ak8975.selftest",             t_ak8975_selftest},
	{"ak8975.fuse-rom-access",      t_ak8975_fuse_rom_access},
#endif
#ifdef DESIGN_VERIFICATION_AT24CXX
	{"at24cxx.loopback",            t_at24cxx_loopback},
	{"at24cxx.current-addr-read",   t_at24cxx_current_addr_read},
	{"at24cxx.random-read",         t_at24cxx_current_random_read},
	{"at24cxx.seq-read",            t_at24cxx_sequential_read},
	{"at24cxx.lb-speed",            t_at24cxx_loopback_speed},
#endif
#ifdef DESIGN_VERIFICATION_BMI160
	{"bmi160.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_DS3231
	{"ds3231.set-clock",            t_ds3231_set_clock},
	{"ds3231.get-clock",            t_ds3231_get_clock},
	{"ds3231.set-alarm1",           t_ds3231_set_alarm1},
	{"ds3231.set-alarm2",           t_ds3231_set_alarm2},
	{"ds3231.square-wave",          t_ds3231_square_wave},
	{"ds3231.32khz-wave",           t_ds3231_32khz_wave},
	{"ds3231.get-temprature",       t_ds3231_get_temprature},
#endif
#ifdef DESIGN_VERIFICATION_ESP32
	{"esp32.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_GPRS
	{"gprs.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_GSM
	{"gsm.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_GT9271
	{"gt9271.", },
#endif
#ifdef DESIGN_VERIFICATION_HC_05
	{"hc-05.test",                  t_hc_05_test_cmd},
#endif
#ifdef DESIGN_VERIFICATION_HMC5883L
	{"hmc5883l.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_HYDROLOGY
	{"hydrology.init",              t_hydrology_init},
	{"hydrology.test",              t_hydrology},
#endif
#ifdef DESIGN_VERIFICATION_ICM20602
	{"icm20602.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_IIC
	{"iic.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_IS25LPWP256D
	{"is25lpwp256d.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_BASICTIMER
	{"basic-timer.get-tick",        t_basic_timer_get_tick},
#endif
#ifdef DESIGN_VERIFICATION_CHINESE
	{"chinese.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_CRC
	{"crc.test",                    t_crc},
#endif
#ifdef DESIGN_VERIFICATION_DELAY
	{"delay.test",                 t_delay},
#endif
#ifdef DESIGN_VERIFICATION_FATFS
	{"fatfs.operate",               t_fatfs_operate},
	{"fatfs.loopback",              t_fatfs_loopback},
	{"fatfs.miscellaneous",         t_fatfs_miscellaneous},
	{"fatfs.file-check",            t_fatfs_file_check},
	{"fatfs.scan-files",            t_fatfs_scan_files},
	{"fatfs.append",                t_fatfs_append},
	{"fatfs.delete-node",           t_fatfs_delete_node},
	{"fatfs.expend",                t_fatfs_expend},
	{"fatfs.diskio",                t_fatfs_diskio},
	{"fatfs.contiguous-file",       t_fatfs_contiguous_file},
	{"fatfs.raw-speed",             t_fatfs_raw_speed},
#endif
#ifdef DESIGN_VERIFICATION_FSM
	{"fsm.example",                 t_fsm_example},
#endif
#ifdef DESIGN_VERIFICATION_GENERAL
	{"general.success",             t_general_success},
	{"general.error",               t_general_error},
	{"general.timeout",             t_general_timeout},
#endif
#ifdef DESIGN_VERIFICATION_BUTTON
	{"button.add",                  t_button_add},
	{"button.drop",                 t_button_drop},
#endif
#ifdef DESIGN_VERIFICATION_LCD
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_LED
	{"led.add",                     t_led_add},
	{"led.drop",                    t_led_drop},
#endif
#ifdef DESIGN_VERIFICATION_RNG
	{"random.number",               t_random_number},
	{"random.array",                t_random_array},
#endif
#ifdef DESIGN_VERIFICATION_RS485
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_RTC
	{"rtc.set-clock",               t_rtc_set_clock},
	{"rtc.get-clock",               t_rtc_get_clock},
#endif
#ifdef DESIGN_VERIFICATION_RTCTASK
	{"rtc-task.add",                t_rtc_task_add},
#endif
#ifdef DESIGN_VERIFICATION_SEIRALPORT
	{"serial-port.shell",           t_serial_port_shell},
#endif
#ifdef DESIGN_VERIFICATION_SHELL
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SLIST
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SWITCH
	{"switch.add",                  t_switch_add},
	{"switch.drop",                 t_switch_drop},
#endif
#ifdef DESIGN_VERIFICATION_TIMTASK
	{"timt-ask.add",                 t_tim_task_add},
#endif
#ifdef DESIGN_VERIFICATION_TOUCHSCREEN
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MAX30205
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MPU6050
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MT29F4G08
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MY9221
	{"my9221.send",                 t_my9221_send_packet},
#endif
#ifdef DESIGN_VERIFICATION_NBIOT
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_OLED
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_RAK477
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SHT20
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SPL06
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SSD1306
	{"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_TLC5971
	{"tlc5971.send",                t_tlc5971_send_packet},
#endif
#ifdef DESIGN_VERIFICATION_W25QXXX
	{"w25qxxx.loopback",            t_w25qxxx_loopback},
	{"w25qxxx.info",                t_w25qxxx_read_info},
	{"w25qxxx.erase",               t_w25qxxx_chip_erase},
#endif
#ifdef DESIGN_VERIFICATION_XMODEM
	{"test", fuction},
#endif
};

static int idle_task_init(void)
{
	int ret;

	ret = fatfs_init();

	if (ret)
		goto err;

// 	ret = hydrology_device_reboot();
//
// 	if (ret)
// 		goto err;

// 	ret = button_task_init();
//
// 	if (ret)
// 		goto err;
//
// 	ret = switch_task_init();
//
// 	if (ret)
// 		goto err;

	return 0;
err:
	pr_err("Failed to init test platform, error code: %d\n", ret);
	return ret;
}

static void idle_task_exit(void)
{
// 	button_task_exit();
// 	switch_task_init();
}

static void idle_task_schedule(void)
{
	tim_task_loop();
	rtc_task_loop();
}

int parse_test_all_case(char *cmd)
{
	u32 i = 0;
	u32 argc = 0;
	char *argv[128];

	do {
		argv[argc] = strsep(&cmd, " ");
		pr_info("[%d] %s\n", argc, argv[argc]);
		argc++;
	} while (cmd);

	for (i = 0; i < ARRAY_SIZE(kinetis_case_table); i++) {
		if (!strcmp(kinetis_case_table[i].command, argv[0]) &&
			kinetis_case_table[i].function != NULL)
			return kinetis_case_table[i].function(argc, argv);
	}

	return NOT_EXSIST;
}

int k_test_case_schedule(void)
{
	char *buffer;
	int ret;

	ret = idle_task_init();
	if (ret)
		goto err;

	pr_info("/----------Test platform has been activated.----------/");

	buffer = kmalloc(128, __GFP_ZERO);
	
	// Force log flush
	pr_info("");

	while (1) {
		printf("/ # ");
		if (get_user_input_string(buffer, 128) == 0) {
			if (buffer[0] == 27) {
				pr_info("\n");
				break;
			} else if (buffer[0] != '\0') {
				ret = parse_test_all_case(buffer);

				if (ret == PASS)
					pr_info("TEST PASS\n");
				else if (ret == FAIL)
					pr_info("TEST FAIL\n");
				else
					pr_info("TEST NOT EXSIST\n");
			}
		}

		idle_task_schedule();
	}

	kfree(buffer);

err:
	idle_task_exit();

	return ret;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */
