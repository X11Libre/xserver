/*
 * Copyright 2008 Jerome Glisse
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef AMDGPU_DRI2_H
#define AMDGPU_DRI2_H

#include <xorg-server.h>

struct amdgpu_dri2 {
	drmVersionPtr pKernelDRMVersion;
	Bool available;
	Bool enabled;
	char *device_name;
};

#ifdef DRI2

#include "dri2.h"
Bool amdgpu_dri2_screen_init(ScreenPtr pScreen);
void amdgpu_dri2_close_screen(ScreenPtr pScreen);

#else

static inline Bool amdgpu_dri2_screen_init(ScreenPtr pScreen)
{
	return FALSE;
}

static inline void amdgpu_dri2_close_screen(ScreenPtr pScreen)
{
}

#endif

#endif /* AMDGPU_DRI2_H */
