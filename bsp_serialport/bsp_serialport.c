#include "peripheral/bsp_serialport.h"


/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in structure SerialPort_1, design the function you need and initialize it in the main function.
  * @step 3:  You need to provide an ms timer for function SerialPort_GetTick.
  * @step 4:  For receiving data, you need to put it in like a ring, using interrupts or DMA.
  * @step 5:  Finally, you can process the received data in function SerialPort_RxBuffer_Process.Note: maximum 256 bytes received.
  */

#include "usart.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"
#include "protocol/hydrology.h"

#define DEBUG
#include "idebug.h"

#define SerialPort_printf    p_dbg

SerialPort_TypeDef SerialPort_1;
SerialPort_TypeDef SerialPort_2;
SerialPort_TypeDef SerialPort_3;

int SerialPort_RxDataFlag = 0;

uint32_t SerialPort_GetTick(void)
{
  return HAL_GetTick();
}

void SerialPort_SetRxState(int state)
{
  SerialPort_RxDataFlag = state;
}

int SerialPort_ReadRxState(void)
{
  return SerialPort_RxDataFlag;
}

void SerialPort_RxBuffer_Init(void)
{
  memset(&SerialPort_1, 0, sizeof(SerialPort_TypeDef));
}

uint8_t SerialPort_Alloc(SerialPort_TypeDef *Instance, uint32_t Interval, uint16_t Size)
{
  Instance->RxBuffer = (uint16_t *)malloc(Size * sizeof(uint16_t));
  if(Instance->RxBuffer == NULL)
  {
    SerialPort_printf("SerialPort malloc failed !");
    return false;
  }
  memset(Instance->RxBuffer, 0xFF, Instance->RxBuffer_Size * sizeof(uint16_t));
  Instance->RxBuffer_Size = Size;
  Instance->RxScanInterval = Interval;
  
  return true;
}

void SerialPort_Free(SerialPort_TypeDef *Instance)
{
  memset(Instance, 0, sizeof(SerialPort_TypeDef));
  free(Instance->RxBuffer);
  Instance->RxBuffer = NULL;
}

uint8_t SerialPort_Open(SerialPort_TypeDef *Instance, uint32_t Interval, uint16_t Size)
{
  if(SerialPort_Alloc(Instance, Interval, Size) == false)
  {
    return false;
  }
  
  if(Instance == &SerialPort_1)
  {
    HAL_UART_Receive_DMA(&huart4, (uint8_t*)Instance->RxBuffer, Instance->RxBuffer_Size);
  } 
  else if(Instance == &SerialPort_2)
  {
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)Instance->RxBuffer, Instance->RxBuffer_Size);
  }
  else if(Instance == &SerialPort_3)
  {
    HAL_UART_Receive_DMA(&huart3, (uint8_t*)Instance->RxBuffer, Instance->RxBuffer_Size);
  }
  
  return true;
}

void SerialPort_Close(SerialPort_TypeDef *Instance)
{
  SerialPort_Free(Instance);
  
  if(Instance == &SerialPort_1)
  {
    HAL_UART_MspDeInit(&huart4);
  } 
  else if(Instance == &SerialPort_2)
  {
    HAL_UART_MspDeInit(&huart1);
  }
  else if(Instance == &SerialPort_3)
  {
    HAL_UART_MspDeInit(&huart3);
  }
}

void SerialPort_Send(SerialPort_TypeDef *Instance, uint8_t* pData, uint16_t Len)
{
  if(Instance == &SerialPort_1)
  {
    HAL_UART_Transmit_IT(&huart4, pData, Len);
  } 
  else if(Instance == &SerialPort_2)
  {
    HAL_UART_Transmit_IT(&huart1, pData, Len);
  }
  else if(Instance == &SerialPort_3)
  {
    HAL_UART_Transmit_IT(&huart3, pData, Len);
  }
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void SerialPort_RxBuffer_FindTail(SerialPort_TypeDef *Instance)
{
  uint16_t buffer_cnt = 0;
  
  while((Instance->RxBuffer[Instance->Rx_pTail] >> 8) != 0xFF)
  {
    buffer_cnt++;
    
    if(Instance->Rx_pTail == (Instance->RxBuffer_Size - 1))
    {
      Instance->Rx_pTail = 0;
    }
    else
    {
      Instance->Rx_pTail++;
    }
    
    if(buffer_cnt == (Instance->RxBuffer_Size - 1))
    {
      break;
    }
  }
}

void SerialPort_Extract_ValidData(SerialPort_TypeDef *Instance, uint8_t* pData, uint16_t* Len)
{
  uint8_t rxdata_size = 0;
  uint16_t i, j;
  
  if(Instance->Rx_pTail > Instance->Rx_pHead)
  {
    rxdata_size = Instance->Rx_pTail - Instance->Rx_pHead;
    
    pData = (uint8_t*)malloc(rxdata_size);
    if(pData == NULL)
    {
      return;
    }
    for(i = Instance->Rx_pHead, j = 0;i < Instance->Rx_pTail;i++, j++)
    {
      pData[j] = Instance->RxBuffer[i];
    }
    memset(&Instance->RxBuffer[Instance->Rx_pHead], 0xFF, rxdata_size * sizeof(uint16_t));
  }
  else if(Instance->Rx_pTail < Instance->Rx_pHead)
  {
    rxdata_size = Instance->RxBuffer_Size - Instance->Rx_pHead + Instance->Rx_pTail;
    
    pData = (uint8_t*)malloc(rxdata_size);
    if(pData == NULL)
    {
      return;
    }
    for(i = Instance->Rx_pHead, j = 0;i < Instance->RxBuffer_Size;i++, j++)
    {
      pData[j] = Instance->RxBuffer[i];
    }
    for(i = 0;i < Instance->Rx_pTail;i++, j++)
    {
      pData[j] = Instance->RxBuffer[i];
    }
    memset(&Instance->RxBuffer[Instance->Rx_pHead], 0xFF, (Instance->RxBuffer_Size - Instance->Rx_pHead) * sizeof(uint16_t));
    memset(&Instance->RxBuffer[0], 0xFF, Instance->Rx_pTail * sizeof(uint16_t));
  }
  else
  {
    rxdata_size = 0;
    *Len = 0;
  }
  
  if(rxdata_size != 0)
  {
    *Len = rxdata_size;
    Instance->Rx_pHead = Instance->Rx_pTail;
    SerialPort_SetRxState(1);
  }
}

void SerialPort_Receive(SerialPort_TypeDef *Instance, uint8_t* pData, uint16_t* Len)
{
  uint8_t rxdata_tmptail = Instance->Rx_pTail;
  uint8_t wait_rx_done = 0;
  
  SerialPort_RxBuffer_FindTail(Instance);
  
  if(Instance->Rx_pHead != Instance->Rx_pTail)
  {
    if(rxdata_tmptail != Instance->Rx_pTail)
    {
      Instance->RxBeginTime = SerialPort_GetTick();
    }
    else
    {
      Instance->RxCurrentTime = SerialPort_GetTick();
      Instance->RxTimeDiff = Instance->RxCurrentTime >= Instance->RxBeginTime ? 
                                               Instance->RxCurrentTime - Instance->RxBeginTime : 
                                               Instance->RxCurrentTime + UINT32_MAX - Instance->RxBeginTime;
      if(Instance->RxTimeDiff > Instance->RxScanInterval)
      {
        wait_rx_done = 1;
      }
    }
  }

  if(wait_rx_done == 1)
  {
    Instance->Tx_SendDone = 0;
    SerialPort_Extract_ValidData(Instance, pData, Len);
  }
}

