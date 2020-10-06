#include "algorithm/k-fsm.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  This driver USES single linked list structure to register tasks.Depending on the user's needs, choose to use RTC or timer.
  * @step 3:  Write callback functions and initialization functions, such as function Task_Temperature_Humidit_Callback and HydrologyTask_Init.
  * @step 4:  Modify four areas: XXTask_TypeDef/Task_XX_Callback/HydrologyTask_Init.
  * @step 5:  Finally, HydrologyTask_Init is called in the main function.
  */

#include "peripheral/nb_app.h"

#define DEBUG
#include "core/idebug.h"

#define FSM_printf    p_dbg

State FSM_Step(pStateMachine machine, SM_VAR *sm_var, pTransition **table)
{
    if (sm_var->_repeats < 2) {
        pTransition t = table[machine->current][sm_var->_condition];
        (*(t->action))(machine, sm_var);

        if (machine->current == t->next)
            sm_var->_repeats++;
        else
            sm_var->_repeats = 0;

        machine->current = t->next;
    } else {
        sm_var->_condition = cERROR_REPEATS_L3;
        pTransition t = table[machine->current][sm_var->_condition];
        (*(t->action))(machine, sm_var);

        sm_var->_repeats = 0;

        if (machine->current == sNB_UDP_RECEIVE)
            machine->current = sNB_UDP_CLOSE;
        else if (machine->current == sNB_UDP_CLOSE)
            machine->current = sNB_END;
        else
            machine->current = sNB_RESET;
    }

    return machine->current;
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef DESIGN_VERIFICATION_FSM
#include "dv/k-test.h"

extern int FSM_NB_None(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_Init(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_GetSign(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_GetModuleInfo(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_CreateUDP(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_CloseUDP(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_UDPRegister(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_UDPSendData(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_WaitReceiveData(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_Reset(pStateMachine machine, SM_VAR *sm_var);
extern int FSM_NB_End(pStateMachine machine, SM_VAR *sm_var);

Transition NB_Fail2Reset = {
    sNB_RESET,
    FSM_NB_Reset
};

Transition NB_Reset2None = {
    sNB_NONE,
    FSM_NB_None
};

Transition NB_None2Init = {
    sNB_INIT,
    FSM_NB_Init
};

Transition NB_Init2ModuleInfo = {
    sNB_MODULE_INFO,
    FSM_NB_GetModuleInfo
};

Transition NB_ModuleInfo2Sign = {
    sNB_SIGN,
    FSM_NB_GetSign
};

Transition NB_Sign2UDPCreate = {
    sNB_UDP_CREATE,
    FSM_NB_CreateUDP
};

Transition NB_UDPCreate2Register = {
    sNB_UDP_REGISTER,
    FSM_NB_UDPRegister
};

//Transition NB_Register2GuyuCertificate =
//{
//  sNB_CERTIFICATE,
//  FSM_NB_WaitReceiveData
//};
//
//Transition NB_GuyuCertificate2SendData =
//{
//  sNB_UDP_SEND,
//  FSM_NB_UDPSendData
//};

//Transition NB_UDPCreate2SendData =
//{
//  sNB_UDP_SEND,
//  FSM_NB_UDPSendData
//};

Transition NB_UDPRegister2SendData = {
    sNB_UDP_SEND,
    FSM_NB_UDPSendData
};

Transition NB_UDPSendData2ReceiveData = {
    sNB_UDP_RECEIVE,
    FSM_NB_WaitReceiveData
};

Transition NB_UDPReceiveData2UDPClose = {
    sNB_UDP_CLOSE,
    FSM_NB_CloseUDP
};

//Transition NB_SendData2UDPClose =
//{
//  sNB_UDP_CLOSE,
//  FSM_NB_CloseUDP
//};

Transition NB_UDPClose2End = {
    sNB_END,
    FSM_NB_End////////////////////////////
};

//Transition NB_Sign2CoAPCreate =
//{
//  sNB_CoAP_SEVER,
//  FSM_NB_CreateUDP
//};
//
//Transition NB_CoAPCreate2SendData =
//{
//  sNB_CoAP_SEND,
//  FSM_NB_UDPSendData
//};

Transition NB_IOT_Trans_Table[FSM_STATES][FSM_CONDITIONS];

void NB_IOT_FSM_Init(void)
{
    NB_IOT_Trans_Table[sNB_NONE][0] = NB_None2Init;
    NB_IOT_Trans_Table[sNB_NONE][1] = NB_Reset2None;
    NB_IOT_Trans_Table[sNB_NONE][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_INIT][0] = NB_Init2ModuleInfo;
    NB_IOT_Trans_Table[sNB_INIT][1] = NB_None2Init;
    NB_IOT_Trans_Table[sNB_INIT][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_MODULE_INFO][0] = NB_ModuleInfo2Sign;
    NB_IOT_Trans_Table[sNB_MODULE_INFO][1] = NB_Init2ModuleInfo;
    NB_IOT_Trans_Table[sNB_MODULE_INFO][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_SIGN][0] = NB_Sign2UDPCreate;
    NB_IOT_Trans_Table[sNB_SIGN][1] = NB_ModuleInfo2Sign;
    NB_IOT_Trans_Table[sNB_SIGN][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_UDP_CREATE][0] = NB_UDPCreate2Register;
    NB_IOT_Trans_Table[sNB_UDP_CREATE][1] = NB_Sign2UDPCreate;
    NB_IOT_Trans_Table[sNB_UDP_CREATE][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_UDP_CLOSE][0] = NB_UDPClose2End;
    NB_IOT_Trans_Table[sNB_UDP_CLOSE][1] = NB_UDPReceiveData2UDPClose;
    NB_IOT_Trans_Table[sNB_UDP_CLOSE][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_UDP_REGISTER][0] = NB_UDPRegister2SendData;
    NB_IOT_Trans_Table[sNB_UDP_REGISTER][1] = NB_UDPCreate2Register;
    NB_IOT_Trans_Table[sNB_UDP_REGISTER][2] = NB_Fail2Reset;

//  NB_IOT_Trans_Table[sNB_CERTIFICATE][0] = NB_GuyuCertificate2SendData;
//  NB_IOT_Trans_Table[sNB_CERTIFICATE][1] = NB_Register2GuyuCertificate;
//  NB_IOT_Trans_Table[sNB_CERTIFICATE][2] = NB_GuyuCertificate2SendData;

    NB_IOT_Trans_Table[sNB_UDP_SEND][0] = NB_UDPSendData2ReceiveData;
    NB_IOT_Trans_Table[sNB_UDP_SEND][1] = NB_UDPRegister2SendData;
    NB_IOT_Trans_Table[sNB_UDP_SEND][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_UDP_RECEIVE][0] = NB_UDPReceiveData2UDPClose;
    NB_IOT_Trans_Table[sNB_UDP_RECEIVE][1] = NB_UDPSendData2ReceiveData;
    NB_IOT_Trans_Table[sNB_UDP_RECEIVE][2] = NB_UDPReceiveData2UDPClose;

    NB_IOT_Trans_Table[sNB_RESET][0] = NB_Reset2None;
    NB_IOT_Trans_Table[sNB_RESET][1] = NB_Fail2Reset;
    NB_IOT_Trans_Table[sNB_RESET][2] = NB_Fail2Reset;

    NB_IOT_Trans_Table[sNB_END][0] = NB_UDPClose2End;
    NB_IOT_Trans_Table[sNB_END][1] = NB_UDPClose2End;
    NB_IOT_Trans_Table[sNB_END][2] = NB_Fail2Reset;
}

int t_FSM_Example(int argc, char **argv)
{

    return PASS;
}

#endif

