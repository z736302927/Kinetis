#include "chip/at24cxx.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "protocol/iic_soft.h"
#include "timer/k-delay.h"
#include "string.h"
#include "rng.h"
#include "i2c.h"

#define DEBUG
#include "core/idebug.h"

#define at24cxx_printf                  p_dbg

#define AT24CXX_ADDR                    0x50
#define PAGE_SIZE                       8
#define AT24CXX_MAX_ADDR                255
#define AT24CXX_VOLUME                  256

void at24cxx_Delayus(uint32_t ticks)
{
    Delay_us(ticks);
}

void at24cxx_Delayms(uint32_t ticks)
{
    Delay_ms(ticks);
}

void at24cxx_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    IIC_PortMultiTransmmit(IIC_1, AT24CXX_ADDR, Addr, pData, Length);
}

void at24cxx_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    IIC_PortMultiReceive(IIC_1, AT24CXX_ADDR, Addr, pData, Length);
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void at24cxx_ByteWrite(uint8_t Addr, uint8_t Data)
{
    at24cxx_PortMultiTransmmit(Addr, &Data, 1);
    at24cxx_Delayms(5);
}

void at24cxx_PageWrite(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    at24cxx_PortMultiTransmmit(Addr, pData, Length);
}

void at24cxx_MultiPageWrite(uint32_t Addr, uint8_t *pData, uint16_t Length)
{
    uint8_t NumOfPage = 0, NumOfSingle = 0, SubAddr = 0, Count = 0, Temp = 0;

    /* Mod operation, if Addr is an integer multiple of PAGE_SIZE, SubAddr value is 0 */
    SubAddr = Addr % PAGE_SIZE;

    /* The difference count is just enough to line up to the page address */
    Count = PAGE_SIZE - SubAddr;
    /* Figure out how many integer pages to write */
    NumOfPage =  Length / PAGE_SIZE;
    /* mod operation is used to calculate the number of bytes less than one page */
    NumOfSingle = Length % PAGE_SIZE;

    /* SubAddr=0, then Addr is just aligned by page */
    if(SubAddr == 0)
    {
        /* Length < PAGE_SIZE */
        if(NumOfPage == 0)
            at24cxx_PageWrite(Addr, pData, Length);
        else /* Length > PAGE_SIZE */
        {
            /* Let me write down all the integer pages */
            while(NumOfPage--)
            {
                at24cxx_PageWrite(Addr, pData, PAGE_SIZE);
                Addr +=  PAGE_SIZE;
                pData += PAGE_SIZE;
            }

            /* If you have more than one page of data, write it down*/
            at24cxx_PageWrite(Addr, pData, NumOfSingle);
        }
    }
    /* If the address is not aligned with PAGE_SIZE */
    else
    {
        /* Length < PAGE_SIZE */
        if(NumOfPage == 0)
        {
            /* The remaining count positions on the current page are smaller than NumOfSingle */
            if(NumOfSingle > Count)
            {
                Temp = NumOfSingle - Count;

                /* Fill in the front page first */
                at24cxx_PageWrite(Addr, pData, Count);
                Addr +=  Count;
                pData += Count;

                /* Let me write the rest of the data */
                at24cxx_PageWrite(Addr, pData, Temp);
            }
            else /* The remaining count position of the current page can write NumOfSingle data */
                at24cxx_PageWrite(Addr, pData, Length);
        }
        else /* Length > PAGE_SIZE */
        {
            /* The address is not aligned and the extra count is treated separately, not added to the operation */
            Length -= Count;
            NumOfPage =  Length / PAGE_SIZE;
            NumOfSingle = Length % PAGE_SIZE;

            at24cxx_PageWrite(Addr, pData, Count);
            Addr +=  Count;
            pData += Count;

            /* Write all the integer pages */
            while(NumOfPage--)
            {
                at24cxx_PageWrite(Addr, pData, PAGE_SIZE);
                Addr +=  PAGE_SIZE;
                pData += PAGE_SIZE;
            }

            /* If you have more than one page of data, write it down */
            if(NumOfSingle != 0)
                at24cxx_PageWrite(Addr, pData, NumOfSingle);
        }
    }
}

void at24cxx_WriteData(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    uint32_t RemainSpace = 0;

    RemainSpace = AT24CXX_MAX_ADDR - Addr;

    if(RemainSpace < Length)
    {
        kinetis_debug_trace(KERN_DEBUG, "There is not enough space left to write the specified length.");
        return ;
    }

    at24cxx_MultiPageWrite(Addr, pData, Length);
}

void at24cxx_CurrentAddrRead(uint8_t *pData)
{
    IIC_Soft_Start();
    IIC_Soft_SendByte((AT24CXX_ADDR << 1) | 0x01);

    if(IIC_Soft_WaitAck())
    {
        IIC_Soft_Stop();
        return ;
    }

    *pData = IIC_Soft_ReadByte(0);
    IIC_Soft_Stop();
}

void at24cxx_RandomRead(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    uint32_t RemainSpace = 0;

    RemainSpace = AT24CXX_MAX_ADDR - Addr;

    if(RemainSpace < Length)
    {
        kinetis_debug_trace(KERN_DEBUG, "There is not enough space left to read the specified length.");
        return ;
    }

    at24cxx_PortMultiReceive(Addr, pData, Length);
}

void at24cxx_ReadData(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
    at24cxx_RandomRead(Addr, pData, Length);
}

