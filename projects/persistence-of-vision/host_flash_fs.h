/**
 * @file host_flash_fs.h
 * @brief QSPI Flash FatFS interface for host image data
 * @note Manages QSPI Flash initialization, FatFS mounting,
 *       and column-based image data reading
 */

#ifndef POV_HOST_FLASH_FS_H
#define POV_HOST_FLASH_FS_H

#include <linux/types.h>
#include "host_pov_config.h"

/* Forward declaration for FatFS file type */
#ifndef FIL
typedef struct __FIL FIL;
#endif

/*********************************************************************
 * Image File Header Format
 *********************************************************************/

#define POV_IMG_MAGIC       0x504F5621  /* "POV!" */
#define POV_IMG_VERSION     1

struct pov_image_header {
	u32 magic;               /* Magic number: POV_IMG_MAGIC */
	u16 version;             /* Format version */
	u16 columns;             /* Number of columns (should be 720) */
	u16 leds_per_group;      /* LEDs per SPI group (64) */
	u8 spi_groups;           /* Number of SPI groups (4) */
	u8 reserved[5];          /* Reserved for future use */
};

/*********************************************************************
 * Host Flash FS Device Structure
 *********************************************************************/

struct host_flash_fs_device {
	u8 initialized;
	u8 file_open;
	struct pov_image_header header;
	u16 cached_col;            /* Currently cached column */
	u8 col_data[POV_BYTES_PER_COL]; /* Column data cache */

#if MCU_PLATFORM_STM32
	FIL *file;                 /* FatFS file object */
#endif
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize QSPI Flash and mount FatFS
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 */
int host_flash_fs_init(struct host_flash_fs_device *dev);

/**
 * @brief Load image file and read header
 * @param dev: Device pointer
 * @param filename: Image file path (e.g., "0:pov.img")
 * @return 0 on success, negative error code on failure
 */
int host_flash_fs_load_image(struct host_flash_fs_device *dev,
	const char *filename);

/**
 * @brief Read image data for a specific column
 * @param dev: Device pointer
 * @param column: Column number (0~719)
 * @param data: Output buffer (must be at least POV_BYTES_PER_COL bytes)
 * @return 0 on success, negative error code on failure
 * @note Data layout: [group0:192B][group1:192B][group2:192B][group3:192B]
 */
int host_flash_fs_read_column(struct host_flash_fs_device *dev,
	u16 column, u8 *data);

/**
 * @brief Close image file and free resources
 * @param dev: Device pointer
 */
void host_flash_fs_close_image(struct host_flash_fs_device *dev);

/**
 * @brief Deinitialize Flash FS
 * @param dev: Device pointer
 */
void host_flash_fs_deinit(struct host_flash_fs_device *dev);

/**
 * @brief Get image header information
 * @param dev: Device pointer
 * @return Pointer to header, or NULL if not loaded
 */
const struct pov_image_header *host_flash_fs_get_header(
	struct host_flash_fs_device *dev);

#endif /* POV_HOST_FLASH_FS_H */
