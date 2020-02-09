#include "mpu6050/mpu6050.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "iic_soft/iic_soft.h"

#define DEBUG
#include "idebug/idebug.h"

#define mpu6050_printf                  p_dbg

#define MPU6050_ADDR                    0x00

void mpu6050_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void mpu6050_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void mpu6050_PortTransmmit(uint8_t Addr, uint8_t Data)
{
  IIC_Soft_WriteSingleByteWithAddr(MPU6050_ADDR, Addr, Data);
}

void mpu6050_PortReceive(uint8_t Addr, uint8_t *pData)
{
  IIC_Soft_ReadSingleByteWithAddr(MPU6050_ADDR, Addr, pData);
}

void mpu6050_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_WriteMultiByteWithAddr(MPU6050_ADDR, Addr, pData, Length);
}

void mpu6050_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_ReadMultiByteWithAddr(MPU6050_ADDR, Addr, pData, Length);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define X_AXIS                          0x00
#define Y_AXIS                          0x01
#define Z_AXIS                          0x02
#define I2C_SLAVE0                      0x01
#define I2C_SLAVE1                      0x02
#define I2C_SLAVE2                      0x03
#define I2C_SLAVE3                      0x04
#define I2C_SLAVE4                      0x05
#define SELF_TEST_X_GYRO                0x00
#define SELF_TEST_Y_GYRO                0x01
#define SELF_TEST_Z_GYRO                0x02
#define SELF_TEST_X_ACCEL               0x0D
#define SELF_TEST_Y_ACCEL               0x0E
#define SELF_TEST_Z_ACCEL               0x0F
#define XG_OFFSET_H                     0x13
#define XG_OFFSET_L                     0x14
#define YG_OFFSET_H                     0x15
#define YG_OFFSET_L                     0x16
#define ZG_OFFSET_H                     0x17
#define ZG_OFFSET_L                     0x18
#define SMPLRT_DIV                      0x19
#define CONFIG                          0x1A
#define GYRO_CONFIG                     0x1B
#define ACCEL_CONFIG                    0x1C
#define ACCEL_CONFIG2                   0x1D
#define LP_ACCEL_ODR                    0x1E
#define WOM_THR                         0x1F
#define FIFO_EN                         0x23
#define I2C_MST_CTRL                    0x24
#define I2C_SLV0_ADDR                   0x25
#define I2C_SLV0_REG                    0x26
#define I2C_SLV0_CTRL                   0x27
#define I2C_SLV1_ADDR                   0x28
#define I2C_SLV1_REG                    0x29
#define I2C_SLV1_CTRL                   0x2A
#define I2C_SLV2_ADDR                   0x2B
#define I2C_SLV2_REG                    0x2C
#define I2C_SLV2_CTRL                   0x2D
#define I2C_SLV3_ADDR                   0x2E
#define I2C_SLV3_REG                    0x2F
#define I2C_SLV3_CTRL                   0x30
#define I2C_SLV4_ADDR                   0x31
#define I2C_SLV4_REG                    0x32
#define I2C_SLV4_DO                     0x33
#define I2C_SLV4_CTRL                   0x34
#define I2C_SLV4_DI                     0x35
#define I2C_MST_STATUS                  0x36
#define INT_PIN_CFG                     0x37
#define INT_ENABLE                      0x38
#define INT_STATUS                      0x3A
#define ACCEL_XOUT_H                    0x3B
#define ACCEL_XOUT_L                    0x3C
#define ACCEL_YOUT_H                    0x3D
#define ACCEL_YOUT_L                    0x3E
#define ACCEL_ZOUT_H                    0x3F
#define ACCEL_ZOUT_L                    0x40
#define TEMP_OUT_H                      0x41
#define TEMP_OUT_L                      0x42
#define GYRO_XOUT_H                     0x43
#define GYRO_XOUT_L                     0x44
#define GYRO_YOUT_H                     0x45
#define GYRO_YOUT_L                     0x46
#define GYRO_ZOUT_H                     0x47
#define GYRO_ZOUT_L                     0x48
#define EXT_SENS_DATA_00                0x49
#define EXT_SENS_DATA_01                0x4A
#define EXT_SENS_DATA_02                0x4B
#define EXT_SENS_DATA_03                0x4C
#define EXT_SENS_DATA_04                0x4D
#define EXT_SENS_DATA_05                0x4E
#define EXT_SENS_DATA_06                0x4F
#define EXT_SENS_DATA_07                0x50
#define EXT_SENS_DATA_08                0x51
#define EXT_SENS_DATA_09                0x52
#define EXT_SENS_DATA_10                0x53
#define EXT_SENS_DATA_11                0x54
#define EXT_SENS_DATA_12                0x55
#define EXT_SENS_DATA_13                0x56
#define EXT_SENS_DATA_14                0x57
#define EXT_SENS_DATA_15                0x58
#define EXT_SENS_DATA_16                0x59
#define EXT_SENS_DATA_17                0x5A
#define EXT_SENS_DATA_18                0x5B
#define EXT_SENS_DATA_19                0x5C
#define EXT_SENS_DATA_20                0x5D
#define EXT_SENS_DATA_21                0x5E
#define EXT_SENS_DATA_22                0x5F
#define EXT_SENS_DATA_23                0x60
#define I2C_SLV0_DO                     0x63
#define I2C_SLV1_DO                     0x64
#define I2C_SLV2_DO                     0x65
#define I2C_SLV3_DO                     0x66
#define I2C_MST_DELAY_CTRL              0x67
#define SIGNAL_PATH_RESET               0x68
#define MOT_DETECT_CTRL                 0x69
#define USER_CTRL                       0x6A
#define PWR_MGMT_1                      0x6B
#define PWR_MGMT_2                      0x6C
#define FIFO_COUNTH                     0x72
#define FIFO_COUNTL                     0x73
#define FIFO_R_W                        0x74
#define WHO_AM_I                        0x75
#define XA_OFFSET_H                     0x77
#define XA_OFFSET_L                     0x78
#define YA_OFFSET_H                     0x7A
#define YA_OFFSET_L                     0x7B
#define ZA_OFFSET_H                     0x7D
#define ZA_OFFSET_L                     0x7E

