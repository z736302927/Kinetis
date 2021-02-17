#include "kinetis/test-kinetis.h"
#include "kinetis/shell.h"
#include "kinetis/idebug.h"

#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "string.h"

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
int t_current_random_read(int argc, char **argv);
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
int t_FSM_Example(int argc, char **argv);
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
#endif

#ifdef DESIGN_VERIFICATION_LED
int t_led_add(int argc, char **argv);
int t_led_drop(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MEMORY
int t_memory_Test(int argc, char **argv);
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
int t_w25qxxx_loopback(int argc, char **argv);
#endif

struct test_case_typedef {
    char *command;
    int (*function)(int argc, char **argv);
};

struct test_case_typedef kinetis_case_table[] = {
    
#ifdef DESIGN_VERIFICATION_AK8975
    {"ak8975.basicinfo", t_ak8975_basic_info},
    {"ak8975.magnetic", t_ak8975_magnetic},
    {"ak8975.selftest", t_ak8975_selftest},
    {"ak8975.fuseromaccess", t_ak8975_fuse_rom_access},
#endif
#ifdef DESIGN_VERIFICATION_AT24CXX
    {"at24cxx.lb", t_at24cxx_loopback},
    {"at24cxx.current_addr_read", t_at24cxx_current_addr_read},
    {"at24cxx.random_read", t_current_random_read},
    {"at24cxx.seq_read", t_at24cxx_sequential_read},
    {"at24cxx.lb_speed", t_at24cxx_loopback_speed},
#endif
#ifdef DESIGN_VERIFICATION_BMI160
    {"bmi160.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_DS3231
    {"ds3231.setclock", t_ds3231_set_clock},
    {"ds3231.getclock", t_ds3231_get_clock},
    {"ds3231.setalarm1", t_ds3231_set_alarm1},
    {"ds3231.setalarm2", t_ds3231_set_alarm2},
    {"ds3231.squarewave", t_ds3231_square_wave},
    {"ds3231.32khzwave", t_ds3231_32khz_wave},
    {"ds3231.gettemprature", t_ds3231_get_temprature},
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
    {"hc-05.test", t_hc_05_test_cmd},
#endif
#ifdef DESIGN_VERIFICATION_HMC5883L
    {"hmc5883l.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_HYDROLOGY
    {"hydrology.test", t_hydrology},
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
    {"basic-timer.gettick", t_basic_timer_get_tick},
#endif
#ifdef DESIGN_VERIFICATION_CHINESE
    {"chinese.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_CRC
    {"crc.test", t_crc},
#endif
#ifdef DESIGN_VERIFICATION_DELAY
    {"delay.delay", t_delay},
#endif
#ifdef DESIGN_VERIFICATION_FATFS
    {"fatfs.loopback", t_fatfs_loopback},
    {"fatfs.miscellaneous", t_fatfs_miscellaneous},
    {"fatfs.filecheck", t_fatfs_file_check},
    {"fatfs.scanfiles", t_fatfs_scan_files},
    {"fatfs.append", t_fatfs_append},
    {"fatfs.deletenode", t_fatfs_delete_node},
    {"fatfs.expend", t_fatfs_expend},
    {"fatfs.diskio", t_fatfs_diskio},
    {"fatfs.contiguousfile", t_fatfs_contiguous_file},
    {"fatfs.rawspeed", t_fatfs_raw_speed},
#endif
#ifdef DESIGN_VERIFICATION_FSM
    {"fsm.Example", t_fsm_example},
#endif
#ifdef DESIGN_VERIFICATION_GENERAL
    {"general.success", t_general_success},
    {"general.error", t_general_error},
    {"general.timeout", t_general_timeout},
#endif
#ifdef DESIGN_VERIFICATION_BUTTON
    {"button.add", t_button_add},
    {"button.drop", t_button_drop},
#endif
#ifdef DESIGN_VERIFICATION_LCD
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_LED
    {"led.add", t_led_add},
    {"led.drop", t_led_drop},
#endif
#ifdef DESIGN_VERIFICATION_MEMORY
    {"memory.test", t_memory_Test},
#endif
#ifdef DESIGN_VERIFICATION_RNG
    {"random.number", t_random_number},
    {"random.array", t_random_array},
#endif
#ifdef DESIGN_VERIFICATION_RS485
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_RTC
    {"rtc.setclock", t_rtc_set_clock},
    {"rtc.setclock", t_rtc_get_clock},
#endif
#ifdef DESIGN_VERIFICATION_RTCTASK
    {"rtc-task.add", t_rtc_task_add},
#endif
#ifdef DESIGN_VERIFICATION_SEIRALPORT
    {"serial-port.shell", t_serial_port_shell},
#endif
#ifdef DESIGN_VERIFICATION_SHELL
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SLIST
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SWITCH
    {"switch.add", t_switch_add},
    {"switch.drop", t_switch_drop},
#endif
#ifdef DESIGN_VERIFICATION_TIMTASK
    {"timtask.add", t_tim_task_add},
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
    {"my9221.send", t_my9221_send_packet},
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
    {"tlc5971.send", t_tlc5971_send_packet},
#endif
#ifdef DESIGN_VERIFICATION_W25QXXX
    {"w25qxxx.loopback", t_w25qxxx_loopback},
#endif
#ifdef DESIGN_VERIFICATION_XMODEM
    {"test", fuction},
#endif
};

int parse_test_all_case(char *cmd)
{
    u32 i = 0;
    u32 argc = 0;
    char *argv[128];

    do {
        argv[argc] = strsep(&cmd, " ");
        printk(KERN_DEBUG "[%d] %s", argc, argv[argc]);
        argc++;
    } while (cmd);

    for (i = 0; i < ARRAY_SIZE(kinetis_case_table); i++) {
        if (!strcmp(kinetis_case_table[i].command, argv[0]) &&
            kinetis_case_table[i].function != NULL)
            return kinetis_case_table[i].function(argc, argv);
    }

    return NOT_EXSIST;
}

void k_test_case_schedule(void)
{
    char *buffer;
    int ret;
    
    buffer = kmalloc(128, __GFP_ZERO);

    while (1) {
        if (shell_get_user_input(buffer) == true) {
            if (buffer[0] == '\r')
                printf("/ # ");
            else if (buffer[0] == 27)
                break;
            else {
                ret = parse_test_all_case(buffer);

                if (ret == PASS)
                    printk(KERN_DEBUG "TEST PASS");
                else if (ret == FAIL)
                    printk(KERN_DEBUG "TEST FAIL");
                else
                    printk(KERN_DEBUG "TEST NOT EXSIST");

                printf("\r\n/ # ");
            }
        }
    }

    kfree(buffer);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */
