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
DECLARE_HOOK_PROC(WindowDestroy, _notify_window_destroy)
DECLARE_HOOK_PROC(WindowPosition, _notify_window_position)
DECLARE_HOOK_PROC(Close, _notify_screen_close)

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

    ARRAY_FOR_EACH(pScreen->_notify_window_position, walk) {
        if (walk.ptr->func)
            walk.ptr->func(pScreen, pWin, walk.ptr->arg, x, y);
    }

    if (pScreen->PositionWindow)
        (*pScreen->PositionWindow) (pWin, x, y);
}

void dixScreenRaiseClose(ScreenPtr pScreen)
{
    if (!pScreen)
        return;

    ARRAY_FOR_EACH(pScreen->_notify_screen_close, walk) {
        if (walk.ptr->func)
            walk.ptr->func(pScreen, walk.ptr->arg);
    }

    if (pScreen->CloseScreen)
        (*pScreen->CloseScreen) (pScreen);
}
