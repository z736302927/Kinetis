#include "protocol/hydrology.h"
#include "protocol/hydrology-config.h"
#include "protocol/hydrology-cmd.h"
#include "protocol/hydrology-identifier.h"
#include "task/hydrology-task.h"
#include "string.h"
#include "stdio.h"
#include "core/k-memory.h"
#include <linux/crc16.h>

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "task/k-timtask.h"
#include "timer/k-rtc.h"
#include "peripheral/k-serialport.h"
#include "timer/k-delay.h"
#include "timer/k-basictimer.h"
#include "ff.h"
#include "lib/k-fatfs.h"
#include "core/idebug.h"

int Hydrology_ReadFileSize(char *filename, uint32_t *Size)
{
    FIL HydrologyFile;
    FRESULT res;
    char buffer[64];

    snprintf(buffer, sizeof(buffer), "%s%s", HYDROLOGY_FILE_PATH, filename);

    res = f_open(&HydrologyFile, buffer, FA_OPEN_EXISTING | FA_READ);

    if (res == FR_OK)
        *Size = HydrologyFile.obj.objsize;
    else {
        Printf_FatFs_Err(res);
        kinetis_debug_trace(KERN_DEBUG, "Read the size of %s failed.", filename);
        return false;
    }

    f_close(&HydrologyFile);
    return true;
}

int Hydrology_ReadStoreInfo(char *filename, long addr, uint8_t *data, int len)
{
    FIL HydrologyFile;
    FRESULT res;
    uint32_t bytesread;
    char buffer[64];

    snprintf(buffer, sizeof(buffer), "%s%s", HYDROLOGY_FILE_PATH, filename);

    res = f_open(&HydrologyFile, buffer, FA_OPEN_EXISTING | FA_READ);

    if (res == FR_OK) {
        f_lseek(&HydrologyFile, addr);
        res = f_read(&HydrologyFile, data, len, (void *)&bytesread);

        if (res != FR_OK)
            return false;
    } else {
        Printf_FatFs_Err(res);
        kinetis_debug_trace(KERN_DEBUG, "Read %s failed.", filename);
        return false;
    }

    f_close(&HydrologyFile);
    return true;
}

int Hydrology_WriteStoreInfo(char *filename, long addr, uint8_t *data, int len)
{
    FIL HydrologyFile;
    DIR HydrologyDir;
    FRESULT res;
    uint32_t byteswritten;
    char buffer[64];

    snprintf(buffer, sizeof(buffer), "%s%s", HYDROLOGY_FILE_PATH, filename);

    res = f_open(&HydrologyFile, buffer, FA_OPEN_EXISTING | FA_WRITE);

    if (res == FR_OK) {
        f_lseek(&HydrologyFile, addr);
        res = f_write(&HydrologyFile, data, len, (void *)&byteswritten);

        if (res != FR_OK)
            return false;

        res = f_close(&HydrologyFile);

        if (res != FR_OK)
            return false;
    } else if (res == FR_NO_FILE || res == FR_NO_PATH) {
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, strlen(HYDROLOGY_FILE_PATH), "%s", HYDROLOGY_FILE_PATH);
        /* Try opening a directory. */
        res = f_opendir(&HydrologyDir, buffer);

        if (res != FR_OK) {
            /* Failure to open directory, create directory. */
            res = f_mkdir(buffer);
        }

        if (res != FR_OK)
            return false;

        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s%s", HYDROLOGY_FILE_PATH, filename);
        res = f_open(&HydrologyFile, buffer, FA_CREATE_NEW | FA_WRITE);

        if (res != FR_OK)
            return false;

        res = f_lseek(&HydrologyFile, addr);

        if (res != FR_OK)
            return false;

        res = f_write(&HydrologyFile, data, len, (void *)&byteswritten);

        if (res != FR_OK)
            return false;

        res = f_close(&HydrologyFile);

        if (res != FR_OK)
            return false;

        res = f_closedir(&HydrologyDir);

        if (res != FR_OK)
            return false;
    } else {
        Printf_FatFs_Err(res);
        kinetis_debug_trace(KERN_DEBUG, "Write %s failed.", filename);
        return false;
    }

    return true;
}

