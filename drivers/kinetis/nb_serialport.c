#include "kinetis/nb_serial-port.h"
#include "kinetis/nb_bc95.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in structure NB_IOT_UART1, design the function you need and initialize it in the main function.
  * @step 3:  You need to provide an ms timer for function NB_IOT_UART_GetTick.
  * @step 4:  For receiving data, you need to put it in like a ring, using interrupts or DMA.
  * @step 5:  Finally, you can process the received data in function NB_InterRxCallback.Note: maximum 256 bytes received.
  */

#include "usart.h"
#include "string.h"
#include "stdlib.h"
#include "kinetis/serial-port.h"

#define DEBUG
#include "kinetis/idebug.h"

#define NB_IOT_UART_printf    p_dbg

#define NB_IOT_UART_RXBUFFER_TYPE    u16
#define NB_IOT_UART_RXBUFFER_SIZE    256

void NB_IOT_UART_RxBuffer_FindTail(void);
void NB_IOT_UART_Extract_ValidData(void);

NB_RxCallback  NB_InterRxCallback = NULL;
struct serial_port NB_IOT_UART1;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)
        NB_IOT_UART1.Tx_SendDone = 1;

    if (huart->Instance == USART1) {

    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
//    HAL_UART_Receive_IT(&huart1, (u8*)NB_IOT_UART1.RxBuffer, NB_IOT_UART1.RxBuffer_Size);
    }

    if (huart->Instance == UART4) {
//    HAL_UART_Receive_IT(&huart4, (u8*)NB_IOT_UART1.RxBuffer, NB_IOT_UART1.RxBuffer_Size);
    }
}

void NB_IOT_UART_RxBuffer_Process(u8 *pdata, u16 len)
{
    NB_InterRxCallback((char *)pdata, len);
//  HAL_UART_Transmit(&huart3, pdata, len, 0xFFFF);
//  p_hex(pdata, len);
}

u32 NB_IOT_UART_GetTick(void)
{
    return basic_timer_get_ms();
}

void NB_IOT_UART_RxBuffer_Init(void)
{
    NB_IOT_UART1.Rx_pHead = 0;
    NB_IOT_UART1.Rx_pTail = 0;
    NB_IOT_UART1.Tx_SendDone = 0;
}

void NB_IOT_UART_Open(NB_RxCallback cb, u32 baud)
{
    NB_InterRxCallback = cb;
    NB_IOT_UART1.RxScanInterval = 10;
    NB_IOT_UART1.RxBuffer_Size = NB_IOT_UART_RXBUFFER_SIZE;
    NB_IOT_UART1.RxBuffer = (u16 *)kmalloc(NB_IOT_UART1.RxBuffer_Size * sizeof(NB_IOT_UART_RXBUFFER_TYPE), __GFP_ZERO);

    if (NB_IOT_UART1.RxBuffer == NULL) {
        NB_IOT_UART_printf("NB_IOT_UART malloc failed !");
        return;
    }

    memset(NB_IOT_UART1.RxBuffer, 0xFF, NB_IOT_UART1.RxBuffer_Size * sizeof(NB_IOT_UART_RXBUFFER_TYPE));
//  HAL_UART_Receive_DMA(&huart4, (u8*)NB_IOT_UART1.RxBuffer, NB_IOT_UART1.RxBuffer_Size);
}

void NB_IOT_UART_Close(void)
{
//  kfree(NB_IOT_UART1.RxBuffer);
//  HAL_UART_MspDeInit(&huart4);
}

