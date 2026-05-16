#ifndef BOOTLOADER_BOOTLOADER_H
#define BOOTLOADER_BOOTLOADER_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * Bootloader Address Layout
 *********************************************************************/

#define ROM_BASE_ADDR            0x08000000u
#define ROM_TOTAL_SIZE           (1024u * 1024u)

#define CONFIG_BASE_ADDR         0x080F0000u
#define CONFIG_SIZE              (64u * 1024u)

#define BOOTLOADER_BASE_ADDR     (CONFIG_BASE_ADDR + CONFIG_SIZE)
#define BOOTLOADER_SIZE          (64u * 1024u)

#define FIRMWARE_BASE_ADDR       (BOOTLOADER_BASE_ADDR + BOOTLOADER_SIZE)
#define FIRMWARE_MAX_SIZE        (896u * 1024u)

/*********************************************************************
 * Constants
 *********************************************************************/

/* Magic number for config region validation */
#define BOOTLOADER_MAGIC         0x4B4C4652u   /* "KLFR" */

/* Firmware path on FatFS */
#define BL_FIRMWARE_PATH         "0:/firmware"

/* Flash read/write chunk size (heap allocated) */
#define BL_FLASH_CHUNK           4096

/*********************************************************************
 * Enumerations
 *********************************************************************/

enum bootloader_mode {
	BL_MODE_RUN       = 0x00,
	BL_MODE_UPDATE    = 0x01,
	BL_MODE_RECOVERY  = 0x02,
};

enum bootloader_fsm_state {
	BL_STATE_INIT       = 0,
	BL_STATE_CHECK_MODE,
	BL_STATE_JUMP_APP,
	BL_STATE_WAIT_CMD,
	BL_STATE_RECEIVE,
	BL_STATE_FLASH,
	BL_STATE_VERIFY,
	BL_STATE_COMPLETE,
	BL_STATE_ERROR
};

/*********************************************************************
 * Flash Operations (shared by bootloader and application)
 *********************************************************************/

struct flash_ops {
	int (*erase)(u32 addr, u32 size);
	int (*write)(u32 addr, const u8 *data, u32 size);
	int (*read)(u32 addr, u8 *buf, u32 size);
};

/*********************************************************************
 * Bootloader Platform Operations (function pointer table)
 *********************************************************************/

struct bootloader_ops {
	void (*disable_irq)(void);
	void (*cleanup_hw_resource)(void);
	void (*disable_cache)(void);
	void (*set_stack_pointer)(u32 pointer);
	void (*redirect_isr_table)(u32 address);
	struct flash_ops *flash;
	void (*system_reset)(void);
};

/*********************************************************************
 * Bootloader Config Structure (stored in flash)
 *********************************************************************/

struct bootloader_config {
	u32 magic;              /* Magic number: must be BOOTLOADER_MAGIC */
	u32 version;            /* Config structure version (currently 1) */
	u8  mode;               /* Current bootloader mode (enum bootloader_mode) */
	u8  reserved1[3];       /* Padding for alignment */
	u32 firmware_size;      /* Current firmware size in bytes */
	u32 firmware_crc32;     /* Current firmware CRC32 */
	u8  firmware_ver_major; /* Firmware version major */
	u8  firmware_ver_minor; /* Firmware version minor */
	u16 firmware_ver_patch; /* Firmware version patch */
	u32 update_count;       /* Total number of successful updates */
	u32 last_update_time;   /* Timestamp of last update (if available) */
	u8  reserved2[32];      /* Reserved for future use */
} __packed;

/*********************************************************************
 * Bootloader Context
 *********************************************************************/

struct mavlink_device;
struct serial_port;

struct bootloader_context {
	struct bootloader_config config;

	struct mavlink_device *app_mavlink;
	struct serial_port *app_port;

	struct bootloader_ops *ops;

	u8 *config_addr;

	enum bootloader_fsm_state state;
	u8 retry_count;          /* Number of consecutive ERROR -> WAIT_CMD cycles */
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize bootloader context
 * @param config_addr: Pointer to bootloader config region in flash
 * @param ops: Platform operations structure (must not be NULL)
 * @return Bootloader context pointer on success, NULL on failure
 */
struct bootloader_context *bootloader_init(u8 *config_addr,
					    struct bootloader_ops *ops);

/**
 * @brief Free bootloader context and all owned resources
 * @param bl_ctx: Bootloader context to free (NULL-safe)
 */
void bootloader_free(struct bootloader_context *bl_ctx);

/**
 * @brief Main bootloader entry and FSM loop
 * @return 0 on success (never returns normally in production),
 *         negative error code on failure after max retries
 */
int bootloader_flow(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_BOOTLOADER_H */
