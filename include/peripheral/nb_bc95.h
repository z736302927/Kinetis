#ifndef __NB_BC95_H
#define __NB_BC95_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "nb_board.h"

/*
BC95 series of modules mainly interact with users based on UART.
All of the following operations about BC95 revolve around UART.
*/

typedef enum
{
    CMD_TEST,
    CMD_READ,
    CMD_SET,
    CMD_EXCUTE
} NB_CmdProperty;

typedef enum
{
    ACTION_OK_EXIT_ERROR_NEXT,     //Successful execution will exit, if the error will continue to execute the next instruction.
    ACTION_OK_NEXT_ERROR_TRY       //After successful execution, the next instruction is executed, and an error is attempted.
    //If the maximum number of attempts is not successful, then exit.
} NB_CmdAction;

//AT instruction structure type
typedef struct
{
    const char     *pCMD;          // AT Command
    NB_CmdProperty  property;      // Command current attribute (TEST,READ,SET,EXCUTE)
    char           *pArgument;     // Command parameter
    char           *pExpectRes;    // Expected response
    unsigned char   cmd_try;       // Number of attempts after an error
    unsigned char   haveTried;     // Number of attempts after an error has occurred
    NB_CmdAction    cmd_action;    // AT Command behavior
    uint16_t        max_timeout;   // Maximum timeout
    uint8_t         lmt_period;    // Repeat execution for a limited time interval
} NB_CmdInfo;

typedef NB_CmdInfo *CmdHandle;

typedef enum
{
    PROCESS_NONE,
    PROCESS_INIT = MSG_INIT,
    PROCESS_MODULE_INFO = MSG_MODULE_INFO,
    PROCESS_SIGN = MSG_SIGN,
    PROCESS_NET_REG = MSG_REG,
    PROCESS_UDP_CR = MSG_UDP_CREATE,
    PROCESS_UDP_CL = MSG_UDP_CLOSE,
    PROCESS_UDP_ST = MSG_UDP_SEND,
    PROCESS_UDP_RE = MSG_UDP_RECE,
    PROCESS_COAP  = MSG_COAP,
    PROCESS_COAP_ST = MSG_COAP_SEND,
    PROCESS_COAP_RE = MSG_COAP_RECE
} NB_Process;

typedef enum
{
    TYPES_CIMI = MSG_IMSI,
    TYPES_CGSN = MSG_IMEI,
    TYPES_CGMI = MSG_MID,
    TYPES_CGMM = MSG_MMODEL,
    TYPES_CGMR = MSG_MREV,
    TYPES_NBAND = MSG_BAND,
    TYPES_UDP_CR = MSG_UDP_CREATE,
    TYPES_UDP_CL = MSG_UDP_CLOSE,
    TYPES_UDP_SEND = MSG_UDP_SEND,
    TYPES_UDP_RECE = MSG_UDP_RECE
} NB_RptMsgType;

typedef struct
{
    NB_Process state;
    int sub_state;
} NB_ModuleState;


typedef void (*NB_RxCallback)(char *, uint16_t);
typedef void (*NB_SerilPort_Open)(NB_RxCallback, uint32_t);
typedef void (*NB_SerilPort_Send)(uint8_t *, uint16_t);
typedef void (*NB_SerilPort_Close)(void);

typedef struct
{
    NB_SerilPort_Open  Open;
    NB_SerilPort_Send  Send;
    NB_SerilPort_Close Close;
} NB_SerilPortTypeDef;

typedef void (*NB_TimeoutCallback)(void);

//==============================================================================
typedef void (*NB_Timer_Init)(NB_TimeoutCallback);
typedef void (*NB_Timer_Start)(uint32_t);
typedef void (*NB_Timer_Stop)(void);

typedef struct
{
    NB_Timer_Init   Init;
    NB_Timer_Start  Start;
    NB_Timer_Stop   Stop;
} NB_TimerTypeDef;


typedef struct
{
    const uint32_t          Baudrate;
    NB_SerilPortTypeDef   *Uart;
    NB_TimerTypeDef       *Timer;
} NB_HW_Object;

typedef NB_HW_Object *HWAttrs_Handle;
extern int bc95_open(NB_Handle handle);
extern int bc95_setbaud(NB_Handle handle, int baud);
extern int bc95_reboot(NB_Handle handle);
extern int bc95_init(NB_Handle handle);
extern int bc95_moduleInfo(NB_Handle handle);
extern int bc95_register(NB_Handle handle);
extern const char *bc95_getIMSI(NB_Handle handle);
extern int bc95_getSignal(NB_Handle handle);
extern int bc95_createUDP(NB_Handle handle);
extern int bc95_closeUDP(NB_Handle handle);
extern int bc95_sendUDP(NB_Handle handle, int len, char *msg);
extern int bc95_receUDP(NB_Handle handle);
extern int bc95_coapServer(NB_Handle handle, Bool isSet, char *coap);
extern int bc95_coapSentIndication(NB_Handle handle, int code);
extern int bc95_coapReceIndication(NB_Handle handle, int code);
extern int bc95_coapSendMsg(NB_Handle handle, int len, char *msg);
extern int bc95_coapReadMsg(NB_Handle handle);
extern int bc95_main(NB_Handle handle);

#ifdef __cplusplus
}
#endif

#endif   /* __NB_BC95_H */
