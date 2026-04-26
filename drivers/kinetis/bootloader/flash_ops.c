/**
 * @file flash_ops.c
 * @brief STM32F4 internal Flash operations implementation
 *
 * Implements sector query, erase (with IWDG feeding), 4-byte aligned write,
 * and APP validity check for STM32F4xx series (1MB Flash, 12 sectors).
 */

#include <kinetis/bootloader/flash_ops.h>
#include <linux/printk.h>
#include <string.h>

/* STM32 HAL headers - adjust include path as needed */
#ifdef STM32_HAL_LIBRARY
#include "stm32f4xx_hal.h"
#else
/* Stub types for compilation without HAL */
typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct {
	u32 TypeErase;
	u32 Sector;
	u32 NbSectors;
	u32 VoltageRange;
} FLASH_EraseInitTypeDef;
#define FLASH_TYPEERASE_SECTORS     0x00u
#define FLASH_VOLTAGE_RANGE_3       0x02u
#define FLASH_TYPEPROGRAM_WORD      0x02u
#endif

/*********************************************************************
 * STM32F4 Flash Sector Map (1MB device)
 *********************************************************************/

static const struct flash_sector_info g_flash_sectors[] = {
	{ 0,  0x08000000u, 16u * 1024u },      /* Sector 0:  16KB */
	{ 1,  0x08004000u, 16u * 1024u },      /* Sector 1:  16KB */
	{ 2,  0x08008000u, 16u * 1024u },      /* Sector 2:  16KB */
	{ 3,  0x0800C000u, 16u * 1024u },      /* Sector 3:  16KB */
	{ 4,  0x08010000u, 64u * 1024u },      /* Sector 4:  64KB  */
	{ 5,  0x08020000u, 128u * 1024u },     /* Sector 5:  128KB */
	{ 6,  0x08040000u, 128u * 1024u },     /* Sector 6:  128KB */
	{ 7,  0x08060000u, 128u * 1024u },     /* Sector 7:  128KB */
	{ 8,  0x08080000u, 128u * 1024u },     /* Sector 8:  128KB */
	{ 9,  0x080A0000u, 128u * 1024u },     /* Sector 9:  128KB */
	{ 10, 0x080C0000u, 128u * 1024u },     /* Sector 10: 128KB */
	{ 11, 0x080E0000u, 128u * 1024u },     /* Sector 11: 128KB */
};

#define FLASH_SECTOR_COUNT  (sizeof(g_flash_sectors) / sizeof(g_flash_sectors[0]))

/*********************************************************************
 * IWDG Feed (Watchdog)
 *********************************************************************/

/**
 * @brief Feed the Independent Watchdog during long operations
 * @note This function is called between sector erases to prevent reset.
 *       The user must implement IWDG_Refresh() according to their hardware setup.
 */
static void flash_ops_feed_iwdg(void)
{
#ifdef STM32_HAL_LIBRARY
	IWDG_Refresh(&hiwdg);
#endif
}

/*********************************************************************
 * Sector Operations
 *********************************************************************/

const struct flash_sector_info *flash_ops_get_sector(u32 addr)
{
	u32 i;

	if (addr < FLASH_BASE_ADDR || addr >= FLASH_BASE_ADDR + FLASH_TOTAL_SIZE)
		return NULL;

	for (i = 0; i < FLASH_SECTOR_COUNT; i++) {
		if (addr >= g_flash_sectors[i].base_addr &&
		    addr < g_flash_sectors[i].base_addr + g_flash_sectors[i].size)
			return &g_flash_sectors[i];
	}

	return NULL;
}

int flash_ops_get_sector_number(u32 addr)
{
	const struct flash_sector_info *info = flash_ops_get_sector(addr);

	if (!info)
		return -1;

	return info->sector;
}

/*********************************************************************
 * Erase Operations
 *********************************************************************/

int flash_ops_erase_sector(u8 sector)
{
#ifdef STM32_HAL_LIBRARY
	HAL_StatusTypeDef status;
	FLASH_EraseInitTypeDef erase_init;
	u32 sector_error = 0;

	if (sector > 11)
		return -1;

	erase_init.TypeErase     = FLASH_TYPEERASE_SECTORS;
	erase_init.Sector        = sector;
	erase_init.NbSectors     = 1;
	erase_init.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

	status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
	if (status != HAL_OK) {
		pr_err("Flash erase sector %d failed: status=%d, error=0x%08lx\n",
		       sector, status, sector_error);
		return -2;
	}
#else
	(void)sector;
#endif

	return 0;
}

