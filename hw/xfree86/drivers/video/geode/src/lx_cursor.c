/* Copyright (c) 2003-2008 Advanced Micro Devices, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "geode.h"

Bool
LXCursorInit(ScreenPtr pScrn)
{
    return xf86_cursors_init(pScrn,
                             LX_CURSOR_MAX_WIDTH, LX_CURSOR_MAX_HEIGHT,
                             HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
                             HARDWARE_CURSOR_INVERT_MASK |
                             HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
                             HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32 |
                             HARDWARE_CURSOR_ARGB);
}

void
LXLoadARGBCursorImage(ScrnInfoPtr pScrni, unsigned char *src)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    vg_set_color_cursor_shape(pGeode->CursorStartOffset, src,
                              LX_CURSOR_MAX_WIDTH, LX_CURSOR_MAX_HEIGHT,
                              LX_CURSOR_MAX_WIDTH * 4, 0, 0);
}