void Hydrology_ReadTime(uint8_t *time)
{
    RTC_CalendarShow((uint8_t *)&time[0], (uint8_t *)&time[1], (uint8_t *)&time[2],
        (uint8_t *)&time[3], (uint8_t *)&time[4], (uint8_t *)&time[5], NULL, KRTC_FORMAT_BCD);
}

void Hydrology_SetTime(uint8_t *t_time)
{
//  uint8_t temp[7];
//  uint8_t time[12];
//  uint8_t i = 0;
//
//  for(i = 0;i < 12;i++)
//    time[i] = t_time[i] - '0';
//
//  temp[0] = (time[0] << 4) + time[1];
//  temp[1] = (time[2] << 4) + time[3];
//  temp[2] = (time[4] << 4) + time[5];
//  temp[3] = (time[6] << 4) + time[7];
//  temp[4] = (time[8] << 4) + time[9];
//  temp[5] = (time[10] << 4) + time[11];

    RTC_CalendarConfig(t_time[0], t_time[1], t_time[2], t_time[3],
        t_time[4], t_time[5], NULL, KRTC_FORMAT_BCD);
}

extern struct TimTask_TypeDef HydrologyTask_LinkMaintenance;

void Hydrology_DisableLinkPacket(void)
{
    TimTask_Stop(&HydrologyTask_LinkMaintenance);
}

void Hydrology_EnableLinkPacket(void)
{
    TimTask_Start(&HydrologyTask_LinkMaintenance);
}

void Hydrology_DisconnectLink(void)
{

}

int Hydrology_OpenPort(void)
{
    switch (g_Hydrology.source) {
        case MsgFormServer:
            break;

        case MsgFormClient:
            break;
    }

    return true;
}

int Hydrology_ClosePort(void)
{
    switch (g_Hydrology.source) {
        case MsgFormServer:
            break;

        case MsgFormClient:
            break;
    }

    return true;
}

int Hydrology_PortTransmmitData(uint8_t *pData, uint16_t Len)
{
    SerialPort_TypeDef Hydrology_Port;

    kinetis_dump_buffer(pData, Len);
    kinetis_debug_trace(KERN_DEBUG, " ");

    Hydrology_OpenPort();

    switch (g_Hydrology.source) {
        case MsgFormServer:
//            NB_IOT_SendData(Data, Len);
            g_Hydrology.source = MsgFormServer;
            break;

        case MsgFormClient:
            Hydrology_Port.PortNbr = 2;
            Hydrology_Port.TxBuffer = pData;
            Hydrology_Port.TxBuffer_Size = Len;
            SerialPort_Send(&Hydrology_Port);
            break;
    }

    return true;
}

int Hydrology_PortReceiveData(uint8_t **ppData, uint16_t *pLen, uint32_t Timeout)
{
    SerialPort_TypeDef Hydrology_Port;
    uint32_t Refer = 0;
    uint32_t Delta = 0;
    int ret;

    switch (g_Hydrology.source) {
        case MsgFormServer:
            g_Hydrology.source = MsgFormServer;
            break;

        case MsgFormClient:
            memset(&Hydrology_Port, 0, sizeof(SerialPort_TypeDef));
            Hydrology_Port.PortNbr = 2;
            Hydrology_Port.TempBuffer_Size = 300;
            Hydrology_Port.RxScanInterval = 10;
            Hydrology_Port.Endchar = NULL;
            Hydrology_Port.Endchar_Size = 0;
            SerialPort_Open(&Hydrology_Port);

            Refer = BasicTimer_GetSSTick();

            for (;;) {
                if (SerialPort_Receive(&Hydrology_Port) == true) {
                    kinetis_dump_buffer(Hydrology_Port.RxBuffer, Hydrology_Port.RxBuffer_Size);
                    *ppData = (uint8_t *)Hydrology_Port.RxBuffer;
                    *pLen = Hydrology_Port.RxBuffer_Size;
                    SerialPort_Close(&Hydrology_Port);
                    ret = true;
                    break;
                } else {
                    Delta = BasicTimer_GetSSTick() >= Refer ?
                        BasicTimer_GetSSTick() - Refer :
                        BasicTimer_GetSSTick() + (DELAY_TIMER_UNIT - Refer);

                    if (Delta > Timeout) {
                        kinetis_debug_trace(KERN_DEBUG, "[warning]Receive data timeout.");
                        SerialPort_Close(&Hydrology_Port);
                        ret = false;
                        break;
                    }
                }
            }

            break;
    }

    return ret;
}

