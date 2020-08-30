#include "idebug.h"
#include "timer/k-basictimer.h"
#include "stdio.h"
#include "stdarg.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the serial print function UART.
  * @step 3:  Modify variables dbg_level and set the debug level.
  * @step 4:  Modify the time fetch function OS_TIME_MS().
  * @step 5:
  */

#include "usart.h"

#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART3 and Loop until the end of transmission */
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);

    return ch;
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void kinetis_debug_trace(int dbg_level, const char *format, ...)
{
    char buffer[256];
    va_list args;
    int size;

    if(dbg_level > KERN_DEFAULT)
        return;

    printf("[%5d.%06d] ", BasicTimer_GetSSTick(), BasicTimer_GetUSTick());

    switch(dbg_level)
    {
        case KERN_EMERG:
        case KERN_ALERT:
        case KERN_CRIT:
        case KERN_ERR:
            printf("%s err in %d\n", __FUNCTION__, __LINE__);
            break;

        case KERN_WARNING:
            printf("%s warning in %d\n", __FUNCTION__, __LINE__);
            break;

        case KERN_NOTICE:
            break;

        case KERN_DEBUG:
            break;
    }

    va_start(args, format);
    size = vsprintf(buffer, format, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, size, 0xFFFF);

    printf("\r\n");
}

void kinetis_dump_buffer(void *Buffer, int Size)
{
    int i;

    printf("[%5d.%06d] ", BasicTimer_GetSSTick(), BasicTimer_GetUSTick());

    for(i = 0; i < Size; i++)
    {
        if((i % 32 == 0) && (i != 0))
        {
            printf("\r\n");
            printf("[%5d.%06d] ", BasicTimer_GetSSTick(), BasicTimer_GetUSTick());
        }

        printf("%02X ", ((uint8_t *)Buffer)[i]);
    }

    printf("\r\n");
}







