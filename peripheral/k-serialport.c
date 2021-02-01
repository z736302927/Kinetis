#include "peripheral/k-serialport.h"
#include "timer/k-delay.h"
#include "linux/gfp.h"
#include "core/k-memory.h"

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
#include "core/idebug.h"
#include "timer/k-basictimer.h"

int SerialPort_RxDataFlag = 0;
uint16_t serialport_index;

uint32_t SerialPort_GetTick(void)
{
    return BasicTimer_GetMSTick();
}

void SerialPort_SetRxState(int state)
{
    SerialPort_RxDataFlag = state;
}

int SerialPort_GetRxState(void)
{
    return SerialPort_RxDataFlag;
}

void serialport_get_rxdata(uint16_t data)
{
    SerialPort_TypeDef *Instance;
    
    Instance->TempBuffer[serialport_index] = data;
    serialport_index++;

    if (serialport_index >= Instance->TempBuffer_Size)
        serialport_index = 0;
}

void SerialPort_RxBuffer_Init(SerialPort_TypeDef *Instance)
{
    memset(Instance->TempBuffer, 0xFF, Instance->TempBuffer_Size * sizeof(uint16_t));
}

uint8_t SerialPort_Alloc(SerialPort_TypeDef *Instance)
{
    Instance->TempBuffer = (uint16_t *)kmalloc(Instance->TempBuffer_Size, __GFP_ZERO);

    if (Instance->TempBuffer == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "SerialPort Rx malloc failed !");
        return false;
    } else
        SerialPort_RxBuffer_Init(Instance);

    return true;
}

void SerialPort_Free(SerialPort_TypeDef *Instance)
{
    kfree(Instance->TempBuffer);
    Instance->TempBuffer = NULL;
}

uint32_t SerialPort_GetBaud(SerialPort_TypeDef *Instance)
{
    uint32_t retval;

    if (Instance->PortNbr == 1)
        retval = huart1.Init.BaudRate;
    else if (Instance->PortNbr == 2)
        retval = huart2.Init.BaudRate;
    else if (Instance->PortNbr == 3)
        retval = huart3.Init.BaudRate;

    return retval;
}

uint32_t SerialPort_GetMinInterval(SerialPort_TypeDef *Instance)
{
    uint32_t Baud = 0;
    uint32_t Interval;
    Baud = SerialPort_GetBaud(Instance);
    Interval = 1000 / (Baud / 10) + 1;
    return Interval;
}

uint8_t SerialPort_Open(SerialPort_TypeDef *Instance)
{
    Instance->RxScanInterval += SerialPort_GetMinInterval(Instance);

    if (SerialPort_Alloc(Instance) == false)
        return false;

    if (Instance->PortNbr == 1)
        HAL_UART_Receive_DMA(&huart1, (uint8_t *)Instance->TempBuffer, Instance->TempBuffer_Size);
    else if (Instance->PortNbr == 2)
        HAL_UART_Receive_DMA(&huart2, (uint8_t *)Instance->TempBuffer, Instance->TempBuffer_Size);
    else if (Instance->PortNbr == 3)
        HAL_UART_Receive_DMA(&huart3, (uint8_t *)Instance->TempBuffer, Instance->TempBuffer_Size);

    return true;
}

void SerialPort_Close(SerialPort_TypeDef *Instance)
{
//    if(Instance->PortNbr == 1)
//        HAL_UART_MspDeInit(&huart1);
//    else if(Instance->PortNbr == 2)
//        HAL_UART_MspDeInit(&huart2);
//    else if(Instance->PortNbr == 3)
//        HAL_UART_MspDeInit(&huart3);
    SerialPort_Free(Instance);
}

