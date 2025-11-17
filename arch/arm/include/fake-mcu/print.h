//#ifndef _FAKE__MCU_PRINT_H
//#define _FAKE__MCU_PRINT_H
//
//#include <stdio.h>
//
//#define KERN_SOH	"\001"		/* ASCII Start Of Header */
//#define KERN_SOH_ASCII	'\001'
//
//#define KERN_EMERG	KERN_SOH "0"	/* system is unusable */
//#define KERN_ALERT	KERN_SOH "1"	/* action must be taken immediately */
//#define KERN_CRIT	KERN_SOH "2"	/* critical conditions */
//#define KERN_ERR	KERN_SOH "3"	/* error conditions */
//#define KERN_WARNING	KERN_SOH "4"	/* warning conditions */
//#define KERN_NOTICE	KERN_SOH "5"	/* normal but significant condition */
//#define KERN_INFO	KERN_SOH "6"	/* informational */
//#define KERN_DEBUG	KERN_SOH "7"	/* debug-level messages */
//
//#define KERN_DEFAULT	""		/* the default kernel loglevel */
//
///*
// * Annotation for a "continued" line of log printout (only done after a
// * line that had no enclosing \n). Only to be used by core/arch code
// * during early bootup (a continued line is not SMP-safe otherwise).
// */
//#define KERN_CONT	KERN_SOH "c"
//
///* integer equivalents of KERN_<LEVEL> */
//#define LOGLEVEL_SCHED		-2	/* Deferred messages from sched code
//					 * are set to this special level */
//#define LOGLEVEL_DEFAULT	-1	/* default (or last) loglevel */
//#define LOGLEVEL_EMERG		0	/* system is unusable */
//#define LOGLEVEL_ALERT		1	/* action must be taken immediately */
//#define LOGLEVEL_CRIT		2	/* critical conditions */
//#define LOGLEVEL_ERR		3	/* error conditions */
//#define LOGLEVEL_WARNING	4	/* warning conditions */
//#define LOGLEVEL_NOTICE		5	/* normal but significant condition */
//#define LOGLEVEL_INFO		6	/* informational */
//#define LOGLEVEL_DEBUG		7	/* debug-level messages */
//
//static inline int printk_get_level(const char *buffer)
//{
//	if (buffer[0] == KERN_SOH_ASCII && buffer[1]) {
//		switch (buffer[1]) {
//		case '0' ... '7':
//		case 'c':	/* KERN_CONT */
//			return buffer[1];
//		}
//	}
//	return 0;
//}
//
//int printk(const char *fmt, ...);
//
//#ifndef pr_fmt
//#define pr_fmt(fmt) fmt
//#endif
//
//#define pr_emerg(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_alert(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_crit(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_err(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_warn(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_notice(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_info(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//
//#define pr_cont(fmt, ...) \
//	printk(fmt, ##__VA_ARGS__)
//
//#ifdef DEBUG
//#define pr_devel(fmt, ...) \
//	printk(pr_fmt(fmt), ##__VA_ARGS__)
//#else
//#define pr_devel(fmt, ...) \
//	no_printk(pr_fmt(fmt), ##__VA_ARGS__)
//#endif
//
//#endif
