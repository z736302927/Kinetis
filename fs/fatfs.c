

#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <kinetis/design_verification.h>
#include <kinetis/real-time-clock.h>

#include "fatfs/diskio.h"     /* Declarations of disk functions */
#include "fatfs/ff_gen_drv.h"
#ifdef KINETIS_FAKE_SIM
#include "fatfs/drivers/fake_ram_diskio.h"
#else
#include "fatfs/drivers/sd_diskio.h"
#endif
#include "fatfs/ff.h"         /* FatFs API and configuration */

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */
/* File system object for  disk logical drive */
static FATFS disk_fatfs;
/* File object */
static FIL test_file;
/*  disk logical drive path */
static char disk_path[4];
/* a work buffer for the f_mkfs() */
static uint8_t work_buffer[FF_MAX_SS];

/* Volume to partition mapping table for multi-partition support */
#if FF_MULTI_PARTITION
PARTITION VolToPart[] = {
	{0, 0},	/* Logical drive 0 -> Physical drive 0, auto-detect partition */
	/* Add more partitions as needed */
};
#endif

int process_fatfs_err(FRESULT fresult);

int fatfs_init(void)
{
	MKFS_PARM opt;
	int ret;

	/*## FatFS: Link the SD driver ###########################*/
#ifdef KINETIS_FAKE_SIM
	ret = FATFS_LinkDriver(&fake_ram_disk_driver, disk_path);
#else
	ret = FATFS_LinkDriver(&SD_Driver, disk_path);
#endif

	/* additional user code for init */
	if (ret == 0) {
		FRESULT res;

		/* First try to mount immediately to check if filesystem exists */
		res = f_mount(&disk_fatfs, (TCHAR const *)disk_path, 1);

		if (res == FR_NO_FILESYSTEM) {
			pr_info("No filesystem found, creating FAT filesystem...\n");

			/* Create filesystem */
			opt.fmt = FM_FAT;      // 或直接指定 FM_FAT16
			opt.n_fat = 2;         // 标准配置：两个FAT表
			opt.align = 0;         // 数据区从紧随FAT表后开始（无特殊对齐）
			opt.n_root = 512;      // **FAT16特有**：根目录可容纳512个条目
			opt.au_size = 4096;    // 指定簇大小为4KB（8192KB / 4KB ≈ 2000簇，很合理）
			res = f_mkfs((const TCHAR *)disk_path, &opt, work_buffer, sizeof(work_buffer));
			if (res != FR_OK) {
				pr_err("Failed to create filesystem: %d\n", res);
				process_fatfs_err(res);
				return -EIO;
			}

			/* Mount after formatting */
			res = f_mount(&disk_fatfs, (TCHAR const *)disk_path, 1);
			if (res != FR_OK) {
				pr_err("Failed to mount after formatting: %d\n", res);
				process_fatfs_err(res);
				return -EIO;
			}

			pr_info("Filesystem created and mounted successfully.\n");
		} else if (res != FR_OK) {
			pr_err("Mount failed: %d\n", res);
			process_fatfs_err(res);
			return -EIO;
		} else
			pr_info("Filesystem mounted successfully.\n");
	} else {
		pr_err("Failed to link low level driver.\n");
		return -ENOSYS;
	}

	return 0;
}

int process_fatfs_err(FRESULT fresult)
{
	int ret;

	switch (fresult) {
	case FR_OK:                   //(0)
		pr_info("Operation successful.\n");
		break;

	case FR_DISK_ERR:             //(1)
		pr_info("Hardware input/output driver error.\n");
		break;

	case FR_INT_ERR:              //(2)
		pr_info("Assertion error.\n");
		break;

	case FR_NOT_READY:            //(3)
		pr_info("The physical device doesn't work.\n");
		break;

	case FR_NO_FILE:              //(4)
		pr_info("Unable to locate file.\n");
		break;

	case FR_NO_PATH:              //(5)
		pr_info("Unable to find path.\n");
		break;

	case FR_INVALID_NAME:         //(6)
		pr_info("Invalid path name, "
			"May be not set the FF_USE_LFN feature.\n");
		break;

	case FR_DENIED:               //(7)
		pr_info("Access denied.\n");
		break;
	case FR_EXIST:                //(8)
		pr_info("File already exists.\n");
		break;

	case FR_INVALID_OBJECT:       //(9)
		pr_info("Invalid file or path.\n");
		break;

	case FR_WRITE_PROTECTED:      //(10)
		pr_info("Logical device write protection.\n");
		break;

	case FR_INVALID_DRIVE:        //(11)
		pr_info("Invalid logic device.\n");
		break;

	case FR_NOT_ENABLED:          //(12)
		pr_info("Invalid workspace.\n");
		break;

	case FR_NO_FILESYSTEM:        //(13)
		pr_info("No file system, please using f_mkfs().\n");
		ret = f_mkfs((const TCHAR *)disk_path, 0,
				work_buffer, sizeof(work_buffer));

		if (ret != FR_OK) {
			pr_err("Failed to create filesystem: %d\n", ret);
			return ret;
		}

		break;

	case FR_MKFS_ABORTED:         //(14)
		pr_info("The f_mkfs function failed "
			"because of a function parameter problem.\n");
		break;

	case FR_TIMEOUT:              //(15)
		pr_info("The operation timed out.\n");
		break;

	case FR_LOCKED:               //(16)
		pr_info("The file is protected.\n");
		break;

	case FR_NOT_ENOUGH_CORE:      //(17)
		pr_info("Long filename support failed to get heap space.\n");
		break;

	case FR_TOO_MANY_OPEN_FILES:  //(18)
		pr_info("Too many files open.\n");
		break;

	case FR_INVALID_PARAMETER:    // (19)
		pr_info("Invalid parameter.\n");
		break;

	default:
		break;
	}

	return 0;
}

