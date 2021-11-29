/******************** (C) COPYRIGHT 2017 ANO Tech ********************************
 * 作�?   ：匿名科�?
 * 官网    ：www.anotc.com
 * 淘宝    ：anotc.taobao.com
 * 技术Q�?�?90169595
 * 描述    ：ICM20602驱动
**********************************************************************************/
#include "Drv_icm20602.h"
#include "Ano_Filter.h"
#include "Ano_Math.h"
#include "Drv_spi.h"
#include "Drv_led.h"
#include "Drv_heating.h"


void Drv_Icm20602CSPin_Init(void)
{

}

static void icm20602_enable(u8 ena)
{

}

static void icm20602_readbuf(u8 reg, u8 length, u8 *data)
{
    icm20602_enable(1);
    Drv_SPI2_RW(reg | 0x80);
    Drv_SPI2_Receive(data, length);
    icm20602_enable(0);
}

static u8 icm20602_writebyte(u8 reg, u8 data)
{
    u8 status;

    icm20602_enable(1);
    status = Drv_SPI2_RW(reg);
    Drv_SPI2_RW(data);
    icm20602_enable(0);
    return status;
}
/**************************实现函数********************************************
*功　　�?	  �?修改 �?指定设备 指定寄存器一个字�?中的1个位
reg	   寄存器地址
bitNum  要修改目标字节的bitNum�?
data  �? 时，目标位将被清0 否则将被置位
*******************************************************************************/
static void icm20602_writeBit(u8 reg, u8 bitNum, u8 data)
{
    u8 b;
    icm20602_readbuf(reg, 1, &b);
    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
    icm20602_writebyte(reg, b);
}
/**************************实现函数********************************************
*功　　�?	    �?修改 �?指定设备 指定寄存器一个字�?中的多个�?
reg	   寄存器地址
bitStart  目标字节的起始位
length   位长�?
data    存放改变目标字节位的�?
******************************************************************************
static void icm20602_writeBits(u8 reg,u8 bitStart,u8 length,u8 data)
{
    u8 b,mask;
    icm20602_readbuf(reg, 1, &b);
    mask = (0xFF << (bitStart + 1)) | 0xFF >> ((8 - bitStart) + length - 1);
    data <<= (8 - length);
    data >>= (7 - bitStart);
    b &= mask;
    b |= data;
    icm20602_writebyte(reg, b);
}*/

