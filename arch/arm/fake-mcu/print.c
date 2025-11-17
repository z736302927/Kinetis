#include <generated/deconfig.h>
#include <linux/printk.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <fake-mcu/print.h>

///* Must be called under logbuf_lock. */
//int vprintk_store(const char *fmt, va_list args)
//{
//	static char textbuf[1024];
//	char *text = textbuf;
//	size_t text_len;
//    int level;
//    int kern_level;
//
//	/*
//	 * The printf needs to come first; we need the syslog
//	 * prefix which might be passed-in as a parameter.
//	 */
//	text_len = snprintf(text, sizeof(textbuf), fmt, args);
//
//	/* mark and strip a trailing newline */
//	if (text_len && text[text_len-1] == '\n') {
//		text_len--;
//	}
//
//	/* strip kernel syslog prefix and extract log level or control flags */
//
//    while ((kern_level = printk_get_level(text)) != 0) {
//        switch (kern_level) {
//        case '0' ... '7':
//            if (level == LOGLEVEL_DEFAULT)
//                level = kern_level - '0';
//            break;
//        case 'c':	/* KERN_CONT */
//            ;
//        }
//
//        text_len -= 2;
//        text += 2;
//    }
//
//    printf("%s\n", text);
//
//	return text_len;
//}
//
//int printk(const char *fmt, ...)
//{
//	va_list args;
//	int r;
//
//	va_start(args, fmt);
//	r = vprintk_store(fmt, args);
//	va_end(args);
//
//	return r;
//}