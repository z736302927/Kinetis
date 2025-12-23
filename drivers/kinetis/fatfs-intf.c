
#include <generated/deconfig.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <kinetis/fatfs.h>

#include "../../fs/fatfs/ff.h"

#define FATFS_BUFFER_SIZE 64

/**
 * Convert FATFS error code to Linux error code
 * @fr: FATFS error code
 * @return: Linux error code
 */
static int fatfs_error_to_linux_error(FRESULT fr)
{
	switch (fr) {
	case FR_OK:
		return 0;
	case FR_DISK_ERR:
	case FR_INVALID_OBJECT:
		return -EIO;
	case FR_INT_ERR:
		return -EIO;
	case FR_NOT_READY:
		return -EAGAIN;
	case FR_NO_FILE:
	case FR_NO_PATH:
		return -ENOENT;
	case FR_INVALID_NAME:
		return -EINVAL;
	case FR_DENIED:
	case FR_EXIST:
		return -EACCES;
	case FR_WRITE_PROTECTED:
		return -EROFS;
	case FR_INVALID_DRIVE:
	case FR_NOT_ENABLED:
	case FR_NO_FILESYSTEM:
		return -ENODEV;
	case FR_MKFS_ABORTED:
		return -EIO;
	case FR_TIMEOUT:
		return -ETIMEDOUT;
	case FR_LOCKED:
		return -EDEADLK;
	case FR_NOT_ENOUGH_CORE:
		return -ENOMEM;
	case FR_TOO_MANY_OPEN_FILES:
		return -EMFILE;
	case FR_INVALID_PARAMETER:
		return -EINVAL;
	default:
		return -EIO;
	}
}

u32 fatfs_get_flash_size(void)
{
	FATFS *pfs;
	DWORD fre_clust, fre_sect, tot_sect;
	FRESULT res;

	/* Gets device information and empty cluster size. */
	res = f_getfree("0:", &fre_clust, &pfs);

	if (res != FR_OK) {
		pr_err("Failed to get free space: %d\n", res);
		return 0;
	}

	/* The total number of sectors and the number of empty sectors are calculated. */
	tot_sect = (pfs->n_fatent - 2) * pfs->csize;
	fre_sect = fre_clust * pfs->csize;
	/* Print information using floating point calculation */
	pr_err("Total equipment space: %.2f MB, Available space: %.2f MB.\n",
		(float)tot_sect * FATFS_SECTOR_SIZE / 1024.0f / 1024.0f,
		(float)fre_sect * FATFS_SECTOR_SIZE / 1024.0f / 1024.0f);

	return fre_sect * FATFS_SECTOR_SIZE;
}

