#include <linux/printk.h>
#include <linux/limits.h>

#include "kinetis/nb_timer.h"
#include "kinetis/nb_app.h"
#include "kinetis/nb_board.h"
#include "kinetis/nb_bc95.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  You should call the function NB_IOT_Init in the main function to initialize the NB module.
  * @step 3:  You should implement the struct members of NB_UartPort_Inst,NB_Timer_Inst and HWAtrrs_Object.
  * @step 4:  After each state is complete, NB_IOT_ResponseCallback is called.Including send and receive, you can achieve custom functions.
  * @step 5:  Finally, you only need to call NB_IOT_SendData to send and receive data to the cloud platform.
  * @step 6:  Note that you can use NB_IOT_Turnoff_Pipe to decide whether to close the channel or not.
  */
#include "kinetis/fsm.h"

#define DEBUG
#include "kinetis/idebug.h"


#define NB_Communication_Mode   0

int NB_IOT_ResponseCallback(NB_MessageTypeDef, int, char *);

extern void NB_IOT_UART_Open(NB_RxCallback cb, u32 baud);
extern void NB_IOT_UART_Send(u8 *pdata, u16 len);
extern void NB_IOT_UART_Close(void);
extern void NB_IOT_UART_RxBuffer_Init(void);

char *NB_UserDataPacket = NULL;
int NB_UserDataLength = 0;
u8 NB_MessageComing = 0;
u8 NB_Turnoff_Pipe = 1;

u8 NB_Response_Result = false;
u8 NB_Response_Done = false;
u8 *NB_Module_IMEI = NULL;

NB_SerilPortTypeDef  NB_UartPort_Inst = {
    .Open = NB_IOT_UART_Open,
    .Send = NB_IOT_UART_Send,
    .Close = NB_IOT_UART_Close
};

NB_TimerTypeDef NB_Timer_Inst = {
    .Init = NB_IOT_SetTim,
    .Start = NB_IOT_StartTim,
    .Stop = NB_IOT_StopTim
};

NB_HW_Object  HWAtrrs_Object = {
    .Baudrate = 9600,
    .Uart = &NB_UartPort_Inst,
    .Timer = &NB_Timer_Inst
};

NB_ConfigTypeDef  NB_Config_Inst = {
    .Module = NULL,
    .Object = (void *) &HWAtrrs_Object,
    .AppReceiveCallback = NB_IOT_ResponseCallback,
    .Log = NULL
};

int NB_IOT_ResponseCallback(NB_MessageTypeDef types, int len, char *msg)
{
    switch (types) {
        case MSG_INIT: {
            NB_Response_Done = true;

            if (*msg == 'S') {
                pr_debug("NET = ON");
                NB_Response_Result = true;
            } else
                NB_Response_Result = false;
        }
        break;

        case MSG_IMSI: {
            pr_debug("IMSI = %s", msg);
            NB_Response_Done = true;
            NB_Response_Result = true;
        }
        break;

        case MSG_REG: {
            pr_debug("NET = %s", (*msg) == 1 ? "ON" : "0FF");
            NB_Response_Done = true;

            if ((*msg) == 1)
                NB_Response_Result = true;
            else
                NB_Response_Result = false;
        }
        break;

        case MSG_SIGN: {
            pr_debug("%sdbm", msg);
            NB_Response_Done = true;
            NB_Response_Result = true;
        }
        break;

        case MSG_MODULE_INFO: {
            pr_debug("Minfo = %s", msg);
            NB_Response_Done = true;
            NB_Response_Result = true;
        }
        break;

        case MSG_MID: {
            pr_debug("MID = %s", msg);
        }
        break;

        case MSG_MMODEL: {
            pr_debug("Model = %s", msg);
        }
        break;

        case MSG_MREV: {
            pr_debug("REV = %s", msg);
        }
        break;

        case MSG_BAND: {
            pr_debug("Freq = %s", msg);
        }
        break;

        case MSG_IMEI: {
            pr_debug("IMEI = %s", msg);
            NB_Module_IMEI = (u8 *)msg;
            NB_Response_Done = true;
            NB_Response_Result = true;
        }
        break;

        case MSG_UDP_CREATE: {
            NB_Response_Done = true;

            if (*msg == 'S')
                NB_Response_Result = true;
            else
                NB_Response_Result = false;
        }
        break;

        case MSG_UDP_CLOSE: {
            NB_Response_Done = true;

            if (*msg == 'S')
                NB_Response_Result = true;
            else
                NB_Response_Result = false;
        }
        break;

        case MSG_UDP_SEND: {
            NB_Response_Done = true;

            if (*msg == 'S')
                NB_Response_Result = true;
            else
                NB_Response_Result = false;
        }
        break;

        case MSG_UDP_RECE: {
            pr_debug("UDP_RECE = %s", msg);
            print_hex_dump(KERN_DEBUG, "UDP_RECE: ", DUMP_PREFIX_OFFSET,
                16, 1,
                msg, len, false);
            NB_Response_Done = true;

            if (*msg == 'F')
                NB_Response_Result = false;
            else {
                NB_MessageComing = 0;
                NB_Response_Result = true;

                if (len > 3) {
//                     HydrologySetMsgSrc(MSG_FORM_SERVER);
//                     hydrology_port_receive((char *)msg, len, 5000);
                }
            }
        }
        break;

        case MSG_COAP: {
            NB_Response_Done = true;

            if (*msg == 'S')
                NB_Response_Result = true;
            else
                NB_Response_Result = false;
        }
        break;

        case MSG_COAP_SEND: {
            NB_Response_Done = true;

            if (*msg == 'S')
                NB_Response_Result = true;
            else
                NB_Response_Result = false;
        }
        break;

        case MSG_COAP_RECE: {
            pr_debug("COAP_RECE = %s", msg);
            print_hex_dump(KERN_DEBUG, "COAP_RECE: ", DUMP_PREFIX_OFFSET,
                16, 1,
                msg, len, false);
            NB_Response_Done = true;

            if (*msg == 'F') {
                NB_MessageComing = 1;
                NB_Response_Result = false;
            } else {
                NB_MessageComing = 0;
                NB_Response_Result = true;

                if (len > 3) {
//                     HydrologySetMsgSrc(MSG_FORM_SERVER);
//                     HydrologyProcessReceieve((char *)msg, len);
                }
            }
        }
        break;

        default :
            break;
    }

    return 0;
}

