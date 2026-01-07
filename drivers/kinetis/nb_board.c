#include "kinetis/nb_board.h"
/**
  * @step 1:  For file nb_app.c call, no need to care about the implementation process.
  */

extern const NB_ModuleTypeDef BC95_FxnTable;

u8 NBModule_open(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->Open == NULL)
        return false;

    handle->Module = (void *)&BC95_FxnTable;

    handle->Module->Open(handle);

    return true;
}

u8 NBModule_Init(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->nbModuleInit == NULL)
        return false;

    handle->Module->nbModuleInit(handle);
    return true;
}

u8 NBModule_Info(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->getModuleInfo == NULL)
        return false;

    handle->Module->getModuleInfo(handle);

    return true;
}

//Check if NB module is registered in the network
u8 NBModule_isRegister(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->getModuleRegisterInfo == NULL)
        return false;

    return handle->Module->getModuleRegisterInfo(handle);
}

const char *NBModule_IMSI(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->getUSIMinfo == NULL)
        return false;

    return handle->Module->getUSIMinfo(handle);
}


// return : true->Instruction executed，false->An error occurred during the execution of the instruction
// Note：
// The original signal value is asynchronous to the callback function by message
extern u8 NBModule_Sign(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->getSign == NULL)
        return false;

    return handle->Module->getSign(handle);
}

u8 NBModule_CreateUDP(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->createUdp == NULL)
        return false;

    return handle->Module->createUdp(handle);
}

u8 NBModule_CloseUDP(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->closeUdp == NULL)
        return false;

    return handle->Module->closeUdp(handle);
}

u8 NBModule_SendData(NB_Handle handle, int len, char *msg)
{
    if (handle == NULL)
        return false;

    if (handle->Module->sendUdp == NULL)
        return false;

    return handle->Module->sendUdp(handle, len, msg);
}

u8 NBModule_ReceiveData(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->receUdp == NULL)
        return false;

    return handle->Module->receUdp(handle);
}

// brief : Set and query NB module current CoAP server information
//         isSet  -> true -> write,
//                   false-> read
u8 NBModule_CoAPServer(NB_Handle handle, bool isSet, char *coap)
{
    if (handle == NULL)
        return false;

    if (handle->Module->coapServer == NULL)
        return false;

    return handle->Module->coapServer(handle, isSet, coap);
}

// brief : Set the result indication mode after CoAP sends the message
//         code   -> 0  no response
//                   !0 response
u8 NBModule_CoAPSentIndication(NB_Handle handle, int code)
{
    if (handle == NULL)
        return false;

    if (handle->Module->coapSentIndication == NULL)
        return false;

    return handle->Module->coapSentIndication(handle, code);
}

// brief : Set CoAP receive message prompt mode
//         code -> 0 means cache and 1 means receive directly.(currently only two modes are supported)
u8 NBModule_CoAPReceIndication(NB_Handle handle, int code)
{
    if (handle == NULL)
        return false;

    if (handle->Module->coapSetReceMode == NULL)
        return false;

    return handle->Module->coapSetReceMode(handle, code);
}

u8 NBModule_CoAPSendMsg(NB_Handle handle, int len, char *msg)
{
    if (handle == NULL)
        return false;

    if (handle->Module->coapSentMsg == NULL)
        return false;

    return handle->Module->coapSentMsg(handle, len, msg);
}

u8 NBModule_CoAPReceMsg(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->coapReceMsg == NULL)
        return false;

    return handle->Module->coapReceMsg(handle);
}

u8 NBModule_Reboot(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->Reboot == NULL)
        return false;

    return handle->Module->Reboot(handle);
}

// A continuous call in a loop
extern int NBModule_Main(NB_Handle handle)
{
    if (handle == NULL)
        return false;

    if (handle->Module->mainThread == NULL)
        return false;

    return handle->Module->mainThread(handle);

}
