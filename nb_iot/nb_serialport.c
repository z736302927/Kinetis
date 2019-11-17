#include "nb_iot/nb_serialport.h"
#include "nb_iot/nb_bc95.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in structure SerialPort1, design the function you need and initialize it in the main function.
  * @step 3:  You need to provide an ms timer for function SerialPort_GetTick.
  * @step 4:  For receiving data, you need to put it in like a ring, using interrupts or DMA.
  * @step 5:  Finally, you can process the received data in function NB_InterRxCallback.Note: maximum 256 bytes received.
  */

#include "usart.h"
#include "string.h"
#include "bsp_serialport/bsp_serialport.h"

#define DEBUG
#include "idebug/idebug.h"

#define NB_UART_printf    p_dbg

#define NB_PORT_RXBUFFER_SIZE    256

static void NB_IOT_UART_Buffer_FindTail(void);
static void NB_IOT_UART_Extract_ValidData(void);

NB_RxCallback  NB_InterRxCallback = NULL;

extern SerialPort_TypeDef SerialPort1;

uint8_t NB_Port_RxBuffer[NB_PORT_RXBUFFER_SIZE];
uint8_t NB_Port_RxBuffer_pHead = 0;
uint8_t NB_Port_RxBuffer_pTail = 0;
uint8_t NB_Port_Tx_Done = 0;
uint32_t NB_Port_RxBeginTime = 0;
uint32_t NB_Port_RxCurrentTime = 0;
uint32_t NB_Port_RxTimeDiff = 0;


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == LPUART1)
  {
    NB_Port_Tx_Done = 1;
  }
  if(huart->Instance == USART1)
  {
    SerialPort1.Tx_SendDone = 1;
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == LPUART1)
  {
    HAL_UART_Receive_IT(&hlpuart1, NB_Port_RxBuffer, sizeof(NB_Port_RxBuffer)); 
  }
  if(huart->Instance == USART1)
  {
//    HAL_UART_Receive_IT(&huart1, (uint8_t*)SerialPort1.RxBuffer, SerialPort1.RxBuffer_Size); 
  }
}

void NB_IOT_UART_Buffer_Reset(void)
{
  NB_Port_RxBuffer_pHead = 0;
  NB_Port_RxBuffer_pTail = 0;
  NB_Port_Tx_Done = 0;
  memset(NB_Port_RxBuffer, 0xFF, sizeof(NB_Port_RxBuffer));
}

void NB_IOT_UART_Open(NB_RxCallback cb, uint32_t baud)
{
  NB_InterRxCallback = cb;
  memset(NB_Port_RxBuffer, 0xFF, sizeof(NB_Port_RxBuffer));
  HAL_UART_Receive_IT(&hlpuart1, NB_Port_RxBuffer, sizeof(NB_Port_RxBuffer));  
}

void NB_IOT_UART_Close(void)
{
  HAL_UART_MspDeInit(&hlpuart1); 
}

void NB_IOT_UART_Send(uint8_t* pdata,uint16_t len)
{
  HAL_UART_Transmit_IT(&hlpuart1, pdata, len);
//  NB_UART_printf("%s", pdata);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void NB_IOT_UART_Receive(void)
{
  uint8_t rxdata_tmptail = NB_Port_RxBuffer_pTail;
  uint8_t wait_rx_done = 0;
  
  if(NB_Port_Tx_Done == 1)
  {
    NB_IOT_UART_Buffer_FindTail();
    
    if(NB_Port_RxBuffer_pHead != NB_Port_RxBuffer_pTail)
    {
      if(rxdata_tmptail != NB_Port_RxBuffer_pTail)
      {
        NB_Port_RxBeginTime = HAL_GetTick();
      }
      else
      {
        NB_Port_RxCurrentTime = HAL_GetTick();
        NB_Port_RxTimeDiff = NB_Port_RxCurrentTime >= NB_Port_RxBeginTime ? 
                                                 NB_Port_RxCurrentTime - NB_Port_RxBeginTime : 
                                                 NB_Port_RxCurrentTime + UINT32_MAX - NB_Port_RxBeginTime;
        if(NB_Port_RxTimeDiff > 10)
        {
          wait_rx_done = 1;
        }
      }
    }
 
    if(wait_rx_done == 1)
    {
      NB_Port_Tx_Done = 0;
      
      NB_IOT_UART_Extract_ValidData();
    }
  }
}

static void NB_IOT_UART_Buffer_FindTail(void)
{
  while(NB_Port_RxBuffer[NB_Port_RxBuffer_pTail] != 0xFF)
  {
    if(NB_Port_RxBuffer_pTail == NB_PORT_RXBUFFER_SIZE - 1)
    {
      NB_Port_RxBuffer_pTail = 0;
    }
    else
    {
      NB_Port_RxBuffer_pTail++;
    }
  }
}

static void NB_IOT_UART_Extract_ValidData(void)
{
  uint8_t rxdata_temp[NB_PORT_RXBUFFER_SIZE];
  uint8_t rxdata_size = 0;
  
  memset(rxdata_temp, 0, sizeof(rxdata_temp));
  if(NB_Port_RxBuffer_pTail > NB_Port_RxBuffer_pHead)
  {
    rxdata_size = NB_Port_RxBuffer_pTail - NB_Port_RxBuffer_pHead;
    memcpy(rxdata_temp, &NB_Port_RxBuffer[NB_Port_RxBuffer_pHead], rxdata_size);
  }
  else if(NB_Port_RxBuffer_pTail < NB_Port_RxBuffer_pHead)
  {
    rxdata_size = NB_PORT_RXBUFFER_SIZE - NB_Port_RxBuffer_pHead + NB_Port_RxBuffer_pTail;
    memcpy(rxdata_temp, &NB_Port_RxBuffer[NB_Port_RxBuffer_pHead], NB_PORT_RXBUFFER_SIZE - NB_Port_RxBuffer_pHead);
    memcpy(&rxdata_temp[NB_PORT_RXBUFFER_SIZE - NB_Port_RxBuffer_pHead], &NB_Port_RxBuffer[0], NB_Port_RxBuffer_pTail);
  }
  else
  {
    rxdata_size = 0;
  }
  
  if(rxdata_size != 0)
  {
//    NB_UART_printf("%s",rxdata_temp);
    NB_InterRxCallback((char*)rxdata_temp, rxdata_size);
    
    memset(NB_Port_RxBuffer, 0xFF, sizeof(NB_Port_RxBuffer));
    NB_Port_RxBuffer_pHead = NB_Port_RxBuffer_pTail;
  }
}