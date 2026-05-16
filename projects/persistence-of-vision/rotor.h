#ifndef POV_ROTOR_H
#define POV_ROTOR_H

#include <linux/types.h>

#include "kinetis/fatfs.h"
#include <kinetis/mavlink.h>
#include <kinetis/led.h>

#include "../fs/fatfs/ff.h"

#include "config.h"
#include "hall.h"

struct flash_ops;

#define POV_IMG_MAGIC       0x504F5621  /* "POV!" */
#define POV_IMG_VERSION     1

enum pov_fsm_state {
	POV_STATE_WAIT_MOTOR = 0,
	POV_STATE_WAIT_PHONE,
	POV_STATE_SEND_SPEED,
	POV_STATE_DISPLAYING,
	POV_STATE_CHECK_CMD,
	POV_STATE_ERROR
};

struct pov_image_header {
	u32 magic;               /* Magic number: POV_IMG_MAGIC */
	u16 version;             /* Format version */
	u16 columns;             /* Number of columns (should be 720) */
	u16 leds_per_group;      /* LEDs per SPI group (64) */
	u8 spi_groups;           /* Number of SPI groups (4) */
	u8 reserved[5];          /* Reserved for future use */
};

#define POV_MAX_IMAGE_COUNT     8

struct pov_rotor {
	enum pov_fsm_state state;

	FIL image_file;
	char *image_names[POV_MAX_IMAGE_COUNT];
	u8 image_count;
	u8 images_open;
	struct pov_image_header image_header;
	u32 pending_column;

	u32 angle;

	struct mavlink_device *motor_mavlink;     /* ↔ stator (motor control) */
	struct mavlink_device *app_mavlink; /* ↔ phone  (file transfer, image cmds) */
	struct hall_device *hall;
	u8 image_current;        /* Currently playing image index */

	struct serial_port *motor_port;  /* ↔ stator (MAVLink) */
	struct serial_port *app_port;    /* ↔ phone  (ZMODEM) */

	struct flash_ops *flash;

	struct rgb_tricolor rgb_strip[POV_SPI_GROUPS][POV_LEDS_PER_GROUP];
};

int pov_create_fake_images(const char *path);

/**
 * @brief Allocate and initialize rotor device
 * @param flash: Flash operations for persisting bootloader config
 * @return Pointer to rotor instance, or NULL on failure
 */
struct pov_rotor *pov_rotor_alloc(struct flash_ops *flash);

/**
 * @brief Free rotor instance and all owned resources
 * @param rotor: Rotor instance to free (NULL-safe)
 */
void pov_rotor_free(struct pov_rotor *rotor);

/**
 * @brief Main rotor display loop (state machine)
 * @param rotor: Rotor instance
 * @return 0 on success, negative error code on failure
 */
int pov_rotor_display(struct pov_rotor *rotor);

#endif
