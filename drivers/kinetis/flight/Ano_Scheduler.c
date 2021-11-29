/******************** (C) COPYRIGHT 2017 ANO Tech ********************************
 * 作�?   ：匿名科�?
 * 官网    ：www.anotc.com
 * 淘宝    ：anotc.taobao.com
 * 技术Q�?�?90169595
 * 描述    ：任务调�?
**********************************************************************************/
#include "Ano_Scheduler.h"
#include "include.h"
#include "Ano_RC.h"
#include "Ano_Parameter.h"
#include "Drv_time.h"
#include "Drv_led.h"
#include "Drv_icm20602.h"
#include "Drv_ak8975.h"
#include "Drv_spl06.h"
#include "Ano_FlightCtrl.h"
#include "Ano_FlightDataCal.h"
#include "Ano_AttCtrl.h"
#include "Ano_Imu.h"
#include "Drv_laser.h"
#include "Ano_LocCtrl.h"
#include "Ano_AltCtrl.h"
#include "Ano_MotorCtrl.h"
#include "Ano_Parameter.h"
#include "Ano_MagProcess.h"
#include "Ano_Power.h"
#include "Ano_OF.h"
#include "Drv_heating.h"
#include "Ano_FlyCtrl.h"
#include "Ano_UWB.h"
#include "Ano_Sensor_Basic.h"
#include "Drv_OpenMV.h"
#include "Ano_OPMV_CBTracking_Ctrl.h"
#include "Ano_OPMV_LineTracking_Ctrl.h"
#include "Ano_OPMV_Ctrl.h"
#include "Ano_OF_DecoFusion.h"
#include "Ano_Imu_Task.h"
#include "Drv_BSP.h"

u32 test_dT_1000hz[3], test_rT[6];

static void Loop_1000Hz(void)	//1ms执行一�?
{
    test_dT_1000hz[0] = test_dT_1000hz[1];
    test_rT[3] = test_dT_1000hz[1] = ktime_to_us(ktime_get());
    test_dT_1000hz[2] = (u32)(test_dT_1000hz[1] - test_dT_1000hz[0]) ;
//////////////////////////////////////////////////////////////////////
    /*传感器数据读�?/
    Fc_Sensor_Get();

    /*惯性传感器数据准备*/
    Sensor_Data_Prepare(1);

    /*姿态解算更�?/
    IMU_Update_Task(1);

    /*获取WC_Z加速度*/
    WCZ_Acc_Get_Task();
    WCXY_Acc_Get_Task();

    /*飞行状态任�?/
    Flight_State_Task(1, CH_N);

    /*开关状态任�?/
    Swtich_State_Task(1);

    /*光流融合数据准备任务*/
    ANO_OF_Data_Prepare_Task(0.001f);


    /*数传数据交换*/
    ANO_DT_Task1Ms();

//////////////////////////////////////////////////////////////////////
    test_rT[4] = ktime_to_us(ktime_get());
    test_rT[5] = (u32)(test_rT[4] - test_rT[3]) ;
}

static void Loop_500Hz(void)	//2ms执行一�?
{
    /*姿态角速度环控�?/
    Att_1level_Ctrl(2 * 1e-3f);

    /*电机输出控制*/
    Motor_Ctrl_Task(2);

    /*UWB数据获取*/
    Ano_UWB_Get_Data_Task(2);
}

static void Loop_200Hz(void)	//5ms执行一�?
{
    /*获取姿态角（ROLL PITCH YAW�?/
    calculate_RPY();

    /*姿态角度环控制*/
    Att_2level_Ctrl(5e-3f, CH_N);

    /*处理遥控数据*/
    rc_input_task(0.005f);

    /*高度数据融合任务*/
    WCZ_Fus_Task(5);
}