FRESULT fatfs_miscellaneous(void)
{
	FATFS *pfs;
	DWORD fre_clust, fre_sect, tot_sect;
	FRESULT res;
	DIR my_dir;
	/* File read buffer */
	uint8_t rtext[100];
	int32_t byteswritten = 0;
	uint32_t bytesread = 0;

	pr_info("Device information acquisition.\n");
	/* Gets device information and empty cluster size. */
	res = f_getfree(disk_path, &fre_clust, &pfs);
	/* The total number of sectors and the number of empty sectors are calculated. */
	tot_sect = (pfs->n_fatent - 2) * pfs->csize;
	fre_sect = fre_clust * pfs->csize;
	/* Print information (4096 bytes/sector) */
	pr_info("Total equipment space:%6u MB.\r\n"
		"Available space: %6u MB.\n",
		tot_sect * 4 / 1024, fre_sect * 4 / 1024);

	pr_info("File location and formatting write function test\n");
	res = f_open(&test_file, "FatFs.txt", FA_OPEN_EXISTING | FA_WRITE | FA_READ);

	if (res == FR_OK) {
		/* File location */
		res = f_lseek(&test_file, f_size(&test_file) - 1);

		if (res == FR_OK) {
			/* Format write, parameter format similar to printf function. */
			byteswritten = f_printf(&test_file,
					"add a new line to the original file.");

			if (byteswritten == -1)
				pr_err(" \n");

			byteswritten = f_printf(&test_file,
					"Total equipment space:%6lu MB.\r\n"
					"Available space: %6lu MB.\n",
					tot_sect * 4 / 1024, fre_sect * 4 / 1024);

			if (byteswritten == -1)
				pr_err(" \n");

			/* The file is positioned at the start of the file. */
			res = f_lseek(&test_file, 0);
			/* Read all contents of the file into the cache. */
			res = f_read(&test_file, rtext, f_size(&test_file), &bytesread);

			if (res == FR_OK)
				pr_info("The file content: %s\n", rtext);
		}

		f_close(&test_file);

		pr_info("Directory creation and rename function test\n");
		/* Try opening a directory. */
		res = f_opendir(&my_dir, "TestDir");

		if (res != FR_OK) {
			/* Failure to open directory, create directory. */
			res = f_mkdir("TestDir");
		} else {
			/* If the directory already exists, close it. */
			res = f_closedir(&my_dir);
			/* Delete the file */
			f_unlink("TestDir/testdir.txt");
		}

		if (res == FR_OK) {
			/* Rename and move files. */
			res = f_rename("FatFs.txt", "TestDir/testdir.txt");
		}
	} else {
		pr_info("Failed to open file: %d\n", res);
		pr_debug(
			"You may need to run the FatFs migration and "
			"read and write test project again.\n");
	}

	return res;
}

/**
  * @brief  scan_files Recursively scan the files in FatFs.
  * @param  path:Initial scan path.
  * @retval result:The return value of the file system.
  */
FRESULT fatfs_scan_files(char *path)
{
	FRESULT res;     //The part of the variable that is modified in the recursive process is not global.
	FILINFO fno;
	DIR my_dir;
	int i;
	char *fn;        //The file name.

	//Open directory
	res = f_opendir(&my_dir, path);

	if (res == FR_OK) {
		i = strlen(path);

		for (;;) {
			//Reading the contents of the directory automatically reads the next file.
			res = f_readdir(&my_dir, &fno);

			//Is empty means all items read, jump out.
			if (res != FR_OK || fno.fname[0] == 0)
				break;

			fn = fno.fname;

			//Dot to indicate current directory, skip.
			if (*fn == '.')
				continue;

			//Directory, recursive read
			if (fno.fattrib & AM_DIR) {
				//Synthesize the full directory name.
				sprintf(&path[i], "/%s", fn);
				//The recursive traversal.
				res = fatfs_scan_files(path);
				path[i] = 0;

				//Turn failure on and out of the loop.
				if (res != FR_OK)
					break;
			} else {
				pr_info("%s/%s\n", path, fn);                //Output file name
				/* Here you can extract the file path for a particular format. */
			}
		}
	}

	return res;
}

/*------------------------------------------------------------/
/ Open or create a file in append mode
/ (This function was sperseded by FA_OPEN_APPEND flag at FatFs R0.12a)
/------------------------------------------------------------*/
FRESULT fatfs_open_append(
	FIL *fp,            /* [OUT] File object to create */
	const char *path    /* [IN]  File name to be opened */
)
{
	FRESULT fr;

	/* Opens an existing file. If not exist, creates a new file. */
	fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);

	if (fr == FR_OK) {
		/* Seek to end of the file to append data */
		fr = f_lseek(fp, f_size(fp));

		if (fr != FR_OK)
			f_close(fp);
	}

	return fr;
}

/*------------------------------------------------------------/
/ Delete a sub-directory even if it contains any file
/-------------------------------------------------------------/
/ The delete_node() function is for R0.12+.
/ It works regardless of FF_FS_RPATH.
*/
FRESULT fatfs_delete_node(
	TCHAR *path,    /* Path name buffer with the sub-directory to delete */
	UINT sz_buff,   /* Size of path name buffer (items) */
	FILINFO *fno    /* Name read buffer */
)
{
	UINT i, j;
	FRESULT fr;
	DIR dir;


	fr = f_opendir(&dir, path); /* Open the sub-directory to make it empty */

	if (fr != FR_OK)
		return fr;

	for (i = 0; path[i]; i++) ; /* Get current path length */

	path[i++] = _T('/');

	for (;;) {
		fr = f_readdir(&dir, fno);  /* Get a directory item */

		if (fr != FR_OK || !fno->fname[0]) {
			break;  /* End of directory? */
		}

		j = 0;

		do {    /* Make a path name */
			if (i + j + 1 >= sz_buff) { /* Buffer overflow check (+1 for null terminator) */
				fr = FR_NOT_ENOUGH_CORE;
				break;    /* Fails with 100 when buffer overflow */
			}

			path[i + j] = fno->fname[j];
		} while (fno->fname[j++]);

		if (fno->fattrib & AM_DIR)      /* Item is a sub-directory */
			fr = fatfs_delete_node(path, sz_buff, fno);
		else                            /* Item is a file */
			fr = f_unlink(path);

		if (fr != FR_OK)
			break;
	}

	path[--i] = 0;  /* Restore the path name */
	f_closedir(&dir);

	if (fr == FR_OK) {
		fr = f_unlink(path);  /* Delete the empty sub-directory */
	}

	return fr;
}

/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker                            /
/-----------------------------------------------------------------------/
/ WARNING: The data on the target drive will be lost!
*/

static DWORD fatfs_diskio_pseudo(        /* Pseudo random number generator */
	DWORD pns   /* 0:Initialize, !0:Read */
)
{
	static DWORD lfsr;
	UINT n;


	if (pns) {
		lfsr = pns;

		for (n = 0; n < 32; n++)
			fatfs_diskio_pseudo(0);
	}

	if (lfsr & 1) {
		lfsr >>= 1;
		lfsr ^= 0x80200003;
	} else
		lfsr >>= 1;

	return lfsr;
}

