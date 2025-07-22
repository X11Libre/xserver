/* Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/*
 * File Contents:   This file contailns the panel include files and
 *                  external pointer to the hardware.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 * */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __KERNEL__

#include <linux/string.h>
#include <asm/io.h>

#endif

#include "panel.h"
#include "gfx_defs.h"
#include "geode.h"

#define PLATFORM_DYNAMIC		1       /* runtime selection            */
#define PLATFORM_DRACO			0       /* Draco + 9210                         */
#define PLATFORM_CENTAURUS		1       /* Centaurus + 9211 RevA        */
#define PLATFORM_DORADO			1       /* Dorado + 9211 RevC           */
#define PLATFORM_GX2BASED		1       /* Redcloud                             */

unsigned char *XpressROMPtr;

#include "pnl_init.c"
#include "pnl_bios.c"