void at24cxx_SequentialRead(uint8_t *pData, uint32_t Length)
{
    IIC_Soft_Start();
    IIC_Soft_SendByte((AT24CXX_ADDR << 1) | 0x01);

    if(IIC_Soft_WaitAck())
    {
        IIC_Soft_Stop();
        return ;
    }

    while(Length)
    {
        if(Length == 1)
            *pData = IIC_Soft_ReadByte(0);
        else
            *pData = IIC_Soft_ReadByte(1);

        pData++;
        Length--;
    }

    IIC_Soft_Stop();
}

#ifdef DESIGN_VERIFICATION_AT24CXX
#include "dv/k-test.h"
#include "stdlib.h"
#include "dv/k-rng.h"
#include "timer/k-basictimer.h"

static uint8_t Tx_Buffer[AT24CXX_VOLUME];
static uint8_t Rx_Buffer[AT24CXX_VOLUME];

int t_at24cxx_ReadWirte(int argc, char **argv)
{
    uint16_t BufferLength = 0;
    uint32_t TestAddr = 0;
    uint16_t times = 128;
    uint16_t i = 0, j = 0;

    if(argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    for(j = 0; j < times; j++)
    {
        BufferLength = Random_Get8bit();

        if(BufferLength <= 0)
            BufferLength = 10;

        TestAddr = Random_Get8bit();

        if(BufferLength >= (AT24CXX_MAX_ADDR - TestAddr + 1))
            BufferLength = AT24CXX_MAX_ADDR - TestAddr + 1;

        memset(Tx_Buffer, 0, BufferLength);
        memset(Rx_Buffer, 0, BufferLength);
        kinetis_debug_trace(KERN_DEBUG, "BufferLength = %d.", BufferLength);
        kinetis_debug_trace(KERN_DEBUG, "TestAddr = 0x%02X.", TestAddr);

        for(i = 0; i < BufferLength; i++)
            Tx_Buffer[i] = Random_Get8bit();

        at24cxx_WriteData(TestAddr, Tx_Buffer, BufferLength);
        at24cxx_ReadData(TestAddr, Rx_Buffer, BufferLength);

        for(i = 0; i < BufferLength; i++)
        {
            if(Tx_Buffer[i] != Rx_Buffer[i])
            {
                kinetis_debug_trace(KERN_DEBUG, "Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X",
                    i, Tx_Buffer[i], i, Rx_Buffer[i]);
                kinetis_debug_trace(KERN_DEBUG, "Data writes and reads do not match, TEST FAILED !");
                return FAIL;
            }
        }
    }

    kinetis_debug_trace(KERN_DEBUG, "at24cxx Read and write TEST PASSED !");

    return PASS;
}

int t_at24cxx_CurrentAddrRead(int argc, char **argv)
{
    uint8_t Data = 0;

    at24cxx_CurrentAddrRead(&Data);
    kinetis_debug_trace(KERN_DEBUG, "at24cxx current address data %d", Data);

    return PASS;
}

int t_at24cxx_RandomRead(int argc, char **argv)
{
    uint16_t TestLen = 0;
    uint32_t TestAddr = 0;
    uint16_t times = 128;
    uint16_t i = 0;

    if(argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    TestAddr = Random_Get8bit();
    TestLen = Random_Get8bit() % (AT24CXX_MAX_ADDR - TestAddr);
    at24cxx_RandomRead(TestAddr, &Rx_Buffer[TestAddr], TestLen);
    kinetis_debug_trace(KERN_DEBUG, "at24cxx Random Read");

    for(i = 0; i < TestLen; i++)
        kinetis_debug_trace(KERN_DEBUG, "Data[%d] = %d", i, Rx_Buffer[TestAddr + i]);

    return PASS;
}

int t_at24cxx_SequentialRead(int argc, char **argv)
{
    uint8_t Data = 0;

    at24cxx_SequentialRead(&Data, 1);
    kinetis_debug_trace(KERN_DEBUG, "at24cxx Sequential Read Data %d", Data);

    return PASS;
}

int t_at24cxx_ReadWirteSpeed(int argc, char **argv)
{
    uint32_t Timestamp = 0;
    uint16_t i = 0;

    kinetis_debug_trace(KERN_DEBUG, "Starting at24cxx raw write test");
    Timestamp = BasicTimer_GetUSTick();

    for(i = 0; i < AT24CXX_VOLUME; i++)
        Tx_Buffer[i] = Random_Get8bit();

    at24cxx_WriteData(0, Tx_Buffer, AT24CXX_VOLUME);

    Timestamp = BasicTimer_GetUSTick() - Timestamp;
    kinetis_debug_trace(KERN_DEBUG, "%u bytes written and it took %luus.", AT24CXX_VOLUME, Timestamp);

    kinetis_debug_trace(KERN_DEBUG, "Starting at24cxx raw read test");
    Timestamp = BasicTimer_GetUSTick();

    at24cxx_ReadData(0, Rx_Buffer, AT24CXX_VOLUME);

    Timestamp = BasicTimer_GetUSTick() - Timestamp;
    kinetis_debug_trace(KERN_DEBUG, "%u bytes read and it took %luus.", AT24CXX_VOLUME, Timestamp);
    kinetis_debug_trace(KERN_DEBUG, "Test completed.");

    return PASS;
}

#endif
