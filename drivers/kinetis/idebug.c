#include "kinetis/idebug.h"
#include "stdarg.h"

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify the serial print function UART.
  * @step 3:  Modify variables dbg_level and set the debug level.
  * @step 4:  Modify the time fetch function OS_TIME_MS().
  * @step 5:
  */

#include "usart.h"

//#ifdef __GNUC__
///* With GCC, small printf (option LD Linker->Libraries->Small printf
//   set to 'Yes') calls __io_putchar() */
//#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
int fputc(int ch, FILE *f)
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART3 and Loop until the end of transmission */
    HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 0xFFFF);

    return ch;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

enum log_flags {
    LOG_NEWLINE	= 2,	/* text ended with a newline */
    LOG_CONT	= 8,	/* text is a fragment of a continuation line */
};

#ifdef CONFIG_PRINTK_CALLER
#define PREFIX_MAX		48
#else
#define PREFIX_MAX		32
#endif
#define LOG_LINE_MAX		(1024 - PREFIX_MAX)

#define LOG_LEVEL(v)		((v) & 0x07)
#define LOG_FACILITY(v)		((v) >> 3 & 0xff)

int printk(const char *fmt, ...)
{
    static char time[32];
    size_t pre_len = 0;
    static char textbuf[LOG_LINE_MAX];
    char *text = textbuf;
    size_t text_len;
    int lflags = 0;
    va_list args;
    int level = LOGLEVEL_DEFAULT, kern_level;
    
    va_start(args, fmt);
    text_len = vsnprintf(textbuf, LOG_LINE_MAX - 2, fmt, args);
    va_end(args);

    /* mark and strip a trailing newline */
    if (text_len && text[text_len - 1] == '\n') {
        text_len--;
        lflags |= LOG_NEWLINE;
    }

    while ((kern_level = printk_get_level(text)) != 0) {
        switch (kern_level) {
            case '0' ... '7':
                if (level == LOGLEVEL_DEFAULT)
                    level = kern_level - '0';

                break;

            case 'c':	/* KERN_CONT */
                lflags |= LOG_CONT;
                lflags &= ~LOG_NEWLINE;
                text_len++;
        }

        text_len -= 2;
        text += 2;
    }

    if (level > LOGLEVEL_DEFAULT)
        return -EINVAL;

    switch (level) {
        case LOGLEVEL_DEFAULT:
            break;

        case LOGLEVEL_EMERG:
        case LOGLEVEL_ALERT:
        case LOGLEVEL_CRIT:
        case LOGLEVEL_ERR:
            break;

        case LOGLEVEL_WARNING:
            break;

        case LOGLEVEL_NOTICE:
        case LOGLEVEL_INFO:
        case LOGLEVEL_DEBUG:
            break;
    }
    
    if (lflags & LOG_NEWLINE) {
        snprintf(time, sizeof(time), "[%5d.%06d] ",
            basic_timer_get_ss(), basic_timer_get_timer_cnt());
        pre_len = strlen(time);
        
        memmove(text + pre_len, text, text_len);
        memmove(text, time, pre_len);
        text_len += pre_len;
        
        text[text_len] = '\n';
        text[text_len + 1] = '\r';
        text_len += 2;
    }
    
    HAL_UART_Transmit(&huart1, (u8 *)text, text_len, 0xFFFF);
    
    return text_len;
}

void kinetis_dump_buffer(void *Buffer, int Size)
{
    int i;
    printk("[%5d.%06d] ", basic_timer_get_ss(), basic_timer_get_timer_cnt());

    for (i = 0; i < Size; i++) {
        if ((i % 32 == 0) && (i != 0)) {
            printk("\r\n");
            printk("[%5d.%06d] ", basic_timer_get_ss(), basic_timer_get_timer_cnt());
        }

        printk("%02X ", ((u8 *)Buffer)[i]);
    }

    printk("\r\n");
}







