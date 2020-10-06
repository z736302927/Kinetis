#include "sht20/sht20.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "i2c_soft/i2c_soft.h"

extern I2C_HandleTypeDef hi2c2;

void SHT20_Init(void)
{

}

uint16_t SHT20_ReadTemp(void)
{
    uint16_t temp = 0;
    uint8_t tmpdata[3] = {0, 0, 0};
    uint8_t tmpcmd = 0xE3;

//  if(IIC_Read_nByte(SHT20_IIC_ADDR, SHT20_MEASURE_TEMP_CMD, tmpdata, 3) != 0)
//    return 0;
    HAL_I2C_Master_Transmit(&hi2c2, 0x80, &tmpcmd, 1, 100);
    HAL_I2C_Master_Receive(&hi2c2, 0x81, tmpdata, 3, 100);

    temp = tmpdata[0];
    temp <<= 8;
    temp += (tmpdata[1] & 0xFC);

    return temp;
}

uint16_t SHT20_ReadRH(void)
{
    uint16_t rh = 0;
    uint8_t tmpdata[3] = {0, 0, 0};
    uint8_t tmpcmd = 0xE5;

//  if(IIC_Read_nByte(SHT20_IIC_ADDR, SHT20_MEASURE_RH_CMD, tmpdata, 3) != 0)
//    return 0;
    HAL_I2C_Master_Transmit(&hi2c2, 0x80, &tmpcmd, 1, 100);
    HAL_I2C_Master_Receive(&hi2c2, 0x81, tmpdata, 3, 100);

    rh = tmpdata[0];
    rh <<= 8;
    rh += (tmpdata[1] & 0xF0);

    return rh;
}

void SHT20_SoftReset(void)
{
    IIC_Write_1Byte(SHT20_IIC_ADDR, SHT20_SOFT_RESET_CMD, 0);
}

float SHT20_Convert(uint16_t value, uint8_t isTemp)
{
    float tmp = 0.0;

    if (isTemp)
        tmp = -46.85 + (175.72 * value) / (1 << 16);
    else
        tmp = -6 + (125.0 * value) / (1 << 16);

    return tmp;
}

void SHT20_Read_TempAndRH(float *Temperature, float *Humidit)
{
    uint16_t temp = 0;
    uint16_t rh = 0;

    temp = SHT20_ReadTemp();
    rh = SHT20_ReadRH();

    *Temperature = SHT20_Convert(temp, 1);
    *Humidit = SHT20_Convert(rh, 0);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/
#ifdef DESIGN_VERIFICATION_SHT20
{"test", fuction},
#endif