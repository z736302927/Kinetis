#include "nb_iot/nb_timer.h"
#include "nb_iot/nb_app.h"
#include "nb_iot/nb_board.h"
#include "nb_iot/nb_bc95.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  You should call the function NB_IOT_Init in the main function to initialize the NB module.
  * @step 3:  You should implement the struct members of NB_UartPort_Inst,NB_Timer_Inst and HWAtrrs_Object.
  * @step 4:  After each state is complete, NB_IOT_ResponseCallback is called.Including send and receive, you can achieve custom functions.
  * @step 5:  Finally, you only need to call NB_IOT_SendData to send and receive data to the cloud platform.
  * @step 6:  Note that you can use NB_IOT_Turnoff_Pipe to decide whether to close the channel or not.
  */
#include "bsp_fsm/bsp_fsm.h"
#include "string.h"
#include "usart.h"
#include "iwdg.h"
#include "hydrology-protocol/message.h"

#define DEBUG
#include "idebug/idebug.h"


#define NB_printf    p_dbg

#define NB_Communication_Mode   1

int NB_IOT_ResponseCallback(NB_MessageTypeDef, int ,char*);

extern void NB_IOT_UART_Open(NB_RxCallback cb, uint32_t baud);
extern void NB_IOT_UART_Send(uint8_t* pdata,uint16_t len);
extern void NB_IOT_UART_Close(void);
extern void NB_IOT_UART_Buffer_Reset(void);

char* NB_UserDataPacket = NULL;
int NB_UserDataLength = 0;
uint8_t NB_MessageComing = 0;
uint8_t NB_Turnoff_Pipe = 1;

uint8_t NB_Response_Result = FALSE;
uint8_t NB_Response_Done = FALSE;
uint8_t* NB_Module_IMEI = NULL;

NB_SerilPortTypeDef  NB_UartPort_Inst = 
{
  .Open = NB_IOT_UART_Open,
  .Send = NB_IOT_UART_Send,
  .Close = NB_IOT_UART_Close
};

NB_TimerTypeDef NB_Timer_Inst = 
{
  .Init = NB_IOT_SetTim,
  .Start = NB_IOT_StartTim,
  .Stop = NB_IOT_StopTim
};

NB_HW_Object  HWAtrrs_Object = 
{
  .Baudrate = 9600,
  .Uart = &NB_UartPort_Inst,
  .Timer = &NB_Timer_Inst
};

NB_ConfigTypeDef  NB_Config_Inst = 
{
  .Module = NULL,
  .Object = (void*)&HWAtrrs_Object,
  .AppReceiveCallback = NB_IOT_ResponseCallback,
  .Log = NULL
};

