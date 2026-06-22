
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/iopoll.h>
#include <fake-mcu/interface.h>

#include <kinetis/tim-task.h>
#include <kinetis/serial-port.h>
#include <kinetis/mavlink.h>
#include <kinetis/fatfs-intf.h>
#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/real-time-clock.h>
#include <kinetis/basic-timer.h>
#include "kinetis/bootloader.h"
#include "kinetis/nrf24l01_port.h"

#include "lrzsz/zmodem.h"
#include "fatfs/ff.h"

#include "project.h"
#include "config.h"
#include "rotor.h"
#include "stator.h"
#include "interface.h"

// #include "wifi/tcp_ap.h"
// #include "wifi/http_server.h"
// #include "wifi/wifi.h"

#if KINETIS_FAKE_SIM == 0
#include "main.h"
#endif

#if KINETIS_FAKE_SIM
#include <pthread.h>
#include <unistd.h>

static volatile u8 stator_thread_switch;

static void *stator_thread_func(void *arg)
{
	struct pov_stator *stator = arg;

	while (stator_thread_switch) {
		pov_stator_fsm(stator);
		tim_task_loop();
	}

	return NULL;
}

static volatile u8 phone_thread_switch;

static void *phone_thread_func(void *arg)
{
	struct pov_rotor *rotor = (struct pov_rotor *)arg;
	struct serial_port *phone_port = NULL;
	struct mavlink_device *phone_mavlink = NULL;
	u32 operation;
	int ret;

	phone_port = serial_port_alloc(&usart1_serial_port_ops, "phone");
	if (!phone_port) {
		return NULL;
	}

	phone_mavlink = mavlink_init(phone_port, 3, 0, "phone");
	if (!phone_mavlink) {
		serial_port_free(phone_port);
		return NULL;
	}

	serial_port_start_thread(phone_port, SERIAL_PORT_DF_OTHERS,
		NULL, rotor->app_port);
	serial_port_start_thread(rotor->app_port, SERIAL_PORT_DF_OTHERS,
		NULL, phone_port);

	pov_create_fake_images(APP_IMAGE_PATH);

	/* Wait for connection: bidirectional heartbeat exchange.
	 * Both sides may power on in any order, so keep sending and
	 * listening until each side has heard the other. */
	while (1) {
		mavlink_send_heartbeat(phone_mavlink, MAV_SYS_ACTIVE, 0, 0);

		/* Check if rotor's heartbeat arrived */
		if (serial_port_data_available(phone_port) > 0) {
			mavlink_receive_and_process(phone_mavlink, 10);

			if (phone_mavlink->sys_status == MAV_SYS_ACTIVE) {
				break;
			}
		}

		sleep(1);
	}

	while (phone_thread_switch) {
		/* Simulate user thinking time: 3~7 seconds */
		sleep(3 + (get_random_u32() % 5));

		/* Randomly pick an operation (0-7) */
		operation = get_random_u32() % 8;

		switch (operation) {
		/* ── Heartbeat ─────────────────────────────────── */
		case 0:
			mavlink_send_heartbeat(phone_mavlink, MAV_SYS_ACTIVE, 0, 0);
			pr_info("phone: sent HEARTBEAT\n");
			break;

		/* ── Upload a test .pov file ──────────────────── */
		case 1: {
			DIR dir;
			FILINFO fno;
			FRESULT res;

			res = f_opendir(&dir, APP_IMAGE_PATH);
			if (res != FR_OK) {
				pr_err("failed to open '%s': %d\n", APP_IMAGE_PATH, res);
				break;
			}

			while (1) {
				res = f_readdir(&dir, &fno);
				if (res != FR_OK || fno.fname[0] == 0) {
					break;    /* End of directory or error */
				}

				/* Skip directories and system files */
				if (fno.fattrib & (AM_DIR | AM_SYS)) {
					continue;
				}

				/* Check if it's an .pov file */
				if (fno.fsize > 0) {
					char *ext = strrchr(fno.fname, '.');
					const char *file_list[] = { fno.fname };
					if (ext && strcmp(ext, ".pov") == 0) {
						ret = mavlink_send_ft_start(phone_mavlink, MAV_FT_UPLOAD, fno.fname, fno.fsize, 3000);
						if (ret != 0) {
							break;
						}
						pr_info("send image: %s\n", fno.fname);
						ret = lrzsz_sz(phone_port, 1, file_list);
						if (ret != 0) {
							break;
						}
					}
				}
			}

			f_closedir(&dir);

			break;
		}

		/* ── IMAGE_CONTROL: next ─────────────────────── */
		case 2:
			ret = mavlink_send_image_control(phone_mavlink,
					MAV_IMAGE_NEXT, 0);
			if (ret < 0) {
				pr_err("phone: IMAGE_CONTROL NEXT failed: %d\n", ret);
			} else {
				pr_info("phone: sent IMAGE_CONTROL NEXT\n");
			}
			break;

		/* ── IMAGE_CONTROL: previous ─────────────────── */
		case 3:
			ret = mavlink_send_image_control(phone_mavlink,
					MAV_IMAGE_PREVIOUS, 0);
			if (ret < 0) {
				pr_err("phone: IMAGE_CONTROL PREVIOUS failed: %d\n", ret);
			} else {
				pr_info("phone: sent IMAGE_CONTROL PREVIOUS\n");
			}
			break;

		/* ── IMAGE_CONTROL: replay ───────────────────── */
		case 4:
			ret = mavlink_send_image_control(phone_mavlink,
					MAV_IMAGE_REPLAY, 0);
			if (ret < 0) {
				pr_err("phone: IMAGE_CONTROL REPLAY failed: %d\n", ret);
			} else {
				pr_info("phone: sent IMAGE_CONTROL REPLAY\n");
			}
			break;

		/* ── IMAGE_CONTROL: play index 0 ─────────────── */
		case 5:
			ret = mavlink_send_image_control(phone_mavlink,
					MAV_IMAGE_PLAY_INDEX, 0);
			if (ret < 0) {
				pr_err("phone: IMAGE_CONTROL PLAY_INDEX failed: %d\n", ret);
			} else {
				pr_info("phone: sent IMAGE_CONTROL PLAY_INDEX 0\n");
			}
			break;

		default:
			break;
		}

		if (ret != 0) {
			break;
		}
	}

	pr_info("phone: thread stopping\n");

	serial_port_stop_thread(phone_port);
	serial_port_stop_thread(rotor->app_port);
	mavlink_free(phone_mavlink);
	serial_port_free(phone_port);

	return NULL;
}

