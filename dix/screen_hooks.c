/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/screen_hooks_priv.h"
#include "include/dix.h"
#include "include/os.h"
#include "include/scrnintstr.h"
#include "include/windowstr.h"

#define DECLARE_HOOK_PROC(NAME, FIELD, TYPE) \
    void dixScreenHook##NAME(ScreenPtr pScreen, TYPE func) \
    { \
        AddCallback(&pScreen->FIELD, (CallbackProcPtr)func, pScreen); \
    } \
    \
    void dixScreenUnhook##NAME(ScreenPtr pScreen, TYPE func) \
    { \
        DeleteCallback(&pScreen->FIELD, (CallbackProcPtr)func, pScreen); \
    }

DECLARE_HOOK_PROC(WindowDestroy, hookWindowDestroy, XorgScreenWindowDestroyProcPtr);
DECLARE_HOOK_PROC(WindowPosition, hookWindowPosition, XorgScreenWindowPositionProcPtr);
DECLARE_HOOK_PROC(Close, hookClose, XorgScreenCloseProcPtr);
DECLARE_HOOK_PROC(PixmapDestroy, hookPixmapDestroy, XorgScreenPixmapDestroyProcPtr);

int dixScreenRaiseWindowDestroy(WindowPtr pWin)
{
    if (!pWin)
        return Success;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    CallCallbacks(&pScreen->hookWindowDestroy, pWin);

    return (pScreen->DestroyWindow ? pScreen->DestroyWindow(pWin) : Success);
}

void dixScreenRaiseWindowPosition(WindowPtr pWin, uint32_t x, uint32_t y)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    XorgScreenWindowPositionParamRec param = {
        .window = pWin,
        .x = x,
        .y = y,
    };

    CallCallbacks(&pScreen->hookWindowPosition, &param);

    if (pScreen->PositionWindow)
        pScreen->PositionWindow(pWin, x, y);
}

void dixScreenRaiseClose(ScreenPtr pScreen) {
    if (!pScreen)
        return;

    CallCallbacks(&pScreen->hookClose, NULL);

    if (pScreen->CloseScreen)
        pScreen->CloseScreen(pScreen);
}

void dixScreenRaisePixmapDestroy(PixmapPtr pPixmap)
{
    if (!pPixmap)
        return;

    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    CallCallbacks(&pScreen->hookPixmapDestroy, pPixmap);
    /* we must not call the original ScreenRec->DestroyPixmap() here */
}
