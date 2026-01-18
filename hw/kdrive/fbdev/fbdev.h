/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _KDRIVE_FBDEV_H_
#define _KDRIVE_FBDEV_H_
#include <stdio.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include "kdrive.h"

#ifdef RANDR
#include "randrstr.h"
#endif

#if defined(GLAMOR) && defined(GLXEXT)
#include <epoxy/egl.h>
#endif

typedef struct _fbdevPriv {
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    __u16 red[256];
    __u16 green[256];
    __u16 blue[256];
    int fd;
    char *fb;
    char *fb_base;
} FbdevPriv;

typedef struct _fbdevScrPriv {
    Rotation randr;
    Bool shadow;
#if defined(GLAMOR) && defined(GLXEXT)
    Bool glamor_initialized;
    EGLDisplay display;
    EGLContext ctx;
    void* glamor_make_current;
#endif
} FbdevScrPriv;

extern KdCardFuncs fbdevFuncs;
extern const char *fbdevDevicePath;
extern Bool fbDisableShadow;

#if defined(GLAMOR) && defined(GLXEXT)
extern const char *fbdev_glvnd_provider;
#endif

Bool fbdevCardInit(KdCardInfo * card);

Bool fbdevScreenInit(KdScreenInfo * screen);

Bool fbdevInitScreen(ScreenPtr pScreen);

Bool fbdevFinishInitScreen(ScreenPtr pScreen);

Bool fbdevCreateResources(ScreenPtr pScreen);

void fbdevPreserve(KdCardInfo * card);

Bool fbdevEnable(ScreenPtr pScreen);

Bool fbdevDPMS(ScreenPtr pScreen, int mode);

void fbdevDisable(ScreenPtr pScreen);

void fbdevRestore(KdCardInfo * card);

void fbdevScreenFini(KdScreenInfo * screen);

void fbdevCardFini(KdCardInfo * card);

void fbdevGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

void fbdevPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

Bool fbdevMapFramebuffer(KdScreenInfo * screen);

#if defined(GLAMOR) && defined(GLXEXT)
Bool fbdevInitAccel(ScreenPtr screen);

void fbdevEnableAccel(ScreenPtr screen);

void fbdevDisableAccel(ScreenPtr screen);

void fbdevFiniAccel(ScreenPtr screen);
#endif

#endif                          /* _KDRIVE_FBDEV_H_ */