void NB_IOT_UART_Send(u8 *pdata, u16 len)
{
//  HAL_UART_Transmit_IT(&huart4, pdata, len);
//  HAL_UART_Transmit(&huart1, pdata, len, 0xFFFF);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void NB_IOT_UART_Receive(void)
{
    u8 rxdata_tmptail = NB_IOT_UART1.Rx_pTail;
    u8 wait_rx_done = 0;

//  if(NB_IOT_UART1.Tx_SendDone == 1)
//  {
    NB_IOT_UART_RxBuffer_FindTail();

    if (NB_IOT_UART1.Rx_pHead != NB_IOT_UART1.Rx_pTail) {
        if (rxdata_tmptail != NB_IOT_UART1.Rx_pTail)
            NB_IOT_UART1.RxBeginTime = NB_IOT_UART_GetTick();
        else {
            NB_IOT_UART1.RxCurrentTime = NB_IOT_UART_GetTick();
            NB_IOT_UART1.RxTimeDiff = NB_IOT_UART1.RxCurrentTime >= NB_IOT_UART1.RxBeginTime ?
                NB_IOT_UART1.RxCurrentTime - NB_IOT_UART1.RxBeginTime :
                NB_IOT_UART1.RxCurrentTime + UINT32_MAX - NB_IOT_UART1.RxBeginTime;

            if (NB_IOT_UART1.RxTimeDiff > NB_IOT_UART1.RxScanInterval)
                wait_rx_done = 1;
        }
    }

    if (wait_rx_done == 1) {
        NB_IOT_UART1.Tx_SendDone = 0;
        NB_IOT_UART_Extract_ValidData();
    }

//  }
}

void NB_IOT_UART_RxBuffer_FindTail(void)
{
    u16 buffer_cnt = 0;

    while ((NB_IOT_UART1.RxBuffer[NB_IOT_UART1.Rx_pTail] >> 8) != 0xFF) {
        buffer_cnt++;

        if (NB_IOT_UART1.Rx_pTail == NB_IOT_UART_RXBUFFER_SIZE - 1)
            NB_IOT_UART1.Rx_pTail = 0;
        else
            NB_IOT_UART1.Rx_pTail++;

        if (buffer_cnt == NB_IOT_UART_RXBUFFER_SIZE - 1)
            break;
    }
}

void NB_IOT_UART_Extract_ValidData(void)
{
    u8 rxdata_tmp[NB_IOT_UART_RXBUFFER_SIZE];
    u8 rxdata_size = 0;
    u16 i, j;

    memset(rxdata_tmp, 0, sizeof(rxdata_tmp));

    if (NB_IOT_UART1.Rx_pTail > NB_IOT_UART1.Rx_pHead) {
        rxdata_size = NB_IOT_UART1.Rx_pTail - NB_IOT_UART1.Rx_pHead;

        for (i = NB_IOT_UART1.Rx_pHead, j = 0; i < NB_IOT_UART1.Rx_pTail; i++, j++)
            rxdata_tmp[j] = NB_IOT_UART1.RxBuffer[i];

        memset(&NB_IOT_UART1.RxBuffer[NB_IOT_UART1.Rx_pHead], 0xFF, rxdata_size * sizeof(NB_IOT_UART_RXBUFFER_TYPE));
    } else if (NB_IOT_UART1.Rx_pTail < NB_IOT_UART1.Rx_pHead) {
        rxdata_size = NB_IOT_UART_RXBUFFER_SIZE - NB_IOT_UART1.Rx_pHead + NB_IOT_UART1.Rx_pTail;

        for (i = NB_IOT_UART1.Rx_pHead, j = 0; i < NB_IOT_UART_RXBUFFER_SIZE; i++, j++)
            rxdata_tmp[j] = NB_IOT_UART1.RxBuffer[i];

        for (i = 0; i < NB_IOT_UART1.Rx_pTail; i++, j++)
            rxdata_tmp[j] = NB_IOT_UART1.RxBuffer[i];

        memset(&NB_IOT_UART1.RxBuffer[NB_IOT_UART1.Rx_pHead], 0xFF, (NB_IOT_UART_RXBUFFER_SIZE - NB_IOT_UART1.Rx_pHead) * sizeof(NB_IOT_UART_RXBUFFER_TYPE));
        memset(&NB_IOT_UART1.RxBuffer[0], 0xFF, NB_IOT_UART1.Rx_pTail * sizeof(NB_IOT_UART_RXBUFFER_TYPE));
    } else
        rxdata_size = 0;

    if (rxdata_size != 0) {
        NB_IOT_UART_RxBuffer_Process(rxdata_tmp, rxdata_size);

        NB_IOT_UART1.Rx_pHead = NB_IOT_UART1.Rx_pTail;
    }
}
