#include "bsp_fatfs/bsp_fatfs.h"
#include "ff.h"
#include "string.h"


FATFS DISKFatFs;  /* File system object for  disk logical drive */
FIL MyFile;       /* File object */
char DISKPath[4]; /*  disk logical drive path */
static uint8_t buffer[_MAX_SS]; /* a work buffer for the f_mkfs() */

static void Printf_FatFs_Err(FRESULT fresult);
static FRESULT Miscellaneous(void);
static FRESULT FatFs_File_Check(void);
static FRESULT FatFs_Scan_Files (char* path);


/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "ff_gen_drv.h"

#define DEBUG
#include "idebug/idebug.h"

#define FatFs_printf    p_dbg
#define FatFs_err       p_err_fun

#define STM32_MICROCONTROLER    0

int FatFs_Test(void)
{
  FRESULT res;                                         /* FatFs function common result code */
  uint32_t byteswritten, bytesread;                    /* File write/read counts */                              
  uint8_t wtext[] = "This is STM32 working with FatFs";/* File write buffer */
  uint8_t rtext[100];                                  /* File read buffer */ 

#if STM32_MICROCONTROLER  
  /*##-1- Link the  disk I/O driver #######################################*/
  if(FATFS_LinkDriver(&SD_Driver, DISKPath) == 0)
#endif
  {
    /*##-2- Register the file system object to the FatFs module ##############*/
    res = f_mount(&DISKFatFs, (TCHAR const*)DISKPath, 0);
    if(res != FR_OK)
    {
      /* FatFs Initialization Error */
      Printf_FatFs_Err(res);
    }
    else
    {
      /*##-3- Create a FAT file system (format) on the logical drive #########*/
      if(res == FR_NO_FILESYSTEM)
        res = f_mkfs((TCHAR const*)DISKPath, FM_ANY, 0, buffer, sizeof(buffer));
      if(res != FR_OK) 
      {
        /* FatFs Format Error */
        Printf_FatFs_Err(res);
      }
      else
      {
        /*##-4- Create and Open a new text file object with write access #####*/
        res = f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE);
        if(res != FR_OK) 
        {
          /* 'STM32.TXT' file Open for write Error */
          Printf_FatFs_Err(res);
        }
        else
        {
          /*##-5- Write data to the text file ################################*/
          res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);
          
          if((byteswritten == 0) || (res != FR_OK))
          {
            /* 'STM32.TXT' file Write or EOF Error */
            Printf_FatFs_Err(res);
          }
          else
          {
              /*##-6- Close the open text file #################################*/
            f_close(&MyFile);
            
              /*##-7- Open the text file object with read access ###############*/
            res = f_open(&MyFile, "STM32.TXT", FA_READ);
            if(res != FR_OK)
            {
              /* 'STM32.TXT' file Open for read Error */
              Printf_FatFs_Err(res);
            }
            else
            {
                /*##-8- Read data from the text file ###########################*/
              res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);
              
              if((bytesread == 0) || (res != FR_OK))
              {
                /* 'STM32.TXT' file Read or EOF Error */
                Printf_FatFs_Err(res);
              }
              else
              {
                  /*##-9- Close the open text file #############################*/
                f_close(&MyFile);
                
                /*##-10- Compare read data with the expected data ############*/
                if ((bytesread != byteswritten))
                {                
                  /* Read data is different from the expected data */
                  Printf_FatFs_Err(res); 
                }
                else
                {
                  /* Success of the demo: no error occurrence */
                  FatFs_printf("FatFs TEST PASS");
                }
              }
            }
          }
        }
      }
    }
  }
#if STM32_MICROCONTROLER  
  /*##-11- Unlink the  disk I/O driver ####################################*/
  FATFS_UnLinkDriver(DISKPath);
#endif
  
  /* FatFs multifunctional test. */
  res = Miscellaneous();
  
  FatFs_printf("File information acquisition test.");
  res = FatFs_File_Check();

  FatFs_printf("Document scanning test.");
  strcpy(DISKPath, "1:");
  FatFs_Scan_Files(DISKPath); 
  
  return 0;
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