#endif /* KINETIS_FAKE_SIM */

// static void wifi_task_cb(struct tim_task *task)
// {
// 	wifi_schedule();
// }

int board_init(void)
{
	pr_info("==== %s v%s ====\n", PROJECT_NAME, PROJECT_VERSION);
	pr_info("Platform: %s\n",
		MCU_PLATFORM_STM32 ? "STM32" : "Simulated");
#if KINETIS_FAKE_SIM
	return 0;
#else
	arm_dwt_init();

	return stm32_hal_init();
#endif
}

int app_main(void)
{
	struct pov_rotor *rotor = NULL;
	struct pov_stator *stator = NULL;
	struct serial_port_ops *motor_ops;
#if KINETIS_FAKE_SIM
	static pthread_t timer_thread;
	static pthread_t phone_thread;
#endif
	int ret = 0;

#if POV_RUN_MODE == POV_RUN_HOST
	pr_info("pov: running as rotor (rotating end)\n");

#if KINETIS_FAKE_SIM == 0
#if POV_USE_NRF24L01
	motor_ops = &nrf24l01_serial_port_ops;
#else
	motor_ops = &usart2_serial_port_ops;
#endif
	rotor = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
		motor_ops, &usart6_serial_port_ops,
		&spi1_master, &spi6_master,
		hall_read_rotated_time);
	if (!rotor) {
		pr_err("pov: failed to allocate rotor\n");
		return -ENOMEM;
	}
	stm32_serial_port[1] = rotor->motor_port;
	stm32_serial_port[2] = rotor->app_port;
