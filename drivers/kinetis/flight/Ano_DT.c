/******************** (C) COPYRIGHT 2017 ANO Tech ********************************
 * 作�?   ：匿名科�?
 * 官网    ：www.anotc.com
 * 淘宝    ：anotc.taobao.com
 * 技术Q�?�?90169595
 * 描述    ：数据传�?
**********************************************************************************/
/*============================================================================
更新�?
20201119：更新V7版协�?
===========================================================================*/

#include "Ano_DT.h"
#include "include.h"
#include "Ano_USB.h"
#include "Drv_usart.h"
#include "Ano_Parameter.h"
#include "Ano_Imu.h"
#include "Ano_Imu_Data.h"
#include "Ano_Imu_Calibration.h"
#include "Ano_MagProcess.h"
#include "Ano_Power.h"
#include "Ano_MotionCal.h"
#include "Ano_FlightCtrl.h"
#include "Ano_RC.h"
#include "Ano_LocCtrl.h"
#include "Drv_spl06.h"
/*============================================================================
******************************************************************************
******************************************************************************
基础定义部分，定义数据传输使用到的基本元素，用户使用及二次开发无需修改本部分�?
******************************************************************************
******************************************************************************
============================================================================*/
#define BYTE0(dwTemp)       ( *( (char *)(&dwTemp)		) )
#define BYTE1(dwTemp)       ( *( (char *)(&dwTemp) + 1) )
#define BYTE2(dwTemp)       ( *( (char *)(&dwTemp) + 2) )
#define BYTE3(dwTemp)       ( *( (char *)(&dwTemp) + 3) )

#define DT_RX_BUFNUM  64
#define DT_ODNUM	5	//非循环发送数据缓冲大�?
#define DT_SENDPTR_HID	Usb_Hid_Adddata
#define DT_SENDPTR_U2	Uart5_Send

//越往前发送优先级越高，如果需要修改，这里和h文件里的枚举需要同时改
const u8  _cs_idlist[CSID_NUM]	 	= {0x20, 0x21, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0f, 0x30, 0x32, 0x33, 0x34, 0x40, 0x41, 0xFA};
//循环发送数据结构体
typedef struct {
    u8 WTS;		 //wait to send等待发送标�?
    u16 fre_ms; //发送周�?
    u16 time_cnt_ms; //计时变量
} _dt_frame_st;
typedef struct {
    _dt_frame_st txSet_u1[CSID_NUM];
    _dt_frame_st txSet_u2[CSID_NUM];
    _dt_frame_st txSet_usb[CSID_NUM];
} _dt_st;
_dt_st dt;
u8 CycleSendData[64];		//循环发送数据临时缓�?
//非循环发送数据结构体
typedef struct {
    u8 WTS;
    u8 type;
    u8 len;
    u8 data[64];
} _dt_otherdata_st;
_dt_otherdata_st OtherSendData[DT_ODNUM];
u8 otherDataTmp[64];	//非循环发送数据临时缓�?