static void Loop_100Hz(void)	//10ms执行一�?
{
    test_rT[0] = ktime_to_us(ktime_get());
//////////////////////////////////////////////////////////////////////
    /*遥控器数据处�?/
    RC_duty_task(10);

    /*飞行模式设置任务*/
    Flight_Mode_Set(10);

    //
    GPS_Data_Processing_Task(10);

    /*高度速度环控�?/
    Alt_1level_Ctrl(10e-3f);

    /*高度环控�?/
    Alt_2level_Ctrl(10e-3f);

    /*匿名光流状态检�?/
    AnoOF_Check_State(0.01f);//AnoOF_DataAnl_Task(10);

    /*灯光控制*/
    LED_Task2(10);
//////////////////////////////////////////////////////////////////////
    test_rT[1] = ktime_to_us(ktime_get());
    test_rT[2] = (u32)(test_rT[1] - test_rT[0]) ;

}

static void Loop_50Hz(void)	//20ms执行一�?
{

    //
    ImuServices_20ms_c();
    /*罗盘数据处理任务*/
    Mag_Update_Task(20);
    /*程序指令控制*/
    FlyCtrl_Task(20);
    //
    ANO_OFDF_Task(20);
//	/*UWB数据计算*/
//	Ano_UWB_Data_Calcu_Task(20);
    /*位置速度环控�?/
    Loc_1level_Ctrl(20, CH_N);
    /*OPMV检测是否掉�?/
    OpenMV_Offline_Check(20);
    /*OPMV色块追踪数据处理任务*/
    ANO_CBTracking_Task(20);
    /*OPMV寻线数据处理任务*/
    ANO_LTracking_Task(20);
    /*OPMV控制任务*/
    ANO_OPMV_Ctrl_Task(20);
}

static void Loop_20Hz(void)	//50ms执行一�?
{
    /*电压相关任务*/
    Power_UpdateTask(50);
    //恒温控制
    Thermostatic_Ctrl_Task(50);
}

static void Loop_2Hz(void)	//500ms执行一�?
{
    /*延时存储任务*/
    fmu_para_write_task(500);
}
//系统任务配置，创建不同执行频率的“线程�?
static sched_task_t sched_tasks[] = {
    {Loop_1000Hz, 1000,  0, 0},
    {Loop_500Hz, 500,  0, 0},
    {Loop_200Hz, 200,  0, 0},
    {Loop_100Hz, 100,  0, 0},
    {Loop_50Hz,  50,  0, 0},
    {Loop_20Hz,  20,  0, 0},
    {Loop_2Hz,   2,  0, 0},
};
//根据数组长度，判断线程数�?
#define TASK_NUM (sizeof(sched_tasks)/sizeof(sched_task_t))

void Scheduler_Setup(void)
{
    uint8_t index = 0;

    //初始化任务表
    for (index = 0; index < TASK_NUM; index++) {
        //计算每个任务的延时周期数
        sched_tasks[index].interval_ticks = TICK_PER_SECOND / sched_tasks[index].rate_hz;

        //最短周期为1，也就是1ms
        if (sched_tasks[index].interval_ticks < 1)
            sched_tasks[index].interval_ticks = 1;
    }
}
//这个函数放到main函数的while(1)中，不停判断是否有线程应该执�?
void Scheduler_Run(void)
{
    uint8_t index = 0;
    //循环判断所有线程，是否应该执行


    for (index = 0; index < TASK_NUM; index++) {
        //获取系统当前时间，单位MS
        uint32_t tnow = SysTick_GetTick();

        //进行判断，如果当前时间减去上一次执行的时间，大于等于该线程的执行周期，则执行线�?
        if (tnow - sched_tasks[index].last_run >= sched_tasks[index].interval_ticks) {

            //更新线程的执行时间，用于下一次判�?
            sched_tasks[index].last_run = tnow;
            //执行线程函数，使用的是函数指�?
            sched_tasks[index].task_func();

        }
    }


}



/******************* (C) COPYRIGHT 2014 ANO TECH *****END OF FILE************/


