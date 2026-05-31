
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

/**
 * Create multi-level directories recursively
 * @param path: Directory path to create (e.g., "0:/lrzsz/sz")
 * @return: 0 on success, negative error code on failure
 */
int fatfs_create_dirs(const char *path)
{
	char buffer[FATFS_BUFFER_SIZE];
	char *p;
	char *next_sep;
	char full_path[FATFS_BUFFER_SIZE];
	int path_len;
	int level_len;
	FRESULT res;
	FILINFO fno;

	if (!path || strlen(path) == 0) {
		return -EINVAL;
	}

	/* Copy path to buffer to avoid modifying original */
	if (snprintf(buffer, sizeof(buffer), "%s", path) >= sizeof(buffer)) {
		return -ENAMETOOLONG;
	}

	/* Skip drive prefix (e.g., "0:") */
	p = buffer;
	if (p[0] >= '0' && p[0] <= '9' && p[1] == ':') {
		p += 2;
	}

	/* Skip leading slash */
	if (*p == '/' || *p == '\\') {
		p++;
	}

	/* Create each directory level */
	while (*p) {
		next_sep = strchr(p, '/');
		if (!next_sep) {
			next_sep = strchr(p, '\\');
		}

		if (next_sep) {
			*next_sep = '\0';
		}

		/* Build partial path back to current level */
		/* full_path = "0:/lrzsz" */
		path_len = p - buffer;
		level_len = strlen(p);
		if (path_len + level_len + 1 >= sizeof(full_path)) {
			return -ENAMETOOLONG;
		}
		memcpy(full_path, buffer, path_len + level_len);
		full_path[path_len + level_len] = '\0';

		/* Check if directory already exists */
		res = f_stat(full_path, &fno);
		if (res == FR_OK) {
			/* Directory exists, check if it's actually a directory */
			if (!(fno.fattrib & AM_DIR)) {
				pr_err("%s exists but is not a directory\n", full_path);
				return -ENOTDIR;
			}
			/* Directory exists, skip creation */
		} else if (res == FR_NO_FILE) {
			/* Directory does not exist, create it */
			res = f_mkdir(full_path);
			if (res != FR_OK) {
				pr_err("Failed to create directory %s: %d\n", full_path, res);
				return fatfs_error_to_linux_error(res);
			}
		} else {
			/* Other error */
			pr_err("Failed to stat directory %s: %d\n", full_path, res);
			return fatfs_error_to_linux_error(res);
		}

		if (next_sep) {
			*next_sep = '/';
			p = next_sep + 1;
		} else {
			break;
		}
	}

	return 0;
}

