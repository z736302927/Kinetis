#include "kinetis/my9221.h"

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
#include "kinetis/rng.h"
#include "spi.h"
#include <linux/delay.h>

#define DEBUG
#include "kinetis/idebug.h"

#define my9221_printf                  p_dbg

#define MY9221_NUM                     40

typedef struct _RGBLED_TypeDef {
    u16 R;
    u16 G;
    u16 B;
} RGBLED_TypeDef;

typedef struct _MY9221_TypeDef {
    unsigned TEMP: 5;
    unsigned HSPD: 1;
    unsigned BS: 2;
    unsigned GCK: 2;
    unsigned SEP: 1;
    unsigned OSC: 1;
    unsigned POL: 1;
    unsigned CNTSET: 1;
    unsigned ONEST: 1;

    RGBLED_TypeDef Led1;
    RGBLED_TypeDef Led2;
    RGBLED_TypeDef Led3;
    RGBLED_TypeDef Led4;
} MY9221_TypeDef;

void my9221_PortTransmmit(u16 Data)
{

}

u16 my9221_PortReceive(void)
{
    u16 Data = 0;

    return Data;
}

void my9221_port_multi_transmmit(u16 *pData, u32 Length)
{

}

void my9221_port_multi_receive(u16 *pData, u32 Length)
{

}

void my9221_DI_Low(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);
}

void my9221_DI_High(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
}

void my9221_Delay70ns(void)
{

}

void my9221_Delay230ns(void)
{

}

void my9221_Delay600ns(void)
{

}

void my9221_Delayus(u32 ticks)
{
    udelay(ticks);
}

void my9221_Delayms(u32 ticks)
{
    mdelay(ticks);
}

u32 my9221_GetTick(void)
{
    return basic_timer_get_ms_tick();
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

MY9221_TypeDef g_my9221[MY9221_NUM];

/*
 * Free Run Mode:   osc = x & cntset = 0 & onest = x
 * Forced Run Mode: osc = 1 & cntset = 1 & onest = 0
 * One Shot Mode:   osc = 1 & cntset = 1 & onest = 1
*/

void my9221_Init(unsigned osc, unsigned cntset, unsigned onest)
{
    for (u8 i = 0; i < MY9221_NUM; i++) {
        /* Reserved */
        g_my9221[i].TEMP = 0;
        /* Selection of output current reaction rate */
        g_my9221[i].HSPD = 1;
        /* Gray-scale*/
        g_my9221[i].BS = 0;
        /* Built-in gray scale clock rate */
        g_my9221[i].GCK = 0;
        /* Output current dispersion and non-dispersion */
        g_my9221[i].SEP = 0;
        /* Gray-scale clock frequency source */
        g_my9221[i].OSC = osc;
        /* Output current polarity */
        g_my9221[i].POL = 0;
        /* Automatic change screen mode or forced change screen mode */
        g_my9221[i].CNTSET = cntset;
        /* Repeat display or single display */
        g_my9221[i].ONEST = onest;
    }
}

void my9221_InternalLatchPulseGeneration(void)
{
    /* When all gray-scale data is transferred to the shift register, keep DCKI at
     * a fixed reference (either high or low) and maintain it above 220us.(Tstart > 220us)
     */
    my9221_Delayus(220);
    /* Transmit 4 DI signals.(twH (DI) > 70 ns, these (DI) > 230 ns, Tstop *)
     */
    my9221_DI_High();
    my9221_Delay70ns();
    my9221_DI_Low();
    my9221_Delay230ns();
    my9221_DI_High();
    my9221_Delay70ns();
    my9221_DI_Low();
    my9221_Delay230ns();
    my9221_DI_High();
    my9221_Delay70ns();
    my9221_DI_Low();
    my9221_Delay230ns();
    my9221_DI_High();
    my9221_Delay70ns();
    my9221_DI_Low();
    /* After the descending edge of the fourth DI signal, the Tstop* > 200ns can be
     * used to transmit new gray-scale data.
     * Note: in tandem applications, Tsop(minimum) must be greater than [200ns+N*10ns],
     * where N is the number of chips in series.
     */
    my9221_Delay600ns();;
}

void my9221_SendPacket(void)
{
    my9221_port_multi_transmmit((u16 *)g_my9221, 14 * MY9221_NUM);
    my9221_InternalLatchPulseGeneration();
}

#ifdef DESIGN_VERIFICATION_MY9221
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