void SerialPort_Send(SerialPort_TypeDef *Instance)
{
    if (Instance->PortNbr == 1)
        HAL_UART_Transmit_IT(&huart1, Instance->TxBuffer, Instance->TxBuffer_Size);
    else if (Instance->PortNbr == 2)
        HAL_UART_Transmit_IT(&huart2, Instance->TxBuffer, Instance->TxBuffer_Size);
    else if (Instance->PortNbr == 3)
        HAL_UART_Transmit_IT(&huart3, Instance->TxBuffer, Instance->TxBuffer_Size);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


void SerialPort_RxBuffer_FindTail(SerialPort_TypeDef *Instance)
{
    uint16_t Count = 0;

    while ((Instance->TempBuffer[Instance->Rx_pTail] >> 8) != 0xFF) {
        Count++;

        if (Instance->Rx_pTail == (Instance->TempBuffer_Size - 1))
            Instance->Rx_pTail = 0;
        else
            Instance->Rx_pTail++;

        if (Count == (Instance->TempBuffer_Size - 1))
            break;
    }
}

uint8_t *SerialPort_Find_Endchar(SerialPort_TypeDef *Instance, uint16_t *Size)
{
    uint16_t i, j;
    uint8_t *pBuffer;

    if (Instance->Rx_pTail > Instance->Rx_pHead) {
        *Size = Instance->Rx_pTail - Instance->Rx_pHead;
        pBuffer = (uint8_t *)kmalloc(*Size, __GFP_ZERO);

        if (pBuffer == NULL)
            return NULL;

        for (i = Instance->Rx_pHead, j = 0; i < Instance->Rx_pTail; i++, j++)
            pBuffer[j] = (uint8_t)Instance->TempBuffer[i];

        return pBuffer;
    } else if (Instance->Rx_pTail < Instance->Rx_pHead) {
        *Size = Instance->TempBuffer_Size - Instance->Rx_pHead + Instance->Rx_pTail;
        pBuffer = (uint8_t *)kmalloc(*Size, __GFP_ZERO);

        if (pBuffer == NULL)
            return NULL;

        for (i = Instance->Rx_pHead, j = 0; i < Instance->TempBuffer_Size; i++, j++)
            pBuffer[j] = (uint8_t)Instance->TempBuffer[i];

        for (i = 0; i < Instance->Rx_pTail; i++, j++)
            pBuffer[j] = (uint8_t)Instance->TempBuffer[i];

        return pBuffer;
    }

    return NULL;
}

void SerialPort_Extract_ValidData(SerialPort_TypeDef *Instance)
{
    uint16_t i, j;

    if (Instance->Rx_pTail > Instance->Rx_pHead) {
        Instance->RxBuffer_Size = Instance->Rx_pTail - Instance->Rx_pHead;
        Instance->RxBuffer = kmalloc(Instance->RxBuffer_Size, __GFP_ZERO);

        if (Instance->RxBuffer == NULL)
            return;

        for (i = Instance->Rx_pHead, j = 0; i < Instance->Rx_pTail; i++, j++)
            Instance->RxBuffer[j] = Instance->TempBuffer[i];

        memset(&Instance->TempBuffer[Instance->Rx_pHead], 0xFF, Instance->RxBuffer_Size * sizeof(uint16_t));
    } else if (Instance->Rx_pTail < Instance->Rx_pHead) {
        Instance->RxBuffer_Size = Instance->TempBuffer_Size - Instance->Rx_pHead + Instance->Rx_pTail;
        Instance->RxBuffer = kmalloc(Instance->RxBuffer_Size, __GFP_ZERO);

        if (Instance->RxBuffer == NULL)
            return;

        for (i = Instance->Rx_pHead, j = 0; i < Instance->TempBuffer_Size; i++, j++)
            Instance->RxBuffer[j] = Instance->TempBuffer[i];

        for (i = 0; i < Instance->Rx_pTail; i++, j++)
            Instance->RxBuffer[j] = Instance->TempBuffer[i];

        memset(&Instance->TempBuffer[Instance->Rx_pHead], 0xFF, (Instance->TempBuffer_Size - Instance->Rx_pHead) * sizeof(uint16_t));
        memset(&Instance->TempBuffer[0], 0xFF, Instance->Rx_pTail * sizeof(uint16_t));
    } else
        Instance->RxBuffer_Size = 0;

    if (Instance->RxBuffer_Size != 0) {
        Instance->Rx_pHead = Instance->Rx_pTail;
        SerialPort_SetRxState(1);
    }
}

uint8_t SerialPort_Receive(SerialPort_TypeDef *Instance)
{
    static uint32_t Refer = 0;
    uint32_t Delta = 0;
    uint8_t rxdata_tmptail = Instance->Rx_pTail;
    uint8_t wait_rx_done = 0;
    uint16_t Size;
    SerialPort_RxBuffer_FindTail(Instance);

    if (Instance->Rx_pHead != Instance->Rx_pTail) {
        if (rxdata_tmptail != Instance->Rx_pTail)
            Refer = SerialPort_GetTick();
        else {
            Delta = SerialPort_GetTick() >= Refer ?
                SerialPort_GetTick() - Refer :
                SerialPort_GetTick() + (DELAY_TIMER_UNIT - Refer);

            if (Delta > Instance->RxScanInterval)
                wait_rx_done = 1;
        }
    }

    if (wait_rx_done == 1) {
        if (Instance->Endchar != NULL) {
            Instance->CurrentBuffer = SerialPort_Find_Endchar(Instance, &Size);

            if (Instance->CurrentBuffer != NULL) {
                if (strncmp((char *)Instance->Endchar,
                        (char *)&Instance->CurrentBuffer[Size - Instance->Endchar_Size],
                        Instance->Endchar_Size) != 0) {
                    kfree(Instance->CurrentBuffer);
                    wait_rx_done = 0;
                    return false;
                } else {
                    kfree(Instance->CurrentBuffer);
                    SerialPort_Extract_ValidData(Instance);

                    if (Instance->RxBuffer_Size > 1)
                        Instance->RxBuffer[Instance->RxBuffer_Size - 1] = '\0';

                    return true;
                }
            }
        } else {
            SerialPort_Extract_ValidData(Instance);
            return true;
        }
    }

    return false;
}

#ifdef DESIGN_VERIFICATION_SEIRALPORT
#include "dv/k-test.h"

int t_SerialPort_Shell(int argc, char **argv)
{
    SerialPort_TypeDef Instance;
    char *word = "\r";
    memset(&Instance, 0, sizeof(SerialPort_TypeDef));
    Instance.PortNbr = 1;
    Instance.TxBuffer_Size = 128;
    Instance.TempBuffer_Size = 200;
    Instance.RxScanInterval = 10;
    Instance.Endchar = kmalloc(strlen(word), GFP_KERNEL);
    memcpy(Instance.Endchar, word, strlen(word));
    Instance.Endchar_Size = strlen(word);
    SerialPort_Open(&Instance);

    while (1) {
        if (SerialPort_Receive(&Instance) == true) {
            if (Instance.RxBuffer[0] == '\r')
                printf("\r\n/ # ");
            else if (Instance.RxBuffer[0] == 27)
                break;
            else {
                if (ParseTest_AllCase(Instance.RxBuffer) == PASS)
                    kinetis_debug_trace(KERN_DEBUG, "TEST PASS");
                else
                    kinetis_debug_trace(KERN_DEBUG, "TEST FAIL");

                printf("\r\n/ # ");
            }

            kfree(Instance.RxBuffer);
        }
    }

    SerialPort_Close(&Instance);
    kfree(Instance.Endchar);
}

#endif