uint8_t g_Temp_Sensitivity = 0;

void mpu6050_ReadGyroSelfTestRegisters(uint8_t Axis, uint8_t *pData)
{
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortReceive(SELF_TEST_X_GYRO, pData);
      break;
      
    case Y_AXIS:
      mpu6050_PortReceive(SELF_TEST_Y_GYRO, pData);
      break;
      
    case Z_AXIS:
      mpu6050_PortReceive(SELF_TEST_Z_GYRO, pData);
      break;
      
    default:break;
  }
}

void mpu6050_WriteGyroSelfTestRegisters(uint8_t Axis, uint8_t Data)
{
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortTransmmit(SELF_TEST_X_GYRO, Data);
      break;
      
    case Y_AXIS:
      mpu6050_PortTransmmit(SELF_TEST_Y_GYRO, Data);
      break;
      
    case Z_AXIS:
      mpu6050_PortTransmmit(SELF_TEST_Z_GYRO, Data);
      break;
      
    default:break;
  }
}

void mpu6050_ReadAccelSelfTestRegisters(uint8_t Axis, uint8_t *pData)
{
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortReceive(SELF_TEST_X_ACCEL, pData);
      break;
      
    case Y_AXIS:
      mpu6050_PortReceive(SELF_TEST_Y_ACCEL, pData);
      break;
      
    case Z_AXIS:
      mpu6050_PortReceive(SELF_TEST_Z_ACCEL, pData);
      break;
      
    default:break;
  }
}

void mpu6050_WriteAccelSelfTestRegisters(uint8_t Axis, uint8_t Data)
{
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortTransmmit(SELF_TEST_X_ACCEL, Data);
      break;
      
    case Y_AXIS:
      mpu6050_PortTransmmit(SELF_TEST_Y_ACCEL, Data);
      break;
      
    case Z_AXIS:
      mpu6050_PortTransmmit(SELF_TEST_Z_ACCEL, Data);
      break;
      
    default:break;
  }
}

