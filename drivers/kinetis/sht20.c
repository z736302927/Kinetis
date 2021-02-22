#include "kinetis/sht20.h"
#include "kinetis/iic_soft.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define SHT20_ADDR                    0x40

static inline void sht20_port_transmmit(u8 addr, u8 tmp)
{
    iic_port_transmmit(IIC_SW_1, SHT20_ADDR, addr, tmp);
}

static inline void sht20_port_receive(u8 addr, u8 *pdata)
{
    iic_port_receive(IIC_SW_1, SHT20_ADDR, addr, pdata);
}

static inline void sht20_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_transmmit(IIC_SW_1, SHT20_ADDR, addr, pdata, length);
}

static inline void sht20_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_receive(IIC_SW_1, SHT20_ADDR, addr, pdata, length);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

//#define SHT20_MEASURE_TEMP_CMD      0xE3
//#define SHT20_MEASURE_RH_CMD        0xE5
//#define SHT20_SOFT_RESET_CMD        0xFE

//void sht20_init(void)
//{

//}

//u16 sht20_read_temperature(void)
//{
//    u16 temperature = 0;
//    u8 tmp[3] = {0, 0, 0};

//    sht20_port_transmmit(SHT20_MEASURE_TEMP_CMD, );
//    sht20_port_multi_receive(, tmp, 3);

//    temperature = tmp[0];
//    temperature <<= 8;
//    temperature += (tmp[1] & 0xFC);

//    return temperature;
//}

//u16 sht20_read_rh(void)
//{
//    u16 rh = 0;
//    u8 tmp[3] = {0, 0, 0};

//    sht20_port_transmmit(SHT20_MEASURE_RH_CMD, );
//    sht20_port_multi_receive(, tmp, 3);

//    rh = tmp[0];
//    rh <<= 8;
//    rh += (tmp[1] & 0xF0);

//    return rh;
//}

//void sht20_soft_reset(void)
//{
//    sht20_port_transmmit(SHT20_SOFT_RESET_CMD, );
//}

//static float sht20_convert(u16 value, u8 isTemp)
//{
//    float tmp = 0.0;

//    if (isTemp)
//        tmp = -46.85 + (175.72 * value) / (1 << 16);
//    else
//        tmp = -6 + (125.0 * value) / (1 << 16);

//    return tmp;
//}

//void sht20_read_temp_and_rh(float *temperature, float *humidit)
//{
//    u16 temp = 0;
//    u16 rh = 0;

//    temp = sht20_read_temperature();
//    rh = sht20_read_rh();

//    *temperature = sht20_convert(temp, 1);
//    *humidit = sht20_convert(rh, 0);
//}

#ifdef DESIGN_VERIFICATION_SHT20

#endif