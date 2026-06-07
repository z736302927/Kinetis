#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H

/*********************************************************************
 * Flash Sector Sizes
 *
 * H7: Uniform 128KB sectors (bank 1, sectors 0-7)
 *     Program via 256-bit FLASHWORD (32 bytes)
 * F4: Mixed: 4×16KB + 1×64KB + 7×128KB = 1MB
 *     Program via 32-bit word
 *********************************************************************/
#if defined(STM32H743xx)
#define BL_FLASH_SECTOR_SIZE     0x20000u  /* 128KB per sector */
#define BL_FLASH_WORD_SIZE       32u       /* 256-bit FLASHWORD */
#define BL_FLASH_VOLTAGE_RANGE   FLASH_VOLTAGE_RANGE_3
#define BL_FLASH_TYPEPROGRAM     FLASH_TYPEPROGRAM_FLASHWORD
#elif defined(STM32F407xx)
#define BL_FLASH_SECTOR_SIZE_REF 0x20000u  /* default sector size reference */
#define BL_FLASH_WORD_SIZE       4u        /* 32-bit word */
#define BL_FLASH_VOLTAGE_RANGE   FLASH_VOLTAGE_RANGE_3
#define BL_FLASH_TYPEPROGRAM     FLASH_TYPEPROGRAM_WORD
#define BL_F4_SECTOR_COUNT       12
/* Defined in interface.c: const u32 bl_f4_sector_size[12] */
extern const u32 bl_f4_sector_size[BL_F4_SECTOR_COUNT];
#else
#define BL_FLASH_SECTOR_SIZE     0x20000u
#define BL_FLASH_WORD_SIZE       32u
#define BL_FLASH_VOLTAGE_RANGE   FLASH_VOLTAGE_RANGE_3
#define BL_FLASH_TYPEPROGRAM     FLASH_TYPEPROGRAM_FLASHWORD
#endif

#endif /* BOOTLOADER_CONFIG_H */
