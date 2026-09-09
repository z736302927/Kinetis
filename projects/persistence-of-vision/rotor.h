#ifndef POV_ROTOR_H
#define POV_ROTOR_H

#include <linux/types.h>

#include <kinetis/fatfs.h>
#include <kinetis/mavlink.h>
#include <kinetis/led.h>
#include <kinetis/bootloader.h>
#include <kinetis/tlc5971.h>
// #include <kinetis/w25qxxx.h>
#include <kinetis/w25qxx_port.h>

#include "fatfs/ff.h"

#if KINETIS_FAKE_SIM
#include "lrzsz/zmodem.h"
#endif

// #include "wifi/wifi.h"

#include "config.h"
#include "hall.h"

struct flash_ops;

#define POV_IMG_MAGIC       0x504F5621  /* "POV!" */
#define POV_IMG_VERSION     2

#define POV_PAYLOAD_RAW_RGB		1
#define POV_PAYLOAD_LED_DRIVER		2

enum pov_fsm_state {
	POV_STATE_WAIT_MOTOR = 0,
	POV_STATE_WAIT_PHONE,
	POV_STATE_SEND_SPEED,
	POV_STATE_WAIT_SPEED,
	POV_STATE_DISPLAYING,
	POV_STATE_CHECK_CMD,
	POV_STATE_CHECK_RING,
	POV_STATE_STATOR_UPDATE,
	POV_STATE_ROTOR_UPDATE,
	POV_STATE_ERROR
};

struct pov_image_header {
	u32 magic;               /* Magic number: POV_IMG_MAGIC */
	u16 version;             /* Format version */
	u16 columns;             /* Number of columns (should be 720) */
	u16 leds_per_group;      /* LEDs per SPI group (64) */
	u8 spi_groups;           /* Number of SPI groups (4) */
	u8 payload_format;       /* 1=raw RGB, 2=LED driver format */
	u16 bytes_per_col;       /* payload stride after header (12 for raw RGB) */
	u8 reserved[2];          /* Reserved for future use */
};

static inline void pov_image_header_init(struct pov_image_header *header,
	u8 payload_format, u16 bytes_per_col,
	u16 columns)
{
	header->magic = POV_IMG_MAGIC;
	header->version = POV_IMG_VERSION;
	header->columns = columns;
	header->leds_per_group = POV_LEDS_PER_GROUP;
	header->spi_groups = POV_SPI_GROUPS;
	header->payload_format = payload_format;
	header->bytes_per_col = bytes_per_col;
	header->reserved[0] = 0;
	header->reserved[1] = 0;
}

#define POV_MAX_IMAGE_COUNT     8

enum pov_fault_type {
	POV_FAULT_NONE = 0,
	POV_FAULT_STORAGE_WRITE,
	POV_FAULT_STORAGE_READ_ONLY,
	POV_FAULT_STAGED_CORRUPT,
	POV_FAULT_FLASH_ERASE,
	POV_FAULT_FLASH_WRITE,
	POV_FAULT_FLASH_READ,
	POV_FAULT_RESET_AFTER_FLAG,
};

struct pov_rotor {
	u8 led_driver_frame[2][POV_PREFETCH_COLS][POV_SPI_GROUPS][POV_LED_DRIVER_GROUP_BYTES];

	enum pov_fsm_state state;

	FIL image_file;
	char *image_names[POV_MAX_IMAGE_COUNT];
	u8 image_count;
	u8 images_open;
	struct pov_image_header image_header;
	u32 pending_column;

	u32 angle;

	u32 downtime_s;
	u8 speed_stable_samples;

	struct mavlink_device *motor_mavlink;     /* ↔ stator (motor control) */
	struct mavlink_device *app_mavlink; /* ↔ phone  (file transfer, image cmds) */
#if POV_USE_MT6701
	struct mt6701_device *angle_sensor;
#else
	struct hall_device *angle_sensor;
#endif
	u8 image_current;        /* Currently playing image index */

	struct serial_port *motor_port;  /* ↔ stator (MAVLink) */
	struct serial_port *app_port;    /* ↔ phone  (ZMODEM) */
	struct serial_port *ring_cw_port;
	struct serial_port *ring_ccw_port;

	struct flash_ops *flash;
	struct bootloader_ops *bl_ops;

// 	struct w25qxxx_device *w25qxxx;
	w25qxx_handle_t w25qxxx;

// 	struct wifi_manager *wifi;

	u64 last_recv_stator_heartbeat;    /* Last stator heartbeat timestamp */
	u64 last_send_stator_heartbeat;      /* Last time we sent a heartbeat (rate-limit) */

	volatile u32 fsm_error_count;

	bool ready;

	u8 prefetch_ping;
	u8 prefetch_sent_index;

	u8 arm_id;
	char image_path[32];
	char firmware_path[48];
	u32 installed_firmware_size;
	u32 installed_firmware_crc32;
	u32 firmware_boot_count;
#if KINETIS_FAKE_SIM
	u8 *sim_flash;
	u32 sim_flash_size;
	struct lrzsz_transfer_control zm_xfer_ctrl;
	volatile u32 fake_reboot_count;
	volatile u8 forced_offline;
	volatile u8 paused;
	volatile u8 firmware_fault;
	volatile u32 firmware_fault_injected;
	volatile u8 storage_fault;
	volatile u32 storage_fault_injected;
#endif
	u16 ring_next_seq;
	struct mavlink_device *ring_cw_mavlink;
	struct mavlink_device *ring_ccw_mavlink;
	struct mavlink_image_cmd image_cmd;
	u8 image_cmd_delay_frames;
};

int pov_create_fake_images(const char *path);

void pov_fake_firmware_fault_set(struct pov_rotor *rotor,
	enum pov_fault_type fault);

struct pov_rotor *pov_rotor_alloc(struct flash_ops *flash, struct bootloader_ops *bl_ops,
	struct serial_port_ops *motor_serial_ops, struct serial_port_ops *app_serial_ops,
	struct serial_port_ops *ring_cw_serial_ops, struct serial_port_ops *ring_ccw_serial_ops,
	struct spi_master *w25q_spi, struct spi_master *mt6701_spi,
	u32(*read_rotated_time)(struct hall_device *dev));

void pov_rotor_free(struct pov_rotor *rotor);

int pov_rotor_display(struct pov_rotor *rotor, volatile u8 *running);

#endif