int fatfs_build_path(char *buffer, size_t buffer_size,
	const char *file_path, const char *file_name)
{
	int written;
	size_t path_len;
	bool need_sep;

	/* If file_path is NULL, file_name is already the full path */
	if (!file_path) {
		written = snprintf(buffer, buffer_size, "%s", file_name);
	} else {
		path_len = strlen(file_path);
		need_sep = (path_len > 0 && file_path[path_len - 1] != '/' && file_path[path_len - 1] != '\\');

		written = snprintf(buffer, buffer_size, "%s%s%s", file_path, need_sep ? "/" : "", file_name);
	}

	if (written < 0 || (size_t)written >= buffer_size) {
		return -ENAMETOOLONG;
	}

	return 0;
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
	DIR dir;
	FRESULT res;
	char buffer[FATFS_BUFFER_SIZE];
	int ret = 0;

	if (!file_path || !file_name) {
		pr_err("Invalid parameters: file_path or file_name is NULL\n");
		return -EINVAL;
	}

	if (snprintf(buffer, sizeof(buffer), "%s", file_path) >= sizeof(buffer)) {
		pr_err("Directory path too long: %s\n", file_path);
		return -ENAMETOOLONG;
	}

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

	ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
	if (ret < 0) {
		pr_err("File path too long: %s%s\n", file_path, file_name);
		goto out;
	}

	res = f_open(&file, buffer, FA_READ);
	if (res == FR_OK) {
		ret = 0;
		f_close(&file);
	} else {
		pr_err("Failed to open file %s: %d\n", buffer, res);
		ret = fatfs_error_to_linux_error(res);
	}

out:
	res = f_closedir(&dir);
	if (res != FR_OK) {
		pr_err("Failed to close directory %s: %d\n", file_path, res);
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

	if (snprintf(buffer, sizeof(buffer), "%s", file_path) >= sizeof(buffer)) {
		pr_err("Directory path too long: %s\n", file_path);
		return -ENAMETOOLONG;
	}

	/* Try opening a directory. */
	res = f_opendir(&dir, buffer);
	if (res == FR_OK) {
		dir_opened = true;
	} else if (res == FR_NO_PATH) {
		/* Directory doesn't exist, create multi-level path. */
		ret = fatfs_create_dirs(buffer);
		if (ret < 0) {
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

	ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
	if (ret < 0) {
		pr_err("File path too long: %s%s\n", file_path, file_name);
		goto out;
	}

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

int fatfs_delete_dir(const char *dir_path)
{
	FRESULT res;
	DIR dir;
	FILINFO fno;
	char buffer[FATFS_BUFFER_SIZE];
	char entry_path[FATFS_BUFFER_SIZE];

	if (!dir_path) {
		pr_err("Invalid parameter: dir_path is NULL\n");
		return -EINVAL;
	}

	if (snprintf(buffer, sizeof(buffer), "%s", dir_path) >= sizeof(buffer)) {
		pr_err("Directory path too long: %s\n", dir_path);
		return -ENAMETOOLONG;
	}

	/* Open directory */
	res = f_opendir(&dir, buffer);
	if (res != FR_OK) {
		if (res == FR_NO_PATH) {
			/* Directory doesn't exist, nothing to delete */
			return 0;
		}
		pr_err("Failed to open directory %s: %d\n", buffer, res);
		return fatfs_error_to_linux_error(res);
	}

	/* Delete all entries in the directory */
	while (1) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK) {
			pr_err("Failed to read directory %s: %d\n", buffer, res);
			f_closedir(&dir);
			return fatfs_error_to_linux_error(res);
		}

		/* End of directory */
		if (fno.fname[0] == '\0') {
			break;
		}

		/* Skip "." and ".." */
		if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0) {
			continue;
		}

		/* Build full path */
		if (snprintf(entry_path, sizeof(entry_path), "%s/%s", buffer, fno.fname) >= sizeof(entry_path)) {
			pr_err("Entry path too long: %s/%s\n", buffer, fno.fname);
			f_closedir(&dir);
			return -ENAMETOOLONG;
		}

		/* Delete file or directory */
		if (fno.fattrib & AM_DIR) {
			/* Recursively delete subdirectory */
			res = f_unlink(entry_path);
			if (res != FR_OK) {
				pr_err("Failed to delete directory %s: %d\n", entry_path, res);
				f_closedir(&dir);
				return fatfs_error_to_linux_error(res);
			}
		} else {
			/* Delete file */
			res = f_unlink(entry_path);
			if (res != FR_OK) {
				pr_err("Failed to delete file %s: %d\n", entry_path, res);
				f_closedir(&dir);
				return fatfs_error_to_linux_error(res);
			}
		}
	}

	f_closedir(&dir);

	/* Delete the empty directory itself */
	res = f_unlink(buffer);
	if (res != FR_OK) {
		pr_err("Failed to delete directory %s: %d\n", buffer, res);
		return fatfs_error_to_linux_error(res);
	}

	return 0;
}

int fatfs_delete_file(char *file_path, char *file_name)
{
	FRESULT res;
	char buffer[FATFS_BUFFER_SIZE];
	int ret;

	if (!file_name) {
		pr_err("Invalid parameter: file_name is NULL\n");
		return -EINVAL;
	}

	ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
	if (ret < 0) {
		pr_err("File path too long: %s%s\n", file_path ? file_path : "", file_name);
		return ret;
	}

	res = f_unlink(buffer);
	if (res != FR_OK) {
		pr_err("Failed to delete file %s: %d\n", buffer, res);
		return fatfs_error_to_linux_error(res);
	}

	return 0;
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

	ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
	if (ret < 0) {
		pr_err("File path too long: %s%s\n", file_path, file_name);
		return ret;
	}

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK) {
		*size = file.obj.objsize;
		res = f_close(&file);
		if (res != FR_OK) {
			pr_err("Failed to close file %s: %d\n", buffer, res);
			ret = fatfs_error_to_linux_error(res);
		}
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

	ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
	if (ret < 0) {
		pr_err("File path too long: %s%s\n", file_path, file_name);
		return ret;
	}

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK) {
		res = f_lseek(&file, offset);
		if (res != FR_OK) {
			pr_err("Seek %s failed. fs ret code(%d)\n", file_name, res);
			ret = fatfs_error_to_linux_error(res);
			goto out_close;
		}

		res = f_read(&file, pdata, len, (void *)&bytes_read);
		if (res != FR_OK) {
			pr_err("Read %s failed. fs ret code(%d)\n", file_name, res);
			ret = fatfs_error_to_linux_error(res);
			goto out_close;
		}
		if (bytes_read != len) {
			pr_err("Partial read: requested %d bytes, got %d bytes\n", len, bytes_read);
			ret = -EIO;
			goto out_close;
		}

out_close:
		res = f_close(&file);
		if (res != FR_OK) {
			pr_err("Failed to close file %s: %d\n", buffer, res);
			if (ret == 0) {
				ret = fatfs_error_to_linux_error(res);
			}
		}
	} else {
		pr_err("Read %s failed. fs ret code(%d)\n", file_name, res);
		ret = fatfs_error_to_linux_error(res);
	}

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

	ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
	if (ret < 0) {
		pr_err("File path too long: %s%s\n", file_path, file_name);
		return ret;
	}

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_WRITE);
	if (res == FR_OK) {
		res = f_lseek(&file, offset);
		if (res != FR_OK) {
			pr_err("Seek %s failed. fs ret code(%d)\n", file_name, res);
			ret = fatfs_error_to_linux_error(res);
			goto out_close;
		}

		res = f_write(&file, pdata, len, (void *)&bytes_written);
		if (res != FR_OK) {
			pr_err("Write %s failed. fs ret code(%d)\n", file_name, res);
			ret = fatfs_error_to_linux_error(res);
			goto out_close;
		}

		if (bytes_written != len) {
			pr_err("Partial write: requested %d bytes, wrote %d bytes\n", len, bytes_written);
			ret = -EIO;
		}

out_close:
		res = f_close(&file);
		if (res != FR_OK) {
			pr_err("Close file %s failed. fs ret code(%d)\n", file_name, res);
			if (ret == 0) {
				ret = fatfs_error_to_linux_error(res);
			}
		}
	} else {
		pr_err("Open file %s for write failed. fs ret code(%d)\n", file_name, res);
		ret = fatfs_error_to_linux_error(res);
	}

	return ret;
}