/*============================================================================
******************************************************************************
******************************************************************************
数据接口，本部分通过宏定义将各种需要收发的数据进行定义，普通用户只需修改本部�?
定义即可实现不同数据的收发功�?
******************************************************************************
******************************************************************************
============================================================================*/
//0x01
#define ACC_RAW_X      (st_imuData.f_acc_cmpss_nb[0])
#define ACC_RAW_Y      (st_imuData.f_acc_cmpss_nb[1])
#define ACC_RAW_Z      (st_imuData.f_acc_cmpss_nb[2])
#define GYR_RAW_X      (st_imuData.f_gyr_dps_nb[0])
#define GYR_RAW_Y      (st_imuData.f_gyr_dps_nb[1])
#define GYR_RAW_Z      (st_imuData.f_gyr_dps_nb[2])
#define SHOCK_STA      (st_imuData.data_sta)
//0x02
#define ECP_RAW_X      (mag.val[0])
#define ECP_RAW_Y      (mag.val[1])
#define ECP_RAW_Z      (mag.val[2])
#define BARO_ALT       (spl_data.height)
#define TEMPERATURE    (st_imuData.f_temperature)
#define BARO_STA       (0)
#define ECP_STA        (0)
//0x03
#define ANGLE_ROL      (imu_data.rol)
#define ANGLE_PIT      (imu_data.pit)
#define ANGLE_YAW      (imu_data.yaw)
#define ATT_FUSION_STA (0)
//0x04
//#define QUA0_10K       (imu_att.qua.w *1e4f)
//#define QUA1_10K       (imu_att.qua.x *1e4f)
//#define QUA2_10K       (imu_att.qua.y *1e4f)
//#define QUA3_10K       (imu_att.qua.z *1e4f)
//0X05
#define ALT_FU	       (wcz_hei_fus.out)
#define ALT_ADD	       (jsdata.of_alt)
#define ALT_STA        (0)
//0X06
#define FC_MODE	       (flag.flight_mode)
#define FC_LOCKED	   (flag.unlock_sta)
#define FC_CMD_ID	   (0)
#define FC_CMD_0       (0)
#define FC_CMD_1       (0)
//0X07
#define HCA_VEL_X      (loc_ctrl_1.fb[0])
#define HCA_VEL_Y      (loc_ctrl_1.fb[1])
#define HCA_VEL_Z      (wcz_spe_fus.out)
////0x08
#define ULHCA_POS_X      (0)
#define ULHCA_POS_Y      (0)
////0X09
//#define HCA_WIND_X      (nowind_est.wind_vel_h[X])
//#define HCA_WIND_Y      (nowind_est.wind_vel_h[Y])
////0X0A
//#define TAR_ROL         (0)
//#define TAR_PIT         (0)
//#define TAR_YAW         (0)
//0X0B
#define HCA_TAR_VEL_X   (fc_in.tar_vel_cmps_h[X])
#define HCA_TAR_VEL_Y   (fc_in.tar_vel_cmps_h[Y])
#define HCA_TAR_VEL_Z   (fc_in.tar_vel_cmps_h[Z])
//0X0D
#define BAT_VOLTAGE_100 (Plane_Votage*100)
////0x20
//#define PWM_1          (mt_ct_val.moto_pwm_u16[0])
//#define PWM_2          (mt_ct_val.moto_pwm_u16[1])
//#define PWM_3          (mt_ct_val.moto_pwm_u16[2])
//#define PWM_4          (mt_ct_val.moto_pwm_u16[3])
//#define PWM_5          (0)//
//#define PWM_6          (0)
//#define PWM_7          (0)
//#define PWM_8          (0)
////0x21
//#define CTRL_ROL	(ctrl_val.rol_ctrl_val)
//#define CTRL_PIT	(ctrl_val.pit_ctrl_val)
//#define CTRL_THR	(ctrl_val.thr_ctrl_val)
//#define CTRL_YAW	(ctrl_val.yaw_ctrl_val)

/*======================================================================================================================
//数据发送初始化
======================================================================================================================*/
void ANO_DT_Init(void)
{
    //串口1发送配�?////////////////////////////////////////////////////////////
//	//PWM
//	dt.txSet_u2[CSID_X20].fre_ms = 0;  //外部触发
    //ACC-GRO
    dt.txSet_u2[CSID_X01].fre_ms = 10; //10ms
    //ECP-TEM-BARO
    dt.txSet_u2[CSID_X02].fre_ms = 20; //20ms
    //ATT_ANG
    dt.txSet_u2[CSID_X03].fre_ms = 10; //ms
    //ATT_QUA
    dt.txSet_u2[CSID_X04].fre_ms = 0; //
    //height
    dt.txSet_u2[CSID_X05].fre_ms = 50; //
    //fc_mode
    dt.txSet_u2[CSID_X06].fre_ms = 50;//
    //velocity
    dt.txSet_u2[CSID_X07].fre_ms = 50;//
    //pos
    dt.txSet_u2[CSID_X08].fre_ms = 50;//
//	//wind_vel
//	dt.txSet_u2[CSID_X09].fre_ms = 100;//
    //电压
    dt.txSet_u2[CSID_X0D].fre_ms = 200;//
    //传感器状�?
    dt.txSet_u2[CSID_X0E].fre_ms = 200;//
    //GPS数据
    dt.txSet_u2[CSID_X30].fre_ms = 200;//
    //遥控数据
    dt.txSet_u2[CSID_X40].fre_ms = 100;//
//	//实时控制数据
//	dt.txSet_u2[CSID_X41].fre_ms = 50;//
//	//FC_RGB
//	dt.txSet_u2[CSID_X0F].fre_ms = 0;//
    //
#ifdef DEBUG
    //
//	dt.txSet_usb[CSID_X40].fre_ms = 1;
    dt.txSet_u2[CSID_XFA].fre_ms = 2;
#endif

    //USB发送配�?//////////////////////////////////////////////////////////////
    for (u8 i = 0; i < CSID_NUM; i++) {
        dt.txSet_usb[i].fre_ms = dt.txSet_u2[i].fre_ms;
        dt.txSet_usb[i].time_cnt_ms = dt.txSet_u2[i].time_cnt_ms;
    }

}






