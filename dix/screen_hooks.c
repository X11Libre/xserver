/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include <dix-config.h>

#include <X11/Xdefs.h>

#include "dix/dix_priv.h"
#include "dix/screen_hooks_priv.h"
#include "include/dix.h"
#include "include/os.h"
#include "include/scrnintstr.h"
#include "include/windowstr.h"

#define DECLARE_HOOK_PROC(NAME, FIELD, TYPE) \
    void dixScreenHook##NAME(ScreenPtr pScreen, TYPE func) \
    { \
        AddCallback(&pScreen->FIELD, (CallbackProcPtr)(func), pScreen); \
    } \
    \
    void dixScreenUnhook##NAME(ScreenPtr pScreen, TYPE func) \
    { \
        DeleteCallback(&pScreen->FIELD, (CallbackProcPtr)(func), pScreen); \
    }

DECLARE_HOOK_PROC(WindowDestroy, hookWindowDestroy, XorgScreenWindowDestroyProcPtr)
DECLARE_HOOK_PROC(WindowPosition, hookWindowPosition, XorgScreenWindowPositionProcPtr)
DECLARE_HOOK_PROC(WindowRealize, hookWindowRealize, XorgScreenWindowRealizeProcPtr)
DECLARE_HOOK_PROC(WindowUnrealize, hookWindowUnrealize, XorgScreenWindowUnrealizeProcPtr)
DECLARE_HOOK_PROC(WindowMove, hookWindowMove, XorgScreenWindowMoveProcPtr)
DECLARE_HOOK_PROC(PostWindowMove, hookPostWindowMove, XorgScreenWindowMoveProcPtr)
DECLARE_HOOK_PROC(WindowResize, hookWindowResize, XorgScreenWindowResizeProcPtr)
DECLARE_HOOK_PROC(PostWindowResize, hookPostWindowResize, XorgScreenWindowResizeProcPtr)
DECLARE_HOOK_PROC(WindowChangeBorderWidth, hookWindowChangeBorderWidth, XorgScreenWindowChangeBorderWidthProcPtr)
DECLARE_HOOK_PROC(PostWindowChangeBorderWidth, hookPostWindowChangeBorderWidth, XorgScreenWindowChangeBorderWidthProcPtr)
DECLARE_HOOK_PROC(WindowReparent, hookWindowReparent, XorgScreenWindowReparentProcPtr)
DECLARE_HOOK_PROC(PostWindowReparent, hookPostWindowReparent, XorgScreenWindowReparentProcPtr)
DECLARE_HOOK_PROC(Close, hookClose, XorgScreenCloseProcPtr)
DECLARE_HOOK_PROC(PostClose, hookPostClose, XorgScreenCloseProcPtr)
DECLARE_HOOK_PROC(PixmapDestroy, hookPixmapDestroy, XorgScreenPixmapDestroyProcPtr)
DECLARE_HOOK_PROC(PostCreateResources, hookPostCreateResources,
                  XorgScreenPostCreateResourcesProcPtr)

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

    CallCallbacks(&pScreen->hookPostClose, NULL);
}

void dixScreenRaisePixmapDestroy(PixmapPtr pPixmap)
{
    if (!pPixmap)
        return;

    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    CallCallbacks(&pScreen->hookPixmapDestroy, pPixmap);
    /* we must not call the original ScreenRec->DestroyPixmap() here */
}

Bool dixScreenRaiseCreateResources(ScreenPtr pScreen)
{
    if (!pScreen)
        return FALSE;

    if (pScreen->CreateScreenResources) {
        if (!pScreen->CreateScreenResources(pScreen))
            return FALSE;
    }

    Bool ret = TRUE;
    CallCallbacks(&pScreen->hookPostCreateResources, &ret);
    return ret;
}

Bool dixScreenRaiseRealizeWindow(WindowPtr pWin)
{
    if (!pWin)
        return FALSE;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    CallCallbacks(&pScreen->hookWindowRealize, pWin);

    return (pScreen->RealizeWindow ? pScreen->RealizeWindow(pWin) : TRUE);
}

void dixScreenRaiseUnrealizeWindow(WindowPtr pWin)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    pWin->realized = FALSE;
    CallCallbacks(&pScreen->hookWindowUnrealize, pWin);
    if (pScreen->UnrealizeWindow)
        pScreen->UnrealizeWindow(pWin);
}

void dixScreenRaiseMoveWindow(WindowPtr pWin, int x, int y, WindowPtr pSib,
                              VTKind kind)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    XorgScreenWindowMoveParamRec param = {
        .window = pWin,
        .x = x,
        .y = y,
        .sibling = pSib,
        .kind = kind,
    };

    CallCallbacks(&pScreen->hookWindowMove, &param);
    if (pScreen->MoveWindow)
        pScreen->MoveWindow(pWin, x, y, pSib, kind);
    CallCallbacks(&pScreen->hookPostWindowMove, &param);
}

void dixScreenRaiseResizeWindow(WindowPtr pWin, int x, int y,
                                unsigned int w, unsigned int h, WindowPtr pSib)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    XorgScreenWindowResizeParamRec param = {
        .window = pWin, .x = x, .y = y, .w = w, .h = h, .sibling = pSib,
    };

    CallCallbacks(&pScreen->hookWindowResize, &param);
    if (pScreen->ResizeWindow)
        pScreen->ResizeWindow(pWin, x, y, w, h, pSib);
    CallCallbacks(&pScreen->hookPostWindowResize, &param);
}

void dixScreenRaiseChangeBorderWidth(WindowPtr pWin, unsigned int bw)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    XorgScreenWindowChangeBorderWidthParamRec param = {
        .window = pWin, .borderWidth = bw,
    };

    CallCallbacks(&pScreen->hookWindowChangeBorderWidth, &param);
    if (pScreen->ChangeBorderWidth)
        pScreen->ChangeBorderWidth(pWin, bw);
    CallCallbacks(&pScreen->hookPostWindowChangeBorderWidth, &param);
}

void dixScreenRaiseReparentWindow(WindowPtr pWin, WindowPtr pPriorParent)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    XorgScreenWindowReparentParamRec param = {
        .window = pWin, .priorParent = pPriorParent,
    };

    CallCallbacks(&pScreen->hookWindowReparent, &param);
    if (pScreen->ReparentWindow)
        pScreen->ReparentWindow(pWin, pPriorParent);
    CallCallbacks(&pScreen->hookPostWindowReparent, &param);
}

Bool dixScreenRaiseDisplayCursor(ScreenPtr pScreen, DeviceIntPtr pDev, CursorPtr pCursor)
{
    /* for now just calling the screen proc, but in the future we'll also handle hide
       counters and animations here, so we don't need fragile proc wrapping anymore */
    if (pScreen && pScreen->DisplayCursor)
        return pScreen->DisplayCursor(pDev, pScreen, pCursor);
    return FALSE;
}