void mpu6050_ReadGyroOffsetRegisters(uint8_t Axis, uint16_t *pData)
{
  uint8_t SubData1 = 0;
  uint8_t SubData2 = 0;
  
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortReceive(XG_OFFSET_H, &SubData1);
      mpu6050_PortReceive(XG_OFFSET_L, &SubData2);
      break;
      
    case Y_AXIS:
      mpu6050_PortReceive(YG_OFFSET_H, &SubData1);
      mpu6050_PortReceive(YG_OFFSET_L, &SubData2);
      break;
      
    case Z_AXIS:
      mpu6050_PortReceive(ZG_OFFSET_H, &SubData1);
      mpu6050_PortReceive(ZG_OFFSET_L, &SubData2);
      break;
      
    default:break;
  }
  
  *pData = (SubData1 << 8) | SubData2;
}

void mpu6050_WriteGyroOffsetRegisters(uint8_t Axis, uint16_t Data)
{
  uint8_t SubData1 = 0;
  uint8_t SubData2 = 0;
  
  SubData1 = Data >> 8;
  SubData2 = Data & 0xFF;
  
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortTransmmit(XG_OFFSET_H, SubData1);
      mpu6050_PortTransmmit(XG_OFFSET_L, SubData2);
      break;
      
    case Y_AXIS:
      mpu6050_PortTransmmit(YG_OFFSET_H, SubData1);
      mpu6050_PortTransmmit(YG_OFFSET_L, SubData2);
      break;
      
    case Z_AXIS:
      mpu6050_PortTransmmit(ZG_OFFSET_H, SubData1);
      mpu6050_PortTransmmit(ZG_OFFSET_L, SubData2);
      break;
      
    default:break;
  }
}

void mpu6050_SampleRateDivider(uint8_t Data)
{
  mpu6050_PortTransmmit(SMPLRT_DIV, Data);
}