uint32_t Hydrology_GetFlashSize(void)
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
    kinetis_debug_trace(KERN_DEBUG, "Total equipment space: %u MB.", tot_sect * 4 / 1024);
    kinetis_debug_trace(KERN_DEBUG, "Available space: %u MB.", fre_sect * 4 / 1024);

    return fre_sect * 4 * 1024;
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

Hydrology g_Hydrology;

int Hydrology_ResourceInit(void)
{
    int ret;
    uint8_t Data;
    uint32_t flash_size, min_size;

    flash_size = Hydrology_GetFlashSize();

    min_size = HYDROLOGY_END;
    min_size += sizeof(HydrologyElementInfo) * ELEMENT_COUNT;
    min_size += HYDROLOGY_D_PIC_REVSPACE;
    min_size += HYDROLOGY_D_RGZS_REVSPACE;

    if (min_size >= flash_size) {
        kinetis_debug_trace(KERN_ERR, "ERR Current flash size is %.2f KB", (float)flash_size / 1024);
        kinetis_debug_trace(KERN_ERR, "ERR Flash size minimum requirement %.2f KB", (float)min_size / 1024);
        return false;
    }

    ret = Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_INIT_MARK,
            &Data, 1);

    if (ret == false) {
        kinetis_debug_trace(KERN_DEBUG, "It is first time to use device");
        kinetis_debug_trace(KERN_DEBUG, "Writing to flash");
        HydrologyD_Reset();
        HydrologyH_Reset();
        kinetis_debug_trace(KERN_DEBUG, "Writing to flash has completed");
    }

    return true;
}

void HydrologyD_Shundown(void)
{
    HydrologyTask_Deinit();
}

int HydrologyD_Setup(void)
{
    int ret;

    HydrologyTask_Init();
    ret = Hydrology_ResourceInit();

    return ret;
}

int HydrologyD_Reboot(void)
{
    int ret;

    HydrologyD_Shundown();
    ret = HydrologyD_Setup();

    return ret;
}

void Hydrology_ReadObservationTime(HydrologyElementInfo *Element, uint8_t *observationtime)
{
    uint32_t addr = 0;

    if (Element->D % 2 == 0)
        addr = Element->D / 2 + Element->Addr;
    else
        addr = (Element->D + 1) / 2 + Element->Addr;

    Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, addr, observationtime, 5);
}

void Hydrology_SetObservationTime(HydrologyElementInfo *Element)
{
    long addr = 0;
    uint8_t observationtime[6] = {0, 0, 0, 0, 0, 0};

    if (Element->D % 2 == 0)
        addr = Element->D / 2 + Element->Addr;
    else
        addr = (Element->D + 1) / 2 + Element->Addr;

    Hydrology_ReadTime(observationtime);
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, addr, observationtime, 5);
}

