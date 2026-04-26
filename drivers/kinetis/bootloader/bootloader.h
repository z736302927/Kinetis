/**
 * @file bootloader.h
 * @brief Bootloader core interface for STM32F4 OTA firmware update
 *
 * Defines address macros, boot mode enumerations, update state machine,
 * config region structure, and the main Bootloader API.
 */

#ifndef BOOTLOADER_BOOTLOADER_H
#define BOOTLOADER_BOOTLOADER_H

#include <linux/types.h>
#include <kinetis/bootloader/flash_ops.h>
#include <kinetis/bootloader/protocol_adapter.h>

/*********************************************************************
 * Boot Mode Enumeration
 *********************************************************************/

/** Boot mode values stored in config region */
enum boot_mode {
	BOOT_MODE_RUN       = 0x00,   /**< Normal boot: jump to APP */
	BOOT_MODE_UPDATE    = 0x01,   /**< Firmware update: receive new firmware */
	BOOT_MODE_RECOVERY  = 0x02,   /**< Recovery: APP is invalid, wait for download */
};

/*********************************************************************
 * Update State Machine
 *********************************************************************/

/** Update process states */
enum update_state {
	UPDATE_STATE_IDLE             = 0,   /**< Idle, waiting for command */
	UPDATE_STATE_WAIT_PROTOCOL    = 1,   /**< Waiting for protocol connection */
	UPDATE_STATE_ERASING          = 2,   /**< Erasing APP Flash region */
	UPDATE_STATE_RECEIVING        = 3,   /**< Receiving firmware data */
	UPDATE_STATE_VERIFYING        = 4,   /**< CRC32 verification in progress */
	UPDATE_STATE_COMPLETE         = 5,   /**< Update complete, preparing reset */
	UPDATE_STATE_ERROR            = 6,   /**< Update failed */
};

/*********************************************************************
 * Config Region Structure
 *********************************************************************/

/** Magic number for config region validation */
#define BOOT_CONFIG_MAGIC   0x4B4C4652u   /* "KLFR" */

/** Config region stored at CONFIG_BASE_ADDR */
struct boot_config {
	u32 magic;              /**< Magic number: must be BOOT_CONFIG_MAGIC */
	u32 version;            /**< Config structure version (currently 1) */
	u8  boot_mode;          /**< Current boot mode (enum boot_mode) */
	u8  reserved1[3];       /**< Padding for alignment */
	u32 firmware_size;      /**< Current firmware size in bytes */
	u32 firmware_crc32;     /**< Current firmware CRC32 */
	u8  firmware_ver_major; /**< Firmware version major */
	u8  firmware_ver_minor; /**< Firmware version minor */
	u16 firmware_ver_patch; /**< Firmware version patch */
	u32 update_count;       /**< Total number of successful updates */
	u32 last_update_time;   /**< Timestamp of last update (if available) */
	u8  reserved2[32];      /**< Reserved for future use */
};

/** Size of boot_config structure must fit in one Flash write */
#define BOOT_CONFIG_SIZE    sizeof(struct boot_config)

/*********************************************************************
 * Firmware Header (first block format)
 *********************************************************************/

/** Firmware header: first 8 bytes of the first data block */
struct firmware_header {
	u32 total_size;         /**< Total firmware size in bytes */
	u32 crc32;              /**< Expected CRC32 of the entire firmware */
};

#define FIRMWARE_HEADER_SIZE  sizeof(struct firmware_header)

/*********************************************************************
 * Update Statistics (runtime, not stored in Flash)
 *********************************************************************/

struct update_stats {
	u32 bytes_received;     /**< Total bytes received so far */
	u32 bytes_written;      /**< Total bytes written to Flash */
	u32 blocks_received;    /**< Number of data blocks received */
	u8  progress;           /**< Current progress percentage (0-100) */
	u32 expected_crc32;     /**< CRC32 from firmware header */
	u32 firmware_size;      /**< Firmware size from firmware header */
	u32 write_offset;       /**< Current Flash write offset from APP_BASE_ADDR */
	u8  header_parsed;      /**< Whether firmware header has been parsed */
};

/*********************************************************************
 * Bootloader API
 *********************************************************************/

/**
 * @brief Initialize Bootloader: read config, determine boot mode
 * @return 0 on success, negative on error
 */
int bootloader_init(void);

/**
 * @brief Main Bootloader entry point (runs the state machine)
 * @note This function does not return in normal operation.
 *       It either jumps to APP or enters update loop.
 */
void bootloader_main(void) __attribute__((noreturn));

/**
 * @brief Jump to the APP firmware
 * @note This function does not return on success.
 *       Disables interrupts, sets MSP, remaps VTOR, and jumps.
 */
void bootloader_jump_to_app(void) __attribute__((noreturn));

/**
 * @brief Handle a protocol data event during update
 * @param data: Pointer to received data block
 * @param len: Length of data block in bytes
 * @return 0 on success, negative on error
 * @note First block is expected to contain firmware_header (8 bytes) + payload.
 *       Subsequent blocks are written directly to Flash.
 */
int bootloader_on_data_received(const u8 *data, u32 len);

/**
 * @brief Get current update state
 * @return Current update_state enum value
 */
enum update_state bootloader_get_state(void);

/**
 * @brief Get current update statistics
 * @return Pointer to update_stats structure
 */
const struct update_stats *bootloader_get_stats(void);

/**
 * @brief Read boot config from Flash config region
 * @param config: Output config structure
 * @return 0 on success, negative on error
 */
int bootloader_read_config(struct boot_config *config);

/**
 * @brief Write boot config to Flash config region
 * @param config: Config structure to write
 * @return 0 on success, negative on error
 */
int bootloader_write_config(const struct boot_config *config);

/**
 * @brief Set boot mode in config region
 * @param mode: Boot mode to set
 * @return 0 on success, negative on error
 */
int bootloader_set_boot_mode(u8 mode);

/**
 * @brief Software reset (NVIC_SystemReset)
 * @note This function does not return
 */
void bootloader_reset(void) __attribute__((noreturn));

#endif /* BOOTLOADER_BOOTLOADER_H */
