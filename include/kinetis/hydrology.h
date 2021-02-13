#ifndef __MHYDROLOGY_H
#define __MHYDROLOGY_H

#include "stdint.h"
#include "kinetis/slist.h"

#define ELEMENT_COUNT                   298

#define ERC1                            1
#define ERC2                            2
#define ERC3                            3
#define ERC4                            4
#define ERC5                            5
#define ERC6                            6
#define ERC7                            7
#define ERC8                            8
#define ERC9                            9
#define ERC10                           10
#define ERC11                           11
#define ERC12                           12
#define ERC13                           13
#define ERC14                           14
#define ERC15                           15
#define ERC16                           16
#define ERC17                           17
#define ERC18                           18
#define ERC19                           19

#define SOH                             0x7E
#define STX                             0x02
#define SYN                             0x16
#define ETX                             0x03
#define ETB                             0x17
#define ENQ                             0x05
#define EOT                             0x04
#define ACK                             0x06
#define NAK                             0x15
#define ESC                             0x1B

typedef enum tagHydrologyMode {
    HYDROLOGY_M1,
    HYDROLOGY_M2,
    HYDROLOGY_M3,
    HYDROLOGY_M4,
} HydrologyMode;

typedef enum tagHydrologyRTUType {
    Rainfall = 0x50,
    RiverCourse = 0x48,
    Reservoir = 0x4B,
    GateDam = 0x5A,
    PumpingStation = 0x44,
    Tide = 0x54,
    SoilMoisture = 0x4D,
    Groundwater = 0x47,
    WaterQuality = 0x51,
    WaterIntake = 0x49,
    Outfall = 0x4F,
} HydrologyRTUType;

typedef enum tagHydrologyBodyType {
    LinkMaintenance = 0x2F,               //遥测站链路维持报
    Test,                                 //遥测站测试报
    EvenPeriodInformation,                //均匀时段水文信息报
    TimerReport,                          //遥测站定时报
    AddReport,                            //遥测站加报报
    Hour,                                 //遥测站小时报
    ArtificialNumber,                     //遥测站人工置数报
    Picture,                              //遥测站图片报
    Realtime,                             //中心站查询遥测站实时数据
    Period,                               //中心站查询遥测站时段数据
    InquireArtificialNumber,              //中心站查询遥测站人工置数
    SpecifiedElement,                     //中心站查询遥测站指定要素实时数据
    ConfigurationModification = 0x40,     //遥测站配置修改
    ConfigurationRead,                    //遥测站配置读取
    ParameterModification,                //中心站修改遥测站运行参数
    ParameterRead,                        //中心站读取遥测站运行参数
    WaterPumpMotor,                       //中心站查询水泵电机实时工作数据
    SoftwareVersion,                      //中心站查询遥测站查询遥测站软件版本
    Status,                               //中心站查询遥测站状态信息
    InitializeSolidStorage,               //初始化固态存储数据
    Reset,                                //恢复遥测站出厂设置
    ChangePassword,                       //中心站修改传输密码
    SetClock,                             //中心站设置遥测站时钟
    SetICCard,                            //中心站设置遥测站IC卡状态
    Pump,                                 //中心站设置遥测站水泵开关命令响应/ 水泵状态自报
    Valve,                                //中心站设置遥测站控制阀门开关命令响应/ 阀门状态自报
    Gate,                                 //中心站设置遥测站控制闸门开关命令响应/ 闸门状态信息自报
    WaterSetting,                         //中心站设置遥测站水量定值控制命令响应
    Record,                               //中心站查询遥测站事件记录
    Time,                                 //中心站查询遥测站时钟
} HydrologyBodyType;

typedef enum tagHydrologyMsgSrcType {
    MsgFormServer,
    MsgFormClient
} HydrologyMsgSrcType;

//#pragma pack(1)

typedef struct tagHydrologyElementInfo {
    u8 ID;
    u8 D;
    u8 d;
    u32 Addr;
} HydrologyElementInfo;

typedef struct tagHydrologyElement {
    u8 guide[2];
    u8 *value;
    u32 num;
} HydrologyElement;

//遥测站上行报文报头结构
typedef struct tagHydrologyUpHeader {
    u8 framestart[2];
    u8 centeraddr;
    u8 remoteaddr[5];
    u8 password[2];
    u8 funcode;
    u8 dir_len[2];
    u8 paketstart;
    u8 count_seq[3];
    u8 len;
} HydrologyUpHeader;

//遥测站下行报文报头结构
typedef struct tagHydrologyDownHeader {
    u8 framestart[2];
    u8 remoteaddr[5];
    u8 centeraddr;
    u8 password[2];
    u8 funcode;
    u8 dir_len[2];
    u8 paketstart;
    u8 count_seq[3];
    u8 len;
} HydrologyDownHeader;

//遥测站上行报文正文结构
typedef struct tagHydrologyUpBody {
    u8 streamid[2];
    u8 sendtime[6];
    u8 rtuaddrid[2];
    u8 rtuaddr[5];
    u8 rtutype;
    u8 observationtimeid[2];
    u8 observationtime[5];
    u16 count;
    HydrologyElement **element;
    u16 len;
} HydrologyUpBody;

//遥测站下行报文正文结构
typedef struct tagHydrologyDownBody {
    u8 streamid[2];
    u8 sendtime[6];
    u8 rtuaddrid[2];
    u8 rtuaddr[5];
    u16 count;
    HydrologyElement **element;
    u16 len;
} HydrologyDownBody;

typedef struct tagHydrologyPacket {
    void *header;
    void *body;
    u8 end;
    u16 crc16;
    u8 *buffer;
    u16 len;
} HydrologyPacket;

typedef struct tagHydrology {
    HydrologyPacket *uppacket;
    HydrologyPacket *downpacket;
    unsigned source: 1;
} Hydrology;

//#pragma pack()

int Hydrology_read_file_size(char *filename, u32 *Size);
int Hydrology_ReadStoreInfo(char *filename, long addr, u8 *data, int len);
int Hydrology_WriteStoreInfo(char *filename, long addr, u8 *data, int len);
void Hydrology_ReadTime(u8 *time);
void Hydrology_SetTime(u8 *t_time);
int Hydrology_OpenPort(void);
int Hydrology_ClosePort(void);
int Hydrology_PortTransmmitData(u8 *pData, u16 Len);
int Hydrology_PortReceiveData(u8 **ppData, u16 *pLen, u32 Timeout);
void Hydrology_ReadObservationTime(HydrologyElementInfo *Element, u8 *observationtime);
void Hydrology_SetObservationTime(HydrologyElementInfo *Element);
void Hydrology_GetGuideID(u8 *value, u8 D, u8 d);
int Hydrology_ConvertToHexElement(double input, int D, int d, u8 *out);
int Hydrology_MallocElement(u8 element, u8 D, u8 d,
    HydrologyElement *ele);
void Hydrology_GetStreamID(u8 *streamid);
int HydrologyD_ProcessSend(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyMode Mode, HydrologyBodyType Funcode);
int Hydrology_ReadSpecifiedElementInfo(HydrologyElementInfo *Element,
    HydrologyBodyType Funcode, u16 Index);
int HydrologyH_ProcessSend(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyMode Mode, HydrologyBodyType Funcode, u8 End);
int HydrologyD_Process(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyMode Mode, HydrologyBodyType Funcode);
void Hydrology_DisableLinkPacket(void);
void Hydrology_EnableLinkPacket(void);
void Hydrology_DisconnectLink(void);
int HydrologyD_Reboot(void);

extern Hydrology g_Hydrology;

#endif
