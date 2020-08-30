#include "peripheral/nb_board.h"
/**
  * @step 1:  For file nb_app.c call, no need to care about the implementation process.
  */

extern const NB_ModuleTypeDef BC95_FxnTable;

extern Bool NBModule_open(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->Open == null)
        return FALSE;

    handle->Module = (void *)&BC95_FxnTable;

    handle->Module->Open(handle);

    return TRUE;
}

extern Bool NBModule_Init(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->nbModuleInit == null)
        return FALSE;

    handle->Module->nbModuleInit(handle);
    return TRUE;
}

uint8_t NBModule_Info(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->getModuleInfo == null)
        return FALSE;

    handle->Module->getModuleInfo(handle);

    return TRUE;
}

//Check if NB module is registered in the network
uint8_t NBModule_isRegister(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->getModuleRegisterInfo == null)
        return FALSE;

    return handle->Module->getModuleRegisterInfo(handle);
}

const char *NBModule_IMSI(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->getUSIMinfo == null)
        return FALSE;

    return handle->Module->getUSIMinfo(handle);
}


// return : TRUE->Instruction executed£¬FALSE->An error occurred during the execution of the instruction
// Note£º
// The original signal value is asynchronous to the callback function by message
extern uint8_t NBModule_Sign(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->getSign == null)
        return FALSE;

    return handle->Module->getSign(handle);
}

uint8_t NBModule_CreateUDP(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->createUdp == null)
        return FALSE;

    return handle->Module->createUdp(handle);
}

uint8_t NBModule_CloseUDP(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->closeUdp == null)
        return FALSE;

    return handle->Module->closeUdp(handle);
}

uint8_t NBModule_SendData(NB_Handle handle, int len, char *msg)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->sendUdp == null)
        return FALSE;

    return handle->Module->sendUdp(handle, len, msg);
}

uint8_t NBModule_ReceiveData(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->receUdp == null)
        return FALSE;

    return handle->Module->receUdp(handle);
}

// brief : Set and query NB module current CoAP server information
//         isSet  -> true -> write,
//                   false-> read
uint8_t NBModule_CoAPServer(NB_Handle handle, Bool isSet, char *coap)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->coapServer == null)
        return FALSE;

    return handle->Module->coapServer(handle, isSet, coap);
}

// brief : Set the result indication mode after CoAP sends the message
//         code   -> 0  no response
//                   !0 response
uint8_t NBModule_CoAPSentIndication(NB_Handle handle, int code)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->coapSentIndication == null)
        return FALSE;

    return handle->Module->coapSentIndication(handle, code);
}

// brief : Set CoAP receive message prompt mode
//         code -> 0 means cache and 1 means receive directly.(currently only two modes are supported)
uint8_t NBModule_CoAPReceIndication(NB_Handle handle, int code)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->coapSetReceMode == null)
        return FALSE;

    return handle->Module->coapSetReceMode(handle, code);
}

uint8_t NBModule_CoAPSendMsg(NB_Handle handle, int len, char *msg)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->coapSentMsg == null)
        return FALSE;

    return handle->Module->coapSentMsg(handle, len, msg);
}

uint8_t NBModule_CoAPReceMsg(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->coapReceMsg == null)
        return FALSE;

    return handle->Module->coapReceMsg(handle);
}

uint8_t NBModule_Reboot(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->Reboot == null)
        return FALSE;

    return handle->Module->Reboot(handle);
}

// A continuous call in a loop
extern int NBModule_Main(NB_Handle handle)
{
    if(handle == null)
        return FALSE;

    if(handle->Module->mainThread == null)
        return FALSE;

    return handle->Module->mainThread(handle);

}
