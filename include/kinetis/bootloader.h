#ifndef BOOTLOADER_BOOTLOADER_H
#define BOOTLOADER_BOOTLOADER_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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

struct flash_ops {
	int (*erase)(u64 addr, u32 size);
	int (*write)(u64 addr, const u8 *data, u32 size);
	int (*read)(u64 addr, u8 *buf, u32 size);
};

struct bootloader_ops {
	void (*disable_irq)(void);
	void (*cleanup_hw_resource)(void);
	void (*disable_cache)(void);
	void (*set_stack_pointer)(u64 pointer);
	void (*redirect_isr_table)(u64 address);
	void (*system_reset)(void);
	struct flash_ops *flash;
};

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

struct mavlink_device;
struct serial_port;

struct bootloader_mem_layout {
	u8  *rom_base;
	u32  rom_size;
	u32  bootloader_size;
	u32  firmware_max_size;
	struct {
		u8  *start;          /* SRAM start (inclusive)           */
		u8  *end;            /* SRAM end   (exclusive)          */
	} sram;
};

struct bootloader_context {
	u8 *rom_base;
	u8 *config_addr;
	u8 *firmware_addr;
	struct bootloader_mem_layout layout;
	struct bootloader_config config;

	struct mavlink_device *app_mavlink;
	struct serial_port *app_port;

	struct bootloader_ops *ops;

	enum bootloader_fsm_state state;
	u8 retry_count;          /* Number of consecutive ERROR -> WAIT_CMD cycles */
};

#ifndef ROM_BASE_ADDR
#define ROM_BASE_ADDR            0x08000000u
#endif
#ifndef ROM_TOTAL_SIZE
#define ROM_TOTAL_SIZE           (1024u * 1024u)
#endif

#define BOOTLOADER_BASE_ADDR     ROM_BASE_ADDR
#define BOOTLOADER_SIZE          (64u * 1024u)

#define CONFIG_BASE_ADDR         (BOOTLOADER_BASE_ADDR + BOOTLOADER_SIZE)
#define CONFIG_SIZE              sizeof(struct bootloader_config)

#define FIRMWARE_BASE_ADDR       (CONFIG_BASE_ADDR + CONFIG_SIZE)
#define FIRMWARE_MAX_SIZE        (ROM_TOTAL_SIZE - BOOTLOADER_SIZE - CONFIG_SIZE)

/* Magic number for config region validation */
#define BOOTLOADER_MAGIC         0x4B4C4652u   /* "KLFR" */

/* Firmware path on FatFS */
#define BL_FIRMWARE_PATH         "0:/firmware"
#define BL_BACKUP_PATH         	 "0:/backup"
#define BL_COMPUTER_PATH		 "0:/computer"
#define BL_FIRMWARE_NAME    	 "firmware.bin"

/* Flash read/write chunk size (heap allocated) */
#define BL_FLASH_CHUNK           4096

/**
 * @brief Initialize bootloader context
 * @param layout:   Memory layout descriptor (chip-specific flash/SRAM geometry)
 * @param chip_ops: Platform chip operations (must not be NULL)
 * @param flash_ops: Flash read/write/erase operations (must not be NULL)
 * @param serial_ops: Serial port operations (must not be NULL)
 * @return Bootloader context pointer on success, NULL on failure
 * @note The layout structure is copied internally; caller can free its copy
 *       after return.  layout.bootloader_size must be at least large enough
 *       to hold the bootloader code itself and leave room for the config region.
 */
struct bootloader_context *bootloader_init(const struct bootloader_mem_layout *layout,
	struct bootloader_ops *chip_ops,
	struct flash_ops *flash_ops,
	struct serial_port_ops *serial_ops);

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
int bootloader_flow(struct bootloader_context *bl_ctx);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_BOOTLOADER_H */
