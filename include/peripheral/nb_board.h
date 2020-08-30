#ifndef __NB_BOARD_H
#define __NB_BOARD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

//Error const number

typedef enum
{
    MSG_NONE,
    MSG_INIT,
    MSG_IMSI,
    MSG_MODULE_INFO,
    MSG_MID,     //The manufacturer ID
    MSG_MMODEL,  //Manufacturer's model
    MSG_MREV,    //Manufacturer version number
    MSG_BAND,    //Working frequency band
    MSG_IMEI,    //Mobile device id
    MSG_SIGN,    //Signal strength
    MSG_REG,
    MSG_UDP_CREATE,
    MSG_UDP_CLOSE,
    MSG_UDP_SEND,
    MSG_UDP_RECE,
    MSG_COAP,
    MSG_COAP_SEND,
    MSG_COAP_RECE,
    MSG_END
} NB_MessageTypeDef;

typedef struct NB_ConfigTypeDef *NB_Handle;


typedef int (*NB_Open)(NB_Handle);
typedef int (*NB_Reboot)(NB_Handle);
typedef int (*NB_Close)(NB_Handle);
typedef int (*NB_AutoInitModule)(NB_Handle);
typedef int (*NB_ModuleInfo)(NB_Handle);
typedef int (*NB_NetRegisterInfo)(NB_Handle);
typedef const char *(*NB_USIMInfo)(NB_Handle);
typedef int (*NB_NetSign)(NB_Handle);
typedef int (*NB_DefPDP)(NB_Handle);//Define the NB module communication context
typedef int (*NB_DeactPDP)(NB_Handle);//Disabled NB module context information
typedef int (*NB_CreateUdp)(NB_Handle);
typedef int (*NB_CloseUdp)(NB_Handle);
typedef int (*NB_SendUdpData)(NB_Handle, int, char *);
typedef int (*NB_CoAPServer)(NB_Handle,  bool, char *);
typedef int (*NB_CoAPSentIndication)(NB_Handle, int code);//CoAP send prompt setting
typedef int (*NB_CoAPReceMode)(NB_Handle, int);//CoAP receive data mode Settings
typedef int (*NB_CoAPSentMsg)(NB_Handle, int, char *);
typedef int (*NB_Reset)(NB_Handle);
typedef int (*NB_Ping)(NB_Handle);
//typedef int (*NB_isUdpReceData)(NB_Handle, int, char*);//Determine whether the received data is Udp
//typedef int (*NB_IsCoAPReceData)(NB_Handle, int, char*);//Determine whether the received data is CoAP data
typedef int (*NB_ReceUdpData)(NB_Handle);//Receiving UDP data
typedef int (*NB_CoAPReceMsg)(NB_Handle);//CoAP protocol information receiving

typedef int (*NB_MainThread)(NB_Handle);

typedef struct
{
    NB_Open                 Open;
    NB_Reboot               Reboot;
    NB_AutoInitModule       nbModuleInit;
    NB_ModuleInfo           getModuleInfo;
    NB_NetRegisterInfo      getModuleRegisterInfo;
    NB_USIMInfo             getUSIMinfo;
    NB_NetSign              getSign;
//  NB_DefPDP               definePDP;
//  NB_DeactPDP             deactPDP;
    NB_CreateUdp            createUdp;
    NB_CloseUdp             closeUdp;
    NB_SendUdpData          sendUdp;
//  NB_isUdpReceData        isUdprecedata;
    NB_ReceUdpData          receUdp;
    NB_CoAPServer           coapServer;
    NB_CoAPSentIndication   coapSentIndication;
    NB_CoAPReceMode         coapSetReceMode;
    NB_CoAPSentMsg          coapSentMsg;
    NB_CoAPReceMsg          coapReceMsg;
//  NB_Reset                nbReset;
//  NB_Ping                 nbPing;
//  NB_Close                closeFxn;
    NB_MainThread           mainThread;
} NB_ModuleTypeDef;


typedef int (*NB_ReceiveCallback)(NB_MessageTypeDef, int, char *);

typedef int (*NB_Log)(char *, int);

typedef struct NB_ConfigTypeDef
{
    NB_ModuleTypeDef   *Module;
    void               *Object;         //NB NB uart object pointer
    NB_ReceiveCallback AppReceiveCallback;
    NB_Log             Log;
} NB_ConfigTypeDef;


extern uint8_t NBModule_open(NB_Handle handle);
extern uint8_t NBModule_Reboot(NB_Handle handle);
extern uint8_t NBModule_Init(NB_Handle handle);
extern uint8_t NBModule_Info(NB_Handle handle);
extern uint8_t NBModule_isRegister(NB_Handle handle);
extern const char *NBModule_IMSI(NB_Handle handle);
extern uint8_t NBModule_Sign(NB_Handle handle);
extern uint8_t NBModule_DeactPDP(NB_Handle handle);
extern uint8_t NBModule_CreateUDP(NB_Handle handle);
extern uint8_t NBModule_CloseUDP(NB_Handle handle);
extern uint8_t NBModule_SendData(NB_Handle handle, int len, char *msg);
extern uint8_t NBModule_ReceiveData(NB_Handle handle);
extern uint8_t NBModule_CoAPServer(NB_Handle handle, bool isSet, char *coap);
extern uint8_t NBModule_CoAPSentIndication(NB_Handle handle, int code);
extern uint8_t NBModule_CoAPReceIndication(NB_Handle handle, int code);
extern uint8_t NBModule_CoAPSendMsg(NB_Handle handle, int len, char *msg);
extern uint8_t NBModule_CoAPReceMsg(NB_Handle handle);
extern void NBModule_Reset(NB_Handle handle);
extern void NBModule_Ping(NB_Handle handle, char *ipAddr);
extern int NBModule_Main(NB_Handle handle);

#ifdef __cplusplus
}
#endif

#endif    //__NB_BOARD_H