int fatfs_diskio(
	BYTE pdrv,      /* Physical drive number to be checked (all data on the drive will be lost) */
	UINT ncyc,      /* Number of test cycles */
	DWORD *buff,    /* Pointer to the working buffer */
	UINT sz_buff    /* Size of the working buffer in unit of byte */
)
{
	UINT n, cc, ns;
	DWORD sz_drv, lba, lba2, sz_eblk, pns = 1;
	WORD sz_sect;
	BYTE *pbuff = (BYTE *)buff;
	DSTATUS ds;
	DRESULT dr;


	pr_info("test_diskio(%u, %u, 0x%08X, 0x%08X)\n",
		pdrv, ncyc, (UINT)buff, sz_buff);

	if (sz_buff < FF_MAX_SS + 8) {
		pr_info("Insufficient work area to run the program.\n");
		return 1;
	}

	for (cc = 1; cc <= ncyc; cc++) {
		pr_info("**** Test cycle %u of %u start ****\n",
			cc, ncyc);

		pr_info(" disk_initalize(%u)\n", pdrv);
		ds = disk_initialize(pdrv);

		if (ds & STA_NOINIT) {
			pr_info(" - failed.\n");
			return 2;
		} else
			pr_info(" - ok.\n");

		pr_info("**** Get drive size ****\n");
		pr_info(" disk_ioctl(%u, GET_SECTOR_COUNT, 0x%08X)\n",
			pdrv, (UINT)&sz_drv);
		sz_drv = 0;
		dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 3;
		}

		if (sz_drv < 128) {
			pr_info("Failed: Insufficient drive size to test.\n");
			return 4;
		}

		pr_info(" Number of sectors on the drive %u is %u.\n",
			pdrv, sz_drv);

#if FF_MAX_SS != FF_MIN_SS
		pr_info("**** Get sector size ****\n");
		pr_info(" disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)\n",
			pdrv, (UINT)&sz_sect);
		sz_sect = 0;
		dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 5;
		}

		pr_info(" Size of sector is %u bytes.\n", sz_sect);
#else
		sz_sect = FF_MAX_SS;
#endif

		pr_info("**** Get block size ****\n");
		pr_info(" disk_ioctl(%u, GET_BLOCK_SIZE, 0x%X)\n",
			pdrv, (UINT)&sz_eblk);
		sz_eblk = 0;
		dr = disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else
			pr_info(" - failed.\n");

		if (dr == RES_OK || sz_eblk >= 2)
			pr_info(" Size of the erase block is %u sectors.\n", sz_eblk);
		else
			pr_info(" Size of the erase block is unknown.\n");

		/* Single sector write test */
		pr_info("**** Single sector write test ****\n");
		lba = 0;

		for (n = 0, fatfs_diskio_pseudo(pns); n < sz_sect; n++)
			pbuff[n] = (BYTE)fatfs_diskio_pseudo(0);

		pr_info(" disk_write(%u, 0x%X, %u, 1)\n",
			pdrv, (UINT)pbuff, lba);
		dr = disk_write(pdrv, pbuff, lba, 1);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 6;
		}

		pr_info(" disk_ioctl(%u, CTRL_SYNC, NULL)\n", pdrv);
		dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 7;
		}

		memset(pbuff, 0, sz_sect);
		pr_info(" disk_read(%u, 0x%X, %u, 1)\n",
			pdrv, (UINT)pbuff, lba);
		dr = disk_read(pdrv, pbuff, lba, 1);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 8;
		}

		for (n = 0, fatfs_diskio_pseudo(pns);
			n < sz_sect && pbuff[n] == (BYTE)fatfs_diskio_pseudo(0);
			n++) ;

		if (n == sz_sect)
			pr_info(" Read data matched.\n");
		else {
			pr_info(" Read data differs from the data written.\n");
			return 10;
		}

		pns++;

		pr_info("**** Multiple sector write test ****\n");
		lba = 5;
		ns = sz_buff / sz_sect;

		if (ns > 4)
			ns = 4;

		if (ns > 1) {
			for (n = 0, fatfs_diskio_pseudo(pns); n < (UINT)(sz_sect * ns); n++)
				pbuff[n] = (BYTE)fatfs_diskio_pseudo(0);

			pr_info(" disk_write(%u, 0x%X, %u, %u)\n",
				pdrv, (UINT)pbuff, lba, ns);
			dr = disk_write(pdrv, pbuff, lba, ns);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 11;
			}

			pr_info(" disk_ioctl(%u, CTRL_SYNC, NULL)\n", pdrv);
			dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 12;
			}

			memset(pbuff, 0, sz_sect * ns);
			pr_info(" disk_read(%u, 0x%X, %u, %u)\n",
				pdrv, (UINT)pbuff, lba, ns);
			dr = disk_read(pdrv, pbuff, lba, ns);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 13;
			}

			for (n = 0, fatfs_diskio_pseudo(pns);
				n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)fatfs_diskio_pseudo(0);
				n++) ;

			if (n == (UINT)(sz_sect * ns))
				pr_info(" Read data matched.\n");
			else {
				pr_info(" Read data differs from the data written.\n");
				return 14;
			}
		} else
			pr_info(" Test skipped.\n");

		pns++;

		pr_info("**** Single sector write test (unaligned buffer address) ****\n");
		lba = 5;

		for (n = 0, fatfs_diskio_pseudo(pns); n < sz_sect; n++)
			pbuff[n + 3] = (BYTE)fatfs_diskio_pseudo(0);

		pr_info(" disk_write(%u, 0x%X, %u, 1)\n",
			pdrv, (UINT)(pbuff + 3), lba);
		dr = disk_write(pdrv, pbuff + 3, lba, 1);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 15;
		}

		pr_info(" disk_ioctl(%u, CTRL_SYNC, NULL)\n", pdrv);
		dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 16;
		}

		memset(pbuff + 5, 0, sz_sect);
		pr_info(" disk_read(%u, 0x%X, %u, 1)\n",
			pdrv, (UINT)(pbuff + 5), lba);
		dr = disk_read(pdrv, pbuff + 5, lba, 1);

		if (dr == RES_OK)
			pr_info(" - ok.\n");
		else {
			pr_info(" - failed.\n");
			return 17;
		}

		for (n = 0, fatfs_diskio_pseudo(pns);
			n < sz_sect && pbuff[n + 5] == (BYTE)fatfs_diskio_pseudo(0);
			n++) ;

		if (n == sz_sect)
			pr_info(" Read data matched.\n");
		else {
			pr_info(" Read data differs from the data written.\n");
			return 18;
		}

		pns++;

		pr_info("**** 4GB barrier test ****\n");

		if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2)) {
			lba = 6;
			lba2 = lba + 0x80000000 / (sz_sect / 2);

			for (n = 0, fatfs_diskio_pseudo(pns); n < (UINT)(sz_sect * 2); n++)
				pbuff[n] = (BYTE)fatfs_diskio_pseudo(0);

			pr_info(" disk_write(%u, 0x%X, %u, 1)\n",
				pdrv, (UINT)pbuff, lba);
			dr = disk_write(pdrv, pbuff, lba, 1);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 19;
			}

			pr_info(" disk_write(%u, 0x%X, %u, 1)\n",
				pdrv, (UINT)(pbuff + sz_sect), lba2);
			dr = disk_write(pdrv, pbuff + sz_sect, lba2, 1);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 20;
			}

			pr_info(" disk_ioctl(%u, CTRL_SYNC, NULL)\n", pdrv);
			dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 21;
			}

			memset(pbuff, 0, sz_sect * 2);
			pr_info(" disk_read(%u, 0x%X, %u, 1)\n", pdrv, (UINT)pbuff, lba);
			dr = disk_read(pdrv, pbuff, lba, 1);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 22;
			}

			pr_info(" disk_read(%u, 0x%X, %u, 1)\n", pdrv, (UINT)(pbuff + sz_sect), lba2);
			dr = disk_read(pdrv, pbuff + sz_sect, lba2, 1);

			if (dr == RES_OK)
				pr_info(" - ok.\n");
			else {
				pr_info(" - failed.\n");
				return 23;
			}

			for (n = 0, fatfs_diskio_pseudo(pns);
				pbuff[n] == (BYTE)fatfs_diskio_pseudo(0) && n < (UINT)(sz_sect * 2);
				n++) ;

			if (n == (UINT)(sz_sect * 2))
				pr_info(" Read data matched.\n");
			else {
				pr_info(" Read data differs from the data written.\n");
				return 24;
			}
		} else
			pr_info(" Test skipped.\n");

		pns++;

		pr_info("**** Test cycle %u of %u completed ****\n", cc, ncyc);
	}

	return 0;
}