static void Printf_FatFs_Err(FRESULT fresult)
{
  switch(fresult)
  {
    case FR_OK:                   //(0)
      FatFs_printf("Operation successful.");
    break;
    case FR_DISK_ERR:             //(1)
      FatFs_printf("Hardware input/output driver error.");
    break;
    case FR_INT_ERR:              //(2)
      FatFs_printf("Assertion error.");
    break;
    case FR_NOT_READY:            //(3)
      FatFs_printf("The physical device doesn't work.");
    break;
    case FR_NO_FILE:              //(4)
      FatFs_printf("Unable to locate file.");
    break;
    case FR_NO_PATH:              //(5)
      FatFs_printf("Unable to find path.");
    break;
    case FR_INVALID_NAME:         //(6)
      FatFs_printf("Invalid path name.");
    break;
    case FR_DENIED:               //(7)
    case FR_EXIST:                //(8)
      FatFs_printf("Access denied.");
    break;
    case FR_INVALID_OBJECT:       //(9)
      FatFs_printf("Invalid file or path.");
    break;
    case FR_WRITE_PROTECTED:      //(10)
      FatFs_printf("Logical device write protection.");
    break;
    case FR_INVALID_DRIVE:        //(11)
      FatFs_printf("Invalid logic device.");
    break;
    case FR_NOT_ENABLED:          //(12)
      FatFs_printf("Invalid workspace.");
    break;
    case FR_NO_FILESYSTEM:        //(13)
      FatFs_printf("Invalid file system.");
    break;
    case FR_MKFS_ABORTED:         //(14)
      FatFs_printf("The f_mkfs function failed because of a function parameter problem.");
    break;
    case FR_TIMEOUT:              //(15)
      FatFs_printf("The operation timed out.");
    break;
    case FR_LOCKED:               //(16)
      FatFs_printf("The file is protected.");
    break;
    case FR_NOT_ENOUGH_CORE:      //(17)
      FatFs_printf("Long filename support failed to get heap space.");
    break;
    case FR_TOO_MANY_OPEN_FILES:  //(18)
      FatFs_printf("Too many files open.");
    break;
    case FR_INVALID_PARAMETER:    // (19)
      FatFs_printf("Invalid parameter.");
    break;
    default:  
    break;

  }
}

static FRESULT Miscellaneous(void)
{
  FATFS *pfs;
  DWORD fre_clust, fre_sect, tot_sect;
  FRESULT res;
  DIR MyDir;
  uint8_t rtext[100];                                  /* File read buffer */ 
  int32_t byteswritten = 0;
  uint32_t bytesread = 0;

  FatFs_printf("Device information acquisition.");
  /* Gets device information and empty cluster size. */
  res = f_getfree("1:", &fre_clust, &pfs);
  /* The total number of sectors and the number of empty sectors are calculated. */
  tot_sect = (pfs->n_fatent - 2) * pfs->csize;
  fre_sect = fre_clust * pfs->csize;
  /* Print information (4096 bytes/sector) */
  FatFs_printf("Total equipment space:%6lu MB.\r\nAvailable space: %6lu MB.", tot_sect * 4 / 1024, fre_sect * 4 / 1024);
  
  FatFs_printf("File location and formatting write function test");
  res = f_open(&MyFile, "FatFs.txt", FA_OPEN_EXISTING | FA_WRITE | FA_READ);
  if (res == FR_OK)
  {
    /* File location */
    res = f_lseek(&MyFile, f_size(&MyFile) - 1);
    if (res == FR_OK)
    {
      /* Format write, parameter format similar to printf function. */
      byteswritten = f_printf(&MyFile, "add a new line to the original file.");
      if(byteswritten == EOF)
      {
        FatFs_err;
      }
      byteswritten = f_printf(&MyFile, "Total equipment space:%6lu MB.\r\nAvailable space: %6lu MB.", tot_sect * 4 / 1024, fre_sect * 4 / 1024);
      if(byteswritten == EOF)
      {
        FatFs_err;
      }

      /* The file is positioned at the start of the file. */
      res = f_lseek(&MyFile, 0);
      /* Read all contents of the file into the cache. */
      res = f_read(&MyFile, rtext, f_size(&MyFile), &bytesread);
      if(res == FR_OK)
      {
        FatFs_printf("The file content£º%s", rtext);
      }
    }
    f_close(&MyFile);    
    
    FatFs_printf("Directory creation and rename function test");
    /* Try opening a directory. */
    res = f_opendir(&MyDir, "TestDir");
    if(res != FR_OK)
    {
      /* Failure to open directory, create directory. */
      res = f_mkdir("TestDir");
    }
    else
    {
      /* If the directory already exists, close it. */
      res = f_closedir(&MyDir);
      /* Delete the file */
      f_unlink("TestDir/testdir.txt");
    }
    if(res == FR_OK)
    {
      /* Rename and move files. */
      res = f_rename("FatFs.txt", "TestDir/testdir.txt");      
    } 
  }
  else
  {
    FatFs_printf("Failed to open file£º%d", res);
    FatFs_printf("You may need to run the FatFs migration and read and write test project again.");
  }
  return res;
}


