
#include <generated/deconfig.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include "../../fs/fatfs/ff.h"

u32 fatfs_get_flash_size(void)
{
	FATFS *pfs;
	DWORD fre_clust, fre_sect, tot_sect;
	FRESULT res;

	/* Gets device information and empty cluster size. */
	res = f_getfree("0:", &fre_clust, &pfs);

	if (res != FR_OK)
		return res;

	/* The total number of sectors and the number of empty sectors are calculated. */
	tot_sect = (pfs->n_fatent - 2) * pfs->csize;
	fre_sect = fre_clust * pfs->csize;
	/* Print information (4096 bytes/sector) */
	pr_err("Total equipment space: %u MB.\n", tot_sect * 4 / 1024);
	pr_err("Available space: %u MB.\n", fre_sect * 4 / 1024);

	return fre_sect * 4 * 1024;
}

int fatfs_find_file(char *file_path, char *file_name)
{
	/* Search a directory for objects and display it */
	FRESULT fr;	/* Return value */
	DIR dir;	/* Directory object */
	FILINFO fno;	/* File information */

	/* Start to search for photo files */
	fr = f_findfirst(&dir, &fno, file_path, file_name);
	if (fr != FR_OK)
		return -ENODATA;

//	/* Repeat while an item is found */
//	while (fr == FR_OK && fno.fname[0]) {
//		/* Print the object name */
//		pr_info("%s\n", fno.fname);
//		/* Search for next item */
//		fr = f_findnext(&dir, &fno);
//	}

	f_closedir(&dir);

	return 0;
}

int fatfs_create_file(char *file_path, char *file_name)
{
	FIL file;
	DIR dir;
	FRESULT res;
	char buffer[64];

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, strlen(file_path), "%s", file_path);
	/* Try opening a directory. */
	res = f_opendir(&dir, buffer);
	/* Failure to open directory, create directory. */
	if (res == FR_NO_PATH) {
		res = f_mkdir(buffer);
		if (res != FR_OK)
			return -EACCES;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);
	res = f_open(&file, buffer, FA_CREATE_NEW | FA_WRITE);
	if (res != FR_OK)
		return -EACCES;

	res = f_close(&file);
	if (res != FR_OK)
		return -EACCES;

	res = f_closedir(&dir);
	if (res != FR_OK)
		return -EACCES;

	return 0;
}

int fatfs_delete_file(char *file_path, char *file_name)
{
	FRESULT res;
	char buffer[64];

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);
	res = f_unlink(buffer);
	if (res != FR_OK)
		return -EACCES;

	return 0;
}

int fatfs_read_file_size(char *file_path, char *file_name,
	u32 *size)
{
	FIL file;
	FRESULT res;
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK)
		*size = file.obj.objsize;
	else {
		pr_err("Read the size of %s failed. fs ret code(%d)\n", file_name, res);
		return -EINVAL;
	}

	f_close(&file);

	return 0;
}

int fatfs_read_store_info(char *file_path, char *file_name,
	long offset, u8 *pdata, int len)
{
	FIL file;
	FRESULT res;
	u32 bytes_read;
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK) {
		f_lseek(&file, offset);
		res = f_read(&file, pdata, len, (void *)&bytes_read);
		if (res != FR_OK)
			return -EACCES;
	} else {
		pr_err("Read %s failed.\n. fs ret code(%d)", file_name, res);
		return -EINVAL;
	}

	f_close(&file);

	return 0;
}

int fatfs_write_store_info(char *file_path, char *file_name,
	long offset, u8 *pdata, int len)
{
	FIL file;
	FRESULT res;
	u32 bytes_written;
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "%s%s", file_path, file_name);

	res = f_open(&file, buffer, FA_OPEN_EXISTING | FA_WRITE);
	if (res == FR_OK) {
		f_lseek(&file, offset);
		res = f_write(&file, pdata, len, (void *)&bytes_written);
		if (res != FR_OK)
			return -EACCES;

		res = f_close(&file);
		if (res != FR_OK)
			return -EACCES;
	} else {
		pr_err("Write %s failed. fs ret code(%d)\n", file_name, res);
		return -EINVAL;
	}

	return 0;
}