/*----------------------------------------------------------------------/
/ Test if the file is contiguous                                        /
/----------------------------------------------------------------------*/

FRESULT fatfs_contiguous_file(
	FIL *fp,    /* [IN]  Open file object to be checked */
	int *cont   /* [OUT] 1:Contiguous, 0:Fragmented or zero-length */
)
{
	DWORD clst, clsz, step;
	FSIZE_t fsz;
	FRESULT fr;

	*cont = 0;
	/* Validates and prepares the file */
	fr = f_lseek(fp, 0);

	if (fr != FR_OK)
		return fr;

#if FF_MAX_SS == FF_MIN_SS
	clsz = (DWORD)fp->obj.fs->csize * FF_MAX_SS;    /* Cluster size */
#else
	clsz = (DWORD)fp->obj.fs->csize * fp->obj.fs->ssize;
#endif
	fsz = fp->obj.objsize;

	if (fsz > 0) {
		/* A cluster leading the first cluster for first test */
		clst = fp->obj.sclust - 1;

		while (fsz) {
			step = (fsz >= clsz) ? clsz : (DWORD)fsz;
			/* Advances file pointer a cluster */
			fr = f_lseek(fp, f_tell(fp) + step);

			if (fr != FR_OK)
				return fr;

			/* Is not the cluster next to previous one? */
			if (clst + 1 != fp->clust)
				break;

			clst = fp->clust;
			/* Get current cluster for next test */
			fsz -= step;
		}

		/* All done without fail? */
		if (fsz == 0)
			*cont = 1;
	}

	return FR_OK;
}

/*---------------------------------------------------------------------*/
/* Raw Read/Write Throughput Checker                                   */
/*---------------------------------------------------------------------*/
int fatfs_raw_speed(
	BYTE pdrv,      /* Physical drive number */
	DWORD lba,      /* Start LBA for read/write test */
	DWORD len,      /* Number of bytes to read/write (must be multiple of sz_buff) */
	void *buff,     /* Read/write buffer */
	UINT sz_buff    /* Size of read/write buffer (must be multiple of FF_MAX_SS) */
)
{
	WORD ss;
	DWORD ofs, tmr;

#if FF_MIN_SS != FF_MAX_SS

	if (disk_ioctl(pdrv, GET_SECTOR_SIZE, &ss) != RES_OK) {
		pr_info("disk_ioctl() failed.\n");
		return -EINVAL;
	}

#else
	ss = FF_MAX_SS;
#endif

	pr_info("Starting raw write test at sector %u "
		"in %u bytes of data chunks...\n", lba, sz_buff);
	tmr = ktime_get();

	for (ofs = 0; ofs < len / ss; ofs += sz_buff / ss) {
		if (disk_write(pdrv, buff, lba + ofs, sz_buff / ss) != RES_OK) {
			pr_info("disk_write() failed.\n");
			return -EINVAL;
		}
	}

	if (disk_ioctl(pdrv, CTRL_SYNC, 0) != RES_OK) {
		pr_info("disk_ioctl() failed.\n");
		return -EINVAL;
	}

	tmr = ktime_get() - tmr;
	pr_info("%u bytes written and it took %u us (%.3f)B/s.\n",
		len, tmr, (float)len * 1000 / tmr);

	pr_info("Starting raw read test at sector %u "
		"in %u bytes of data chunks...\n", lba, sz_buff);
	tmr = ktime_get();

	for (ofs = 0; ofs < len / ss; ofs += sz_buff / ss) {
		if (disk_read(pdrv, buff, lba + ofs, sz_buff / ss) != RES_OK) {
			pr_info("disk_read() failed.\n");
			return -EINVAL;
		}
	}

	tmr = ktime_get() - tmr;
	pr_info("%u bytes read and it took %u us (%.3f)B/s.\n",
		len, tmr, (float)len * 1000 / tmr);
	pr_info("Test completed.\n");

	return 0;
}

