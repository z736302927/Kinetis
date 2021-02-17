#include "kinetis/tlc5971.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "spi.h"

#define TLC5971_NUM                     40

struct tlc5971 {
    u16 gsr0;
    u16 gsg0;
    u16 gsb0;
    u16 gsr1;
    u16 gsg1;
    u16 gsb1;
    u16 gsr2;
    u16 gsg2;
    u16 gsb2;
    u16 gsr3;
    u16 gsg3;
    u16 gsb3;

    unsigned bcr: 7;
    unsigned bcg: 7;
    unsigned bcb: 7;
    unsigned blank: 1;
    unsigned dsprpt: 1;
    unsigned tmgrst: 1;
    unsigned extgck: 1;
    unsigned outtmg: 1;

    unsigned write_cmd: 6;
};

static float g_port_speed = 0.05;

static inline void tlc5971_port_transmmit(u16 tmp)
{
//  HAL_SPI_Transmit(&hspi5, &tmp, 1, 1000);
}

static inline u16 tlc5971_port_receive(void)
{
    u16 tmp = 0;

//  HAL_SPI_Receive(&hspi5, &tmp, 1, 1000);

    return tmp;
}

static inline void tlc5971_port_multi_transmmit(u16 *pdata, u32 Length)
{
//  HAL_SPI_Transmit(&hspi5, pdata, Length, 1000);
    while (HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY) {
    }
}

static inline void tlc5971_port_multi_receive(u16 *pdata, u32 Length)
{
//  HAL_SPI_Receive(&hspi5, pdata, Length, 1000);
    while (HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY) {
    }
}

static inline void tlc5971_cs_low(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);
}

static inline void tlc5971_cs_high(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
}

static inline void tlc5971_set_port_speed(void)
{
    g_port_speed = 0.05;
}

/* The time consumed by one of the clock edges of the SPI, unit is us. */
static inline float tlc5971_get_port_speed(void)
{
    return g_port_speed;
}

static inline void tlc5971_hard_reset(void)
{

}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

struct tlc5971 g_tcl5971[TLC5971_NUM];

void tlc5971_init(void)
{
    u32 i;
    
    for (i = 0; i < TLC5971_NUM; i++) {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].blank = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].dsprpt = 0;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].tmgrst = 0;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].extgck = 1;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].outtmg = 1;
        /*  */
        g_tcl5971[i].write_cmd = 0x25;
    }
}

void tlc5971_Init_For_Timing1(void)
{
    for (u8 i = 0; i < TLC5971_NUM; i++) {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].blank = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].dsprpt = 1;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].tmgrst = 0;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].extgck = 0;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].outtmg = 1;
        /*  */
        g_tcl5971[i].write_cmd = 0x25;
    }
}

void tlc5971_Init_For_Timing2(void)
{
    for (u8 i = 0; i < TLC5971_NUM; i++) {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].blank = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].dsprpt = 0;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].tmgrst = 1;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].extgck = 1;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].outtmg = 1;
        /*  */
        g_tcl5971[i].write_cmd = 0x25;
    }
}

void tlc5971_Init_For_Timing3(void)
{
    for (u8 i = 0; i < TLC5971_NUM; i++) {
        /* Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
        g_tcl5971[i].blank = 0;
        /* Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].dsprpt = 0;
        /* Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
        g_tcl5971[i].tmgrst = 1;
        /* GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
        g_tcl5971[i].extgck = 1;
        /* GS reference clock edge select bit for OUTXn on-off timing control in FC data (0 = falling edge, 1 = rising edge). */
        g_tcl5971[i].outtmg = 1;
        /*  */
        g_tcl5971[i].write_cmd = 0x25;
    }
}

static void tlc5971_internal_latch_pulse_generation(void)
{
    /* The time that generates the internal latch pulse is 8x the period between the last
      SCLK rising edge and the second to last SCLK rising edge. The time changes
      depending on the period of the shift clock within the range of 2.74 ms to 666 ns.
    */
    udelay((u32)(g_port_speed * 8));
    /* The next shift clock should start after 1.34 us
      or more from the internal latch pulse generation timing.
    */
    udelay(1.34);
}

void tlc5971_sync_with_scki(void)
{
    tlc5971_port_multi_transmmit(0x00, 65536 / 16);
}

/* This mode is ideal for illumination applications that change the display image
 * at low frequencies.
 */
void tlc5971_send_packet_with_timing1(void)
{
    tlc5971_port_multi_transmmit((u16 *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_internal_latch_pulse_generation();
}

/* This mode is ideal for video image applications that change the display image
 * with high frequencies or for certain display applications that must synchronize
 * all TLC5971s.
 */
void tlc5971_send_packet_with_timing2(void)
{
    tlc5971_port_multi_transmmit((u16 *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_internal_latch_pulse_generation();
    tlc5971_port_multi_transmmit(0x00, 4096);
}

/* There is another control procedure that is recommended for a long chain of
 * cascaded devices.
 */
void tlc5971_send_packet_with_timing3(void)
{
    tlc5971_port_multi_transmmit((u16 *)g_tcl5971, 14 * TLC5971_NUM);
    tlc5971_internal_latch_pulse_generation();
    tlc5971_port_multi_transmmit(0x00, (65536 - TLC5971_NUM * 224) / 16);
}

#ifdef DESIGN_VERIFICATION_TLC5971
#include "kinetis/test-kinetis.h"

int t_tlc5971_send_packet(int argc, char **argv)
{
    tlc5971_send_packet_with_timing1();
    tlc5971_send_packet_with_timing2();
    tlc5971_send_packet_with_timing3();

    return PASS;
}

#endif


