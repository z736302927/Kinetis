#include "tlc5971/tlc5971.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "stdlib.h"
#include "string.h"
#include "rng.h"
#include "spi.h"

#define DEBUG
#include "core/idebug.h"

#define tlc5971_printf                  p_dbg

#define TLC5971_NUM                     40

typedef struct _TLC5971_TypeDef
{
    uint16_t GSR0;
    uint16_t GSG0;
    uint16_t GSB0;
    uint16_t GSR1;
    uint16_t GSG1;
    uint16_t GSB1;
    uint16_t GSR2;
    uint16_t GSG2;
    uint16_t GSB2;
    uint16_t GSR3;
    uint16_t GSG3;
    uint16_t GSB3;

    unsigned BCR: 7;
    unsigned BCG: 7;
    unsigned BCB: 7;
    unsigned BLANK: 1;
    unsigned DSPRPT: 1;
    unsigned TMGRST: 1;
    unsigned EXTGCK: 1;
    unsigned OUTTMG: 1;

    unsigned WriteCommand: 6;
} TLC5971_TypeDef;

float g_PortSpeed = 0.05;

void tlc5971_PortTransmmit(uint16_t Data)
{
//  HAL_SPI_Transmit(&hspi5, &Data, 1, 1000);
}

uint16_t tlc5971_PortReceive(void)
{
    uint16_t Data = 0;

//  HAL_SPI_Receive(&hspi5, &Data, 1, 1000);

    return Data;
}

void tlc5971_PortMultiTransmmit(uint16_t *pData, uint32_t Length)
{
//  HAL_SPI_Transmit(&hspi5, pData, Length, 1000);
    while(HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY)
    {
    }
}

void tlc5971_PortMultiReceive(uint16_t *pData, uint32_t Length)
{
//  HAL_SPI_Receive(&hspi5, pData, Length, 1000);
    while(HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY)
    {
    }
}

void tlc5971_CS_Low(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);
}

void tlc5971_CS_High(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
}

void tlc5971_SetPortSpeed(void)
{
    g_PortSpeed = 0.05;
}

/* The time consumed by one of the clock edges of the SPI, unit is us. */
float tlc5971_GetPortSpeed(void)
{
    return g_PortSpeed;
}

void tlc5971_HardReset(void)
{

}

void tlc5971_Delayus(uint32_t ticks)
{
    Delay_us(ticks);
}

void tlc5971_Delayms(uint32_t ticks)
{
    Delay_ms(ticks);
}

uint32_t tlc5971_GetTick(void)
{
    return BasicTimer_GetMSTick();
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

TLC5971_TypeDef g_tcl5971[TLC5971_NUM];

void tlc5971_Init(void)
{
    for(uint8_t i = 0; i < TLC5971_NUM; i++)
    {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].BLANK = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].DSPRPT = 0;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].TMGRST = 0;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].EXTGCK = 1;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].OUTTMG = 1;
        /*  */
        g_tcl5971[i].WriteCommand = 0x25;
    }
}

void tlc5971_InitForTiming1(void)
{
    for(uint8_t i = 0; i < TLC5971_NUM; i++)
    {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].BLANK = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].DSPRPT = 1;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].TMGRST = 0;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].EXTGCK = 0;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].OUTTMG = 1;
        /*  */
        g_tcl5971[i].WriteCommand = 0x25;
    }
}

void tlc5971_InitForTiming2(void)
{
    for(uint8_t i = 0; i < TLC5971_NUM; i++)
    {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].BLANK = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].DSPRPT = 0;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].TMGRST = 1;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].EXTGCK = 1;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].OUTTMG = 1;
        /*  */
        g_tcl5971[i].WriteCommand = 0x25;
    }
}

void tlc5971_InitForTiming3(void)
{
    for(uint8_t i = 0; i < TLC5971_NUM; i++)
    {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].BLANK = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].DSPRPT = 0;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].TMGRST = 1;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].EXTGCK = 1;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].OUTTMG = 1;
        /*  */
        g_tcl5971[i].WriteCommand = 0x25;
    }
}

void tlc5971_InternalLatchPulseGeneration(void)
{
    /* The time that generates the internal latch pulse is 8x the period between the last
      SCLK rising edge and the second to last SCLK rising edge. The time changes
      depending on the period of the shift clock within the range of 2.74 ms to 666 ns.
    */
    tlc5971_Delayus((uint32_t)(g_PortSpeed * 8));
    /*The next shift clock should start after 1.34 us
      or more from the internal latch pulse generation timing.
    */
    tlc5971_Delayus(1.34);
}

void tlc5971_SyncWithSCKI(void)
{
    tlc5971_PortMultiTransmmit(0x00, 65536 / 16);
}

/* This mode is ideal for illumination applications that change the display image
 * at low frequencies.
 */
void tlc5971_SendPacketWithTiming1(void)
{
    tlc5971_PortMultiTransmmit((uint16_t *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_InternalLatchPulseGeneration();
}

/* This mode is ideal for video image applications that change the display image
 * with high frequencies or for certain display applications that must synchronize
 * all TLC5971s.
 */
void tlc5971_SendPacketWithTiming2(void)
{
    tlc5971_PortMultiTransmmit((uint16_t *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_InternalLatchPulseGeneration();
    tlc5971_PortMultiTransmmit(0x00, 4096);
}

/* There is another control procedure that is recommended for a long chain of
 * cascaded devices.
 */
void tlc5971_SendPacketWithTiming3(void)
{
    tlc5971_PortMultiTransmmit((uint16_t *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_InternalLatchPulseGeneration();
    tlc5971_PortMultiTransmmit(0x00, (65536 - TLC5971_NUM * 224) / 16);
}

#ifdef DESIGN_VERIFICATION_TLC5971
static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void ds3231_Test(void)
{
    uint32_t TmpRngdata = 0;
    uint16_t BufferLength = 0;
    uint32_t TestAddr = 0;

    Random_Get8bit(&hrng, &TmpRngdata);
    BufferLength = TmpRngdata & 0xFF;
    ds3231_printf("BufferLength = %d.", BufferLength);

    if(Tx_Buffer == NULL || Rx_Buffer == NULL)
    {
        ds3231_printf("Failed to allocate memory !");
        return;
    }

    memset(Tx_Buffer, 0, BufferLength);
    memset(Rx_Buffer, 0, BufferLength);

    Random_Get8bit(&hrng, &TmpRngdata);
    TestAddr = TmpRngdata & 0xFF;
    ds3231_printf("TestAddr = 0x%02X.", TestAddr);

    for(uint16_t i = 0; i < BufferLength; i += 4)
    {
        Random_Get8bit(&hrng, &TmpRngdata);
        Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
        Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
        Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
        Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
    }

    ds3231_WriteData(TestAddr, Tx_Buffer, BufferLength);
    ds3231_ReadData(TestAddr, Rx_Buffer, BufferLength);

    for(uint16_t i = 0; i < BufferLength; i++)
    {
        if(Tx_Buffer[i] != Rx_Buffer[i])
        {
            ds3231_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X",
                i, Tx_Buffer[i],
                i, Rx_Buffer[i]);
            ds3231_printf("Data writes and reads do not match, TEST FAILED !");
            return ;
        }
    }

    ds3231_printf("ds3231 Read and write TEST PASSED !");
}

#endif