/*============================================================================
******************************************************************************
******************************************************************************
数据发送相关函�?
******************************************************************************
******************************************************************************
============================================================================*/
//===========================================================
//此函数是协议中所有发送数据功能使用到的发送函�?
//移植时，用户应根据自身应用的情况，根据使用的通信方式，实现此函数
//===========================================================
void ANO_DT_Send_Data(u8 type, u8 *dataToSend, u8 length)
{
    if ((type & USE_HID)) // && fc_sta.unlock_sta == 0)
        DT_SENDPTR_HID(dataToSend, length);

    if (type & USE_U2)
        DT_SENDPTR_U2(dataToSend, length);
}
//===========================================================
//根据ID填充数据帧的data区域
//===========================================================
static void DTFrameAddData(u8 frame_num, u8 *_cnt)
{
    s16 temp_data;
    s32 temp_data_32;

    switch (frame_num) {
    case 0x01: {
        //ACC GYR
        temp_data = (s16)(ACC_RAW_X);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(ACC_RAW_Y);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(ACC_RAW_Z);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(GYR_RAW_X);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(GYR_RAW_Y);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(GYR_RAW_Z);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        //STATE
        CycleSendData[(*_cnt)++] = SHOCK_STA;
    }
    break;

    case 0x02: {
        //ECP
        temp_data = (s16)(ECP_RAW_X);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(ECP_RAW_Y);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(ECP_RAW_Z);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        //BARO_ALT
        temp_data_32 = (s32)(BARO_ALT);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE2(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE3(temp_data_32);
        //temperature
        temp_data = (s16)(TEMPERATURE * 1e2f);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        //BARO_STA
        CycleSendData[(*_cnt)++] = BARO_STA;
        //ECP_STA
        CycleSendData[(*_cnt)++] = ECP_STA;
    }
    break;

    case 0x03: {
        //ATT_angle
        temp_data = (s16)(ANGLE_ROL * 100);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(ANGLE_PIT * 100);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(ANGLE_YAW * 100);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        //STATE
        CycleSendData[(*_cnt)++] = ATT_FUSION_STA;
    }
    break;

    case 0x04: {

    }
    break;

    case 0x05: {
        //HEIGHT
        temp_data_32 = (s32)(ALT_FU);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE2(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE3(temp_data_32);
        temp_data_32 = (s32)(ALT_ADD);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE2(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE3(temp_data_32);
        //
        CycleSendData[(*_cnt)++] = ALT_STA;
    }
    break;

    case 0x06: {
        //MODE	LOCKED	FUN	CMD
        CycleSendData[(*_cnt)++] = FC_MODE;
        CycleSendData[(*_cnt)++] = FC_LOCKED;
        CycleSendData[(*_cnt)++] = FC_CMD_ID;
        CycleSendData[(*_cnt)++] = FC_CMD_0;
        CycleSendData[(*_cnt)++] = FC_CMD_1;
    }
    break;

    case 0x07: {
        //velocity
        temp_data = (s16)(HCA_VEL_X);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(HCA_VEL_Y);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        temp_data = (s16)(HCA_VEL_Z);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
    }
    break;

    case 0x08: {
        //pos
        temp_data_32 = (s32)(ULHCA_POS_X);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE2(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE3(temp_data_32);
        temp_data_32 = (s32)(ULHCA_POS_Y);
        CycleSendData[(*_cnt)++] = BYTE0(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE2(temp_data_32);
        CycleSendData[(*_cnt)++] = BYTE3(temp_data_32);
    }
    break;

    case 0x09: {

    }
    break;

    case 0x0D: {
        //电压
        temp_data = (s16)BAT_VOLTAGE_100;
        CycleSendData[(*_cnt)++] = BYTE0(temp_data);
        CycleSendData[(*_cnt)++] = BYTE1(temp_data);
        CycleSendData[(*_cnt)++] = 0;
        CycleSendData[(*_cnt)++] = 0;
    }
    break;

    case 0x0E: {
        //传感器状�?
        CycleSendData[(*_cnt)++] = switchs.of_flow_on;
        CycleSendData[(*_cnt)++] = 0;
        CycleSendData[(*_cnt)++] = switchs.gps_on;
        CycleSendData[(*_cnt)++] = switchs.of_tof_on;
    }
    break;

    case 0x30: {
        //
        for (u8 i = 0; i < 23; i++)
            CycleSendData[(*_cnt)++] = ext_sens.fc_gps.byte[i];
    }
    break;

    case 0x40: {
        for (u8 i = 0; i < 20; i++)
            CycleSendData[(*_cnt)++] = rc_in.rc_ch.byte_data[i];
    }
    break;

    case 0x41: {

    }
    break;

    case 0x0f: { //FC_RGB

    }
    break;

    case 0x20: {

    }
    break;

    case 0xfa: {

    }
    break;

    default :
        break;
    }
}

//===========================================================
//根据ID组织发送数据帧
//===========================================================
static void DTFrameSend(u8 type, u8 id_addr, u8 addr)
{
    u8 _cnt = 0;
    u8 _id;
    vs16 _temp;

    CycleSendData[_cnt++] = FRAME_HEAD;
    CycleSendData[_cnt++] = addr;

    if (id_addr >= CSID_NUM)
        return;

    _id = _cs_idlist[id_addr];
    CycleSendData[_cnt++] = _id;
    CycleSendData[_cnt++] = 0;
    //==
    DTFrameAddData(_id, &_cnt);
    //==
    CycleSendData[3] = _cnt - 4;
    //==
    u8 check_sum1 = 0, check_sum2 = 0;

    for (u8 i = 0; i < _cnt; i++) {
        check_sum1 += CycleSendData[i];
        check_sum2 += check_sum1;
    }

    CycleSendData[_cnt++] = check_sum1;
    CycleSendData[_cnt++] = check_sum2;

    ANO_DT_Send_Data(type, CycleSendData, _cnt);
}

//===========================================================
//非循环发送数据的写入缓冲�?
//===========================================================
static u8 OtherSendDataAdd(u8 _type, u8 *_data, u8 _len)
{
    for (u8 i = 0; i < DT_ODNUM; i++) {
        if (OtherSendData[i].WTS == 0) {
            OtherSendData[i].type = _type;
            OtherSendData[i].len = _len;

            for (u8 j = 0; j < _len; j++)
                OtherSendData[i].data[j] = *(_data + j);

            OtherSendData[i].WTS = 1;
            return 1;
        }
    }

    return 0;
}
//===========================================================
//判断是否有需要发送的非循环数据，如有，则启动发�?
//===========================================================
static void OtherSendDataCheck(void)
{
    for (u8 i = 0; i < DT_ODNUM; i++) {
        if (OtherSendData[i].WTS) {
            ANO_DT_Send_Data(OtherSendData[i].type, OtherSendData[i].data, OtherSendData[i].len);
            OtherSendData[i].WTS = 0;
            return;
        }
    }
}
//===========================================================
//检查循环发送的数据有没有到指定的时间间�?
//===========================================================
static void CheckDotMs(u8 type, u8 id_addr)
{
    u16 *_dot_ms;
    u16 *_dot_cnt_ms;
    u8   *_dot_WTS;

    switch (type) {
    case USE_HID:
        _dot_ms = & dt.txSet_usb[id_addr].fre_ms;
        _dot_cnt_ms = & dt.txSet_usb[id_addr].time_cnt_ms;
        _dot_WTS = & dt.txSet_usb[id_addr].WTS;
        break;

    case USE_U1:
        _dot_ms = & dt.txSet_u1[id_addr].fre_ms;
        _dot_cnt_ms = & dt.txSet_u1[id_addr].time_cnt_ms;
        _dot_WTS = & dt.txSet_u1[id_addr].WTS;
        break;

    case USE_U2:
        _dot_ms = & dt.txSet_u2[id_addr].fre_ms;
        _dot_cnt_ms = & dt.txSet_u2[id_addr].time_cnt_ms;
        _dot_WTS = & dt.txSet_u2[id_addr].WTS;
        break;

    default:
        return ;
    }

    if (id_addr >= CSID_NUM)
        return ;

    if ((*_dot_ms) != 0) {
        if ((*_dot_cnt_ms) < (*_dot_ms))
            (*_dot_cnt_ms)++;
        else {
            //清除计时并置位等待发送标�?
            (*_dot_WTS) = 1;
            (*_dot_cnt_ms) = 1;
        }
    }
}
//===========================================================
//检查有没有需要循环发送的数据
//===========================================================
u8 CheckDotWts(u8 type, u8 id_addr)
{
    u8 _addr;
    u8   *_dot_WTS;

    switch (type) {
    case USE_HID:
        _addr = 0xaf;
        _dot_WTS = & dt.txSet_usb[id_addr].WTS;
        break;

    case USE_U1:
        _addr = 0xff;
        _dot_WTS = & dt.txSet_u1[id_addr].WTS;
        break;

    case USE_U2:
        _addr = 0xaf;//swj
        _dot_WTS = & dt.txSet_u2[id_addr].WTS;
        break;

    default:
        return 0;
    }

    if (id_addr >= CSID_NUM)
        return 0;

    if ((*_dot_WTS)) {
        //复位等待发送标�?
        (*_dot_WTS) = 0;
        //实际发�?
        DTFrameSend(type, id_addr, _addr);
        return 1;
    } else
        return 0;
}

//===========================================================
//发送Check帧，见协议文�?
//===========================================================
static void SendCheck(u8 type, u8 dest_addr, u8 cid, u8 sc, u8 ac)
{
    u8 _cnt = 0;

    otherDataTmp[_cnt++] = FRAME_HEAD;
    otherDataTmp[_cnt++] = dest_addr;
    otherDataTmp[_cnt++] = 0;
    otherDataTmp[_cnt++] = 0;

    otherDataTmp[_cnt++] = cid;
    otherDataTmp[_cnt++] = sc;
    otherDataTmp[_cnt++] = ac;

    otherDataTmp[3] = _cnt - 4;
    u8 check_sum1 = 0, check_sum2 = 0;

    for (u8 i = 0; i < _cnt; i++) {
        check_sum1 += otherDataTmp[i];
        check_sum2 += check_sum1;
    }

    otherDataTmp[_cnt++] = check_sum1;
    otherDataTmp[_cnt++] = check_sum2;

    OtherSendDataAdd(type, otherDataTmp, _cnt);
}
//===========================================================
//发送参�?
//===========================================================
static void SendPar(u8 type, u8 dest_addr, u16 p_id)
{
    s32 p_val = 0;
    p_val = fmu_read_para(p_id);

    u8 _cnt = 0;

    otherDataTmp[_cnt++] = FRAME_HEAD;
    otherDataTmp[_cnt++] = dest_addr;
    otherDataTmp[_cnt++] = 0xE2;
    otherDataTmp[_cnt++] = 0;

    otherDataTmp[_cnt++] = BYTE0(p_id);
    otherDataTmp[_cnt++] = BYTE1(p_id);

    otherDataTmp[_cnt++] = BYTE0(p_val);
    otherDataTmp[_cnt++] = BYTE1(p_val);
    otherDataTmp[_cnt++] = BYTE2(p_val);
    otherDataTmp[_cnt++] = BYTE3(p_val);

    otherDataTmp[3] = _cnt - 4;
    u8 check_sum1 = 0, check_sum2 = 0;

    for (u8 i = 0; i < _cnt; i++) {
        check_sum1 += otherDataTmp[i];
        check_sum2 += check_sum1;
    }

    otherDataTmp[_cnt++] = check_sum1;
    otherDataTmp[_cnt++] = check_sum2;

    OtherSendDataAdd(type, otherDataTmp, _cnt);
}
//===========================================================
//接收到参数后启动参数保存
//===========================================================
static void WritePar(u16 p_id, s32 p_val)
{
    switch (p_id) {

    default:
        p_val = p_val;
        break;
    }

    //触发延时写入
    //Data_Save_Trig();
}
//===========================================================
//发送字符串给上位机log区域显示
//===========================================================
void AnoDTSendStr(u8 type, u8 dest_addr, u8 string_color, char *str)
{
    u8 _cnt = 0;

    otherDataTmp[_cnt++] = FRAME_HEAD;
    otherDataTmp[_cnt++] = dest_addr;
    otherDataTmp[_cnt++] = 0xA0;
    otherDataTmp[_cnt++] = 0;

    otherDataTmp[_cnt++] = string_color;
    u8 i = 0;

    while (*(str + i) != '\0') { //单引号字符，双引号字符串
        otherDataTmp[_cnt++] = *(str + i++);

        if (_cnt > 50)
            break;
    }

    otherDataTmp[3] = _cnt - 4;
    u8 check_sum1 = 0, check_sum2 = 0;

    for (u8 i = 0; i < _cnt; i++) {
        check_sum1 += otherDataTmp[i];
        check_sum2 += check_sum1;
    }

    otherDataTmp[_cnt++] = check_sum1;
    otherDataTmp[_cnt++] = check_sum2;

    OtherSendDataAdd(type, otherDataTmp, _cnt);
}
//===========================================================
//发送字符串给上位机log区域显示
//===========================================================
void AnoDTSendF1(u8 type, u8 dest_addr, u8 d1)
{
    u8 _cnt = 0;

    otherDataTmp[_cnt++] = FRAME_HEAD;
    otherDataTmp[_cnt++] = dest_addr;
    otherDataTmp[_cnt++] = 0xF1;
    otherDataTmp[_cnt++] = 0;

    otherDataTmp[_cnt++] = d1;

    otherDataTmp[3] = _cnt - 4;
    u8 check_sum1 = 0, check_sum2 = 0;

    for (u8 i = 0; i < _cnt; i++) {
        check_sum1 += otherDataTmp[i];
        check_sum2 += check_sum1;
    }

    otherDataTmp[_cnt++] = check_sum1;
    otherDataTmp[_cnt++] = check_sum2;

    OtherSendDataAdd(type, otherDataTmp, _cnt);
}







/*============================================================================
******************************************************************************
******************************************************************************
数据接收相关函数
******************************************************************************
******************************************************************************
============================================================================*/
static void AnoDTDataAnl(u8 type, u8 *data, u8 len);
//===========================================================
//根据不同通信方式，定义接收数据缓冲区
//===========================================================
typedef struct {
    u8 DT_RxBuffer[DT_RX_BUFNUM];
    u8 _data_len;
    u8 _data_cnt;
    u8 rxstate;
} _dt_rx_anl_st;
//===========================================================
//数据每接收一字节，调用本函数，进行数据解�?
//===========================================================
void AnoDTRxOneByte(u8 type, u8 data)
{
    static _dt_rx_anl_st rx_anl[3];

    switch (rx_anl[type].rxstate) {
    case 0: {
        if (data == 0xAA) {
            rx_anl[type].rxstate = 1;
            rx_anl[type].DT_RxBuffer[0] = data;
        }
    }
    break;

    case 1: {
        if ((data == HW_TYPE || data == HW_ALL || data == 0x05)) {
            rx_anl[type].rxstate = 2;
            rx_anl[type].DT_RxBuffer[1] = data;
        } else
            rx_anl[type].rxstate = 0;
    }
    break;

    case 2: {
        rx_anl[type].rxstate = 3;
        rx_anl[type].DT_RxBuffer[2] = data;
    }
    break;

    case 3: {
        if (data < (DT_RX_BUFNUM - 5)) {
            rx_anl[type].rxstate = 4;
            rx_anl[type].DT_RxBuffer[3] = data;
            rx_anl[type]._data_len = data;
            rx_anl[type]._data_cnt = 0;
        } else
            rx_anl[type].rxstate = 0;
    }
    break;

    case 4: {
        if (rx_anl[type]._data_len > 0) {
            rx_anl[type]._data_len--;
            rx_anl[type].DT_RxBuffer[4 + rx_anl[type]._data_cnt++] = data;

            if (rx_anl[type]._data_len == 0)
                rx_anl[type].rxstate = 5;
        } else
            rx_anl[type].rxstate = 0;
    }
    break;

    case 5: {
        rx_anl[type].rxstate = 6;
        rx_anl[type].DT_RxBuffer[4 + rx_anl[type]._data_cnt++] = data;
    }
    break;

    case 6: {
        u8 _data_cnt;
        rx_anl[type].rxstate = 0;
        rx_anl[type].DT_RxBuffer[4 + rx_anl[type]._data_cnt] = data;
        _data_cnt = rx_anl[type]._data_cnt + 5;
        //ano_dt_data_ok = 1;
        AnoDTDataAnl(type, rx_anl[type].DT_RxBuffer, _data_cnt);
    }
    break;

    default : {
        rx_anl[type].rxstate = 0;
    }
    break;
    }
}
void AnoDTRxOneByteUart(u8 data)
{
    AnoDTRxOneByte(USE_U2, data);
}
void AnoDTRxOneByteUsb(u8 data)
{
    AnoDTRxOneByte(USE_HID, data);
}
u8 test_cali_cnt;
//===========================================================
//AnoDTDataAnl函数是协议数据解析函数，函数参数是符合协议格式的一个数据帧，该函数会首先对协议数据进行校验
//此函数可以不用用户自行调用，由函数Data_Receive_Prepare自动调用
//===========================================================
static void AnoDTDataAnl(u8 type, u8 *data, u8 len)
{
    u8 check_sum1 = 0, check_sum2 = 0;

    if (*(data + 3) != (len - 6))	//判断数据长度是否正确
        return;

    for (u8 i = 0; i < len - 2; i++) {
        check_sum1 += *(data + i);
        check_sum2 += check_sum1;
    }

    if ((check_sum1 != *(data + len - 2)) || (check_sum2 != *(data + len - 1)))	//判断sum校验
        return;

    //=============================================================================
    if (*(data + 2) == 0X40) { //摇杆数据

    } else if (*(data + 2) == 0X41) { //实时控制数据

    } else if (*(data + 2) == 0X30) { //GPS数据

    } else if (*(data + 2) == 0X32) { //通用位置

    } else if (*(data + 2) == 0X33) { //通用速度

    } else if (*(data + 2) == 0X34) { //通用测距

    } else if (*(data + 2) == 0X0D) {		//电压

    } else if (*(data + 2) == 0XE0) {		//命令E0
        switch (*(data + 4)) {	//CID
        case 0x01: {
            if (*(data + 5) == 0x00) {
                if (*(data + 6) == 0x01) { //acc

                } else if (*(data + 6) == 0x02) { //gyro
                    if (flag.unlock_sta == 0)
                        st_imu_cali.gyr_cali_on = 1;
                } else if (*(data + 6) == 0x03) { //horizontal

                } else if (*(data + 6) == 0x04) { //ecp

                } else if (*(data + 6) == 0x05) { //6 side

                } else if (*(data + 6) == 0x10) { //imu_reset

                } else if (*(data + 6) == 0x61) {	//存储航点

                } else if (*(data + 6) == 0x62) {	//清空航点

                } else if (*(data + 6) == 0xAA) {	//恢复默认PID
                    pid_reset();
                    data_save();

                } else if (*(data + 6) == 0xAB) {	//恢复默认参数
                    para_reset(1);
                    data_save();
                } else if (*(data + 6) == 0xAC) {	//恢复所有参�?
                    pid_reset();
                    para_reset(2);
                    data_save();
                }
            } else if (*(data + 5) == 0x01) {
                if (*(data + 6) == 0x01) { //飞控模式

                }
            } else if (*(data + 5) == 0x30) { //校准exe
                //校准命令
                if (*(data + 6) == 0x01) { //
                    if (flag.unlock_sta == 0) {
                        test_cali_cnt++;
                        //开�?面校�?
                        st_imu_cali.acc_cali_on = 1;
                        Ano_Parame.set.acc_calibrated = 0;
                    }
                } else if (*(data + 6) == 0x02) {
                    if (flag.unlock_sta == 0) {
                        //开始mag校准
                        mag.mag_CALIBRATE = 2;
                        Ano_Parame.set.mag_calibrated = 0;
                    }
                }
            }
        }
        break;

        case 0x10: { //指令控制
            if (*(data + 5) == 0x00) {
                switch (*(data + 6)) {
                case 0x01: { //解锁

                }
                break;

                case 0x02: { //上锁

                }
                break;

                case 0x03: {

                }
                break;

                case 0x04: { //立即悬停

                }
                break;

                case 0x05: { //起飞，带高度参数

                }
                break;

                case 0x06: { //降落

                }
                break;

                case 0x07: { //返航

                }
                break;

                case 0x08: { //翻滚

                }
                break;

                case 0x09: { //环绕

                }
                break;

                case 0x0a: { //无头

                }
                break;

                case 0x60: { //开始航�?

                }
                break;

                case 0x61: { //暂停航点

                }
                break;

                case 0x62: { //取消航点

                }
                break;

                default :
                    break;
                }
            } else if (*(data + 5) == 0x01) {

            } else if (*(data + 5) == 0x02) {

            } else if (*(data + 5) == 0x03) {

            }
        }
        break;

        case 0x11:
            break;

        default:
            break;
        }

        //需返回CHECK�?
        SendCheck(type, SWJ_ADDR, *(data + 2), check_sum1, check_sum2);
    } else if (*(data + 2) == 0X00) { //Check

    } else if (*(data + 2) == 0XE1) {
        //读取参数
        SendPar(type, SWJ_ADDR, *(u16 *)(data + 4));
    } else if (*(data + 2) == 0xE2) {
        //写入参数
        fmu_write_para(*(u16 *)(data + 4), *(s32 *)(data + 6));
        //写入参数后需返回CHECK帧，告诉上位机参数写入成�?
        SendCheck(type, SWJ_ADDR, *(data + 2), check_sum1, check_sum2);
    } else if (*(data + 2) == 0x60) {
        //读取航点
    } else if (*(data + 2) == 0x61) {
        //写入航点
    }
}











//===========================================================
//整体数据通信的调度器，需�?ms调用一�?
//===========================================================
void ANO_DT_Task1Ms(void)
{
    //检查有没有非循环发送的数据需要发�?
    OtherSendDataCheck();

    //检查循环发送的数据有没有需要发送的
    for (u8 i = 0; i < CSID_NUM; i++) {
        CheckDotMs(USE_U2, i);
        CheckDotMs(USE_HID, i);
        //CheckDotMs(USE_U1,i);
    }

    //串口数据发送执�?
    for (u8 i = 0; i < CSID_NUM; i++)
        CheckDotWts(USE_U2, i);

    //USB数据发送执�?
    for (u8 i = 0; i < CSID_NUM; i++) {
        if (CheckDotWts(USE_HID, i))
            break;
    }

    //USB发送服务程�?
    Usb_Hid_Send();
}
/******************* (C) COPYRIGHT 2014 ANO TECH *****END OF FILE************/