void Hydrology_GetBCDnums(double num, int *intergerpart, int *decimerpart,
    int d, char *pout_intergerValue, uint8_t *pout_decimerValue)
{
    char strfloat[20];
    int i = 0;
    int len = 0;
    int j = 0;
    int k = 0;

    for (i = 0; i < 20; i++)
        strfloat[i] = 'X';

    sprintf(strfloat, "%f", num);

    for (i = 0; i < 20; i++) {
        if ('X' != strfloat[i])
            len++;
    }

    for (i = 0; i < len; i++) {
        if ('0' != strfloat[i])
            break;
    }

    len = len - i - 1;

    for (i = 0; i < len; i++) {
        if ('.' == strfloat[i]) {
            j = i;
            break;
        }
    }

    if (i < len) {
        *decimerpart = len - j - 1;

        if (*decimerpart > d)
            *decimerpart = d;

        *intergerpart = j;

        for (i = 0; i < j; i++)
            pout_intergerValue[i] = strfloat[i];

        pout_intergerValue[j] = 0;

        for (i = j + 1; (i < len) && (k < (*decimerpart)); i++)
            pout_decimerValue[k++] = strfloat[i];

        pout_decimerValue[k] = 0;
    } else {
        *decimerpart = 0;
        *intergerpart = len;
        sprintf(pout_intergerValue, "%d", (int)num);
    }
}

int Hydrology_GetEvenNum(int num)
{
    if (num % 2 == 0)
        return num;
    else
        return num + 1;
}

/* The byte is 5 bits high, D represents the number of data bytes, and 3 bits low,
 * D represents the number of decimal place.
 */
void Hydrology_GetGuideID(uint8_t *value, uint8_t D, uint8_t d)
{
    uint8_t high5 = 0;
    uint8_t low3 = 0;
    uint8_t evenD = 0;

    evenD = Hydrology_GetEvenNum(D);

    high5 = evenD / 2;
    high5 = high5 << 3;
    /* D has to be between 0 and 7 to be represented in 3 digits. */
    low3 = d;
    *value = high5 | low3;
}

int Hydrology_ConvertToHexElement(double input, int D, int d, uint8_t *out)
{
    /* StrInterValue represents an integer value */
    char strInterValue[20] = {0};
    /* StrDeciValue means a small value */
    uint8_t strDeciValue[20] = {0};
    /* Interger represents an integer number */
    int integer = 0;
    /* Interger for integer number decimer for decimal number */
    int decimer = 0;
    /* Represents integral value */
    //int intergerValue = 0;
    /* Representing a small number */
    //int decimerValue = 0;
    /* Total represents the total number of input digits (minus the decimal point). */
    int total = 0;
    /* Even D */
    int evenD = 0;
    /* Represents the difference between evenD and total */
    int difftotal = 0;
    /* Indicates that integer bits need to be completed */
    int diffInterger = 0;
    /* Indicates that the decimal place needs to be filled */
    int diffDecimer = 0;
    /* Represents the number of digits in the decimal place to delete */
    //int delDecimer = 0;
    int i = 0;
    int j = 0;
    int m = 0;

    uint8_t tmp[30];

    for (m = 0; m < 30; m++)
        tmp[m] = '0';

    Hydrology_GetBCDnums(input, &integer, &decimer, d, strInterValue, strDeciValue);
    evenD = Hydrology_GetEvenNum(D);
    total = integer + decimer;

    /* Input configuration parameters are guaranteed */
    if (evenD >= total) {
        difftotal = evenD - total;

        /*This is definitely going to happen, Hydrology_GetBCDnums guarantees */
        if (d >= decimer) {
            /* The number of digits in the decimal place that need to be filled in */
            diffDecimer = d - decimer;
            /* Integer bit needs to fill in the number of digits of 0,
             * assuming that difftotal is always greater than diffDecimer
             */
            diffInterger = difftotal - diffDecimer;
        }

        /* The current decimal and integer bits are integrated into the TMP array,
         * divided into the following parts 0-- >diffInterger-1 integer fill 0 number,
         * diffInterger-- >diffInterger + interger is the number of integer bits,
         * diffInter+ interge --> evenD is the number of decimal bits
         */
        memcpy(&tmp[diffInterger], strInterValue, integer);
        memcpy(&tmp[diffInterger + integer], strDeciValue, decimer);

        tmp[evenD] = 0;

        for (i = 0; i < evenD; i = i + 2)
            out[j++] = (tmp[i] - '0') * 16 + (tmp[i + 1] - '0');
    }
    /* That will not happen now */
    else
        return false;

    return true;
}