#ifdef DESIGN_VERIFICATION_FATFS
#include "kinetis/test-kinetis.h"

int t_fatfs_truncate(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT bw;
	const char *test_data = "This is a long string that will be truncated";

	f_mount(&fs, "", 0);

	/* Create and write a file */
	fr = f_open(&fil, "truncate.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}

	fr = f_write(&fil, test_data, strlen(test_data), &bw);
	if (fr || bw != strlen(test_data)) {
		pr_err("Failed to write file\n");
		f_close(&fil);
		return -EACCES;
	}

	/* Truncate to first 10 bytes */
	fr = f_lseek(&fil, 10);
	if (fr) {
		pr_err("Failed to seek\n");
		f_close(&fil);
		return -EACCES;
	}

	fr = f_truncate(&fil);
	if (fr) {
		pr_err("Failed to truncate file: %d\n", fr);
		f_close(&fil);
		return -EACCES;
	}

	pr_info("File truncated from %u to 10 bytes\n", bw);
	f_close(&fil);

	return 0;
}

#if !FF_FS_READONLY
int t_fatfs_sync(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT bw;
	const char *test_data = "Data to be synced";

	f_mount(&fs, "", 0);

	/* Create and write a file */
	fr = f_open(&fil, "sync.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}

	fr = f_write(&fil, test_data, strlen(test_data), &bw);
	if (fr) {
		pr_err("Failed to write file\n");
		f_close(&fil);
		return -EACCES;
	}

	/* Sync the file */
	fr = f_sync(&fil);
	if (fr) {
		pr_err("Failed to sync file: %d\n", fr);
		f_close(&fil);
		return -EACCES;
	}

	pr_info("File synced successfully (%u bytes written)\n", bw);
	f_close(&fil);

	return 0;
}
#endif

#if FF_USE_CHMOD
int t_fatfs_chmod(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	FILINFO fno;

	f_mount(&fs, "", 0);

	/* Create a test file */
	fr = f_open(&fil, "chmod.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}
	f_close(&fil);

	/* Set file to read-only */
	fr = f_chmod("chmod.txt", AM_RDO, AM_RDO);
	if (fr) {
		pr_err("Failed to set read-only attribute: %d\n", fr);
		return -EACCES;
	}

	/* Check the attribute */
	fr = f_stat("chmod.txt", &fno);
	if (fr == FR_OK) {
		pr_info("File attributes: %s\n",
			(fno.fattrib & AM_RDO) ? "Read-only" : "Normal");
	}

	/* Reset to normal */
	fr = f_chmod("chmod.txt", 0, AM_RDO);
	if (fr) {
		pr_err("Failed to reset attribute: %d\n", fr);
		return -EACCES;
	}

	pr_info("File attribute test passed\n");
	return 0;
}
#endif

#if FF_USE_CHMOD
int t_fatfs_utime(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	FILINFO fno;

	f_mount(&fs, "", 0);

	/* Create a test file */
	fr = f_open(&fil, "utime.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}
	f_close(&fil);

	/* Change timestamp */
	fno.fdate = (2025 - 1980) << 9 | 3 << 5 | 15;  /* 2025/03/15 */
	fno.ftime = 10 << 11 | 30 << 5;              /* 10:30:00 */

	fr = f_utime("utime.txt", &fno);
	if (fr) {
		pr_err("Failed to change timestamp: %d\n", fr);
		return -EACCES;
	}

	pr_info("File timestamp changed to 2025/03/15 10:30\n");
	return 0;
}
#endif

#if FF_FS_RPATH >= 2
int t_fatfs_chdir(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	TCHAR path[256];

	f_mount(&fs, "", 0);

	/* Create a directory */
	fr = f_mkdir("testdir");
	if (fr && fr != FR_EXIST) {
		pr_err("Failed to create directory: %d\n", fr);
		return -EACCES;
	}

	/* Change to the directory */
	fr = f_chdir("testdir");
	if (fr) {
		pr_err("Failed to change directory: %d\n", fr);
		return -EACCES;
	}

	/* Get current directory */
	fr = f_getcwd(path, sizeof(path));
	if (fr) {
		pr_err("Failed to get current directory: %d\n", fr);
		return -EACCES;
	}

	pr_info("Current directory: %s\n", path);

	/* Change back to root */
	fr = f_chdir("/");
	if (fr) {
		pr_err("Failed to change to root directory\n");
		return -EACCES;
	}

	return 0;
}
#endif

#if FF_USE_FIND
int t_fatfs_find(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	DIR dir;
	FILINFO fno;

	f_mount(&fs, "", 0);

	/* Find all .txt files */
	fr = f_findfirst(&dir, &fno, "", "*.txt");
	if (fr == FR_OK) {
		pr_info("Files matching *.txt:\n");
		while (fr == FR_OK && fno.fname[0]) {
			pr_info("  %s\n", fno.fname);
			fr = f_findnext(&dir, &fno);
		}
		f_closedir(&dir);
		pr_info("File find test completed\n");
		return 0;
	}

	pr_err("File find failed: %d\n", fr);
	return -EACCES;
}
#endif

#if FF_USE_LABEL
int t_fatfs_label(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	TCHAR label[64];
	DWORD vsn;

	f_mount(&fs, "", 0);

	/* Set volume label */
	fr = f_setlabel("KINETIS");
	if (fr) {
		pr_err("Failed to set volume label: %d\n", fr);
		return -EACCES;
	}

	pr_info("Volume label set to: KINETIS\n");

	/* Get volume label */
	fr = f_getlabel("", label, &vsn);
	if (fr) {
		pr_err("Failed to get volume label: %d\n", fr);
		return -EACCES;
	}

	pr_info("Volume label: %s, Serial: 0x%08lX\n", label, vsn);
	return 0;
}
#endif

#if FF_USE_STRFUNC
int t_fatfs_char_io(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT i;
	TCHAR str[] = "Hello";

	f_mount(&fs, "", 0);

	/* Test f_putc */
	fr = f_open(&fil, "char_io.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}

	for (i = 0; i < strlen(str); i++) {
		f_putc(str[i], &fil);
	}
	f_close(&fil);

	pr_info("Character I/O test completed\n");
	return 0;
}
#endif

int t_fatfs_file_status(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT bw;
	const char *test_data = "Test data for status check";

	f_mount(&fs, "", 0);

	/* Create a test file */
	fr = f_open(&fil, "status.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}

	fr = f_write(&fil, test_data, strlen(test_data), &bw);
	if (fr) {
		pr_err("Failed to write file\n");
		f_close(&fil);
		return -EACCES;
	}

	/* Test file status functions */
	pr_info("File position (f_tell): %lu\n", f_tell(&fil));
	pr_info("File size (f_size): %lu\n", f_size(&fil));
	pr_info("EOF status (f_eof): %d\n", f_eof(&fil));
	pr_info("Error status (f_error): %d\n", f_error(&fil));

	/* Seek to end and check EOF */
	f_lseek(&fil, f_size(&fil));
	pr_info("After seek to end, EOF status (f_eof): %d\n", f_eof(&fil));

	f_close(&fil);
	return 0;
}

int t_fatfs_unmount(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;

	/* Mount the filesystem */
	fr = f_mount(&fs, "", 0);
	if (fr) {
		pr_err("Failed to mount filesystem: %d\n", fr);
		return -EACCES;
	}
	pr_info("Filesystem mounted\n");

	/* Unmount the filesystem */
	fr = f_mount(NULL, "", 0);
	if (fr) {
		pr_err("Failed to unmount filesystem: %d\n", fr);
		return -EACCES;
	}
	pr_info("Filesystem unmounted successfully\n");

	return 0;
}

#if FF_USE_STRFUNC
int t_fatfs_gets(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT bw;
	TCHAR buffer[64];
	const char *test_data = "Line1\nLine2\nLine3\n";

	f_mount(&fs, "", 0);

	/* Create and write a test file */
	fr = f_open(&fil, "gets.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}

	fr = f_write(&fil, test_data, strlen(test_data), &bw);
	if (fr) {
		pr_err("Failed to write file\n");
		f_close(&fil);
		return -EACCES;
	}
	f_close(&fil);

	/* Read file line by line */
	fr = f_open(&fil, "gets.txt", FA_READ);
	if (fr) {
		pr_err("Failed to open file: %d\n", fr);
		return -EACCES;
	}

	pr_info("Reading file line by line:\n");
	while (f_gets(buffer, sizeof(buffer), &fil)) {
		pr_info("  %s", buffer);
	}

	f_close(&fil);
	return 0;
}
#endif

#if FF_USE_FORWARD
int t_fatfs_forward(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT bw, bf;
	UINT total_bytes = 0;
	BYTE src_data[256];
	UINT i;

	f_mount(&fs, "", 0);

	/* Prepare test data */
	for (i = 0; i < sizeof(src_data); i++)
		src_data[i] = (BYTE)i;

	/* Create and write a test file */
	fr = f_open(&fil, "forward.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fr) {
		pr_err("Failed to create file: %d\n", fr);
		return -EACCES;
	}

	fr = f_write(&fil, src_data, sizeof(src_data), &bw);
	if (fr) {
		pr_err("Failed to write file\n");
		f_close(&fil);
		return -EACCES;
	}
	f_close(&fil);

	/* Forward data to a callback function */
	fr = f_open(&fil, "forward.txt", FA_READ);
	if (fr) {
		pr_err("Failed to open file: %d\n", fr);
		return -EACCES;
	}

	/* Forward all data in chunks */
	fr = f_forward(&fil, (UINT(*)(const BYTE*, UINT))NULL, sizeof(src_data), &bf);
	if (fr) {
		pr_err("Forward failed (expected with NULL callback): %d\n", fr);
	}

	f_close(&fil);
	pr_info("Forward test completed\n");
	return 0;
}
#endif

#if FF_CODE_PAGE == 0
int t_fatfs_setcp(int argc, char **argv)
{
	FRESULT fr;

	/* Set code page to UTF-8 (code page 437) */
	fr = f_setcp(437);
	if (fr) {
		pr_err("Failed to set code page: %d\n", fr);
		return -EACCES;
	}

	pr_info("Code page set to 437 (OEM US)\n");
	return 0;
}
#endif

#if FF_MULTI_PARTITION
int t_fatfs_disk_partition(int argc, char **argv)
{
	FRESULT fr;
	BYTE pdrv = 0;
	LBA_t ptbl[4];
	BYTE work[FF_MAX_SS];

	/* Partition table: divide drive into 4 equal partitions */
	ptbl[0] = 100;   /* Partition 1 starts at sector 100 */
	ptbl[1] = 200;   /* Partition 2 starts at sector 200 */
	ptbl[2] = 300;   /* Partition 3 starts at sector 300 */
	ptbl[3] = 400;   /* Partition 4 starts at sector 400 */

	fr = f_fdisk(pdrv, ptbl, work);
	if (fr) {
		pr_err("Failed to partition disk: %d\n", fr);
		return -EACCES;
	}

	pr_info("Disk partitioned successfully into 4 partitions\n");
	pr_info("  Partition 1: starts at sector %lu\n", ptbl[0]);
	pr_info("  Partition 2: starts at sector %lu\n", ptbl[1]);
	pr_info("  Partition 3: starts at sector %lu\n", ptbl[2]);
	pr_info("  Partition 4: starts at sector %lu\n", ptbl[3]);

	return 0;
}
#endif


int t_fatfs_operate(int argc, char **argv)
{
	int ret = 0;

	if (argc > 1) {
		if (!strcmp(argv[1], "mount"))
			ret = f_mount(&disk_fatfs, (TCHAR const *)disk_path, 0);
		else if (!strcmp(argv[1], "mkfs"))
			ret = f_mkfs((const TCHAR *)disk_path, 0, work_buffer, sizeof(work_buffer));
		else
			return -EINVAL;
	}

	if (ret) {
		process_fatfs_err(ret);
		return ret;
	}

	return 0;
}

int t_fatfs_loopback(int argc, char **argv)
{
	FRESULT res;                                         /* FatFs function common result code */
	MKFS_PARM opt;
	uint32_t byteswritten, bytesread;                    /* File write/read counts */
	uint8_t wtext[] = "This is FatFs working with test";/* File write buffer */
	uint8_t rtext[100];                                  /* File read buffer */

	/*##-1- Link the  disk I/O driver #######################################*/
#ifdef KINETIS_FAKE_SIM
	if (FATFS_LinkDriver(&fake_ram_disk_driver, disk_path) == 0) {
#else
	if (FATFS_LinkDriver(&SD_Driver, disk_path) == 0) {
#endif
		/*##-2- Register the file system object to the FatFs module ##############*/
		res = f_mount(&disk_fatfs, (TCHAR const *)disk_path, 0);

		if (res != FR_OK) {
			/* FatFs Initialization Error */
			process_fatfs_err(res);
		} else {
			/*##-3- Create a FAT file system (format) on the logical drive #########*/
			if (res == FR_NO_FILESYSTEM) {
				opt.fmt =  FM_FAT32;
				opt.n_fat = 1;
				opt.align = 4;
				opt.n_root = 1;
				opt.au_size = 1;

				res = f_mkfs((const TCHAR *)disk_path, &opt, work_buffer, sizeof(work_buffer));
			}

			if (res != FR_OK) {
				/* FatFs Format Error */
				process_fatfs_err(res);
			} else {
				/*##-4- Create and Open a new text file object with write access #####*/
				res = f_open(&test_file, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);

				if (res != FR_OK) {
					/* 'test.txt' file Open for write Error */
					process_fatfs_err(res);
				} else {
					/*##-5- Write data to the text file ################################*/
					res = f_write(&test_file, wtext, sizeof(wtext), (void *)&byteswritten);

					if ((byteswritten == 0) || (res != FR_OK)) {
						/* 'test.txt' file Write or EOF Error */
						process_fatfs_err(res);
					} else {
						/*##-6- Close the open text file #################################*/
						f_close(&test_file);

						/*##-7- Open the text file object with read access ###############*/
						res = f_open(&test_file, "test.txt", FA_READ);

						if (res != FR_OK) {
							/* 'test.txt' file Open for read Error */
							process_fatfs_err(res);
						} else {
							/*##-8- Read data from the text file ###########################*/
							res = f_read(&test_file, rtext, sizeof(rtext), (void *)&bytesread);

							if ((bytesread == 0) || (res != FR_OK)) {
								/* 'test.txt' file Read or EOF Error */
								process_fatfs_err(res);
							} else {
								/*##-9- Close the open text file #############################*/
								f_close(&test_file);

								/*##-10- Compare read data with the expected data ############*/
								if ((bytesread != byteswritten)) {
									/* Read data is different from the expected data */
									process_fatfs_err(res);
								} else {
									/* Success of the demo: no error occurrence */
									pr_info("FatFs TEST PASS\n");
								}
							}
						}
					}
				}
			}
		}
	}

	/*##-11- Unlink the  disk I/O driver ####################################*/
	FATFS_UnLinkDriver(disk_path);

	return 0;
}

int t_fatfs_miscellaneous(int argc, char **argv)
{
	FRESULT res;

	/* FatFs multifunctional test. */
	res = fatfs_miscellaneous();

	if (res == FR_OK)
		return 0;
	else
		return -EACCES;
}

int t_fatfs_file_check(int argc, char **argv)
{
	static FILINFO finfo;
	FRESULT res;

	/* Get file information */
	res = f_stat("TestDir/testdir.txt", &finfo);

	if (res == FR_OK) {
		pr_info("testdir.txt File information\n");
		pr_info("The file size: %ud(B)\n", finfo.fsize);
		pr_info("The time stamp: %u/%02u/%02u, %02u:%02u\n",
			(finfo.fdate >> 9) + 1980,
			finfo.fdate >> 5 & 15,
			finfo.fdate & 31,
			finfo.ftime >> 11,
			finfo.ftime >> 5 & 63);
		pr_info("attribute: %c%c%c%c%c\n",
			(finfo.fattrib & AM_DIR) ? 'D' : '-',      // Directory
			(finfo.fattrib & AM_RDO) ? 'R' : '-',      // A read-only file
			(finfo.fattrib & AM_HID) ? 'H' : '-',      // Hidden files
			(finfo.fattrib & AM_SYS) ? 'S' : '-',      // System files
			(finfo.fattrib & AM_ARC) ? 'A' : '-');     // Archive
		return 0;
	}

	return -EACCES;
}

int t_fatfs_scan_files(int argc, char **argv)
{
	pr_info("Document scanning test.\n");
	strcpy(disk_path, "1:");
	fatfs_scan_files(disk_path);

	return 0;
}

int t_fatfs_append(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;

	/* Open or create a log file and ready to append */
	f_mount(&fs, "", 0);
	fr = fatfs_open_append(&fil, "logfile.txt");

	if (fr != FR_OK)
		return -EACCES;

	/* Append a line */
	f_printf(&fil, "%s\n", get_rtc_string(&general_rtc));

	/* Close the file */
	f_close(&fil);

	return 0;
}

int t_fatfs_delete_node(int argc, char **argv)  /* How to use */
{
	FRESULT fr;
	FATFS fs;
	TCHAR buff[256];
	FILINFO fno;

	f_mount(&fs, _T("5:"), 0);

	/* Directory to be deleted */
	strncpy(buff, _T("5:dir"), sizeof(buff) - 1);
	buff[sizeof(buff) - 1] = '\0';  // Ensure null termination

	/* Delete the directory */
	fr = fatfs_delete_node(buff, sizeof buff / sizeof buff[0], &fno);

	/* Check the result */
	if (fr) {
		pr_info(_T("Failed to delete the directory. (%u)"), fr);
		return -EACCES;
	} else {
		pr_info(_T("The directory and "
				"the contents have successfully been deleted."));
		return 0;
	}
}

/*----------------------------------------------------------------------/
/ Allocate a contiguous area to the file
/-----------------------------------------------------------------------/
/ This function checks if the file is contiguous with desired size.
/ If not, a block of contiguous sectors is allocated to the file.
/ If the file has been opened without FA_WRITE flag, it only checks if
/ the file is contiguous and returns the result.
/-----------------------------------------------------------------------/
/ This function can work with FatFs R0.09 - R0.11a.
/ It is incompatible with R0.12+. Use f_expand function instead.
/----------------------------------------------------------------------*/
int t_fatfs_expand(int argc, char **argv)
{
	FRESULT fr;
	DRESULT dr;
	FATFS fs;
	FIL fil;
	DWORD org;

	/* Open or create a file to write */
	f_mount(&fs, "", 0);
	fr = f_open(&fil, "fastrec.log", FA_READ | FA_WRITE | FA_OPEN_ALWAYS);

	if (fr)
		return -EACCES;

	/* Check if the file is 256MB in size and occupies a contiguous area.
	/  If not, a contiguous area will be re-allocated to the file. */
	org = f_expand(&fil, 0x10000000, 1);

	if (!org) {
		pr_info("Function failed due to any error or "
			"insufficient contiguous area.\n");
		f_close(&fil);
		return -EACCES;
	}

	/*
	 * Now you can read/write the file without filesystem layer.
	 * Write 512KiB from top of the file
	*/
	dr = disk_write(fs.pdrv, work_buffer, org, 1024);

	f_close(&fil);

	if (dr != RES_OK)
		return -EACCES;

	return 0;
}

int t_fatfs_diskio(int argc, char **argv)
{
	int rc;
	DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */

	/* Check function/compatibility of the physical drive #0 */
	rc = fatfs_diskio(0, 3, buff, sizeof buff);

	if (rc) {
		pr_info("Sorry the function/compatibility test failed. (rc=%d)\n"
			"FatFs will not work with this disk driver.\n", rc);
		return -EACCES;
	} else {
		pr_info("Congratulations! The disk driver works well.\n");
		return 0;
	}
}

int t_fatfs_contiguous_file(int argc, char **argv)
{
	int ret;
	int cont;
	/* Check function/compatibility of the physical drive #0 */
	ret = fatfs_contiguous_file(&test_file, &cont);

	if (ret)
		return -EACCES;
	else
		return 0;
}

int t_fatfs_raw_speed(int argc, char **argv)
{
	int ret;
	DWORD buff[FF_MAX_SS * 2];  /* Working buffer (4 sector in size) */

	/* Check function/compatibility of the physical drive #0 */
	ret = fatfs_raw_speed(0, 0, 2 * sizeof(buff), buff, sizeof(buff));

	if (ret)
		return -EACCES;
	else
		return 0;
}

int t_fatfs_speed(int argc, char **argv)
{
	FRESULT fr;
	FATFS fs;
	FIL fil;
	UINT bw, br;
	DWORD tmr;
	DWORD file_size_kb = 1024;  /* Default 1MB */
	DWORD file_size_bytes;
	DWORD block_size = 8192;    /* 8KB block size */
	BYTE *buffer;
	UINT i, blocks;
	bool is_read = false;
	const char *filename = "speed_test.bin";

	/* Parse command line arguments */
	if (argc < 2) {
		pr_info("Usage: t_fatfs_speed <read|write> [size_kb]\n");
		pr_info("  Example:\n");
		pr_info("    t_fatfs_speed read      - Test read speed (default 1MB)\n");
		pr_info("    t_fatfs_speed write     - Test write speed (default 1MB)\n");
		pr_info("    t_fatfs_speed read 2048 - Test read speed with 2MB\n");
		pr_info("    t_fatfs_speed write 512 - Test write speed with 512KB\n");
		return -EINVAL;
	}

	/* Parse operation type */
	if (!strcmp(argv[1], "read")) {
		is_read = true;
	} else if (!strcmp(argv[1], "write")) {
		is_read = false;
	} else {
		pr_info("Invalid operation: %s (must be 'read' or 'write')\n", argv[1]);
		return -EINVAL;
	}

	/* Parse file size (optional) */
	if (argc > 2) {
		file_size_kb = simple_strtoul(argv[2], NULL, 10);
		if (file_size_kb == 0) {
			pr_info("Invalid file size: %s\n", argv[2]);
			return -EINVAL;
		}
	}

	file_size_bytes = file_size_kb * 1024;

	/* Allocate buffer */
	buffer = kmalloc(block_size, GFP_KERNEL);
	if (!buffer) {
		pr_info("Failed to allocate buffer\n");
		return -ENOMEM;
	}

	/* Initialize buffer for write */
	if (!is_read) {
		for (i = 0; i < block_size; i++)
			buffer[i] = (BYTE)(i & 0xFF);
	}

	f_mount(&fs, "", 0);

	if (!is_read) {
		/* Write speed test */
		pr_info("=== File Write Speed Test ===\n");
		pr_info("File size: %lu KB (%lu bytes)\n", file_size_kb, file_size_bytes);
		pr_info("Block size: %lu KB\n", block_size / 1024);

		/* Create file */
		fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr) {
			pr_info("Failed to create file: %d\n", fr);
			kfree(buffer);
			return -EACCES;
		}

		/* Write file */
		blocks = file_size_bytes / block_size;
		tmr = ktime_get();

		for (i = 0; i < blocks; i++) {
			fr = f_write(&fil, buffer, block_size, &bw);
			if (fr || bw != block_size) {
				pr_info("Write error at block %u\n", i);
				f_close(&fil);
				kfree(buffer);
				return -EIO;
			}
		}

		tmr = ktime_get() - tmr;
		f_close(&fil);

		/* Calculate and print results */
		pr_info("Write completed:\n");
		pr_info("  Total bytes: %lu\n", file_size_bytes);
		pr_info("  Time: %lu us\n", tmr);
		pr_info("  Speed: %.3f MB/s\n", (float)file_size_bytes * 1000 / tmr / 1024 / 1024);

	} else {
		/* Read speed test */
		pr_info("=== File Read Speed Test ===\n");
		pr_info("File size: %lu KB (%lu bytes)\n", file_size_kb, file_size_bytes);
		pr_info("Block size: %lu KB\n", block_size / 1024);

		/* Open file */
		fr = f_open(&fil, filename, FA_READ);
		if (fr) {
			pr_info("Failed to open file: %d (file may not exist, run write test first)\n", fr);
			kfree(buffer);
			return -EACCES;
		}

		/* Read file */
		blocks = file_size_bytes / block_size;
		tmr = ktime_get();

		for (i = 0; i < blocks; i++) {
			fr = f_read(&fil, buffer, block_size, &br);
			if (fr || br != block_size) {
				pr_info("Read error at block %u\n", i);
				f_close(&fil);
				kfree(buffer);
				return -EIO;
			}
		}

		tmr = ktime_get() - tmr;
		f_close(&fil);

		/* Calculate and print results */
		pr_info("Read completed:\n");
		pr_info("  Total bytes: %lu\n", file_size_bytes);
		pr_info("  Time: %lu us\n", tmr);
		pr_info("  Speed: %.3f MB/s\n", (float)file_size_bytes * 1000 / tmr / 1024 / 1024);
	}

	kfree(buffer);
	return 0;
}

#endif