int flash_ops_erase_range(u32 start_addr, u32 end_addr)
{
	const struct flash_sector_info *info;
	u32 addr;
	int ret;

	if (start_addr >= end_addr)
		return -1;

	/* Erase each sector that overlaps with [start_addr, end_addr) */
	for (addr = start_addr; addr < end_addr; ) {
		info = flash_ops_get_sector(addr);
		if (!info) {
			pr_err("Invalid flash address 0x%08lx in erase range\n", addr);
			return -2;
		}

		/* Skip sectors that are entirely before start_addr (shouldn't happen) */
		if (info->base_addr + info->size <= start_addr) {
			addr = info->base_addr + info->size;
			continue;
		}

		/* Skip sectors that are entirely after end_addr */
		if (info->base_addr >= end_addr)
			break;

		pr_debug("Erasing sector %d (0x%08lX - 0x%08lX)\n",
			 info->sector, info->base_addr,
			 info->base_addr + info->size - 1);

		ret = flash_ops_erase_sector(info->sector);
		if (ret < 0)
			return ret;

		/* Feed watchdog after each sector erase */
		flash_ops_feed_iwdg();

		/* Move to next sector */
		addr = info->base_addr + info->size;
	}

	return 0;
}

int flash_ops_erase_app_region(void)
{
	pr_info("Erasing APP region (0x%08lX - 0x%08lX)\n",
		APP_BASE_ADDR, CONFIG_BASE_ADDR - 1);

	return flash_ops_erase_range(APP_BASE_ADDR, CONFIG_BASE_ADDR);
}

int flash_ops_erase_config_region(void)
{
	pr_info("Erasing config region (0x%08lX - 0x%08lX)\n",
		CONFIG_BASE_ADDR, CONFIG_BASE_ADDR + CONFIG_SIZE - 1);

	return flash_ops_erase_range(CONFIG_BASE_ADDR,
				     CONFIG_BASE_ADDR + CONFIG_SIZE);
}

/*********************************************************************
 * Write Operations
 *********************************************************************/

int flash_ops_write(u32 addr, const u8 *data, u32 len)
{
#ifdef STM32_HAL_LIBRARY
	HAL_StatusTypeDef status;
	u32 i;
	u32 aligned_len;
	u32 word;
	u8 padded[4];

	if (!data || len == 0)
		return -1;

	/* Address must be 4-byte aligned */
	if (addr % 4 != 0) {
		pr_err("Flash write address 0x%08lx not 4-byte aligned\n", addr);
		return -2;
	}

	/* Calculate 4-byte aligned length */
	aligned_len = (len + 3u) & ~3u;

	/* Write each 32-bit word */
	for (i = 0; i < aligned_len; i += 4) {
		if (i + 4 <= len) {
			/* Full word from data */
			word = *(u32 *)&data[i];
		} else {
			/* Partial word: pad with 0xFF */
			memset(padded, 0xFF, 4);
			memcpy(padded, &data[i], len - i);
			word = *(u32 *)padded;
		}

		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
		if (status != HAL_OK) {
			pr_err("Flash write failed at 0x%08lx: status=%d\n",
			       addr + i, status);
			return -3;
		}
	}
#else
	(void)addr;
	(void)data;
	(void)len;
#endif

	return 0;
}

/*********************************************************************
 * APP Validity Check
 *********************************************************************/

int flash_ops_is_app_valid(void)
{
	u32 stack_top;
	u32 reset_handler;

	/* Read stack top (first word of APP vector table) */
	stack_top = *(volatile u32 *)APP_BASE_ADDR;

	/* Check: stack top must be in SRAM range */
	if (stack_top < SRAM_BASE_ADDR || stack_top > SRAM_END_ADDR) {
		pr_debug("APP invalid: stack top 0x%08lx not in SRAM range\n",
			 stack_top);
		return 0;
	}

	/* Read reset handler (second word of APP vector table) */
	reset_handler = *(volatile u32 *)(APP_BASE_ADDR + 4);

	/* Check: reset handler must be in Flash range */
	if (reset_handler < FLASH_BASE_ADDR ||
	    reset_handler >= FLASH_BASE_ADDR + FLASH_TOTAL_SIZE) {
		pr_debug("APP invalid: reset handler 0x%08lx not in Flash range\n",
			 reset_handler);
		return 0;
	}

	return 1;
}

/*********************************************************************
 * Flash Lock/Unlock
 *********************************************************************/

int flash_ops_unlock(void)
{
#ifdef STM32_HAL_LIBRARY
	HAL_StatusTypeDef status;

	status = HAL_FLASH_Unlock();
	if (status != HAL_OK) {
		pr_err("Flash unlock failed: %d\n", status);
		return -1;
	}
#endif

	return 0;
}

void flash_ops_lock(void)
{
#ifdef STM32_HAL_LIBRARY
	HAL_FLASH_Lock();
#endif
}