int Hydrology_MallocElement(uint8_t element, uint8_t D, uint8_t d,
    HydrologyElement *ele)
{
    ele->guide[0] = element;
    Hydrology_GetGuideID(&(ele->guide[1]), D, d);

    if (D % 2 == 0) {
        ele->value = (uint8_t *)kmalloc(D / 2, __GFP_ZERO);

        if (ele->value == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "element->value malloc failed");
            return false;
        }

        ele->num = D / 2;
    } else {
        ele->value = (uint8_t *)kmalloc((D + 1) / 2, __GFP_ZERO);

        if (ele->value == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "element->value malloc failed");
            return false;
        }

        ele->num = (D + 1) / 2;
    }

    return true;
}

//int Hydrology_ReadAnalog(float *value, int index)
//{
//    long addr = HYDROLOGY_ANALOG1 + index * 4;
//    uint8_t temp_value[4];
//    int ret;
//
//    ret = Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA,
//            addr, temp_value, HYDROLOGY_ANALOG_LEN);
//
//    if(ret == true)
//        *value = *((float *)temp_value);
//    else
//        *value = 0;
//
//    return ret;
//}
//
//int Hydrology_ReadPulse(long *value, int index)
//{
//    long addr = HYDROLOGY_PULSE1 + index * 4;
//    uint8_t temp_value[4];
//    int ret;
//
//    ret = Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA,
//            addr, temp_value, HYDROLOGY_PULSE_LEN);
//
//    if(ret == true)
//        *value = *((long *)temp_value);
//    else
//        *value = 0;
//
//    return ret;
//}
//
//int Hydrology_ReadSwitch(int *value)
//{
//    uint8_t temp_value[4];
//    int ret;
//
//    ret = Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA,
//            HYDROLOGY_SWITCH1, temp_value, HYDROLOGY_SWITCH_LEN);
//
//    if(ret == true)
//        *value = *((int *)temp_value);
//    else
//        *value = 0;
//
//    return ret;
//}

//extern int IsDebug;
//extern uint8_t IsQuery;
//extern uint8_t isUARTConfig;
//
//uint8_t ADCElementCount = 0;
//uint8_t ISR_COUNTElementCount = 0;
//uint8_t IO_STATUSElementCount = 0;
//uint8_t RS485ElementCount = 0;

//int Hydrology_CalElementInfo(void)
//{
//    int i = 0, acount = 0, pocunt = 0;
//    float floatvalue = 0;
//    long intvalue1 = 0;
//    int intvalue2 = 0;
//    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;
//    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

//    switch(Funcode)
//    {
//        case LinkMaintenance:
//            break;

//        case EvenPeriodInformation:
//        case Hour:
//            g_Hydrology.epi =
//                (HydrologyEvenPeriodInfo **)kmalloc(sizeof(HydrologyEvenPeriodInfo *) * upbody->count);

//            if(g_Hydrology.epi == NULL)
//            {
//    kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.epi malloc failed", i);
//                return false;
//            }

//            Hydrology_ReadStoreInfo(HYDROLOGY_FILE_EPI, 0, g_Hydrology.epi, 12 * 2 * upbody->count);
//            break;

//        case Test:
//        case TimerReport:
//        case AddReport:
//            for(i = 0; i < upbody->count; i++)
//            {
//                switch(Element_table[i].type)
//                {
//                    case HYDROLOGY_ANALOG:
//                        if(Hydrology_ReadAnalog(&floatvalue, acount++) == false)
//                            return false;

//                        if(Hydrology_MallocElement(Element_table[i].ID,
//                                Element_table[i].D, Element_table[i].d,
//                                upbody->element[i]) == false)
//                            return false;