static FRESULT FatFs_File_Check(void)
{  
  static FILINFO finfo;
  FRESULT res;  

  /* Get file information */
  res = f_stat("TestDir/testdir.txt",&finfo);
  if(res == FR_OK)
  {
    FatFs_printf("¡°testdir.txt¡±File information£º");
    FatFs_printf("The file size: %ld(B)", finfo.fsize);
    FatFs_printf("The time stamp: %u/%02u/%02u, %02u:%02u",
           (finfo.fdate >> 9) + 1980, finfo.fdate >> 5 & 15, finfo.fdate & 31,finfo.ftime >> 11, finfo.ftime >> 5 & 63);
    FatFs_printf("attribute: %c%c%c%c%c",
           (finfo.fattrib & AM_DIR) ? 'D' : '-',      // Directory
           (finfo.fattrib & AM_RDO) ? 'R' : '-',      // A read-only file
           (finfo.fattrib & AM_HID) ? 'H' : '-',      // Hidden files
           (finfo.fattrib & AM_SYS) ? 'S' : '-',      // System files
           (finfo.fattrib & AM_ARC) ? 'A' : '-');     // Archive
  }
  return res;
}

/**
  * @brief  scan_files Recursively scan the files in FatFs.
  * @param  path:Initial scan path.
  * @retval result:The return value of the file system.
  */
static FRESULT FatFs_Scan_Files(char* path) 
{ 
  FRESULT res;     //The part of the variable that is modified in the recursive process is not global.
  FILINFO fno;
  DIR MyDir;
  int i;            
  char *fn;        //The file name. 
  
  //Open directory
  res = f_opendir(&MyDir, path); 
  if (res == FR_OK) 
  { 
    i = strlen(path); 
    for (;;) 
    { 
      //Reading the contents of the directory automatically reads the next file.
      res = f_readdir(&MyDir, &fno);                 
      //Is empty means all items read, jump out.
      if (res != FR_OK || fno.fname[0] == 0)
        break;
      fn = fno.fname;
      //Dot to indicate current directory, skip.
      if (*fn == '.') 
        continue;   
      //Directory, recursive read
      if (fno.fattrib & AM_DIR)         
      {       
        //Synthesize the full directory name.
        sprintf(&path[i], "/%s", fn);     
        //The recursive traversal.
        res = FatFs_Scan_Files(path);  
        path[i] = 0;         
        //Turn failure on and out of the loop.
        if (res != FR_OK) 
          break; 
      } 
      else 
      { 
        FatFs_printf("%s/%s", path, fn);                //Output file name
        /* Here you can extract the file path for a particular format. */        
      }
    }
  } 
  return res; 
}