void NB_IOT_Init(void)
{
#if MCU_PLATFORM_STM32
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

    NB_IOT_UART_RxBuffer_Init();

    NBModule_open(&NB_Config_Inst);

    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
#else
#endif
}

int NB_IOT_ProcessRespond(void)
{
    u32 begintime;
    u32 currenttime;
    u32 timediff;

    begintime = basic_timer_get_ms();

    while (1) {
        NB_IOT_UART_Receive();
        NBModule_Main(&NB_Config_Inst);
        NB_IOT_PollTim();

        if (NB_Response_Done == true) {
            NB_Response_Done = false;

            if (NB_Response_Result == true) {
                NB_Response_Result = false;

                return true;
            } else
                return false;
        } else {
            currenttime = basic_timer_get_ms();
            timediff = currenttime >= begintime ? currenttime - begintime :
                currenttime + U32_MAX - begintime;

            if (timediff > 5000) /* 5s */
                return false;
        }
    }
}

int NB_IOT_WaitArrivalofMsg(void)
{
    u32 begintime;
    u32 currenttime;
    u32 timediff;

    begintime = basic_timer_get_ms();

    while (1) {
//    HAL_IWDG_Refresh(&hiwdg);
        NB_IOT_UART_Receive();
        NBModule_Main(&NB_Config_Inst);
        NB_IOT_PollTim();

        if (NB_Response_Done == true) {
            NB_Response_Done = false;

            if (NB_Response_Result == true) {
                NB_Response_Result = false;

                return true;
            } else
                return false;
        } else {
            currenttime = basic_timer_get_ms();
            timediff = currenttime >= begintime ? currenttime - begintime :
                currenttime + U32_MAX - begintime;

            if (timediff > 15000) { /* 10s */
                pr_debug("No data came !");
                return -1;
            }
        }
    }
}

int NB_IOT_Shutdown(void)
{
    int retValue = false;

#if MCU_PLATFORM_STM32
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    mdelay(1000);

    if ((GPIOA->IDR  & GPIO_PIN_0) != 0x00u)
        retValue = false;
    else
        retValue = true;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
#else
#endif

    NB_Response_Result = false;
    NB_Response_Done = false;

//  retValue = NBModule_Reboot(&NB_Config_Inst);
//  HAL_Delay(3000);

    return retValue;
}

void NB_IOT_Turnoff_Pipe(u8 onoff)
{
    NB_Turnoff_Pipe = onoff;
}