//                        Hydrology_ConvertToHexElement((double)floatvalue,
//                            Element_table[i].D, Element_table[i].d,
//                            upbody->element[i]->value);
//                        break;

//                    case HYDROLOGY_PULSE:
//                        if(Hydrology_ReadPulse(&intvalue1, pocunt++) == false)
//                            return false;

//                        if(Hydrology_MallocElement(Element_table[i].ID,
//                                Element_table[i].D, Element_table[i].d,
//                                upbody->element[i]) == false)
//                            return false;

//                        Hydrology_ConvertToHexElement((double)intvalue1,
//                            Element_table[i].D, Element_table[i].d,
//                            upbody->element[i]->value);
//                        break;

//                    case HYDROLOGY_SWITCH:
//                        if(Hydrology_ReadSwitch(&intvalue2) == false)
//                            return false;

//                        if(Hydrology_MallocElement(Element_table[i].ID,
//                                Element_table[i].D, Element_table[i].d,
//                                upbody->element[i]) == false)
//                            return false;

//                        Hydrology_ConvertToHexElement((double)intvalue2,
//                            Element_table[i].D, Element_table[i].d,
//                            upbody->element[i]->value);
//                        break;
//                }
//            }

//            break;

//        case ArtificialNumber:
//        case Picture:
//            upbody->element[i]->guide[0] = Element_table[i].ID;
//            upbody->element[i]->guide[1] = Element_table[i].ID;

////            if(endpoint->CurrentTimes != endpoint->TotalTimes)
////                maxpacket = endpoint->MaxPacket;
////            else
////                maxpacket = endpoint->TotalPacket % endpoint->MaxPacket;
////
////            upbody->element[i]->value = (uint8_t *)kmalloc(maxpacket, __GFP_ZERO);
////
////            if(upbody->element[i]->value == NULL)
////            {
////    kinetis_debug_trace(KERN_DEBUG, "upbody->element[%d]->value malloc failed", i);
////                return false;
////            }
////            else
////            {
////                upbody->element[i]->num = maxpacket;
////                Hydrology_ReadStoreInfo("HydrologyPicture.jpg",
////                    endpoint->CurrentTimes * endpoint->MaxPacket,
////                    upbody->element[i]->value, maxpacket);
////            }

//            upbody->element[i]->value =
//                (uint8_t *)kmalloc(strlen("HydrologyPicture.jpg"), __GFP_ZERO);

//            if(upbody->element[i]->value == NULL)
//            {
//    kinetis_debug_trace(KERN_DEBUG, "upbody->element[%d]->value malloc failed", i);
//                return false;
//            }
//            else
//            {
//                Hydrology_ReadFileSize("HydrologyPicture.jpg",
//                    &(upbody->element[i]->num));
//                memcpy(upbody->element[i]->value, "HydrologyPicture.jpg",
//                    strlen("HydrologyPicture.jpg"));
//            }


//            break;

//        case Realtime:
//        case Period:
//        case InquireArtificialNumber:
//        case SpecifiedElement:
//        case ConfigurationModification:
//        case ConfigurationRead:
//        case ParameterModification:
//        case ParameterRead:
//            for(i = 0; i < downbody->count; i++)
//            {
//                upbody->element[i]->num = (downbody->element[i]->guide[1] >> 3);
//                upbody->element[i]->value = (uint8_t *)kmalloc(upbody->element[i]->num, __GFP_ZERO);

//                if(upbody->element[i]->value == NULL)
//                    continue;

//                HydrologyReadSuiteElement(Funcode,
//                    upbody->element[i]->guide, upbody->element[i]->value);
//            }

//            break;

//        case WaterPumpMotor:
//        case SoftwareVersion:
//        case Status:
//        case InitializeSolidStorage:
//        case Reset:
//        case ChangePassword:
//        case SetClock:
//        case SetICCard:
//        case Pump:
//        case Valve:
//        case Gate:
//        case WaterSetting:
//        case Record:
//        case Time:

//            break;
//    }

//    return true;
//}