/**
 * Convert FAT date/time to Unix timestamp
 * @fdate: FAT date (bits 15-9: year-1980, bits 8-5: month, bits 4-0: day)
 * @ftime: FAT time (bits 15-11: hour, bits 10-5: minute, bits 4-0: second/2)
 * @return: Unix timestamp (seconds since 1970-01-01)
 */
static u64 fat_time_to_unix_timestamp(WORD fdate, WORD ftime)
{
	u16 year = ((fdate >> 9) & 0x7F) + 1980;
	u8 month = (fdate >> 5) & 0x0F;
	u8 day = fdate & 0x1F;
	u8 hour = (ftime >> 11) & 0x1F;
	u8 minute = (ftime >> 5) & 0x3F;
	u8 second = (ftime & 0x1F) * 2;

	/* Simplified Unix timestamp calculation (days since 1970-01-01) */
	u64 days = 365 * (year - 1970) + (year - 1969) / 4 - (year - 1901) / 100 + (year - 1601) / 400;
	u8 month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	/* Add days for previous months in current year */
	for (u8 m = 1; m < month; m++) {
		days += month_days[m - 1];
		if (m == 2 && (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0))) {
			days += 1;  /* Leap year February */
		}
	}
	days += day - 1;

	return days * 86400ULL + hour * 3600ULL + minute * 60ULL + second;
}

/**
 * Get file last modification time
 * @file_path: Directory path
 * @file_name: File name
 * @modtime: Pointer to store modification time (Unix timestamp)
 * @return: 0 on success, negative error code on failure
 */
int fatfs_get_file_mtime(char *file_path, char *file_name, u64 *modtime)
{
	FRESULT res;
	FILINFO fno;
	char buffer[FATFS_BUFFER_SIZE];
	int ret;

	if (!file_path && file_name) {
		res = f_stat(file_name, &fno);
	} else {
		ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
		if (ret < 0) {
			pr_err("File path too long: %s%s\n", file_path, file_name);
			return ret;
		}
		res = f_stat(buffer, &fno);
	}
	if (res != FR_OK) {
		return fatfs_error_to_linux_error(res);
	}

	*modtime = fat_time_to_unix_timestamp(fno.fdate, fno.ftime);
	return 0;
}

/**
 * Get file permissions
 * @file_path: Directory path
 * @file_name: File name
 * @mode: Pointer to store file permissions (mode_t format, e.g., 0644)
 * @return: 0 on success, negative error code on failure
 */