void NB_IOT_SendData(char *pdata, int len)
{
//     enum SState state;
//     struct state_machine machine;
//     struct sm_var var;
// 
//     if (NB_Turnoff_Pipe == 1)
//         machine.current_state = sNB_NONE;
//     else
//         machine.current_state = sNB_UDP_REGISTER;
// 
//     var._repeats = 0;
//     var._condition = cOK;
// 
//     NB_UserDataPacket = pdata;
//     NB_UserDataLength = len;
// 
//     while (machine.current_state != sNB_END) {
//         state = fsm_step(&machine, &var);
// 
//         if (NB_Turnoff_Pipe == 0 && machine.current_state == sNB_UDP_CLOSE)
//             break;
//     }
// 
//     if ((state == sNB_END) && (var._condition == cOK))
//         pr_debug("The data was successfully sent through the nb-iot device to the server.");
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

int FSM_NB_None(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    _retValue = true;

    sm_var->_condition = cOK;
    pr_debug("NB-IOT Device will start sending data.");

    return _retValue;
}

int FSM_NB_Init(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    NBModule_Init(&NB_Config_Inst);
    _retValue = NB_IOT_ProcessRespond();

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("NB-IOT Device initialization successful.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("NB-IOT Device initialization failed.");
    }

    return _retValue;
}

int FSM_NB_GetSign(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    NBModule_Sign(&NB_Config_Inst);
    _retValue = NB_IOT_ProcessRespond();

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("Signal detected in this area.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("No signal detected in this area.");
    }

    return _retValue;
}

int FSM_NB_GetModuleInfo(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    NBModule_Info(&NB_Config_Inst);
    _retValue = NB_IOT_ProcessRespond();

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("Device information obtained successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("Failed to get device information.");
    }

    return _retValue;
}

int FSM_NB_CreateUDP(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

#if NB_Communication_Mode
    NBModule_CreateUDP(&NB_Config_Inst);
#else
    NBModule_CoAPServer(&NB_Config_Inst, 1, NULL);
#endif
    _retValue = NB_IOT_ProcessRespond();

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("UDP Communication created successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("Failed to create UDP communication");
    }

    return _retValue;
}

int FSM_NB_CloseUDP(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

#if NB_Communication_Mode
    NBModule_CloseUDP(&NB_Config_Inst);
    _retValue = NB_IOT_ProcessRespond();
#else
    _retValue = true;
#endif
#if MCU_PLATFORM_STM32
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
#else
#endif

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("UDP Communication closed successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("Shutdown UDP communication failed.");
    }

    return _retValue;
}

int FSM_NB_UDPRegister(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

#if NB_Communication_Mode
    char regPacket[30];
    u8 msgLen = 0;
//  char* regPacket = "ep=863703036005069&pw=123456";

    msgLen = sprintf(regPacket, "ep=%s&pw=736302", "1RFN7HGAL3VKMD95");//NB_Module_IMEI
    regPacket[msgLen] = 0;
    NBModule_SendData(&NB_Config_Inst, msgLen, regPacket);
    _retValue = NB_IOT_ProcessRespond();
#else
//  msgLen = sprintf(regPacket, "ep=%s&pw=736302", NB_Module_IMEI);
//  regPacket[msgLen] = 0;
//  NBModule_CoAPSendMsg(&NB_Config_Inst, msgLen, regPacket);
    _retValue = true;
#endif

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("Guyu server registered successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("Guyu server registration failed.");
    }

    return _retValue;
}

int FSM_NB_UDPSendData(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

#if MCU_PLATFORM_STM32
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
#else
#endif

#if NB_Communication_Mode
    NBModule_SendData(&NB_Config_Inst, NB_UserDataLength, NB_UserDataPacket);
#else
    NBModule_CoAPSendMsg(&NB_Config_Inst, NB_UserDataLength, NB_UserDataPacket);
#endif
    _retValue = NB_IOT_ProcessRespond();

    if (_retValue == true) { /////////////////////////////
        sm_var->_condition = cOK;
        pr_debug("Data sent successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("Data transmission failure.");
    }

    return _retValue;
}

int FSM_NB_WaitReceiveData(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    _retValue = NB_IOT_WaitArrivalofMsg();

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("Data received successfully.");

        return _retValue;
    } else if (_retValue == false) {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("Data received failure.");
    } else {
        sm_var->_condition = cOK;
        _retValue = true;
    }

    return _retValue;
}

int FSM_NB_Reset(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    _retValue = NB_IOT_Shutdown();

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("NB Device reset successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("NB Device reset failure.");
    }

    return _retValue;
}

int FSM_NB_End(struct state_machine *machine, struct sm_var *sm_var)
{
    int _retValue = false;

    _retValue = true;

    if (_retValue == true) {
        sm_var->_condition = cOK;
        pr_debug("The state machine entered the final state successfully.");

        return _retValue;
    } else {
        sm_var->_condition = cERROR_REPEATS_S3;
        pr_debug("The state machine failed to enter the final state.");
    }

    return _retValue;
}
#ifdef DESIGN_VERIFICATION_NBIOT
{"test", fuction},
#endif