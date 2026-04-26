/**
 * @file host_flash_fs.c
 * @brief QSPI Flash FatFS interface implementation
 * @note Reads image data from QSPI Flash via FatFS, supporting
 *       sequential column access with seek optimization
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "host_flash_fs.h"

#if MCU_PLATFORM_STM32
#include <kinetis/fatfs.h>
#include "../fs/fatfs/ff.h"
#endif

/*********************************************************************
 * Initialization
 *********************************************************************/

int host_flash_fs_init(struct host_flash_fs_device *dev)
{
	if (!dev)
		return -EINVAL;

	memset(dev, 0, sizeof(*dev));

#if MCU_PLATFORM_STM32
	/*
	 * Initialize QSPI Flash in memory-mapped mode:
	 * - Configure QSPI peripheral (MT25TL01G or W25Q256)
	 * - Mount FatFS on QSPI Flash
	 *
	 * HAL calls:
	 *   MX_QUADSPI_Init();
	 *   BSP_QSPI_EnableMemoryMappedMode();
	 *   fatfs_init();
	 */
	int ret = fatfs_init();
	if (ret < 0) {
		pr_err("Host Flash FS: FatFS init failed: %d\n", ret);
		return ret;
	}

	pr_info("Host Flash FS: QSPI Flash + FatFS initialized\n");
#else
	pr_info("Host Flash FS: initialized in simulation mode\n");
#endif

	dev->initialized = 1;
	return 0;
}

/*********************************************************************
 * Image Loading
 *********************************************************************/

int host_flash_fs_load_image(struct host_flash_fs_device *dev,
	const char *filename)
{
	if (!dev || !dev->initialized)
		return -EINVAL;

	if (!filename)
		return -EINVAL;

#if MCU_PLATFORM_STM32
	FRESULT res;
	UINT br;

	dev->file = kzalloc(sizeof(FIL), GFP_KERNEL);
	if (!dev->file)
		return -ENOMEM;

	/* Open image file */
	res = f_open(dev->file, filename, FA_READ);
	if (res != FR_OK) {
		pr_err("Host Flash FS: failed to open '%s': %d\n", filename, res);
		kfree(dev->file);
		dev->file = NULL;
		return -EIO;
	}

	/* Read header */
	res = f_read(dev->file, &dev->header, sizeof(dev->header), &br);
	if (res != FR_OK || br < sizeof(dev->header)) {
		pr_err("Host Flash FS: failed to read header: %d\n", res);
		f_close(dev->file);
		kfree(dev->file);
		dev->file = NULL;
		return -EIO;
	}

	/* Validate header */
	if (dev->header.magic != POV_IMG_MAGIC) {
		pr_err("Host Flash FS: invalid magic: 0x%08x\n", dev->header.magic);
		f_close(dev->file);
		kfree(dev->file);
		dev->file = NULL;
		return -EINVAL;
	}

	if (dev->header.columns != POV_DISPLAY_COLS ||
		dev->header.spi_groups != POV_SPI_GROUPS) {
		pr_err("Host Flash FS: image format mismatch (cols=%d, groups=%d)\n",
			dev->header.columns, dev->header.spi_groups);
		f_close(dev->file);
		kfree(dev->file);
		dev->file = NULL;
		return -EINVAL;
	}

	dev->file_open = 1;
	dev->cached_col = 0xFFFF;  /* No cached column yet */

	pr_info("Host Flash FS: loaded '%s' (%d cols, %d groups)\n",
		filename, dev->header.columns, dev->header.spi_groups);
#else
	/* Simulation: create fake header */
	dev->header.magic = POV_IMG_MAGIC;
	dev->header.version = POV_IMG_VERSION;
	dev->header.columns = POV_DISPLAY_COLS;
	dev->header.leds_per_group = POV_LEDS_PER_GROUP;
	dev->header.spi_groups = POV_SPI_GROUPS;
	dev->file_open = 1;
	dev->cached_col = 0xFFFF;

	pr_info("Host Flash FS: simulated image loaded (%d cols)\n",
		dev->header.columns);
#endif

	return 0;
}

/*********************************************************************
 * Column Reading
 *********************************************************************/

int host_flash_fs_read_column(struct host_flash_fs_device *dev,
	u16 column, u8 *data)
{
	if (!dev || !dev->initialized || !dev->file_open)
		return -EINVAL;

	if (!data || column >= POV_DISPLAY_COLS)
		return -EINVAL;

#if MCU_PLATFORM_STM32
	FRESULT res;
	UINT br;

	/* Seek to column offset: header + column * bytes_per_col */
	u32 offset = sizeof(struct pov_image_header) +
		(u32)column * POV_BYTES_PER_COL;

	res = f_lseek(dev->file, offset);
	if (res != FR_OK) {
		pr_err("Host Flash FS: seek to col %d failed: %d\n", column, res);
		return -EIO;
	}

	/* Read column data */
	res = f_read(dev->file, data, POV_BYTES_PER_COL, &br);
	if (res != FR_OK || br != POV_BYTES_PER_COL) {
		pr_err("Host Flash FS: read col %d failed: %d (got %u)\n",
			column, res, br);
		return -EIO;
	}
#else
	/* Simulation: generate test pattern */
	for (u8 g = 0; g < POV_SPI_GROUPS; g++) {
		for (u16 i = 0; i < POV_BYTES_PER_GROUP; i++) {
			/* Simple gradient pattern based on column */
			u8 val = (u8)((column * 255) / POV_DISPLAY_COLS);
			data[g * POV_BYTES_PER_GROUP + i] = val;
		}
	}
#endif

	dev->cached_col = column;
	return 0;
}

/*********************************************************************
 * Cleanup
 *********************************************************************/

void host_flash_fs_close_image(struct host_flash_fs_device *dev)
{
	if (!dev || !dev->file_open)
		return;

#if MCU_PLATFORM_STM32
	if (dev->file) {
		f_close(dev->file);
		kfree(dev->file);
		dev->file = NULL;
	}
#endif

	dev->file_open = 0;
	dev->cached_col = 0xFFFF;
	pr_info("Host Flash FS: image closed\n");
}

void host_flash_fs_deinit(struct host_flash_fs_device *dev)
{
	if (!dev)
		return;

	host_flash_fs_close_image(dev);
	dev->initialized = 0;
	pr_info("Host Flash FS: deinitialized\n");
}

const struct pov_image_header *host_flash_fs_get_header(
	struct host_flash_fs_device *dev)
{
	if (!dev || !dev->file_open)
		return NULL;

	return &dev->header;
}
