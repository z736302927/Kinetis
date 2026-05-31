#ifndef _FATFS_INTF_H
#define _FATFS_INTF_H

#include <linux/types.h>

int fatfs_build_path(char *buffer, size_t buffer_size,
	const char *file_path, const char *file_name);
u32 fatfs_get_flash_size(void);
int fatfs_create_dirs(const char *path);
int fatfs_find_file(char *file_path, char *file_name);
int fatfs_create_file(char *file_path, char *file_name);
int fatfs_delete_file(char *file_path, char *file_name);
int fatfs_delete_dir(const char *dir_path);
int fatfs_read_file_size(char *file_path, char *file_name,
	u32 *size);
int fatfs_read_store_info(char *file_path, char *file_name,
	long offset, u8 *pdata, int len);
int fatfs_write_store_info(char *file_path, char *file_name,
	long offset, u8 *pdata, int len);
int fatfs_copy_file(char *src_path, char *src_name,
	char *dst_path, char *dst_name);

#endif /* _FATFS_INTF_H */