int NB_IOT_ResponseCallback(NB_MessageTypeDef types, int len, char* msg)
{
  switch(types)
  {
    case MSG_INIT:
    {
      NB_printf("INIT = %s", msg);
      NB_Response_Done = TRUE;

      if(*msg == 'S')
      {
        NB_printf("NET = ON");
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;
    
    case MSG_IMSI:
    {
      NB_printf("IMSI = %s", msg);
      NB_Response_Done = TRUE;
      NB_Response_Result = TRUE;
    }
    break;
    
    case MSG_REG:
    {
      NB_printf("NET = %s", (*msg) == 1 ?"ON":"0FF");
      NB_Response_Done = TRUE;

      if((*msg) == 1)
      {
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;
    
    case MSG_SIGN:
    {
      NB_printf("%sdbm", msg);
      NB_Response_Done = TRUE;
      NB_Response_Result = TRUE;
    }
    break;
    
    case MSG_MODULE_INFO:
    {
      NB_printf("Minfo = %s", msg);
      NB_Response_Done = TRUE;
      NB_Response_Result = TRUE;
    }
    break;
    
    case MSG_MID:
    {
      NB_printf("MID = %s", msg);
    }
    break;
    
    case MSG_MMODEL:
    {
      NB_printf("Model = %s", msg);
    }
    break;
    
    case MSG_MREV:
    {
      NB_printf("REV = %s", msg);
    }
    break;
    
    case MSG_BAND:
    {
      NB_printf("Freq = %s", msg);
    }
    break;
    
    case MSG_IMEI:
    {
      NB_printf("IMEI = %s", msg);
      NB_Module_IMEI = (uint8_t*)msg;
      NB_Response_Done = TRUE;
      NB_Response_Result = TRUE;
    }
    break;
    
    case MSG_UDP_CREATE:
    {
      NB_printf("UDP_CR = %s", msg);
      NB_Response_Done = TRUE;
      
      if(*msg == 'S')
      {
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;
    
    case MSG_UDP_CLOSE:
    {
      NB_printf("UDP_CL = %s", msg);
      NB_Response_Done = TRUE;

      if(*msg == 'S')
      {
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;
    
    case MSG_UDP_SEND:
    {
      NB_printf("UDP_SEND = %s", msg);
      NB_Response_Done = TRUE;

      if(*msg == 'S')
      {
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;
    
    case MSG_UDP_RECE:
    {
      NB_printf("UDP_RECE = %s", msg);
      p_hex(msg,  len);
      NB_Response_Done = TRUE;

      if(*msg == 'F')
      {
        NB_Response_Result = FALSE;
      }
      else
      {
        NB_Response_Result = TRUE;
        if(len > 3)
        {
          hydrologyProcessReceieve((char*)msg, len);
        }
      }
    }
    break;
    
    case MSG_COAP:
    {
      NB_printf("COAP = %s", msg);
      NB_Response_Done = TRUE;

      if(*msg == 'S')
      {
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;
    
    case MSG_COAP_SEND:
    {
      NB_printf("COAP_SENT = %s", msg);
      NB_Response_Done = TRUE;

      if(*msg == 'S')
      {
        NB_Response_Result = TRUE;
      }
      else
      {
        NB_Response_Result = FALSE;
      }
    }
    break;

    case MSG_COAP_RECE:
    {
      NB_printf("COAP_RECE = ");
      p_hex(msg,  len);
      NB_Response_Done = TRUE;

      if(*msg == 'F')
      {
        NB_Response_Result = FALSE;
      }
      else
      {
        NB_Response_Result = TRUE;
        if(len > 3)
        {
          hydrologyProcessReceieve((char*)msg, len);
        }
      }
    }
    break;

    default :break;
  }

  return 0;
}

void NB_IOT_Init(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
  
  NBModule_open(&NB_Config_Inst);
  
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int NB_IOT_ProcessRespond(void)
{
  uint32_t begintime;
  uint32_t currenttime;
  uint32_t timediff;
  
  begintime = HAL_GetTick();
  
  while(1)
  {
    NB_IOT_UART_Receive();
    NBModule_Main(&NB_Config_Inst);
    NB_IOT_PollTim();
    if(NB_Response_Done == TRUE)
    {
      NB_Response_Done = FALSE;
      
      if(NB_Response_Result == TRUE)
      {
        NB_Response_Result = FALSE;
        
        return TRUE;
      }
      else
      {
        return FALSE;
      }
    }
    else
    {
      currenttime = HAL_GetTick();
      timediff = currenttime >= begintime ? 
                                               currenttime - begintime : 
                                               currenttime + UINT32_MAX - begintime;
      if(timediff > 5000) /* 5s */
      {
        return FALSE;
      }
    }
  }
}

int NB_IOT_JudgeArrivalofMsg(void)
{
  uint32_t begintime;
  uint32_t currenttime;
  uint32_t timediff;
  
  begintime = HAL_GetTick();
  
  while(1)
  {
    HAL_IWDG_Refresh(&hiwdg);
    if(NB_MessageComing == 1)
    {
      NB_MessageComing = 0;
      
      return TRUE;
    }
    else
    {
      currenttime = HAL_GetTick();
      timediff = currenttime >= begintime ? 
                                               currenttime - begintime : 
                                               currenttime + UINT32_MAX - begintime;
      if(timediff > 10000) /* 10s */
      {
        NB_printf("No data came !");
        return FALSE;
      }
    }
  }
}

int NB_IOT_Shutdown(void)
{
  int retValue = FALSE;
  
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_Delay(1000);
  
  if((GPIOA->IDR  & GPIO_PIN_0) != 0x00u)
  {
    retValue = FALSE;
  }
  else
  {
    retValue = TRUE;
  }
  
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
  
  NB_Response_Result = FALSE;
  NB_Response_Done = FALSE;
  NB_IOT_UART_Buffer_Reset();
  
//  retValue = NBModule_Reboot(&NB_Config_Inst);
//  HAL_Delay(3000);
  
  return retValue;
}

void NB_IOT_Turnoff_Pipe(uint8_t onoff)
{
  NB_Turnoff_Pipe = onoff; 
}

void NB_IOT_SendData(char* pdata, int len)
{
  State state;
  StateMachine machine;
  SM_VAR var;

  if(NB_Turnoff_Pipe == 1)
  {
    machine.current = sNB_NONE;
  }
  else
  {
    machine.current = sNB_UDP_REGISTER;
  }
  var._repeats = 0;
  var._condition = cOK;
  
  NB_UserDataPacket = pdata;
  NB_UserDataLength = len;

  while(machine.current != sNB_END)
  {
    state = FSM_Step(&machine,&var);
    
    if(NB_Turnoff_Pipe == 0 && machine.current == sNB_UDP_CLOSE)
    {
      break;
    }
  }
  
  if((state == sNB_END) && (var._condition == cOK))
  {
    NB_printf("The data was successfully sent through the nb-iot device to the server.");
  }
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

int FSM_NB_None(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;

  _retValue = TRUE;
  
  sm_var->_condition = cOK;
  NB_printf("NB-IOT Device will start sending data.");
  
  return _retValue;
}

int FSM_NB_Init(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;
  
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  NBModule_Init(&NB_Config_Inst);
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("NB-IOT Device initialization successful.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("NB-IOT Device initialization failed.");
  }

  return _retValue;
}

int FSM_NB_GetSign(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;

  NBModule_Sign(&NB_Config_Inst);
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("Signal detected in this area.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("No signal detected in this area.");
  }

  return _retValue;
}

int FSM_NB_GetModuleInfo(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;

  NBModule_Info(&NB_Config_Inst);
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("Device information obtained successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("Failed to get device information.");
  }

  return _retValue;
}

int FSM_NB_CreateUDP(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;

#if NB_Communication_Mode 
  NBModule_CreateUDP(&NB_Config_Inst);
#else
  NBModule_CoAPServer(&NB_Config_Inst,1,NULL);
#endif
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("UDP Communication created successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("Failed to create UDP communication");
  }

  return _retValue;
}

int FSM_NB_CloseUDP(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;

  NBModule_CloseUDP(&NB_Config_Inst);
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("UDP Communication closed successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("Shutdown UDP communication failed.");
  }

  return _retValue;
}

int FSM_NB_UDPRegister(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;
  char regPacket[30];
  uint8_t msgLen = 0;
  
  msgLen = sprintf(regPacket, "ep=%s&pw=736302", "1RFN7HGAL3VKMD95");//NB_Module_IMEI
  regPacket[msgLen] = 0;
  
  //char* regPacket = "ep=863703036005069&pw=123456";
  
#if NB_Communication_Mode 
  NBModule_SendData(&NB_Config_Inst, msgLen, regPacket);
#else
  NBModule_CoAPSendMsg(&NB_Config_Inst, msgLen, regPacket);
#endif
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("Guyu server registered successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("Guyu server registration failed.");
  }

  return _retValue;
}

int FSM_NB_UDPSendData(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;
  
#if NB_Communication_Mode 
  NBModule_SendData(&NB_Config_Inst, NB_UserDataLength, NB_UserDataPacket);
#else
  NBModule_CoAPSendMsg(&NB_Config_Inst, NB_UserDataLength, NB_UserDataPacket);
#endif
  _retValue = NB_IOT_ProcessRespond();
  
  if(_retValue == TRUE)/////////////////////////////
  {
    sm_var->_condition = cOK;
    NB_printf("Data sent successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("Data transmission failure.");
  }

  return _retValue;
}

int FSM_NB_WaitReceiveData(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;
  
  if(NB_IOT_JudgeArrivalofMsg() == TRUE)
  {
#if NB_Communication_Mode 
    NBModule_ReceiveData(&NB_Config_Inst);
#else
    NBModule_CoAPReceMsg(&NB_Config_Inst);
#endif
    _retValue = NB_IOT_ProcessRespond();
  }
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("Data received successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("Data received failure.");
  }

  return _retValue;
}

int FSM_NB_Reset(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;
  
  _retValue = NB_IOT_Shutdown();
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("NB Device reset successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("NB Device reset failure.");
  }

  return _retValue;
}

int FSM_NB_End(pStateMachine machine, SM_VAR * sm_var)
{
  int _retValue = FALSE;
  
  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
  _retValue = TRUE;
  
  if(_retValue == TRUE)
  {
    sm_var->_condition = cOK;
    NB_printf("The state machine entered the final state successfully.");
    
    return _retValue;
  }
  else
  {
    sm_var->_condition = cERROR_REPEATS_S3;
    NB_printf("The state machine failed to enter the final state.");
  }

  return _retValue;
}