void Hydrology_GetStreamID(uint8_t *streamid)
{
    static unsigned short id = 0;
    id++;
    id = id % 65536;

    if (id == 0)
        id = 1;

    streamid[0] = (id >> 8) & 0xff;
    streamid[1] = id & 0xff;
}

int Hydrology_ReadSpecifiedElementInfo(HydrologyElementInfo *Element,
    HydrologyBodyType Funcode, uint16_t Index)
{
    uint32_t Addr = 0;
    int ret;

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
        case Realtime:
        case Period:
        case InquireArtificialNumber:
        case SpecifiedElement:
        case WaterPumpMotor:
        case Status:
        case SetICCard:
            if (Index > 0x75)
                Index -= 0xF0;
            else
                Index += 0x0D;

            Addr = Index * sizeof(HydrologyElementInfo);
            break;

        case ConfigurationModification:
        case ConfigurationRead:
            Index += 131;
            Addr = (Index - 1) * sizeof(HydrologyElementInfo);
            break;

        case ParameterModification:
        case ParameterRead:
            Index += 131 + 15;
            Index -= 0x20;
            Addr = Index * sizeof(HydrologyElementInfo);
            break;

        case SoftwareVersion:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
            Index += 131 + 15;
            Index -= 0x20;
            Addr = Index * sizeof(HydrologyElementInfo);
            break;

        case ChangePassword:
            if (Index == 0x03) {
                Index += 131;
                Addr = (Index - 1) * sizeof(HydrologyElementInfo);
            } else {
                Index += 131 + 15;
                Index -= 0x20;
                Addr = Index * sizeof(HydrologyElementInfo);
            }

            break;

        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Time:
            break;
    }

    ret = Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_INFO, Addr, (uint8_t *)Element,
            sizeof(HydrologyElementInfo));

    return ret;
}

#ifdef DESIGN_VERIFICATION_HYDROLOGY
#include "dv/k-test.h"

int t_HydrologyD_M1M2(HydrologyMode Mode);
int t_HydrologyH_M1M2M3(HydrologyMode Mode);
int t_HydrologyD_M3(void);
int t_HydrologyD_M4(void);
int t_HydrologyH_M4(void);

int t_Hydrology(int argc, char **argv)
{
    int ret = false;
    uint8_t Host = false;
    HydrologyMode Mode = HYDROLOGY_M1;

    if (argc > 1) {
        if (!strcmp(argv[1], "Host"))
            Host = true;
        else
            Host = false;

    }

    if (argc > 2) {
        if (!strcmp(argv[2], "M1"))
            Mode = HYDROLOGY_M1;
        else if (!strcmp(argv[2], "M2"))
            Mode = HYDROLOGY_M2;
        else if (!strcmp(argv[2], "M3"))
            Mode = HYDROLOGY_M3;
        else if (!strcmp(argv[2], "M4"))
            Mode = HYDROLOGY_M4;

    }

    if (Host == false) {
        switch (Mode) {
            case HYDROLOGY_M1:
                ret = t_HydrologyD_M1M2(HYDROLOGY_M1);
                break;

            case HYDROLOGY_M2:
                ret = t_HydrologyD_M1M2(HYDROLOGY_M2);
                break;

            case HYDROLOGY_M3:
                ret = t_HydrologyD_M3();
                break;

            case HYDROLOGY_M4:
                ret = t_HydrologyD_M4();
                break;
        }
    } else {
        switch (Mode) {
            case HYDROLOGY_M1:
                ret = t_HydrologyH_M1M2M3(HYDROLOGY_M1);
                break;

            case HYDROLOGY_M2:
                ret = t_HydrologyH_M1M2M3(HYDROLOGY_M2);
                break;

            case HYDROLOGY_M3:
                ret = t_HydrologyH_M1M2M3(HYDROLOGY_M3);
                break;

            case HYDROLOGY_M4:
                ret = t_HydrologyH_M4();
                break;
        }
    }

    if (ret == true)
        return PASS;
    else
        return FAIL;
}

#endif

