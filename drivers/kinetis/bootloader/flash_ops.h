/**
 * @file flash_ops.h
 * @brief STM32F4 internal Flash operations for Bootloader
 *
 * Provides sector query, erase (with IWDG feeding), 4-byte aligned write,
 * and APP validity check for STM32F4xx series (1MB Flash).
 */

#ifndef BOOTLOADER_FLASH_OPS_H
#define BOOTLOADER_FLASH_OPS_H

#include <linux/types.h>

/*********************************************************************
 * Flash Address Map
 *********************************************************************/

#define FLASH_BASE_ADDR            0x08000000u
#define FLASH_TOTAL_SIZE           (1024u * 1024u)   /* 1MB */

#define BOOTLOADER_BASE_ADDR      0x08000000u
#define BOOTLOADER_SIZE           (64u * 1024u)      /* 64KB */

#define APP_BASE_ADDR             0x08010000u
#define APP_MAX_SIZE              (896u * 1024u)     /* 896KB */

#define CONFIG_BASE_ADDR          0x080F0000u
#define CONFIG_SIZE               (64u * 1024u)      /* 64KB */

/*********************************************************************
 * SRAM Address Range (for APP validity check)
 *********************************************************************/

#define SRAM_BASE_ADDR            0x20000000u
#define SRAM_END_ADDR             0x20030000u        /* 192KB SRAM */

/*********************************************************************
 * STM32F4 Flash Sector Definition
 *********************************************************************/

/** Sector information structure */
struct flash_sector_info {
	u8  sector;        /**< HAL sector number */
	u32 base_addr;     /**< Sector start address */
	u32 size;          /**< Sector size in bytes */
};

/**
 * @brief Get sector information for a given Flash address
 * @param addr: Flash address
 * @return Pointer to sector info, or NULL if address is invalid
 */
const struct flash_sector_info *flash_ops_get_sector(u32 addr);

/**
 * @brief Get the sector number for a given Flash address (HAL format)
 * @param addr: Flash address
 * @return Sector number (0-11), or -1 if invalid
 */
int flash_ops_get_sector_number(u32 addr);

/**
 * @brief Erase a single Flash sector
 * @param sector: HAL sector number (0-11)
 * @return 0 on success, negative on error
 * @note Feeds IWDG during erase operation
 */
int flash_ops_erase_sector(u8 sector);

/**
 * @brief Erase all Flash sectors in the specified address range
 * @param start_addr: Start address (inclusive)
 * @param end_addr: End address (exclusive)
 * @return 0 on success, negative on error
 * @note Feeds IWDG between sector erases
 */
int flash_ops_erase_range(u32 start_addr, u32 end_addr);

/**
 * @brief Erase the entire APP Flash region
 * @return 0 on success, negative on error
 * @note Erases sectors from APP_BASE_ADDR to CONFIG_BASE_ADDR - 1
 */
int flash_ops_erase_app_region(void);

/**
 * @brief Erase the config Flash region
 * @return 0 on success, negative on error
 */
int flash_ops_erase_config_region(void);

/**
 * @brief Write data to Flash with 4-byte alignment
 * @param addr: Destination Flash address (must be 4-byte aligned)
 * @param data: Source data pointer
 * @param len: Data length in bytes
 * @return 0 on success, negative on error
 * @note If len is not 4-byte aligned, trailing bytes are padded with 0xFF.
 *       Flash must be unlocked before calling this function.
 */
int flash_ops_write(u32 addr, const u8 *data, u32 len);

/**
 * @brief Check if APP region contains a valid firmware image
 * @return 1 if valid, 0 if invalid
 * @note Checks: stack top must be in SRAM range, reset vector must be in Flash range
 */
int flash_ops_is_app_valid(void);

/**
 * @brief Unlock Flash for write/erase operations
 * @return 0 on success, negative on error
 */
int flash_ops_unlock(void);

/**
 * @brief Lock Flash after write/erase operations
 */
void flash_ops_lock(void);

#endif /* BOOTLOADER_FLASH_OPS_H */