void mpu6050_FIFOMode(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(CONFIG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(CONFIG, TmpReg);
}

void mpu6050_FsyncSet(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(CONFIG, &TmpReg);
  
  Data &= 0x07;
  TmpReg &= ~(0x07 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(CONFIG, TmpReg);
}

void mpu6050_ConfigDLPF(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(CONFIG, &TmpReg);
  
  Data &= 0x07;
  TmpReg &= ~(0x07 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(CONFIG, TmpReg);
}

void mpu6050_GyroSelfTest(uint8_t Axis, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(GYRO_CONFIG, &TmpReg);
  Data &= 0x01;
  
  switch(Axis)
  {
    case X_AXIS:
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      break;
      
    case Y_AXIS:
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      break;
      
    case Z_AXIS:
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      break;
      
    default:break;
  }
  
  mpu6050_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void mpu6050_GyroFullScaleSelect(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(GYRO_CONFIG, &TmpReg);
  
  Data &= 0x03;
  TmpReg &= ~(0x03 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void mpu6050_Fchoice_b(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(GYRO_CONFIG, &TmpReg);
  
  Data &= 0x03;
  TmpReg &= ~(0x03 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(GYRO_CONFIG, TmpReg);
}

void mpu6050_AccelSelfTest(uint8_t Axis, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(ACCEL_CONFIG, &TmpReg);
  Data &= 0x01;
  
  switch(Axis)
  {
    case X_AXIS:
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      break;
      
    case Y_AXIS:
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      break;
      
    case Z_AXIS:
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      break;
      
    default:break;
  }
  
  mpu6050_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void mpu6050_AccelFullScaleSelect(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(ACCEL_CONFIG, &TmpReg);
  
  Data &= 0x03;
  TmpReg &= ~(0x03 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(ACCEL_CONFIG, TmpReg);
}

void mpu6050_Accel_Fchoice_b(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(ACCEL_CONFIG2, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void mpu6050_ConfigAccelDLPF(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(ACCEL_CONFIG2, &TmpReg);
  
  Data &= 0x07;
  TmpReg &= ~(0x07 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(ACCEL_CONFIG2, TmpReg);
}

void mpu6050_LowPowerAccelODRControl(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(LP_ACCEL_ODR, &TmpReg);
  
  Data &= 0x0F;
  TmpReg &= ~(0x0F << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(LP_ACCEL_ODR, TmpReg);
}

void mpu6050_WakeonMotionThreshold(uint8_t Data)
{
  mpu6050_PortTransmmit(WOM_THR, Data);
}

void mpu6050_FIFOEnableWithTemp(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(FIFO_EN, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_FIFOEnableWithGyro(uint8_t Axis, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(FIFO_EN, &TmpReg);
  Data &= 0x01;
  
  switch(Axis)
  {
    case X_AXIS:
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      break;
      
    case Y_AXIS:
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      break;
      
    case Z_AXIS:
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      break;
      
    default:break;
  }
  
  mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_FIFOEnableWithAccel(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(FIFO_EN, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_FIFOEnableWithExtSensor(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(FIFO_EN, &TmpReg);
  Data &= 0x01;
  
  switch(Slave)
  {
    case I2C_SLAVE2:
      TmpReg &= ~(0x01 << 2);
      TmpReg |= (Data << 2);
      break;
      
    case I2C_SLAVE1:
      TmpReg &= ~(0x01 << 1);
      TmpReg |= (Data << 1);
      break;
      
    case I2C_SLAVE0:
      TmpReg &= ~(0x01 << 0);
      TmpReg |= (Data << 0);
      break;
      
    default:break;
  }
  
  mpu6050_PortTransmmit(FIFO_EN, TmpReg);
}

void mpu6050_EnableMultimaster(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_WaitForExtSensor(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_EnableSlave3FIFO(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 5);
  TmpReg |= (Data << 5);
  
  mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_I2CSignalBetweenRead(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 4);
  TmpReg |= (Data << 4);
  
  mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_I2CMasterClock(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_CTRL, &TmpReg);
  
  Data &= 0x0F;
  TmpReg &= ~(0x0F << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(I2C_MST_CTRL, TmpReg);
}

void mpu6050_I2CSlaveAddr(uint8_t Slave, uint8_t Dir, uint8_t Addr)
{
  uint8_t TmpReg = 0;
  
  TmpReg |= (Dir << 7);
  TmpReg |= (Addr << 0);
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortTransmmit(I2C_SLV0_ADDR, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortTransmmit(I2C_SLV1_ADDR, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortTransmmit(I2C_SLV2_ADDR, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortTransmmit(I2C_SLV3_ADDR, TmpReg);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortTransmmit(I2C_SLV4_ADDR, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveReg(uint8_t Slave, uint8_t Reg)
{
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortTransmmit(I2C_SLV0_REG, Reg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortTransmmit(I2C_SLV1_REG, Reg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortTransmmit(I2C_SLV2_REG, Reg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortTransmmit(I2C_SLV3_REG, Reg);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortTransmmit(I2C_SLV4_REG, Reg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveEnable(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  Data &= 0x01;
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 7);
      TmpReg |= (Data << 7);
      mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveSwapBytes(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  Data &= 0x01;
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 6);
      TmpReg |= (Data << 6);
      mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveDisReg(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  Data &= 0x01;
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveGroupType(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  Data &= 0x01;
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveNumberOfReadBytes(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  Data &= 0x0F;
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortReceive(I2C_SLV0_CTRL, &TmpReg);
      TmpReg &= ~(0x0F << 0);
      TmpReg |= (Data << 0);
      mpu6050_PortTransmmit(I2C_SLV0_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortReceive(I2C_SLV1_CTRL, &TmpReg);
      TmpReg &= ~(0x0F << 0);
      TmpReg |= (Data << 0);
      mpu6050_PortTransmmit(I2C_SLV1_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortReceive(I2C_SLV2_CTRL, &TmpReg);
      TmpReg &= ~(0x0F << 0);
      TmpReg |= (Data << 0);
      mpu6050_PortTransmmit(I2C_SLV2_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortReceive(I2C_SLV3_CTRL, &TmpReg);
      TmpReg &= ~(0x0F << 0);
      TmpReg |= (Data << 0);
      mpu6050_PortTransmmit(I2C_SLV3_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
      TmpReg &= ~(0x0F << 0);
      TmpReg |= (Data << 0);
      mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlave4DO(uint8_t Slave, uint8_t Data)
{
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortTransmmit(I2C_SLV0_DO, Data);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortTransmmit(I2C_SLV1_DO, Data);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortTransmmit(I2C_SLV2_DO, Data);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortTransmmit(I2C_SLV3_DO, Data);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortTransmmit(I2C_SLV4_DO, Data);
      break;
      
    default:break;
  }
}

void mpu6050_I2CSlaveEnableInt(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
}

void mpu6050_I2CSlaveMasterDelay(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_SLV4_CTRL, &TmpReg);
  
  Data &= 0x1F;
  TmpReg &= ~(0x1F << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(I2C_SLV4_CTRL, TmpReg);
}

void mpu6050_I2CSlave4DI(uint8_t *pData)
{
  mpu6050_PortReceive(I2C_SLV4_DI, pData);
}

uint8_t mpu6050_StatusOfFsyncInt(void)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);
  
  if(TmpReg & 0x80)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t mpu6050_Slave4TransferDone(void)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);
  
  if(TmpReg & 0x40)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t mpu6050_SlaveLoosesArbitration(void)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);
  
  if(TmpReg & 0x20)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t mpu6050_SlaveReceivesNACK(uint8_t Slave)
{
  uint8_t TmpReg = 0;
  uint8_t TmpValue = 0;
  
  mpu6050_PortReceive(I2C_MST_STATUS, &TmpReg);
  
  switch(Slave)
  {
    case I2C_SLAVE4:
      if(TmpReg & 0x10)
      {
        TmpValue = 1;
      }
      else
      {
        TmpValue = 0;
      }
      break;
      
    case I2C_SLAVE3:
      if(TmpReg & 0x08)
      {
        TmpValue = 1;
      }
      else
      {
        TmpValue = 0;
      }
      break;
      
    case I2C_SLAVE2:
      if(TmpReg & 0x04)
      {
        TmpValue = 1;
      }
      else
      {
        TmpValue = 0;
      }
      break;
      
    case I2C_SLAVE1:
      if(TmpReg & 0x02)
      {
        TmpValue = 1;
      }
      else
      {
        TmpValue = 0;
      }
      break;
      
    case I2C_SLAVE0:
      if(TmpReg & 0x01)
      {
        TmpValue = 1;
      }
      else
      {
        TmpValue = 0;
      }
      break;
      
    default:break;
  }
  
  return TmpValue;
}

void mpu6050_LogicLevelForInt(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_EnablePullup(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_LatchIntPin(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 5);
  TmpReg |= (Data << 5);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_IntAnyrd2Clear(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 4);
  TmpReg |= (Data << 4);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_LogicLevelForFsync(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_FsyncIntMode(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 2);
  TmpReg |= (Data << 2);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_BypassMode(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_PIN_CFG, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 1);
  TmpReg |= (Data << 1);
  
  mpu6050_PortTransmmit(INT_PIN_CFG, TmpReg);
}

void mpu6050_IntForWakeOnMotion(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_ENABLE, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntForFIFOOverflow(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_ENABLE, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 4);
  TmpReg |= (Data << 4);
  
  mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntForFsync(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_ENABLE, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntForRawSensorDataReady(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(INT_ENABLE, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(INT_ENABLE, TmpReg);
}

void mpu6050_IntStatus(uint8_t *pData)
{
  mpu6050_PortReceive(INT_STATUS, pData);
}

void mpu6050_AccelMeasurements(uint16_t *pData)
{
  uint8_t TmpVal[6];
  
  mpu6050_PortMultiReceive(ACCEL_XOUT_H, TmpVal, 6);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
  pData[1] = (TmpVal[2] << 8) | TmpVal[3];
  pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void mpu6050_TempMeasurement(uint16_t *pData)
{
  uint8_t TmpVal[2];
  
  mpu6050_PortMultiReceive(TEMP_OUT_H, TmpVal, 2);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void mpu6050_GetTemperature(float *pData)
{
  uint16_t TmpVal;
  
  mpu6050_TempMeasurement(&TmpVal);
  
  pData[0] = ((TmpVal - 0) / g_Temp_Sensitivity) + 21;
}

void mpu6050_GyroMeasurements(uint16_t *pData)
{
  uint8_t TmpVal[6];
  
  mpu6050_PortMultiReceive(GYRO_XOUT_H, TmpVal, 6);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
  pData[1] = (TmpVal[2] << 8) | TmpVal[3];
  pData[2] = (TmpVal[4] << 8) | TmpVal[5];
}

void mpu6050_GetAccelAndGyro(uint16_t *pData)
{
  uint8_t TmpVal[14];
  
  mpu6050_PortMultiReceive(ACCEL_XOUT_H, TmpVal, 14);
  
  pData[0] = (TmpVal[0] << 8) | TmpVal[1];
  pData[1] = (TmpVal[2] << 8) | TmpVal[3];
  pData[2] = (TmpVal[4] << 8) | TmpVal[5];
  
  pData[3] = (TmpVal[8] << 8) | TmpVal[9];
  pData[4] = (TmpVal[10] << 8) | TmpVal[11];
  pData[5] = (TmpVal[12] << 8) | TmpVal[13];
}

void mpu6050_ExternalSensorData(uint8_t Reg, uint8_t *pData)
{
  mpu6050_PortReceive(Reg, pData);
}

void mpu6050_DelaysShadowOfExtSensor(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
}

void mpu6050_I2CSlaveDelayEnable(uint8_t Slave, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  Data &= 0x01;
  
  switch(Slave)
  {
    case I2C_SLAVE0:
      mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 0);
      TmpReg |= (Data << 0);
      mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE1:
      mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 1);
      TmpReg |= (Data << 1);
      mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE2:
      mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 2);
      TmpReg |= (Data << 2);
      mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE3:
      mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 3);
      TmpReg |= (Data << 3);
      mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
      break;
      
    case I2C_SLAVE4:
      mpu6050_PortReceive(I2C_MST_DELAY_CTRL, &TmpReg);
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      mpu6050_PortTransmmit(I2C_MST_DELAY_CTRL, TmpReg);
      break;
      
    default:break;
  }
}

void mpu6050_GyroSignalPathReset(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(SIGNAL_PATH_RESET, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 2);
  TmpReg |= (Data << 2);
  
  mpu6050_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void mpu6050_AccelSignalPathReset(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(SIGNAL_PATH_RESET, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 1);
  TmpReg |= (Data << 1);
  
  mpu6050_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void mpu6050_TempSignalPathReset(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(SIGNAL_PATH_RESET, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(SIGNAL_PATH_RESET, TmpReg);
}

void mpu6050_EnableWakeonMotion(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(MOT_DETECT_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  mpu6050_PortTransmmit(MOT_DETECT_CTRL, TmpReg);
}

void mpu6050_AccelIntMode(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(MOT_DETECT_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(MOT_DETECT_CTRL, TmpReg);
}

void mpu6050_EnableFIFO(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(USER_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_EnableI2CMaster(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(USER_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 5);
  TmpReg |= (Data << 5);
  
  mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_DisableI2CSlave(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(USER_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 4);
  TmpReg |= (Data << 4);
  
  mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_ResetFIFO(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(USER_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 2);
  TmpReg |= (Data << 2);
  
  mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_ResetI2CMaster(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(USER_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 1);
  TmpReg |= (Data << 1);
  
  mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_ResetSignalPath(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(USER_CTRL, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(USER_CTRL, TmpReg);
}

void mpu6050_SoftReset(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (Data << 7);
  
  mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_EnterSleep(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (Data << 6);
  
  mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_CycleSample(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 5);
  TmpReg |= (Data << 5);
  
  mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_GyroStandby(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 4);
  TmpReg |= (Data << 4);
  
  mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_PowerDownPTAT(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);
  
  Data &= 0x01;
  TmpReg &= ~(0x01 << 3);
  TmpReg |= (Data << 3);
  
  mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_ClockSourceSelect(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_1, &TmpReg);
  
  Data &= 0x07;
  TmpReg &= ~(0x07 << 0);
  TmpReg |= (Data << 0);
  
  mpu6050_PortTransmmit(PWR_MGMT_1, TmpReg);
}

void mpu6050_AccelDisabled(uint8_t Axis, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_2, &TmpReg);
  Data &= 0x01;
  
  switch(Axis)
  {
    case X_AXIS:
      TmpReg &= ~(0x01 << 5);
      TmpReg |= (Data << 5);
      break;
      
    case Y_AXIS:
      TmpReg &= ~(0x01 << 4);
      TmpReg |= (Data << 4);
      break;
      
    case Z_AXIS:
      TmpReg &= ~(0x01 << 3);
      TmpReg |= (Data << 3);
      break;
      
    default:break;
  }
  
  mpu6050_PortTransmmit(PWR_MGMT_2, TmpReg);
}

void mpu6050_GyroDisabled(uint8_t Axis, uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  mpu6050_PortReceive(PWR_MGMT_2, &TmpReg);
  Data &= 0x01;
  
  switch(Axis)
  {
    case X_AXIS:
      TmpReg &= ~(0x01 << 2);
      TmpReg |= (Data << 2);
      break;
      
    case Y_AXIS:
      TmpReg &= ~(0x01 << 1);
      TmpReg |= (Data << 1);
      break;
      
    case Z_AXIS:
      TmpReg &= ~(0x01 << 0);
      TmpReg |= (Data << 0);
      break;
      
    default:break;
  }
  
  mpu6050_PortTransmmit(PWR_MGMT_2, TmpReg);
}

void mpu6050_FIFOCount(uint16_t *pData)
{
  uint8_t TmpReg[2];
  
  mpu6050_PortMultiReceive(FIFO_COUNTH, TmpReg, 2);
  
  TmpReg[0] &= 0x1F;
  pData[0] = (TmpReg[0] << 8) | TmpReg[1];
}

void mpu6050_FIFORead(uint8_t *pData)
{
  mpu6050_PortReceive(FIFO_R_W, pData);
}

void mpu6050_FIFOWrite(uint8_t Data)
{
  mpu6050_PortTransmmit(FIFO_R_W, Data);
}

void mpu6050_WhoAmI(uint8_t *pData)
{
  mpu6050_PortReceive(WHO_AM_I, pData);
}

void mpu9250_ReadAccelOffsetRegisters(uint8_t Axis, uint16_t *pData)
{
  uint8_t SubData1 = 0;
  uint8_t SubData2 = 0;
  
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortReceive(XA_OFFSET_H, &SubData1);
      mpu6050_PortReceive(XA_OFFSET_L, &SubData2);
      break;
      
    case Y_AXIS:
      mpu6050_PortReceive(YA_OFFSET_H, &SubData1);
      mpu6050_PortReceive(YA_OFFSET_L, &SubData2);
      break;
      
    case Z_AXIS:
      mpu6050_PortReceive(ZA_OFFSET_H, &SubData1);
      mpu6050_PortReceive(ZA_OFFSET_L, &SubData2);
      break;
      
    default:break;
  }
  
  *pData = (SubData1 << 7) | (SubData2 >> 1);
}

void mpu9250_WriteAccelOffsetRegisters(uint8_t Axis, uint16_t Data)
{
  uint8_t SubData1 = 0;
  uint8_t SubData2 = 0;
  
  SubData1 = Data >> 7;
  SubData2 = (Data & 0xFE) >> 1;
  
  switch(Axis)
  {
    case X_AXIS:
      mpu6050_PortTransmmit(XA_OFFSET_H, SubData1);
      mpu6050_PortTransmmit(XA_OFFSET_L, SubData2);
      break;
      
    case Y_AXIS:
      mpu6050_PortTransmmit(YA_OFFSET_H, SubData1);
      mpu6050_PortTransmmit(YA_OFFSET_L, SubData2);
      break;
      
    case Z_AXIS:
      mpu6050_PortTransmmit(ZA_OFFSET_H, SubData1);
      mpu6050_PortTransmmit(ZA_OFFSET_L, SubData2);
      break;
      
    default:break;
  }
}

#if 0

static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void mpu6050_Test(void)
{
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0xFF;
  mpu6050_printf("BufferLength = %d.", BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    mpu6050_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, BufferLength);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  TestAddr = TmpRngdata & 0xFF;
  mpu6050_printf("TestAddr = 0x%02X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
  }
  
  mpu6050_WriteData(TestAddr, Tx_Buffer, BufferLength);
  mpu6050_ReadData(TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      mpu6050_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      mpu6050_printf("Data writes and reads do not match, TEST FAILED !");
      return ;
    }
  }
  
  mpu6050_printf("mpu6050 Read and write TEST PASSED !");
}

#endif