static void icm20602_setIntEnabled(void)
{
    icm20602_writeBit(MPUREG_INT_PIN_CFG, ICM_INTCFG_INT_LEVEL_BIT, ICM_INTMODE_ACTIVEHIGH);
    icm20602_writeBit(MPUREG_INT_PIN_CFG, ICM_INTCFG_INT_OPEN_BIT, ICM_INTDRV_PUSHPULL);
    icm20602_writeBit(MPUREG_INT_PIN_CFG, ICM_INTCFG_LATCH_INT_EN_BIT, ICM_INTLATCH_50USPULSE);  //MPU6050_INTLATCH_WAITCLEAR );
    icm20602_writeBit(MPUREG_INT_PIN_CFG, ICM_INTCFG_INT_RD_CLEAR_BIT, ICM_INTCLEAR_ANYREAD);

    icm20602_writeBit(MPUREG_INT_ENABLE, ICM_INTERRUPT_DATA_RDY_BIT, 1);
}
static void icm20602_INT_Config(void)
{
    //定义中断结构�?
    NVIC_InitTypeDef NVIC_InitStructure ;
    //定义外部中断结构�?
    EXTI_InitTypeDef EXTI_InitStructure;
    //定义IO初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//注意要打开SYSCFG时钟

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7 ;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_SetBits(GPIOD, GPIO_Pin_7);

    //配置中断�?
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource7);
    // 配置EXTI_Line1下降沿触�?
    EXTI_ClearITPendingBit(EXTI_Line7);
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    //打开中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;                //通道设置
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;    	//中断占先等级6
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           	//中断响应优先�?
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 //打开中断
    NVIC_Init(&NVIC_InitStructure);                                 //初始�?

    icm20602_setIntEnabled();
}
/**************************实现函数********************************************
*功　　�?	    初始化icm进入可用状态�?
*******************************************************************************/
u8 Drv_Icm20602Reg_Init(void)
{

    u8 tmp;

    icm20602_writebyte(MPU_RA_PWR_MGMT_1, 0x80);
    mdelay(10);
    icm20602_writebyte(MPU_RA_PWR_MGMT_1, 0x01);
    mdelay(10);

    icm20602_readbuf(MPUREG_WHOAMI, 1, &tmp);

    if (tmp != MPU_WHOAMI_20602)
        return 0;

    /*复位reg*/
    icm20602_writebyte(MPU_RA_SIGNAL_PATH_RESET, 0x03);
    mdelay(10);
    /*复位reg*/
    icm20602_writebyte(MPU_RA_USER_CTRL, 0x01);
    mdelay(10);

    icm20602_writebyte(0x70, 0x40); //dmp
    mdelay(10);
    icm20602_writebyte(MPU_RA_PWR_MGMT_2, 0x00);
    mdelay(10);
    icm20602_writebyte(MPU_RA_SMPLRT_DIV, 0);
    mdelay(10);

    icm20602_writebyte(MPU_RA_CONFIG, ICM20602_LPF_20HZ);
    mdelay(10);
    icm20602_writebyte(MPU_RA_GYRO_CONFIG, (3 << 3));
    mdelay(10);
    icm20602_writebyte(MPU_RA_ACCEL_CONFIG, (3 << 3));
    mdelay(10);
    /*加速度计LPF 20HZ*/
    icm20602_writebyte(0X1D, 0x04);
    mdelay(10);
    /*关闭低功�?/
    icm20602_writebyte(0X1E, 0x00);
    mdelay(10);
    /*关闭FIFO*/
    icm20602_writebyte(0X23, 0x00);
    mdelay(10);
    icm20602_INT_Config();


    return 1;

}



u8 mpu_buffer[14];

void Drv_Icm20602_Read()
{
    icm20602_readbuf(MPUREG_ACCEL_XOUT_H, 14, mpu_buffer);
    //
    ICM_Get_Data();
}

#include "Ano_Sensor_Basic.h"
void ICM_Get_Data()
{
    s16 temp[2][3];
    //	/*读取buffer原始数据*/
    temp[0][X] = (s16)((((u16)mpu_buffer[0]) << 8) | mpu_buffer[1]);//>>1;// + 2 *sensor.Tempreature_C;// + 5 *sensor.Tempreature_C;
    temp[0][Y] = (s16)((((u16)mpu_buffer[2]) << 8) | mpu_buffer[3]);//>>1;// + 2 *sensor.Tempreature_C;// + 5 *sensor.Tempreature_C;
    temp[0][Z] = (s16)((((u16)mpu_buffer[4]) << 8) | mpu_buffer[5]);//>>1;// + 4 *sensor.Tempreature_C;// + 7 *sensor.Tempreature_C;

    temp[1][X] = (s16)((((u16)mpu_buffer[ 8]) << 8) | mpu_buffer[ 9]) ;
    temp[1][Y] = (s16)((((u16)mpu_buffer[10]) << 8) | mpu_buffer[11]) ;
    temp[1][Z] = (s16)((((u16)mpu_buffer[12]) << 8) | mpu_buffer[13]) ;

    sensor.Tempreature = ((((int16_t)mpu_buffer[6]) << 8) | mpu_buffer[7]); //tempreature
    /*icm20602温度*/
    sensor.Tempreature_C = sensor.Tempreature / 326.8f + 25 ; //sensor.Tempreature/340.0f + 36.5f;

    //调整物理坐标轴与软件坐标轴方向定义一�?
    sensor.Acc_Original[X] = temp[0][X];
    sensor.Acc_Original[Y] = temp[0][Y];
    sensor.Acc_Original[Z] = temp[0][Z];

    sensor.Gyro_Original[X] = temp[1][X];
    sensor.Gyro_Original[Y] = temp[1][Y];
    sensor.Gyro_Original[Z] = temp[1][Z];
}
