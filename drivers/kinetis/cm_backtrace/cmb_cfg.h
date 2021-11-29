/*
 * This file is part of the CmBacktrace Library.
 *
 * Copyright (c) 2016, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2016-12-15
 */

#ifndef _CMB_CFG_H_
#define _CMB_CFG_H_

#include <generated/deconfig.h>
#include <linux/printk.h>

///* C stack block start address, defined on linker script file, default is _sstack */
//#define CMB_CSTACK_BLOCK_START         0x30000000
///* C stack block end address, defined on linker script file, default is _estack */
//#define CMB_CSTACK_BLOCK_END           0x30048000
///* code section start address, defined on linker script file, default is _stext */
//#define CMB_CODE_SECTION_START         0x24000000
///* code section end address, defined on linker script file, default is _etext */
//#define CMB_CODE_SECTION_END           0x24080000

/* C stack block start address, defined on linker script file, default is _sstack */
#define CMB_CSTACK_BLOCK_START         0x24000000
/* C stack block end address, defined on linker script file, default is _estack */
#define CMB_CSTACK_BLOCK_END           0x24004000
/* code section start address, defined on linker script file, default is _stext */
#define CMB_CODE_SECTION_START         0x08000000
/* code section end address, defined on linker script file, default is _etext */
#define CMB_CODE_SECTION_END           0x08050000

/* print line, must config by user */
#define cmb_println(fmt, ...)              printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)/* e.g., printf(__VA_ARGS__);printf("\r\n") */
/* enable bare metal(no OS) platform */
#define CMB_USING_BARE_METAL_PLATFORM
/* enable OS platform */
/* #define CMB_USING_OS_PLATFORM */
/* OS platform type, must config when CMB_USING_OS_PLATFORM is enable */
/* #define CMB_OS_PLATFORM_TYPE           CMB_OS_PLATFORM_RTT or CMB_OS_PLATFORM_UCOSII or CMB_OS_PLATFORM_UCOSIII or CMB_OS_PLATFORM_FREERTOS */
/* cpu platform type, must config by user */
#define CMB_CPU_PLATFORM_TYPE          CMB_CPU_ARM_CORTEX_M7/* CMB_CPU_ARM_CORTEX_M0 or CMB_CPU_ARM_CORTEX_M3 or CMB_CPU_ARM_CORTEX_M4 or CMB_CPU_ARM_CORTEX_M7 */
/* enable dump stack information */
#define CMB_USING_DUMP_STACK_INFO
/* language of print information */
#define CMB_PRINT_LANGUAGE             CMB_PRINT_LANGUAGE_ENGLISH/* CMB_PRINT_LANGUAGE_ENGLISH(default) or CMB_PRINT_LANGUAGE_CHINESE */
#endif /* _CMB_CFG_H_ */