int fatfs_get_file_mode(char *file_path, char *file_name, mode_t *mode)
{
	FRESULT res;
	FILINFO fno;
	char buffer[FATFS_BUFFER_SIZE];
	int ret;

	if (!file_path && file_name) {
		res = f_stat(file_name, &fno);
	} else {
		ret = fatfs_build_path(buffer, sizeof(buffer), file_path, file_name);
		if (ret < 0) {
			pr_err("File path too long: %s%s\n", file_path, file_name);
			return ret;
		}
		res = f_stat(buffer, &fno);
	}
	if (res != FR_OK) {
		return fatfs_error_to_linux_error(res);
	}

	/* Convert FAT attributes to Unix-style mode */
	if (fno.fattrib & AM_RDO) {
		*mode = 0444;  /* Read-only: r--r--r-- */
	} else {
		*mode = 0644;  /* Read/write: rw-r--r-- */
	}

	return 0;
}

/**
 * @brief Copy a file from source to destination, overwriting if destination exists
 * @param src_path: Source directory path
 * @param src_name: Source file name
 * @param dst_path: Destination directory path
 * @param dst_name: Destination file name (if NULL, uses src_name)
 * @return 0 on success, negative error code on failure
 */
int fatfs_copy_file(char *src_path, char *src_name,
	char *dst_path, char *dst_name)
{
	FIL src_fp;
	FIL dst_fp;
	FRESULT res;
	char src_buffer[FATFS_BUFFER_SIZE];
	char dst_buffer[FATFS_BUFFER_SIZE];
	u8 copy_buf[512];
	u32 bytes_read;
	u32 bytes_written;
	int ret = 0;

	if (!src_name) {
		pr_err("Invalid parameter: src_name is NULL\n");
		return -EINVAL;
	}

	/* Build source full path */
	ret = fatfs_build_path(src_buffer, sizeof(src_buffer), src_path, src_name);
	if (ret < 0) {
		pr_err("Source path too long: %s%s\n", src_path ? src_path : "", src_name);
		return ret;
	}

	/* If dst_name is not provided, use source file name */
	if (!dst_name)
		dst_name = src_name;

	/* Build destination full path */
	ret = fatfs_build_path(dst_buffer, sizeof(dst_buffer), dst_path, dst_name);
	if (ret < 0) {
		pr_err("Destination path too long: %s%s\n", dst_path ? dst_path : "", dst_name);
		return ret;
	}

	/* Open source file for reading */
	res = f_open(&src_fp, src_buffer, FA_OPEN_EXISTING | FA_READ);
	if (res != FR_OK) {
		pr_err("Failed to open source file %s: %d\n", src_buffer, res);
		return fatfs_error_to_linux_error(res);
	}

	/* Open destination file for writing (create / overwrite) */
	res = f_open(&dst_fp, dst_buffer, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		pr_err("Failed to open destination file %s: %d\n", dst_buffer, res);
		f_close(&src_fp);
		return fatfs_error_to_linux_error(res);
	}

	/* Copy data in chunks */
	while (1) {
		res = f_read(&src_fp, copy_buf, sizeof(copy_buf), (void *)&bytes_read);
		if (res != FR_OK) {
			pr_err("Failed to read from %s: %d\n", src_buffer, res);
			ret = fatfs_error_to_linux_error(res);
			break;
		}

		if (bytes_read == 0) {
			break;  /* End of file */
		}

		res = f_write(&dst_fp, copy_buf, bytes_read, (void *)&bytes_written);
		if (res != FR_OK) {
			pr_err("Failed to write to %s: %d\n", dst_buffer, res);
			ret = fatfs_error_to_linux_error(res);
			break;
		}

		if (bytes_written != bytes_read) {
			pr_err("Partial write: requested %d bytes, wrote %d bytes\n",
				bytes_read, bytes_written);
			ret = -EIO;
			break;
		}
	}

	/* Close destination file */
	res = f_close(&dst_fp);
	if (res != FR_OK) {
		pr_err("Failed to close destination file %s: %d\n", dst_buffer, res);
		if (ret == 0)
			ret = fatfs_error_to_linux_error(res);
	}

	/* Close source file */
	res = f_close(&src_fp);
	if (res != FR_OK) {
		pr_err("Failed to close source file %s: %d\n", src_buffer, res);
		if (ret == 0)
			ret = fatfs_error_to_linux_error(res);
	}

	return ret;
}
