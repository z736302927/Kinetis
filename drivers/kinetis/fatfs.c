#include "kinetis/fatfs.h"
#include "kinetis/real-time-clock.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"

#include "fs/fatfs/diskio.h"     /* Declarations of disk functions */
#include "fs/fatfs/ff_gen_drv.h"
#include "fs/fatfs/drivers/flash_diskio.h"

#include <linux/printk.h>
#include <linux/errno.h>

#include <stdio.h>
#include <string.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */
/* File system object for  disk logical drive */
static FATFS disk_fatfs;
/* File object */
static FIL my_file;
/*  disk logical drive path */
static char disk_path[4];
/* a work buffer for the f_mkfs() */
static uint8_t work_buffer[FF_MAX_SS];

int fatfs_init(void)
{
    int ret;

    /*## FatFS: Link the SD driver ###########################*/
    ret = FATFS_LinkDriver(&flash_disk_driver, disk_path);

    /* additional user code for init */
    if (ret == 0) {
        FRESULT res;

        res = f_mount(&disk_fatfs, (TCHAR const *)disk_path, 0);

        if (res == FR_NO_FILESYSTEM) {
            res = f_mkfs((const TCHAR *)disk_path, 0, work_buffer, sizeof(work_buffer));

            if (res != FR_OK) {
                printf_fatfs_err(res);
                return -ENOSYS;
            }
        }
    } else {
        printk(KERN_ERR "Failed to link low level driver.");
        return -ENOSYS;
    }
    
    return 0;
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void printf_fatfs_err(FRESULT fresult)
{
    switch (fresult) {
        case FR_OK:                   //(0)
            printk(KERN_DEBUG "Operation successful.\n");
            break;

        case FR_DISK_ERR:             //(1)
            printk(KERN_DEBUG "Hardware input/output driver error.\n");
            break;

        case FR_INT_ERR:              //(2)
            printk(KERN_DEBUG "Assertion error.\n");
            break;

        case FR_NOT_READY:            //(3)
            printk(KERN_DEBUG "The physical device doesn't work.\n");
            break;

        case FR_NO_FILE:              //(4)
            printk(KERN_DEBUG "Unable to locate file.\n");
            break;

        case FR_NO_PATH:              //(5)
            printk(KERN_DEBUG "Unable to find path.\n");
            break;

        case FR_INVALID_NAME:         //(6)
            printk(KERN_DEBUG "Invalid path name.\n");
            break;

        case FR_DENIED:               //(7)
        case FR_EXIST:                //(8)
            printk(KERN_DEBUG "Access denied.\n");
            break;

        case FR_INVALID_OBJECT:       //(9)
            printk(KERN_DEBUG "Invalid file or path.\n");
            break;

        case FR_WRITE_PROTECTED:      //(10)
            printk(KERN_DEBUG "Logical device write protection.\n");
            break;

        case FR_INVALID_DRIVE:        //(11)
            printk(KERN_DEBUG "Invalid logic device.\n");
            break;

        case FR_NOT_ENABLED:          //(12)
            printk(KERN_DEBUG "Invalid workspace.\n");
            break;

        case FR_NO_FILESYSTEM:        //(13)
            printk(KERN_DEBUG "Invalid file system.\n");
            break;

        case FR_MKFS_ABORTED:         //(14)
            printk(KERN_DEBUG "The f_mkfs function failed because of a function parameter problem.\n");
            break;

        case FR_TIMEOUT:              //(15)
            printk(KERN_DEBUG "The operation timed out.\n");
            break;

        case FR_LOCKED:               //(16)
            printk(KERN_DEBUG "The file is protected.\n");
            break;

        case FR_NOT_ENOUGH_CORE:      //(17)
            printk(KERN_DEBUG "Long filename support failed to get heap space.\n");
            break;

        case FR_TOO_MANY_OPEN_FILES:  //(18)
            printk(KERN_DEBUG "Too many files open.\n");
            break;

        case FR_INVALID_PARAMETER:    // (19)
            printk(KERN_DEBUG "Invalid parameter.\n");
            break;

        default:
            break;

    }
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

    printk(KERN_DEBUG "Device information acquisition.");
    /* Gets device information and empty cluster size. */
    res = f_getfree("1:", &fre_clust, &pfs);
    /* The total number of sectors and the number of empty sectors are calculated. */
    tot_sect = (pfs->n_fatent - 2) * pfs->csize;
    fre_sect = fre_clust * pfs->csize;
    /* Print information (4096 bytes/sector) */
    printk(KERN_DEBUG "Total equipment space:%6u MB.\r\nAvailable space: %6u MB.",
        tot_sect * 4 / 1024, fre_sect * 4 / 1024);

    printk(KERN_DEBUG "File location and formatting write function test");
    res = f_open(&my_file, "FatFs.txt", FA_OPEN_EXISTING | FA_WRITE | FA_READ);

    if (res == FR_OK) {
        /* File location */
        res = f_lseek(&my_file, f_size(&my_file) - 1);

        if (res == FR_OK) {
            /* Format write, parameter format similar to printf function. */
            byteswritten = f_printf(&my_file,
                    "add a new line to the original file.");

            if (byteswritten == EOF)
                printk(KERN_ERR " ");

            byteswritten = f_printf(&my_file,
                    "Total equipment space:%6lu MB.\r\nAvailable space: %6lu MB.",
                    tot_sect * 4 / 1024, fre_sect * 4 / 1024);

            if (byteswritten == EOF)
                printk(KERN_ERR " ");

            /* The file is positioned at the start of the file. */
            res = f_lseek(&my_file, 0);
            /* Read all contents of the file into the cache. */
            res = f_read(&my_file, rtext, f_size(&my_file), &bytesread);

            if (res == FR_OK)
                printk(KERN_DEBUG "The file content: %s", rtext);
        }

        f_close(&my_file);

        printk(KERN_DEBUG "Directory creation and rename function test");
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
        printk(KERN_DEBUG "Failed to open file: %d", res);
        printk(KERN_DEBUG
            "You may need to run the FatFs migration and read and write test project again.");
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
                printk(KERN_DEBUG "%s/%s", path, fn);                //Output file name
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
            if (i + j >= sz_buff) { /* Buffer over flow? */
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


    printk(KERN_DEBUG "test_diskio(%u, %u, 0x%08X, 0x%08X)", pdrv, ncyc, (UINT)buff, sz_buff);

    if (sz_buff < FF_MAX_SS + 8) {
        printk(KERN_DEBUG "Insufficient work area to run the program.");
        return 1;
    }

    for (cc = 1; cc <= ncyc; cc++) {
        printk(KERN_DEBUG "**** Test cycle %u of %u start ****", cc, ncyc);

        printk(KERN_DEBUG " disk_initalize(%u)", pdrv);
        ds = disk_initialize(pdrv);

        if (ds & STA_NOINIT) {
            printk(KERN_DEBUG " - failed.");
            return 2;
        } else
            printk(KERN_DEBUG " - ok.");

        printk(KERN_DEBUG "**** Get drive size ****");
        printk(KERN_DEBUG " disk_ioctl(%u, GET_SECTOR_COUNT, 0x%08X)", pdrv, (UINT)&sz_drv);
        sz_drv = 0;
        dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 3;
        }

        if (sz_drv < 128) {
            printk(KERN_DEBUG "Failed: Insufficient drive size to test.");
            return 4;
        }

        printk(KERN_DEBUG " Number of sectors on the drive %u is %u.", pdrv, sz_drv);

#if FF_MAX_SS != FF_MIN_SS
        printk(KERN_DEBUG "**** Get sector size ****");
        printk(KERN_DEBUG " disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (UINT)&sz_sect);
        sz_sect = 0;
        dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 5;
        }

        printk(KERN_DEBUG " Size of sector is %u bytes.", sz_sect);
#else
        sz_sect = FF_MAX_SS;
#endif

        printk(KERN_DEBUG "**** Get block size ****");
        printk(KERN_DEBUG " disk_ioctl(%u, GET_BLOCK_SIZE, 0x%X)", pdrv, (UINT)&sz_eblk);
        sz_eblk = 0;
        dr = disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else
            printk(KERN_DEBUG " - failed.");

        if (dr == RES_OK || sz_eblk >= 2)
            printk(KERN_DEBUG " Size of the erase block is %u sectors.", sz_eblk);
        else
            printk(KERN_DEBUG " Size of the erase block is unknown.");

        /* Single sector write test */
        printk(KERN_DEBUG "**** Single sector write test ****");
        lba = 0;

        for (n = 0, fatfs_diskio_pseudo(pns); n < sz_sect; n++)
            pbuff[n] = (BYTE)fatfs_diskio_pseudo(0);

        printk(KERN_DEBUG " disk_write(%u, 0x%X, %u, 1)", pdrv, (UINT)pbuff, lba);
        dr = disk_write(pdrv, pbuff, lba, 1);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 6;
        }

        printk(KERN_DEBUG " disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 7;
        }

        memset(pbuff, 0, sz_sect);
        printk(KERN_DEBUG " disk_read(%u, 0x%X, %u, 1)", pdrv, (UINT)pbuff, lba);
        dr = disk_read(pdrv, pbuff, lba, 1);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 8;
        }

        for (n = 0, fatfs_diskio_pseudo(pns);
            n < sz_sect && pbuff[n] == (BYTE)fatfs_diskio_pseudo(0);
            n++) ;

        if (n == sz_sect)
            printk(KERN_DEBUG " Read data matched.");
        else {
            printk(KERN_DEBUG " Read data differs from the data written.");
            return 10;
        }

        pns++;

        printk(KERN_DEBUG "**** Multiple sector write test ****");
        lba = 5;
        ns = sz_buff / sz_sect;

        if (ns > 4)
            ns = 4;

        if (ns > 1) {
            for (n = 0, fatfs_diskio_pseudo(pns); n < (UINT)(sz_sect * ns); n++)
                pbuff[n] = (BYTE)fatfs_diskio_pseudo(0);

            printk(KERN_DEBUG " disk_write(%u, 0x%X, %u, %u)", pdrv, (UINT)pbuff, lba, ns);
            dr = disk_write(pdrv, pbuff, lba, ns);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 11;
            }

            printk(KERN_DEBUG " disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 12;
            }

            memset(pbuff, 0, sz_sect * ns);
            printk(KERN_DEBUG " disk_read(%u, 0x%X, %u, %u)", pdrv, (UINT)pbuff, lba, ns);
            dr = disk_read(pdrv, pbuff, lba, ns);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 13;
            }

            for (n = 0, fatfs_diskio_pseudo(pns);
                n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)fatfs_diskio_pseudo(0);
                n++) ;

            if (n == (UINT)(sz_sect * ns))
                printk(KERN_DEBUG " Read data matched.");
            else {
                printk(KERN_DEBUG " Read data differs from the data written.");
                return 14;
            }
        } else
            printk(KERN_DEBUG " Test skipped.");

        pns++;

        printk(KERN_DEBUG "**** Single sector write test (unaligned buffer address) ****");
        lba = 5;

        for (n = 0, fatfs_diskio_pseudo(pns); n < sz_sect; n++)
            pbuff[n + 3] = (BYTE)fatfs_diskio_pseudo(0);

        printk(KERN_DEBUG " disk_write(%u, 0x%X, %u, 1)", pdrv, (UINT)(pbuff + 3), lba);
        dr = disk_write(pdrv, pbuff + 3, lba, 1);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 15;
        }

        printk(KERN_DEBUG " disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 16;
        }

        memset(pbuff + 5, 0, sz_sect);
        printk(KERN_DEBUG " disk_read(%u, 0x%X, %u, 1)", pdrv, (UINT)(pbuff + 5), lba);
        dr = disk_read(pdrv, pbuff + 5, lba, 1);

        if (dr == RES_OK)
            printk(KERN_DEBUG " - ok.");
        else {
            printk(KERN_DEBUG " - failed.");
            return 17;
        }

        for (n = 0, fatfs_diskio_pseudo(pns);
            n < sz_sect && pbuff[n + 5] == (BYTE)fatfs_diskio_pseudo(0);
            n++) ;

        if (n == sz_sect)
            printk(KERN_DEBUG " Read data matched.");
        else {
            printk(KERN_DEBUG " Read data differs from the data written.");
            return 18;
        }

        pns++;

        printk(KERN_DEBUG "**** 4GB barrier test ****");

        if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2)) {
            lba = 6;
            lba2 = lba + 0x80000000 / (sz_sect / 2);

            for (n = 0, fatfs_diskio_pseudo(pns); n < (UINT)(sz_sect * 2); n++)
                pbuff[n] = (BYTE)fatfs_diskio_pseudo(0);

            printk(KERN_DEBUG " disk_write(%u, 0x%X, %u, 1)", pdrv, (UINT)pbuff, lba);
            dr = disk_write(pdrv, pbuff, lba, 1);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 19;
            }

            printk(KERN_DEBUG " disk_write(%u, 0x%X, %u, 1)", pdrv, (UINT)(pbuff + sz_sect), lba2);
            dr = disk_write(pdrv, pbuff + sz_sect, lba2, 1);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 20;
            }

            printk(KERN_DEBUG " disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 21;
            }

            memset(pbuff, 0, sz_sect * 2);
            printk(KERN_DEBUG " disk_read(%u, 0x%X, %u, 1)", pdrv, (UINT)pbuff, lba);
            dr = disk_read(pdrv, pbuff, lba, 1);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 22;
            }

            printk(KERN_DEBUG " disk_read(%u, 0x%X, %u, 1)", pdrv, (UINT)(pbuff + sz_sect), lba2);
            dr = disk_read(pdrv, pbuff + sz_sect, lba2, 1);

            if (dr == RES_OK)
                printk(KERN_DEBUG " - ok.");
            else {
                printk(KERN_DEBUG " - failed.");
                return 23;
            }

            for (n = 0, fatfs_diskio_pseudo(pns);
                pbuff[n] == (BYTE)fatfs_diskio_pseudo(0) && n < (UINT)(sz_sect * 2);
                n++) ;

            if (n == (UINT)(sz_sect * 2))
                printk(KERN_DEBUG " Read data matched.");
            else {
                printk(KERN_DEBUG " Read data differs from the data written.");
                return 24;
            }
        } else
            printk(KERN_DEBUG " Test skipped.");

        pns++;

        printk(KERN_DEBUG "**** Test cycle %u of %u completed ****", cc, ncyc);
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
    fr = f_lseek(fp, 0);            /* Validates and prepares the file */

    if (fr != FR_OK)
        return fr;

