
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
#include <kinetis/bootloader.h>
#include <kinetis/nrf24l01_port.h>

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

static u32 phone_pick_operation(const u32 *weights, u32 n, u32 total)
{
	u32 r = get_random_u32() % total;
	u32 sum = 0;

	for (u32 i = 0; i < n; i++) {
		sum += weights[i];
		if (r < sum)
			return i;
	}
	return n - 1;
}

static void *phone_thread_func(void *arg)
{
	struct pov_rotor *rotor = (struct pov_rotor *)arg;
	struct serial_port *phone_port = NULL;
	struct mavlink_device *phone_mavlink = NULL;
	u32 operation;
	u32 burst_count;
	static const u32 weights[] = { 10, 15, 15, 12, 8, 8, 10, 10, 5, 7 };
	int ret = 0;

	sleep(10);

	phone_port = serial_port_alloc(&usart1_serial_port_ops, "phone");
	if (!phone_port)
		return NULL;

	phone_mavlink = mavlink_init(phone_port, 3, 0, "phone");
	if (!phone_mavlink) {
		serial_port_free(phone_port);
		return NULL;
	}

	serial_port_start_thread(phone_port, SERIAL_PORT_DF_OTHERS,
		NULL, rotor->app_port);
	serial_port_start_thread(rotor->app_port, SERIAL_PORT_DF_OTHERS,
		NULL, phone_port);

	/* Phone actively queries rotor until it responds. */
	while (phone_thread_switch) {
		u32 old_rx = phone_mavlink->rx_count;

		mavlink_query_motor_status(phone_mavlink, 0, 500);

		if (phone_mavlink->rx_count > old_rx)
			break;

		sleep(1);
	}

	if (!phone_thread_switch)
		goto cleanup;

	/*
	 * Operation weights (total = 100):
	 *   0  Upload single .pov image        15
	 *   1  IMAGE_CONTROL NEXT              15
	 *   2  IMAGE_CONTROL PREVIOUS          12
	 *   3  IMAGE_CONTROL REPLAY            8
	 *   4  IMAGE_CONTROL PLAY_INDEX (0..N)  8
	 *   5  Query motor status              10
	 *   6  Burst: 2-4 rapid operations     10
	 *   7  Download request (FT_START DOWN) 5
	 *   8  File list request                7
	 */
	while (phone_thread_switch) {
		/* Variable idle delay simulating user browsing */
		sleep(1 + (get_random_u32() % 3));

		operation = phone_pick_operation(weights, ARRAY_SIZE(weights), 100);

		ret = 0;
		switch (operation) {
		/* ── Upload single .pov file ───────────────── */
		case 0: {
			DIR dir;
			FILINFO fno;
			FRESULT res;
			const char *chosen = NULL;
			u32 chosen_size = 0;

			res = f_opendir(&dir, APP_IMAGE_PATH);
			if (res != FR_OK) {
				pr_err("phone: failed to open '%s': %d\n",
					APP_IMAGE_PATH, res);
				break;
			}

			/* Collect just one random .pov file */
			while (1) {
				res = f_readdir(&dir, &fno);
				if (res != FR_OK || fno.fname[0] == 0)
					break;
				if (fno.fattrib & (AM_DIR | AM_SYS))
					continue;

				char *ext = strrchr(fno.fname, '.');
				if (ext && strcmp(ext, ".pov") == 0) {
					chosen = fno.fname;
					chosen_size = fno.fsize;
					/* 50% chance to stop on first match
					 * for single-file realism */
					if (get_random_u32() & 1)
						break;
				}
			}

			f_closedir(&dir);

			if (chosen) {
				char full_path[128];
				const char *file_list[] = { full_path };
				u8 layer_idx = get_random_u32() % POV_LAYER_COUNT;
				char layer_path[16];

				snprintf(full_path, sizeof(full_path), "%s/%s",
					APP_IMAGE_PATH, chosen);
				snprintf(layer_path, sizeof(layer_path),
					"layer%d", layer_idx);
				ret = mavlink_send_ft_start(phone_mavlink,
					MAV_FT_UPLOAD, layer_path, chosen_size, 3000);
				if (ret == 0) {
					pr_info("phone: upload '%s' -> %s (%u B)\n",
						full_path, layer_path, chosen_size);
					ret = lrzsz_sz(phone_port, 1, file_list);
				}
			}
			break;
		}

		/* ── IMAGE_CONTROL NEXT ────────────────────── */
		case 1:
			ret = mavlink_send_image_control(phone_mavlink,
				MAV_IMAGE_NEXT, 0);
			if (ret < 0)
				pr_err("phone: IMAGE_CONTROL NEXT failed: %d\n", ret);
			else
				pr_info("phone: sent IMAGE_CONTROL NEXT\n");
			break;

		/* ── IMAGE_CONTROL PREVIOUS ────────────────── */
		case 2:
			ret = mavlink_send_image_control(phone_mavlink,
				MAV_IMAGE_PREVIOUS, 0);
			if (ret < 0)
				pr_err("phone: IMAGE_CONTROL PREVIOUS failed: %d\n", ret);
			else
				pr_info("phone: sent IMAGE_CONTROL PREVIOUS\n");
			break;

		/* ── IMAGE_CONTROL REPLAY ──────────────────── */
		case 3:
			ret = mavlink_send_image_control(phone_mavlink,
				MAV_IMAGE_REPLAY, 0);
			if (ret < 0)
				pr_err("phone: IMAGE_CONTROL REPLAY failed: %d\n", ret);
			else
				pr_info("phone: sent IMAGE_CONTROL REPLAY\n");
			break;

		/* ── IMAGE_CONTROL PLAY_INDEX ──────────────── */
		case 4:
			ret = mavlink_send_image_control(phone_mavlink,
				MAV_IMAGE_PLAY_INDEX, get_random_u32() % 5);
			if (ret < 0)
				pr_err("phone: IMAGE_CONTROL PLAY_INDEX failed: %d\n", ret);
			else
				pr_info("phone: sent IMAGE_CONTROL PLAY_INDEX\n");
			break;

		/* ── Query motor status ────────────────────── */
		case 5:
			ret = mavlink_query_motor_status(phone_mavlink,
				0, 3000);
			if (ret < 0)
				pr_err("phone: query motor status failed: %d\n", ret);
			else
				pr_info("phone: query motor status OK\n");
			break;

		/* ── Burst: rapid-fire 2~4 operations ──────── */
		case 6:
			burst_count = 2 + (get_random_u32() % 3);
			pr_info("phone: burst start (%u ops)\n",
				burst_count);
			for (u32 i = 0; i < burst_count && phone_thread_switch; i++) {
				usleep(500000 + (get_random_u32() % 500000));
				mavlink_send_image_control(phone_mavlink,
					get_random_u32() & 1 ?
					MAV_IMAGE_NEXT :
					MAV_IMAGE_PREVIOUS, 0);
			}
			pr_info("phone: burst end\n");
			/* Longer pause after burst */
			sleep(4 + (get_random_u32() % 6));
			break;

		/* ── Download request ──────────────────────── */
		case 7: {
			ret = mavlink_send_ft_start(phone_mavlink,
				MAV_FT_DOWNLOAD, "layer0", 0, 5000);
			if (ret < 0) {
				pr_err("phone: FT_START DOWNLOAD failed: %d\n", ret);
				break;
			}
			pr_info("phone: rotor ready, receiving...\n");

			ret = lrzsz_rz(phone_port, PHONE_DOWNLOAD_DIR);
			if (ret <= 0)
				pr_warn("phone: download receive failed (%d)\n", ret);
			else
				pr_info("phone: downloaded %d bytes\n", ret);
			break;
		}

		/* ── File list request ─────────────────────── */
		case 8: {
			u32 old_rx = phone_mavlink->rx_count;

			ret = mavlink_send_ft_list(phone_mavlink, 0, 0, 0, "");
			if (ret < 0) {
				pr_err("phone: FT_LIST request failed: %d\n", ret);
				break;
			}
			pr_info("phone: sent FT_LIST request\n");

			/* Give the rotor time to stream list entries back */
			for (int i = 0; i < 50 && phone_thread_switch; i++) {
				if (phone_mavlink->rx_count > old_rx)
					break;
				usleep(50000);
			}
			pr_info("phone: received %u list message(s) from rotor\n",
				phone_mavlink->rx_count - old_rx);
			break;
		}

		default:
			break;
		}
	}

cleanup:
	pr_info("phone: thread stopping\n");

	serial_port_stop_thread(phone_port);
	serial_port_stop_thread(rotor->app_port);
	mavlink_free(phone_mavlink);
	serial_port_free(phone_port);

	return NULL;
}

