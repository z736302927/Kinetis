/* SPDX-License-Identifier: GPL-2.0-only */
/*
* Copyright (C) 2012 Invensense, Inc.
*/

#ifndef __AT24_PLATFORM_H_
#define __AT24_PLATFORM_H_

#include <linux/init.h>

int __init at24_init(void);
void __exit at24_exit(void);

#endif
