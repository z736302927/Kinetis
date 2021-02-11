#include "tlc5971/tlc5971.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

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
#include "kinetis/idebug.h"

#define tlc5971_printf                  p_dbg

#define TLC5971_NUM                     40

typedef struct _TLC5971_TypeDef {
    u16 GSR0;
    u16 GSG0;
    u16 GSB0;
    u16 GSR1;
    u16 GSG1;
    u16 GSB1;
    u16 GSR2;
    u16 GSG2;
    u16 GSB2;
    u16 GSR3;
    u16 GSG3;
    u16 GSB3;

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

void tlc5971_PortTransmmit(u16 Data)
{
//  HAL_SPI_Transmit(&hspi5, &Data, 1, 1000);
}

u16 tlc5971_PortReceive(void)
{
    u16 Data = 0;

//  HAL_SPI_Receive(&hspi5, &Data, 1, 1000);

    return Data;
}

void tlc5971_port_multi_transmmit(u16 *pData, u32 Length)
{
//  HAL_SPI_Transmit(&hspi5, pData, Length, 1000);
    while (HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY) {
    }
}

void tlc5971_port_multi_receive(u16 *pData, u32 Length)
{
//  HAL_SPI_Receive(&hspi5, pData, Length, 1000);
    while (HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY) {
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

void tlc5971_Delayus(u32 ticks)
{
    udelay(ticks);
}

void tlc5971_Delayms(u32 ticks)
{
    mdelay(ticks);
}

u32 tlc5971_GetTick(void)
{
    return basic_timer_get_ms();
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

TLC5971_TypeDef g_tcl5971[TLC5971_NUM];

void tlc5971_Init(void)
{
    for (u8 i = 0; i < TLC5971_NUM; i++) {
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
    for (u8 i = 0; i < TLC5971_NUM; i++) {
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
    for (u8 i = 0; i < TLC5971_NUM; i++) {
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
    for (u8 i = 0; i < TLC5971_NUM; i++) {
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
    tlc5971_Delayus((u32)(g_PortSpeed * 8));
    /* The next shift clock should start after 1.34 us
      or more from the internal latch pulse generation timing.
    */
    tlc5971_Delayus(1.34);
}

void tlc5971_SyncWithSCKI(void)
{
    tlc5971_port_multi_transmmit(0x00, 65536 / 16);
}

/* This mode is ideal for illumination applications that change the display image
 * at low frequencies.
 */
void tlc5971_SendPacketWithTiming1(void)
{
    tlc5971_port_multi_transmmit((u16 *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_InternalLatchPulseGeneration();
}

/* This mode is ideal for video image applications that change the display image
 * with high frequencies or for certain display applications that must synchronize
 * all TLC5971s.
 */
void tlc5971_SendPacketWithTiming2(void)
{
    tlc5971_port_multi_transmmit((u16 *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_InternalLatchPulseGeneration();
    tlc5971_port_multi_transmmit(0x00, 4096);
}

/* There is another control procedure that is recommended for a long chain of
 * cascaded devices.
 */
void tlc5971_SendPacketWithTiming3(void)
{
    tlc5971_port_multi_transmmit((u16 *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_InternalLatchPulseGeneration();
    tlc5971_port_multi_transmmit(0x00, (65536 - TLC5971_NUM * 224) / 16);
}

#ifdef DESIGN_VERIFICATION_TLC5971
static u8 tx_buffer[256];
static u8 rx_buffer[256];

void ds3231_Test(void)
{
    u32 TmpRngdata = 0;
    u16 BufferLength = 0;
    u32 TestAddr = 0;

    Random_Get8bit(&hrng, &TmpRngdata);
    BufferLength = TmpRngdata & 0xFF;
    ds3231_printf("BufferLength = %d.", BufferLength);

    if (tx_buffer == NULL || rx_buffer == NULL) {
        ds3231_printf("Failed to allocate memory !");
        return;
    }

    memset(tx_buffer, 0, BufferLength);
    memset(rx_buffer, 0, BufferLength);

    Random_Get8bit(&hrng, &TmpRngdata);
    TestAddr = TmpRngdata & 0xFF;
    ds3231_printf("TestAddr = 0x%02X.", TestAddr);

    for (u16 i = 0; i < BufferLength; i += 4) {
        Random_Get8bit(&hrng, &TmpRngdata);
        tx_buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
        tx_buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
        tx_buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
        tx_buffer[i + 0] = (TmpRngdata & 0x000000FF);
    }

    ds3231_WriteData(TestAddr, tx_buffer, BufferLength);
    ds3231_ReadData(TestAddr, rx_buffer, BufferLength);

    for (u16 i = 0; i < BufferLength; i++) {
        if (tx_buffer[i] != rx_buffer[i]) {
            ds3231_printf("tx_buffer[%d] = 0x%02X, rx_buffer[%d] = 0x%02X",
                i, tx_buffer[i],
                i, rx_buffer[i]);
            ds3231_printf("Data writes and reads do not match, TEST FAILED !");
            return ;
        }
    }

    ds3231_printf("ds3231 Read and write TEST PASSED !");
}

#endif