static volatile u8 arm_thread_switch;

static void *arm_thread_func(void *arg)
{
	struct pov_rotor *rotor = arg;

	while (arm_thread_switch) {
		pov_rotor_display(rotor);
	}

	return NULL;
}

static struct pov_rotor *create_arm(struct pov_rotor *last, pthread_t *arm_thread)
{
	struct pov_rotor *current;
	int ret;

	current = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
			&usart1_serial_port_ops, &usart2_serial_port_ops,
			&uart4_serial_port_ops, &uart5_serial_port_ops,
			&spi1_master, &spi1_master, hall_read_rotated_time);
	if (!current) {
		pr_err("pov: failed to allocate arm\n");
		return NULL;
	}

	serial_port_start_thread(current->ring_ccw_port, SERIAL_PORT_DF_OTHERS,
		NULL, last->ring_cw_port);
	serial_port_start_thread(last->ring_cw_port, SERIAL_PORT_DF_OTHERS,
		NULL, current->ring_ccw_port);

	arm_thread_switch = 1;
	ret = pthread_create(arm_thread, NULL, arm_thread_func, current);
	if (ret != 0) {
		arm_thread_switch = 0;
		pr_err("pov: failed to create timer thread: %d\n", ret);
		pov_rotor_free(current);
		return NULL;
	}

	return current;
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

