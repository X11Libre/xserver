/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
#include <dix-config.h>

#include <stddef.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>

#include "dix/screenint_priv.h"
#include "mi/mi_priv.h"
#include "os/ddx_priv.h"
#include "os/osdep.h"

#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "dixfontstr.h"
#include "extinit_priv.h"
#include "Xnest.h"
#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "Pointer.h"
#include "Keyboard.h"
#include "Handlers.h"
#include "Events.h"
#include "Init.h"
#include "Args.h"
#include "Drawable.h"
#include "XNGC.h"
#include "XNFont.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif

Bool xnestDoFullGeneration = TRUE;

/* Xnest doesn't support GLX yet, so we don't link it, but still have
   satisfy DIX's symbol requirements */
#ifdef GLXEXT
void
GlxExtensionInit(void)
{
}

Bool noGlxExtension = FALSE;
#endif

void
InitOutput(ScreenInfo * screen_info, int argc, char *argv[])
{
    int i, j;

    xnestOpenDisplay(argc, argv);

    screen_info->imageByteOrder = xnestUpstreamInfo.setup->image_byte_order;
    screen_info->bitmapScanlineUnit = xnestUpstreamInfo.setup->bitmap_format_scanline_unit;
    screen_info->bitmapScanlinePad = xnestUpstreamInfo.setup->bitmap_format_scanline_pad;
    screen_info->bitmapBitOrder = xnestUpstreamInfo.setup->bitmap_format_bit_order;
    screen_info->numPixmapFormats = 0;
    for (i = 0; i < xnestNumPixmapFormats; i++)
        for (j = 0; j < xnestNumDepths; j++)
            if ((xnestPixmapFormats[i].depth == 1) ||
                (xnestPixmapFormats[i].depth == xnestDepths[j])) {
                screen_info->formats[screen_info->numPixmapFormats].depth =
                    xnestPixmapFormats[i].depth;
                screen_info->formats[screen_info->numPixmapFormats].bitsPerPixel =
                    xnestPixmapFormats[i].bits_per_pixel;
                screen_info->formats[screen_info->numPixmapFormats].scanlinePad =
                    xnestPixmapFormats[i].scanline_pad;
                screen_info->numPixmapFormats++;
                break;
            }

    xnestFontPrivateIndex = xfont2_allocate_font_private_index();

    if (!xnestNumScreens)
        xnestNumScreens = 1;

    for (i = 0; i < xnestNumScreens; i++)
        AddScreen(xnestOpenScreen, argc, argv);

    xnestNumScreens = screen_info->numScreens;

    xnestDoFullGeneration = xnestFullGeneration;
}

static void
xnestNotifyConnection(int fd, int ready, void *data)
{
    xnestCollectEvents();
}

void
InitInput(int argc, char *argv[])
{
    int rc;

    rc = AllocDevicePair(serverClient, "Xnest",
                         &xnestPointerDevice,
                         &xnestKeyboardDevice,
                         xnestPointerProc, xnestKeyboardProc, FALSE);

    if (rc != Success)
        FatalError("Failed to init Xnest default devices.\n");

    mieqInit();

    SetNotifyFd(XConnectionNumber(xnestDisplay), xnestNotifyConnection, X_NOTIFY_READ, NULL);

    RegisterBlockAndWakeupHandlers(xnestBlockHandler, xnestWakeupHandler, NULL);
}

void
CloseInput(void)
{
    mieqFini();
}

void
ddxGiveUp(enum ExitCode error)
{
    xnestDoFullGeneration = TRUE;
    xnestCloseDisplay();
}

#ifdef __APPLE__
void
DarwinHandleGUI(int argc, char *argv[])
{
}
#endif

void
OsVendorInit(void)
{
    return;
}

void
OsVendorFatalError(const char *f, va_list args)
{
    return;
}

#if defined(DDXBEFORERESET)
void
ddxBeforeReset(void)
{
    return;
}
#endif

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
}
#endif