#if FF_MAX_SS == FF_MIN_SS
    clsz = (DWORD)fp->obj.fs->csize * FF_MAX_SS;    /* Cluster size */
#else
    clsz = (DWORD)fp->obj.fs->csize * fp->obj.fs->ssize;
#endif
    fsz = fp->obj.objsize;

    if (fsz > 0) {
        clst = fp->obj.sclust - 1;  /* A cluster leading the first cluster for first test */

        while (fsz) {
            step = (fsz >= clsz) ? clsz : (DWORD)fsz;
            fr = f_lseek(fp, f_tell(fp) + step);    /* Advances file pointer a cluster */

            if (fr != FR_OK)
                return fr;

            if (clst + 1 != fp->clust) {
                break;  /* Is not the cluster next to previous one? */
            }

            clst = fp->clust;
            fsz -= step;          /* Get current cluster for next test */
        }

        if (fsz == 0) {
            *cont = 1;  /* All done without fail? */
        }
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
        printk(KERN_DEBUG "disk_ioctl() failed.");
        return false;
    }

#else
    ss = FF_MAX_SS;
#endif

    printk(KERN_DEBUG "Starting raw write test at sector %u in %u bytes of data chunks...", lba, sz_buff);
    tmr = basic_timer_get_timer_cnt();

    for (ofs = 0; ofs < len / ss; ofs += sz_buff / ss) {
        if (disk_write(pdrv, buff, lba + ofs, sz_buff / ss) != RES_OK) {
            printk(KERN_DEBUG "disk_write() failed.");
            return false;
        }
    }

    if (disk_ioctl(pdrv, CTRL_SYNC, 0) != RES_OK) {
        printk(KERN_DEBUG "disk_ioctl() failed.");
        return false;
    }

    tmr = basic_timer_get_timer_cnt() - tmr;
    printk(KERN_DEBUG "%u bytes written and it took %u timer ticks.", len, tmr);

    printk(KERN_DEBUG "Starting raw read test at sector %u in %u bytes of data chunks...", lba, sz_buff);
    tmr = basic_timer_get_timer_cnt();

    for (ofs = 0; ofs < len / ss; ofs += sz_buff / ss) {
        if (disk_read(pdrv, buff, lba + ofs, sz_buff / ss) != RES_OK) {
            printk(KERN_DEBUG "disk_read() failed.");
            return false;
        }
    }

    tmr = basic_timer_get_timer_cnt() - tmr;
    printk(KERN_DEBUG "%u bytes read and it took %u timer ticks.", len, tmr);
    printk(KERN_DEBUG "Test completed.");

    return true;
}