int fatfs_find_file(char *file_path, char *file_name)
{
	/* Search a directory for objects and display it */
	FIL file;
	FILINFO fno;
	DIR dir;
	FRESULT res;
	char buffer[FATFS_BUFFER_SIZE];
	int ret = 0;

	if (!file_path || !file_name) {
		pr_err("Invalid parameters: file_path or file_name is NULL\n");
		return -EINVAL;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s", file_path);

	/* Start to search for photo files */
	// 	fr = f_findfirst(&dir, &fno, file_path, file_name);
	// 	if (fr != FR_OK) {
	// 		pr_err("Failed to find file %s in %s: %d\n", file_name, file_path, fr);
	// 		ret = fatfs_error_to_linux_error(fr);
	// 	}

	res = f_opendir(&dir, buffer);
	if (res != FR_OK) {
		pr_err("Failed to open directory %s: %d\n", buffer, res);
		return fatfs_error_to_linux_error(res);
	}

	res = f_open(&file, buffer, FA_READ);
	if (res == FR_OK) {
		ret = 0;
		f_close(&file);
	} else {
		pr_err("Failed to open file %s: %d\n", buffer, res);
		ret = fatfs_error_to_linux_error(res);
	}

	res = f_closedir(&dir);
	if (res != FR_OK) {
		pr_err("Failed to close directory %s: %d\n", buffer, res);
	}

	return ret;
}

int fatfs_create_file(char *file_path, char *file_name)
{
	FIL file;
	DIR dir;
	FRESULT res;
	char buffer[FATFS_BUFFER_SIZE];
	bool dir_opened = false;
	int ret = 0;

	if (!file_path || !file_name) {
		pr_err("Invalid parameters: file_path or file_name is NULL\n");
		return -EINVAL;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s", file_path);

	/* Try opening a directory. */
	res = f_opendir(&dir, buffer);
	if (res == FR_OK) {
		dir_opened = true;
	} else if (res == FR_NO_PATH) {
		/* Failure to open directory, create directory. */
		res = f_mkdir(buffer);
		if (res != FR_OK) {
			pr_err("Failed to create directory %s: %d\n", buffer, res);
			ret = fatfs_error_to_linux_error(res);
			goto out;
		}
		/* Try to open the newly created directory */
		res = f_opendir(&dir, buffer);
		if (res != FR_OK) {
			pr_err("Failed to open directory %s after creation: %d\n", buffer, res);
			ret = fatfs_error_to_linux_error(res);
			goto out;
		}
		dir_opened = true;
	} else {
		pr_err("Failed to open directory %s: %d\n", buffer, res);
		ret = fatfs_error_to_linux_error(res);
		goto out;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);
	res = f_open(&file, buffer, FA_CREATE_NEW | FA_WRITE);
	if (res != FR_OK) {
		pr_err("Failed to create file %s: %d\n", buffer, res);
		ret = fatfs_error_to_linux_error(res);
		goto out;
	}

	res = f_close(&file);
	if (res != FR_OK) {
		pr_err("Failed to close file %s: %d\n", buffer, res);
		ret = fatfs_error_to_linux_error(res);
		goto out;
	}

out:
	if (dir_opened) {
		res = f_closedir(&dir);
		if (res != FR_OK) {
			pr_err("Failed to close directory %s: %d\n", file_path, res);
			ret = fatfs_error_to_linux_error(res);
		}
	}

	return ret;
}

int fatfs_delete_file(char *file_path, char *file_name)
{
	FRESULT res;
	char buffer[FATFS_BUFFER_SIZE];
	int ret = 0;

	if (!file_path || !file_name) {
		pr_err("Invalid parameters: file_path or file_name is NULL\n");
		return -EINVAL;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);
	res = f_unlink(buffer);
	if (res != FR_OK) {
		pr_err("Failed to delete file %s: %d\n", buffer, res);
		ret = fatfs_error_to_linux_error(res);
	}

	return ret;
}

int fatfs_read_file_size(char *file_path, char *file_name,
	u32 *size)
{
	FIL file;
	FRESULT res;
	char buffer[FATFS_BUFFER_SIZE];
	int ret = 0;

	if (!file_path || !file_name || !size) {
		pr_err("Invalid parameters: file_path, file_name, or size is NULL\n");
		return -EINVAL;
	}

	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK) {
		*size = file.obj.objsize;
		f_close(&file);
	} else {
		pr_err("Read the size of %s failed. fs ret code(%d)\n", file_name, res);
		ret = fatfs_error_to_linux_error(res);
	}

	return ret;
}

int fatfs_read_store_info(char *file_path, char *file_name,
	long offset, u8 *pdata, int len)
{
	FIL file;
	FRESULT res;
	u32 bytes_read;
	char buffer[FATFS_BUFFER_SIZE];
	int ret = 0;

	if (!file_path || !file_name || !pdata || len <= 0) {
		pr_err("Invalid parameters: file_path, file_name, pdata is NULL or len <= 0\n");
		return -EINVAL;
	}

	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK) {
		f_lseek(&file, offset);
		res = f_read(&file, pdata, len, (void *)&bytes_read);
		f_close(&file);
		if (res != FR_OK) {
			pr_err("Read %s failed. fs ret code(%d)\n", file_name, res);
			ret = fatfs_error_to_linux_error(res);
			goto out;
		}
		if (bytes_read != len) {
			pr_err("Partial read: requested %d bytes, got %d bytes\n", len, bytes_read);
			ret = -EIO;
			goto out;
		}
	} else {
		pr_err("Read %s failed. fs ret code(%d)\n", file_name, res);
		ret = fatfs_error_to_linux_error(res);
	}

out:

	return ret;
}

int fatfs_write_store_info(char *file_path, char *file_name,
	long offset, u8 *pdata, int len)
{
	FIL file;
	FRESULT res;
	u32 bytes_written;
	char buffer[FATFS_BUFFER_SIZE];
	int ret = 0;

	if (!file_path || !file_name || !pdata || len <= 0) {
		pr_err("Invalid parameters: file_path, file_name, pdata is NULL or len <= 0\n");
		return -EINVAL;
	}

	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_WRITE);
	if (res == FR_OK) {
		f_lseek(&file, offset);
		res = f_write(&file, pdata, len, (void *)&bytes_written);
		if (res != FR_OK) {
			pr_err("Write %s failed. fs ret code(%d)\n", file_name, res);
			f_close(&file);
			ret = fatfs_error_to_linux_error(res);
			goto out;
		}

		if (bytes_written != len) {
			pr_err("Partial write: requested %d bytes, wrote %d bytes\n", len, bytes_written);
			f_close(&file);
			ret = -EIO;
			goto out;
		}

		res = f_close(&file);
		if (res != FR_OK) {
			pr_err("Close file %s failed. fs ret code(%d)\n", file_name, res);
			ret = fatfs_error_to_linux_error(res);
			goto out;
		}
	} else {
		pr_err("Open file %s for write failed. fs ret code(%d)\n", file_name, res);
		ret = fatfs_error_to_linux_error(res);
	}

out:

	return ret;
}