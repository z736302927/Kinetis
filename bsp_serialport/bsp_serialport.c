#include "bsp_serialport/bsp_serialport.h"


/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in structure SerialPort1, design the function you need and initialize it in the main function.
  * @step 3:  You need to provide an ms timer for function SerialPort_GetTick.
  * @step 4:  For receiving data, you need to put it in like a ring, using interrupts or DMA.
  * @step 5:  Finally, you can process the received data in function SerialPort_RxBuffer_Process.Note: maximum 256 bytes received.
  */

#include "usart.h"
#include "string.h"
#include "stdlib.h"
#include "hydrology-protocol/message.h"

#define DEBUG
#include "idebug/idebug.h"

#define SerialPort_printf    p_dbg

#define SERIALPORT_RXBUFFER_TYPE    uint16_t
#define SERIALPORT_RXBUFFER_SIZE    256

void SerialPort_RxBuffer_FindTail(void);
void SerialPort_Extract_ValidData(void);

SerialPort_TypeDef SerialPort1;

void SerialPort_RxBuffer_Process(uint8_t* pdata, uint16_t len)
{
//  HAL_UART_Transmit(&huart1, pdata, len, 0xFFFF);
//  p_hex(pdata, len);
  hydrologySetMsgSrc(MsgFormClient);
  hydrologyProcessReceieve((char*)pdata, len);
}

uint32_t SerialPort_GetTick(void)
{
  return HAL_GetTick();
}

void SerialPort_RxBuffer_Init(void)
{
  SerialPort1.Rx_pHead = 0;
  SerialPort1.Rx_pTail = 0;
  SerialPort1.Tx_SendDone = 0;
}

void SerialPort_Open(void)
{
  SerialPort1.RxScanInterval = 10;
  SerialPort1.RxBuffer_Size = SERIALPORT_RXBUFFER_SIZE;
//  SerialPort1.RxBuffer = (uint16_t*)malloc(SerialPort1.RxBuffer_Size * sizeof(SERIALPORT_RXBUFFER_TYPE));
  if(SerialPort1.RxBuffer == NULL)
  {
    SerialPort_printf("SerialPort malloc failed !");
    return;
  }
  
  memset(SerialPort1.RxBuffer, 0xFF, SerialPort1.RxBuffer_Size * sizeof(SERIALPORT_RXBUFFER_TYPE));
  HAL_UART_Receive_DMA(&huart1, (uint8_t*)SerialPort1.RxBuffer, SerialPort1.RxBuffer_Size);  
}

void SerialPort_Close(void)
{
//  free(SerialPort1.RxBuffer);
  HAL_UART_MspDeInit(&huart1); 
}

void SerialPort_Send(uint8_t* pdata, uint16_t len)
{
  HAL_UART_Transmit_IT(&huart1, pdata, len);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void SerialPort_Receive(void)
{
  uint8_t rxdata_tmptail = SerialPort1.Rx_pTail;
  uint8_t wait_rx_done = 0;
  
  SerialPort_RxBuffer_FindTail();
  
  if(SerialPort1.Rx_pHead != SerialPort1.Rx_pTail)
  {
    if(rxdata_tmptail != SerialPort1.Rx_pTail)
    {
      SerialPort1.RxBeginTime = SerialPort_GetTick();
    }
    else
    {
      SerialPort1.RxCurrentTime = SerialPort_GetTick();
      SerialPort1.RxTimeDiff = SerialPort1.RxCurrentTime >= SerialPort1.RxBeginTime ? 
                                               SerialPort1.RxCurrentTime - SerialPort1.RxBeginTime : 
                                               SerialPort1.RxCurrentTime + UINT32_MAX - SerialPort1.RxBeginTime;
      if(SerialPort1.RxTimeDiff > SerialPort1.RxScanInterval)
      {
        wait_rx_done = 1;
      }
    }
  }

  if(wait_rx_done == 1)
  {
    SerialPort1.Tx_SendDone = 0;
    SerialPort_Extract_ValidData();
  }
}

void SerialPort_RxBuffer_FindTail(void)
{
  uint16_t buffer_cnt = 0;
  
  while((SerialPort1.RxBuffer[SerialPort1.Rx_pTail] >> 8) != 0xFF)
  {
    buffer_cnt++;
    
    if(SerialPort1.Rx_pTail == SERIALPORT_RXBUFFER_SIZE - 1)
    {
      SerialPort1.Rx_pTail = 0;
    }
    else
    {
      SerialPort1.Rx_pTail++;
    }
    
    if(buffer_cnt == SERIALPORT_RXBUFFER_SIZE - 1)
    {
      break;
    }
  }
}

void SerialPort_Extract_ValidData(void)
{
  uint8_t rxdata_tmp[SERIALPORT_RXBUFFER_SIZE];
  uint8_t rxdata_size = 0;
  uint16_t i, j;
  
  memset(rxdata_tmp, 0, sizeof(rxdata_tmp));
  if(SerialPort1.Rx_pTail > SerialPort1.Rx_pHead)
  {
    rxdata_size = SerialPort1.Rx_pTail - SerialPort1.Rx_pHead;
    
    for(i = SerialPort1.Rx_pHead, j = 0;i < SerialPort1.Rx_pTail;i++, j++)
    {
      rxdata_tmp[j] = SerialPort1.RxBuffer[i];
    }
    memset(&SerialPort1.RxBuffer[SerialPort1.Rx_pHead], 0xFF, rxdata_size * sizeof(SERIALPORT_RXBUFFER_TYPE));
  }
  else if(SerialPort1.Rx_pTail < SerialPort1.Rx_pHead)
  {
    rxdata_size = SERIALPORT_RXBUFFER_SIZE - SerialPort1.Rx_pHead + SerialPort1.Rx_pTail;
    
    for(i = SerialPort1.Rx_pHead, j = 0;i < SERIALPORT_RXBUFFER_SIZE;i++, j++)
    {
      rxdata_tmp[j] = SerialPort1.RxBuffer[i];
    }
    for(i = 0;i < SerialPort1.Rx_pTail;i++, j++)
    {
      rxdata_tmp[j] = SerialPort1.RxBuffer[i];
    }
    memset(&SerialPort1.RxBuffer[SerialPort1.Rx_pHead], 0xFF, (SERIALPORT_RXBUFFER_SIZE - SerialPort1.Rx_pHead) * sizeof(SERIALPORT_RXBUFFER_TYPE));
    memset(&SerialPort1.RxBuffer[0], 0xFF, SerialPort1.Rx_pTail * sizeof(SERIALPORT_RXBUFFER_TYPE));
  }
  else
  {
    rxdata_size = 0;
  }
  
  if(rxdata_size != 0)
  {
    SerialPort_RxBuffer_Process(rxdata_tmp, rxdata_size);
    
    SerialPort1.Rx_pHead = SerialPort1.Rx_pTail;
  }
}