int app_main(int argc, char **argv)
{
	struct pov_rotor *first, *last, *prev;
	struct pov_stator *stator = NULL;
	struct serial_port_ops *motor_ops;
#if KINETIS_FAKE_SIM
	pthread_t stator_thread;
	pthread_t phone_thread;
	pthread_t arm_threads[POV_ARM_COUNT];
	u8 arm_count = 0;
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
	first = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
			motor_ops, &usart6_serial_port_ops,
			&uart4_serial_port_ops, &uart5_serial_port_ops,
			&spi1_master, &spi6_master,
			hall_read_rotated_time);
	if (!first) {
		pr_err("pov: failed to allocate first\n");
		return -ENOMEM;
	}
	stm32_serial_port[1] = first->motor_port;
	stm32_serial_port[2] = first->app_port;
	stm32_serial_port[3] = first->ring_cw_port;
	stm32_serial_port[4] = first->ring_ccw_port;
#if POV_USE_MT6701
#else
	hall_dev = first->angle_sensor;
#endif

// 	first->wifi = wifi_init(WIFI_SSID, WIFI_PASSWORD, 1,
// 		WIFI_STATIC_IP, WIFI_STATIC_GW, WIFI_STATIC_NETMASK);
// 	if (first->wifi) {
// 	} else {
// 		return -ENOMEM;
// 	}
//
// 	ret = tim_task_add(&first->wifi->schedule_task, "wifi_poll", WIFI_POLL_INTERVAL_MS * 1000,
// 				true, false, wifi_task_cb);
// 	if (ret != 0)
// 		return ret;

#else
	u32 scenario = 0;
	u32 arg2 = 0;
	u32 arg3 = 0;
	char *arg_str = NULL;

	if (argc > 1)
		scenario = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		arg2 = simple_strtoul(argv[2], NULL, 10);
	if (argc > 3)
		arg3 = simple_strtoul(argv[3], NULL, 10);
	if (argc > 4)
		arg_str = argv[4];

	pr_info("pov: scenario=%u arg2=%u arg3=%u arg_str=%s\n",
		scenario, arg2, arg3, arg_str ? arg_str : "(null)");

	first = pov_rotor_alloc(&general_flash_ops, &general_bootloader_ops,
			&usart1_serial_port_ops, &usart2_serial_port_ops,
			&uart4_serial_port_ops, &uart5_serial_port_ops,
			&spi1_master, &spi1_master, hall_read_rotated_time);
	if (!first) {
		pr_err("pov: failed to allocate first\n");
		return -ENOMEM;
	}
	stator = pov_stator_alloc(&usart1_serial_port_ops,
			&general_flash_ops, &general_bootloader_ops,
			hall_read_rotated_time);
	if (!stator) {
		pr_err("pov: failed to allocate stator\n");
		pov_rotor_free(first);
		return -ENOMEM;
	}
	stator->hall->priv = first->angle_sensor;

	serial_port_start_thread(first->motor_port, SERIAL_PORT_DF_OTHERS,
		NULL, stator->motor_port);
	serial_port_start_thread(stator->motor_port, SERIAL_PORT_DF_OTHERS,
		NULL, first->motor_port);

	stator_thread_switch = 1;
	ret = pthread_create(&stator_thread, NULL, stator_thread_func, stator);
	if (ret != 0) {
		stator_thread_switch = 0;
		pr_err("pov: failed to create timer thread: %d\n", ret);
		goto out;
	}

	prev = first;
	for (int i = 1; i < POV_ARM_COUNT; i++) {
		last = create_arm(prev, &arm_threads[arm_count]);
		if (!last)
			goto out;
		arm_count++;
		prev->angle_sensor->priv = last->angle_sensor;
		prev = last;
	}

	pov_create_fake_images(APP_IMAGE_PATH);
	fatfs_create_dirs(PHONE_DOWNLOAD_DIR);
	phone_thread_switch = 1;
	ret = pthread_create(&phone_thread, NULL, phone_thread_func, first);
	if (ret != 0) {
		phone_thread_switch = 0;
		pr_err("pov: failed to create phone thread: %d\n", ret);
		goto out;
	}
	sleep(2);
#endif /* KINETIS_FAKE_SIM */

	ret = pov_rotor_display(first);

out:
#if KINETIS_FAKE_SIM
	if (phone_thread_switch) {
		phone_thread_switch = 0;
		pthread_join(phone_thread, NULL);
	}
	if (stator_thread_switch) {
		stator_thread_switch = 0;
		pthread_join(stator_thread, NULL);
	}
	arm_thread_switch = 0;
	for (int i = 0; i < arm_count; i++)
		pthread_join(arm_threads[i], NULL);
#endif
	pov_rotor_free(first);
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