#if POV_USE_MT6701
#else
	hall_dev = rotor->angle_sensor;
#endif

// 	rotor->wifi = wifi_init(WIFI_SSID, WIFI_PASSWORD, 1,
// 		WIFI_STATIC_IP, WIFI_STATIC_GW, WIFI_STATIC_NETMASK);
// 	if (rotor->wifi) {
// 	} else {
// 		return -ENOMEM;
// 	}
// 
// 	ret = tim_task_add(&rotor->wifi->schedule_task, "wifi_poll", WIFI_POLL_INTERVAL_MS * 1000,
// 				true, false, wifi_task_cb);
// 	if (ret != 0)
// 		return ret;

#else
	rotor = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
		&usart1_serial_port_ops, &usart2_serial_port_ops,
		&spi1_master, &spi1_master, hall_read_rotated_time);
	if (!rotor) {
		pr_err("pov: failed to allocate rotor\n");
		return -ENOMEM;
	}
	stator = pov_stator_alloc(&usart1_serial_port_ops,
				 &general_flash_ops, &general_bootloader_ops,
				 hall_read_rotated_time);
	if (!stator) {
		pr_err("pov: failed to allocate stator\n");
		pov_rotor_free(rotor);
		return -ENOMEM;
	}

	serial_port_start_thread(rotor->motor_port, SERIAL_PORT_DF_OTHERS,
		NULL, stator->motor_port);
	serial_port_start_thread(stator->motor_port, SERIAL_PORT_DF_OTHERS,
		NULL, rotor->motor_port);

	/* Start timer thread for stator simulation */
	stator_thread_switch = 1;
	ret = pthread_create(&timer_thread, NULL, stator_thread_func, stator);
	if (ret != 0) {
		stator_thread_switch = 0;
		pr_err("pov: failed to create timer thread: %d\n", ret);
		goto out;
	}

// 	/* Start phone APP simulation thread */
// 	phone_thread_switch = 1;
// 	ret = pthread_create(&phone_thread, NULL, phone_thread_func, rotor);
// 	if (ret != 0) {
// 		phone_thread_switch = 0;
// 		pr_err("pov: failed to create phone thread: %d\n", ret);
// 		/* Stop timer thread before exit to prevent resource leak */
// 		stator_thread_switch = 0;
// 		pthread_join(timer_thread, NULL);
// 		goto out;
// 	}
	mdelay(1000);
#endif /* KINETIS_FAKE_SIM */

	ret = pov_rotor_display(rotor);

#if KINETIS_FAKE_SIM
	/* Shutdown threads */
	phone_thread_switch = 0;
	pthread_join(phone_thread, NULL);

	stator_thread_switch = 0;
	pthread_join(timer_thread, NULL);
#endif /* KINETIS_FAKE_SIM */

out:
	pov_rotor_free(rotor);
	pov_stator_free(stator);

#elif POV_RUN_MODE == POV_RUN_SLAVE
	pr_info("pov: running as stator (fixed end)\n");

#if POV_USE_NRF24L01
	motor_ops = &nrf24l01_serial_port_ops;
#else
	motor_ops = &usart2_serial_port_ops;
#endif
	stator = pov_stator_alloc(motor_ops,
				&general_flash_ops, &general_bootloader_ops,
				hall_read_rotated_time);
	if (!stator) {
		pr_err("pov: failed to allocate stator\n");
		return -ENOMEM;
	}
	stm32_serial_port[1] = stator->motor_port;
	hall_dev = stator->hall;

	/* Keep running until shutdown signal */
	while (1) {
		pov_stator_fsm(stator);
		tim_task_loop();
	}

#else
	pr_err("pov: invalid run mode %d\n", POV_RUN_MODE);
	ret = -EINVAL;
#endif

	pr_info("pov: exit with %d\n", ret);

	return ret;
}
