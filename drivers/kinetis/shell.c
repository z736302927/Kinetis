#include "kinetis/shell.h"
#include "stdio.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "usart.h"
#include "peripheral/serialport.h"
#include "linux/gfp.h"
#include "kinetis/memory.h"
#include "string.h"

static u16 shell_Buffer[128];
u16 *shell_Pointer = shell_Buffer;

void shell_Init(void)
{
    HAL_UART_Receive_DMA(&huart1, (u8 *)shell_Buffer, 128);
    memset(shell_Buffer, 0xFF, sizeof(shell_Buffer));
}

static inline u8 shell_PortReceive(void)
{
    u8 Data = 0;

    if (*shell_Pointer & 0xFF00)
        Data = 0xFF;
    else {
        Data = (u8) * shell_Pointer;
        *shell_Pointer = 0xFFFF;

        if (shell_Pointer == &shell_Buffer[127])
            shell_Pointer = shell_Buffer;
        else
            shell_Pointer++;
    }

//    HAL_UART_Receive_IT(&huart1, &Data, 1);

//    if(shell_PortState() == false) {}

//    SerialPort_TypeDef Instance;

//    Instance.PortNbr = 1;
//    Instance.TxBuffer_Size = 1;
//    Instance.TempBuffer_Size = 1;
//    Instance.RxScanInterval = 10;
//    Instance.Endchar = NULL;
//    SerialPort_Open(&Instance);

//    if(SerialPort_Receive(&Instance) == true)
//    {
//        Data = Instance.RxBuffer[0];
//        kfree(Instance.RxBuffer);
//    }

//    SerialPort_Close(&Instance);

    return Data;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

u8 shell_GetUserInput(char *pointer)
{
    char inputchar = '\0';
    u16 inputlen = 0;

    while (inputchar != '\r') {
        inputchar = shell_PortReceive();

        if (inputchar == 0xFF)
            continue;
        else if (inputchar == '\b') {
            if (inputlen != 0) {
                pointer--;
                inputlen--;
                printf("\b \b");
            }
        } else {
            *pointer = inputchar;
            pointer++;
            inputlen++;
            printf("%c", inputchar);
        }
    }

    pointer--;

    if (inputlen == 1)
        *pointer = '\r';
    else
        *pointer = '\0';

    printf("\r\n");

    return true;
}



