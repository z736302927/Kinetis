#include <generated/deconfig.h> 
//#include "my9221.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define MY9221_NUM                     40

struct rgb_led {
    u16 red;
    u16 green;
    u16 blue;
};

struct my9221 {
    unsigned temp: 5;
    unsigned hspd: 1;
    unsigned bs: 2;
    unsigned gck: 2;
    unsigned sep: 1;
    unsigned osc: 1;
    unsigned pol: 1;
    unsigned cntset: 1;
    unsigned onest: 1;

    struct rgb_led led1;
    struct rgb_led led2;
    struct rgb_led led3;
    struct rgb_led led4;
};

static inline void my9221_port_transmmit(u16 tmp)
{

}

static inline u16 my9221_port_receive(void)
{
    u16 tmp = 0;

    return tmp;
}

static inline void my9221_port_multi_transmmit(u16 *pdata, u32 Length)
{

}

static inline void my9221_port_multi_receive(u16 *pdata, u32 Length)
{

}

static inline void my9221_di_low(void)
{
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}

static inline void my9221_di_high(void)
{
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
}

static inline void my9221_delay_70ns(void)
{

}

static inline void my9221_delay_230ns(void)
{

}

static inline void my9221_delay_600ns(void)
{

}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

struct my9221 g_my9221[MY9221_NUM];

/*
 * Free Run Mode:   osc = x & cntset = 0 & onest = x
 * Forced Run Mode: osc = 1 & cntset = 1 & onest = 0
 * One Shot Mode:   osc = 1 & cntset = 1 & onest = 1
*/

void my9221_init(unsigned osc, unsigned cntset, unsigned onest)
{
    for (u8 i = 0; i < MY9221_NUM; i++) {
        /* Reserved */
        g_my9221[i].temp = 0;
        /* Selection of output current reaction rate */
        g_my9221[i].hspd = 1;
        /* Gray-scale*/
        g_my9221[i].bs = 0;
        /* Built-in gray scale clock rate */
        g_my9221[i].gck = 0;
        /* Output current dispersion and non-dispersion */
        g_my9221[i].sep = 0;
        /* Gray-scale clock frequency source */
        g_my9221[i].osc = osc;
        /* Output current polarity */
        g_my9221[i].pol = 0;
        /* Automatic change screen mode or forced change screen mode */
        g_my9221[i].cntset = cntset;
        /* Repeat display or single display */
        g_my9221[i].onest = onest;
    }
}

static void my9221_internal_latch_pulse_generation(void)
{
    /* When all gray-scale data is transferred to the shift register, keep DCKI at
     * a fixed reference (either high or low) and maintain it above 220us.(Tstart > 220us)
     */
    //udelay(220);
    /* Transmit 4 DI signals.(twH (DI) > 70 ns, these (DI) > 230 ns, Tstop *)
     */
    my9221_di_high();
    my9221_delay_70ns();
    my9221_di_low();
    my9221_delay_230ns();
    my9221_di_high();
    my9221_delay_70ns();
    my9221_di_low();
    my9221_delay_230ns();
    my9221_di_high();
    my9221_delay_70ns();
    my9221_di_low();
    my9221_delay_230ns();
    my9221_di_high();
    my9221_delay_70ns();
    my9221_di_low();
    /* After the descending edge of the fourth DI signal, the Tstop* > 200ns can be
     * used to transmit new gray-scale data.
     * Note: in tandem applications, Tsop(minimum) must be greater than [200ns+N*10ns],
     * where N is the number of chips in series.
     */
    my9221_delay_600ns();;
}

void my9221_send_packet(void)
{
    my9221_port_multi_transmmit((u16 *)g_my9221, 14 * MY9221_NUM);
    my9221_internal_latch_pulse_generation();
}

#ifdef DESIGN_VERIFICATION_MY9221
#include "kinetis/test-kinetis.h"

int t_my9221_send_packet(int argc, char **argv)
{
    my9221_send_packet();

    return PASS;
}
#endif







