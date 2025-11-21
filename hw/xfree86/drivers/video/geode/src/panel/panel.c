/* Copyright (c) 2005 Advanced Micro Devices, Inc.
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
 * File Contents:   This file contains the file inclusions, macro definitions
 *                  for the panel.
 * 
 * SubModule:       Geode FlatPanel library
 * */

#if defined(linux)              /* Linux */

#ifdef __KERNEL__

#include <linux/string.h>
#include <asm/io.h>

#elif !defined(XFree86Server)
#error blah

#include <linux/fs.h>
#include <asm/mman.h>

#endif                          /* __KERNEL__ */
#elif defined(_WIN32)           /* windows */

#include <windows.h>

#endif

#include "panel.h"
#include "gfx_defs.h"

extern unsigned char *gfx_virt_regptr;
extern unsigned char *gfx_virt_fbptr;
extern unsigned char *gfx_virt_vidptr;
extern unsigned char *gfx_virt_vipptr;
extern unsigned long gfx_detect_video(void);

#define PLATFORM_DYNAMIC		1       /* runtime selection */
#define PLATFORM_DRACO			1       /* Draco + 9210 */
#define PLATFORM_CENTAURUS		1       /* Centaurus + 9211 RevA */
#define PLATFORM_DORADO			1       /* Dorado + 9211 RevC */
#define PLATFORM_REDCLOUD		1       /* GX2 */

unsigned char *XpressROMPtr;

#include "pnl_init.c"
#include "pnl_bios.c"