#ifdef DESIGN_VERIFICATION_FATFS
#include "kinetis/test-kinetis.h"

int t_fatfs_loopback(int argc, char **argv)
{
    FRESULT res;                                         /* FatFs function common result code */
    MKFS_PARM opt;
    uint32_t byteswritten, bytesread;                    /* File write/read counts */
    uint8_t wtext[] = "This is STM32 working with FatFs";/* File write buffer */
    uint8_t rtext[100];                                  /* File read buffer */

    /*##-1- Link the  disk I/O driver #######################################*/
    if (FATFS_LinkDriver(&flash_disk_driver, disk_path) == 0) {
        /*##-2- Register the file system object to the FatFs module ##############*/
        res = f_mount(&disk_fatfs, (TCHAR const *)disk_path, 0);

        if (res != FR_OK) {
            /* FatFs Initialization Error */
            printf_fatfs_err(res);
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
                printf_fatfs_err(res);
            } else {
                /*##-4- Create and Open a new text file object with write access #####*/
                res = f_open(&my_file, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE);

                if (res != FR_OK) {
                    /* 'STM32.TXT' file Open for write Error */
                    printf_fatfs_err(res);
                } else {
                    /*##-5- Write data to the text file ################################*/
                    res = f_write(&my_file, wtext, sizeof(wtext), (void *)&byteswritten);

                    if ((byteswritten == 0) || (res != FR_OK)) {
                        /* 'STM32.TXT' file Write or EOF Error */
                        printf_fatfs_err(res);
                    } else {
                        /*##-6- Close the open text file #################################*/
                        f_close(&my_file);

                        /*##-7- Open the text file object with read access ###############*/
                        res = f_open(&my_file, "STM32.TXT", FA_READ);

                        if (res != FR_OK) {
                            /* 'STM32.TXT' file Open for read Error */
                            printf_fatfs_err(res);
                        } else {
                            /*##-8- Read data from the text file ###########################*/
                            res = f_read(&my_file, rtext, sizeof(rtext), (void *)&bytesread);

                            if ((bytesread == 0) || (res != FR_OK)) {
                                /* 'STM32.TXT' file Read or EOF Error */
                                printf_fatfs_err(res);
                            } else {
                                /*##-9- Close the open text file #############################*/
                                f_close(&my_file);

                                /*##-10- Compare read data with the expected data ############*/
                                if ((bytesread != byteswritten)) {
                                    /* Read data is different from the expected data */
                                    printf_fatfs_err(res);
                                } else {
                                    /* Success of the demo: no error occurrence */
                                    printk(KERN_DEBUG "FatFs TEST PASS");
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

    return PASS;
}

int t_fatfs_miscellaneous(int argc, char **argv)
{
    FRESULT res;

    /* FatFs multifunctional test. */
    res = fatfs_miscellaneous();

    if (res == FR_OK)
        return PASS;
    else
        return FAIL;
}

int t_fatfs_file_check(int argc, char **argv)
{
    static FILINFO finfo;
    FRESULT res;

    /* Get file information */
    res = f_stat("TestDir/testdir.txt", &finfo);

    if (res == FR_OK) {
        printk(KERN_DEBUG "testdir.txt File information");
        printk(KERN_DEBUG "The file size: %ud(B)", finfo.fsize);
        printk(KERN_DEBUG "The time stamp: %u/%02u/%02u, %02u:%02u",
            (finfo.fdate >> 9) + 1980, finfo.fdate >> 5 & 15, finfo.fdate & 31, finfo.ftime >> 11, finfo.ftime >> 5 & 63);
        printk(KERN_DEBUG "attribute: %c%c%c%c%c",
            (finfo.fattrib & AM_DIR) ? 'D' : '-',      // Directory
            (finfo.fattrib & AM_RDO) ? 'R' : '-',      // A read-only file
            (finfo.fattrib & AM_HID) ? 'H' : '-',      // Hidden files
            (finfo.fattrib & AM_SYS) ? 'S' : '-',      // System files
            (finfo.fattrib & AM_ARC) ? 'A' : '-');     // Archive
        return PASS;
    }

    return FAIL;
}

int t_fatfs_scan_files(int argc, char **argv)
{
    FRESULT res;

    printk(KERN_DEBUG "Document scanning test.");
    strcpy(disk_path, "1:");
    fatfs_scan_files(disk_path);

    if (res == FR_OK)
        return PASS;
    else
        return FAIL;
}

int t_fatfs_append(int argc, char **argv)
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    uint8_t year, month, date, hours, minutes, seconds;

    /* Open or create a log file and ready to append */
    f_mount(&fs, "", 0);
    fr = fatfs_open_append(&fil, "logfile.txt");

    if (fr != FR_OK)
        return FAIL;

    rtc_calendar_show(&year, &month, &date, &hours, &minutes, &seconds, NULL,
        KRTC_FORMAT_BIN);

    /* Append a line */
    f_printf(&fil, "%02u/%02u/%u, %2u:%02u\n", date, month, year, hours, minutes);

    /* Close the file */
    f_close(&fil);

    return PASS;
}

int t_fatfs_delete_node(int argc, char **argv)  /* How to use */
{
    FRESULT fr;
    FATFS fs;
    TCHAR buff[256];
    FILINFO fno;

    f_mount(&fs, _T("5:"), 0);

    /* Directory to be deleted */
    strncpy(buff, _T("5:dir"), strlen(_T("5:dir")));

    /* Delete the directory */
    fr = fatfs_delete_node(buff, sizeof buff / sizeof buff[0], &fno);

    /* Check the result */
    if (fr) {
        printk(KERN_DEBUG _T("Failed to delete the directory. (%u)"), fr);
        return FAIL;
    } else {
        printk(KERN_DEBUG _T("The directory and the contents have successfully been deleted."));
        return PASS;
    }
}

/*----------------------------------------------------------------------/
/ Allocate a contiguous area to the file
/-----------------------------------------------------------------------/
/ This function checks if the file is contiguous with desired size.
/ If not, a block of contiguous sectors is allocated to the file.
/ If the file has been opened without FA_WRITE flag, it only checks if
/ the file is contiguous and returns the resulut.
/-----------------------------------------------------------------------/
/ This function can work with FatFs R0.09 - R0.11a.
/ It is incompatible with R0.12+. Use f_expand function instead.
/----------------------------------------------------------------------*/

int t_fatfs_expend(int argc, char **argv)
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
        return FAIL;

    /* Check if the file is 256MB in size and occupies a contiguous area.
    /  If not, a contiguous area will be re-allocated to the file. */
    org = f_expand(&fil, 0x10000000, 1);

    if (!org) {
        printk(KERN_DEBUG "Function failed due to any error or insufficient contiguous area.");
        f_close(&fil);
        return FAIL;
    }

    /* Now you can read/write the file without filesystem layer. */
    dr = disk_write(fs.pdrv, work_buffer, org, 1024);   /* Write 512KiB from top of the file */

    f_close(&fil);

    if (dr != RES_OK)
        return FAIL;

    return PASS;
}

int t_fatfs_diskio(int argc, char **argv)
{
    int rc;
    DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */

    /* Check function/compatibility of the physical drive #0 */
    rc = fatfs_diskio(0, 3, buff, sizeof buff);

    if (rc) {
        printk(KERN_DEBUG "Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not work with this disk driver.", rc);
        return FAIL;
    } else {
        printk(KERN_DEBUG "Congratulations! The disk driver works well.");
        return PASS;
    }
}

int t_fatfs_contiguous_file(int argc, char **argv)
{
    int rc;
    DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */

    /* Check function/compatibility of the physical drive #0 */
    rc = fatfs_diskio(0, 3, buff, sizeof buff);

    if (rc) {
        printk(KERN_DEBUG "Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not work with this disk driver.", rc);
        return FAIL;
    } else {
        printk(KERN_DEBUG "Congratulations! The disk driver works well.");
        return PASS;
    }
}

int t_fatfs_raw_speed(int argc, char **argv)
{
    int rc;
    DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */

    /* Check function/compatibility of the physical drive #0 */
    rc = fatfs_diskio(0, 3, buff, sizeof buff);

    if (rc) {
        printk(KERN_DEBUG "Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not work with this disk driver.", rc);
        return FAIL;
    } else {
        printk(KERN_DEBUG "Congratulations! The disk driver works well.");
        return PASS;
    }
}

#endif

