/******************** (C) COPYRIGHT 2017 ANO Tech ********************************
 * 作�?   ：匿名科�?
 * 官网    ：www.anotc.com
 * 淘宝    ：anotc.taobao.com
 * 技术Q�?�?90169595
 * 描述    ：飞控初始化
**********************************************************************************/
#include "include.h"
#include "Drv_pwm_out.h"
#include "Drv_led.h"
#include "Drv_spi.h"
#include "Drv_icm20602.h"
#include "drv_ak8975.h"
#include "drv_ak09915.h"
#include "drv_spl06.h"
#include "Drv_w25qxx.h"
#include "Drv_i2c_soft.h"
#include "Drv_laser.h"
#include "Ano_FlightCtrl.h"
#include "Drv_adc.h"
#include "Drv_heating.h"
#include "Ano_RC.h"
#include "Ano_Sensor_Basic.h"
#include "Drv_UP_Flow.h"
#include "Ano_DT.h"
#include "Drv_bmi088.h"
#include "Ano_Imu_Data.h"
#include "Drv_BSP.h"

struct ano_tc_fmu {
	struct fmu_para para;
	struct rc_input_state rc;
	struct fmu_sensor_state sensor_state;
	struct fc_state fc_stv;
	
	//�ǶȻ����Ʋ���
	_PID_arg_st arg_2[VEC_RPY];
	
	//���ٶȻ����Ʋ���
	_PID_arg_st arg_1[VEC_RPY];
	
	
	//�ǶȻ���������
	_PID_val_st val_2[VEC_RPY];
	
	//���ٶȻ���������
	_PID_val_st val_1[VEC_RPY];

	u8 mag_type;
#define MAG_NULL		0
#define MAG_AK09915		1
#define MAG_AK8975		2

};

struct ano_tc_fmu *kinetis_fmu;


void CheckSenser(void)
{
    if (fmu_ak09915_check())
        kinetis_fmu->mag_type = MAG_AK09915;
    else if (fmu_ak8975_check())
        kinetis_fmu->mag_type = MAG_AK8975;
    else
        kinetis_fmu->mag_type = MAG_NUL;
}

u8 of_init_type;
u8 All_Init()
{
    NVIC_PriorityGroupConfig(NVIC_GROUP);		

    SysTick_Configuration(); 		

    mdelay(100);					
    Drv_LED_Init();					

    Flash_Init();             		

    para_read();              		

    rc_module_init();

    pwm_out_init();					
    mdelay(50);					

    Drv_SPI2_init();          		
    Drv_AK8975CSPin_Init();   		
    Drv_SPL06CSPin_Init();    		
    DrvGpioSenser088CsPinInit();
    mdelay(10);	
	
    CheckSenser();
    kinetis_fmu->gyro_ok = kinetis_fmu->acc_ok = DrvBmi088Init();

    if (kinetis_fmu->mag_type == MAG_NUL) {
        kinetis_fmu->mag_ok = 0;
        LED_STA.errMag = 1;
    } else {
        kinetis_fmu->mag_ok = 1;       //标记罗盘OK

        if (kinetis_fmu->mag_type == MAG_AK09915)
            DrvAk09915Init();
    }

    kinetis_fmu->baro_ok = Drv_Spl0601_Init();       		//气压计初始化

    Usb_Hid_Init();					//飞控usb接口的hid初始�?
    mdelay(100);					//延时

    Usart2_Init(500000);			//串口2初始化，函数参数为波特率
    mdelay(10);					//延时
//	Uart4_Init(115200);				//首先判断是否连接的是激光模�?
//	if(!Drv_Laser_Init())			//激光没有有效连接，则配置为光流模式
//		Uart4_Init(500000);
//	mdelay(10);					//延时
//	Usart3_Init(500000);			//连接UWB
//	mdelay(10);					//延时
    //
    Usart3_Init(500000);			//连接OPENMV
    //
//	Uart4_Init(19200);	//接优像光�?
//  Uart5_Init(115200);//接大功率激�?
    Uart5_Init(500000); //数传
//	MyDelayMs(200);
    //优像光流初始�?
//	of_init_type = (Drv_OFInit()==0)?0:2;
//	if(of_init_type==2)//优像光流初始化成�?
//	{
//		//大功率激光初始化
//		Drv_Laser_Init();
//	}
//	else if(of_init_type==0)//优像光流初始化失�?
//	{
    Uart4_Init(500000);	//接匿名光�?
//	}
    //

    Drv_AdcInit();
    mdelay(100);					//延时

    fmu_pid_init();               		//PID初始�?

    mdelay(100);					//延时
    Drv_GpsPin_Init();				//GPS初始�?串口1

    Drv_HeatingInit();
    //
    Sensor_Basic_Init();
    //
    ANO_DT_Init();
    //传感器灵敏度初始�?
    ImuSensitivityInit(Ano_Parame.set.acc_calibrated, (float *)Ano_Parame.set.acc_sensitivity_ref); //test_st.test);//
    //
    AnoDTSendStr(USE_HID | USE_U2, SWJ_ADDR, LOG_COLOR_GREEN, "SYS init OK!");
    return (1);
}

void DrvGpioCsPinCtrlBmi088Acc(u8 ena)
{
    if (ena)
        GPIO_ResetBits(BMI088_CSPOT_ACC, BMI088_CSPIN_ACC);
    else
        GPIO_SetBits(BMI088_CSPOT_ACC, BMI088_CSPIN_ACC);
}
void DrvGpioCsPinCtrlBmi088Gyr(u8 ena)
{
    if (ena)
        GPIO_ResetBits(BMI088_CSPOT_GYR, BMI088_CSPIN_GYR);
    else
        GPIO_SetBits(BMI088_CSPOT_GYR, BMI088_CSPIN_GYR);
}
/******************* (C) COPYRIGHT 2014 ANO TECH *****END OF FILE************/
