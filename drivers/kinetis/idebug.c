#include "kinetis/idebug.h"
#include "stdarg.h"

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/device.h>

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

/**
 * vscnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * The return value is the number of characters which have been written into
 * the @buf not including the trailing '\0'. If @size is == 0 the function
 * returns 0.
 *
 * If you're not already dealing with a va_list consider using scnprintf().
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	int i;

	i = vsnprintf(buf, size, fmt, args);

	if (likely(i < size))
		return i;
	if (size != 0)
		return size - 1;
	return 0;
}

/**
 * scnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @...: Arguments for the format string
 *
 * The return value is the number of characters written into @buf not including
 * the trailing '\0'. If @size is == 0 the function returns 0.
 */

int scnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vscnprintf(buf, size, fmt, args);
	va_end(args);

	return i;
}

/*
 * Device logging functions
 */

#ifdef CONFIG_PRINTK

int dev_vprintk_emit(int level, const struct device *dev,
		     const char *fmt, va_list args)
{
    static char time[32];
    size_t pre_len = 0;
    static char textbuf[LOG_LINE_MAX];
    char *text = textbuf;
    size_t text_len;
    int lflags = 0;
    
    text_len = vsnprintf(textbuf, LOG_LINE_MAX - 2, fmt, args);

    /* mark and strip a trailing newline */
    if (text_len && text[text_len - 1] == '\n') {
        text_len--;
        lflags |= LOG_NEWLINE;
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
EXPORT_SYMBOL(dev_vprintk_emit);

int dev_printk_emit(int level, const struct device *dev, const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);

	r = dev_vprintk_emit(level, dev, fmt, args);

	va_end(args);

	return r;
}
EXPORT_SYMBOL(dev_printk_emit);

static void __dev_printk(const char *level, const struct device *dev,
			struct va_format *vaf)
{
	if (dev)
		dev_printk_emit(level[1] - '0', dev, "%s: %pV",
				dev_name(dev), vaf);
	else
		printk("%s(NULL device *): %pV", level, vaf);
}

void dev_printk(const char *level, const struct device *dev,
		const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	__dev_printk(level, dev, &vaf);

	va_end(args);
}
EXPORT_SYMBOL(dev_printk);

#define define_dev_printk_level(func, kern_level)		\
void func(const struct device *dev, const char *fmt, ...)	\
{								\
	struct va_format vaf;					\
	va_list args;						\
								\
	va_start(args, fmt);					\
								\
	vaf.fmt = fmt;						\
	vaf.va = &args;						\
								\
	__dev_printk(kern_level, dev, &vaf);			\
								\
	va_end(args);						\
}								\
EXPORT_SYMBOL(func);

define_dev_printk_level(_dev_emerg, KERN_EMERG);
define_dev_printk_level(_dev_alert, KERN_ALERT);
define_dev_printk_level(_dev_crit, KERN_CRIT);
define_dev_printk_level(_dev_err, KERN_ERR);
define_dev_printk_level(_dev_warn, KERN_WARNING);
define_dev_printk_level(_dev_notice, KERN_NOTICE);
define_dev_printk_level(_dev_info, KERN_INFO);

#endif