#ifndef __K_FATFS_H
#define __K_FATFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

#include "fs/fatfs/ff.h"         /* Declarations of sector size */

int fatfs_init(void);
void printf_fatfs_err(FRESULT fresult);
FRESULT fatfs_miscellaneous(void);
FRESULT fatfs_scan_files(char *path);
FRESULT fatfs_open_append(FIL *fp, const char *path);
FRESULT fatfs_delete_node(TCHAR *path, UINT sz_buff, FILINFO *fno);
int fatfs_diskio(BYTE pdrv, UINT ncyc, DWORD *buff, UINT sz_buff);
FRESULT fatfs_contiguous_file(FIL *fp, int *cont);
int fatfs_raw_speed(BYTE pdrv, DWORD lba, DWORD len, void *buff, UINT sz_buff);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_FATFS_H */
