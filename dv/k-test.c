#include "dv/k-test.h"
#include "dv/k-shell.h"
#include "string.h"
#include "stdio.h"
#include "core/k-memory.h"
#include "linux/gfp.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define DEBUG
#include "core/idebug.h"

#ifdef DESIGN_VERIFICATION_AT24CXX
int t_at24cxx_ReadWirte(int argc, char **argv);
int t_at24cxx_CurrentAddrRead(int argc, char **argv);
int t_at24cxx_RandomRead(int argc, char **argv);
int t_at24cxx_SequentialRead(int argc, char **argv);
int t_at24cxx_ReadWirteSpeed(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_BASICTIMER
int t_BasicTimer_GetTick(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_BUTTON
int t_Button_Attach(int argc, char **argv);
int t_Button_Detach(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_DELAY
int t_Delay(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_FATFS
int t_FatFs_ReadWrite(int argc, char **argv);
int t_FatFs_Miscellaneous(int argc, char **argv);
int t_FatFs_File_Check(int argc, char **argv);
int t_FatFs_Scan_Files(int argc, char **argv);
int t_FatFs_Append(int argc, char **argv);
int t_FatFs_Delete_Node(int argc, char **argv);
int t_FatFs_Expend(int argc, char **argv);
int t_FatFs_Diskio(int argc, char **argv);
int t_FatFs_Contiguous_File(int argc, char **argv);
int t_FatFs_Raw_Speed(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_FSM
int t_FSM_Example(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_GENERAL
int t_General_Success(int argc, char **argv);
int t_General_Error(int argc, char **argv);
int t_General_Timeout(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_HC_05
int t_hc_05_TestCommand(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_HYDROLOGY
int t_Hydrology(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_LED
int t_LED_Toggle(int argc, char **argv);
int t_LED_Delete(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_MEMORY
int t_memory_Test(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RNG
int t_Random_Number(int argc, char **argv);
int t_Random_Array(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RS485
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_RTC
int t_RTC_SetClock(int argc, char **argv);
int t_RTC_GetClock(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_RTCTASK
int t_RTCTask_Add(int argc, char **argv);
int t_RTCTask_Delete(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_SEIRALPORT
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_SHELL
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_SLIST
{"test", fuction},
#endif

#ifdef DESIGN_VERIFICATION_SWITCH
int t_Switch_Attach(int argc, char **argv);
int t_Switch_Detach(int argc, char **argv);
#endif

#ifdef DESIGN_VERIFICATION_TIMTASK
int t_TimTask_Add(int argc, char **argv);
int t_TimTask_Add(int argc, char **argv);
#endif

typedef struct _Test_Case_TypeDef
{
    char *Command;
    int (*Function)(int argc, char **argv);
} Test_Case_TypeDef;

Test_Case_TypeDef Kinetis_Case_Table[] =
{
#ifdef DESIGN_VERIFICATION_AK8975
    {"ak8975.BasicInfo", t_ak8975_BasicInfo},
    {"ak8975.Magnetic", t_ak8975_Magnetic},
    {"ak8975.Selftest", t_ak8975_Selftest},
    {"ak8975.FuseROMAccess", t_ak8975_FuseROMAccess},
#endif
#ifdef DESIGN_VERIFICATION_AT24CXX
    {"at24cxx.ReadWirte", t_at24cxx_ReadWirte},
    {"at24cxx.CurrentAddrRead", t_at24cxx_CurrentAddrRead},
    {"at24cxx.RandomRead", t_at24cxx_RandomRead},
    {"at24cxx.SequentialRead", t_at24cxx_SequentialRead},
    {"at24cxx.ReadWirteSpeed", t_at24cxx_ReadWirteSpeed},
#endif
#ifdef DESIGN_VERIFICATION_BMI160
    {"bmi160.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_DS3231
    {"ds3231.SetClock", t_ds3231_SetClock},
    {"ds3231.GetClock", t_ds3231_GetClock},
    {"ds3231.SetAlarm1", t_ds3231_SetAlarm1},
    {"ds3231.SetAlarm2", t_ds3231_SetAlarm2},
    {"ds3231.SquareWave", t_ds3231_SquareWave},
    {"ds3231.32kHzWave", t_ds3231_32kHzWave},
    {"ds3231.GetTemprature", t_ds3231_GetTemprature},
#endif
#ifdef DESIGN_VERIFICATION_ESP32
    {"esp32.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_GPRS
    {"gprs.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_GSM
    {"gsm.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_GT9271
    {"gt9271.", },
#endif
#ifdef DESIGN_VERIFICATION_HC_05
    {"hc-05.Test", t_hc_05_TestCommand},
#endif
#ifdef DESIGN_VERIFICATION_HMC5883L
    {"hmc5883l.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_HYDROLOGY
    {"hydrology.Test", t_Hydrology},
#endif
#ifdef DESIGN_VERIFICATION_ICM20602
    {"icm20602.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_IIC
    {"iic.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_IS25LPWP256D
    {"is25lpwp256d.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_BASICTIMER
    {"basictimer.GetTick", t_BasicTimer_GetTick},
#endif
#ifdef DESIGN_VERIFICATION_CHINESE
    {"chinese.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_CRC
    {"crc.", fuction},
#endif
#ifdef DESIGN_VERIFICATION_DELAY
    {"delay.Delay", t_Delay},
#endif
#ifdef DESIGN_VERIFICATION_FATFS
    {"fatfs.ReadWrite", t_FatFs_ReadWrite},
    {"fatfs.Miscellaneous", t_FatFs_Miscellaneous},
    {"fatfs.FileCheck", t_FatFs_File_Check},
    {"fatfs.ScanFiles", t_FatFs_Scan_Files},
    {"fatfs.Append", t_FatFs_Append},
    {"fatfs.DeleteNode", t_FatFs_Delete_Node},
    {"fatfs.Expend", t_FatFs_Expend},
    {"fatfs.Diskio", t_FatFs_Diskio},
    {"fatfs.ContiguousFile", t_FatFs_Contiguous_File},
    {"fatfs.RawSpeed", t_FatFs_Raw_Speed},
#endif
#ifdef DESIGN_VERIFICATION_FSM
    {"fsm.Example", t_FSM_Example},
#endif
#ifdef DESIGN_VERIFICATION_GENERAL
    {"general.Success", t_General_Success},
    {"general.Error", t_General_Error},
    {"general.Timeout", t_General_Timeout},
#endif
#ifdef DESIGN_VERIFICATION_BUTTON
    {"button.Attach", t_Button_Attach},
    {"button.Detach", t_Button_Detach},
#endif
#ifdef DESIGN_VERIFICATION_LCD
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_LED
    {"led.Toggle", t_LED_Toggle},
    {"led.Delete", t_LED_Delete},
#endif
#ifdef DESIGN_VERIFICATION_MEMORY
    {"memory.Test", t_memory_Test},
#endif
#ifdef DESIGN_VERIFICATION_RNG
    {"random.Number", t_Random_Number},
    {"random.Array", t_Random_Array},
#endif
#ifdef DESIGN_VERIFICATION_RS485
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_RTC
    {"rtc.SetClock", t_RTC_SetClock},
    {"rtc.GetClock", t_RTC_GetClock},
#endif
#ifdef DESIGN_VERIFICATION_RTCTASK
    {"rtctask.Add", t_RTCTask_Add},
    {"rtctask.Delete", t_RTCTask_Delete},
#endif
#ifdef DESIGN_VERIFICATION_SEIRALPORT
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SHELL
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SLIST
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SWITCH
    {"switch.Attach", t_Switch_Attach},
    {"switch.Detach", t_Switch_Detach},
#endif
#ifdef DESIGN_VERIFICATION_TIMTASK
    {"timtask.Add", t_TimTask_Add},
    {"timtask.Delete", t_TimTask_Delete},
#endif
#ifdef DESIGN_VERIFICATION_TOUCHSCREEN
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MAX30205
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MPU6050
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MT29F4G08
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_MY9221
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_NBIOT
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_OLED
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_RAK477
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SHT20
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SPL06
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_SSD1306
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_TLC5971
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_W25QXXX
    {"test", fuction},
#endif
#ifdef DESIGN_VERIFICATION_XMODEM
    {"test", fuction},
#endif
};

/**
 * strpbrk - Find the first occurrence of a set of characters
 * @cs: The string to be searched
 * @ct: The characters to search for
 */
char *strpbrk(const char *cs, const char *ct)
{
    const char *sc1, *sc2;

    for(sc1 = cs; *sc1 != '\0'; ++sc1)
    {
        for(sc2 = ct; *sc2 != '\0'; ++sc2)
        {
            if(*sc1 == *sc2)
                return (char *)sc1;
        }
    }

    return NULL;
}

/**
 * strsep - Split a string into tokens
 * @s: The string to be searched
 * @ct: The characters to search for
 *
 * strsep() updates @s to point after the token, ready for the next call.
 *
 * It returns empty tokens, too, behaving exactly like the libc function
 * of that name. In fact, it was stolen from glibc2 and de-fancy-fied.
 * Same semantics, slimmer shape. ;)
 */
char *strsep(char **s, const char *ct)
{
    char *sbegin = *s;
    char *end;

    if(sbegin == NULL)
        return NULL;

    end = strpbrk(sbegin, ct);

    if(end)
        *end++ = '\0';

    *s = end;
    return sbegin;
}

uint8_t ParseTest_AllCase(char *Command)
{
    uint32_t i = 0;
    uint32_t argc = 0;
    char *argv[128];

    do
    {
        argv[argc] = strsep(&Command, " ");
        kinetis_debug_trace(KERN_DEBUG, "[%d] %s", argc, argv[argc]);
        argc++;
    }
    while(Command);

    for(i = 0; i < sizeof(Kinetis_Case_Table) / sizeof(Test_Case_TypeDef); i++)
    {
        if((!strcmp(Kinetis_Case_Table[i].Command, argv[0])) && (Kinetis_Case_Table[i].Function != NULL))
            return Kinetis_Case_Table[i].Function(argc, argv);
    }

    return NOT_EXSIST;
}

void k_TestCase_Schedule(void)
{
    char *Buffer;
    uint8_t Result;
    Buffer = kmalloc(128, __GFP_ZERO);

    while(1)
    {
        if(shell_GetUserInput(Buffer) == true)
        {
            if(Buffer[0] == '\r')
                printf("/ # ");
            else if(Buffer[0] == 27)
                break;
            else
            {
                Result = ParseTest_AllCase(Buffer);

                if(Result == PASS)
                    kinetis_debug_trace(KERN_DEBUG, "TEST PASS");
                else if(Result == FAIL)
                    kinetis_debug_trace(KERN_DEBUG, "TEST FAIL");
                else
                    kinetis_debug_trace(KERN_DEBUG, "TEST NOT EXSIST");

                printf("\r\n/ # ");
            }
        }
    }

    kfree(Buffer);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/
